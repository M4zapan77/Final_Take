#if !UE_BUILD_SHIPPING
#include "StartupVideoComponent.h"
#include "StartupVideoWidget.h"
#include "StartupTravelSubsystem.h"
#include "StudioFlowManager.h"
#include "StudioFlowWidget.h"
#include "ActingComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "WardrobeWidget.h"
#include "MediaPlayer.h"
#include "MediaSoundComponent.h"
#include "FileMediaSource.h"
#include "Components/Button.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SWindow.h"
#include "UnrealClient.h"

namespace StartupVideoPlayTest
{
    class FRun
    {
    public:
        FString Scenario=TEXT("Auto"),Directory;
        TWeakObjectPtr<UWorld> InitialWorld;
        TWeakObjectPtr<AStudioFlowManager> InitialFlow;
        TWeakObjectPtr<UMediaPlayer> HubPlayer;
        TWeakObjectPtr<UMediaSoundComponent> HubSound;
        double Started=FPlatformTime::Seconds(),HubStarted=-1,StepStarted=-1,PointerStarted=0,ProbeStarted=-1,GameplayAt=-1,NotPlayingSince=-1;
        FVector2D PointerPosition;
        FVector ProbePosition;
        int32 Step=0,PanelStep=0,FailuresBefore=0,FaultStep=0;
        bool bDone=false,bSetup=false,bStudio=false,bGame=false,bHub=false,bLoop=false,bPanels=false,bSkipped=false,bHeld=false,bReleased=false,bConsumed=false;
        bool bFailures=false,bBlack=false,bLoaded=false,bMediaStopped=false,bInput=false,bReadyClicked=false,bWDown=false;
        TSet<FString> Captured;
        TObjectPtr<UFileMediaSource> OriginalStudio,OriginalGame,OriginalHub;
        TStrongObjectPtr<UFileMediaSource> Broken;
        void Capture(const TCHAR* Name)
        {
            if (Captured.Contains(Name) || FScreenshotRequest::IsScreenshotRequested()) { return; }
            Captured.Add(Name); IFileManager::Get().MakeDirectory(*Directory,true); FScreenshotRequest::RequestScreenshot(Directory/(FString(Name)+TEXT(".png")),true,false);
        }
        void Finish(bool bPass,const TCHAR* Message,UWorld* World)
        {
            if (bDone) { return; } bDone=true;
            const auto Bool=[](bool Value){return Value?TEXT("true"):TEXT("false");};
            const FString Json=FString::Printf(TEXT("{\n\"passed\":%s,\n\"scenario\":\"%s\",\n\"studioPlayback\":%s,\n\"gamePlayback\":%s,\n\"hubPlayback\":%s,\n\"hubLoops\":%s,\n\"optionsContinuous\":%s,\n\"skipConsumed\":%s,\n\"missingAndFailedMedia\":%s,\n\"fadeBlack\":%s,\n\"levelLoaded\":%s,\n\"mediaStopped\":%s,\n\"gameplayInput\":%s,\n\"message\":\"%s\"\n}\n"),Bool(bPass),*Scenario,Bool(bStudio),Bool(bGame),Bool(bHub),Bool(bLoop),Bool(bPanels),Bool(bConsumed),Bool(bFailures),Bool(bBlack),Bool(bLoaded),Bool(bMediaStopped),Bool(bInput),Message);
            IFileManager::Get().MakeDirectory(*Directory,true); FFileHelper::SaveStringToFile(Json,*(Directory/TEXT("result.json")));
            if (auto* Flow=AStudioFlowManager::Find(World)) { Flow->StartupVideos->StopVideos(); Flow->Movie->StopMovie(); }
            UE_LOG(LogTemp,Display,TEXT("StartupVideoPlay %s: %s"),bPass?TEXT("PASS"):TEXT("FAIL"),Message);
            FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
        }
        bool Check(bool Condition,const TCHAR* Message,UWorld* World) { if (!Condition) { Finish(false,Message,World); } return Condition; }
        void MouseDown(const FVector2D& Point)
        {
            PointerPosition=Point; auto Window=GEngine->GameViewport->GetWindow();
            FSlateApplication::Get().SetCursorPos(Point);
            const TSet<FKey> Empty; FPointerEvent Move(0,Point,Point,Empty,EKeys::Invalid,0,FModifierKeysState()); FSlateApplication::Get().ProcessMouseMoveEvent(Move,true);
            const TSet<FKey> Down={EKeys::LeftMouseButton}; FPointerEvent Event(0,Point,Point,Down,EKeys::LeftMouseButton,0,FModifierKeysState());
            FSlateApplication::Get().ProcessMouseButtonDownEvent(Window.IsValid()?Window->GetNativeWindow():nullptr,Event);
        }
        void MouseUp()
        {
            const TSet<FKey> Empty; FPointerEvent Event(0,PointerPosition,PointerPosition,Empty,EKeys::LeftMouseButton,0,FModifierKeysState());
            FSlateApplication::Get().ProcessMouseButtonUpEvent(Event);
        }
        void Click(UButton* Button)
        {
            const FGeometry Geometry=Button->GetCachedGeometry(); MouseDown(Geometry.LocalToAbsolute(Geometry.GetLocalSize()*.5)); MouseUp();
        }
        void Setup(AStudioFlowManager* Flow,UWorld* World)
        {
            InitialWorld=World; InitialFlow=Flow; bSetup=true;
            OriginalStudio=Flow->StartupVideos->Settings.StudioIntro; OriginalGame=Flow->StartupVideos->Settings.GameIntroduction; OriginalHub=Flow->StartupVideos->Settings.HubBackground;
            if (Scenario==TEXT("Failures"))
            {
                auto* Video=Flow->StartupVideos.Get(); Video->Settings.StudioIntro=nullptr; Video->Settings.GameIntroduction=nullptr; Video->Settings.HubBackground=nullptr;
                Video->BeginSequence(false); FailuresBefore=Video->MediaFailures;
                const FString BadPath=FPaths::ProjectSavedDir()/TEXT("Automation/StartupBad.mp4");
                IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir()/TEXT("Automation")),true); FFileHelper::SaveStringToFile(TEXT("This is intentionally not a video"),*BadPath);
                Broken.Reset(NewObject<UFileMediaSource>(GetTransientPackage())); Broken->SetFilePath(BadPath);
            }
        }
        void Tick(UWorld* World);
        void Hub(AStudioFlowManager* Flow,UWorld* World);
        void Faults(AStudioFlowManager* Flow,UWorld* World);
        void Gameplay(AStudioFlowManager* Flow,UWorld* World);
    };

    void FRun::Tick(UWorld* World)
    {
        if (bDone) { return; }
        if (FPlatformTime::Seconds()-Started>180) { Finish(false,TEXT("Video integration timed out"),World); return; }
        auto* Flow=AStudioFlowManager::Find(World); if (!Flow) { return; }
        if (!bSetup) { Setup(Flow,World); }
        if (auto* Travel=UStartupTravelSubsystem::Find(World); Travel && Travel->IsGameplayTransitionActive() && Travel->GetBlackOpacity()>.99)
        { bBlack=true; Capture(TEXT("FadeToBlack")); }
        if (World!=InitialWorld.Get()) { Gameplay(Flow,World); return; }
        auto* Video=Flow->StartupVideos.Get();
        if (Video->IsIntro())
        {
            if (Video->bHasVideoFrame && Video->Phase==EStartupVideoPhase::Playing && Video->BlackOpacity==0 && Video->MediaPlayer && Video->MediaPlayer->GetTime().GetTotalSeconds()>2)
            {
                const bool Studio=Video->Stage==EStartupVideoStage::StudioIntro;
                if (Studio) { bStudio=true; Capture(TEXT("StudioIntro")); } else { bGame=true; Capture(TEXT("GameIntroduction")); }
                if (!Check(FMath::Abs(Video->GetAspectRatio()-16.f/9.f)<.01f && !Flow->Movie->HasActiveSession(),TEXT("Aspect ratio or intro gameplay gate incorrect"),World)) { return; }
                const bool Skip=(Scenario==TEXT("SkipStudio") && Studio) || (Scenario==TEXT("SkipGame") && !Studio);
                if (Skip && !bSkipped)
                {
                    const FGeometry Geometry=Video->OverlayWidget->GetCachedGeometry();
                    MouseDown(Geometry.LocalToAbsolute(FVector2D(Geometry.GetLocalSize().X*.17,Geometry.GetLocalSize().Y*.88)));
                    bSkipped=Video->bSkipped; bHeld=true; PointerStarted=FPlatformTime::Seconds();
                    Check(bSkipped,TEXT("Actual left mouse click did not skip the introduction"),World);
                }
            }
        }
        if (bHeld && !bReleased)
        {
            if (!Check(!Video->IsHubInteractive() && Flow->Screen!=EStudioScreen::StartingGame,TEXT("Held skip click leaked to Start Game"),World)) { return; }
            if (FPlatformTime::Seconds()-PointerStarted>.9) { MouseUp(); bReleased=true; bConsumed=true; }
        }
        if (Video->Stage==EStartupVideoStage::Hub && Video->IsHubInteractive())
        {
            if (Scenario==TEXT("Failures")) { Faults(Flow,World); }
            else { Hub(Flow,World); }
        }
        if (Video->Stage==EStartupVideoStage::StartingGame && Video->BlackOpacity>.95)
        { bBlack=true; Capture(TEXT("FadeToBlack")); }
    }

    void FRun::Hub(AStudioFlowManager* Flow,UWorld* World)
    {
        auto* Video=Flow->StartupVideos.Get(); const double Now=FPlatformTime::Seconds();
        if (!bHub)
        {
            bHub=Video->bHasVideoFrame && Video->MediaPlayer && Video->MediaPlayer->IsPlaying();
            if (!Check(bHub,TEXT("HUB background did not decode and play"),World)) { return; }
            if (Scenario==TEXT("Auto") && !Check(bStudio && bGame && !Video->bSkipped,TEXT("Automatic sequence did not play both intro videos"),World)) { return; }
            if (Scenario==TEXT("SkipStudio") && !Check(bStudio && !bGame && bConsumed,TEXT("Skipping studio did not bypass the remaining introduction"),World)) { return; }
            if (Scenario==TEXT("SkipGame") && !Check(bStudio && bGame && bConsumed,TEXT("Skipping game intro was not consumed"),World)) { return; }
            HubPlayer=Video->MediaPlayer; HubSound=Video->MediaSound; HubStarted=Now; StepStarted=Now; return;
        }
        if (Step>0) { return; }
        if (!Check(Video->MediaPlayer==HubPlayer.Get(),TEXT("Options navigation reopened the hub player"),World)) { return; }
        // WMF briefly reports a non-playing state while its loop seek completes.
        // Require the same player and prompt resumption rather than a particular state on every frame.
        if (!Video->MediaPlayer->IsPlaying())
        {
            if (NotPlayingSince<0)
            {
                NotPlayingSince=Now;
                UE_LOG(LogTemp,Display,TEXT("StartupVideoPlay: loop boundary: time %.3f, duration %.3f, looping %d, paused %d, loops %d"),Video->MediaPlayer->GetTime().GetTotalSeconds(),Video->MediaPlayer->GetDuration().GetTotalSeconds(),Video->MediaPlayer->IsLooping(),Video->MediaPlayer->IsPaused(),Video->HubLoops);
            }
            if (!Check(Now-NotPlayingSince<.75,TEXT("HUB playback failed to resume after its loop boundary"),World)) { return; }
        }
        else { NotPlayingSince=-1; }
        if (Now-StepStarted>.7 && !FScreenshotRequest::IsScreenshotRequested())
        {
            auto* Widget=Flow->ActiveWidget.Get();
            switch (PanelStep)
            {
            case 0: Capture(TEXT("Hub")); break;
            case 1: Click(Widget->HowButton); break;
            case 2: if (!Check(Flow->Screen==EStudioScreen::HowToPlay,TEXT("HUB How to Play not interactive above video"),World)) { return; } Capture(TEXT("Options")); break;
            case 3: Click(Widget->BackButton); break;
            case 4: Click(Widget->CreditsButton); break;
            case 5: if (!Check(Flow->Screen==EStudioScreen::Credits,TEXT("Credits navigation failed"),World)) { return; } Click(Widget->BackButton); break;
            case 6: Click(Widget->AudioButton); break;
            case 7: if (!Check(Flow->Screen==EStudioScreen::AudioSettings,TEXT("Audio options failed"),World)) { return; } Click(Widget->BackButton); bPanels=true; break;
            }
            ++PanelStep; StepStarted=Now;
        }
        if (Video->HubLoops>=2 && Now-HubStarted>16 && bPanels && Flow->Screen==EStudioScreen::MainMenu)
        {
            bLoop=true; bFailures=true; Click(Flow->ActiveWidget->PlayButton); Step=1;
            Check(Flow->Screen==EStudioScreen::StartingGame,TEXT("Actual Start Game click did not begin fading"),World);
        }
    }

    void FRun::Faults(AStudioFlowManager* Flow,UWorld* World)
    {
        auto* Video=Flow->StartupVideos.Get();
        if (FaultStep==0)
        {
            if (!Check(Video->MediaFailures>=3 && !Video->bHasVideoFrame && Flow->Screen==EStudioScreen::MainMenu,TEXT("Missing videos did not fall through to a black functional hub"),World)) { return; }
            Capture(TEXT("MissingVideosHub")); Click(Flow->ActiveWidget->HowButton);
            if (!Check(Flow->Screen==EStudioScreen::HowToPlay,TEXT("Black fallback menu did not accept mouse input"),World)) { return; }
            Click(Flow->ActiveWidget->BackButton);
            Video->Settings.StudioIntro=Broken.Get(); Video->Settings.GameIntroduction=nullptr; Video->Settings.HubBackground=OriginalHub;
            FailuresBefore=Video->MediaFailures; Video->BeginSequence(false); FaultStep=1; return;
        }
        if (FaultStep==1)
        {
            if (!Check(Video->MediaFailures>=FailuresBefore+2 && Video->bHasVideoFrame,TEXT("Failed decoder intro did not continue to the HUB"),World)) { return; }
            bHub=true; Video->Settings.StudioIntro=nullptr; Video->Settings.GameIntroduction=Broken.Get(); Video->Settings.HubBackground=OriginalHub;
            FailuresBefore=Video->MediaFailures; Video->BeginSequence(false); FaultStep=2; return;
        }
        if (FaultStep==2)
        {
            if (!Check(Video->MediaFailures>=FailuresBefore+2 && Video->bHasVideoFrame,TEXT("Failed second decoder did not continue"),World)) { return; }
            Video->Settings.StudioIntro=nullptr; Video->Settings.GameIntroduction=nullptr; Video->Settings.HubBackground=Broken.Get();
            FailuresBefore=Video->MediaFailures; Video->BeginSequence(false); FaultStep=3; return;
        }
        if (FaultStep==3)
        {
            if (!Check(Video->MediaFailures>=FailuresBefore+3 && !Video->bHasVideoFrame,TEXT("Failed HUB decoder did not keep black menu usable"),World)) { return; }
            bFailures=true; bPanels=true;
            Click(Flow->ActiveWidget->PlayButton); Step=1; FaultStep=4;
        }
    }

    void FRun::Gameplay(AStudioFlowManager* Flow,UWorld* World)
    {
        auto* Travel=UStartupTravelSubsystem::Find(World); if (!Travel || Travel->IsGameplayTransitionActive()) { return; }
        if(Flow->Screen==EStudioScreen::MovieIntroduction && Flow->ActiveWidget)
        {
            // Exercise actual Slate hit testing after the cross-level input shield.
            // Calling BeginMovie directly would miss a GameOnly focus regression.
            Click(Flow->ActiveWidget->StartButton);return;
        }
        if (!Check(Travel->GameplayLoads==1 && Travel->GetBlackOpacity()==0 && (Flow->Movie->IsWardrobeOpen() || (bReadyClicked && Flow->Movie->State==EMovieState::Recording))
            && Flow->StartupVideos->Stage==EStartupVideoStage::Inactive,TEXT("Level did not become ready with videos stopped"),World)) { return; }
        bLoaded=true; bMediaStopped=(!HubPlayer.IsValid() || (!HubPlayer->IsPlaying() && HubPlayer->GetUrl().IsEmpty()))
            && (!HubSound.IsValid() || (!HubSound->IsPlaying() && !HubSound->GetMediaPlayer()));
        if (!Check(bMediaStopped,TEXT("Old media/audio survived gameplay travel"),World)) { return; }
        if (GameplayAt<0) { GameplayAt=FPlatformTime::Seconds(); }
        if (!bReadyClicked)
        {
            if (FPlatformTime::Seconds()-GameplayAt<.4) { return; }
            Capture(TEXT("GameplayWardrobe")); if (FScreenshotRequest::IsScreenshotRequested()) { return; }
            Click(Flow->Movie->ActiveWardrobeWidget->ReadyButton); bReadyClicked=true; return;
        }
        if (Flow->Movie->State==EMovieState::Recording)
        {
            auto* Player=UGameplayStatics::GetPlayerController(World,0);
            if (!bWDown)
            { ProbePosition=Player->GetPawn()->GetActorLocation(); ProbeStarted=FPlatformTime::Seconds(); Player->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,EKeys::W,IE_Pressed,FPlatformTime::Cycles64())); bWDown=true; }
            if (FPlatformTime::Seconds()-ProbeStarted>.25)
            {
                Player->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,EKeys::W,IE_Released,FPlatformTime::Cycles64()));
                bInput=!Player->IsMoveInputIgnored() && !Player->IsLookInputIgnored() && FVector::DistSquared2D(ProbePosition,Player->GetPawn()->GetActorLocation())>1;
                Finish(bInput && bBlack,TEXT("Video order, skip shield, HUB loop/options, black fade, real level travel and gameplay input verified"),World);
            }
        }
    }
    TSharedPtr<FRun> Active;
    void OnTick(UWorld* World,ELevelTick Tick,float Delta)
    {
        if (!World || !World->IsGameWorld() || !FParse::Param(FCommandLine::Get(),TEXT("StartupVideoSmoke"))) { return; }
        if (!Active)
        {
            Active=MakeShared<FRun>(); Active->Directory=FPaths::ProjectSavedDir()/TEXT("Automation/StartupVideo");
            FParse::Value(FCommandLine::Get(),TEXT("VideoScenario="),Active->Scenario); FParse::Value(FCommandLine::Get(),TEXT("VideoReportDir="),Active->Directory);
        }
        Active->Tick(World);
    }
    struct FRegistration
    {
        FDelegateHandle Handle;
        FRegistration() { Handle=FWorldDelegates::OnWorldTickStart.AddStatic(&OnTick); }
        ~FRegistration() { FWorldDelegates::OnWorldTickStart.Remove(Handle); Active.Reset(); }
    } Registration;
}
#endif
