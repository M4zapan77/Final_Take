#pragma once

#include "CoreMinimal.h"
#include "WardrobeTypes.generated.h"

class UStaticMesh;
class USkeletalMesh;
class UMaterialInterface;
class UTexture2D;
class USoundBase;

UENUM(BlueprintType)
enum class ECosmeticCategory : uint8 { Head, Body, Face, Prop, Eyes, Mouth, Eyebrows, LeftProp };

/** IDs identify catalog entries, never widget indices. NAME_None means unequipped. */
USTRUCT(BlueprintType)
struct MYPROJECT_API FCosmeticSelection
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe", meta=(DisplayName="Hat ID"))
    FName HeadId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe", meta=(DisplayName="Outfit ID"))
    FName BodyId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe", meta=(DisplayName="Face Accessory ID"))
    FName FaceId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe", meta=(DisplayName="Right Hand Prop ID"))
    FName PropId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Face") FName EyeId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Face") FName MouthId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Face") FName EyebrowId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe") FName LeftHandPropId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe") FLinearColor SkinColor=FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe")
    bool bUseBodyColor = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe")
    FLinearColor BodyColor = FLinearColor::White;

    FName GetId(ECosmeticCategory Category) const;
    void SetId(ECosmeticCategory Category, FName Id);
};

/** Static attachments, same-skeleton modular parts, or material-only body variants. */
USTRUCT(BlueprintType)
struct MYPROJECT_API FCosmeticDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe")
    FName Id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe")
    ECosmeticCategory Category = ECosmeticCategory::Head;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe")
    TObjectPtr<UTexture2D> Icon;

    /** Straight-alpha facial sprite. Independent from the body's UVs and material. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Face") TObjectPtr<UTexture2D> FaceTexture;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Materials") int32 ClothingStyle=0;
    /** Optional red-channel mask on the body's original UV0. White is clothing; black is skin. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Materials") TObjectPtr<UTexture2D> ClothingMask;

    /** Optional acting gesture presentation for a recorded UseProp action (e.g. EatDonut). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Prop")
    FName UseGestureId;
    /** Presentation carried by the existing CollectProp/UseProp event, never a second scoring event. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Prop|Audio") TObjectPtr<USoundBase> PickupSound;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Prop|Audio") TObjectPtr<USoundBase> UseSound;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Prop|Audio", meta=(ClampMin="0",ClampMax="2")) float SoundVolume = 1.f;

    /** Optional stable tags copied into each performance for costume-combination awards. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Awards")
    TArray<FName> CosmeticTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe|Mesh")
    TObjectPtr<UStaticMesh> StaticMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe|Mesh")
    TObjectPtr<USkeletalMesh> SkeletalMesh;

    /** A modular part follows the existing mesh's pose; it never replaces the player's rig. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe|Mesh")
    bool bUseLeaderPose = true;

    /** Socket or bone on the existing skeletal mesh. Empty/missing uses ActorRelativeTransform. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe|Attachment")
    FName SocketName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe|Attachment")
    FTransform RelativeTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe|Attachment")
    FTransform ActorRelativeTransform = FTransform::Identity;

    /** Fit a primitive in actor space at equip time, then retain the resolved socket-local offset. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe|Attachment")
    bool bFitInActorSpace = false;
    /** Optional independently authored transform on the left hand; uses the same prop asset and ID. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Wardrobe|Attachment") bool bUseLeftHandTransform=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Wardrobe|Attachment") FTransform LeftHandRelativeTransform=FTransform::Identity;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Attachment") bool bSocketScaleResolved=false;

    /** On a material-only Body entry these replace the original body's slots. Otherwise the part's slots. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe|Materials")
    TArray<TObjectPtr<UMaterialInterface>> Materials;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe|Materials")
    bool bUseTint = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe|Materials")
    FName TintParameter = TEXT("Color");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe|Materials")
    FLinearColor Tint = FLinearColor::White;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FFacialLayerSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Face") FVector2D Position=FVector2D(.5,.42);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Face") FVector2D Scale=FVector2D(.9,.7);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Face") bool bVisible=true;
    bool IsValid() const {return !Position.ContainsNaN()&&!Scale.ContainsNaN()&&Scale.X>0&&Scale.Y>0;}
};

/** Copied with every take: later catalog or layer-layout changes cannot alter an old face. */
USTRUCT(BlueprintType)
struct MYPROJECT_API FFacialSurfaceSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Face") TObjectPtr<UStaticMesh> SurfaceMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Face") TObjectPtr<UMaterialInterface> Material;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Face") TObjectPtr<UTexture2D> NoneTexture;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Face") FName HeadBone=TEXT("Head");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Face") FTransform RelativeTransform=FTransform::Identity;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Face") FFacialLayerSettings Eyes;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Face") FFacialLayerSettings Mouth;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Face") FFacialLayerSettings Eyebrows;
    FFacialSurfaceSettings(){Mouth.Position=FVector2D(.5,.72);Mouth.Scale=FVector2D(.62,.65);Eyebrows.Position=FVector2D(.5,.22);Eyebrows.Scale=FVector2D(.85,.8);}
    bool IsValid() const {return RelativeTransform.IsValid()&&Eyes.IsValid()&&Mouth.IsValid()&&Eyebrows.IsValid();}
};

/** Immutable value data per take, including the resolved asset definitions used at confirmation. */
USTRUCT(BlueprintType)
struct MYPROJECT_API FRecordedAppearance
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe")
    bool bHasAppearance = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe")
    FCosmeticSelection Selection;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe")
    TArray<FCosmeticDefinition> Items;

    /** Materials before the wardrobe touched the actor. Wardrobe-created MIDs are never shared. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe")
    TArray<TObjectPtr<UMaterialInterface>> BaseBodyMaterials;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Face") FFacialSurfaceSettings FaceSettings;

    bool IsValid() const;
};

namespace WardrobeDefaults
{
    MYPROJECT_API TArray<FCosmeticDefinition> MakeCatalog();
    MYPROJECT_API FCosmeticSelection Casual();
    MYPROJECT_API FText LocalizedName(FName Id,const FText& Fallback);
}
