#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ActingRecording.h"
#include "ActingRecorderTestActor.generated.h"

class APawn;
class APlayerController;
class AReplayActor;
class UActingRecorderComponent;

/** Runtime-only test harness. Replaces the possessed pawn instance after the first take. */
UCLASS(Blueprintable)
class MYPROJECT_API AActingRecorderTestActor : public AActor
{
    GENERATED_BODY()

public:
    AActingRecorderTestActor();

    UFUNCTION(BlueprintCallable, Category = "Acting|Test")
    bool StartTest();

    UFUNCTION(BlueprintCallable, Category = "Acting|Test")
    bool StopAndReplay();

    UFUNCTION(BlueprintCallable, Category = "Acting|Test")
    bool RepeatReplay();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting|Test", meta = (ClampMin = "0.05"))
    double RecordingDurationSeconds = 10.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting|Test")
    TSubclassOf<AReplayActor> ReplayActorClass;

    /** Optional. When unset, spawn the same pawn class as the first performer. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting|Test")
    TSubclassOf<APawn> SecondPawnClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting|Test")
    bool bShowInstructions = true;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Acting|Test")
    TObjectPtr<AReplayActor> ActiveReplay;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

private:
    void TryBindInput();
    void HandleRecordKey();
    void HandleStopKey();
    void HandleRepeatKey();

    UFUNCTION()
    void OnTakeRecorded(const FActingRecording& InRecording);

    UPROPERTY(Transient)
    TObjectPtr<UActingRecorderComponent> Recorder;

    UPROPERTY(Transient)
    TWeakObjectPtr<APawn> RecordingPawn;

    UPROPERTY(Transient)
    TWeakObjectPtr<APlayerController> TestController;

    UPROPERTY(Transient)
    FActingRecording SavedRecording;

    FTransform StartTransform = FTransform::Identity;
    FRotator StartControlRotation = FRotator::ZeroRotator;
    FString Status = TEXT("Ready. R starts the first take.");
};
