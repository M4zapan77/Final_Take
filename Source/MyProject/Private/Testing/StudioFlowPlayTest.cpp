// Opt-in, separate-process integration driver. No console shortcut starts the film; it uses the real menu.
#if !UE_BUILD_SHIPPING
#include "StudioFlowManager.h"
#include "StudioFlowWidget.h"
#include "StudioAudioSubsystem.h"
#include "StudioAudioProfile.h"
#include "AudioMixerBlueprintLibrary.h"
#include "StudioGameMode.h"
#include "StudioCinemaPresenter.h"
#include "ActingComponent.h"
#include "ActingRecorderComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "MoviePresentationWidget.h"
#include "ObjectiveHUDWidget.h"
#include "ObjectiveManagerComponent.h"
#include "ObjectiveZone.h"
#include "InteractableProp.h"
#include "PropInteractionComponent.h"
#include "WardrobeWidget.h"
#include "UnifiedWardrobeWidget.h"
#include "StudioAnimInstance.h"
#include "WardrobeComponent.h"
#include "ReplayActor.h"
#include "Camera/CameraActor.h"
#include "Components/AudioComponent.h"
#include "Components/Slider.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundWaveProcedural.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "InputKeyEventArgs.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace StudioFlowPlayTest
{
    // Quiet synthetic tones exist only in this opt-in test. They are never game assets or shipped defaults.
    USoundWaveProcedural* Tone(UObject* Owner,float Frequency)
    {
        auto* Wave=NewObject<USoundWaveProcedural>(Owner); Wave->SetSampleRate(24000); Wave->NumChannels=1; Wave->Duration=10000.f;
        Wave->OnSoundWaveProceduralUnderflow.BindLambda([Frequency,Phase=0.0](USoundWaveProcedural* Sound,int32 Requested) mutable
        {
            TArray<int16> Samples; Samples.SetNumUninitialized(FMath::Clamp(Requested,256,16384));
            for (int16& Sample:Samples) { Sample=static_cast<int16>(FMath::Sin(Phase)*120.0); Phase+=2.0*PI*Frequency/24000.0; if (Phase>2.0*PI) { Phase-=2.0*PI; } }
            Sound->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()),Samples.Num()*sizeof(int16));
        });
        return Wave;
    }
    int32 ActiveActorAudio(AActor* Actor)
    {
        int32 Count=0; TArray<USceneComponent*> Components; if (Actor && Actor->GetRootComponent()) { Actor->GetRootComponent()->GetChildrenComponents(true,Components); }
        for (USceneComponent* Component:Components) { if (const auto* Audio=Cast<UAudioComponent>(Component); Audio && Audio->IsPlaying()) { ++Count; } }
        return Count;
    }
    class FRun
    {
    public:
        TWeakObjectPtr<UWorld> World;
        TWeakObjectPtr<AStudioFlowManager> Flow;
        TWeakObjectPtr<AMovieManager> Movie;
        TWeakObjectPtr<APlayerController> Player;
        TWeakObjectPtr<AMoneyBagProp> Money;
        FTimerHandle Timer;
        FString Directory;
        TArray<FKey> ReleaseKeys;
        TArray<FMoviePerformance> SavedFilm;
        FCosmeticSelection SelectedLooks[3];
        FMovieResults SavedResults;
        FTransform CameraPose;
        FVector Entry,Counter,Escape,Pickup,ProbeStart;
        double Started=FPlatformTime::Seconds(),StepStarted=0,PreparationStarted=0,ResultsStarted=-1;
        int32 MenuStep=0,Round=0,PreparationId=0,PreparationCount=0,ReadyCount=0,TimeoutCount=0;
        int32 LastTitle=0,LastPlayback=0,TitleCount=0,PlaybackCount=0,LastRevision=0,TransitionCount=0,LastCueSerial=0;
        int32 CueCounts[5]={},ActionStage[3]={};
        TSet<FString> Captured;
        bool bDone=false,bFast=false,bRender=false,bMenus=false,bIntro=false,bAudio=false,bDefaultTiming=false;
        bool bProbeActed=false,bRestartSent=false,bRestartVerified=false,bWardrobeInputSent=false,bWardrobeLocked=false,bWalkHeld=false,bWalkVerified=false;
        bool bReplayVerified=false,bNewMovieVerified=false,bMenuVerified=false,bReopened=false,bZeroVerified=false;
        bool bSawCountdown=false,bQuitClicked=false;
        bool bAudioProbe=false,bSuppliedAudio=false,bMusicPlayed=false,bCuePlayed=false,bSfxPlayed=false,bAudioCleanup=false;

        bool CheckLook(const FCosmeticSelection& Actual,const FCosmeticSelection& Expected)
        {
            return Check(Actual.EyeId==Expected.EyeId && Actual.MouthId==Expected.MouthId && Actual.EyebrowId==Expected.EyebrowId
                && Actual.SkinColor.Equals(Expected.SkinColor) && Actual.HeadId==Expected.HeadId && Actual.BodyId==Expected.BodyId
                && Actual.FaceId==Expected.FaceId && Actual.LeftHandPropId==Expected.LeftHandPropId,
                TEXT("Independent facial, skin, clothing, hat or left-hand appearance changed"));
        }

        AMovieManager* Manager() const { return Movie.Get(); }
        void ReleasePending()
        {
            if (Player.IsValid()) { for (const auto& Key:ReleaseKeys) { Player->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,Key,IE_Released,FPlatformTime::Cycles64())); } }
            ReleaseKeys.Reset();
        }
        void Press(FKey Key)
        {
            Player->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,Key,IE_Pressed,FPlatformTime::Cycles64())); ReleaseKeys.AddUnique(Key);
        }
        void Capture(const TCHAR* Name)
        {
            if (!bRender || Captured.Contains(Name) || FScreenshotRequest::IsScreenshotRequested()) { return; }
            Captured.Add(Name); IFileManager::Get().MakeDirectory(*Directory,true); FScreenshotRequest::RequestScreenshot(Directory/(FString(Name)+TEXT(".png")),true,false);
        }
        bool ScreenshotReady() const { return !bRender || !FScreenshotRequest::IsScreenshotRequested(); }
        void Report(bool bPass,const TCHAR* Message)
        {
            IFileManager::Get().MakeDirectory(*Directory,true);
            const auto Bool=[](bool Value){return Value?TEXT("true"):TEXT("false");};
            const FString Json=FString::Printf(TEXT("{\n  \"passed\": %s,\n  \"menuPages\": %s,\n  \"introduction\": %s,\n  \"audioPreferences\": %s,\n  \"initialDefaults10And30\": %s,\n  \"fullTiming\": %s,\n  \"wardrobes\": %d,\n  \"readyStarts\": %d,\n  \"timeoutStarts\": %d,\n  \"titleCards\": %d,\n  \"playbacks\": %d,\n  \"transitionCards\": %d,\n  \"directorCues\": [%d,%d,%d,%d,%d],\n  \"wardrobeInputLocked\": %s,\n  \"keyboardRestartCleared\": %s,\n  \"movementInput\": %s,\n  \"firstMovieScore\": %.6f,\n  \"stars\": %d,\n  \"replayPreserved\": %s,\n  \"newMovieCleared\": %s,\n  \"zeroMovieRated\": %s,\n  \"returnMenuCleared\": %s,\n  \"playAgain\": %s,\n  \"quitButton\": %s,\n  \"message\": \"%s\"\n}\n"),
                Bool(bPass),Bool(bMenus),Bool(bIntro),Bool(bAudio),Bool(bDefaultTiming),Bool(!bFast),PreparationCount,ReadyCount,TimeoutCount,TitleCount,PlaybackCount,TransitionCount,
                CueCounts[0],CueCounts[1],CueCounts[2],CueCounts[3],CueCounts[4],Bool(bWardrobeLocked),Bool(bRestartVerified),Bool(bWalkVerified),SavedResults.FinalScore,SavedResults.Stars,
                Bool(bReplayVerified),Bool(bNewMovieVerified),Bool(bZeroVerified),Bool(bMenuVerified),Bool(bReopened),Bool(bQuitClicked),Message);
            // Append probe observations without introducing a JSON module dependency.
            FString WithAudio=Json; WithAudio.RemoveFromEnd(TEXT("}\n"));
            WithAudio=WithAudio.TrimEnd(); WithAudio+=FString::Printf(TEXT(",\n  \"syntheticAudioProbe\": %s,\n  \"suppliedAudioProbe\": %s,\n  \"musicPlayback\": %s,\n  \"directorPlayback\": %s,\n  \"actingSoundPlayback\": %s,\n  \"audioCleanup\": %s\n}\n"),Bool(bAudioProbe && !bSuppliedAudio),Bool(bSuppliedAudio),Bool(bMusicPlayed),Bool(bCuePlayed),Bool(bSfxPlayed),Bool(bAudioCleanup));
            FFileHelper::SaveStringToFile(WithAudio,*(Directory/TEXT("result.json")));
        }
        void Fail(const TCHAR* Message)
        {
            if (bDone) { return; } bDone=true; ReleasePending(); Report(false,Message); World->GetTimerManager().ClearTimer(Timer);
            UE_LOG(LogTemp,Error,TEXT("StudioFlowPlay FAIL: %s"),Message); if (Flow.IsValid()) { Flow->ReturnToMainMenu(); }
            FPlatformMisc::RequestExitWithStatus(false,1);
        }
        bool Check(bool Condition,const TCHAR* Message) { if (!Condition) { Fail(Message); } return Condition; }
        bool Clean() const
        {
            for(TActorIterator<AStudioCinemaPresenter> I(World.Get());I;++I)
            {
                if(I->IsPresenting()){return false;}
            }
            const auto* Recorder=Player->GetPawn()->FindComponentByClass<UActingRecorderComponent>();
            return Movie->Performances.IsEmpty() && Movie->ActiveReplays.IsEmpty() && !Movie->MovieResults.bAvailable && Movie->ScoreSummary.RoleScores.IsEmpty()
                && Movie->GetCurrentScore()==0 && (!Recorder || (!Recorder->IsRecording() && Recorder->GetRecordingData().Samples.IsEmpty() && Recorder->GetRecordingData().Events.IsEmpty()));
        }
        void Setup(AStudioFlowManager* Studio)
        {
            Flow=Studio; Movie=Studio->Movie; Player=UGameplayStatics::GetPlayerController(World.Get(),0); bFast=FParse::Param(FCommandLine::Get(),TEXT("StudioFastTest"));
            bRender=FParse::Param(FCommandLine::Get(),TEXT("StudioScreenshots"));
            if (!Check(Player.IsValid() && Movie.IsValid() && Studio->ActiveWidget && Studio->Screen==EStudioScreen::MainMenu && !Movie->HasActiveSession(),TEXT("Normal startup did not show the main menu"))) { return; }
            bDefaultTiming=Movie->PreparationDurationSeconds==10 && Movie->RecordingDurationSeconds==30 && Movie->Roles.Num()==3;
            if (!Check(bDefaultTiming && Movie->PremiereCamera,TEXT("Saved movie defaults or fixed camera missing"))) { return; }
            CameraPose=Movie->PremiereCamera->GetActorTransform();
            bSuppliedAudio=FParse::Param(FCommandLine::Get(),TEXT("StudioSuppliedAudio"));
            bAudioProbe=bSuppliedAudio || FParse::Param(FCommandLine::Get(),TEXT("StudioAudioProbe"));
            if (bSuppliedAudio)
            {
                FApp::SetUnfocusedVolumeMultiplier(1.f);FApp::SetVolumeMultiplier(1.f);
                if(!Check(Movie->AudioProfile && Studio->Settings.MenuMusic && Studio->Settings.BankAmbience && Studio->Settings.CinemaAmbience,TEXT("Supplied audio profile was not loaded"))){return;}
                UAudioMixerBlueprintLibrary::StartRecordingOutput(World.Get(),bFast?90.f:300.f);
            }
            if (bAudioProbe && !bSuppliedAudio)
            {
                Studio->Settings.MenuMusic=Tone(Studio,220); Studio->Settings.IntroductionMusic=Studio->Settings.MenuMusic;
                Studio->Settings.WardrobeMusic=Tone(Studio,330);
                auto* Voice=Tone(Studio,660); for (auto& Cue:Studio->Settings.Announcements) { Cue.Sound=Voice; }
                Movie->PremiereSettings.PremiereMusic=Tone(Studio,440); Movie->PremiereSettings.ResultsMusic=Tone(Studio,550);
                APawn* Pawn=Player->GetPawn(); auto* Acting=Pawn->FindComponentByClass<UActingComponent>();
                if (!Acting) { Acting=NewObject<UActingComponent>(Pawn); Pawn->AddInstanceComponent(Acting); Acting->RegisterComponent(); }
                Acting->SetLiveActionsEnabled(false);
                for (int32 Index=0;Index<Acting->Sounds.Num();++Index) { Acting->Sounds[Index].Sound=Tone(Studio,800+Index*70); }
            }
            if (bFast)
            {
                Movie->PreparationDurationSeconds=2; Movie->RecordingDurationSeconds=5; Movie->BetweenActorsSeconds=.55; Movie->WrapSeconds=.55;
                Studio->Settings.NextActorCueDelay=.15; Movie->PremiereSettings.TitleFadeInSeconds=.05; Movie->PremiereSettings.TitleHoldSeconds=.15; Movie->PremiereSettings.TitleFadeOutSeconds=.1;
            }
            for (TActorIterator<AObjectiveZone> It(World.Get());It;++It)
            { if (It->ZoneTag==TEXT("Bank.Entry")) { Entry=It->GetActorLocation(); } else if (It->ZoneTag==TEXT("Bank.Counter")) { Counter=It->GetActorLocation(); } else if (It->ZoneTag==TEXT("Bank.Escape")) { Escape=It->GetActorLocation(); } }
            for (TActorIterator<AMoneyBagProp> It(World.Get());It;++It) { Money=*It; break; }
            if (!Check(Money.IsValid() && !Entry.IsNearlyZero() && !Counter.IsNearlyZero() && !Escape.IsNearlyZero(),TEXT("Bank objective setup missing"))) { return; }
            Pickup=Money->GetActorLocation()-FVector(120,0,0); Pickup.Z=Entry.Z;
            for (const TCHAR* Name:{TEXT("sg.ShadowQuality"),TEXT("sg.GlobalIlluminationQuality"),TEXT("sg.ReflectionQuality")})
            { if (auto* Value=IConsoleManager::Get().FindConsoleVariable(Name)) { Value->Set(0,ECVF_SetByCode); } }
            StepStarted=World->GetTimeSeconds();
        }
        void Menu();
        void Wardrobe();
        void Recording();
        void Premiere();
        void Results();
        void Tick();
    };

    void FRun::Menu()
    {
        auto* Widget=Flow->ActiveWidget.Get(); const double Age=World->GetTimeSeconds()-StepStarted;
        const auto* Clock=World->GetSubsystem<UActingSceneClockSubsystem>();
        if (!Check(Widget && Widget->IsInViewport() && Clock->GetSceneTimeSeconds()==0 && !Clock->IsClockRunning() && !Movie->HasActiveSession()
            && Player->IsMoveInputIgnored() && Player->IsLookInputIgnored() && Player->GetPawn()->IsHidden() && Player->bShowMouseCursor,TEXT("Menu advanced the timeline or lost input ownership"))) { return; }
        if (Round==3)
        {
            if (!bMenuVerified)
            {
                bMenuVerified=Clean() && Flow->DirectorVoice->Sound==nullptr && !Flow->DirectorVoice->IsPlaying();
                bAudioCleanup=ActiveActorAudio(Player->GetPawn())==0 && ActiveActorAudio(Movie.Get())==0 && !Flow->DirectorVoice->IsPlaying();
                const auto Levels=UStudioAudioSubsystem::Find(Flow.Get())->GetLevels(); bAudio&=Levels.Master==.5f && Levels.Music==.25f && Levels.Effects==.75f;
                if (!Check(bMenuVerified && bAudio,TEXT("Return to menu retained film/audio or lost preferences"))) { return; }
                Capture(TEXT("ReturnedMenu")); StepStarted=World->GetTimeSeconds(); return;
            }
            if (Age<.4 || !ScreenshotReady()) { return; }
            Widget->PlayButton->OnClicked.Broadcast(); return;
        }
        if (Age<.3 || !ScreenshotReady()) { return; }
        switch (MenuStep)
        {
        case 0: Capture(TEXT("MainMenu")); ++MenuStep; break;
        case 1: Widget->HowButton->OnClicked.Broadcast(); ++MenuStep; break;
        case 2:
            if (!Check(Flow->Screen==EStudioScreen::HowToPlay && Widget->ViewData.HowToPlay.ToString().Contains(TEXT("WASD")),TEXT("How to Play page missing"))) { return; }
            Capture(TEXT("HowToPlay")); ++MenuStep; break;
        case 3: Widget->BackButton->OnClicked.Broadcast(); Widget->CreditsButton->OnClicked.Broadcast(); ++MenuStep; break;
        case 4:
            if (!Check(Flow->Screen==EStudioScreen::Credits && !Widget->ViewData.Credits.IsEmpty(),TEXT("Credits page missing"))) { return; }
            Capture(TEXT("Credits")); ++MenuStep; break;
        case 5: Widget->BackButton->OnClicked.Broadcast(); Widget->AudioButton->OnClicked.Broadcast(); ++MenuStep; break;
        case 6:
            if (!Check(Flow->Screen==EStudioScreen::AudioSettings && Widget->MasterSlider && Widget->MusicSlider && Widget->EffectsSlider,TEXT("Audio page missing"))) { return; }
            Widget->MasterSlider->SetValue(.5f); Widget->MasterSlider->OnValueChanged.Broadcast(.5f);
            Widget->MusicSlider->SetValue(.25f); Widget->MusicSlider->OnValueChanged.Broadcast(.25f);
            Widget->EffectsSlider->SetValue(.75f); Widget->EffectsSlider->OnValueChanged.Broadcast(.75f); ++MenuStep; break;
        case 7:
            bAudio=FMath::IsNearlyEqual(UStudioAudioSubsystem::MusicMultiplier(Flow.Get()),.125f) && FMath::IsNearlyEqual(UStudioAudioSubsystem::EffectsMultiplier(Flow.Get()),.375f)
                && FMath::IsNearlyEqual(Flow->BackgroundMusic->VolumeMultiplier,.125f*.72f);
            if (!Check(bAudio,TEXT("Audio sliders reset another channel or did not update the mix"))) { return; }
            Capture(TEXT("AudioSettings")); ++MenuStep; break;
        case 8: Widget->BackButton->OnClicked.Broadcast(); bMenus=true; Widget->PlayButton->OnClicked.Broadcast(); break;
        }
        StepStarted=World->GetTimeSeconds();
    }

    void FRun::Wardrobe()
    {
        UWardrobeWidget* Widget=Movie->ActiveWardrobeWidget;
        if (!Check(Widget && Widget->IsInViewport() && Movie->GetSceneTimeSeconds()==0 && !Movie->LiveActingComponent->CanTriggerActions()
            && Player->IsMoveInputIgnored() && Player->IsLookInputIgnored() && Player->bShowMouseCursor,TEXT("Wardrobe clock, input or widget incorrect"))) { return; }
        if (PreparationId!=Movie->PreparationId)
        {
            PreparationId=Movie->PreparationId; ++PreparationCount; PreparationStarted=World->GetTimeSeconds();
            if (bRestartSent && !bRestartVerified) { bRestartVerified=Clean() && Movie->GetCurrentTakeNumber()==1; if (!Check(bRestartVerified,TEXT("R did not clear the active recording and score"))) { return; } }
            if (Round==0)
            {
                const int32 Index=Movie->CurrentTakeIndex;
                const FName Heads[]={TEXT("Head.RobberMask"),TEXT("Head.PoliceCap"),TEXT("Head.Chicken")};
                const FName Props[]={NAME_None,TEXT("Prop.Donut"),TEXT("Prop.Banana")};
                auto* Unified=Cast<UUnifiedWardrobeWidget>(Widget);
                if (!Check(Unified && Player->GetPawn()->FindComponentByClass<USkeletalMeshComponent>()->GetAnimClass()->IsChildOf(UStudioAnimInstance::StaticClass()),TEXT("Unified UI or Base3 locomotion Blueprint is not active"))) { return; }
                const ECustomizationTab Tabs[]={ECustomizationTab::Hats,ECustomizationTab::Clothing,ECustomizationTab::Objects};
                int32 TabIndex=0;
                for (ECosmeticCategory Category:{ECosmeticCategory::Head,ECosmeticCategory::Body,ECosmeticCategory::Prop})
                {
                    const FName Id=Category==ECosmeticCategory::Head?Heads[Index]:Category==ECosmeticCategory::Body?(Index==0?FName(TEXT("Body.Suit")):FName(TEXT("Body.Uniform"))):Props[Index];
                    Unified->SelectTab(Tabs[TabIndex++]);
                    const FName Control=Id.IsNone()?FName(TEXT("Item.None")):FName(*(FString(TEXT("Item."))+Id.ToString()));
                    auto* Choice=Unified->FindControl(Control);
                    if (!Check(Choice!=nullptr,TEXT("Unified cosmetic button missing"))) { return; }
                    Choice->OnClicked.Broadcast();
                }
                const FName Eyes[]={TEXT("Eyes.Classic"),TEXT("Eyes.Angry"),TEXT("Eyes.Happy")};
                const FName Mouths[]={TEXT("Mouth.Smile"),TEXT("Mouth.Open"),TEXT("Mouth.Laugh")};
                const FName Brows[]={TEXT("Brows.Neutral"),TEXT("Brows.Angry"),TEXT("Brows.Surprised")};
                const FName LeftProps[]={TEXT("Prop.Microphone"),TEXT("Prop.Banana"),TEXT("Prop.Donut")};
                const FLinearColor Skin[]={FLinearColor(.45,.22,.85),FLinearColor(.32,.65,1),FLinearColor(1,.35,.6)};
                if (!Check(Unified->ChooseItem(ECosmeticCategory::Eyes,Eyes[Index]) && Unified->ChooseItem(ECosmeticCategory::Mouth,Mouths[Index])
                    && Unified->ChooseItem(ECosmeticCategory::Eyebrows,Brows[Index]) && Unified->ChooseItem(ECosmeticCategory::LeftProp,LeftProps[Index])
                    && Unified->ChooseSkin(Skin[Index]),TEXT("Complete independent role appearance could not be selected"))) { return; }
                SelectedLooks[Index]=Movie->LiveWardrobeComponent->GetSelection();
                Unified->SelectTab(ECustomizationTab::Eyes);
            }
            else if (!Check(Movie->LiveWardrobeComponent->GetSelection().EyeId.IsNone() && Movie->LiveWardrobeComponent->GetSelection().MouthId.IsNone()
                && Movie->LiveWardrobeComponent->GetSelection().EyebrowId.IsNone() && Movie->LiveWardrobeComponent->GetSelection().LeftHandPropId.IsNone()
                && Movie->LiveWardrobeComponent->GetSelection().SkinColor.Equals(FLinearColor::White),TEXT("New movie retained a previous custom face, skin or left prop"))) { return; }
        }
        const double Age=World->GetTimeSeconds()-PreparationStarted;
        if (Round==0 && !bWardrobeInputSent)
        {
            ProbeStart=Player->GetPawn()->GetActorLocation(); Player->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,EKeys::W,IE_Pressed,FPlatformTime::Cycles64())); Press(EKeys::One); bWardrobeInputSent=true;
        }
        if (Round==0 && !bWardrobeLocked && Age>.25)
        {
            Player->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,EKeys::W,IE_Released,FPlatformTime::Cycles64()));
            bWardrobeLocked=FVector::DistSquared(ProbeStart,Player->GetPawn()->GetActorLocation())<1 && Movie->GetCurrentScore()==0;
            if (!Check(bWardrobeLocked,TEXT("Wardrobe accepted gameplay movement or actions"))) { return; }
        }
        if (Age>.4 && Widget->ViewData.RemainingSeconds<Widget->ViewData.DurationSeconds-.15) { bSawCountdown=true; }
        if (Round==0 && Movie->CurrentTakeIndex==0 && bRestartVerified && Age>.3) { Capture(TEXT("Wardrobe")); }
        if (Round==3)
        {
            bReopened=Clean() && Movie->GetCurrentTakeNumber()==1;
            bool bAllCues=true; for (int32 Count:CueCounts) { bAllCues&=Count>0; }
            if (!Check(bReopened && PreparationCount==8 && bSawCountdown && bAllCues && bMenus && bIntro && bRestartVerified && bReplayVerified && bNewMovieVerified && bZeroVerified && bMenuVerified && bAudio
                && (!bAudioProbe || (bMusicPlayed && bCuePlayed && bSfxPlayed && bAudioCleanup)),TEXT("Complete game loop checks were not reached"))) { return; }
            if (!Check(Flow->ReturnToMainMenu(),TEXT("Busy-film menu cleanup failed"))) { return; }
            ReleasePending(); bQuitClicked=true; bDone=true;
            if(bSuppliedAudio){UAudioMixerBlueprintLibrary::StopRecordingOutput(World.Get(),EAudioRecordingExportType::WavFile,TEXT("SuppliedAudioMix"),Directory);}
            Report(true,TEXT("Normal menu launch, intro/tutorial, input, 15 objectives, transitions/cues, premiere, immutable replay, clean restart/new movie/menu, play again and Quit passed"));
            World->GetTimerManager().ClearTimer(Timer); UE_LOG(LogTemp,Display,TEXT("StudioFlowPlay PASS; clicking the native Quit button"));
            FTimerHandle QuitWatchdog; World->GetTimerManager().SetTimer(QuitWatchdog,FTimerDelegate::CreateLambda([](){ FPlatformMisc::RequestExitWithStatus(false,1); }),2.f,false);
            Flow->ActiveWidget->QuitButton->OnClicked.Broadcast(); return;
        }
        if (Round==0 && Movie->CurrentTakeIndex==2) { if (Age>Movie->PreparationDurationSeconds-.1) { TimeoutCount=1; } return; }
        if (Age>.6 && ScreenshotReady())
        {
            Widget->ReadyButton->OnClicked.Broadcast(); ++ReadyCount;
            Check(Movie->State==EMovieState::Recording && !Widget->Ready() && !Movie->ReadyForTake(PreparationId),TEXT("Ready was not guarded exactly once"));
        }
    }

    void FRun::Recording()
    {
        const double Time=Movie->GetSceneTimeSeconds(); APawn* Pawn=Player->GetPawn();
        const auto* Recorder=Pawn->FindComponentByClass<UActingRecorderComponent>();
        // The recorder freezes the performer before MovieManager's late tick publishes the cut.
        if (!Recorder || !Recorder->IsRecording()) { return; }
        if (bAudioProbe && Time<.3 && Flow->DirectorVoice->IsPlaying()) { bCuePlayed=true; }
        if (bAudioProbe && ActiveActorAudio(Pawn)>0) { bSfxPlayed=true; }
        if (!Check(Flow->Screen==EStudioScreen::Recording && !Player->IsMoveInputIgnored() && !Player->IsLookInputIgnored() && !Player->bShowMouseCursor
            && !Pawn->IsHidden() && Movie->LiveActingComponent->CanTriggerActions(),TEXT("Action did not restore player control"))) { return; }
        if (!bRestartVerified)
        {
            if (!bProbeActed && Time>.1) { Press(EKeys::One); Press(EKeys::Z); bProbeActed=true; }
            if (!bRestartSent && Time>.35)
            {
                if (!Check(Pawn->FindComponentByClass<UActingRecorderComponent>()->GetRecordingData().Events.Num()==2 && Movie->GetCurrentScore()>0,TEXT("Probe did not record keyboard gesture and sound before R"))) { return; }
                Press(EKeys::R); bRestartSent=true;
            }
            return;
        }
        UObjectiveHUDWidget* HUD=Movie->ActiveObjectiveHUD;
        if (!Check(HUD && HUD->IsInViewport() && HUD->ViewData.bRecording && !HUD->ViewData.RoleName.IsEmpty()
            && HUD->ViewData.Controls.ToString().Contains(TEXT("Siren")) && HUD->ViewData.RemainingSeconds>0,TEXT("Recording HUD lacks role, controls or countdown"))) { return; }
        if (Round==2) { if (!Check(Movie->GetCurrentScore()==0,TEXT("Score leaked into the new film"))) { return; } return; }
        const int32 Index=Movie->CurrentTakeIndex;
        if (Index==0)
        {
            if (Time<2.2)
            {
                if (!bWalkHeld) { ProbeStart=Pawn->GetActorLocation(); Player->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,EKeys::W,IE_Pressed,FPlatformTime::Cycles64())); bWalkHeld=true; }
                return;
            }
            if (bWalkHeld)
            {
                Player->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,EKeys::W,IE_Released,FPlatformTime::Cycles64())); bWalkHeld=false;
                bWalkVerified=FVector::DistSquared2D(ProbeStart,Pawn->GetActorLocation())>100 && Movie->ObjectiveManager->IsObjectiveCompleted(TEXT("Robber.EnterBank"));
                if (!Check(bWalkVerified,TEXT("W movement did not enter the actual bank"))) { return; }
            }
            Pawn->SetActorLocation(Time<3.7?Pickup:Escape);
            if (Time>2.4 && ActionStage[0]==0) { Press(EKeys::E); ActionStage[0]=1; }
            else if (Time>2.65 && ActionStage[0]==1) { Press(EKeys::One); Press(EKeys::Z); ActionStage[0]=2; }
        }
        else if (Index==1)
        {
            Pawn->SetActorLocation(Time<.25 || Time>2.95?Entry:Pickup+FVector(0,50,0));
            if (Time>2.7 && ActionStage[1]==0) { Press(EKeys::Five); ActionStage[1]=1; }
            else if (Time>3.05 && ActionStage[1]==1) { Press(EKeys::E); Press(EKeys::V); ActionStage[1]=2; }
        }
        else
        {
            Pawn->SetActorLocation(Time<.35?Counter:Pickup+FVector(0,50,0));
            if (Time>2.7 && ActionStage[2]==0) { Press(EKeys::Three); ActionStage[2]=1; }
            else if (Time>2.95 && ActionStage[2]==1) { Press(EKeys::One); Press(EKeys::X); ActionStage[2]=2; }
        }
        if (Time>4.1)
        {
            if (!Check(Movie->GetCurrentScore()==100 && ActionStage[Index]==2,TEXT("Normal E/gesture/sound controls did not complete this role's objectives"))) { return; }
            if (Index==0) { Capture(TEXT("RecordingHUD")); }
            if (Index>0)
            {
                for (int32 Prior=0;Prior<Index;++Prior)
                {
                    const AReplayActor* Replay=Movie->ActiveReplays[Prior];
                    // Timer callbacks precede TG_PostUpdateWork. Compare the committed
                    // cursor with the actor's evaluated time, then bound its age.
                    if (!Check(!Replay->GetActorEnableCollision() && Replay->Acting->DispatchedEventCount==Movie->Performances[Prior].Recording.Events.FilterByPredicate([&](const auto& Event){return Event.TimeSeconds<=Replay->GetEvaluatedTakeTimeSeconds();}).Num()
                        && Movie->GetSceneTimeSeconds()-Replay->GetEvaluatedTakeTimeSeconds()<=FMath::Max(.15,static_cast<double>(World->GetDeltaSeconds())*2)
                        && Movie->Performances[Prior].ObjectiveResult.Score==100,TEXT("Concurrent replay events or retained score incorrect"))) { return; }
                    if (!CheckLook(Replay->Wardrobe->GetSelection(),SelectedLooks[Prior])) { return; }
                }
            }
        }
    }

    void FRun::Premiere()
    {
        if (!Check(Flow->Screen==EStudioScreen::Premiere && Player->IsMoveInputIgnored() && Player->IsLookInputIgnored() && Player->GetPawn()->IsHidden()
            && (!Movie->ActiveObjectiveHUD || !Movie->ActiveObjectiveHUD->IsInViewport()) && Movie->GetPremiereCamera()==Movie->PremiereCamera
            && Movie->PremiereCamera->GetActorTransform().Equals(CameraPose),TEXT("Premiere input, HUD or existing camera incorrect"))) { return; }
        for(TActorIterator<AStudioCinemaPresenter> I(World.Get());I;++I)
        {
            if(!Check(I->IsPresenting() && I->GetFilmCamera()==Movie->GetPremiereCamera()
                && Player->GetViewTarget()==I->GetAudienceCamera() && I->ScreenTexture,
                TEXT("Cinema failed to present the existing synchronized movie camera"))){return;}
            if(Movie->PresentationPhase==EMoviePresentationPhase::Playback && Movie->GetSceneTimeSeconds()>1
                && !Check(I->CapturedFrames>0,TEXT("Theater screen did not capture movie frames"))){return;}
        }
        if (Movie->PresentationPhase==EMoviePresentationPhase::TitleCard)
        {
            if (LastTitle!=Movie->PresentationId) { LastTitle=Movie->PresentationId; ++TitleCount; }
            Check(Movie->GetSceneTimeSeconds()==0,TEXT("Title advanced the recording clock"));
            if (Round==0 && Movie->GetPhaseElapsedSeconds()>.075) { Capture(TEXT("TitleCard")); }
        }
        else if (Movie->PresentationPhase==EMoviePresentationPhase::Playback)
        {
            if (LastPlayback!=Movie->PresentationId) { LastPlayback=Movie->PresentationId; ++PlaybackCount; }
            if (Movie->GetSceneTimeSeconds()>4.1)
            {
                for (int32 Index=0;Index<3;++Index)
                {
                    const AReplayActor* Replay=Movie->ActiveReplays[Index]; const auto& Performance=Movie->Performances[Index];
                    if (!Check(!Replay->GetActorEnableCollision() && Replay->Acting->DispatchedEventCount==Performance.Recording.Events.FilterByPredicate([&](const auto& Event){return Event.TimeSeconds<=Replay->GetEvaluatedTakeTimeSeconds();}).Num(),TEXT("Premiere dispatched duplicate or missing events"))) { return; }
                    if (!CheckLook(Replay->Wardrobe->GetSelection(),Performance.Recording.Appearance.Selection)) { return; }
                    if (!Check(Replay->PerformanceMesh->GetSkeletalMeshAsset()->GetPathName().Contains(TEXT("/Base3/SK_Base3."))
                        && Replay->PerformanceMesh->GetAnimClass()->IsChildOf(UStudioAnimInstance::StaticClass()),TEXT("Premiere used the placeholder model or lost locomotion"))) { return; }
                    for (int32 Other=0;Other<Index;++Other)
                    {
                        if (!Check(Replay->PerformanceMesh->GetMaterial(0)!=Movie->ActiveReplays[Other]->PerformanceMesh->GetMaterial(0),TEXT("Premiere actors share mutable skin materials"))) { return; }
                    }
                    if (Round<2 && !Check(Performance.ObjectiveResult.Score==100 && Performance.ObjectiveResult.CompletedObjectiveIds.Num()==5 && Performance.Recording.Events.FilterByPredicate([](const auto& Event){return Event.Type!=EActingEventType::Footstep;}).Num()==3,TEXT("Original film lost its fifteen objectives or input events"))) { return; }
                }
                if (Round==0) { Capture(TEXT("Playback")); }
            }
        }
    }

    void FRun::Results()
    {
        auto* Widget=Movie->ActiveMoviePresentation.Get(); if (!Check(Widget && Widget->IsInViewport() && Movie->IsResultsOpen() && Widget->MainMenuButton
            && Widget->MainMenuButton->GetVisibility()!=ESlateVisibility::Collapsed && Player->bShowMouseCursor,TEXT("Results/main-menu controls missing"))) { return; }
        if (ResultsStarted<0) { ResultsStarted=World->GetTimeSeconds(); }
        const double Age=World->GetTimeSeconds()-ResultsStarted;
        if (Age<.35) { return; }
        for(int32 Index=0;Index<Movie->ActiveReplays.Num();++Index)
        {
            if(!Check(Movie->ActiveReplays[Index]->Acting->DispatchedEventCount==Movie->Performances[Index].Recording.Events.Num(),TEXT("Premiere endpoint lost or duplicated recorded events"))){return;}
        }
        if (Round==0)
        {
            SavedFilm=Movie->Performances; SavedResults=Movie->MovieResults;
            if (!Check(SavedResults.FinalScore==100 && SavedResults.Stars==5 && SavedResults.Roles.Num()==3,TEXT("First complete film was not accurately rated"))) { return; }
            if (bRender && !Captured.Contains(TEXT("Results"))) { Capture(TEXT("Results")); return; }
            if (!ScreenshotReady()) { return; }
            Round=1; ResultsStarted=-1; Widget->ReplayButton->OnClicked.Broadcast();
            Check(!Widget->ReplayMovie() && Movie->State==EMovieState::MoviePremiere,TEXT("Replay accepted a duplicate or failed"));
        }
        else if (Round==1)
        {
            bReplayVerified=Movie->Performances.Num()==SavedFilm.Num() && FMovieResults::StaticStruct()->CompareScriptStruct(&SavedResults,&Movie->MovieResults,0);
            if (bReplayVerified) { for (int32 Index=0;Index<SavedFilm.Num();++Index) { bReplayVerified&=FMoviePerformance::StaticStruct()->CompareScriptStruct(&SavedFilm[Index],&Movie->Performances[Index],0); } }
            if (!Check(bReplayVerified,TEXT("Replay changed the original recordings, scores or results"))) { return; }
            Round=2; ResultsStarted=-1; Widget->NewMovieButton->OnClicked.Broadcast();
            bNewMovieVerified=Clean() && Movie->IsWardrobeOpen() && Movie->GetCurrentTakeNumber()==1;
            Check(bNewMovieVerified && !Widget->RecordNewMovie(),TEXT("New Movie did not clear data or allowed stale UI"));
        }
        else
        {
            bZeroVerified=Movie->MovieResults.FinalScore==0 && Movie->MovieResults.Stars==1 && !Movie->MovieResults.bAllObjectivesCompleted;
            for (const auto& Performance:Movie->Performances) { bZeroVerified&=Performance.Recording.Events.IsEmpty() && Performance.ObjectiveResult.CompletedObjectiveIds.IsEmpty(); }
            if (!Check(bZeroVerified,TEXT("Zero-objective movie could not be watched and rated"))) { return; }
            if (bRender && !Captured.Contains(TEXT("ZeroResults"))) { Capture(TEXT("ZeroResults")); return; }
            if (!ScreenshotReady()) { return; }
            Round=3; ResultsStarted=-1; Widget->MainMenuButton->OnClicked.Broadcast(); StepStarted=World->GetTimeSeconds();
            Check(Flow->Screen==EStudioScreen::MainMenu && !Widget->ReturnToMainMenu(),TEXT("Results Return to Main Menu failed or accepted stale UI"));
        }
    }

    void FRun::Tick()
    {
        if (bDone || !World.IsValid()) { return; }
        if (FPlatformTime::Seconds()-Started>600) { Fail(TEXT("Studio loop timed out")); return; }
        if (!Flow.IsValid())
        { if (auto* Studio=AStudioFlowManager::Find(World.Get())) { Setup(Studio); } return; }
        ReleasePending();
        if (!Check(Movie.IsValid() && Player.IsValid() && Player->GetPawn() && Movie->State!=EMovieState::Error && Flow->Screen!=EStudioScreen::Error,TEXT("Studio flow lost the player or entered Error"))) { return; }
        if (Flow->AnnouncementSerial!=LastCueSerial)
        { LastCueSerial=Flow->AnnouncementSerial; const int32 Index=static_cast<int32>(Flow->LastAnnouncement); if (Index<5) { ++CueCounts[Index]; } }
        if (bAudioProbe && Flow->BackgroundMusic->IsPlaying()) { bMusicPlayed=true; }
        if (Movie->State!=EMovieState::Finished) { ResultsStarted=-1; }
        if (Flow->IsFrontEndOpen() && Flow->Screen!=EStudioScreen::MovieIntroduction) { Menu(); return; }
        if (Flow->Screen==EStudioScreen::MovieIntroduction)
        {
            const auto View=Flow->GetView(); const double Age=World->GetTimeSeconds()-StepStarted;
            bIntro=View.Roles.Num()==3 && !View.MovieTitle.IsEmpty() && !View.Synopsis.IsEmpty() && Movie->GetSceneTimeSeconds()==0 && !Movie->HasActiveSession();
            if (!Check(bIntro && !Flow->ActiveWidget->IntroductionControls.IsEmpty(),TEXT("Introduction is missing roles, script or tutorial"))) { return; }
            if (bRender && Round==0 && Age>.3 && !Captured.Contains(TEXT("Introduction"))) { Capture(TEXT("Introduction")); return; }
            if (Age>.7 && ScreenshotReady()) { Flow->ActiveWidget->StartButton->OnClicked.Broadcast(); Check(Movie->IsWardrobeOpen() && !Flow->ActiveWidget->BeginMovie(),TEXT("Introduction failed or started twice")); }
            return;
        }
        if (Movie->IsWardrobeOpen()) { Wardrobe(); }
        else if (Movie->State==EMovieState::Recording) { Recording(); }
        else if (Movie->State==EMovieState::TakeTransition)
        {
            if (LastRevision!=Flow->Revision) { LastRevision=Flow->Revision; ++TransitionCount; }
            if (!Check(Movie->GetSceneTimeSeconds()==0 && !Movie->LiveActingComponent->CanTriggerActions() && Player->IsMoveInputIgnored() && Player->IsLookInputIgnored(),TEXT("Transition clock or input was not frozen"))) { return; }
            if (bAudioProbe)
            {
                if (!Check(ActiveActorAudio(Player->GetPawn())==0,TEXT("Live sound continued after Cut"))) { return; }
                for (AReplayActor* Replay:Movie->ActiveReplays) { if (!Check(ActiveActorAudio(Replay)==0,TEXT("Replay sound continued into the next-character card"))) { return; } }
            }
            if (Round==0 && Movie->GetCurrentTakeNumber()==2 && Movie->GetPhaseElapsedSeconds()>.25) { Capture(TEXT("NextActor")); }
        }
        else if (Movie->State==EMovieState::MoviePremiere) { Premiere(); }
        else if (Movie->State==EMovieState::Finished) { Results(); }
    }

    void OnWorld(UWorld* World,const UWorld::InitializationValues Values)
    {
        if (!World || !World->IsGameWorld() || !FParse::Param(FCommandLine::Get(),TEXT("game")) || !FParse::Param(FCommandLine::Get(),TEXT("StudioSmoke"))) { return; }
        auto Run=MakeShared<FRun>(); Run->World=World; Run->Directory=FPaths::ProjectSavedDir()/TEXT("Automation/StudioPlay");
        FParse::Value(FCommandLine::Get(),TEXT("StudioReportDir="),Run->Directory);
        World->GetTimerManager().SetTimer(Run->Timer,FTimerDelegate::CreateLambda([Run]()
        {
            // Tick may clear its own timer before clicking Quit. Keep its state alive until it returns.
            const TSharedRef<FRun> KeepAlive=Run;
            KeepAlive->Tick();
        }),.05f,true);
    }
    struct FRegistration
    {
        FDelegateHandle Handle;
        FRegistration() { Handle=FWorldDelegates::OnPostWorldInitialization.AddStatic(&OnWorld); }
        ~FRegistration() { FWorldDelegates::OnPostWorldInitialization.Remove(Handle); }
    } Registration;
}
#endif
