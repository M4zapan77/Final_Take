#include "ReplayActor.h"

#include "ActingSceneClockSubsystem.h"
#include "ActingComponent.h"
#include "WardrobeComponent.h"
#include "PropInteractionComponent.h"
#include "MovieManager.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AReplayActor::AReplayActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    bReplicates = false;
    SetActorEnableCollision(false);

    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("ReplayRoot")));
    PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
    PlaceholderMesh->SetupAttachment(RootComponent);
    PlaceholderMesh->SetRelativeScale3D(FVector(0.7, 0.7, 1.6));
    PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PlaceholderMesh->SetGenerateOverlapEvents(false);
    PlaceholderMesh->SetCanEverAffectNavigation(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded())
    {
        PlaceholderMesh->SetStaticMesh(Sphere.Object);
    }

    FacingMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacingMarker"));
    UStaticMeshComponent* Nose = FacingMarker;
    Nose->SetupAttachment(RootComponent);
    Nose->SetStaticMesh(Sphere.Object);
    Nose->SetRelativeLocation(FVector(40.0, 0.0, 35.0));
    Nose->SetRelativeScale3D(FVector(0.35, 0.18, 0.18));
    Nose->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Nose->SetGenerateOverlapEvents(false);
    Nose->SetCanEverAffectNavigation(false);

    PerformanceMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PerformanceMesh"));
    PerformanceMesh->SetupAttachment(RootComponent);
    PerformanceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PerformanceMesh->SetGenerateOverlapEvents(false);
    PerformanceMesh->SetCanEverAffectNavigation(false);
    PerformanceMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Acting = CreateDefaultSubobject<UActingComponent>(TEXT("Acting"));
    Wardrobe = CreateDefaultSubobject<UWardrobeComponent>(TEXT("Wardrobe"));
    Wardrobe->SetReadOnly(true);
    PropInteraction = CreateDefaultSubobject<UPropInteractionComponent>(TEXT("PropInteraction"));
}

void AReplayActor::BeginPlay()
{
    Super::BeginPlay();
    DisablePhysicalInteraction();
    PerformanceMesh->AddTickPrerequisiteActor(this);
}

void AReplayActor::DisablePhysicalInteraction()
{
    SetActorEnableCollision(false);
    TInlineComponentArray<UPrimitiveComponent*> Primitives;
    GetComponents(Primitives, true);
    for (UPrimitiveComponent* Primitive : Primitives)
    {
        Primitive->SetSimulatePhysics(false);
        Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Primitive->SetCollisionResponseToAllChannels(ECR_Ignore);
        Primitive->SetGenerateOverlapEvents(false);
        Primitive->SetCanEverAffectNavigation(false);
    }
}

bool AReplayActor::PlayRecording(const FActingRecording& InRecording, double SceneStartTimeSeconds)
{
    if (!PrepareRecording(InRecording, SceneStartTimeSeconds)) { return false; }
    bPlaying = true;
    SetActorTickEnabled(true);
    UpdatePlayback();
    return true;
}

bool AReplayActor::PrepareRecording(const FActingRecording& InRecording, double SceneStartTimeSeconds)
{
    UActingSceneClockSubsystem* Clock = GetWorld() ? GetWorld()->GetSubsystem<UActingSceneClockSubsystem>() : nullptr;
    if (bPreparingPlayback || IsActorBeingDestroyed() || !Clock || !InRecording.IsValid()
        || !FMath::IsFinite(SceneStartTimeSeconds) || SceneStartTimeSeconds < 0.0)
    {
        return false;
    }

    TGuardValue<bool> Guard(bPreparingPlayback, true);
    StopPlayback(false);
    if (IsActorBeingDestroyed()) { return false; }
    Recording = InRecording;
    SceneClock = Clock;
    PlaybackStartTimeSeconds = SceneStartTimeSeconds;
    Wardrobe->ClearAppearance();
    if (Recording.Appearance.bHasAppearance) { PerformanceMesh->EmptyOverrideMaterials(); }
    PerformanceMesh->SetSkeletalMesh(Recording.ActingSettings.SkeletalMesh);
    if(Recording.ActingSettings.AnimationClass){PerformanceMesh->SetAnimInstanceClass(Recording.ActingSettings.AnimationClass);}
    else {PerformanceMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);}
    PerformanceMesh->bPauseAnims=true;
    PerformanceMesh->SetRelativeTransform(Recording.ActingSettings.MeshRelativeTransform);
    const bool bHasSkeletalMesh = Recording.ActingSettings.SkeletalMesh != nullptr;
    PerformanceMesh->SetVisibility(bHasSkeletalMesh);
    PlaceholderMesh->SetVisibility(!bHasSkeletalMesh);
    FacingMarker->SetVisibility(!bHasSkeletalMesh);
    Acting->ConfigureReplay(Recording.ActingSettings, PerformanceMesh);
    if (!Wardrobe->ApplySnapshot(Recording.Appearance, PerformanceMesh, PlaceholderMesh)) { return false; }
    PropInteraction->ConfigureReplay(Cast<AMovieManager>(GetOwner()), Recording.Appearance.Selection.PropId);
    DisablePhysicalInteraction();
    SetActorTransform(Recording.Samples[0].WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
    return true;
}

