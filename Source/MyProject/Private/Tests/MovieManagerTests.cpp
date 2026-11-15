#if WITH_DEV_AUTOMATION_TESTS

#include "MovieManager.h"
#include "ActingRecorderComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "ReplayActor.h"
#include "Camera/CameraActor.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Testing/ActingRecorderTestCharacter.h"

namespace MovieManagerTests
{
    class FWorld
    {
    public:
        FWorld()
        {
            UWorld::InitializationValues Values;
            Values.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true)
                .CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false)
                .ShouldSimulatePhysics(false).SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("MovieTestWorld")),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            World->InitializeActorsForPlay(FURL());
            World->GetWorldSettings()->NotifyBeginPlay();
        }

        ~FWorld()
        {
            World->EndPlay(EEndPlayReason::Quit);
            World->DestroyWorld(false);
            GEngine->DestroyWorldContext(World);
        }

        void Tick(float Delta = 0.025f)
        {
            ++GFrameCounter;
            World->Tick(LEVELTICK_All, Delta);
        }

        APawn* SpawnPawn()
        {
            APawn* NewPawn = World->SpawnActor<APawn>();
            USceneComponent* Root = NewObject<USceneComponent>(NewPawn);
            NewPawn->AddInstanceComponent(Root);
            NewPawn->SetRootComponent(Root);
            Root->RegisterComponent();
            NewPawn->SetActorLocation(FVector(100.0, 200.0, 100.0));
            return NewPawn;
        }

        AMovieManager* SetupMovie()
        {
            Player = World->SpawnActor<APlayerController>();
            if (!Player->PlayerCameraManager) { Player->SpawnPlayerCameraManager(); }
            Pawn = SpawnPawn();
            Player->Possess(Pawn);
            Player->SetViewTarget(Pawn);
            Manager = World->SpawnActor<AMovieManager>();
            Manager->PreparationDurationSeconds = 0.1;
            Manager->RecordingDurationSeconds = 0.3;
            Manager->bShowDebugOverlay = false;
            Manager->bBindRestartKey = false;
            Manager->bBindActingKeys = false;
            return Manager;
        }

        UWorld* World = nullptr;
        APlayerController* Player = nullptr;
        APawn* Pawn = nullptr;
        AMovieManager* Manager = nullptr;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMovieConcurrentRecordingTest, "OneManStudio.Movie.ConcurrentRecordingAndReplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMovieConcurrentRecordingTest::RunTest(const FString& Parameters)
{
    MovieManagerTests::FWorld Context;
    APawn* Live = Context.SpawnPawn();
    Live->SetActorTransform(FTransform::Identity);
    UActingRecorderComponent* Recorder = NewObject<UActingRecorderComponent>(Live);
    Live->AddInstanceComponent(Recorder);
    Recorder->RegisterComponent();
    FActingRecording FirstTake;
    FirstTake.Samples.AddDefaulted();
    FActingTransformSample& Last = FirstTake.Samples.AddDefaulted_GetRef();
    Last.TimeSeconds = 1.0;
    Last.WorldTransform.SetLocation(FVector(100.0, 0.0, 0.0));
    AReplayActor* Replay = Context.World->SpawnActor<AReplayActor>();
    UActingSceneClockSubsystem* Clock = Context.World->GetSubsystem<UActingSceneClockSubsystem>();
    Clock->StartClock(0.0);
    TestTrue(TEXT("Replay starts"), Replay->PlayRecording(FirstTake, 0.0));
    TestTrue(TEXT("Second take records concurrently"), Recorder->StartRecording(0.5));
    for (int32 Frame = 0; Frame < 10; ++Frame)
    {
        const double NextTime = Clock->GetSceneTimeSeconds() + 0.025f;
        Live->SetActorLocation(FVector(0.0, NextTime * 200.0, 0.0));
        Context.Tick();
    }
    TestTrue(TEXT("Recorder still active while first actor replays"), Recorder->IsRecording() && Replay->IsPlaying());
    TestTrue(TEXT("First replay follows its own X trajectory"), FMath::IsNearlyEqual(Replay->GetActorLocation().X, 25.0, 0.001));
    TestTrue(TEXT("Second recording has distinct Y movement"), Recorder->GetRecording().Samples.Last().WorldTransform.GetLocation().Y > 40.0);
    for (int32 Frame = 0; Frame < 11; ++Frame)
    {
        const double NextTime = Clock->GetSceneTimeSeconds() + 0.025f;
        Live->SetActorLocation(FVector(0.0, NextTime * 200.0, 0.0));
        Context.Tick();
    }
    TestFalse(TEXT("Second take completes independently"), Recorder->IsRecording());
    TestTrue(TEXT("First replay continues beyond second recording end"), Replay->IsPlaying());
    const FActingRecording SecondTake = Recorder->GetRecording();
    TestTrue(TEXT("Second take data valid"), SecondTake.IsValid());
    TestTrue(TEXT("Second endpoint is exact"), FMath::IsNearlyEqual(SecondTake.GetDurationSeconds(), 0.5, 1.e-9));
    TestTrue(TEXT("First take value data not overwritten"), FirstTake.Samples.Last().WorldTransform.GetLocation().Equals(FVector(100.0, 0.0, 0.0)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMovieThreeTakeLoopTest, "OneManStudio.Movie.ThreeTakeLoop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMovieThreeTakeLoopTest::RunTest(const FString& Parameters)
{
    MovieManagerTests::FWorld Context;
    AMovieManager* Manager = Context.SetupMovie();
    const FTransform Origin = Context.Pawn->GetActorTransform();
    TestTrue(TEXT("Movie starts"), Manager->StartMovie());
    TestTrue(TEXT("Preparation freezes movement"), Context.Player->IsMoveInputIgnored());
    bool bSawSecondTake = false;
    bool bSawThirdTake = false;
    bool bSawPremiere = false;
    for (int32 Frame = 0; Frame < 256 && Manager->State != EMovieState::Finished && Manager->State != EMovieState::Error; ++Frame)
    {
        if (Manager->State == EMovieState::Recording)
        {
            const int32 Index = Manager->CurrentTakeIndex;
            FTransform Pose = Manager->Roles[Index].InitialTransform * Origin;
            Pose.AddToTranslation(FVector(80.0 * (Index + 1), 30.0 * (Index + 1), 0.0)
                * (Manager->GetSceneTimeSeconds() + 0.025f));
            Context.Pawn->SetActorTransform(Pose);
        }
        Context.Tick();
        if (Manager->State == EMovieState::Preparation)
        {
            TestTrue(TEXT("Preparation resets scene time to zero"), FMath::IsNearlyZero(Manager->GetSceneTimeSeconds()));
            for (int32 Index = 0; Index < Manager->ActiveReplays.Num(); ++Index)
            {
                TestTrue(TEXT("Previous takes hold their initial pose during preparation"),
                    Manager->ActiveReplays[Index]->GetActorTransform().Equals(Manager->Performances[Index].InitialTransform, 0.001));
            }
        }
        if (Manager->State == EMovieState::Recording || Manager->State == EMovieState::MoviePremiere)
        {
            if (Manager->State == EMovieState::Recording)
            {
                TestEqual(TEXT("Earlier performances replay while the next records"), Manager->ActiveReplays.Num(), Manager->CurrentTakeIndex);
                TestFalse(TEXT("Movement unlocked during recording"), Context.Player->IsMoveInputIgnored());
                bSawSecondTake |= Manager->CurrentTakeIndex == 1;
                bSawThirdTake |= Manager->CurrentTakeIndex == 2;
            }
            for (int32 Index = 0; Index < Manager->ActiveReplays.Num(); ++Index)
            {
                FTransform Expected;
                Manager->Performances[Index].Recording.Evaluate(Manager->GetSceneTimeSeconds(), Expected);
                TestTrue(TEXT("Each replay evaluates the identical shared scene timestamp"),
                    Manager->ActiveReplays[Index]->GetActorTransform().Equals(Expected, 0.002));
                TestFalse(TEXT("Replay never blocks the live performer"), Manager->ActiveReplays[Index]->GetActorEnableCollision());
            }
        }
        if (Manager->State == EMovieState::MoviePremiere)
        {
            bSawPremiere = true;
            TestEqual(TEXT("Premiere has three replays"), Manager->ActiveReplays.Num(), 3);
            TestTrue(TEXT("Premiere uses fixed camera"), Context.Player->GetViewTarget() == Manager->GetPremiereCamera());
            TestTrue(TEXT("Live actor is hidden during premiere"), Context.Pawn->IsHidden());
            TestTrue(TEXT("Premiere locks camera input"), Context.Player->IsLookInputIgnored());
        }
    }
    TestTrue(TEXT("Visited takes 2 and 3 and the premiere"), bSawSecondTake && bSawThirdTake && bSawPremiere);
    TestTrue(TEXT("Movie reaches Finished"), Manager->State == EMovieState::Finished);
    TestEqual(TEXT("Exactly three separate performances retained"), Manager->Performances.Num(), 3);
    for (int32 Index = 0; Index < Manager->Performances.Num(); ++Index)
    {
        const FMoviePerformance& Performance = Manager->Performances[Index];
        TestEqual(TEXT("Role identifier retained"), Performance.RoleId, Manager->Roles[Index].RoleId);
        TestTrue(TEXT("Initial world transform retained"), Performance.InitialTransform.Equals(Manager->Roles[Index].InitialTransform * Origin));
        TestTrue(TEXT("Each recording starts at zero"), FMath::IsNearlyZero(Performance.Recording.Samples[0].TimeSeconds));
        TestTrue(TEXT("Each recording has configured duration"), FMath::IsNearlyEqual(Performance.Recording.GetDurationSeconds(), 0.3, 1.e-9));
        TestTrue(TEXT("Finished movie holds all three final poses"),
            Manager->ActiveReplays[Index]->GetActorTransform().Equals(Performance.Recording.Samples.Last().WorldTransform, 0.002));
    }
    TestTrue(TEXT("Finished timeline holds exact movie endpoint"), FMath::IsNearlyEqual(Manager->GetSceneTimeSeconds(), 0.3, 1.e-9));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMovieRestartTest, "OneManStudio.Movie.RestartAndCleanup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMovieRestartTest::RunTest(const FString& Parameters)
{
    MovieManagerTests::FWorld Context;
    AMovieManager* Manager = Context.SetupMovie();
    const FTransform Original = Context.Pawn->GetActorTransform();
    Manager->bBindRestartKey = true;
    Context.Player->SetIgnoreLookInput(true); // Another system owns a lock; manager must preserve it.
    Manager->StartMovie();
    for (int32 Frame = 0; Frame < 128 && Manager->State != EMovieState::MoviePremiere; ++Frame) { Context.Tick(); }
    TestTrue(TEXT("Reached premiere before restart"), Manager->State == EMovieState::MoviePremiere);
    ACameraActor* OldCamera = Manager->GetPremiereCamera();
    TArray<TWeakObjectPtr<AReplayActor>> OldReplays;
    for (AReplayActor* Replay : Manager->ActiveReplays) { OldReplays.Add(Replay); }
    TestTrue(TEXT("Restart succeeds from premiere"), Manager->RestartMovie());
    TestTrue(TEXT("Restart returns to preparation"), Manager->State == EMovieState::Preparation);
    TestEqual(TEXT("Restart clears all recorded roles"), Manager->Performances.Num(), 0);
    TestEqual(TEXT("Restart removes all replay references"), Manager->ActiveReplays.Num(), 0);
    TestTrue(TEXT("Owned premiere camera destroyed"), !IsValid(OldCamera));
    for (const TWeakObjectPtr<AReplayActor>& Replay : OldReplays) { TestFalse(TEXT("Old replays destroyed"), Replay.IsValid()); }
    UActingRecorderComponent* Recorder = Context.Pawn->FindComponentByClass<UActingRecorderComponent>();
    TestTrue(TEXT("Recorder data also cleared during preparation"), Recorder->GetRecording().Samples.IsEmpty());
    TestTrue(TEXT("Restart returns player to initial transform"), Context.Pawn->GetActorTransform().Equals(Original));
    TestFalse(TEXT("Restart restores visible player"), Context.Pawn->IsHidden());
    TestTrue(TEXT("Restart restores player camera"), Context.Player->GetViewTarget() == Context.Pawn);
    for (int32 Frame = 0; Frame < 16 && Manager->State != EMovieState::Recording; ++Frame) { Context.Tick(); }
    Context.Tick();
    Context.Tick();
    Context.Tick();
    TestTrue(TEXT("Recording contains new samples before cancellation"), Recorder->GetRecording().Samples.Num() > 1);
    TestTrue(TEXT("Restart cancels an in-progress recording"), Manager->RestartMovie());
    TestTrue(TEXT("Cancellation does not store a partial take"), Manager->Performances.IsEmpty());
    TestTrue(TEXT("Cancellation clears component sample buffer"), Recorder->GetRecording().Samples.IsEmpty());
    const UInputComponent* RestartInput = Manager->FindComponentByClass<UInputComponent>();
    TestTrue(TEXT("Repeated restarts bind restart and interaction once each"), RestartInput && RestartInput->KeyBindings.Num() == 2);
    Manager->StopMovie();
    TestTrue(TEXT("Stop returns Idle"), Manager->State == EMovieState::Idle);
    TestFalse(TEXT("Stop releases only manager's movement lock"), Context.Player->IsMoveInputIgnored());
    TestTrue(TEXT("External look lock survives restart and stop"), Context.Player->IsLookInputIgnored());
    Context.Player->SetIgnoreLookInput(false);
    TestFalse(TEXT("All look locks balanced"), Context.Player->IsLookInputIgnored());
    TestTrue(TEXT("Stop restores initial player transform"), Context.Pawn->GetActorTransform().Equals(Original));
    TestTrue(TEXT("Stop leaves scene clock at zero"), FMath::IsNearlyZero(Context.World->GetSubsystem<UActingSceneClockSubsystem>()->GetSceneTimeSeconds()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMovieDefaultTimingTest, "OneManStudio.Movie.DefaultTimingAndFinishedRestart",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMovieDefaultTimingTest::RunTest(const FString& Parameters)
{
    MovieManagerTests::FWorld Context;
    AMovieManager* Manager = Context.SetupMovie();
    const AMovieManager* Defaults = GetDefault<AMovieManager>();
    TestTrue(TEXT("Default preparation is ten seconds"), FMath::IsNearlyEqual(Defaults->PreparationDurationSeconds, 10.0));
    TestTrue(TEXT("Default recording is thirty seconds"), FMath::IsNearlyEqual(Defaults->RecordingDurationSeconds, 30.0));
    Manager->PreparationDurationSeconds = Defaults->PreparationDurationSeconds;
    Manager->RecordingDurationSeconds = Defaults->RecordingDurationSeconds;
    Manager->StartMovie();
    for (int32 Frame = 0; Frame < 1800 && Manager->State != EMovieState::Finished && Manager->State != EMovieState::Error; ++Frame)
    {
        Context.Tick(0.1f);
    }
    TestTrue(TEXT("Full 10/30-second loop and thirty-second premiere finish"), Manager->State == EMovieState::Finished);
    TestEqual(TEXT("Full loop saves three takes"), Manager->Performances.Num(), 3);
    for (const FMoviePerformance& Performance : Manager->Performances)
    {
        TestEqual(TEXT("Thirty seconds at 20 Hz includes initial plus 600 samples"), Performance.Recording.Samples.Num(), 601);
        TestTrue(TEXT("Thirty-second endpoint is exact"), FMath::IsNearlyEqual(Performance.Recording.GetDurationSeconds(), 30.0, 1.e-9));
    }
    TestTrue(TEXT("Restart from Finished succeeds"), Manager->RestartMovie());
    TestTrue(TEXT("Finished restart begins take 1 preparation"), Manager->State == EMovieState::Preparation && Manager->GetCurrentTakeNumber() == 1);
    TestTrue(TEXT("Finished restart clears all takes"), Manager->Performances.IsEmpty());
    TestTrue(TEXT("Finished restart clears timeline"), FMath::IsNearlyZero(Manager->GetSceneTimeSeconds()));
    Manager->StopMovie();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMovieCharacterMovementTest, "OneManStudio.Movie.CharacterMovementLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMovieCharacterMovementTest::RunTest(const FString& Parameters)
{
    MovieManagerTests::FWorld Context;
    AActor* Floor = Context.World->SpawnActor<AActor>();
    UBoxComponent* FloorShape = NewObject<UBoxComponent>(Floor);
    Floor->AddInstanceComponent(FloorShape);
    Floor->SetRootComponent(FloorShape);
    FloorShape->SetBoxExtent(FVector(10000.0, 10000.0, 50.0));
    FloorShape->SetCollisionProfileName(TEXT("BlockAll"));
    FloorShape->RegisterComponent();
    Floor->SetActorLocation(FVector(0.0, 0.0, -50.0));
    APlayerController* Player = Context.World->SpawnActor<APlayerController>();
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActingRecorderTestCharacter* Character = Context.World->SpawnActor<AActingRecorderTestCharacter>(
        FVector(0.0, 0.0, 96.0), FRotator::ZeroRotator, SpawnParameters);
    Player->Possess(Character);
    const FTransform InitialPose = Character->GetActorTransform();
    AMovieManager* Manager = Context.World->SpawnActor<AMovieManager>();
    Manager->PreparationDurationSeconds = 0.1;
    Manager->RecordingDurationSeconds = 0.5;
    Manager->bBindRestartKey = false;
    Manager->bBindActingKeys = false;
    Manager->bShowDebugOverlay = false;
    const bool bInitiallyActive = Character->GetCharacterMovement()->IsActive();
    TestTrue(TEXT("Character movie starts"), Manager->StartMovie());
    Character->Jump();
    Character->AddMovementInput(FVector::ForwardVector, 1.0f);
    Context.Tick();
    TestTrue(TEXT("Preparation prevents movement and jumping"), Character->GetActorTransform().Equals(InitialPose, 0.01));
    TestFalse(TEXT("Character movement disabled during preparation"), Character->GetCharacterMovement()->IsActive());
    for (int32 Frame = 0; Frame < 16 && Manager->State != EMovieState::Recording; ++Frame) { Context.Tick(); }
    TestTrue(TEXT("Character movement restored for action"), Character->GetCharacterMovement()->IsActive() == bInitiallyActive);
    for (int32 Frame = 0; Frame < 64 && Manager->State == EMovieState::Recording && Manager->CurrentTakeIndex == 0; ++Frame)
    {
        Character->AddMovementInput(FVector::ForwardVector, 1.0f);
        Context.Tick();
    }
    TestEqual(TEXT("A moving character produced the first take"), Manager->Performances.Num(), 1);
    if (!Manager->Performances.IsEmpty())
    {
        TestTrue(TEXT("Recording captures actual CharacterMovement displacement"),
            Manager->Performances[0].Recording.Samples.Last().WorldTransform.GetLocation().X > InitialPose.GetLocation().X + 10.0);
    }
    TestTrue(TEXT("Next take returns the character to its own start"),
        Character->GetActorTransform().Equals(Manager->Roles[1].InitialTransform * InitialPose, 0.01));
    TestFalse(TEXT("Character is frozen again for the next preparation"), Character->GetCharacterMovement()->IsActive());
    Manager->StopMovie();
    TestTrue(TEXT("Stop restores original CharacterMovement activation"), Character->GetCharacterMovement()->IsActive() == bInitiallyActive);
    TestFalse(TEXT("Stop releases movement input"), Player->IsMoveInputIgnored());
    TestTrue(TEXT("Stop restores original character pose"), Character->GetActorTransform().Equals(InitialPose, 0.01));
    return true;
}

#endif
