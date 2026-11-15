// Explicit separate-process smoke test. No normal Play or Shipping session executes this driver.
#if !UE_BUILD_SHIPPING
#include "MovieManager.h"
#include "MoviePresentationWidget.h"
#include "ObjectiveHUDWidget.h"
#include "ObjectiveManagerComponent.h"
#include "WardrobeWidget.h"
#include "WardrobeComponent.h"
#include "ActingComponent.h"
#include "ActingRecorderComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "PropInteractionComponent.h"
#include "InteractableProp.h"
#include "ReplayActor.h"
#include "StudioGameMode.h"
#include "Camera/CameraActor.h"
#include "Components/Button.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace PremierePlayTest
{
    class FRun
    {
    public:
        TWeakObjectPtr<UWorld> World;
        TWeakObjectPtr<AMovieManager> Manager;
        TWeakObjectPtr<AMoneyBagProp> Money;
        TWeakObjectPtr<ACameraActor> Camera;
        FTransform Origin,CameraPose;
        FVector InputProbeStart;
        FTimerHandle Timer;
        double Started=FPlatformTime::Seconds(), PreparationStarted=0, ResultsStarted=-1;
        double InputProbeTime=-1;
        int32 PreparationId=0, PreparationCount=0, LastTitleId=0, LastPlaybackId=0, TitleCount=0, PlaybackCount=0, Round=0;
        bool Acted[3]={};
        bool Captured[5]={};
        bool bDone=false, bRender=false, bReplayed=false, bReplayPreserved=false, bNewMovieCleared=false, bZeroRated=false, bBootstrap=false;
        bool bMovementInputVerified=false;
        TArray<FMoviePerformance> SavedFilm;
        FMovieResults SavedResults;
        FString Directory;

        FVector At(FVector Offset) const { return Origin.TransformPositionNoScale(Offset); }
        void Finish(bool bPass,const TCHAR* Message)
        {
            if (bDone) { return; } bDone=true;
            IFileManager::Get().MakeDirectory(*Directory,true);
            const FString Json=FString::Printf(TEXT("{\n  \"passed\": %s,\n  \"preparations\": %d,\n  \"titleCards\": %d,\n  \"playbacks\": %d,\n  \"firstMovieScore\": %.6f,\n  \"stars\": %d,\n  \"replayPreserved\": %s,\n  \"newMovieCleared\": %s,\n  \"zeroMovieRated\": %s,\n  \"nativeGameModeStarted\": %s,\n  \"movementInputVerified\": %s,\n  \"message\": \"%s\"\n}\n"),
                bPass?TEXT("true"):TEXT("false"),PreparationCount,TitleCount,PlaybackCount,SavedResults.FinalScore,SavedResults.Stars,
                bReplayPreserved?TEXT("true"):TEXT("false"),bNewMovieCleared?TEXT("true"):TEXT("false"),bZeroRated?TEXT("true"):TEXT("false"),bBootstrap?TEXT("true"):TEXT("false"),bMovementInputVerified?TEXT("true"):TEXT("false"),Message);
            FFileHelper::SaveStringToFile(Json,*(Directory/TEXT("result.json")));
            if (World.IsValid()) { World->GetTimerManager().ClearTimer(Timer); }
            if (Manager.IsValid()) { Manager->StopMovie(); }
            UE_LOG(LogTemp,Display,TEXT("PremierePlay: %s - %s"),bPass?TEXT("PASS"):TEXT("FAIL"),Message);
            FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
        }
        bool Check(bool bValue,const TCHAR* Message) { if (!bValue) { Finish(false,Message); } return bValue; }
        void Capture(int32 Index,const TCHAR* Name)
        {
            if (!bRender || Captured[Index]) { return; } Captured[Index]=true;
            IFileManager::Get().MakeDirectory(*Directory,true); FScreenshotRequest::RequestScreenshot(Directory/(FString(Name)+TEXT(".png")),true,false);
        }
        bool SameFilm() const
        {
            if (!Manager.IsValid() || SavedFilm.Num()!=Manager->Performances.Num()) { return false; }
            for (int32 Index=0;Index<SavedFilm.Num();++Index)
            { if (!FMoviePerformance::StaticStruct()->CompareScriptStruct(&SavedFilm[Index],&Manager->Performances[Index],0)) { return false; } }
            return FMovieResults::StaticStruct()->CompareScriptStruct(&SavedResults,&Manager->MovieResults,0);
        }
        void Setup(APawn* Pawn)
        {
            AMovieManager* Movie=nullptr;
            for (TActorIterator<AMovieManager> It(World.Get());It;++It)
            { bBootstrap|=It->HasActiveSession(); if (!Movie) { Movie=*It; } It->StopMovie(); }
            bBootstrap &= World->GetAuthGameMode<AStudioGameMode>()!=nullptr;
            if (!Check(bBootstrap,TEXT("StudioGameMode did not start the existing character/movie flow"))) { return; }
            Origin=Pawn->GetActorTransform();
            if (!Movie) { Movie=World->SpawnActor<AMovieManager>(); }
            Manager=Movie;
            FActorSpawnParameters Spawn; Spawn.ObjectFlags|=RF_Transient; Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Money=World->SpawnActor<AMoneyBagProp>(At(FVector(180,-180,-40)),Origin.Rotator(),Spawn);
            Camera=IsValid(Movie->PremiereCamera)?Movie->PremiereCamera.Get():World->SpawnActor<ACameraActor>(At(FVector(-850,-600,450)),(At(FVector(200,0,0))-At(FVector(-850,-600,450))).Rotation(),Spawn);
            if (!Check(Camera.IsValid(),TEXT("Could not prepare assigned camera fixture"))) { return; }
            CameraPose=Camera->GetActorTransform(); Movie->PremiereCamera=Camera.Get();
            Movie->PreparationDurationSeconds=2.0; Movie->RecordingDurationSeconds=2.0;
            Movie->PremiereSettings.TitleFadeInSeconds=.15; Movie->PremiereSettings.TitleHoldSeconds=.6; Movie->PremiereSettings.TitleFadeOutSeconds=.25; Movie->PremiereSettings.ResultsFadeInSeconds=.15;
            Movie->bShowDebugOverlay=false; Movie->bBindActingKeys=Movie->bBindRestartKey=Movie->bBindInteractionKey=false;
            Movie->CosmeticCatalog=WardrobeDefaults::MakeCatalog(); Movie->bUsePerformerCosmeticCatalog=false; Movie->MainMenuMap.Reset();
            const int32 DancePoints[]={50,50,40},SoundPoints[]={50,25,10};
            for (int32 Index=0;Index<3;++Index)
            {
                FMovieObjectiveDefinition Dance; Dance.Id=FName(*FString::Printf(TEXT("Take%d.Dance"),Index)); Dance.DisplayName=FText::FromString(TEXT("Perform Dance")); Dance.Type=EMovieObjectiveType::PerformGesture; Dance.RequiredEventId=TEXT("Dance"); Dance.Points=DancePoints[Index];
                FMovieObjectiveDefinition Sound; Sound.Id=FName(*FString::Printf(TEXT("Take%d.Sound"),Index)); Sound.DisplayName=FText::FromString(TEXT("Perform the role's sound")); Sound.Type=EMovieObjectiveType::PlaySound; Sound.RequiredEventId=ActingIds::Sound(static_cast<EActingSound>(Index)); Sound.Points=SoundPoints[Index];
                Movie->Roles[Index].Objectives={Dance,Sound};
            }
            bRender=FParse::Param(FCommandLine::Get(),TEXT("PremiereSmokeScreenshots"));
            Check(Movie->StartMovie(),TEXT("Could not start premiere smoke movie"));
        }
        void Wardrobe(APawn* Pawn);
        void Recording(APawn* Pawn);
        void Premiere(APawn* Pawn);
        void Results();
        void Tick();
    };

    void FRun::Wardrobe(APawn* Pawn)
    {
        auto* Movie=Manager.Get(); UWardrobeWidget* Widget=Movie->ActiveWardrobeWidget;
        if (!Check(Widget && Widget->IsInViewport() && Movie->ActiveObjectiveHUD && Movie->ActiveObjectiveHUD->IsInViewport(),TEXT("Wardrobe/objective UMG missing"))) { return; }
        if (!Check(Movie->GetSceneTimeSeconds()==0 && !Movie->LiveActingComponent->CanTriggerActions(),TEXT("Wardrobe advanced the scene or enabled actions"))) { return; }
        if (PreparationId!=Movie->PreparationId)
        {
            PreparationId=Movie->PreparationId; ++PreparationCount; PreparationStarted=World->GetTimeSeconds();
            if (Round==0)
            {
                const int32 Index=Movie->CurrentTakeIndex;
                const FName Heads[]={TEXT("Head.RobberMask"),TEXT("Head.PoliceCap"),TEXT("Head.Chicken")};
                const FName Bodies[]={TEXT("Body.Suit"),TEXT("Body.Uniform"),TEXT("Body.Uniform")};
                const FName Props[]={NAME_None,TEXT("Prop.Donut"),TEXT("Prop.Banana")};
                for (ECosmeticCategory Category:{ECosmeticCategory::Head,ECosmeticCategory::Body,ECosmeticCategory::Prop})
                {
                    const FName Id=Category==ECosmeticCategory::Head?Heads[Index]:Category==ECosmeticCategory::Body?Bodies[Index]:Props[Index];
                    bool bFound=false;
                    for (UWardrobeChoiceButton* Choice:Widget->ChoiceButtons)
                    { if (!Choice->bColorChoice && Choice->Category==Category && Choice->CosmeticId==Id) { Choice->OnClicked.Broadcast(); bFound=true; break; } }
                    if (!Check(bFound,TEXT("Native wardrobe choice button missing"))) { return; }
                }
            }
        }
        const double Age=World->GetTimeSeconds()-PreparationStarted;
        if (Round==0 && Movie->CurrentTakeIndex==0 && Age>.15) { Capture(0,TEXT("Wardrobe")); }
        if (Age>.35 && (!bRender || !FScreenshotRequest::IsScreenshotRequested()))
        {
            Widget->ReadyButton->OnClicked.Broadcast();
            Check(Movie->State==EMovieState::Recording && !Movie->ReadyForTake(PreparationId),TEXT("Ready did not start exactly once"));
        }
    }

    void FRun::Recording(APawn* Pawn)
    {
        auto* Movie=Manager.Get();
        const auto* Recorder=Pawn->FindComponentByClass<UActingRecorderComponent>();
        if (!Recorder || !Recorder->IsRecording()) { return; }
        const int32 Index=Movie->CurrentTakeIndex; const double Time=Movie->GetSceneTimeSeconds();
        if (Round==0 && Index==0 && !bMovementInputVerified)
        {
            auto* Player=UGameplayStatics::GetPlayerController(World.Get(),0);
            const auto Device=INPUTDEVICEID_NONE;
            if (InputProbeTime<0)
            {
                InputProbeTime=Time; InputProbeStart=Pawn->GetActorLocation();
                Player->InputKey(FInputKeyEventArgs(nullptr,Device,EKeys::W,IE_Pressed,FPlatformTime::Cycles64()));
            }
            if (Time-InputProbeTime<.2) { return; }
            Player->InputKey(FInputKeyEventArgs(nullptr,Device,EKeys::W,IE_Released,FPlatformTime::Cycles64()));
            if (!Check(FVector::DistSquared2D(InputProbeStart,Pawn->GetActorLocation())>1.0,TEXT("Existing character/controller did not move through the W input path"))) { return; }
            bMovementInputVerified=true;
        }
        Pawn->SetActorLocation(At(FVector(160+Time*50,(Index-1)*180,0)));
        if (Round==0 && !Acted[Index] && Time>.25)
        {
            if (Index==0 && !Check(Movie->LivePropInteraction->CollectProp(Money.Get()),TEXT("Recorded money pickup failed"))) { return; }
            if (Index==1 && !Check(Movie->LivePropInteraction->UseEquippedProp(),TEXT("Recorded donut use failed"))) { return; }
            for (int32 Count=0;Count<(Index==0?1:2);++Count) { Movie->LiveActingComponent->TriggerGesture(EActingGesture::Dance); }
            Movie->LiveActingComponent->TriggerSound(static_cast<EActingSound>(Index));
            Movie->LiveActingComponent->TriggerSound(Index==0?EActingSound::EvilLaugh:Index==1?EActingSound::PoliceSiren:EActingSound::Failure);
            Acted[Index]=true;
            const int32 Expected[]={100,75,50};
            Check(Movie->GetCurrentScore()==Expected[Index],TEXT("Role did not earn its configured score"));
        }
        if (Round==1) { Check(Movie->GetCurrentScore()==0,TEXT("Old score leaked into the new movie")); }
    }

    void FRun::Premiere(APawn* Pawn)
    {
        auto* Movie=Manager.Get(); auto* Player=UGameplayStatics::GetPlayerController(World.Get(),0);
        if (!Check(Movie->ActiveMoviePresentation && Movie->ActiveMoviePresentation->IsInViewport(),TEXT("Movie presentation UMG missing"))) { return; }
        if (!Check(Pawn->IsHidden() && Player->IsMoveInputIgnored() && Player->IsLookInputIgnored()
            && Player->GetViewTarget()==Camera.Get() && Camera->GetActorTransform().Equals(CameraPose),TEXT("Premiere camera or live-player lock changed"))) { return; }
        if (!Check((!Movie->ActiveObjectiveHUD || !Movie->ActiveObjectiveHUD->IsInViewport())
            && (!Player->GetHUD() || !Player->GetHUD()->bShowHUD),TEXT("Gameplay HUD visible during premiere"))) { return; }
        if (Movie->PresentationPhase==EMoviePresentationPhase::TitleCard)
        {
            if (LastTitleId!=Movie->PresentationId) { LastTitleId=Movie->PresentationId; ++TitleCount; }
            if (!Check(Movie->GetSceneTimeSeconds()==0 && !World->GetSubsystem<UActingSceneClockSubsystem>()->IsClockRunning(),TEXT("Title card advanced shared scene clock"))) { return; }
            for (AReplayActor* Replay:Movie->ActiveReplays) { if (!Check(Replay->Acting->DispatchedEventCount==0,TEXT("Title dispatched a recorded event"))) { return; } }
            if (Round==0 && Movie->GetPhaseElapsedSeconds()>.25) { Capture(1,TEXT("TitleCard")); }
        }
        else if (Movie->PresentationPhase==EMoviePresentationPhase::Playback)
        {
            if (LastPlaybackId!=Movie->PresentationId) { LastPlaybackId=Movie->PresentationId; ++PlaybackCount; }
            if (!Check(Movie->ActiveReplays.Num()==3,TEXT("Premiere lost an actor"))) { return; }
            if (Movie->GetSceneTimeSeconds()>.7)
            {
                for (int32 Index=0;Index<3;++Index)
                {
                    const AReplayActor* Replay=Movie->ActiveReplays[Index];
                    if (!Check(!Replay->GetActorEnableCollision() && Replay->Acting->DispatchedEventCount==Movie->Performances[Index].Recording.Events.Num(),TEXT("Premiere event dispatch or replay collision failed"))) { return; }
                    if (Round==0)
                    {
                        const FName ExpectedProp=Index==0?TEXT("Prop.MoneyBag"):Index==1?TEXT("Prop.Donut"):TEXT("Prop.Banana");
                        if (!Check(Replay->PropInteraction->EquippedPropId==ExpectedProp,TEXT("Premiere prop snapshot was not reproduced"))) { return; }
                    }
                }
                if (Round==0) { Capture(2,TEXT("Playback")); }
            }
        }
    }

    void FRun::Results()
    {
        auto* Movie=Manager.Get(); UMoviePresentationWidget* Widget=Movie->ActiveMoviePresentation;
        auto* Player=UGameplayStatics::GetPlayerController(World.Get(),0);
        if (!Check(Movie->IsResultsOpen() && Widget && Widget->IsInViewport() && Player->bShowMouseCursor
            && Widget->ViewData.Phase==EMoviePresentationPhase::Results,TEXT("Automatic results UI/input missing"))) { return; }
        if (!Check(!Widget->ViewData.bCanReturnToMenu && Widget->MainMenuButton->GetVisibility()==ESlateVisibility::Collapsed,TEXT("Unavailable main menu button was shown"))) { return; }
        if (ResultsStarted<0) { ResultsStarted=World->GetTimeSeconds(); }
        const double Age=World->GetTimeSeconds()-ResultsStarted;
        if (Round==0)
        {
            if (!Check(Movie->MovieResults.FinalScore==75.0 && Movie->MovieResults.Stars==4 && Movie->MovieResults.Roles.Num()==3,TEXT("Result score or rating incorrect"))) { return; }
            const int32 Expected[]={100,75,50};
            for (int32 Index=0;Index<3;++Index)
            { if (!Check(Movie->MovieResults.Roles[Index].Score==Expected[Index] && Movie->MovieResults.Roles[Index].CompletedObjectiveIds.Num()==2,TEXT("Individual score or completed objective list incorrect"))) { return; } }
            if (!Check(Movie->MovieResults.Awards.Num()==5 && Movie->MovieResults.bAllObjectivesCompleted,TEXT("Award conditions were not represented"))) { return; }
            for (const auto& Award:Movie->MovieResults.Awards)
            {
                const int32 Winners=Award.Type==EMovieAwardType::DirectorsCut?3:(Award.Type==EMovieAwardType::DancingLegend || Award.Type==EMovieAwardType::SoundChaos)?2:1;
                if (!Check(Award.WinnerRoleIds.Num()==Winners,TEXT("Award tie handling was incorrect"))) { return; }
            }
            if (Age>.25) { Capture(3,TEXT("Results")); }
            if (Age<.65 || (bRender && FScreenshotRequest::IsScreenshotRequested())) { return; }
            if (!bReplayed)
            {
                SavedFilm=Movie->Performances; SavedResults=Movie->MovieResults; bReplayed=true;
                Widget->ReplayButton->OnClicked.Broadcast();
                if (!Check(Movie->State==EMovieState::MoviePremiere && !Widget->ReplayMovie() && SameFilm(),TEXT("Replay button changed movie data or accepted stale UI"))) { return; }
                ResultsStarted=-1;
            }
            else
            {
                if (!Check(SameFilm(),TEXT("Replayed film or scores changed"))) { return; }
                bReplayPreserved=true;
                Movie->bUseDefaultHeistObjectivesIfEmpty=false;
                for (auto& Definition:Movie->Roles) { Definition.Objectives.Reset(); }
                Widget->NewMovieButton->OnClicked.Broadcast();
                bNewMovieCleared=Movie->IsWardrobeOpen() && Movie->GetCurrentTakeNumber()==1 && Movie->Performances.IsEmpty()
                    && Movie->ActiveReplays.IsEmpty() && Movie->ScoreSummary.RoleScores.IsEmpty() && !Movie->MovieResults.bAvailable && Movie->GetCurrentScore()==0;
                if (!Check(bNewMovieCleared && !Widget->RecordNewMovie(),TEXT("New Movie button failed to clear data or accepted stale UI"))) { return; }
                Round=1; ResultsStarted=-1;
            }
        }
        else
        {
            bZeroRated=Movie->MovieResults.FinalScore==0 && Movie->MovieResults.Stars==1 && !Movie->MovieResults.bAllObjectivesCompleted;
            for (const auto& Performance:Movie->Performances) { bZeroRated &= Performance.ObjectiveResult.CompletedObjectiveIds.IsEmpty() && Performance.Recording.Events.IsEmpty(); }
            if (!Check(bZeroRated,TEXT("A movie with no completed objectives could not be watched and rated"))) { return; }
            if (Age>.25) { Capture(4,TEXT("ZeroResults")); }
            if (Age<.65 || (bRender && FScreenshotRequest::IsScreenshotRequested())) { return; }
            if (!Check(PreparationCount==6 && TitleCount==3 && PlaybackCount==3,TEXT("Complete loop did not visit all six wardrobes and three playbacks"))) { return; }
            Finish(true,TEXT("Real UMG, title transition, camera, independent events/outfits/props, ratings/awards, Replay, clean New Movie, and zero-score film passed"));
        }
    }

    void FRun::Tick()
    {
        if (bDone || !World.IsValid()) { return; }
        if (FPlatformTime::Seconds()-Started>150.0) { Finish(false,TEXT("Premiere smoke timed out")); return; }
        APawn* Pawn=UGameplayStatics::GetPlayerPawn(World.Get(),0); if (!Pawn) { return; }
        if (!Manager.IsValid()) { Setup(Pawn); return; }
        auto* Movie=Manager.Get();
        if (!Check(Movie->State!=EMovieState::Error,TEXT("Movie entered Error"))) { return; }
        if (Movie->State!=EMovieState::Finished) { ResultsStarted=-1; }
        if (Movie->IsWardrobeOpen()) { Wardrobe(Pawn); }
        else if (Movie->State==EMovieState::Recording) { Recording(Pawn); }
        else if (Movie->State==EMovieState::MoviePremiere) { Premiere(Pawn); }
        else if (Movie->State==EMovieState::Finished) { Results(); }
    }

    void Start(UWorld* World)
    {
        if (!World || !World->IsGameWorld() || !FParse::Param(FCommandLine::Get(),TEXT("game")) || !FParse::Param(FCommandLine::Get(),TEXT("PremiereSmoke"))) { return; }
        auto Run=MakeShared<FRun>(); Run->World=World;
        Run->Directory=FPaths::ProjectSavedDir()/TEXT("Automation/PremierePlay");
        FParse::Value(FCommandLine::Get(),TEXT("PremiereReportDir="),Run->Directory);
        World->GetTimerManager().SetTimer(Run->Timer,FTimerDelegate::CreateLambda([Run]() { Run->Tick(); }),.05f,true);
    }
    FAutoConsoleCommandWithWorld Command(TEXT("Movie.PremiereSmoke"),TEXT("Separate -game -PremiereSmoke process only. Verifies premiere/results/new movie and exits."),FConsoleCommandWithWorldDelegate::CreateStatic(&Start));
}
#endif
