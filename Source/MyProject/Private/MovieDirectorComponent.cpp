#include "MovieDirectorComponent.h"
#include "MovieManager.h"
#include "StorySessionComponent.h"
#include "ReplayActor.h"
#include "StudioRailRig.h"
#include "CineCameraActor.h"
#include "CineCameraComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

UMovieDirectorComponent::UMovieDirectorComponent(){PrimaryComponentTick.bCanEverTick=false;}
ACameraActor* UMovieDirectorComponent::GetCamera() const{return Camera;}
int32 UMovieDirectorComponent::EventPriority(EStoryEventType Type)
{
    switch(Type)
    {
    case EStoryEventType::Escape:case EStoryEventType::MoneySecured:return 110;
    case EStoryEventType::MoneyConfiscated:case EStoryEventType::Detain:return 100;
    case EStoryEventType::PropImpact:return 95;
    case EStoryEventType::PropThrown:return 90;
    case EStoryEventType::MoneyTaken:case EStoryEventType::MoneyTransferred:return 80;
    case EStoryEventType::GuardDistracted:return 75;
    default:return 0;
    }
}
void UMovieDirectorComponent::Begin(AMovieManager* InMovie,ACameraActor* WideCamera)
{
    Stop();if(!bEnabled || !InMovie || !InMovie->IsChronologicalMovie() || !WideCamera){return;}Movie=InMovie;Wide=WideCamera;
    FActorSpawnParameters Spawn;Spawn.Owner=Movie;Spawn.ObjectFlags|=RF_Transient;Camera=GetWorld()->SpawnActor<ACineCameraActor>(ACineCameraActor::StaticClass(),WideCamera->GetActorTransform(),Spawn);
    if(!Camera){Stop();return;}Camera->SetActorEnableCollision(false);Camera->GetCineCameraComponent()->SetConstraintAspectRatio(false);
    Camera->GetCineCameraComponent()->FocusSettings.FocusMethod=ECameraFocusMethod::Disable;
    Camera->GetCineCameraComponent()->PostProcessSettings=WideCamera->GetCameraComponent()->PostProcessSettings;
    if(bUseExistingRail){for(TActorIterator<AStudioRailRig> It(GetWorld());It;++It){Rail=*It;break;}}
    CurrentShot=NAME_None;ShotCount=0;LastCutTime=0;EventCursor=PendingPriority=0;bFinalShotChosen=false;SelectShot(TEXT("Wide"),NAME_None);
    Update(0);
}
void UMovieDirectorComponent::Stop()
{
    if(IsValid(Camera)){Camera->Destroy();}Camera=nullptr;Movie=nullptr;Wide.Reset();Rail.Reset();CurrentShot=FocusRoleId=TargetId=NAME_None;ShotCount=0;EventCursor=PendingPriority=0;
}
void UMovieDirectorComponent::SelectShot(FName Id,FName Role,FName Target)
{
    CurrentShot=Id;FocusRoleId=Role;TargetId=Target;LastCutTime=Movie?Movie->GetSceneTimeSeconds():0;++ShotCount;OnShotChanged.Broadcast(Id,Role);
}
bool UMovieDirectorComponent::IsVisible(FVector Location,FVector Focus) const
{
    FCollisionQueryParams Query(SCENE_QUERY_STAT(MovieDirectorVisibility),true,GetOwner());
    // Physical props and bodies may cross the shot; only structural occlusion changes the angle.
    FHitResult Hit;return !GetWorld()->LineTraceSingleByObjectType(Hit,Location,Focus,FCollisionObjectQueryParams(ECC_WorldStatic),Query);
}
void UMovieDirectorComponent::Update(float DeltaSeconds)
{
    if(!Movie || !IsValid(Camera) || !Wide.IsValid()){return;}const auto* Story=Movie->StorySession.Get();if(!Story){return;}
    const double Time=Movie->GetSceneTimeSeconds();
    while(Story->SceneEvents.IsValidIndex(EventCursor) && Story->SceneEvents[EventCursor].TimeSeconds<=Time)
    {
        const auto& Event=Story->SceneEvents[EventCursor++];const int32* ConfiguredPriority=Story->SessionScenario.CinematicPriorities.Find(Event.Type);const int32 Priority=ConfiguredPriority?*ConfiguredPriority:EventPriority(Event.Type);
        if(Priority>0 && Priority>=PendingPriority){Pending=Event;PendingPriority=Priority;}
    }
    const bool bCanCut=Time-LastCutTime>=FMath::Max(MinimumShotSeconds,CutCooldownSeconds);
    if(bCanCut && !bFinalShotChosen && Time>=FMath::Max(2.5,Story->GetTimelineDuration()-2.5))
    {bFinalShotChosen=true;SelectShot(Story->FinalSceneState.MoneyOwner().IsNone()?FName(TEXT("Wide")):FName(TEXT("Outcome")),Story->FinalSceneState.MoneyOwner());}
    else if(bCanCut && !bFinalShotChosen && PendingPriority>0 && Time-Pending.TimeSeconds<=3)
    {SelectShot(PendingPriority>=110?FName(TEXT("Outcome")):FName(TEXT("Interaction")),Pending.InstigatorId,Pending.TargetId);PendingPriority=0;}
    else if(bCanCut && !bFinalShotChosen && Time-LastCutTime>=TrackingShotSeconds)
    {
        FName Role;for(const auto& Segment:Story->Segments){if(Time>=Segment.StartSeconds && Time<Segment.EndSeconds){Role=Segment.ActorId;break;}}
        SelectShot(bUseExistingRail && Rail.IsValid() && ShotCount%3==2?FName(TEXT("Rail")):FName(TEXT("Tracking")),Role);PendingPriority=0;
    }
    if(!Movie || !IsValid(Camera) || !Wide.IsValid()){return;}
    auto* WideComponent=Wide->GetCameraComponent();FVector Location=WideComponent->GetComponentLocation();FRotator Rotation=WideComponent->GetComponentRotation();float FOV=WideComponent->FieldOfView;
    bool bTracking=false;FVector Focus;
    if(CurrentShot==TEXT("Rail") && Rail.IsValid() && Rail->GetRailCamera())
    {
        auto* Source=Rail->GetRailCamera()->GetCameraComponent();const FVector Center=Story->Cast.IsEmpty()?FVector::ZeroVector:Story->Cast[0].InitialTransform.GetLocation()-Story->SessionScenario.RoleOffsets[0];
        if(IsVisible(Source->GetComponentLocation(),Center))
        {Location=Source->GetComponentLocation();Rotation=(Center-Location).Rotation();FOV=Source->FieldOfView;}
    }
    else if(CurrentShot!=TEXT("Wide"))
    {
        if(auto* Actor=Movie->FindReplayForRole(FocusRoleId))
        {
            Focus=Actor->GetActorLocation()+FVector(0,0,35);
            AActor* Target=Movie->FindReplayForRole(TargetId);
            if(Target){Focus=(Focus+Target->GetActorLocation()+FVector(0,0,35))*.5;}
            else if(!TargetId.IsNone() && TargetId.ToString().StartsWith(TEXT("NPC."))){Focus=(Focus+Story->GetInteractionLocation(TargetId)+FVector(0,0,35))*.5;}
            const FVector Desired=Focus+Actor->GetActorQuat().RotateVector(TrackingOffset);
            if(IsVisible(Desired,Focus))
            {Location=Desired;Rotation=(Focus-Desired).Rotation();FOV=CurrentShot==TEXT("Outcome")?65.f:70.f;bTracking=true;}
        }
    }
    // A blocked candidate always falls back to the original approved wide camera.
    if(DeltaSeconds>0 && bTracking && Time-LastCutTime>DeltaSeconds*1.5)
    {Location=FMath::VInterpTo(Camera->GetActorLocation(),Location,DeltaSeconds,FollowSpeed);Rotation=FMath::RInterpTo(Camera->GetActorRotation(),Rotation,DeltaSeconds,FollowSpeed);}
    Camera->SetActorLocationAndRotation(Location,Rotation,false,nullptr,ETeleportType::TeleportPhysics);
    const float Focal=Camera->GetCineCameraComponent()->Filmback.SensorWidth/(2*FMath::Tan(FMath::DegreesToRadians(FOV*.5f)));Camera->GetCineCameraComponent()->SetCurrentFocalLength(Focal);
}
