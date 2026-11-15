#pragma once

#include "CoreMinimal.h"
#include "WardrobeTypes.h"
#include "ActingTypes.generated.h"

class UAnimMontage;
class UAnimSequenceBase;
class USkeletalMesh;
class USoundBase;
class USoundAttenuation;
class UAnimInstance;

UENUM(BlueprintType)
enum class EActingEventType : uint8 { Gesture, Sound, StopGesture, CollectProp, UseProp, Footstep, PresentationStop };

UENUM(BlueprintType)
enum class EActingGesture : uint8 { Dance, Point, Scared, Faint, Arrest };

UENUM(BlueprintType)
enum class EActingSound : uint8 { EvilLaugh, Scream, Cry, PoliceSiren, Kiss, Failure };

/** Ordered event data; identifiers resolve against the settings captured with this take. */
USTRUCT(BlueprintType)
struct MYPROJECT_API FActingEvent
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting")
    double TimeSeconds = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting")
    EActingEventType Type = EActingEventType::Gesture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting")
    FName Identifier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting") FName PropId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting") FName TargetId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting") FVector InteractionLocation = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting") FCosmeticDefinition PropVisual;

    bool HasValidPayload() const;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FActingGestureDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting")
    FName Identifier;

    /** Preferred on the live character; its Anim Blueprint needs the montage's slot. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting")
    TObjectPtr<UAnimMontage> Montage;

    /** Optional in-place sequence, played through SlotName on a live Anim Blueprint. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting")
    TObjectPtr<UAnimSequenceBase> Animation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting")
    FName SlotName = TEXT("DefaultSlot");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting", meta = (ClampMin = "0.01"))
    float PlayRate = 1.0f;

    /** Maximum scene-time duration, also used for placeholders with no animation assigned. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting", meta = (ClampMin = "0.01"))
    float DurationSeconds = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting")
    bool bLoop = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting", meta = (ClampMin = "0.0"))
    float BlendInSeconds = 0.1f;
    /** Optional synchronous effect, e.g. a faint impact. Captured with this gesture definition. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acting|Audio") TObjectPtr<USoundBase> ImpactSound;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acting|Audio", meta=(ClampMin="0")) float ImpactDelaySeconds = 1.3f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acting|Audio", meta=(ClampMin="0",ClampMax="2")) float ImpactVolume = .8f;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FActingSoundDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting")
    FName Identifier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting")
    TObjectPtr<USoundBase> Sound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting", meta = (ClampMin = "0.0"))
    float VolumeMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting", meta = (ClampMin = "0.01"))
    float PitchMultiplier = 1.0f;

    /** Limits looping cues such as a siren. Every owned sound is stopped on reset. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting", meta = (ClampMin = "0.01"))
    float MaxDurationSeconds = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting")
    TObjectPtr<USoundAttenuation> Attenuation;
};

/** Each performance retains independent asset references; later role edits do not alter old takes. */
USTRUCT(BlueprintType)
struct MYPROJECT_API FActingPlaybackSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting")
    TArray<FActingGestureDefinition> Gestures;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting")
    TArray<FActingSoundDefinition> Sounds;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acting") TArray<FActingSoundDefinition> Footsteps;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acting") TObjectPtr<USoundAttenuation> DefaultAttenuation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting")
    TObjectPtr<USkeletalMesh> SkeletalMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acting") TSubclassOf<UAnimInstance> AnimationClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acting")
    FTransform MeshRelativeTransform = FTransform::Identity;
};

namespace ActingIds
{
    MYPROJECT_API FName Gesture(EActingGesture Value);
    MYPROJECT_API FName Sound(EActingSound Value);
}
