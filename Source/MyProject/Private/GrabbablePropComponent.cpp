#include "GrabbablePropComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"

UGrabbablePropComponent::UGrabbablePropComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UGrabbablePropComponent::InitializeBody()
{
    if(bInitialized){return true;}
    if(!TargetMesh){TargetMesh=Cast<UStaticMeshComponent>(GetOwner()->GetRootComponent());}
    if(!TargetMesh || !TargetMesh->GetStaticMesh() || !GetOwner()->GetRootComponent()){return false;}
    OriginalRoot=GetOwner()->GetRootComponent();OriginalRootWorld=OriginalRoot->GetComponentTransform();OriginalRootRelative=OriginalRoot->GetRelativeTransform();
    OriginalParent=OriginalRoot->GetAttachParent();OriginalSocket=OriginalRoot->GetAttachSocketName();
    OriginalMobility=static_cast<uint8>(TargetMesh->Mobility);OriginalMeshCollision=static_cast<uint8>(TargetMesh->GetCollisionEnabled());
    bOriginalActorCollision=GetOwner()->GetActorEnableCollision();bOriginalSimulation=TargetMesh->IsSimulatingPhysics();bOriginalHidden=GetOwner()->IsHidden();
    const FTransform MeshWorld=TargetMesh->GetComponentTransform();
    const auto Bounds=TargetMesh->GetStaticMesh()->GetBounds();
    const FVector Extent=(Bounds.BoxExtent*MeshWorld.GetScale3D().GetAbs()).ComponentMax(FVector(4));
    TargetMesh->SetSimulatePhysics(false);TargetMesh->SetMobility(EComponentMobility::Movable);TargetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PhysicsBody=NewObject<UBoxComponent>(GetOwner());GetOwner()->AddInstanceComponent(PhysicsBody);GetOwner()->SetRootComponent(PhysicsBody);
    PhysicsBody->SetMobility(EComponentMobility::Movable);PhysicsBody->SetBoxExtent(Extent);
    PhysicsBody->SetCollisionProfileName(CollisionProfile);PhysicsBody->SetCollisionResponseToChannel(ECC_Camera,ECR_Block);
    PhysicsBody->SetCanEverAffectNavigation(false);PhysicsBody->CanCharacterStepUpOn=ECB_No;
    PhysicsBody->SetNotifyRigidBodyCollision(true);PhysicsBody->SetUseCCD(true);PhysicsBody->SetLinearDamping(.4f);PhysicsBody->SetAngularDamping(4);
    PhysicsBody->RegisterComponent();PhysicsBody->SetWorldTransform(FTransform(MeshWorld.GetRotation(),MeshWorld.TransformPosition(Bounds.Origin)));
    OriginalRoot->AttachToComponent(PhysicsBody,FAttachmentTransformRules::KeepWorldTransform);
    PhysicsBody->SetMassOverrideInKg(NAME_None,FMath::Clamp(MassKg,.2f,25.f),true);
    PhysicsBody->SetPhysicsMaxAngularVelocityInDegrees(480);
    PhysicsBody->OnComponentHit.AddDynamic(this,&UGrabbablePropComponent::HandleHit);
    GetOwner()->SetActorEnableCollision(true);bInitialized=true;return true;
}
FTransform UGrabbablePropComponent::GetBodyTransform() const{return PhysicsBody?PhysicsBody->GetComponentTransform():GetOwner()->GetActorTransform();}
FVector UGrabbablePropComponent::GetBodyVelocity() const{return PhysicsBody?PhysicsBody->GetPhysicsLinearVelocity():FVector::ZeroVector;}
bool UGrabbablePropComponent::IsMoving() const{return PhysicsBody && PhysicsBody->IsSimulatingPhysics() && (GetBodyVelocity().SizeSquared()>4 || PhysicsBody->GetPhysicsAngularVelocityInRadians().SizeSquared()>.01);}
void UGrabbablePropComponent::AttachVisual(AActor* Holder,bool bLeftHand)
{
    if(!Holder || !PhysicsBody){return;}
    GetOwner()->SetActorHiddenInGame(false);
    PhysicsBody->SetSimulatePhysics(false);GetOwner()->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);PhysicsBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto* Mesh=Holder->FindComponentByClass<USkeletalMeshComponent>();FName Bone=bLeftHand?LeftHandSocket:RightHandSocket;
    if(!bLeftHand && !Visual.SocketName.IsNone() && Mesh && Mesh->DoesSocketExist(Visual.SocketName)){Bone=Visual.SocketName;}
    if(Mesh && Mesh->DoesSocketExist(Bone))
    {
        const FTransform Hand=Mesh->GetSocketTransform(Bone);FTransform DesiredMesh;
        if(!Visual.Id.IsNone())
        {
            const FTransform Local=bLeftHand && Visual.bUseLeftHandTransform?Visual.LeftHandRelativeTransform:Visual.RelativeTransform;
            if(Visual.bFitInActorSpace){DesiredMesh=Visual.ActorRelativeTransform*Holder->GetActorTransform();}
            else if(!Visual.bSocketScaleResolved && !Hand.GetScale3D().Equals(Holder->GetActorScale3D(),.01))
            {DesiredMesh=FTransform(Hand.GetRotation()*Local.GetRotation(),Hand.GetLocation()+Hand.TransformVectorNoScale(Local.GetLocation()),Local.GetScale3D()*Holder->GetActorScale3D());}
            else{DesiredMesh=Local*Hand;}
        }
        else
        {
            DesiredMesh=FTransform(Hand.GetRotation()*GripTransform.GetRotation(),Hand.GetLocation()+Hand.TransformVectorNoScale(GripTransform.GetLocation()),OriginalRootWorld.GetScale3D()*GripTransform.GetScale3D());
        }
        const FTransform DesiredBody=TargetMesh->GetRelativeTransform().Inverse()*DesiredMesh;
        GetOwner()->AttachToComponent(Mesh,FAttachmentTransformRules::KeepWorldTransform,Bone);GetOwner()->SetActorTransform(DesiredBody,false,nullptr,ETeleportType::TeleportPhysics);
    }
    else
    {
        GetOwner()->AttachToActor(Holder,FAttachmentTransformRules::KeepWorldTransform);GetOwner()->SetActorRelativeLocation(FVector(65,bLeftHand?-40:40,0));
    }
}
bool UGrabbablePropComponent::Hold(AActor* Holder,bool bLeftHand)
{
    if(!bCanGrab || !Holder || !InitializeBody()){return false;}
    AttachVisual(Holder,bLeftHand);PreviousHolder=Holder;bResumeSimulation=false;return true;
}
void UGrabbablePropComponent::PresentHeld(AActor* Holder,bool bLeftHand){if(InitializeBody()){AttachVisual(Holder,bLeftHand);bResumeSimulation=false;}}
void UGrabbablePropComponent::PresentWorld(const FTransform& Transform)
{
    if(!InitializeBody()){return;}PhysicsBody->SetSimulatePhysics(false);GetOwner()->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    PhysicsBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);GetOwner()->SetActorTransform(Transform,false,nullptr,ETeleportType::TeleportPhysics);bResumeSimulation=false;
}
bool UGrabbablePropComponent::Release(AActor* Holder,bool bThrow,FVector Direction,double SceneTime)
{
    if(!bInitialized || !Holder || (bThrow && !bCanThrow)){return false;}
    GetOwner()->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);Direction=Direction.GetSafeNormal();if(Direction.IsNearlyZero()){Direction=Holder->GetActorForwardVector();}
    const float Radius=Cast<ACharacter>(Holder)?Cast<ACharacter>(Holder)->GetCapsuleComponent()->GetScaledCapsuleRadius():45;
    const float Clearance=Radius+PhysicsBody->Bounds.BoxExtent.Size2D()+15;
    const FVector Flat=FVector(Direction.X,Direction.Y,0).GetSafeNormal();
    GetOwner()->SetActorLocation(Holder->GetActorLocation()+Flat*Clearance+FVector(0,0,20),false,nullptr,ETeleportType::TeleportPhysics);
    PhysicsBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);GetOwner()->SetActorEnableCollision(true);PhysicsBody->SetSimulatePhysics(true);
    PhysicsBody->SetPhysicsLinearVelocity(FVector::ZeroVector);PhysicsBody->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
    if(bThrow){PhysicsBody->AddImpulse((Direction+FVector(0,0,.18)).GetSafeNormal()*FMath::Clamp(ThrowSpeed,200.f,1200.f),NAME_None,true);}
    PreviousHolder=Holder;ReleaseSceneTime=SceneTime;bPaused=false;bResumeSimulation=true;
    PhysicsBody->IgnoreActorWhenMoving(Holder,true);if(auto* C=Cast<ACharacter>(Holder)){C->GetCapsuleComponent()->IgnoreActorWhenMoving(GetOwner(),true);}
    return true;
}
void UGrabbablePropComponent::UpdateCollisionIgnore(double SceneTime)
{
    if(!PreviousHolder.IsValid() || !PhysicsBody){return;}
    if(SceneTime-ReleaseSceneTime>.35 || FVector::DistSquared(GetOwner()->GetActorLocation(),PreviousHolder->GetActorLocation())>FMath::Square(180.0))
    {
        PhysicsBody->IgnoreActorWhenMoving(PreviousHolder.Get(),false);if(auto* C=Cast<ACharacter>(PreviousHolder.Get())){C->GetCapsuleComponent()->IgnoreActorWhenMoving(GetOwner(),false);}PreviousHolder.Reset();
    }
}
void UGrabbablePropComponent::SetScenePaused(bool Paused)
{
    if(!PhysicsBody || bPaused==Paused){return;}bPaused=Paused;
    if(Paused)
    {
        bResumeSimulation=PhysicsBody->IsSimulatingPhysics();HeldVelocity=GetBodyVelocity();HeldAngularVelocity=PhysicsBody->GetPhysicsAngularVelocityInRadians();PhysicsBody->SetSimulatePhysics(false);
    }
    else if(bResumeSimulation)
    {PhysicsBody->SetSimulatePhysics(true);PhysicsBody->SetPhysicsLinearVelocity(HeldVelocity);PhysicsBody->SetPhysicsAngularVelocityInRadians(HeldAngularVelocity);}
}
void UGrabbablePropComponent::RecoverTo(const FTransform& Transform)
{
    if(!PhysicsBody){return;}PhysicsBody->SetSimulatePhysics(false);GetOwner()->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    PhysicsBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);GetOwner()->SetActorTransform(Transform,false,nullptr,ETeleportType::TeleportPhysics);bResumeSimulation=false;
}
void UGrabbablePropComponent::HandleHit(UPrimitiveComponent* HitComponent,AActor* Other,UPrimitiveComponent* OtherComponent,FVector Impulse,const FHitResult& Hit)
{
    if(!Other || Other==PreviousHolder.Get() || !PhysicsBody || !PhysicsBody->IsSimulatingPhysics()){return;}
    const float Speed=FMath::Max(GetBodyVelocity().Size(),Impulse.Size()/FMath::Max(.2f,MassKg));
    if(Speed>=FMath::Max(100.f,MinimumImpactSpeed)){OnImpact.Broadcast(this,Other,Speed,Hit.ImpactPoint);}
}
void UGrabbablePropComponent::RestoreOriginal()
{
    if(!bInitialized){return;}
    if(PreviousHolder.IsValid()){if(auto* C=Cast<ACharacter>(PreviousHolder.Get())){C->GetCapsuleComponent()->IgnoreActorWhenMoving(GetOwner(),false);}}PreviousHolder.Reset();
    PhysicsBody->OnComponentHit.RemoveDynamic(this,&UGrabbablePropComponent::HandleHit);PhysicsBody->SetSimulatePhysics(false);GetOwner()->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    OriginalRoot->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);GetOwner()->SetRootComponent(OriginalRoot);OriginalRoot->SetWorldTransform(OriginalRootWorld);
    if(OriginalParent.IsValid()){OriginalRoot->AttachToComponent(OriginalParent.Get(),FAttachmentTransformRules::KeepWorldTransform,OriginalSocket);OriginalRoot->SetRelativeTransform(OriginalRootRelative);}
    TargetMesh->SetCollisionEnabled(static_cast<ECollisionEnabled::Type>(OriginalMeshCollision));TargetMesh->SetMobility(static_cast<EComponentMobility::Type>(OriginalMobility));TargetMesh->SetSimulatePhysics(bOriginalSimulation);
    GetOwner()->SetActorEnableCollision(bOriginalActorCollision);GetOwner()->SetActorHiddenInGame(bOriginalHidden);GetOwner()->RemoveInstanceComponent(PhysicsBody);PhysicsBody->DestroyComponent();PhysicsBody=nullptr;bInitialized=false;bPaused=false;OnImpact.Clear();
}
void UGrabbablePropComponent::EndPlay(const EEndPlayReason::Type Reason){RestoreOriginal();Super::EndPlay(Reason);}
