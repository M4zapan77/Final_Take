#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StudioRailRig.generated.h"
class UChildActorComponent;
class UStaticMeshComponent;
class ACameraActor;

/** Decorative dolly and optional cinematic angle. Reads scene time; never owns or advances it. */
UCLASS(Blueprintable)
class MYPROJECT_API AStudioRailRig : public AActor
{
    GENERATED_BODY()
public:
    AStudioRailRig();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaSeconds) override;
    UFUNCTION(BlueprintPure, Category = "Rail") ACameraActor* GetRailCamera() const;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail") FVector StartOffset = FVector(-275, 0, 20);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail") FVector EndOffset = FVector(275, 0, 20);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail") FVector LookAtWorld = FVector(100, 0, 110);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail", meta = (ClampMin = ".1"))
    float TravelSeconds = 30;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail", meta = (ClampMin = "12", ClampMax = "100"))
    float FocalLength = 28;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rail") bool bAnimateFromSceneClock = true;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rail") TObjectPtr<USceneComponent> Dolly;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rail")
    TObjectPtr<UStaticMeshComponent> CameraModel;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rail")
    TObjectPtr<UChildActorComponent> CineCamera;

private:
    void UpdatePosition(double Time);
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LeftTrack;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> RightTrack;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Platform;
};
