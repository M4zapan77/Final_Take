#include "StorySessionComponent.h"
#include "StudioSetDoor.h"
#include "StudioBankBounds.h"
#include "MovieManager.h"
#include "ActingSceneClockSubsystem.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/TargetPoint.h"
#include "EngineUtils.h"
bool UStorySessionComponent::FindSetMarker(FName Tag,FTransform& Out)const
{
    for(TActorIterator<ATargetPoint> It(GetWorld());It;++It){if(It->ActorHasTag(Tag)){Out=It->GetActorTransform();return true;}}return false;
}
void UStorySessionComponent::InitializeSet()
{
    SafePoses.Reset();SetDoors.Reset();SafetyBounds.Reset();FTransform Anchor;
    bFinalSet=FindSetMarker(TEXT("StorySceneAnchor"),Anchor);if(bFinalSet){SceneAnchor=Anchor.GetLocation();}
    for(TActorIterator<AStudioBankBounds> It(GetWorld());It;++It){SafetyBounds=*It;break;}
    for(TActorIterator<AStudioSetDoor> It(GetWorld());It;++It){SetDoors.Add(*It);It->ResetDoor();It->OnSafetyOpen.AddUObject(this,&UStorySessionComponent::DoorSafetyOpen);SceneState.DoorOpen.Add(It->DoorId,false);}
}
void UStorySessionComponent::ClearSet()
{
    for(AStudioSetDoor* Door:SetDoors){if(IsValid(Door)){Door->OnSafetyOpen.RemoveAll(this);Door->ResetDoor();}}
    SetDoors.Reset();SafetyBounds.Reset();SafePoses.Reset();bFinalSet=false;
}
bool UStorySessionComponent::RequestDoor(AStudioSetDoor* Door,bool Open)
{
    if(!CanAct() || !IsValid(Door) || SceneState.DoorOpen.FindRef(Door->DoorId)==Open || (!Open && !Door->IsPassageClear())){return false;}
    const FTransform Start=Door->Hinge->GetComponentTransform();SceneState.DoorOpen.Add(Door->DoorId,Open);
    Door->PresentRequest(Open,Movie->GetSceneTimeSeconds(),Start,false);
    AddSceneEvent(EStoryEventType::DoorChanged,Door->DoorId,Door->DoorId,Open?FName(TEXT("Open")):FName(TEXT("Closed")),Start,Door->OpenSeconds);
    return true;
}
void UStorySessionComponent::DoorSafetyOpen(AStudioSetDoor* Door){if(!bScenePlayback){RequestDoor(Door,true);}}
void UStorySessionComponent::UpdateCastSafety()
{
    if(!SafetyBounds.IsValid() || bScenePlayback){return;}
    for(const auto& Member:Cast)
    {
        if(!IsValid(Member.Pawn)){continue;}const FVector Location=Member.Pawn->GetActorLocation();
        if(SafetyBounds->NeedsRecovery(Location))
        {
            const FTransform Pose=SafePoses.Contains(Member.ActorId)?SafePoses[Member.ActorId]:Member.InitialTransform;
            if(auto* C=::Cast<ACharacter>(Member.Pawn)){C->GetCharacterMovement()->StopMovementImmediately();}
            Member.Pawn->SetActorTransform(Pose,false,nullptr,ETeleportType::TeleportPhysics);
            AddSceneEvent(EStoryEventType::ActorRecovered,Member.ActorId,NAME_None,TEXT("Safety"),Pose);
        }
        else if(auto* C=::Cast<ACharacter>(Member.Pawn);!C || C->GetCharacterMovement()->IsMovingOnGround())
        {SafePoses.Add(Member.ActorId,Member.Pawn->GetActorTransform());}
    }
    if(Movie->State==EMovieState::Recording && Movie->GetPhaseElapsedSeconds()>.45 && Movie->Controller.IsValid() && Movie->Controller->GetViewTarget()!=Movie->LivePerformer)
    {Movie->Controller->SetViewTarget(Movie->LivePerformer);}
}
