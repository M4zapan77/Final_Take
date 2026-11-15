#if WITH_DEV_AUTOMATION_TESTS

#include "ActingComponent.h"
#include "ActingRecorderComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "MovieManager.h"
#include "ReplayActor.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include <limits>

namespace ActingEventTests
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
                MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("ActingEventsWorld")),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            World->InitializeActorsForPlay(FURL());
            World->GetWorldSettings()->NotifyBeginPlay();
            Clock = World->GetSubsystem<UActingSceneClockSubsystem>();
        }

        ~FWorld()
        {
            World->EndPlay(EEndPlayReason::Quit);
            World->DestroyWorld(false);
            GEngine->DestroyWorldContext(World);
        }

        void Tick(float Delta = 0.025f) { ++GFrameCounter; World->Tick(LEVELTICK_All, Delta); }

        APawn* MakeLive()
        {
            APawn* Pawn = World->SpawnActor<APawn>();
            USceneComponent* Root = NewObject<USceneComponent>(Pawn);
            Pawn->AddInstanceComponent(Root);
            Pawn->SetRootComponent(Root);
            Root->RegisterComponent();
            Recorder = NewObject<UActingRecorderComponent>(Pawn);
            Pawn->AddInstanceComponent(Recorder);
            Recorder->RegisterComponent();
            Acting = NewObject<UActingComponent>(Pawn);
            Pawn->AddInstanceComponent(Acting);
            Acting->RegisterComponent();
            return Pawn;
        }

        UWorld* World = nullptr;
        UActingSceneClockSubsystem* Clock = nullptr;
        UActingRecorderComponent* Recorder = nullptr;
        UActingComponent* Acting = nullptr;
    };

    void AddEvent(FActingRecording& Recording, double Time, EActingEventType Type, FName Identifier)
    {
        FActingEvent& Event = Recording.Events.AddDefaulted_GetRef();
        Event.TimeSeconds = Time;
        Event.Type = Type;
        Event.Identifier = Identifier;
    }

    FActingRecording MakeTake()
    {
        FActingRecording Take;
        Take.Samples.AddDefaulted();
        FActingTransformSample& End = Take.Samples.AddDefaulted_GetRef();
        End.TimeSeconds = 1.0;
        End.WorldTransform.SetLocation(FVector(100.0, 0.0, 0.0));
        Take.ActingSettings = GetDefault<UActingComponent>()->CapturePlaybackSettings();
        AddEvent(Take, 0.0, EActingEventType::Gesture, TEXT("Dance"));
        AddEvent(Take, 0.1, EActingEventType::Sound, TEXT("EvilLaugh"));
        AddEvent(Take, 0.1, EActingEventType::Gesture, TEXT("Point"));
        AddEvent(Take, 0.4, EActingEventType::Sound, TEXT("Scream"));
        AddEvent(Take, 1.0, EActingEventType::StopGesture, TEXT("Point"));
        return Take;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActingEventDataTest, "OneManStudio.ActingEvents.DataValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FActingEventDataTest::RunTest(const FString& Parameters)
{
    FActingRecording Take = ActingEventTests::MakeTake();
    TestTrue(TEXT("Equal timestamps and an endpoint event are valid"), Take.IsValid());
    FActingRecording Copy = Take;
    Copy.Events[0].Identifier = TEXT("Scared");
    Copy.ActingSettings.Gestures[0].DurationSeconds = 8.0f;
    TestEqual(TEXT("Copies keep independent event identifiers"), Take.Events[0].Identifier, FName(TEXT("Dance")));
    TestEqual(TEXT("Copies keep independent settings"), Take.ActingSettings.Gestures[0].DurationSeconds, 3.0f);
    const auto Reject = [&](double Timestamp)
    {
        FActingRecording Invalid = Take;
        Invalid.Events[1].TimeSeconds = Timestamp;
        TestFalse(TEXT("Invalid event timestamp rejected"), Invalid.IsValid());
    };
    Reject(-0.1);
    Reject(1.1);
    Reject(std::numeric_limits<double>::quiet_NaN());
    Reject(std::numeric_limits<double>::infinity());
    Copy = Take;
    Copy.Events[2].TimeSeconds = 0.05;
    TestFalse(TEXT("Out of order events rejected"), Copy.IsValid());
    Copy = Take;
    Copy.Events[0].Identifier = NAME_None;
    TestFalse(TEXT("Empty identifier rejected"), Copy.IsValid());
    Copy = Take;
    Copy.Events[0].Type = static_cast<EActingEventType>(255);
    TestFalse(TEXT("Unknown event type rejected"), Copy.IsValid());
    Copy.Events.Reset();
    TestTrue(TEXT("Legacy movement-only recordings remain valid"), Copy.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActingEventCaptureTest, "OneManStudio.ActingEvents.CaptureAndGating",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FActingEventCaptureTest::RunTest(const FString& Parameters)
{
    ActingEventTests::FWorld Context;
    Context.MakeLive();
    TestEqual(TEXT("Five gesture defaults"), Context.Acting->Gestures.Num(), 5);
    TestEqual(TEXT("Six soundboard defaults"), Context.Acting->Sounds.Num(), 6);
    TestFalse(TEXT("Input outside recording is gated"), Context.Acting->TriggerGesture(EActingGesture::Dance));
    TestTrue(TEXT("Shared-clock recording starts"), Context.Recorder->StartRecording(1.0, true));
    Context.Clock->StartClock();
    Context.Acting->Gestures[0].DurationSeconds = 0.01f;
    TestTrue(TEXT("Dance accepted at zero"), Context.Acting->TriggerGesture(EActingGesture::Dance));
    Context.Tick(0.15f);
    TestEqual(TEXT("Active take uses its snapshotted gesture settings"), Context.Acting->ActiveGestureId, FName(TEXT("Dance")));
    TestTrue(TEXT("Laugh accepted"), Context.Acting->TriggerSound(EActingSound::EvilLaugh));
    Context.Clock->PauseClock();
    const double PausedTime = Context.Clock->GetSceneTimeSeconds();
    Context.Tick(0.1f);
    TestTrue(TEXT("A deliberately triggered sound while paused retains scene time"), Context.Acting->TriggerSound(EActingSound::Scream));
    TestEqual(TEXT("Event uses the shared timeline, not advancing world time"), Context.Recorder->GetRecordingData().Events.Last().TimeSeconds, PausedTime);
    Context.Clock->ResumeClock();
    Context.Tick(0.1f);
    TestTrue(TEXT("Explicit gesture cancellation is recorded"), Context.Acting->StopGesture());
    for (EActingGesture Gesture : {EActingGesture::Point, EActingGesture::Scared, EActingGesture::Faint, EActingGesture::Arrest})
    {
        TestTrue(TEXT("Configured initial gesture accepted"), Context.Acting->TriggerGesture(Gesture));
    }
    for (EActingSound Sound : {EActingSound::Cry, EActingSound::PoliceSiren, EActingSound::Kiss, EActingSound::Failure})
    {
        TestTrue(TEXT("Configured initial sound accepted"), Context.Acting->TriggerSound(Sound));
    }
    TestFalse(TEXT("Unknown gesture rejected"), Context.Acting->TriggerGestureById(TEXT("Unknown")));
    TestFalse(TEXT("Unknown sound rejected"), Context.Acting->TriggerSoundById(TEXT("Unknown")));
    Context.Acting->SetLiveActionsEnabled(false);
    TestFalse(TEXT("Disabled HUD input cannot append"), Context.Acting->TriggerSound(EActingSound::Cry));
    Context.Recorder->StopRecording();
    const FActingRecording Saved = Context.Recorder->GetRecording();
    TestEqual(TEXT("Every accepted live event appended once"), Saved.Events.Num(), 12);
    TestTrue(TEXT("Completed take is valid"), Saved.IsValid());
    TestEqual(TEXT("Time zero is exact"), Saved.Events[0].TimeSeconds, 0.0);
    TestEqual(TEXT("Sound timestamp is independent of the 20 Hz movement grid"), Saved.Events[1].TimeSeconds, PausedTime);
    Context.Acting->StopAllActions();
    TestEqual(TEXT("Cleanup clears active gesture"), Context.Acting->ActiveGestureId, NAME_None);
    Context.Recorder->ClearRecording();
    TestTrue(TEXT("Clearing recording clears event data and settings"), Context.Recorder->GetRecordingData().Events.IsEmpty()
        && Context.Recorder->GetRecordingData().ActingSettings.Gestures.IsEmpty());
    TestEqual(TEXT("Saved value data survives clearing source"), Saved.Events.Num(), 12);
    AReplayActor* Ghost = Context.World->SpawnActor<AReplayActor>();
    UActingRecorderComponent* GhostRecorder = NewObject<UActingRecorderComponent>(Ghost);
    Ghost->AddInstanceComponent(GhostRecorder);
    GhostRecorder->RegisterComponent();
    TestFalse(TEXT("A replay cannot start live recording even if a Blueprint adds a recorder"), GhostRecorder->StartRecording());
    Ghost->Acting->bRequireActiveRecording = false;
    Ghost->Acting->SetLiveActionsEnabled(true);
    TestFalse(TEXT("Replay cannot enter live input path"), Ghost->Acting->TriggerGesture(EActingGesture::Dance));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActingEventPlaybackTest, "OneManStudio.ActingEvents.PlaybackTimeline",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FActingEventPlaybackTest::RunTest(const FString& Parameters)
{
    ActingEventTests::FWorld Context;
    const FActingRecording Take = ActingEventTests::MakeTake();
    AReplayActor* First = Context.World->SpawnActor<AReplayActor>();
    AReplayActor* Second = Context.World->SpawnActor<AReplayActor>();
    TestTrue(TEXT("Preparation accepts data"), First->PrepareRecording(Take));
    Context.Tick();
    TestEqual(TEXT("Preparation dispatches no time-zero gesture/sound"), First->Acting->DispatchedEventCount, 0);
    TestEqual(TEXT("Preparation resets cursor"), First->GetNextEventIndex(), 0);
    Context.Clock->StartClock();
    First->PlayRecording(Take);
    Second->PlayRecording(Take);
    TestEqual(TEXT("Time zero dispatched immediately once"), First->Acting->DispatchedEventCount, 1);
    Context.Clock->PauseClock();
    for (int32 Frame = 0; Frame < 5; ++Frame) { Context.Tick(); }
    TestEqual(TEXT("Paused frames never duplicate time zero"), First->Acting->DispatchedEventCount, 1);
    Context.Clock->ResumeClock();
    Context.Tick(0.25f);
    TestEqual(TEXT("One hitch catches both equal-time events"), First->Acting->DispatchedEventCount, 3);
    TestEqual(TEXT("Equal-time order retained"), First->Acting->LastTriggeredEvent.Identifier, FName(TEXT("Point")));
    TestEqual(TEXT("Original event timestamp retained after hitch"), First->Acting->LastTriggeredEvent.TimeSeconds, 0.1);
    TestEqual(TEXT("Second actor has independent cursor"), Second->Acting->DispatchedEventCount, 3);
    First->StopPlayback();
    TestEqual(TEXT("Stopping clears first cursor"), First->GetNextEventIndex(), 0);
    TestEqual(TEXT("Stopping clears first presentation count"), First->Acting->DispatchedEventCount, 0);
    TestEqual(TEXT("Stopping first does not reset second"), Second->GetNextEventIndex(), 3);
    Context.Tick(0.2f);
    TestEqual(TEXT("Second actor continues its own soundboard"), Second->Acting->DispatchedEventCount, 4);
    TestEqual(TEXT("Stopped actor cannot dispatch more events"), First->Acting->DispatchedEventCount, 0);
    Context.Clock->PauseClock();
    Context.Tick();
    Context.Tick();
    TestEqual(TEXT("Pausing at an event holds its cursor"), Second->Acting->DispatchedEventCount, 4);
    Context.Clock->StartClock(0.02);
    Context.Tick(0.025f);
    TestEqual(TEXT("Backward seek starts one fresh pass"), Second->Acting->DispatchedEventCount, 1);
    Context.Clock->StartClock(1.0);
    Context.Tick();
    TestEqual(TEXT("Endpoint event included exactly once"), Second->Acting->DispatchedEventCount, 5);
    TestFalse(TEXT("Replay finishes at endpoint"), Second->IsPlaying());
    Context.Tick();
    TestEqual(TEXT("Finished frames never re-dispatch"), Second->Acting->DispatchedEventCount, 5);
    Context.Clock->StopClock();
    Second->PlayRecording(Take);
    TestEqual(TEXT("New take/premiere resets event pass"), Second->Acting->DispatchedEventCount, 1);
    FActingRecording Invalid = Take;
    Invalid.Events[0].TimeSeconds = -1.0;
    TestFalse(TEXT("Invalid data does not replace active playback"), Second->PlayRecording(Invalid));
    TestEqual(TEXT("Rejected data did not dispatch or reset"), Second->Acting->DispatchedEventCount, 1);
    TestFalse(TEXT("Event playback retains collision isolation"), Second->GetActorEnableCollision());
    int32 CallbackCount = 0;
    First->Acting->OnActingEventTriggeredNative.AddLambda([&](const FActingEvent& Event, bool bReplay)
    {
        ++CallbackCount;
        TestTrue(TEXT("Callback identifies replay origin"), bReplay);
        First->StopPlayback();
    });
    First->PlayRecording(Take);
    Context.Tick(0.25f);
    TestEqual(TEXT("A listener stopping playback at time zero prevents later callbacks"), CallbackCount, 1);
    TestFalse(TEXT("Reentrant stop leaves replay stopped"), First->IsPlaying());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecordedFootstepTimelineTest,"OneManStudio.ActingEvents.RecordedFootstepTimeline",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecordedFootstepTimelineTest::RunTest(const FString& Parameters)
{
    ActingEventTests::FWorld Context;Context.MakeLive();
    auto Take=ActingEventTests::MakeTake();Take.Events.Reset();
    ActingEventTests::AddEvent(Take,0,EActingEventType::Footstep,TEXT("Footstep.1"));
    ActingEventTests::AddEvent(Take,.25,EActingEventType::Sound,TEXT("Kiss"));
    ActingEventTests::AddEvent(Take,.5,EActingEventType::Footstep,TEXT("Footstep.2"));
    ActingEventTests::AddEvent(Take,1,EActingEventType::Footstep,TEXT("Footstep.3"));
    TestTrue(TEXT("New event type preserves old recording validation"),Take.IsValid());
    auto* Replay=Context.World->SpawnActor<AReplayActor>();Context.Clock->StartClock();
    if(!TestTrue(TEXT("Footsteps use the existing replay"),Replay->PlayRecording(Take))){return false;}
    TestEqual(TEXT("Time-zero footstep exactly once"),Replay->Acting->DispatchedEventCount,1);
    Context.Clock->PauseClock();Context.Tick(.2f);TestEqual(TEXT("Paused footsteps do not repeat"),Replay->Acting->DispatchedEventCount,1);
    Context.Clock->ResumeClock();Context.Clock->StartClock(.6);Context.Tick();TestEqual(TEXT("Hitch catches each event once"),Replay->Acting->DispatchedEventCount,3);
    Context.Clock->StartClock(1.0);Context.Tick();Context.Tick();TestEqual(TEXT("Endpoint footstep exactly once"),Replay->Acting->DispatchedEventCount,4);
    TestTrue(TEXT("Replay never writes live footsteps"),Context.Recorder->GetRecordingData().Events.IsEmpty());
    Replay->StopPlayback();TestEqual(TEXT("Reset clears event cursor"),Replay->GetNextEventIndex(),0);
    TestEqual(TEXT("Reset stops every owned sound"),Replay->Acting->GetActiveSoundCount(),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActingMovieEventsTest, "OneManStudio.ActingEvents.ThreeTakeMovieAndRestart",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FActingMovieEventsTest::RunTest(const FString& Parameters)
{
    ActingEventTests::FWorld Context;
    APawn* Live = Context.MakeLive();
    APlayerController* Player = Context.World->SpawnActor<APlayerController>();
    if (!Player->PlayerCameraManager) { Player->SpawnPlayerCameraManager(); }
    Player->Possess(Live);
    AMovieManager* Manager = Context.World->SpawnActor<AMovieManager>();
    Manager->PreparationDurationSeconds = 0.1;
    Manager->RecordingDurationSeconds = 0.35;
    Manager->bShowDebugOverlay = false;
    const EActingGesture RoleGestures[] = {EActingGesture::Dance, EActingGesture::Point, EActingGesture::Scared};
    const EActingSound RoleSounds[] = {EActingSound::EvilLaugh, EActingSound::Scream, EActingSound::Cry};
    bool Triggered[3] = {};
    bool TriggeredLater[3] = {};
    bool SawPriorEvents[3] = {};
    bool SawPremiere = false;
    TestTrue(TEXT("Movie starts with existing components"), Manager->StartMovie());
    TestEqual(TEXT("Manager exposes the live ActingComponent"), Manager->LiveActingComponent.Get(), Context.Acting);
    TestFalse(TEXT("Preparation gates actions"), Context.Acting->CanTriggerActions());
    for (int32 Frame = 0; Frame < 256 && Manager->State != EMovieState::Finished && Manager->State != EMovieState::Error; ++Frame)
    {
        if (Manager->State == EMovieState::Preparation)
        {
            for (AReplayActor* Replay : Manager->ActiveReplays)
            {
                TestEqual(TEXT("Preparation resets each previous actor silently"), Replay->Acting->DispatchedEventCount, 0);
            }
        }
        if (Manager->State == EMovieState::Recording)
        {
            const int32 Role = Manager->CurrentTakeIndex;
            if (!Triggered[Role])
            {
                TestTrue(TEXT("Take accepts role gesture"), Context.Acting->TriggerGesture(RoleGestures[Role]));
                TestTrue(TEXT("Take accepts role sound"), Context.Acting->TriggerSound(RoleSounds[Role]));
                Triggered[Role] = true;
            }
            if (Manager->GetSceneTimeSeconds() > 0.1 && !TriggeredLater[Role])
            {
                Context.Acting->TriggerSound(EActingSound::Kiss);
                TriggeredLater[Role] = true;
            }
            if (Manager->GetSceneTimeSeconds() > 0.2)
            {
                TestEqual(TEXT("Live buffer contains only this role's three events"), Context.Recorder->GetRecordingData().Events.Num(), 3);
                for (int32 Prior = 0; Prior < Role; ++Prior)
                {
                    TestEqual(TEXT("Prior actor performs all events while current player records"), Manager->ActiveReplays[Prior]->Acting->DispatchedEventCount, 3);
                    TestEqual(TEXT("Prior actor retains its own first gesture identifier"), Manager->Performances[Prior].Recording.Events[0].Identifier, ActingIds::Gesture(RoleGestures[Prior]));
                }
                SawPriorEvents[Role] = true;
            }
            Live->AddActorWorldOffset(FVector(2.0, Role + 1.0, 0.0));
        }
        if (Manager->State == EMovieState::MoviePremiere)
        {
            SawPremiere = true;
            TestFalse(TEXT("Hidden live player cannot act during premiere"), Context.Acting->CanTriggerActions());
            if (Manager->GetSceneTimeSeconds() > 0.2)
            {
                for (AReplayActor* Replay : Manager->ActiveReplays)
                {
                    TestEqual(TEXT("Premiere independently reproduces each actor's three events"), Replay->Acting->DispatchedEventCount, 3);
                }
            }
        }
        Context.Tick();
    }
    TestTrue(TEXT("Three independent recordings were exercised"), SawPriorEvents[0] && SawPriorEvents[1] && SawPriorEvents[2]);
    TestTrue(TEXT("Premiere runs and finishes"), SawPremiere && Manager->State == EMovieState::Finished);
    TestEqual(TEXT("Three performances retained"), Manager->Performances.Num(), 3);
    for (int32 Role = 0; Role < Manager->Performances.Num(); ++Role)
    {
        TestEqual(TEXT("Events are stored separately per role"), Manager->Performances[Role].Recording.Events.Num(), 3);
        TestEqual(TEXT("Every role starts at scene zero"), Manager->Performances[Role].Recording.Events[0].TimeSeconds, 0.0);
        TestEqual(TEXT("Premiere dispatched each role once"), Manager->ActiveReplays[Role]->Acting->DispatchedEventCount, 3);
    }
    TestTrue(TEXT("Restart succeeds"), Manager->RestartMovie());
    TestTrue(TEXT("Restart removes performances and old replay actors"), Manager->Performances.IsEmpty() && Manager->ActiveReplays.IsEmpty());
    TestTrue(TEXT("Restart clears event buffer"), Context.Recorder->GetRecordingData().Events.IsEmpty());
    TestEqual(TEXT("Restart clears live presentation state"), Context.Acting->DispatchedEventCount, 0);
    const UInputComponent* Input = Manager->FindComponentByClass<UInputComponent>();
    TestTrue(TEXT("Restart binds 11 acting keys, interaction and restart exactly once"), Input && Input->KeyBindings.Num() == 13);
    Manager->StopMovie();
    TestTrue(TEXT("Stop restores live availability setting"), Context.Acting->AreLiveActionsEnabled());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActingAnimationPresentationTest, "OneManStudio.ActingEvents.AnimationPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FActingAnimationPresentationTest::RunTest(const FString& Parameters)
{
    // Existing template assets are loaded read-only; all actors/components/montages are transient.
    USkeletalMesh* MeshAsset = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    UAnimSequence* Sequence = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd"));
    if (!TestNotNull(TEXT("Read-only template skeletal mesh available"), MeshAsset)
        || !TestNotNull(TEXT("Read-only template animation available"), Sequence)) { return false; }
    ActingEventTests::FWorld Context;
    APawn* Live = Context.MakeLive();
    USkeletalMeshComponent* Mesh = NewObject<USkeletalMeshComponent>(Live);
    Live->AddInstanceComponent(Mesh);
    Mesh->SetupAttachment(Live->GetRootComponent());
    Mesh->RegisterComponent();
    Mesh->SetSkeletalMesh(MeshAsset);
    Mesh->SetRelativeLocation(FVector(0.0, 0.0, -90.0));
    Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    Mesh->PlayAnimation(Sequence, true);
    Context.Acting->TargetMesh = Mesh;
    Context.Acting->Gestures[0].Animation = Sequence;
    Context.Acting->Gestures[0].PlayRate = 1.5f;
    Context.Clock->StartClock();
    Context.Recorder->StartRecording(1.0, true);
    TestTrue(TEXT("Live sequence gesture starts"), Context.Acting->TriggerGesture(EActingGesture::Dance));
    TestTrue(TEXT("Live single-node animation ignores root motion"), Mesh->GetAnimInstance()->RootMotionMode == ERootMotionMode::IgnoreRootMotion);
    Context.Tick(0.1f);
    Context.Acting->StopGesture();
    TestEqual(TEXT("Live single-node asset restored after cancellation"), Mesh->GetSingleNodeInstance()->GetCurrentAsset(), static_cast<UAnimationAsset*>(Sequence));
    TestTrue(TEXT("Live single-node playing state restored"), Mesh->GetSingleNodeInstance()->IsPlaying());
    Context.Recorder->StopRecording();
    FActingRecording Take = ActingEventTests::MakeTake();
    Take.Events.SetNum(1);
    Take.ActingSettings = Context.Acting->CapturePlaybackSettings();
    AReplayActor* Replay = Context.World->SpawnActor<AReplayActor>();
    Context.Clock->StartClock();
    Replay->PlayRecording(Take);
    UAnimSingleNodeInstance* Single = Replay->PerformanceMesh->GetSingleNodeInstance();
    if (!TestNotNull(TEXT("Replay owns its animation instance"), Single)) { return false; }
    TestEqual(TEXT("Replay uses the captured sequence"), Single->GetCurrentAsset(), static_cast<UAnimationAsset*>(Sequence));
    TestTrue(TEXT("Replay ignores root motion"), Single->RootMotionMode == ERootMotionMode::IgnoreRootMotion);
    TestFalse(TEXT("Replay never independently advances its animation clock"), Single->IsPlaying());
    Context.Tick(0.1f);
    TestTrue(TEXT("Animation follows absolute take time and play rate"), FMath::IsNearlyEqual(Single->GetCurrentTime(), 0.15f, 0.002f));
    TestTrue(TEXT("Recorded transform owns position during animation"), Replay->GetActorLocation().Equals(FVector(10.0, 0.0, 0.0), 0.01));
    const int32 LegIndex = Replay->PerformanceMesh->GetBoneIndex(TEXT("thigh_r"));
    if (!TestTrue(TEXT("Template has a leg bone for pose verification"), LegIndex >= 0
        && Replay->PerformanceMesh->GetComponentSpaceTransforms().IsValidIndex(LegIndex))) { return false; }
    const FTransform SequenceLegPose = Replay->PerformanceMesh->GetComponentSpaceTransforms()[LegIndex];
    Context.Clock->PauseClock();
    const float HeldPoseTime = Single->GetCurrentTime();
    Context.Tick(0.1f);
    TestEqual(TEXT("Animation freezes with shared scene clock"), Single->GetCurrentTime(), HeldPoseTime);
    TestTrue(TEXT("Bone pose also freezes with shared clock"), Replay->PerformanceMesh->GetComponentSpaceTransforms()[LegIndex].Equals(SequenceLegPose, 0.001));
    Replay->PrepareRecording(Take);
    TestEqual(TEXT("Preparation clears active animation state"), Replay->Acting->ActiveGestureId, NAME_None);
    TestFalse(TEXT("Skeletal animation retains collision isolation"), Replay->PerformanceMesh->IsCollisionEnabled());
    UAnimMontage* Montage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(Sequence, TEXT("ActingTestSlot"), 0.25f, 0.25f);
    if (!TestNotNull(TEXT("Transient test montage created without changing assets"), Montage)) { return false; }
    Take.ActingSettings.Gestures[0].Montage = Montage;
    Context.Clock->StartClock();
    Replay->PlayRecording(Take);
    Single = Replay->PerformanceMesh->GetSingleNodeInstance();
    TestEqual(TEXT("Replay also accepts a montage asset"), Single->GetCurrentAsset(), static_cast<UAnimationAsset*>(Montage));
    Context.Tick(0.1f);
    TestTrue(TEXT("Montage is evaluated at scene time"), FMath::IsNearlyEqual(Single->GetCurrentTime(), 0.15f, 0.002f));
    const FTransform MontageLegPose = Replay->PerformanceMesh->GetComponentSpaceTransforms()[LegIndex];
    Context.Tick(0.1f);
    TestFalse(TEXT("Montage with a custom slot actually animates bones over time"),
        Replay->PerformanceMesh->GetComponentSpaceTransforms()[LegIndex].Equals(MontageLegPose, 0.001));
    Context.Clock->PauseClock();
    const FTransform HeldMontageLegPose = Replay->PerformanceMesh->GetComponentSpaceTransforms()[LegIndex];
    Context.Tick(0.1f);
    TestTrue(TEXT("Montage pose also freezes while paused during blend-in"),
        Replay->PerformanceMesh->GetComponentSpaceTransforms()[LegIndex].Equals(HeldMontageLegPose, 0.001));
    // Exercise the live montage path with a native AnimInstance; the artist supplies the final graph/slot.
    Context.Acting->StopAllActions();
    Mesh->SetAnimInstanceClass(UAnimInstance::StaticClass());
    UAnimInstance* Instance = Mesh->GetAnimInstance();
    if (!TestNotNull(TEXT("Live AnimInstance created"), Instance)) { return false; }
    const auto OriginalRootMode = Instance->RootMotionMode;
    Context.Acting->Gestures[0].Montage = Montage;
    Context.Clock->StartClock();
    Context.Recorder->StartRecording(1.0, true);
    TestTrue(TEXT("Live montage gesture starts"), Context.Acting->TriggerGesture(EActingGesture::Dance));
    TestTrue(TEXT("Live montage ignores root motion"), Instance->RootMotionMode == ERootMotionMode::IgnoreRootMotion);
    Context.Acting->StopAllActions();
    TestTrue(TEXT("Root motion mode is restored on cleanup"), Instance->RootMotionMode == OriginalRootMode);
    TestFalse(TEXT("Owned montage is stopped before root motion is restored"), Instance->Montage_IsActive(Montage));
    Context.Recorder->ClearRecording();
    Context.Acting->StopAllActions();
    return true;
}

#endif
