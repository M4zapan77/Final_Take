#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ActingRecording.h"
#include "ReplayActor.generated.h"

class UActingSceneClockSubsystem;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UActingComponent;
class UWardrobeComponent;
class UPropInteractionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FActingPlaybackFinished);

/** Visual-only actor: no pawn, movement simulation, collision, overlaps, or navigation effects. */
UCLASS(Blueprintable)
class MYPROJECT_API AReplayActor : public AActor
{
    GENERATED_BODY()

public:
    AReplayActor();

    /** Copies a valid take. SceneStartTimeSeconds is an absolute time on the shared scene clock.
     * Does not start/reset that clock. Before the start time, holds the first pose; after the end,
     * holds the last pose and fires OnPlaybackFinished once. Invalid data leaves existing playback intact.
     */
    UFUNCTION(BlueprintCallable, Category = "Acting|Playback")
    bool PlayRecording(const FActingRecording& InRecording, double SceneStartTimeSeconds = 0.0);

    /** Copies a take and resets to its first pose without executing any events (preparation). */
    UFUNCTION(BlueprintCallable, Category = "Acting|Playback")
    bool PrepareRecording(const FActingRecording& InRecording, double SceneStartTimeSeconds = 0.0);

    UFUNCTION(BlueprintCallable, Category = "Acting|Playback")
    void StopPlayback(bool bReturnToFirstPose = true);

    UFUNCTION(BlueprintPure, Category = "Acting|Playback")
    bool IsPlaying() const { return bPlaying; }

    UFUNCTION(BlueprintPure, Category = "Acting|Playback")
    int32 GetNextEventIndex() const { return NextEventIndex; }
    /** Last shared-clock time actually evaluated by this actor; useful for observers running before its tick. */
    UFUNCTION(BlueprintPure, Category="Acting|Playback")
    double GetEvaluatedTakeTimeSeconds() const { return PreviousTakeTime; }

    UFUNCTION(BlueprintPure, Category="Acting|Playback")
    bool EvaluateScenePose(double SceneTimeSeconds, FTransform& OutTransform) const;

    /** Call after adding any new visual components at runtime. Also enforced at BeginPlay and PlayRecording. */
    UFUNCTION(BlueprintCallable, Category = "Acting|Playback")
    void DisablePhysicalInteraction();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Acting|Playback")
    TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Acting|Playback")
    TObjectPtr<UStaticMeshComponent> FacingMarker;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Acting|Playback")
    TObjectPtr<USkeletalMeshComponent> PerformanceMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Acting|Playback")
    TObjectPtr<UActingComponent> Acting;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Acting|Playback")
    TObjectPtr<UWardrobeComponent> Wardrobe;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Acting|Playback")
    TObjectPtr<UPropInteractionComponent> PropInteraction;

    UPROPERTY(BlueprintAssignable, Category = "Acting|Playback")
    FActingPlaybackFinished OnPlaybackFinished;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void UpdatePlayback();

    UPROPERTY(Transient)
    FActingRecording Recording;

    UPROPERTY(Transient)
    TObjectPtr<UActingSceneClockSubsystem> SceneClock;

    double PlaybackStartTimeSeconds = 0.0;
    bool bPlaying = false;
    bool bPreparingPlayback = false;
    int32 NextEventIndex = 0;
    uint32 PlaybackGeneration = 0;
    double PreviousTakeTime = -1.0;
};
