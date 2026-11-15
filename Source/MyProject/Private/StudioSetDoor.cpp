#include "StudioSetDoor.h"
#include "ActingSceneClockSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"
AStudioSetDoor::AStudioSetDoor()
{
    PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickGroup=TG_PostPhysics;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("DoorRoot")));
    Hinge=CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));Hinge->SetupAttachment(RootComponent);
    Leaf=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VaultLeaf"));Leaf->SetupAttachment(Hinge);Leaf->SetMobility(EComponentMobility::Movable);Leaf->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DoorBlocker=CreateDefaultSubobject<UBoxComponent>(TEXT("ClosedDoorCollision"));DoorBlocker->SetupAttachment(Hinge);DoorBlocker->SetBoxExtent(FVector(18,140,150));DoorBlocker->SetRelativeLocation(FVector(0,140,150));DoorBlocker->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Passage=CreateDefaultSubobject<UBoxComponent>(TEXT("SafePassage"));Passage->SetupAttachment(RootComponent);Passage->SetBoxExtent(FVector(100,190,170));Passage->SetRelativeLocation(FVector(0,140,150));Passage->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DoorBlocker->SetCanEverAffectNavigation(false);Passage->SetCanEverAffectNavigation(false);
}
void AStudioSetDoor::BeginPlay(){Super::BeginPlay();ClosedRotation=Hinge->GetComponentQuat();ResetDoor();}
void AStudioSetDoor::ResetDoor(){bOpen=bMoving=bPlayback=false;Hinge->SetWorldRotation(ClosedRotation);DoorBlocker->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);}
FVector AStudioSetDoor::GetInteractionPoint()const{return Passage->GetComponentLocation()-FVector(0,0,100);}
bool AStudioSetDoor::IsPassageClear()const
{
    TArray<FOverlapResult> Hits;FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(StudioDoorSafety),false,this);
    GetWorld()->OverlapMultiByObjectType(Hits,Passage->GetComponentLocation(),Passage->GetComponentQuat(),Objects,FCollisionShape::MakeBox(Passage->GetScaledBoxExtent()),Query);
    return !Hits.ContainsByPredicate([](const FOverlapResult& H){return H.GetActor() && H.GetActor()->IsA<APawn>();});
}
void AStudioSetDoor::PresentRequest(bool Open,double Time,const FTransform& StartHinge,bool Playback)
{
    bOpen=Open;bPlayback=Playback;RequestTime=Time;FromRotation=StartHinge.GetRotation();bMoving=true;
    Hinge->SetWorldRotation(FromRotation);DoorBlocker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void AStudioSetDoor::Tick(float Delta)
{
    Super::Tick(Delta);if(!bMoving){return;}
    if(!bPlayback && !bOpen && !IsPassageClear()){OnSafetyOpen.Broadcast(this);return;}
    const auto* Clock=GetWorld()->GetSubsystem<UActingSceneClockSubsystem>();if(!Clock){return;}
    const double Alpha=FMath::Clamp((Clock->GetSceneTimeSeconds()-RequestTime)/FMath::Max(.1f,OpenSeconds),0.,1.);
    const FQuat Target=ClosedRotation*FRotator(0,bOpen?OpenAngle:0,0).Quaternion();
    Hinge->SetWorldRotation(FQuat::Slerp(FromRotation,Target,Alpha));
    if(Alpha>=1){bMoving=false;if(!bOpen){DoorBlocker->SetCollisionEnabled(bPlayback?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics);}}
}
