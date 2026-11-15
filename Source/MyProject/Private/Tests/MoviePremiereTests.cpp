#if WITH_DEV_AUTOMATION_TESTS
#include "MovieManager.h"
#include "MoviePresentationWidget.h"
#include "ActingComponent.h"
#include "ActingRecorderComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "ObjectiveManagerComponent.h"
#include "PropInteractionComponent.h"
#include "ReplayActor.h"
#include "Camera/CameraActor.h"
#include "Components/Button.h"
#include "Components/SceneComponent.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"

namespace PremiereTests
{
    class FWorld
    {
    public:
        FWorld()
        {
            UWorld::InitializationValues Values;
            Values.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true)
                .CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
            World=UWorld::CreateWorld(EWorldType::Game,false,MakeUniqueObjectName(GetTransientPackage(),UWorld::StaticClass(),TEXT("PremiereWorld")),nullptr,true,ERHIFeatureLevel::Num,&Values);
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            World->InitializeActorsForPlay(FURL()); World->GetWorldSettings()->NotifyBeginPlay();
            Player=World->SpawnActor<APlayerController>(); if (!Player->PlayerCameraManager) { Player->SpawnPlayerCameraManager(); }
            Pawn=World->SpawnActor<APawn>(); auto* Root=NewObject<USceneComponent>(Pawn); Pawn->AddInstanceComponent(Root); Pawn->SetRootComponent(Root); Root->RegisterComponent();
            Player->Possess(Pawn); Player->SetViewTarget(Pawn);
            Camera=World->SpawnActor<ACameraActor>(FVector(-700,-400,350),FRotator(-15,30,0));
            Manager=World->SpawnActor<AMovieManager>(); Manager->PremiereCamera=Camera;
            Manager->PreparationDurationSeconds=10; Manager->RecordingDurationSeconds=.3;
            Manager->PremiereSettings.TitleFadeInSeconds=.05; Manager->PremiereSettings.TitleHoldSeconds=.1; Manager->PremiereSettings.TitleFadeOutSeconds=.05; Manager->PremiereSettings.ResultsFadeInSeconds=.05;
            Manager->bShowDebugOverlay=false; Manager->bBindRestartKey=Manager->bBindActingKeys=Manager->bBindInteractionKey=false;
            const int32 Points[]={100,75,50};
            for (int32 Index=0;Index<3;++Index)
            {
                FMovieObjectiveDefinition Objective; Objective.Id=FName(*FString::Printf(TEXT("Role%d.Sound"),Index)); Objective.DisplayName=FText::FromString(TEXT("Make the required sound"));
                Objective.Type=EMovieObjectiveType::PlaySound; Objective.RequiredEventId=ActingIds::Sound(static_cast<EActingSound>(Index)); Objective.Points=Points[Index];
                Manager->Roles[Index].Objectives={Objective};
            }
        }
        ~FWorld() { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); }
        void Tick(float Delta=.025f) { ++GFrameCounter; World->Tick(LEVELTICK_All,Delta); }
        void RecordThree(bool bAct=true)
        {
            Manager->StartMovie();
            for (int32 Index=0;Index<3 && Manager->State!=EMovieState::Error;++Index)
            {
                Manager->ReadyForTake(Manager->PreparationId);
                if (bAct)
                {
                    for (int32 Dance=0;Dance<=Index;++Dance) { Manager->LiveActingComponent->TriggerGesture(EActingGesture::Dance); }
                    Manager->LiveActingComponent->TriggerSound(static_cast<EActingSound>(Index));
                    if (Index==1) { Manager->LivePropInteraction->UseEquippedProp(); }
                }
                for (int32 Frame=0;Frame<80 && Manager->State==EMovieState::Recording && Manager->CurrentTakeIndex==Index;++Frame)
                { Pawn->AddActorWorldOffset(FVector(Index+1,1,0)); Tick(); }
            }
        }
        void UntilPlayback() { for (int32 Frame=0;Frame<80 && Manager->PresentationPhase==EMoviePresentationPhase::TitleCard;++Frame) { Tick(); } }
        void UntilFinished() { for (int32 Frame=0;Frame<160 && Manager->State!=EMovieState::Finished && Manager->State!=EMovieState::Error;++Frame) { Tick(); } }
        UWorld* World=nullptr; APlayerController* Player=nullptr; APawn* Pawn=nullptr; ACameraActor* Camera=nullptr; AMovieManager* Manager=nullptr;
    };
    bool SameFilm(const TArray<FMoviePerformance>& First,const TArray<FMoviePerformance>& Second)
    {
        if (First.Num()!=Second.Num()) { return false; }
        for (int32 Index=0;Index<First.Num();++Index)
        { if (!FMoviePerformance::StaticStruct()->CompareScriptStruct(&First[Index],&Second[Index],0)) { return false; } }
        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPremiereTitleReplayTest,"OneManStudio.Premiere.TitleAndReplay",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPremiereTitleReplayTest::RunTest(const FString& Parameters)
{
    PremiereTests::FWorld Context; auto* Movie=Context.Manager; const FTransform CameraPose=Context.Camera->GetActorTransform(); Context.RecordThree();
    if (!TestTrue(TEXT("Third take enters premiere title"),Movie->State==EMovieState::MoviePremiere && Movie->PresentationPhase==EMoviePresentationPhase::TitleCard)) { return false; }
    const auto Performances=Movie->Performances; const FMovieResults Results=Movie->MovieResults;
    TestTrue(TEXT("Existing assigned camera is used without moving it"),Movie->GetPremiereCamera()==Context.Camera && Context.Camera->GetActorTransform().Equals(CameraPose));
    TestTrue(TEXT("Live pawn hidden and movement/look locked"),Context.Pawn->IsHidden() && Context.Player->IsMoveInputIgnored() && Context.Player->IsLookInputIgnored());
    Context.Tick(.075f);
    TestTrue(TEXT("Title timer advances while shared scene clock stays stopped at zero"),Movie->GetPhaseElapsedSeconds()>0 && Movie->GetSceneTimeSeconds()==0 && !Context.World->GetSubsystem<UActingSceneClockSubsystem>()->IsClockRunning());
    for (AReplayActor* Replay:Movie->ActiveReplays) { TestEqual(TEXT("Title never fires even time-zero recorded events"),Replay->Acting->DispatchedEventCount,0); }
    Context.UntilPlayback();
    TestTrue(TEXT("Playback begins from shared zero after the full title transition"),Movie->IsMoviePlayback() && Movie->GetSceneTimeSeconds()==0);
    Context.UntilFinished();
    TestTrue(TEXT("Results automatically follow playback"),Movie->IsResultsOpen());
    TestEqual(TEXT("Average from 100,75,50 is 75"),Movie->MovieResults.FinalScore,75.0);
    TestEqual(TEXT("Average 75 receives four stars"),Movie->MovieResults.Stars,4);
    TestTrue(TEXT("All completed objectives retained"),Movie->MovieResults.bAllObjectivesCompleted);
    TestTrue(TEXT("Finished phase has no time left"),Movie->GetPhaseRemainingSeconds()==0);
    for (int32 Index=0;Index<3;++Index) { TestEqual(TEXT("Each replay reproduces its own full event sequence once"),Movie->ActiveReplays[Index]->Acting->DispatchedEventCount,Performances[Index].Recording.Events.Num()); }
    TestTrue(TEXT("Replay action succeeds"),Movie->ReplayMovie());
    TestFalse(TEXT("Duplicate replay request cannot start twice"),Movie->ReplayMovie());
    TestTrue(TEXT("Replaying does not change any reflected recording/score/outfit data"),PremiereTests::SameFilm(Performances,Movie->Performances));
    Context.UntilFinished();
    TestTrue(TEXT("Replayed film returns to results"),Movie->IsResultsOpen());
    TestTrue(TEXT("Replay keeps the original result and award snapshot"),FMovieResults::StaticStruct()->CompareScriptStruct(&Results,&Movie->MovieResults,0));
    TestTrue(TEXT("Replay keeps the same camera"),Movie->GetPremiereCamera()==Context.Camera && Context.Camera->GetActorTransform().Equals(CameraPose));
    TestTrue(TEXT("Replay leaves every performance unchanged"),PremiereTests::SameFilm(Performances,Movie->Performances));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPremiereButtonsTest,"OneManStudio.Premiere.ResultsButtons",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPremiereButtonsTest::RunTest(const FString& Parameters)
{
    PremiereTests::FWorld Context; auto* Movie=Context.Manager; Context.RecordThree(); Context.UntilFinished();
    auto* Widget=NewObject<UMoviePresentationWidget>(Context.World); Widget->Initialize(); Widget->BindMovie(Movie,Movie->PresentationId);
    TestNotNull(TEXT("Functional native UMG hierarchy"),Widget->GetRootWidget());
    TestTrue(TEXT("UI exposes exact scores and rating"),Widget->ViewData.Results.FinalScore==75 && Widget->ViewData.Results.Stars==4 && Widget->ViewData.Results.Roles.Num()==3);
    TestTrue(TEXT("Buttons are enabled only for results"),Widget->ViewData.bCanReplay && Widget->ViewData.bCanRecordNew);
    TestTrue(TEXT("Menu button is hidden when no menu exists"),Widget->MainMenuButton && Widget->MainMenuButton->GetVisibility()==ESlateVisibility::Collapsed && !Widget->ReturnToMainMenu());
    if (!TestNotNull(TEXT("Native replay button"),Widget->ReplayButton.Get())) { return false; }
    Widget->ReplayButton->OnClicked.Broadcast();
    TestTrue(TEXT("Real button delegate starts premiere"),Movie->State==EMovieState::MoviePremiere);
    TestFalse(TEXT("Stale widget cannot start another replay"),Widget->ReplayMovie());
    TestFalse(TEXT("Stale widget cannot clear a movie"),Widget->RecordNewMovie());
    Context.UntilFinished();
    auto* Next=NewObject<UMoviePresentationWidget>(Context.World); Next->Initialize(); Next->BindMovie(Movie,Movie->PresentationId);
    if (!TestNotNull(TEXT("Native new movie button"),Next->NewMovieButton.Get())) { return false; }
    Next->NewMovieButton->OnClicked.Broadcast();
    TestTrue(TEXT("New Movie returns to Take 1 wardrobe"),Movie->IsWardrobeOpen() && Movie->GetCurrentTakeNumber()==1);
    TestTrue(TEXT("New Movie clears results, recordings, actors and scoring"),!Movie->MovieResults.bAvailable && Movie->Performances.IsEmpty() && Movie->ActiveReplays.IsEmpty() && Movie->GetCurrentScore()==0 && Movie->ScoreSummary.RoleScores.IsEmpty());
    TestTrue(TEXT("New Movie clears recorder samples and events"),Context.Pawn->FindComponentByClass<UActingRecorderComponent>()->GetRecordingData().Samples.IsEmpty() && Context.Pawn->FindComponentByClass<UActingRecorderComponent>()->GetRecordingData().Events.IsEmpty());
    TestFalse(TEXT("Previous results button cannot affect the new wardrobe"),Next->RecordNewMovie());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPremiereCleanupTest,"OneManStudio.Premiere.RestartAndCleanup",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPremiereCleanupTest::RunTest(const FString& Parameters)
{
    for (bool bDuringPlayback:{false,true})
    {
        PremiereTests::FWorld Context; auto* Movie=Context.Manager;
        Context.Player->bShowMouseCursor=true; Context.Player->bEnableClickEvents=true; Context.Player->SetIgnoreLookInput(true);
        Context.RecordThree(); if (bDuringPlayback) { Context.UntilPlayback(); }
        TArray<TWeakObjectPtr<AReplayActor>> Old; for (AReplayActor* Replay:Movie->ActiveReplays) { Old.Add(Replay); }
        TestFalse(TEXT("Results-only action cannot fire during a title/playback"),Movie->RecordNewMovie());
        TestTrue(TEXT("Explicit restart can safely interrupt title or playback"),Movie->RestartMovie());
        TestTrue(TEXT("Restart clears presentation phase, results, scores and clock"),Movie->PresentationPhase==EMoviePresentationPhase::None && !Movie->MovieResults.bAvailable && Movie->ScoreSummary.RoleScores.IsEmpty() && Movie->GetSceneTimeSeconds()==0);
        for (auto Replay:Old) { TestFalse(TEXT("Old replay actors destroyed"),Replay.IsValid()); }
        Movie->StopMovie();
        TestTrue(TEXT("Stop restores original cursor and click flags"),Context.Player->bShowMouseCursor && Context.Player->bEnableClickEvents);
        TestTrue(TEXT("Stop preserves another system's look lock"),Context.Player->IsLookInputIgnored());
        TestFalse(TEXT("Manager movement lock released"),Context.Player->IsMoveInputIgnored());
        TestTrue(TEXT("Assigned camera belongs to level and survives cleanup"),IsValid(Context.Camera));
        TestTrue(TEXT("Live pawn visibility and view restored"),!Context.Pawn->IsHidden() && Context.Player->GetViewTarget()==Context.Pawn);
        Context.Player->SetIgnoreLookInput(false);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPremiereZeroTest,"OneManStudio.Premiere.ZeroScore",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPremiereZeroTest::RunTest(const FString& Parameters)
{
    PremiereTests::FWorld Context; auto* Movie=Context.Manager;
    Movie->bUseDefaultHeistObjectivesIfEmpty=false; for (auto& Definition:Movie->Roles) { Definition.Objectives.Reset(); }
    Context.RecordThree(false); Context.UntilFinished();
    TestTrue(TEXT("Empty performances can still be watched and rated"),Movie->IsResultsOpen() && Movie->MovieResults.FinalScore==0 && Movie->MovieResults.Stars==1);
    TestEqual(TEXT("Zero rating label"),Movie->MovieResults.RatingName.ToString(),FString(TEXT("Cinematic Disaster")));
    TestTrue(TEXT("No objective completion invented"),!Movie->MovieResults.bAllObjectivesCompleted && Movie->MovieResults.Roles[0].CompletedObjectiveIds.IsEmpty());
    const FMovieResults Before=Movie->MovieResults;
    Movie->ResultsSettings.MovieTitle=FText::FromString(TEXT("A later movie title")); Movie->Roles[0].DisplayName=FText::FromString(TEXT("A different role"));
    TestTrue(TEXT("Zero movie can be replayed"),Movie->ReplayMovie()); Context.UntilFinished();
    TestTrue(TEXT("Changing authoring settings cannot rewrite an already recorded movie's result"),FMovieResults::StaticStruct()->CompareScriptStruct(&Before,&Movie->MovieResults,0));
    TestTrue(TEXT("Zero movie can be replaced"),Movie->RecordNewMovie());
    TestTrue(TEXT("Fresh movie starts clean"),Movie->IsWardrobeOpen() && Movie->GetCurrentScore()==0 && !Movie->MovieResults.bAvailable);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPremiereConfigurationTest,"OneManStudio.Premiere.Configuration",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPremiereConfigurationTest::RunTest(const FString& Parameters)
{
    PremiereTests::FWorld Context; auto* Movie=Context.Manager;
    Movie->PremiereSettings.TitleHoldSeconds=-1;
    AddExpectedMessage(TEXT("Premiere transition durations"),ELogVerbosity::Warning,EAutomationExpectedMessageFlags::Contains,1);
    TestFalse(TEXT("Invalid title settings rejected before session"),Movie->StartMovie());
    TestTrue(TEXT("Invalid setup does not leave a partial movie"),Movie->State==EMovieState::Error && Movie->Performances.IsEmpty());
    Movie->PremiereSettings.TitleHoldSeconds=0; Movie->PremiereSettings.TitleFadeInSeconds=Movie->PremiereSettings.TitleFadeOutSeconds=0;
    TestTrue(TEXT("Zero transition supports fast tests"),Movie->PremiereSettings.IsValid());
    Movie->MainMenuMap=TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/MissingMenu.MissingMenu")));
    TestFalse(TEXT("Missing menu cannot be used"),Movie->CanReturnToMainMenu());
    Context.RecordThree();
    TestTrue(TEXT("Zero-length title starts synchronized playback immediately"),Movie->IsMoviePlayback());
    Context.UntilFinished();
    TestFalse(TEXT("Missing menu button is unavailable in results"),Movie->GetPresentationView().bCanReturnToMenu);
    TestFalse(TEXT("Missing menu cannot clear retained movie"),Movie->ReturnToMainMenu());
    TestEqual(TEXT("Missing menu request retains three performances"),Movie->Performances.Num(),3);
    return true;
}
#endif
