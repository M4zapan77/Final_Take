#include "StorySessionComponent.h"
#include "StudioBankBounds.h"
#include "MovieManager.h"
#include "GrabbablePropComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "WardrobeComponent.h"
#include "InteractableProp.h"
#include "ReplayActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"

void FStoryPropTrack::Observe(double Time,const FTransform& Transform,bool bBoundary)
{
    FActingRecording::AppendPose(Samples,Time,Transform,PendingHoldTime,true,bBoundary);
}
bool FStoryPropTrack::Evaluate(double Time,FTransform& Out) const
{
    if(Samples.IsEmpty() || !FMath::IsFinite(Time)){return false;}
    if(Time<=Samples[0].TimeSeconds){Out=Samples[0].WorldTransform;return true;}
    int32 Low=0,High=Samples.Num()-1;
    while(Low<High){const int32 Middle=(Low+High+1)/2;if(Samples[Middle].TimeSeconds<=Time){Low=Middle;}else{High=Middle-1;}}
    const auto& A=Samples[Low];if(Low==Samples.Num()-1){Out=A.WorldTransform;return true;}
    const auto& B=Samples[Low+1];Out=FActingRecording::InterpolateTransforms(A.WorldTransform,B.WorldTransform,(Time-A.TimeSeconds)/(B.TimeSeconds-A.TimeSeconds));return true;
}