void AReplayActor::StopPlayback(bool bReturnToFirstPose)
{
    bPlaying = false;
    PerformanceMesh->bPauseAnims=true;
    ++PlaybackGeneration;
    NextEventIndex = 0;
    PreviousTakeTime = -1.0;
    SetActorTickEnabled(false);
    if (bReturnToFirstPose && !Recording.Samples.IsEmpty())
    {
        SetActorTransform(Recording.Samples[0].WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
        Wardrobe->ApplySnapshot(Recording.Appearance, PerformanceMesh, PlaceholderMesh);
        PropInteraction->ConfigureReplay(Cast<AMovieManager>(GetOwner()), Recording.Appearance.Selection.PropId);
    }
    else { PropInteraction->ResetState(); }
    // Reset delegates may start a new playback; finish our own prop cleanup before notifying them.
    Acting->ResetPlaybackState();
}

bool AReplayActor::EvaluateScenePose(double SceneTimeSeconds, FTransform& OutTransform) const
{
    return Recording.Evaluate(SceneTimeSeconds - PlaybackStartTimeSeconds, OutTransform);
}

void AReplayActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bPlaying)
    {
        UpdatePlayback();
    }
}

void AReplayActor::UpdatePlayback()
{
    if (!IsValid(SceneClock))
    {
        StopPlayback(false);
        return;
    }

    const double TakeTime = SceneClock->GetSceneTimeSeconds() - PlaybackStartTimeSeconds;
    PerformanceMesh->bPauseAnims=!SceneClock->IsClockRunning();
    if (TakeTime < PreviousTakeTime)
    {
        // Explicit backward seeks begin a new event pass. Normal pauses never rewind the cursor.
        ++PlaybackGeneration;
        NextEventIndex = 0;
        Acting->ResetPlaybackState();
        Wardrobe->ApplySnapshot(Recording.Appearance, PerformanceMesh, PlaceholderMesh);
        PropInteraction->ConfigureReplay(Cast<AMovieManager>(GetOwner()), Recording.Appearance.Selection.PropId);
    }
    PreviousTakeTime = TakeTime;
    const uint32 Generation = PlaybackGeneration;
    FTransform Pose;
    if (Recording.Evaluate(TakeTime, Pose))
    {
        SetActorTransform(Pose, false, nullptr, ETeleportType::TeleportPhysics);
    }
    const double PresentationTime = FMath::Min(TakeTime, Recording.GetDurationSeconds());
    while (Recording.Events.IsValidIndex(NextEventIndex) && Recording.Events[NextEventIndex].TimeSeconds <= PresentationTime)
    {
        // Advance and copy before calling Blueprint delegates, which may restart/destroy this actor.
        const FActingEvent Event = Recording.Events[NextEventIndex++];
        Acting->DispatchRecordedEvent(Event, PresentationTime, SceneClock->IsClockRunning());
        if (Generation != PlaybackGeneration || !bPlaying || IsActorBeingDestroyed()) { return; }
    }
    Acting->UpdateReplayPresentation(FMath::Max(0.0, PresentationTime), SceneClock->IsClockRunning());
    if (Generation != PlaybackGeneration || !bPlaying || IsActorBeingDestroyed()) { return; }
    if (TakeTime >= Recording.GetDurationSeconds())
    {
        bPlaying = false;
        SetActorTickEnabled(false);
        Acting->StopAllActions();
        if (Generation != PlaybackGeneration || IsActorBeingDestroyed()) { return; }
        OnPlaybackFinished.Broadcast();
    }
}

void AReplayActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopPlayback(false);
    Super::EndPlay(EndPlayReason);
}
