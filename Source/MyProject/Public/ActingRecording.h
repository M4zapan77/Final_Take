#pragma once

#include "CoreMinimal.h"
#include "ActingTypes.h"
#include "WardrobeTypes.h"
#include "ActingRecording.generated.h"

/** One world-space pose, measured in seconds from the start of a take. */
USTRUCT(BlueprintType)
struct MYPROJECT_API FActingTransformSample
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting|Recording")
    double TimeSeconds = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting|Recording")
    FTransform WorldTransform = FTransform::Identity;
};

/** Value data: safe to copy, retain after the performer is destroyed, and reuse for many replays. */
USTRUCT(BlueprintType)
struct MYPROJECT_API FActingRecording
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting|Recording")
    TArray<FActingTransformSample> Samples;

    /** Non-decreasing timestamps; equal-time events retain the order in which the player triggered them. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting|Recording")
    TArray<FActingEvent> Events;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting|Recording")
    FActingPlaybackSettings ActingSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting|Recording")
    FRecordedAppearance Appearance;

    /** Sample at zero, strictly increasing pose times, valid transforms, ordered events inside the take. */
    bool IsValid() const;

    double GetDurationSeconds() const;

    /** Requires validated data. Clamps outside the take and uses the shortest quaternion arc. */
    bool Evaluate(double TimeSeconds, FTransform& OutTransform) const;

    static FTransform InterpolateTransforms(const FTransform& From, const FTransform& To, double Alpha);
};
