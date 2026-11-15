#include "ObjectiveZone.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AObjectiveZone::AObjectiveZone()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;
    Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneBounds"));
    SetRootComponent(Bounds);
    Bounds->SetBoxExtent(FVector(160,160,150));
    Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Bounds->SetGenerateOverlapEvents(false);
    Bounds->SetCanEverAffectNavigation(false);
    FloorMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FloorMarker"));
    FloorMarker->SetupAttachment(Bounds);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    FloorMarker->SetStaticMesh(Cube.Object);
    FloorMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FloorMarker->SetGenerateOverlapEvents(false);
    FloorMarker->SetCanEverAffectNavigation(false);
    SetActorEnableCollision(false);
}

void AObjectiveZone::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RefreshMarker();
}
void AObjectiveZone::RefreshMarker()
{
    const FVector Extent = Bounds->GetUnscaledBoxExtent();
    FloorMarker->SetRelativeScale3D(FVector(Extent.X/50, Extent.Y/50, .02));
    FloorMarker->SetRelativeLocation(FVector(0,0,-Extent.Z+1));
    FloorMarker->SetVisibility(bShowFloorMarker);
}

bool AObjectiveZone::ContainsPoint(FVector WorldPoint) const
{
    const FVector Local = Bounds->GetComponentTransform().InverseTransformPosition(WorldPoint);
    const FVector Extent = Bounds->GetUnscaledBoxExtent();
    return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}
