#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WardrobeTypes.h"
#include "GrabbablePropComponent.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UPrimitiveComponent;
class UGrabbablePropComponent;
DECLARE_MULTICAST_DELEGATE_FourParams(FScenePropImpact,UGrabbablePropComponent*,AActor*,float,const FVector&);

/** Reversible simple collision/physics on an existing mesh actor, with no asset edits. */
UCLASS(ClassGroup=(Acting),BlueprintType,meta=(BlueprintSpawnableComponent))
class MYPROJECT_API UGrabbablePropComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UGrabbablePropComponent();
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Prop") FName ObjectId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Prop") FName CosmeticId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Prop") bool bCanGrab=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Prop") bool bCanThrow=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Prop",meta=(ClampMin="0.2",ClampMax="25")) float MassKg=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Prop",meta=(ClampMin="200",ClampMax="1200")) float ThrowSpeed=800;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Prop|Collision") FName CollisionProfile=TEXT("PhysicsActor");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Prop|Effects",meta=(ClampMin="100")) float MinimumImpactSpeed=180;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Prop|Effects",meta=(ClampMin="0.2")) float ImpactCooldownSeconds=.7f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Prop|Effects") bool bCanKnockMoney=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Prop|Effects") bool bDistractsGuard=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Prop") FTransform GripTransform=FTransform(FQuat::Identity,FVector(20,0,0));
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Prop") FName RightHandSocket=TEXT("RightHand");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Prop") FName LeftHandSocket=TEXT("LeftHand");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Prop") TObjectPtr<UStaticMeshComponent> TargetMesh;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Prop") TObjectPtr<UBoxComponent> PhysicsBody;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Prop") FCosmeticDefinition Visual;
    FScenePropImpact OnImpact;
    bool InitializeBody();
    void RestoreOriginal();
    bool Hold(AActor* Holder,bool bLeftHand);
    bool Release(AActor* Holder,bool bThrow,FVector Direction,double SceneTime);
    void PresentHeld(AActor* Holder,bool bLeftHand);
    void PresentWorld(const FTransform& Transform);
    void SetScenePaused(bool bPaused);
    void UpdateCollisionIgnore(double SceneTime);
    bool IsMoving() const;
    FTransform GetBodyTransform() const;
    FVector GetBodyVelocity() const;
    void RecoverTo(const FTransform& Transform);
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UFUNCTION() void HandleHit(UPrimitiveComponent* HitComponent,AActor* OtherActor,UPrimitiveComponent* OtherComponent,FVector NormalImpulse,const FHitResult& Hit);
    void AttachVisual(AActor* Holder,bool bLeftHand);
    UPROPERTY(Transient) TObjectPtr<USceneComponent> OriginalRoot;
    TWeakObjectPtr<USceneComponent> OriginalParent;
    TWeakObjectPtr<AActor> PreviousHolder;
    FTransform OriginalRootWorld=FTransform::Identity;
    FTransform OriginalRootRelative=FTransform::Identity;
    FName OriginalSocket;
    FVector HeldVelocity=FVector::ZeroVector,HeldAngularVelocity=FVector::ZeroVector;
    double ReleaseSceneTime=0;
    uint8 OriginalMobility=0;
    uint8 OriginalMeshCollision=0;
    bool bOriginalActorCollision=false,bOriginalSimulation=false,bOriginalHidden=false,bResumeSimulation=false,bPaused=false,bInitialized=false;
};
