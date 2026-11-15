#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StudioSetDoor.generated.h"
class USceneComponent;class UStaticMeshComponent;class UBoxComponent;
class AStudioSetDoor;
DECLARE_MULTICAST_DELEGATE_OneParam(FStudioDoorSafetyOpen,AStudioSetDoor*);
/** Small scene-clock door. StorySession records requests; replay never generates requests. */
UCLASS(Blueprintable)
class MYPROJECT_API AStudioSetDoor:public AActor
{
    GENERATED_BODY()
public:
    AStudioSetDoor();
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Door") FName DoorId=TEXT("Scene.VaultDoor");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Door") float OpenAngle=100;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Door",meta=(ClampMin="0.1",ClampMax="2")) float OpenSeconds=.65;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Door") TObjectPtr<USceneComponent> Hinge;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Door") TObjectPtr<UStaticMeshComponent> Leaf;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Door") TObjectPtr<UBoxComponent> DoorBlocker;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Door") TObjectPtr<UBoxComponent> Passage;
    UPROPERTY(BlueprintReadOnly,Category="Door") bool bOpen=false;
    UFUNCTION(BlueprintPure,Category="Door") FVector GetInteractionPoint() const;
    UFUNCTION(BlueprintPure,Category="Door") bool IsPassageClear() const;
    void ResetDoor();
    void PresentRequest(bool Open,double Time,const FTransform& StartHinge,bool Playback);
    FStudioDoorSafetyOpen OnSafetyOpen;
protected:
    virtual void BeginPlay()override;
    virtual void Tick(float Delta)override;
private:
    FQuat ClosedRotation=FQuat::Identity,FromRotation=FQuat::Identity;
    double RequestTime=0;bool bPlayback=false,bMoving=false;
};
