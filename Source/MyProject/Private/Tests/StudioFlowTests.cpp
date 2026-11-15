#if WITH_DEV_AUTOMATION_TESTS
#include "StudioFlowManager.h"
#include "StudioUITheme.h"
#include "Engine/Texture2D.h"
#include "StudioFlowWidget.h"
#include "StudioAudioSubsystem.h"
#include "ActingComponent.h"
#include "ActingRecorderComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "MoviePresentationWidget.h"
#include "ObjectiveHUDWidget.h"
#include "ReplayActor.h"
#include "Blueprint/WidgetTree.h"
#include "Camera/CameraActor.h"
#include "Components/CanvasPanel.h"
#include "Components/SceneComponent.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include <limits>

namespace StudioTests
{
    class FWorld
    {
    public:
        explicit FWorld(bool bExternalLocks=false)
        {
            UWorld::InitializationValues Values;
            Values.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true).CreateNavigation(false)
                .CreateAISystem(false).CreateFXSystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
            World=UWorld::CreateWorld(EWorldType::Game,false,MakeUniqueObjectName(GetTransientPackage(),UWorld::StaticClass(),TEXT("StudioWorld")),nullptr,true,ERHIFeatureLevel::Num,&Values);
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            World->InitializeActorsForPlay(FURL()); World->GetWorldSettings()->NotifyBeginPlay();
            Player=World->SpawnActor<APlayerController>(); if (!Player->PlayerCameraManager) { Player->SpawnPlayerCameraManager(); }
            Pawn=World->SpawnActor<APawn>(); auto* Root=NewObject<USceneComponent>(Pawn); Pawn->AddInstanceComponent(Root); Pawn->SetRootComponent(Root); Root->RegisterComponent();
            Pawn->SetActorEnableCollision(true); Player->Possess(Pawn); Player->SetViewTarget(Pawn);
            if (bExternalLocks) { Player->SetIgnoreMoveInput(true); Player->SetIgnoreLookInput(true); }
            Camera=World->SpawnActor<ACameraActor>(FVector(-600,-400,400),FRotator(-20,30,0));
            Movie=World->SpawnActor<AMovieManager>(); Movie->PremiereCamera=Camera; Movie->PreparationDurationSeconds=5; Movie->RecordingDurationSeconds=.2;
            Movie->PremiereSettings.TitleFadeInSeconds=.025; Movie->PremiereSettings.TitleHoldSeconds=.05; Movie->PremiereSettings.TitleFadeOutSeconds=.025;
            Movie->bShowDebugOverlay=false; Movie->bBindActingKeys=Movie->bBindRestartKey=Movie->bBindInteractionKey=false;
            for (int32 Index=0;Index<3;++Index)
            {
                FMovieObjectiveDefinition Definition; Definition.Id=FName(*FString::Printf(TEXT("Take%d.Sound"),Index));
                Definition.DisplayName=FText::FromString(TEXT("Make a sound")); Definition.Type=EMovieObjectiveType::PlaySound;
                Definition.RequiredEventId=TEXT("EvilLaugh"); Definition.Points=100; Movie->Roles[Index].Objectives={Definition};
            }
            Flow=World->SpawnActor<AStudioFlowManager>(); Flow->Settings.PageFadeSeconds=0; Flow->Settings.BetweenActorsSeconds=.3; Flow->Settings.WrapSeconds=.3; Flow->Settings.NextActorCueDelay=.075;
            bReady=Flow->InitializeFlow(Movie,Player);
        }
        ~FWorld() { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); }
        void Tick(float Delta=.025f) { ++GFrameCounter; World->Tick(LEVELTICK_All,Delta); }
        void Until(EMovieState State,int32 Limit=100) { for (int32 Frame=0;Frame<Limit && Movie->State!=State && Movie->State!=EMovieState::Error;++Frame) { Tick(); } }
        bool Begin() { return Flow->PlayMovie() && Flow->BeginIntroducedMovie(); }
        UWorld* World=nullptr; APlayerController* Player=nullptr; APawn* Pawn=nullptr; ACameraActor* Camera=nullptr;
        AMovieManager* Movie=nullptr; AStudioFlowManager* Flow=nullptr; bool bReady=false;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioIntroductionTest,"OneManStudio.Studio.FrontEndAndIntroduction",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStudioIntroductionTest::RunTest(const FString& Parameters)
{
    StudioTests::FWorld Context; auto* Flow=Context.Flow; auto* Movie=Context.Movie;
    if (!TestTrue(TEXT("Front end initializes"),Context.bReady)) { return false; }
    TestTrue(TEXT("Main menu owns input and hides live performer"),Flow->Screen==EStudioScreen::MainMenu && Context.Player->IsMoveInputIgnored() && Context.Player->IsLookInputIgnored() && Context.Pawn->IsHidden() && !Context.Pawn->GetActorEnableCollision());
    TestTrue(TEXT("Menu uses existing camera without moving it"),Context.Player->GetViewTarget()==Context.Camera);
    TestFalse(TEXT("Main menu does not start a movie"),Movie->HasActiveSession());
    TestFalse(TEXT("Cannot skip introduction"),Flow->BeginIntroducedMovie());
    for (int32 Frame=0;Frame<20;++Frame) { Context.Tick(); }
    TestEqual(TEXT("Menu leaves shared clock at zero"),Context.World->GetSubsystem<UActingSceneClockSubsystem>()->GetSceneTimeSeconds(),0.0);
    TestTrue(TEXT("How to Play page"),Flow->ShowHowToPlay() && Flow->Screen==EStudioScreen::HowToPlay && !Flow->GetView().HowToPlay.IsEmpty());
    TestFalse(TEXT("Play rejected outside main menu"),Flow->PlayMovie());
    TestTrue(TEXT("Back then credits"),Flow->BackToMenu() && Flow->ShowCredits() && !Flow->GetView().Credits.IsEmpty());
    TestTrue(TEXT("Back then audio"),Flow->BackToMenu() && Flow->ShowAudioSettings() && Flow->Screen==EStudioScreen::AudioSettings);
    TestTrue(TEXT("Play enters introduction"),Flow->BackToMenu() && Flow->PlayMovie() && Flow->Screen==EStudioScreen::MovieIntroduction);
    TestFalse(TEXT("Duplicate Play rejected"),Flow->PlayMovie());
    const auto Brief=Flow->GetView(); TestEqual(TEXT("All roles are introduced"),Brief.Roles.Num(),3);
    TestFalse(TEXT("Movie title and synopsis available"),Brief.MovieTitle.IsEmpty() || Brief.Synopsis.IsEmpty());
    for (const auto& Item:Brief.Roles) { TestFalse(TEXT("Each role has script text"),Item.Script.IsEmpty()); }
    TestTrue(TEXT("Introduction begins Take 1 wardrobe"),Flow->BeginIntroducedMovie() && Flow->Screen==EStudioScreen::Wardrobe && Movie->IsWardrobeOpen() && Movie->GetCurrentTakeNumber()==1);
    TestFalse(TEXT("Duplicate start rejected"),Flow->BeginIntroducedMovie());
    TestTrue(TEXT("Live pawn restored before MovieManager captures it"),!Context.Pawn->IsHidden() && Context.Pawn->GetActorEnableCollision());
    TestEqual(TEXT("Wardrobe announcement"),Flow->LastAnnouncement,EDirectorCue::ChooseCostume);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioTransitionsTest,"OneManStudio.Studio.TransitionsReplayAndMenu",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStudioTransitionsTest::RunTest(const FString& Parameters)
{
    StudioTests::FWorld Context; auto* Flow=Context.Flow; auto* Movie=Context.Movie;
    if (!TestTrue(TEXT("Begin introduced movie"),Context.Begin())) { return false; }
    const FTransform CameraPose=Context.Camera->GetActorTransform();
    for (int32 Take=0;Take<3;++Take)
    {
        if (!TestTrue(TEXT("Ready starts recording once"),Movie->ReadyForTake(Movie->PreparationId))) { return false; }
        TestEqual(TEXT("Action cue"),Flow->LastAnnouncement,EDirectorCue::Action);
        TestTrue(TEXT("Movement and camera look restored for action"),!Context.Player->IsMoveInputIgnored() && !Context.Player->IsLookInputIgnored());
        TestTrue(TEXT("Live sound event recorded"),Movie->LiveActingComponent->TriggerSound(EActingSound::EvilLaugh));
        Context.Until(EMovieState::TakeTransition);
        if (!TestEqual(TEXT("Cut enters explicit transition"),Movie->State,EMovieState::TakeTransition)) { return false; }
        TestEqual(TEXT("Cut cue"),Flow->LastAnnouncement,EDirectorCue::Cut);
        TestTrue(TEXT("Transition freezes input and recording"),Context.Player->IsMoveInputIgnored() && Context.Player->IsLookInputIgnored() && !Context.Pawn->FindComponentByClass<UActingRecorderComponent>()->IsRecording() && !Movie->LiveActingComponent->CanTriggerActions());
        TestEqual(TEXT("Transition clock zero"),Movie->GetSceneTimeSeconds(),0.0);
        for (AReplayActor* Replay:Movie->ActiveReplays) { TestFalse(TEXT("Replay media stopped between takes"),Replay->IsPlaying()); }
        for (int32 Frame=0;Frame<4;++Frame) { Context.Tick(); }
        TestEqual(TEXT("Next actor or wrap cue"),Flow->LastAnnouncement,Take<2?EDirectorCue::NextActor:EDirectorCue::Wrap);
        TestEqual(TEXT("Transition still at time zero"),Movie->GetSceneTimeSeconds(),0.0);
        Context.Until(Take<2?EMovieState::Preparation:EMovieState::MoviePremiere);
        if (Take<2) { TestEqual(TEXT("Each next character gets a wardrobe"),Movie->GetCurrentTakeNumber(),Take+2); }
    }
    Context.Until(EMovieState::Finished); if (!TestTrue(TEXT("Results reached"),Movie->IsResultsOpen())) { return false; }
    TestEqual(TEXT("All roles scored"),Movie->MovieResults.FinalScore,100.0);
    const auto Film=Movie->Performances; const auto Results=Movie->MovieResults;
    TestTrue(TEXT("Menu route available without a separate menu map"),Movie->CanReturnToMainMenu() && Movie->MainMenuMap.IsNull());
    TestTrue(TEXT("Replay works"),Movie->ReplayMovie()); Context.Until(EMovieState::Finished);
    TestEqual(TEXT("Replay keeps performance count"),Movie->Performances.Num(),Film.Num());
    for (int32 Index=0;Index<Film.Num();++Index)
    { TestTrue(TEXT("Replay leaves recordings and scores identical"),FMoviePerformance::StaticStruct()->CompareScriptStruct(&Film[Index],&Movie->Performances[Index],0)); }
    TestTrue(TEXT("Replay keeps result identical"),FMovieResults::StaticStruct()->CompareScriptStruct(&Results,&Movie->MovieResults,0));
    TestTrue(TEXT("Fixed camera unchanged"),Context.Camera->GetActorTransform().Equals(CameraPose));
    TestTrue(TEXT("Results returns to main menu"),Movie->ReturnToMainMenu());
    TestTrue(TEXT("Menu clears movie, actors, scores, clock"),Flow->Screen==EStudioScreen::MainMenu && !Movie->HasActiveSession() && Movie->Performances.IsEmpty() && Movie->ActiveReplays.IsEmpty() && Movie->ScoreSummary.RoleScores.IsEmpty() && !Movie->MovieResults.bAvailable && Movie->GetSceneTimeSeconds()==0);
    TestFalse(TEXT("Old results action rejected"),Movie->ReplayMovie());
    TestTrue(TEXT("Another film starts without developer commands"),Context.Begin() && Movie->IsWardrobeOpen() && Movie->GetCurrentTakeNumber()==1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioRestartLocksTest,"OneManStudio.Studio.RestartAndInputOwnership",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStudioRestartLocksTest::RunTest(const FString& Parameters)
{
    StudioTests::FWorld Context(true); auto* Movie=Context.Movie; auto* Flow=Context.Flow;
    if (!TestTrue(TEXT("Start with outside input locks"),Context.Begin() && Movie->ReadyForTake(Movie->PreparationId))) { return false; }
    TestTrue(TEXT("External move/look locks survive menu and wardrobe"),Context.Player->IsMoveInputIgnored() && Context.Player->IsLookInputIgnored());
    Context.Until(EMovieState::TakeTransition);
    TestTrue(TEXT("Restart is safe during transition"),Movie->RestartMovie() && Movie->IsWardrobeOpen() && Movie->GetCurrentTakeNumber()==1 && Movie->Performances.IsEmpty());
    TestEqual(TEXT("Restart returns flow to wardrobe"),Flow->Screen,EStudioScreen::Wardrobe);
    Movie->ReadyForTake(Movie->PreparationId); Context.Until(EMovieState::TakeTransition);
    TestTrue(TEXT("Can leave a busy film safely"),Flow->ReturnToMainMenu());
    TestTrue(TEXT("Menu has no recorder samples or events"),Context.Pawn->FindComponentByClass<UActingRecorderComponent>()->GetRecordingData().Samples.IsEmpty() && Context.Pawn->FindComponentByClass<UActingRecorderComponent>()->GetRecordingData().Events.IsEmpty());
    Flow->Destroy();
    TestTrue(TEXT("Only front end's locks are released"),Context.Player->IsMoveInputIgnored() && Context.Player->IsLookInputIgnored());
    Context.Player->SetIgnoreMoveInput(false); Context.Player->SetIgnoreLookInput(false);
    TestTrue(TEXT("No hidden lock count leaked"),!Context.Player->IsMoveInputIgnored() && !Context.Player->IsLookInputIgnored());
    TestTrue(TEXT("Front end destruction restores pawn and camera"),!Context.Pawn->IsHidden() && Context.Pawn->GetActorEnableCollision() && Context.Player->GetViewTarget()==Context.Pawn);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioUIAudioTest,"OneManStudio.Studio.UIAudioAndConfiguration",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStudioUIAudioTest::RunTest(const FString& Parameters)
{
    StudioTests::FWorld Context;
    auto* Widget=NewObject<UStudioFlowWidget>(Context.World); Widget->Initialize(); Widget->BindFlow(Context.Flow);
    TestTrue(TEXT("All native menu/introduction buttons exist"),Widget->PlayButton && Widget->HowButton && Widget->CreditsButton && Widget->QuitButton && Widget->AudioButton && Widget->StartButton && Widget->BackButton);
    TestTrue(TEXT("Three audio sliders exist"),Widget->MasterSlider && Widget->MusicSlider && Widget->EffectsSlider);
    TestFalse(TEXT("Unowned widget cannot start a movie"),Widget->PlayMovie());
    auto* Custom=NewObject<UStudioFlowWidget>(Context.World); Custom->WidgetTree=NewObject<UWidgetTree>(Custom); auto* Root=Custom->WidgetTree->ConstructWidget<UCanvasPanel>(); Custom->WidgetTree->RootWidget=Root;
    Custom->Initialize(); TestTrue(TEXT("Designer root is preserved"),Custom->WidgetTree->RootWidget==Root && !Custom->PlayButton);
    TestTrue(TEXT("Start movie for recording HUD"),Context.Begin() && Context.Movie->ReadyForTake(Context.Movie->PreparationId));
    auto* HUD=NewObject<UObjectiveHUDWidget>(Context.World); HUD->Initialize(); HUD->BindMovie(Context.Movie);
    TestTrue(TEXT("HUD exposes role, countdown and all gesture/sound controls"),HUD->ViewData.bRecording && !HUD->ViewData.RoleName.IsEmpty() && HUD->ViewData.RemainingSeconds>0 && HUD->ViewData.Controls.ToString().Contains(TEXT("Dance")) && HUD->ViewData.Controls.ToString().Contains(TEXT("Siren")));
    auto* Instance=NewObject<UGameInstance>(); auto* Audio=NewObject<UStudioAudioSubsystem>(Instance);
    FStudioAudioLevels Levels; Levels.Master=.5f; Levels.Music=.25f; Levels.Effects=.75f; Audio->SetLevels(Levels);
    FStudioAudioLevels Defaults; Audio->InitializeDefaults(Defaults); TestEqual(TEXT("Defaults never overwrite user choice"),Audio->GetLevels().Master,.5f);
    Levels.Master=std::numeric_limits<float>::quiet_NaN(); Levels.Music=5; Levels.Effects=-3; Audio->SetLevels(Levels);
    TestTrue(TEXT("Nonfinite and out-of-range audio values are safe"),Audio->GetLevels().Master==0 && Audio->GetLevels().Music==1 && Audio->GetLevels().Effects==0);
    FStudioFlowSettings Settings; TestTrue(TEXT("Native flow settings valid"),Settings.IsValid()); const FDirectorAnnouncement Duplicate=Settings.Announcements[0]; Settings.Announcements.Add(Duplicate); TestFalse(TEXT("Duplicate director cue rejected"),Settings.IsValid());
    Settings=FStudioFlowSettings(); Settings.WrapSeconds=-1; TestFalse(TEXT("Negative transition rejected"),Settings.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioHubNavigationTest,"OneManStudio.Studio.HubNavigationAndCleanReturn",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStudioHubNavigationTest::RunTest(const FString& Parameters)
{
    StudioTests::FWorld C;auto* Flow=C.Flow;auto* Movie=C.Movie;
    TestTrue(TEXT("Play opens studio before a movie session exists"),Flow->PlayMovie() && Flow->Screen==EStudioScreen::MovieIntroduction && !Movie->HasActiveSession());
    TestTrue(TEXT("Studio exposes existing audio settings"),Flow->ShowAudioSettings() && Flow->Screen==EStudioScreen::AudioSettings);
    TestTrue(TEXT("Settings return to their originating studio"),Flow->BackToMenu() && Flow->Screen==EStudioScreen::MovieIntroduction);
    TestTrue(TEXT("Studio tutorial retains its return route"),Flow->ShowHowToPlay() && Flow->BackToMenu() && Flow->Screen==EStudioScreen::MovieIntroduction);
    TestTrue(TEXT("Back reaches main menu"),Flow->BackToMenu() && Flow->Screen==EStudioScreen::MainMenu);
    TestTrue(TEXT("Start Movie enters the existing first wardrobe"),C.Begin() && Movie->IsWardrobeOpen() && Movie->GetCurrentTakeNumber()==1);
    TestFalse(TEXT("Repeated Start cannot bypass or duplicate preparation"),Flow->BeginIntroducedMovie());
    TestTrue(TEXT("Returning to studio safely clears the active movie"),Flow->ReturnToHub() && Flow->Screen==EStudioScreen::MovieIntroduction && !Movie->HasActiveSession() && Movie->Performances.IsEmpty() && Movie->ActiveReplays.IsEmpty());
    TestTrue(TEXT("Another movie enters a clean first wardrobe"),Flow->BeginIntroducedMovie() && Movie->IsWardrobeOpen() && Movie->GetCurrentTakeNumber()==1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioSuppliedUIAssetsTest,"OneManStudio.Studio.SuppliedArtReferences",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStudioSuppliedUIAssetsTest::RunTest(const FString& Parameters)
{
    auto* Theme=UStudioUITheme::LoadDefault();if(!TestNotNull(TEXT("Imported configurable UI theme"),Theme)){return false;}
    TestNotNull(TEXT("Supplied Bungee font face"),Theme->HeadingFace.Get());
    auto* Logo=Theme->Texture(TEXT("Logo"));if(TestNotNull(TEXT("Approved logo"),Logo))
    {
#if WITH_EDITORONLY_DATA
        // NullRHI intentionally has no platform texture resource. Inspect the
        // imported original here; the rendered integration also checks GetSizeX/Y.
        TestEqual(TEXT("Original logo width retained"),static_cast<int32>(Logo->Source.GetSizeX()),4096);TestEqual(TEXT("Original logo height retained"),static_cast<int32>(Logo->Source.GetSizeY()),2252);
        TestTrue(TEXT("UI compression retains alpha"),Logo->CompressionSettings==TC_EditorIcon && !Logo->CompressionNoAlpha && Logo->LODGroup==TEXTUREGROUP_UI);
#endif
    }
    for(const TCHAR* Key:{TEXT("Button.Normal"),TEXT("Button.Hover"),TEXT("Button.Pressed"),TEXT("Wardrobe.ActorPanel"),TEXT("Wardrobe.GridPanel"),TEXT("Wardrobe.Card.Normal"),TEXT("Wardrobe.Card.Selected"),TEXT("Wardrobe.Tab.Normal"),TEXT("Wardrobe.Tab.Selected"),TEXT("Icon.Eyes"),TEXT("Icon.Mouth"),TEXT("Icon.Eyebrows"),TEXT("Icon.Skin"),TEXT("Icon.Clothing"),TEXT("Icon.Hats"),TEXT("Icon.Accessories"),TEXT("Icon.Objects"),TEXT("Star")})
    {TestNotNull(FString::Printf(TEXT("Required supplied art: %s"),Key),Theme->Texture(Key));}
    return true;
}
#endif
