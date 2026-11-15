#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ActingSceneClockSubsystem.generated.h"

/** One clock per gameplay world/PIE instance. Reads world time; never accumulates per-actor deltas. */
UCLASS(BlueprintType)
class MYPROJECT_API UActingSceneClockSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    /** Call once for the whole scene, then PlayRecording on every replay with the same start time. */
    UFUNCTION(BlueprintCallable, Category = "Acting|Scene Clock")
    bool StartClock(double StartTimeSeconds = 0.0);

    UFUNCTION(BlueprintCallable, Category = "Acting|Scene Clock")
    void PauseClock();

    /** Freeze at the recorder's exact completed boundary, eliminating frame overshoot between segments. */
    UFUNCTION(BlueprintCallable, Category="Acting|Clock")
    bool PauseClockAt(double CompletedSceneTimeSeconds);

    UFUNCTION(BlueprintCallable, Category = "Acting|Scene Clock")
    void ResumeClock();

    /** Stops and resets the shared scene time to zero. */
    UFUNCTION(BlueprintCallable, Category = "Acting|Scene Clock")
    void StopClock();

    UFUNCTION(BlueprintPure, Category = "Acting|Scene Clock")
    double GetSceneTimeSeconds() const;

    UFUNCTION(BlueprintPure, Category = "Acting|Scene Clock")
    bool IsClockRunning() const { return bRunning; }

protected:
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
    double HeldTimeSeconds = 0.0;
    double EpochWorldTimeSeconds = 0.0;
    bool bRunning = false;
};
