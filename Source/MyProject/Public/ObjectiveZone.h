#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ObjectiveZone.generated.h"
class UBoxComponent;
class UStaticMeshComponent;

/** Geometric live-player query, independent of collision/overlap events from replay actors. */
UCLASS(Blueprintable)
class MYPROJECT_API AObjectiveZone : public AActor
{
    GENERATED_BODY()
public:
    AObjectiveZone();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective Zone") FName ZoneTag = TEXT("Bank.Entry");
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Objective Zone") TObjectPtr<UBoxComponent> Bounds;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Objective Zone") TObjectPtr<UStaticMeshComponent> FloorMarker;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective Zone") bool bShowFloorMarker = true;
    UFUNCTION(BlueprintPure, Category="Objective Zone") bool ContainsPoint(FVector WorldPoint) const;
    UFUNCTION(BlueprintCallable, Category="Objective Zone") void RefreshMarker();
protected:
    virtual void OnConstruction(const FTransform& Transform) override;
};
