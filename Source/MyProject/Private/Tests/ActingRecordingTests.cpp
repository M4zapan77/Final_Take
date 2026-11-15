#if WITH_DEV_AUTOMATION_TESTS

#include "ActingRecording.h"
#include "ActingRecorderComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "ReplayActor.h"

#include "CollisionShape.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"

namespace ActingRecordingTests
{
    FActingRecording MakeLinearRecording()
    {
        FActingRecording Recording;
        FActingTransformSample& First = Recording.Samples.AddDefaulted_GetRef();
        First.WorldTransform = FTransform(FRotator(0.0, 170.0, 0.0), FVector::ZeroVector);
        FActingTransformSample& Last = Recording.Samples.AddDefaulted_GetRef();
        Last.TimeSeconds = 1.0;
        Last.WorldTransform = FTransform(FRotator(0.0, -170.0, 0.0), FVector(100.0, 0.0, 0.0));
        return Recording;
    }

    /** Transient world only; no maps/assets are created or saved by these tests. */
    class FTestWorld
    {
    public:
        FTestWorld()
        {
            UWorld::InitializationValues Values;
            Values.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true)
                .CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false)
                .ShouldSimulatePhysics(false).SetTransactional(false);
            const FName Name = MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("ActingTestWorld"));
            World = UWorld::CreateWorld(EWorldType::Game, false, Name, nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World)
            {
                GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
                World->InitializeActorsForPlay(FURL());
                // There is no game mode in this isolated world; dispatch BeginPlay explicitly.
                World->GetWorldSettings()->NotifyBeginPlay();
            }
        }

        ~FTestWorld()
        {
            if (World)
            {
                World->EndPlay(EEndPlayReason::Quit);
                World->DestroyWorld(false);
                GEngine->DestroyWorldContext(World);
            }
        }

        void Tick(float DeltaSeconds)
        {
            // A synchronous test runs many simulated frames inside one editor frame.
            // Tick functions use GFrameCounter to avoid running twice in the same frame.
            ++GFrameCounter;
            World->Tick(LEVELTICK_All, DeltaSeconds);
        }

        UWorld* World = nullptr;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActingRecordingDataTest, "OneManStudio.Acting.RecordingData",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FActingRecordingDataTest::RunTest(const FString& Parameters)
{
    FActingRecording Empty;
    FTransform Pose;
    TestFalse(TEXT("Empty recording is invalid"), Empty.IsValid());
    TestFalse(TEXT("Empty recording cannot be evaluated"), Empty.Evaluate(0.0, Pose));

    FActingRecording Recording = ActingRecordingTests::MakeLinearRecording();
    TestTrue(TEXT("Ordered finite samples are valid"), Recording.IsValid());
    TestTrue(TEXT("Can evaluate midpoint"), Recording.Evaluate(0.5, Pose));
    TestTrue(TEXT("Translation interpolates to midpoint"), Pose.GetLocation().Equals(FVector(50.0, 0.0, 0.0), 0.001));
    TestTrue(TEXT("Rotation takes short arc across +/-180 degrees"),
        Pose.GetRotation().Equals(FRotator(0.0, 180.0, 0.0).Quaternion(), 0.001));
    Recording.Evaluate(-5.0, Pose);
    TestTrue(TEXT("Negative time holds the first sample"), Pose.Equals(Recording.Samples[0].WorldTransform));
    Recording.Evaluate(5.0, Pose);
    TestTrue(TEXT("Time beyond duration holds the final sample"), Pose.Equals(Recording.Samples.Last().WorldTransform));

    Recording.Samples.Last().TimeSeconds = 0.0;
    TestFalse(TEXT("Duplicate timestamps rejected"), Recording.IsValid());
    Recording.Samples.Last().TimeSeconds = -1.0;
    TestFalse(TEXT("Reversed timestamps rejected"), Recording.IsValid());
    Recording = ActingRecordingTests::MakeLinearRecording();
    Recording.Samples[0].TimeSeconds = 0.1;
    TestFalse(TEXT("Nonzero first timestamp rejected"), Recording.IsValid());
    Recording.Samples.SetNum(1);
    Recording.Samples[0].TimeSeconds = 0.0;
    TestTrue(TEXT("Immediate stop is a valid single-pose take"), Recording.IsValid());
    TestTrue(TEXT("Single-pose take evaluates without division by zero"), Recording.Evaluate(0.5, Pose));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActingRecorderSamplingTest, "OneManStudio.Acting.RecorderSampling",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FActingRecorderSamplingTest::RunTest(const FString& Parameters)
{
    ActingRecordingTests::FTestWorld TestWorld;
    UWorld* World = TestWorld.World;
    if (!TestNotNull(TEXT("Transient world"), World)) { return false; }
    AActor* Performer = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Performer"), Performer)) { return false; }
    USceneComponent* Root = NewObject<USceneComponent>(Performer);
    Performer->AddInstanceComponent(Root);
    Performer->SetRootComponent(Root);
    Root->RegisterComponent();
    Performer->SetActorLocation(FVector(0.0, 0.0, 120.0));
    UActingRecorderComponent* Recorder = NewObject<UActingRecorderComponent>(Performer);
    Performer->AddInstanceComponent(Recorder);
    Recorder->RegisterComponent();

    const double StartTime = World->GetTimeSeconds();
    TestTrue(TEXT("Starts a ten-second take"), Recorder->StartRecording(10.0));
    TestFalse(TEXT("Active take cannot be silently overwritten"), Recorder->StartRecording(1.0));
    Recorder->SampleRateHz = 5.0f; // Does not change a take that has already started at 20 Hz.
    const float FrameTimes[] = { 0.016f, 0.085f, 0.033f, 0.12f };
    for (int32 Frame = 0; Frame < 512 && Recorder->IsRecording(); ++Frame)
    {
        const float Delta = FrameTimes[Frame % UE_ARRAY_COUNT(FrameTimes)];
        const double NextTakeTime = World->GetTimeSeconds() + Delta - StartTime;
        Performer->SetActorLocation(FVector(NextTakeTime * 100.0, 0.0, 120.0));
        Performer->SetActorRotation(FRotator(0.0, NextTakeTime * 9.0, 0.0));
        TestWorld.Tick(Delta);
    }

    TestFalse(TEXT("Automatically stops after ten seconds"), Recorder->IsRecording());
    const FActingRecording TenSecondTake = Recorder->GetRecording();
    TestTrue(TEXT("Recorded data is reusable and valid"), TenSecondTake.IsValid());
    TestEqual(TEXT("20 Hz plus the initial sample"), TenSecondTake.Samples.Num(), 201);
    TestTrue(TEXT("Exact ten-second endpoint"), FMath::IsNearlyEqual(TenSecondTake.GetDurationSeconds(), 10.0, 1.e-9));
    for (int32 Index = 0; Index < TenSecondTake.Samples.Num(); ++Index)
    {
        const FActingTransformSample& Sample = TenSecondTake.Samples[Index];
        TestTrue(FString::Printf(TEXT("Sample %d is on the 50 ms grid"), Index),
            FMath::IsNearlyEqual(Sample.TimeSeconds, Index * 0.05, 1.e-9));
        TestTrue(FString::Printf(TEXT("Sample %d reconstructs motion across variable frame times"), Index),
            FMath::IsNearlyEqual(Sample.WorldTransform.GetLocation().X, Sample.TimeSeconds * 100.0, 0.005));
        TestTrue(FString::Printf(TEXT("Sample %d reconstructs rotation"), Index),
            Sample.WorldTransform.GetRotation().Equals(FRotator(0.0, Sample.TimeSeconds * 9.0, 0.0).Quaternion(), 0.001));
    }
    TestFalse(TEXT("Stopping twice is harmless"), Recorder->StopRecording());

    Recorder->SampleRateHz = 20.0f;
    Performer->SetActorTransform(FTransform::Identity);
    const double ManualStart = World->GetTimeSeconds();
    TestTrue(TEXT("Zero duration enables manual stop"), Recorder->StartRecording(0.0));
    const float ManualDelta = 0.077f;
    Performer->SetActorLocation(FVector(ManualDelta * 100.0, 0.0, 0.0));
    TestWorld.Tick(ManualDelta);
    TestTrue(TEXT("Manual stop succeeds between grid samples"), Recorder->StopRecording());
    const FActingRecording ManualTake = Recorder->GetRecording();
    TestEqual(TEXT("Manual take has initial, 50 ms, and exact final samples"), ManualTake.Samples.Num(), 3);
    TestTrue(TEXT("Manual endpoint matches world time"),
        FMath::IsNearlyEqual(ManualTake.GetDurationSeconds(), World->GetTimeSeconds() - ManualStart, 1.e-9));
    TestEqual(TEXT("Saved take remains independent when recording again"), TenSecondTake.Samples.Num(), 201);
    Recorder->StartRecording(0.0);
    Recorder->StopRecording();
    TestTrue(TEXT("Immediate stop stays valid"), Recorder->GetRecording().IsValid());
    TestEqual(TEXT("Immediate stop does not duplicate time zero"), Recorder->GetRecording().Samples.Num(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActingSharedClockReplayTest, "OneManStudio.Acting.SharedClockAndReplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FActingSharedClockReplayTest::RunTest(const FString& Parameters)
{
    ActingRecordingTests::FTestWorld TestWorld;
    UWorld* World = TestWorld.World;
    if (!TestNotNull(TEXT("Transient world"), World)) { return false; }
    UActingSceneClockSubsystem* Clock = World->GetSubsystem<UActingSceneClockSubsystem>();
    AReplayActor* First = World->SpawnActor<AReplayActor>();
    AReplayActor* Second = World->SpawnActor<AReplayActor>();
    if (!TestNotNull(TEXT("Shared clock"), Clock) || !TestNotNull(TEXT("First replay"), First)
        || !TestNotNull(TEXT("Second replay"), Second)) { return false; }
    const FActingRecording Recording = ActingRecordingTests::MakeLinearRecording();
    Clock->StartClock(0.0);
    TestTrue(TEXT("First replay accepts recording"), First->PlayRecording(Recording, 0.0));
    TestTrue(TEXT("Second replay accepts the same scene epoch"), Second->PlayRecording(Recording, 0.0));
    TestWorld.Tick(0.25f);
    TestTrue(TEXT("Quarter-second position is interpolated"), First->GetActorLocation().Equals(FVector(25.0, 0.0, 0.0), 0.001));
    TestTrue(TEXT("Both actors have identical poses from the shared clock"), First->GetActorTransform().Equals(Second->GetActorTransform()));

    Clock->PauseClock();
    TestWorld.Tick(0.2f);
    TestTrue(TEXT("Scene pause freezes replay while the world continues"), First->GetActorLocation().Equals(FVector(25.0, 0.0, 0.0), 0.001));
    Clock->ResumeClock();
    World->GetWorldSettings()->SetTimeDilation(0.5f);
    TestWorld.Tick(0.2f);
    TestTrue(TEXT("Shared clock follows game time dilation"), FMath::IsNearlyEqual(Clock->GetSceneTimeSeconds(), 0.35, 0.001));
    World->GetWorldSettings()->SetTimeDilation(1.0f);
    TestFalse(TEXT("Malformed recording rejected without replacing playback"), First->PlayRecording(FActingRecording(), 0.0));
    TestTrue(TEXT("Existing replay remains active after rejected data"), First->IsPlaying());

    First->StopPlayback();
    TestTrue(TEXT("Stopping one replay restores its initial pose"), First->GetActorTransform().Equals(Recording.Samples[0].WorldTransform));
    TestTrue(TEXT("Stopping one replay leaves the shared clock running"), Clock->IsClockRunning());
    for (int32 Frame = 0; Frame < 8; ++Frame) { TestWorld.Tick(0.1f); }
    TestFalse(TEXT("Other replay completes independently"), Second->IsPlaying());
    TestTrue(TEXT("Finished replay holds the exact final pose"), Second->GetActorTransform().Equals(Recording.Samples.Last().WorldTransform));

    Clock->StartClock(0.0);
    First->PlayRecording(Recording, 0.2);
    TestWorld.Tick(0.1f);
    TestTrue(TEXT("Scheduled replay holds first pose before its start time"), First->GetActorTransform().Equals(Recording.Samples[0].WorldTransform));
    TestWorld.Tick(0.2f);
    TestTrue(TEXT("Scheduled replay evaluates relative to the same clock"), FMath::IsNearlyEqual(First->GetActorLocation().X, 10.0, 0.001));
    Clock->StopClock();
    TestFalse(TEXT("StopClock disables the clock"), Clock->IsClockRunning());
    TestTrue(TEXT("StopClock resets scene time"), FMath::IsNearlyZero(Clock->GetSceneTimeSeconds()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActingReplayCollisionTest, "OneManStudio.Acting.ReplayCollision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FActingReplayCollisionTest::RunTest(const FString& Parameters)
{
    ActingRecordingTests::FTestWorld TestWorld;
    UWorld* World = TestWorld.World;
    if (!TestNotNull(TEXT("Transient world"), World)) { return false; }
    AReplayActor* Replay = World->SpawnActor<AReplayActor>();
    if (!TestNotNull(TEXT("Replay"), Replay)) { return false; }
    // Model a visual added dynamically or by a ReplayActor Blueprint with collision enabled.
    UBoxComponent* Extra = NewObject<UBoxComponent>(Replay);
    Replay->AddInstanceComponent(Extra);
    Extra->SetupAttachment(Replay->GetRootComponent());
    Extra->SetBoxExtent(FVector(40.0));
    Extra->RegisterComponent();
    Extra->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Extra->SetGenerateOverlapEvents(true);
    Extra->SetSimulatePhysics(true);
    TestTrue(TEXT("PlayRecording accepts a valid take"), Replay->PlayRecording(ActingRecordingTests::MakeLinearRecording(), 0.0));
    TestFalse(TEXT("Actor collision disabled"), Replay->GetActorEnableCollision());
    TestTrue(TEXT("Placeholder collision disabled"), Replay->PlaceholderMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
    TestTrue(TEXT("Extra component collision disabled"), Extra->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
    TestFalse(TEXT("Extra component generates no overlaps"), Extra->GetGenerateOverlapEvents());
    TestFalse(TEXT("Extra component no longer simulates physics"), Extra->IsSimulatingPhysics());
    TestFalse(TEXT("Extra component cannot affect navigation"), Extra->CanEverAffectNavigation());

    FHitResult Hit;
    const bool bBlocked = World->SweepSingleByChannel(Hit, FVector(-100.0, 0.0, 0.0), FVector(200.0, 0.0, 0.0),
        FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(20.0f));
    TestFalse(TEXT("A player-sized sweep can pass through all replay geometry"), bBlocked);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActingIdleCompactionTest,"OneManStudio.Acting.CompactIdleWithoutEarlyMotion",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FActingIdleCompactionTest::RunTest(const FString& Parameters)
{
    ActingRecordingTests::FTestWorld C;auto* Actor=C.World->SpawnActor<AActor>();auto* Root=NewObject<USceneComponent>(Actor);Actor->AddInstanceComponent(Root);Actor->SetRootComponent(Root);Root->RegisterComponent();
    auto* Recorder=NewObject<UActingRecorderComponent>(Actor);Actor->AddInstanceComponent(Recorder);Recorder->RegisterComponent();Recorder->bCompactStationarySamples=true;
    TestTrue(TEXT("Compact recorder starts"),Recorder->StartRecording(0));for(int32 Index=0;Index<20;++Index){C.Tick(.05f);}
    TestEqual(TEXT("One second stationary retains one start pose"),Recorder->GetRecordingData().Samples.Num(),1);
    Actor->SetActorLocation(FVector(100,0,0));C.Tick(.05f);Recorder->StopRecording();FTransform Pose;
    const auto& Recording=Recorder->GetRecordingData();TestTrue(TEXT("Compact recording retains a valid exact duration"),Recording.IsValid() && Recording.GetDurationSeconds()>1);
    Recording.Evaluate(.75,Pose);TestTrue(TEXT("Idle actor does not drift before actual movement"),Pose.GetLocation().IsNearlyZero());
    Recording.Evaluate(Recording.GetDurationSeconds(),Pose);TestTrue(TEXT("Movement endpoint retained"),Pose.GetLocation().Equals(FVector(100,0,0),.001));
    TestTrue(TEXT("Idle compaction saves movement storage"),Recording.Samples.Num()<8);
    Recorder->ClearRecording();Recorder->StartRecording(.5);for(int32 Index=0;Index<20 && Recorder->IsRecording();++Index){C.Tick(.05f);}
    TestTrue(TEXT("An entirely idle take still ends at its configured duration"),Recorder->GetRecordingData().IsValid() && FMath::IsNearlyEqual(Recorder->GetRecordedDurationSeconds(),.5));return true;
}
#endif // WITH_DEV_AUTOMATION_TESTS
