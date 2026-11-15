#include "ActingRecorderComponent.h"
#include "ActingComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "ReplayActor.h"
#include "WardrobeComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/MovementComponent.h"

UActingRecorderComponent::UActingRecorderComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UActingRecorderComponent::BeginPlay()
{
    Super::BeginPlay();
    AddTickPrerequisiteActor(GetOwner());
    if (UMovementComponent* Movement = GetOwner()->FindComponentByClass<UMovementComponent>())
    {
        AddTickPrerequisiteComponent(Movement);
    }
}

void UActingRecorderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Do not broadcast or start another take while the world/owner is being destroyed.
    bRecording = false;
    SetComponentTickEnabled(false);
    Super::EndPlay(EndPlayReason);
}

bool UActingRecorderComponent::StartRecording(double MaxDurationSeconds, bool bUseSharedSceneClock)
{
    if (bRecording || !IsValid(GetOwner()) || !GetWorld()
        || !FMath::IsFinite(MaxDurationSeconds) || MaxDurationSeconds < 0.0
        || !FMath::IsFinite(SampleRateHz) || !GetOwner()->GetActorTransform().IsValid() || GetOwner()->IsA<AReplayActor>())
    {
        return false;
    }

    Recording.Samples.Reset();
    Recording.Events.Reset();
    Recording.ActingSettings = FActingPlaybackSettings();
    Recording.Appearance = FRecordedAppearance();
    if (UWardrobeComponent* Wardrobe = GetOwner()->FindComponentByClass<UWardrobeComponent>())
    {
        Recording.Appearance = Wardrobe->CaptureAppearance();
    }
    if (UActingComponent* Acting = GetOwner()->FindComponentByClass<UActingComponent>())
    {
        Recording.ActingSettings = Acting->CapturePlaybackSettings();
    }
    RecordingClock = GetWorld()->GetSubsystem<UActingSceneClockSubsystem>();
    bUsesSceneClock = bUseSharedSceneClock;
    if (bUsesSceneClock && !RecordingClock.IsValid()) { return false; }
    RecordingStartSceneTime = RecordingClock.IsValid() ? RecordingClock->GetSceneTimeSeconds() : 0.0;
    SampleIntervalSeconds = 1.0 / FMath::Clamp(static_cast<double>(SampleRateHz), 1.0, 120.0);
    DurationLimitSeconds = MaxDurationSeconds;
    RecordingStartWorldTime = GetWorld()->GetTimeSeconds();
    PreviousObservedTime = 0.0;
    PreviousObservedTransform = GetOwner()->GetActorTransform();
    NextSampleIndex = 1;

    FActingTransformSample& FirstSample = Recording.Samples.AddDefaulted_GetRef();
    FirstSample.TimeSeconds = 0.0;
    FirstSample.WorldTransform = PreviousObservedTransform;
    bRecording = true;
    SetComponentTickEnabled(true);
    return true;
}

bool UActingRecorderComponent::StopRecording()
{
    if (!bRecording)
    {
        return false;
    }
    UpdateRecording(true);
    return true;
}

void UActingRecorderComponent::ClearRecording()
{
    bRecording = false;
    SetComponentTickEnabled(false);
    Recording.Samples.Reset();
    Recording.Events.Reset();
    Recording.ActingSettings = FActingPlaybackSettings();
    Recording.Appearance = FRecordedAppearance();
}

double UActingRecorderComponent::GetRecordingElapsedSeconds() const
{
    if (!bRecording) { return Recording.GetDurationSeconds(); }
    const double Elapsed = bUsesSceneClock && RecordingClock.IsValid()
        ? RecordingClock->GetSceneTimeSeconds() - RecordingStartSceneTime
        : (GetWorld() ? GetWorld()->GetTimeSeconds() - RecordingStartWorldTime : 0.0);
    return FMath::Max(0.0, Elapsed);
}

bool UActingRecorderComponent::RecordActingEvent(EActingEventType EventType, FName Identifier)
{
    FActingEvent Event;
    Event.Type = EventType;
    Event.Identifier = Identifier;
    FActingEvent Recorded;
    return RecordEvent(Event, Recorded);
}

bool UActingRecorderComponent::RecordEvent(const FActingEvent& Event, FActingEvent& OutRecordedEvent)
{
    if (!bRecording || !IsValid(GetOwner()) || GetOwner()->IsA<AReplayActor>() || !Event.HasValidPayload())
    {
        return false;
    }
    const double Timestamp = GetRecordingElapsedSeconds();
    if (!FMath::IsFinite(Timestamp) || (DurationLimitSeconds > 0.0 && Timestamp > DurationLimitSeconds)
        || (!Recording.Events.IsEmpty() && Timestamp < Recording.Events.Last().TimeSeconds)) { return false; }
    OutRecordedEvent = Event;
    OutRecordedEvent.TimeSeconds = Timestamp;
    Recording.Events.Add(OutRecordedEvent);
    return true;
}

void UActingRecorderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (bRecording)
    {
        UpdateRecording(false);
    }
}

void UActingRecorderComponent::UpdateRecording(bool bStopRequested)
{
    if (!IsValid(GetOwner()) || !GetWorld())
    {
        return;
    }

    const double ObservedTime = FMath::Max(PreviousObservedTime, GetRecordingElapsedSeconds());
    const FTransform ObservedTransform = GetOwner()->GetActorTransform();
    const double EndTime = DurationLimitSeconds > 0.0 ? FMath::Min(ObservedTime, DurationLimitSeconds) : ObservedTime;
    const auto TransformAtTime = [&](double SampleTime)
    {
        const double FrameDuration = ObservedTime - PreviousObservedTime;
        const double Alpha = FrameDuration > UE_DOUBLE_SMALL_NUMBER
            ? (SampleTime - PreviousObservedTime) / FrameDuration : 1.0;
        return FActingRecording::InterpolateTransforms(PreviousObservedTransform, ObservedTransform, Alpha);
    };

    // Reconstruct the fixed sample grid between observed frames. Never duplicate a pose at
    // several timestamps just because one frame spanned multiple 50 ms intervals.
    double NextSampleTime = NextSampleIndex * SampleIntervalSeconds;
    while (NextSampleTime <= EndTime)
    {
        FActingTransformSample& Sample = Recording.Samples.AddDefaulted_GetRef();
        Sample.TimeSeconds = NextSampleTime;
        Sample.WorldTransform = TransformAtTime(NextSampleTime);
        NextSampleTime = ++NextSampleIndex * SampleIntervalSeconds;
    }

    const bool bFinished = bStopRequested || (DurationLimitSeconds > 0.0 && ObservedTime >= DurationLimitSeconds);
    if (bFinished)
    {
        // A manual stop need not land on the 20 Hz grid. Preserve its exact final pose/time.
        if (EndTime > Recording.Samples.Last().TimeSeconds)
        {
            FActingTransformSample& LastSample = Recording.Samples.AddDefaulted_GetRef();
            LastSample.TimeSeconds = EndTime;
            LastSample.WorldTransform = TransformAtTime(EndTime);
        }
        else
        {
            Recording.Samples.Last().WorldTransform = TransformAtTime(EndTime);
        }
    }

    PreviousObservedTime = ObservedTime;
    PreviousObservedTransform = ObservedTransform;
    if (bFinished)
    {
        bRecording = false;
        SetComponentTickEnabled(false);
        // Keep the event data stable even if a listener immediately starts a new take.
        const FActingRecording FinishedRecording = Recording;
        // Last operation: a listener may destroy the owner or immediately begin another take.
        OnRecordingFinished.Broadcast(FinishedRecording);
    }
}
