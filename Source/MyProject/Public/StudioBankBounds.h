#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StudioBankBounds.generated.h"
class UBoxComponent;
/** Invisible fixed perimeter/floor, independent of camera collision. */
UCLASS(Blueprintable)
class MYPROJECT_API AStudioBankBounds:public AActor
{
    GENERATED_BODY()
public:
    AStudioBankBounds();
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Safety") FVector2D HalfSize=FVector2D(1000,1150);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Safety") float RecoveryDepth=200;
    UFUNCTION(BlueprintPure,Category="Safety") bool NeedsRecovery(FVector Location)const;
    virtual void OnConstruction(const FTransform& Transform)override;
private:
    UPROPERTY() TArray<TObjectPtr<UBoxComponent>> Walls;
    UPROPERTY() TObjectPtr<UBoxComponent> Floor;
};
