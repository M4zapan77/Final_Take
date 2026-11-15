#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActingTypes.h"
#include "ActingComponent.generated.h"

class UAnimInstance;
class UAnimMontage;
class UAnimationAsset;
class UAudioComponent;
class USkeletalMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FActingEventTriggered, const FActingEvent&, Event, bool, bFromReplay);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActingGestureEnded, FName, Identifier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FActingPresentationIssue, FName, Identifier, FText, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActingAvailabilityChanged, bool, bEnabled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FActingActionsReset);
DECLARE_MULTICAST_DELEGATE_TwoParams(FActingEventTriggeredNative, const FActingEvent&, bool);

/** Live input and per-actor presentation. Movement remains owned by the recorder/replay. */
UCLASS(ClassGroup = (Acting), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class MYPROJECT_API UActingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UActingComponent();

    UFUNCTION(BlueprintCallable, Category = "Acting|Gestures")
    bool TriggerGesture(EActingGesture Gesture);

    UFUNCTION(BlueprintCallable, Category = "Acting|Gestures")
    bool TriggerGestureById(FName Identifier);

    UFUNCTION(BlueprintCallable, Category = "Acting|Soundboard")
    bool TriggerSound(EActingSound Sound);

    UFUNCTION(BlueprintCallable, Category = "Acting|Soundboard")
    bool TriggerSoundById(FName Identifier);

    /** Native interaction entry point: uses the same recording/event dispatch path as gestures. */
    bool TriggerPropEvent(const FActingEvent& Event);

    /** A deliberate player cancellation is recorded. Phase cleanup uses StopAllActions instead. */
    UFUNCTION(BlueprintCallable, Category = "Acting|Gestures")
    bool StopGesture();

    /** Stops this actor's media, restores its animation settings, and clears gesture/HUD state. */
    UFUNCTION(BlueprintCallable, Category = "Acting")
    void StopAllActions();

    UFUNCTION(BlueprintCallable, Category = "Acting")
    void SetLiveActionsEnabled(bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "Acting")
    bool CanTriggerActions() const;

    UFUNCTION(BlueprintPure, Category = "Acting")
    bool AreLiveActionsEnabled() const { return bLiveActionsEnabled; }

    UFUNCTION(BlueprintPure, Category = "Acting")
    FActingPlaybackSettings CapturePlaybackSettings() const;

    /** Default identifiers are prefilled; assign compatible in-place assets in the component's Details. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting|Gestures")
    TArray<FActingGestureDefinition> Gestures;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting|Soundboard")
    TArray<FActingSoundDefinition> Sounds;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acting|Footsteps") TArray<FActingSoundDefinition> Footsteps;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acting|Footsteps", meta=(ClampMin="30")) float FootstepDistanceCm = 145.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acting|Audio") TObjectPtr<USoundAttenuation> DefaultAttenuation;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Acting|Audio") int32 StartedSoundCount = 0;
    UFUNCTION(BlueprintPure, Category="Acting|Audio") int32 GetActiveSoundCount() const;

    /** Optional override. Otherwise uses Character.Mesh, then the owner's first skeletal mesh. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting|Animation")
    TObjectPtr<USkeletalMeshComponent> TargetMesh;

    /** MovieManager also gates input by phase. Disable this only for standalone rehearsal. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting")
    bool bRequireActiveRecording = true;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Acting|State")
    FName ActiveGestureId;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Acting|State")
    FActingEvent LastTriggeredEvent;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Acting|State")
    int32 DispatchedEventCount = 0;

    /** Fires even with an unassigned asset, so placeholder tests and HUD buttons still work. */
    UPROPERTY(BlueprintAssignable, Category = "Acting|Events")
    FActingEventTriggered OnActingEventTriggered;

    UPROPERTY(BlueprintAssignable, Category = "Acting|Events")
    FActingGestureEnded OnGestureEnded;

    UPROPERTY(BlueprintAssignable, Category = "Acting|Events")
    FActingPresentationIssue OnPresentationIssue;

    UPROPERTY(BlueprintAssignable, Category = "Acting|Events")
    FActingAvailabilityChanged OnLiveActionsEnabledChanged;

    UPROPERTY(BlueprintAssignable, Category = "Acting|Events")
    FActingActionsReset OnActionsReset;

    /** Native equivalent, also useful for gameplay listeners without a Blueprint receiver. */
    FActingEventTriggeredNative OnActingEventTriggeredNative;

    // ReplayActor owns these entry points: playback never goes through live input or a recorder.
    void ConfigureReplay(const FActingPlaybackSettings& Settings, USkeletalMeshComponent* Mesh);
    void DispatchRecordedEvent(const FActingEvent& Event, double CurrentTakeTime, bool bClockRunning);
    void UpdateReplayPresentation(double CurrentTakeTime, bool bClockRunning);
    void ResetPlaybackState();
    /** Scene-owned reactions never go through live input, a recorder, or objective scoring. */
    void PresentSceneGesture(FName Identifier,double SceneTime,bool bClockRunning);
    void UpdateScenePresentation(double SceneTime,bool bClockRunning);
    void PlaySceneSound(USoundBase* Sound,double EventSceneTime,double CurrentSceneTime,bool bClockRunning,float Volume=1);

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    bool TriggerLive(EActingEventType Type, FName Identifier);
    void ExecuteEvent(const FActingEvent& Event, double Now, bool bReplay, bool bClockRunning);
    void StartGesture(const FActingEvent& Event, double Now, bool bReplay);
    void FinishGesture(bool bNotify);
    void UpdatePresentation(double Now, bool bClockRunning);
    void ReportIssue(FName Identifier, const TCHAR* Message);
    USkeletalMeshComponent* ResolveMesh() const;
    double GetLiveTime() const;
    const FActingGestureDefinition* FindGesture(FName Identifier) const;
    const FActingSoundDefinition* FindSound(FName Identifier) const;
    void PlayOwnedSound(const FActingSoundDefinition& Definition, double StartTime, double Now, bool bClockRunning);
    void UpdateFootsteps(double Now, bool bClockRunning);

    UPROPERTY(Transient)
    FActingPlaybackSettings ReplaySettings;

    UPROPERTY(Transient)
    FActingGestureDefinition CurrentGesture;

    UPROPERTY(Transient)
    TWeakObjectPtr<UAnimInstance> ControlledAnimInstance;

    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> OwnedMontage;

    UPROPERTY(Transient)
    TObjectPtr<UAnimationAsset> PreviousSingleAsset;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UAudioComponent>> OwnedAudio;

    TArray<double> AudioEndTimes;
    TArray<float> AudioBaseVolumes;
    FVector LastFootstepLocation = FVector::ZeroVector;
    float FootstepTravel = 0;
    double LastFootstepTime = -1;
    int32 FootstepVariation = 0;
    bool bHasFootstepPosition = false;
    bool bGestureImpactPlayed = false;
    double GestureStartTime = 0.0;
    double GestureEndTime = 0.0;
    float PreviousSingleTime = 0.0f;
    float PreviousSingleRate = 1.0f;
    uint8 PreviousAnimationMode = 0;
    uint8 PreviousRootMotionMode = 0;
    uint32 ActionGeneration = 0;
    bool bPreviousSinglePlaying = false;
    bool bPreviousSingleLooping = false;
    bool bRestoreSingleNode = false;
    bool bRootModeOverridden = false;
    bool bReplayOnly = false;
    bool bLiveActionsEnabled = true;
    bool bNotifyingReset = false;
    bool bExternalScenePresentation=false;
    bool bEndingPlay = false;
};
