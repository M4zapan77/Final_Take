#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActingRecording.h"
#include "ActingRecorderComponent.generated.h"

class UActingSceneClockSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActingRecordingFinished, const FActingRecording&, Recording);

/** Records world transforms after movement/physics plus explicit timestamped acting events. */
UCLASS(ClassGroup = (Acting), BlueprintType, meta = (BlueprintSpawnableComponent))
class MYPROJECT_API UActingRecorderComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UActingRecorderComponent();

    /** Starts a fresh take. Zero duration means manual stop. Refuses to overwrite an active take. */
    UFUNCTION(BlueprintCallable, Category = "Acting|Recording")
    bool StartRecording(double MaxDurationSeconds = 10.0, bool bUseSharedSceneClock = false);

    /** Captures the exact end time and broadcasts OnRecordingFinished. False when already stopped. */
    UFUNCTION(BlueprintCallable, Category = "Acting|Recording")
    bool StopRecording();

    /** Cancels any active take and discards its samples without firing OnRecordingFinished. */
    UFUNCTION(BlueprintCallable, Category = "Acting|Recording")
    void ClearRecording();

    /** Captures the current recording timeline time. Replay owners can never append live events. */
    UFUNCTION(BlueprintCallable, Category = "Acting|Recording")
    bool RecordActingEvent(EActingEventType EventType, FName Identifier);

    /** Stamps an extended event on the same recording clock, then copies it to OutRecordedEvent. */
    bool RecordEvent(const FActingEvent& Event, FActingEvent& OutRecordedEvent);

    UFUNCTION(BlueprintPure, Category = "Acting|Recording")
    double GetRecordingElapsedSeconds() const;

    UFUNCTION(BlueprintPure, Category = "Acting|Recording")
    bool IsRecording() const { return bRecording; }

    UFUNCTION(BlueprintPure, Category = "Acting|Recording")
    FActingRecording GetRecording() const { return Recording; }

    // Read-only native access avoids copying the movement samples when resolving an acting event.
    const FActingRecording& GetRecordingData() const { return Recording; }
    bool UsesSharedSceneClock() const { return bUsesSceneClock; }

    UFUNCTION(BlueprintPure, Category = "Acting|Recording")
    double GetRecordedDurationSeconds() const { return Recording.GetDurationSeconds(); }

    /** Snapshotted at StartRecording so changing this during a take cannot alter its sample grid. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting|Recording", meta = (ClampMin = "1.0", ClampMax = "120.0"))
    float SampleRateHz = 20.0f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Acting|Recording") bool bCompactStationarySamples=false;

    UPROPERTY(BlueprintAssignable, Category = "Acting|Recording")
    FActingRecordingFinished OnRecordingFinished;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    void UpdateRecording(bool bStopRequested);

    UPROPERTY(Transient)
    FActingRecording Recording;

    UPROPERTY(Transient)
    TWeakObjectPtr<UActingSceneClockSubsystem> RecordingClock;

    bool bRecording = false;
    double RecordingStartWorldTime = 0.0;
    double RecordingStartSceneTime = 0.0;
    bool bUsesSceneClock = false;
    double DurationLimitSeconds = 0.0;
    double SampleIntervalSeconds = 0.05;
    int32 NextSampleIndex = 1;
    double PreviousObservedTime = 0.0;
    FTransform PreviousObservedTransform = FTransform::Identity;
    double PendingHoldTime=0;
    bool bUseCompaction=false;
};
