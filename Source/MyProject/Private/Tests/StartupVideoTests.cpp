#if WITH_DEV_AUTOMATION_TESTS
#include "StartupVideoComponent.h"
#include "StartupTravelSubsystem.h"
#include "StudioFlowManager.h"
#include "ActingSceneClockSubsystem.h"
#include "FileMediaSource.h"
#include "MediaPlayer.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include <limits>

namespace StartupTests
{
    class FWorld
    {
    public:
        FWorld()
        {
            UWorld::InitializationValues Values;
            Values.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true).CreateNavigation(false)
                .CreateAISystem(false).CreateFXSystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
            World=UWorld::CreateWorld(EWorldType::Game,false,MakeUniqueObjectName(GetTransientPackage(),UWorld::StaticClass(),TEXT("StartupWorld")),nullptr,true,ERHIFeatureLevel::Num,&Values);
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World); World->InitializeActorsForPlay(FURL()); World->GetWorldSettings()->NotifyBeginPlay();
            Owner=World->SpawnActor<AStudioFlowManager>(); Player=World->SpawnActor<APlayerController>(); Videos=Owner->StartupVideos;
            Videos->InitializeVideos(Owner,Player); Videos->Settings.StudioIntro=nullptr; Videos->Settings.GameIntroduction=nullptr; Videos->Settings.HubBackground=nullptr; Videos->Settings.FadeSeconds=.05;
        }
        ~FWorld() { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); }
        void Tick(float Delta=.025f) { ++GFrameCounter; World->Tick(LEVELTICK_All,Delta); }
        UWorld* World=nullptr; AStudioFlowManager* Owner=nullptr; APlayerController* Player=nullptr; UStartupVideoComponent* Videos=nullptr;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStartupDefaultsTest,"OneManStudio.Startup.DefaultsAndValidation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStartupDefaultsTest::RunTest(const FString& Parameters)
{
    const auto* Defaults=GetDefault<UStartupVideoComponent>();
    TestTrue(TEXT("Startup is enabled with valid defaults"),Defaults->Settings.bEnabled && Defaults->Settings.IsValid());
    TestTrue(TEXT("All three assigned sources point to packaged project movies"),Defaults->Settings.StudioIntro && Defaults->Settings.GameIntroduction && Defaults->Settings.HubBackground
        && Defaults->Settings.StudioIntro->Validate() && Defaults->Settings.GameIntroduction->Validate() && Defaults->Settings.HubBackground->Validate());
    TestTrue(TEXT("UI material and starting level assigned"),Defaults->Settings.VideoMaterial && !Defaults->Settings.StartingGameplayLevel.IsNull());
    FStartupVideoSettings Settings; Settings.FadeSeconds=-1; TestFalse(TEXT("Negative fade rejected"),Settings.IsValid());
    Settings=FStartupVideoSettings(); Settings.OpenTimeoutSeconds=0; TestFalse(TEXT("Unbounded open timeout rejected"),Settings.IsValid());
    Settings=FStartupVideoSettings(); Settings.AudioVolume=std::numeric_limits<float>::quiet_NaN(); TestFalse(TEXT("Nonfinite gain rejected"),Settings.IsValid());
    Settings=FStartupVideoSettings(); Settings.HubDimmerOpacity=2; TestFalse(TEXT("Invalid dimmer rejected"),Settings.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStartupMissingTest,"OneManStudio.Startup.MissingMediaFallback",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStartupMissingTest::RunTest(const FString& Parameters)
{
    AddExpectedError(TEXT("StartupVideo: stage"),EAutomationExpectedErrorFlags::Contains,3);
    StartupTests::FWorld Context; auto* Videos=Context.Videos; Videos->BeginSequence(false);
    TestEqual(TEXT("Both missing intros fall through to hub"),Videos->Stage,EStartupVideoStage::Hub);
    TestEqual(TEXT("Missing hub included in failures"),Videos->MediaFailures,3);
    TestFalse(TEXT("No stale image is presented"),Videos->bHasVideoFrame);
    for(int32 Frame=0;Frame<6;++Frame){Context.Tick();}
    TestTrue(TEXT("Black hub becomes interactive"),Videos->IsHubInteractive());
    TestEqual(TEXT("Menu media never advances performance clock"),Context.World->GetSubsystem<UActingSceneClockSubsystem>()->GetSceneTimeSeconds(),0.0);
    TestFalse(TEXT("Cannot skip the HUB"),Videos->SkipIntroductions());
    Videos->Settings.StartingGameplayLevel.Reset(); TestFalse(TEXT("Missing gameplay map does not begin a fade"),Videos->RequestStartGame());
    TestTrue(TEXT("Invalid travel leaves menu functional"),Videos->IsHubInteractive());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStartupSkipTest,"OneManStudio.Startup.SkipAndInputGuard",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStartupSkipTest::RunTest(const FString& Parameters)
{
    AddExpectedError(TEXT("StartupVideo: stage"),EAutomationExpectedErrorFlags::Contains,2);
    StartupTests::FWorld Context; auto* Videos=Context.Videos;
    Videos->BeginSequence(true);
    for(int32 Frame=0;Frame<5;++Frame){Context.Tick();}
    Videos->Stage=EStartupVideoStage::StudioIntro; Videos->Phase=EStartupVideoPhase::Playing; Videos->BlackOpacity=0;
    Videos->MediaPlayer=NewObject<UMediaPlayer>(Videos);
    Videos->IntroPointerDown();
    TestTrue(TEXT("A pointer press skips the entire introduction and blocks interaction"),Videos->bSkipped && Videos->IsInputBlocked() && !Videos->IsHubInteractive());
    Videos->IntroPointerUp();
    TestFalse(TEXT("Release does not enable buttons in the same frame"),Videos->IsHubInteractive());
    Videos->ProcessEvent(Videos->FindFunctionChecked(TEXT("MediaEnded")),nullptr);
    for(int32 Frame=0;Frame<5;++Frame){Context.Tick();}
    TestTrue(TEXT("End notification during a skip cannot open the second intro"),Videos->Stage==EStartupVideoStage::Hub && Videos->MediaFailures==2);
    Videos->StopVideos(); TestTrue(TEXT("Stop removes all media layers and player"),Videos->Stage==EStartupVideoStage::Inactive && !Videos->MediaPlayer && !Videos->BackdropWidget && !Videos->OverlayWidget);
    Videos->Stage=EStartupVideoStage::GameIntro; Videos->Phase=EStartupVideoPhase::Playing;
    TestTrue(TEXT("Second intro supports the same skip transition"),Videos->SkipIntroductions() && Videos->Phase==EStartupVideoPhase::FadeOut);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStartupSessionTest,"OneManStudio.Startup.TravelSessionState",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStartupSessionTest::RunTest(const FString& Parameters)
{
    auto* Instance=NewObject<UGameInstance>(); auto* Travel=NewObject<UStartupTravelSubsystem>(Instance);
    TestFalse(TEXT("Intros initially not marked played"),Travel->HaveIntroductionsPlayed()); Travel->MarkIntroductionsSeen();
    TestTrue(TEXT("Application session remembers introductions"),Travel->HaveIntroductionsPlayed());
    TestFalse(TEXT("No fake gameplay intent before real travel"),Travel->ConsumeGameplayIntent());
    TestFalse(TEXT("Null gameplay map fails without a persistent shield"),Travel->BeginGameplayTravel(TSoftObjectPtr<UWorld>(),.2));
    TestTrue(TEXT("Rejected load never captures input"),!Travel->IsGameplayTransitionActive() && Travel->GetBlackOpacity()==0 && Travel->GameplayLoads==0);
    return true;
}
#endif
