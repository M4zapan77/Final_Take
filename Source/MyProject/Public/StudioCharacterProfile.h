#pragma once
#include "ActingTypes.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StudioCharacterProfile.generated.h"
class UAnimInstance;
class UAnimSequenceBase;
class UMaterialInterface;
class ACharacter;
class UActingComponent;

/** Art configuration only. Movement, collision, possession and recorded transforms remain on the existing actor. */
UCLASS(BlueprintType)
class MYPROJECT_API UStudioCharacterProfile : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character") TObjectPtr<USkeletalMesh> BodyMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character") TSubclassOf<UAnimInstance> AnimationClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character") TObjectPtr<UMaterialInterface> SkinMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    FTransform MeshTransform = FTransform(FRotator(0, -90, 0), FVector(0, 0, -96), FVector(1.05));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character") bool bFitFeetToCapsule = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character") float ReferenceFloorHeight = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face") FFacialSurfaceSettings Face;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TArray<FActingGestureDefinition> DefaultGestures;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TMap<UAnimSequenceBase*, UAnimSequenceBase*> RetargetedAnimations;
    UFUNCTION(BlueprintCallable, Category = "Character") bool ApplyModel(ACharacter* Character) const;
    void ConfigureActing(UActingComponent* Acting) const;
    void ConfigureCatalog(TArray<FCosmeticDefinition>& Catalog) const;
};
