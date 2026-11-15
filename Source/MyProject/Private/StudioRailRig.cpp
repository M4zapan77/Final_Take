#include "StudioRailRig.h"
#include "ActingSceneClockSubsystem.h"
#include "CineCameraActor.h"
#include "CineCameraComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

AStudioRailRig::AStudioRailRig()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    SetActorEnableCollision(false);
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("TrackRoot")));
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube"));
    LeftTrack = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftRail"));
    RightTrack = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightRail"));
    for (UStaticMeshComponent* Track : {LeftTrack.Get(), RightTrack.Get()})
    {
        Track->SetupAttachment(RootComponent);
        Track->SetStaticMesh(Cube);
        Track->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Track->SetCanEverAffectNavigation(false);
    }
    Dolly = CreateDefaultSubobject<USceneComponent>(TEXT("MovingDolly"));
    Dolly->SetupAttachment(RootComponent);
    Platform = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DollyPlatform"));
    Platform->SetupAttachment(Dolly);
    Platform->SetStaticMesh(Cube);
    Platform->SetRelativeScale3D(FVector(1.1, .9, .16));
    Platform->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CameraModel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisibleMovieCamera"));
    CameraModel->SetupAttachment(Dolly);
    CameraModel->SetRelativeLocation(FVector(0, 0, 82));
    CameraModel->SetRelativeScale3D(FVector(1.6));
    CameraModel->SetRelativeRotation(FRotator(0, -90, 0));
    CameraModel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CameraModel->SetCanEverAffectNavigation(false);
    CineCamera = CreateDefaultSubobject<UChildActorComponent>(TEXT("OptionalRailAngle"));
    CineCamera->SetupAttachment(Dolly);
    CineCamera->SetChildActorClass(ACineCameraActor::StaticClass());
    CineCamera->SetRelativeLocation(FVector(0, 0, 155));
}
void AStudioRailRig::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    const FVector Center = (StartOffset + EndOffset) * .5;
    const FVector Delta = EndOffset - StartOffset;
    for (int32 Index = 0; Index < 2; ++Index)
    {
        UStaticMeshComponent* Rail = Index ? RightTrack.Get() : LeftTrack.Get();
        Rail->SetRelativeLocation(FVector(Center.X, Center.Y + (Index ? 30 : -30), 4));
        Rail->SetRelativeRotation(Delta.Rotation());
        Rail->SetRelativeScale3D(FVector((Delta.Size() + 140) / 100, .055, .07));
    }
    UpdatePosition(0);
}
void AStudioRailRig::UpdatePosition(double Time)
{
    const FVector Position =
        FMath::Lerp(StartOffset, EndOffset, FMath::Clamp(Time / FMath::Max(.1f, TravelSeconds), 0.0, 1.0));
    Dolly->SetRelativeLocation(Position);
    const FVector Direction =
        LookAtWorld - (GetActorTransform().TransformPosition(Position) + FVector(0, 0, 155));
    Dolly->SetWorldRotation(FRotator(0, Direction.Rotation().Yaw, 0));
    CineCamera->SetRelativeRotation(FRotator(Direction.Rotation().Pitch, 0, 0));
    if (auto* Camera = Cast<ACineCameraActor>(CineCamera->GetChildActor()))
    {
        Camera->GetCineCameraComponent()->SetCurrentFocalLength(FocalLength);
    }
}
void AStudioRailRig::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bAnimateFromSceneClock)
    {
        if (const auto* Clock = GetWorld()->GetSubsystem<UActingSceneClockSubsystem>())
        {
            UpdatePosition(Clock->GetSceneTimeSeconds());
        }
    }
}
ACameraActor* AStudioRailRig::GetRailCamera() const
{
    return Cast<ACameraActor>(CineCamera->GetChildActor());
}