UGrabbablePropComponent* UStorySessionComponent::RegisterPhysicalProp(AActor* Actor,FName Id,FName CosmeticId)
{
    if(!IsValid(Actor) || Id.IsNone() || PhysicalProps.Contains(Id)){return nullptr;}
    auto* Mesh=Actor->FindComponentByClass<UStaticMeshComponent>();if(!Mesh || !Mesh->GetStaticMesh()){return nullptr;}
    auto* Prop=NewObject<UGrabbablePropComponent>(Actor);Actor->AddInstanceComponent(Prop);Prop->RegisterComponent();
    Prop->ObjectId=Id;Prop->CosmeticId=CosmeticId;Prop->TargetMesh=Mesh;
    if(const auto* Item=Movie->LiveWardrobeComponent->Catalog.FindByPredicate([CosmeticId](const auto& I){return I.Id==CosmeticId;}))
    {Prop->Visual=UWardrobeComponent::FreezeItem(*Item,this);}
    Prop->MassKg=CosmeticId==TEXT("Prop.Donut")?.25f:CosmeticId.IsNone()?4.f:1.f;
    Prop->bDistractsGuard=CosmeticId==TEXT("Prop.Donut");
    if(!Prop->InitializeBody()){Actor->RemoveInstanceComponent(Prop);Prop->DestroyComponent();return nullptr;}
    Prop->OnImpact.AddUObject(this,&UStorySessionComponent::HandlePropImpact);PhysicalProps.Add(Id,Prop);
    auto* State=SceneState.FindObject(Id);if(!State){State=&SceneState.Objects.AddDefaulted_GetRef();State->ObjectId=Id;State->CosmeticId=CosmeticId;}
    State->WorldTransform=Prop->GetBodyTransform();auto& Track=PropTracks.AddDefaulted_GetRef();Track.ObjectId=Id;Track.Observe(0,State->WorldTransform,true);
    return Prop;
}
bool UStorySessionComponent::InitializePhysicalProps()
{
    PropTracks.Reset();PhysicalProps.Reset();CreatedPropActors.Reset();PlaybackOwners.Reset();PlaybackLeftHands.Reset();LastImpactTimes.Reset();QueuedImpacts.Reset();NextPropSampleTime=0;
    if(!RegisterPhysicalProp(MoneyProp,SceneState.MoneyObjectId,TEXT("Prop.MoneyBag"))){return false;}
    int32 ItemIndex=0;bool bStool=false;
    for(TActorIterator<AInteractableProp> It(GetWorld());It;++It)
    {
        if(*It==MoneyProp || (It->PropId!=TEXT("Prop.Donut") && It->PropId!=TEXT("Prop.Banana"))){continue;}
        if(auto* Prop=RegisterPhysicalProp(*It,It->PickupId,It->PropId))
        {
            if(bFinalSet){continue;}
            // Reversible runtime staging keeps the useful props within a ten-second turn.
            FTransform Transform=Prop->GetBodyTransform();Transform.SetLocation(GroundLocation(SceneAnchor+FVector(120,-80+ItemIndex*100,0),Prop->PhysicsBody->GetScaledBoxExtent().Z));
            Prop->RecoverTo(Transform);SceneState.FindObject(Prop->ObjectId)->WorldTransform=Transform;
            PropTracks.Last().Samples.Reset();PropTracks.Last().Observe(0,Transform,true);++ItemIndex;
        }
    }
    for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)
    {
        auto* Mesh=It->GetStaticMeshComponent();if(!Mesh || !Mesh->GetStaticMesh()){continue;}const FString Name=Mesh->GetStaticMesh()->GetName();
        if(!bStool && Name==TEXT("SM_Banquito_Amarillo") && (!bFinalSet || It->ActorHasTag(TEXT("StoryThrowable"))))
        {bStool=RegisterPhysicalProp(*It,FName(*(TEXT("Scene.")+It->GetName())),NAME_None)!=nullptr;}
        if(Name==TEXT("SM_Escritorio"))
        {
            // A thin tabletop collider: imported visual geometry remains untouched.
            auto* Box=NewObject<UBoxComponent>(GetOwner());GetOwner()->AddInstanceComponent(Box);Box->SetBoxExtent(FVector(Mesh->Bounds.BoxExtent.X,Mesh->Bounds.BoxExtent.Y,6));
            Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Box->SetCollisionObjectType(ECC_WorldStatic);Box->SetCollisionResponseToAllChannels(ECR_Ignore);Box->SetCollisionResponseToChannel(ECC_PhysicsBody,ECR_Block);
            Box->SetCanEverAffectNavigation(false);Box->RegisterComponent();Box->SetWorldLocation(FVector(Mesh->Bounds.Origin.X,Mesh->Bounds.Origin.Y,Mesh->Bounds.Origin.Z+Mesh->Bounds.BoxExtent.Z-6));FurnitureCollision.Add(Box);
        }
    }
    SetPhysicsPaused(true);return true;
}
void UStorySessionComponent::ClearPhysicalProps()
{
    for(auto& Pair:PhysicalProps){if(IsValid(Pair.Value)){Pair.Value->RestoreOriginal();Pair.Value->GetOwner()->RemoveInstanceComponent(Pair.Value);Pair.Value->DestroyComponent();}}
    PhysicalProps.Reset();for(AActor* Actor:CreatedPropActors){if(IsValid(Actor)){Actor->Destroy();}}CreatedPropActors.Reset();
    for(UBoxComponent* Box:FurnitureCollision){if(IsValid(Box)){GetOwner()->RemoveInstanceComponent(Box);Box->DestroyComponent();}}FurnitureCollision.Reset();
    PropTracks.Reset();PlaybackOwners.Reset();PlaybackLeftHands.Reset();LastImpactTimes.Reset();QueuedImpacts.Reset();NextPropSampleTime=0;
}
FName UStorySessionComponent::GetHeldObject(FName Id,bool bLeft) const
{for(const auto& Object:SceneState.Objects){if(Object.OwnerId==Id && Object.bLeftHand==bLeft){return Object.ObjectId;}}return NAME_None;}
void UStorySessionComponent::InitializeOutfitProps()
{
    if(!Cast.IsValidIndex(GetRoleIndex())){return;}auto& Member=Cast[GetRoleIndex()];
    const auto Outfit=Member.Appearance.Selection;const FName Slots[]={Outfit.PropId,Outfit.LeftHandPropId};
    for(int32 Hand=0;Hand<2;++Hand)
    {
        const FName Id=FName(*FString::Printf(TEXT("Outfit.%s.%s"),*Member.ActorId.ToString(),Hand?TEXT("Left"):TEXT("Right")));
        if(Slots[Hand].IsNone() || Slots[Hand]==TEXT("Prop.MoneyBag") || PhysicalProps.Contains(Id)){continue;}
        const auto* Item=Member.Appearance.Items.FindByPredicate([&](const auto& I){return I.Id==Slots[Hand];});
        if(!Item || !Item->StaticMesh || !GetHeldObject(Member.ActorId,Hand!=0).IsNone()){continue;}
        auto* Actor=GetWorld()->SpawnActor<AStaticMeshActor>();if(!Actor){continue;}
        auto* Mesh=Actor->GetStaticMeshComponent();Mesh->SetMobility(EComponentMobility::Movable);Mesh->SetStaticMesh(Item->StaticMesh);
        for(int32 Material=0;Material<Item->Materials.Num();++Material){Mesh->SetMaterial(Material,Item->Materials[Material]);}
        Actor->SetActorScale3D(Item->ActorRelativeTransform.GetScale3D());CreatedPropActors.Add(Actor);
        if(auto* Prop=RegisterPhysicalProp(Actor,Id,Slots[Hand]))
        {
            Prop->Visual=*Item;Prop->Hold(Member.Pawn,Hand!=0);
            auto* State=SceneState.FindObject(Id);State->OwnerId=Member.ActorId;State->bLeftHand=Hand!=0;State->CreatedAtSeconds=Movie->GetSceneTimeSeconds();State->WorldTransform=Prop->GetBodyTransform();
            InitialSceneState.Objects.Add(*State);InitialSceneState.Objects.Last().OwnerId=NAME_None;
            auto& Track=PropTracks.Last();Track.Samples.Reset();Track.Observe(State->CreatedAtSeconds,State->WorldTransform,true);
            AddSceneEvent(EStoryEventType::PropCreated,Member.ActorId,Id,Member.ActorId,State->WorldTransform,Hand);
        }
    }
    UpdateHandSuppression(false);
}
void UStorySessionComponent::UpdateHandSuppression(bool bPlayback)
{
    for(const auto& Member:Cast)
    {
        AActor* Actor=bPlayback?static_cast<AActor*>(Movie->FindReplayForRole(Member.ActorId)):static_cast<AActor*>(Member.Pawn.Get());
        auto* Wardrobe=Actor?Actor->FindComponentByClass<UWardrobeComponent>():nullptr;if(!Wardrobe){continue;}
        const auto Selection=Wardrobe->GetSelection();bool Hidden[]={Selection.PropId==TEXT("Prop.MoneyBag"),Selection.LeftHandPropId==TEXT("Prop.MoneyBag")};
        for(const auto& Object:SceneState.Objects)
        {
            const bool Converted=Object.ObjectId.ToString().StartsWith(TEXT("Outfit.")+Member.ActorId.ToString()+TEXT("."));
            const FName Owner=bPlayback?PlaybackOwners.FindRef(Object.ObjectId):Object.OwnerId;
            if(Converted){Hidden[Object.ObjectId.ToString().EndsWith(TEXT(".Left"))?1:0]=true;}
            if(Owner==Member.ActorId){Hidden[(bPlayback?PlaybackLeftHands.FindRef(Object.ObjectId):Object.bLeftHand)?1:0]=true;}
        }
        Wardrobe->SetHandPropsSuppressed(Hidden[0],Hidden[1]);
    }
}
void UStorySessionComponent::ObserveProp(FName Id,bool bBoundary)
{
    const auto* Found=PhysicalProps.Find(Id);auto* State=SceneState.FindObject(Id);if(!Found || !IsValid(*Found) || !State){return;}
    State->WorldTransform=(*Found)->GetBodyTransform();
    if(auto* Track=PropTracks.FindByPredicate([Id](const auto& T){return T.ObjectId==Id;})){Track->Observe(Movie->GetSceneTimeSeconds(),State->WorldTransform,bBoundary);}
}
bool UStorySessionComponent::GrabObject(FName Id)
{
    auto* State=SceneState.FindObject(Id);const auto* Prop=PhysicalProps.Find(Id);
    if(!CanAct() || !SessionScenario.AvailablePropActions.Contains(TEXT("Grab")) || !State || !Prop || !State->OwnerId.IsNone() || !IsReachable((*Prop)->GetOwner()->GetActorLocation(),(*Prop)->GetOwner())){return false;}
    const FName Role=Movie->GetCurrentRoleId();const bool Left=!GetHeldObject(Role).IsNone();if(Left && !GetHeldObject(Role,true).IsNone()){return false;}
    ObserveProp(Id,true);if(!(*Prop)->Hold(Movie->LivePerformer,Left)){return false;}State->OwnerId=Role;State->bLeftHand=Left;
    State->WorldTransform=(*Prop)->GetBodyTransform();AddSceneEvent(EStoryEventType::PropGrabbed,Role,Id,Role,State->WorldTransform,Left);UpdateHandSuppression(false);return true;
}
bool UStorySessionComponent::ReleaseObject(FName Id,bool bThrow)
{
    auto* State=SceneState.FindObject(Id);const auto* Found=PhysicalProps.Find(Id);
    if(!CanAct() || !State || !Found || State->OwnerId!=Movie->GetCurrentRoleId()){return false;}
    const FName Owner=State->OwnerId;FVector Direction=Movie->Controller->GetControlRotation().Vector();Direction.Z=FMath::Clamp(Direction.Z,-.1,.5);
    if(!(*Found)->Release(Movie->LivePerformer,bThrow,Direction,Movie->GetSceneTimeSeconds())){return false;}
    State->OwnerId=NAME_None;ObserveProp(Id,true);
    if(Id==SceneState.MoneyObjectId){SceneState.bMoneySecured=false;SceneState.bRobberEscaped=false;AddSceneEvent(EStoryEventType::MoneyDropped,Owner,Id,NAME_None,State->WorldTransform);}
    AddSceneEvent(bThrow?EStoryEventType::PropThrown:EStoryEventType::PropDropped,Owner,Id,NAME_None,State->WorldTransform,State->bLeftHand);
    if(bThrow){ThrownObjects.Add(Id);}UpdateHandSuppression(false);EvaluateContextObjective();return true;
}
bool UStorySessionComponent::ThrowHeldObject()
{
    if(!CanAct() || !SessionScenario.AvailablePropActions.Contains(TEXT("Throw"))){return false;}FName Id=GetHeldObject(Movie->GetCurrentRoleId());if(Id.IsNone()){Id=GetHeldObject(Movie->GetCurrentRoleId(),true);}return !Id.IsNone() && ReleaseObject(Id,true);
}
void UStorySessionComponent::SetPhysicsPaused(bool Paused){if(Paused){QueuedImpacts.Reset();}for(auto& Pair:PhysicalProps){if(IsValid(Pair.Value)){Pair.Value->SetScenePaused(Paused);}}}
void UStorySessionComponent::HandlePropImpact(UGrabbablePropComponent* Prop,AActor* Target,float Speed,const FVector& Location)
{if(Prop && Target && CanAct() && Speed>=Prop->MinimumImpactSpeed){QueuedImpacts.Add({Prop->ObjectId,Target,Speed,Location});}}
void UStorySessionComponent::UpdatePhysicalProps()
{
    if(!CanAct()){return;}const double Time=Movie->GetSceneTimeSeconds();
    for(auto& Pair:PhysicalProps)
    {
        auto* Prop=Pair.Value.Get();auto* State=SceneState.FindObject(Pair.Key);if(!Prop || !State){continue;}Prop->UpdateCollisionIgnore(Time);
        if(!State->OwnerId.IsNone()){continue;}
        if(Time<NextPropSampleTime){continue;}
        const FVector Position=Prop->GetOwner()->GetActorLocation();const FVector Floor=GroundLocation(Position,0);
        if(Position.ContainsNaN() || (SafetyBounds.IsValid() && SafetyBounds->NeedsRecovery(Position)) || FVector::DistSquared2D(Position,SceneAnchor)>FMath::Square(1500.0) || Position.Z<Floor.Z-150 || Position.Z>Floor.Z+1400)
        {
            FTransform Recovered=InitialSceneState.FindObject(Pair.Key)->WorldTransform;
            Recovered.SetLocation(GroundLocation(SceneAnchor+FVector(100,0,0),Prop->PhysicsBody->GetScaledBoxExtent().Z));Prop->RecoverTo(Recovered);ObserveProp(Pair.Key,true);
            AddSceneEvent(EStoryEventType::PropRecovered,NAME_None,Pair.Key,NAME_None,Recovered);
        }
        if(Time>=NextPropSampleTime){ObserveProp(Pair.Key);}
    }
    if(Time>=NextPropSampleTime){NextPropSampleTime=Time+.05;}
    const auto Impacts=MoveTemp(QueuedImpacts);QueuedImpacts.Reset();
    for(const auto& Impact:Impacts)
    {
        AActor* Target=Impact.Target.Get();const auto* Found=PhysicalProps.Find(Impact.ObjectId);if(!Target || !Found){continue;}
        const FName Key=FName(*(Impact.ObjectId.ToString()+TEXT("|")+Target->GetName()));const double* Last=LastImpactTimes.Find(Key);if(Last && Time-*Last<(*Found)->ImpactCooldownSeconds){continue;}LastImpactTimes.Add(Key,Time);
        FName TargetId;
        for(int32 Index=0;Index<NPCActors.Num();++Index){if(NPCActors[Index]==Target){TargetId=SceneState.NPCs[Index].Id;NPCInteractionIds.Add(SceneState.NPCs[Index].Type==EStoryNPCType::Teller?FName(TEXT("Teller")):SceneState.NPCs[Index].Type==EStoryNPCType::Guard?FName(TEXT("Guard")):FName(TEXT("Witness")));}}
        for(const auto& Member:Cast){if(Member.Pawn==Target){TargetId=Member.ActorId;}}
        AddSceneEvent(EStoryEventType::PropImpact,TargetId,Impact.ObjectId,NAME_None,FTransform(Impact.Location),Impact.Speed);
        if(!TargetId.IsNone())
        {
            if(TargetId==TEXT("NPC.Guard") && (*Found)->bDistractsGuard)
            {SetNPCMood(TargetId,EStoryNPCMood::Distracted,GuardDistractionSeconds,TEXT("Dance"));AddSceneEvent(EStoryEventType::GuardDistracted,TargetId);}
            else if(FindActor(TargetId))
            {
                ReactActor(TargetId,TEXT("Scared"));SceneState.DetainedUntil.Add(TargetId,Time+DetentionSeconds);
                if((*Found)->bCanKnockMoney && SceneState.MoneyOwner()==TargetId)
                {
                    auto* Money=SceneState.FindObject(SceneState.MoneyObjectId);auto* Bag=PhysicalProps.FindRef(Money->ObjectId).Get();
                    if(Bag && Bag->Release(Target,false,Target->GetActorForwardVector(),Time))
                    {Money->OwnerId=NAME_None;SceneState.bMoneySecured=SceneState.bRobberEscaped=false;ObserveProp(Money->ObjectId,true);AddSceneEvent(EStoryEventType::MoneyDropped,TargetId,Money->ObjectId,NAME_None,Money->WorldTransform);UpdateHandSuppression(false);}
                }
            }
            else{SetNPCMood(TargetId,EStoryNPCMood::Frightened,3,TEXT("Scared"));}
        }
        // A nearby impact is also an observable noise, even when it hits furniture.
        if(FVector::DistSquared(Impact.Location,GetInteractionLocation(TEXT("NPC.Guard")))<FMath::Square(450.0) && (*Found)->CosmeticId!=TEXT("Prop.Donut"))
        {const auto& Guard=SceneState.NPCs[1];if(Guard.Mood!=EStoryNPCMood::Distracted){SetNPCMood(Guard.Id,EStoryNPCMood::Alert,3,TEXT("Point"));}}
    }
    EvaluateContextObjective();
}
void UStorySessionComponent::ApplyPhysicalEvent(const FStorySceneEvent& Event)
{
    auto* Prop=PhysicalProps.FindRef(Event.ObjectId).Get();if(!Prop){return;}
    const bool Money=Event.Type==EStoryEventType::MoneyTaken || Event.Type==EStoryEventType::MoneyTransferred || Event.Type==EStoryEventType::MoneyConfiscated || Event.Type==EStoryEventType::MoneyDropped || Event.Type==EStoryEventType::MoneySecured;
    const bool Grab=Event.Type==EStoryEventType::PropGrabbed || Event.Type==EStoryEventType::PropCreated;
    const bool Release=Event.Type==EStoryEventType::PropDropped || Event.Type==EStoryEventType::PropThrown || Event.Type==EStoryEventType::PropRecovered;
    if(!Money && !Grab && !Release){return;}const FName Owner=(Money || Grab)?Event.Payload:NAME_None;PlaybackOwners.Add(Event.ObjectId,Owner);PlaybackLeftHands.Add(Event.ObjectId,Grab && Event.Duration>.5);Prop->GetOwner()->SetActorHiddenInGame(false);
    if(auto* Holder=Movie->FindReplayForRole(Owner)){Prop->PresentHeld(Holder,Grab && Event.Duration>.5);}else{Prop->PresentWorld(Event.WorldTransform);}
    UpdateHandSuppression(true);
}
void UStorySessionComponent::UpdatePropPlayback()
{
    const double Time=Movie->GetSceneTimeSeconds();
    for(const auto& Track:PropTracks)
    {
        auto* Prop=PhysicalProps.FindRef(Track.ObjectId).Get();if(!Prop){continue;}
        if(Track.Samples.IsEmpty() || Time+1.e-6<Track.Samples[0].TimeSeconds){Prop->GetOwner()->SetActorHiddenInGame(true);continue;}
        if(!PlaybackOwners.FindRef(Track.ObjectId).IsNone()){continue;}FTransform Transform;if(Track.Evaluate(Time,Transform)){Prop->GetOwner()->SetActorHiddenInGame(false);Prop->PresentWorld(Transform);}
    }
}
