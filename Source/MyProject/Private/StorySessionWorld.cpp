#include "StorySessionComponent.h"
#include "MovieManager.h"
#include "ActingComponent.h"
#include "ActingRecorderComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "WardrobeComponent.h"
#include "StudioCharacterProfile.h"
#include "StudioAudioProfile.h"
#include "ObjectiveManagerComponent.h"
#include "InteractableProp.h"
#include "ReplayActor.h"
#include "MovieResultsLibrary.h"
#include "GrabbablePropComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace StoryWorldPresentation
{
FText Ending(EStoryOutcome Outcome)
{
    switch(Outcome)
    {
    case EStoryOutcome::RobberEscaped:return NSLOCTEXT("FinalTake","EndingEscape","The robber escaped with the loot.");
    case EStoryOutcome::PoliceRecovered:return NSLOCTEXT("FinalTake","EndingPolice","The police recovered the money.");
    case EStoryOutcome::CivilianBetrayal:return NSLOCTEXT("FinalTake","EndingCivilian","The civilian was the real robber!");
    case EStoryOutcome::FailedRobbery:return NSLOCTEXT("FinalTake","EndingFailed","The robbery failed before it began.");
    default:return NSLOCTEXT("FinalTake","EndingChaos","Nobody understood the plan. The money is still in the bank.");
    }
}
}

FVector UStorySessionComponent::GroundLocation(FVector Location,double Height) const
{
    FHitResult Hit;FCollisionQueryParams Query(SCENE_QUERY_STAT(StoryWorldFloor),true);
    if(GetWorld()->LineTraceSingleByObjectType(Hit,Location+FVector(0,0,1000),Location-FVector(0,0,1500),FCollisionObjectQueryParams(ECC_WorldStatic),Query) && Hit.ImpactNormal.Z>.7){Location.Z=Hit.ImpactPoint.Z+Height+1;}
    return Location;
}

FText UStorySessionComponent::RoleName(int32 Index)
{
    if(Index==0){return NSLOCTEXT("FinalTake","RoleRobber","Robber");}
    if(Index==1){return NSLOCTEXT("FinalTake","RolePolice","Police Officer");}
    return NSLOCTEXT("FinalTake","RoleCivilian","Civilian");
}
FText UStorySessionComponent::RoleBrief(int32 Index)
{
    if(Index==0){return NSLOCTEXT("FinalTake","BriefRobber","Get the money. Plan an escape, and face what the other characters do.");}
    if(Index==1){return NSLOCTEXT("FinalTake","BriefPolice","Recover the money, intercept its holder, and give it to the guard.");}
    return NSLOCTEXT("FinalTake","BriefCivilian","Distract the guard or help the police. Your last turn can change who gets the money.");
}

bool UStorySessionComponent::InitializeScene()
{
    SceneState=FStoryWorldState();SceneEvents.Reset();InteractionPairs.Reset();CreativeIds.Reset();NPCInteractionIds.Reset();ThrownObjects.Reset();
    Outcome=EStoryOutcome::Unresolved;bScenePlayback=false;SceneEventCursor=0;
    SceneAnchor=Movie->MovieOrigin.GetLocation()+FVector(650,0,0);
    EscapeLocation=GroundLocation(SceneAnchor+FVector(-600,0,0));SecureLocation=GroundLocation(SceneAnchor+FVector(320,160,0),30);
    const FVector Starts[]={FVector(-180,0,0),FVector(20,250,0),FVector(20,-250,0)};
    for(int32 Index=0;Index<Cast.Num();++Index)
    {
        auto& Member=Cast[Index];double Height=96;
        if(auto* C=::Cast<ACharacter>(Member.Pawn)){Height=C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();}
        Member.InitialTransform.SetLocation(GroundLocation(SceneAnchor+Starts[Index],Height));
        Member.Pawn->SetActorTransform(Member.InitialTransform,false,nullptr,ETeleportType::TeleportPhysics);
        // Costume bags never create extra copies of the unique story money.
        if(Movie->SessionRoles[Index].DefaultOutfit.PropId==TEXT("Prop.MoneyBag")){Movie->SessionRoles[Index].DefaultOutfit.PropId=NAME_None;}
        Movie->SessionRoles[Index].Objectives.Reset();
    }
    for(TActorIterator<AInteractableProp> It(GetWorld());It;++It){if(It->PropId==TEXT("Prop.MoneyBag")){MoneyProp=*It;break;}}
    bSpawnedMoney=!MoneyProp;
    if(!MoneyProp){MoneyProp=GetWorld()->SpawnActor<AMoneyBagProp>();}
    if(!MoneyProp){return false;}
    OriginalMoneyTransform=MoneyProp->GetActorTransform();OriginalMoneyMesh=MoneyProp->PickupMesh->GetStaticMesh();OriginalMoneyMaterials.Reset();
    for(int32 Index=0;Index<MoneyProp->PickupMesh->GetNumMaterials();++Index){OriginalMoneyMaterials.Add(MoneyProp->PickupMesh->GetMaterial(Index));}
    const auto* Definition=Movie->LiveWardrobeComponent->Catalog.FindByPredicate([](const auto& Item){return Item.Id==TEXT("Prop.MoneyBag");});
    if(Definition && Definition->StaticMesh)
    {
        MoneyVisual=UWardrobeComponent::FreezeItem(*Definition,this);
        MoneyProp->PickupMesh->SetStaticMesh(Definition->StaticMesh);
        for(int32 Index=0;Index<Definition->Materials.Num();++Index){MoneyProp->PickupMesh->SetMaterial(Index,Definition->Materials[Index]);}
    }
    MoneyProp->PickupMesh->SetMobility(EComponentMobility::Movable);MoneyProp->SetActorHiddenInGame(false);MoneyProp->SetActorEnableCollision(false);
    FVector MoneyLocation=GroundLocation(SceneAnchor+FVector(-350,-430,0),0);
    const auto Bounds=MoneyProp->PickupMesh->Bounds;
    MoneyLocation.Z+=110+Bounds.BoxExtent.Z-(Bounds.Origin.Z-MoneyProp->GetActorLocation().Z);
    MoneyProp->SetActorLocation(MoneyLocation,false,nullptr,ETeleportType::TeleportPhysics);
    SceneState.MoneyObjectId=MoneyProp->PickupId;
    auto& Object=SceneState.Objects.AddDefaulted_GetRef();Object.ObjectId=MoneyProp->PickupId;Object.CosmeticId=MoneyProp->PropId;Object.WorldTransform=MoneyProp->GetActorTransform();
    const FName IDs[]={TEXT("NPC.Teller"),TEXT("NPC.Guard"),TEXT("NPC.Witness1"),TEXT("NPC.Witness2")};
    const FVector Positions[]={FVector(-210,-565,0),FVector(420,220,0),FVector(-80,80,0),FVector(350,-150,0)};
    for(int32 Index=0;Index<4;++Index)
    {
        auto* NPC=GetWorld()->SpawnActor<ACharacter>(ACharacter::StaticClass(),GroundLocation(SceneAnchor+Positions[Index]),FRotator(0,Index==0?90:180,0));
        if(!NPC){return false;}
        NPCActors.Add(NPC);NPC->AutoPossessAI=EAutoPossessAI::Disabled;NPC->GetCapsuleComponent()->SetCapsuleSize(42,96);
        NPC->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
        NPC->GetCharacterMovement()->StopMovementImmediately();NPC->GetCharacterMovement()->DisableMovement();NPC->GetCharacterMovement()->Deactivate();NPC->SetActorTickEnabled(false);
        if(Movie->CharacterProfile){Movie->CharacterProfile->ApplyModel(NPC);}
        auto* Wardrobe=NPC->FindComponentByClass<UWardrobeComponent>();
        if(!Wardrobe){Wardrobe=NewObject<UWardrobeComponent>(NPC);NPC->AddInstanceComponent(Wardrobe);Wardrobe->RegisterComponent();}
        Wardrobe->Catalog=Movie->LiveWardrobeComponent->Catalog;
        if(Movie->CharacterProfile){Wardrobe->FacialSettings=Movie->CharacterProfile->Face;}
        FCosmeticSelection Outfit=WardrobeDefaults::Casual();Outfit.PropId=NAME_None;
        Outfit.BodyId=Index==0?FName(TEXT("Body.Suit")):Index==1?FName(TEXT("Body.Uniform")):FName(TEXT("Body.Casual"));
        Outfit.HeadId=Index==1?FName(TEXT("Head.PoliceCap")):NAME_None;
        Wardrobe->BeginSelection(Outfit);Wardrobe->SetSelectionEnabled(false);
        auto* Acting=NewObject<UActingComponent>(NPC);NPC->AddInstanceComponent(Acting);Acting->RegisterComponent();
        if(Movie->CharacterProfile){Movie->CharacterProfile->ConfigureActing(Acting);}if(Movie->AudioProfile){Movie->AudioProfile->ConfigureActing(Acting);}Acting->SetLiveActionsEnabled(false);
        NPC->GetMesh()->bPauseAnims=true;
        auto& Data=SceneState.NPCs.AddDefaulted_GetRef();Data.Id=IDs[Index];Data.Type=Index==0?EStoryNPCType::Teller:Index==1?EStoryNPCType::Guard:EStoryNPCType::Witness;Data.WorldTransform=NPC->GetActorTransform();
    }
    if(!InitializePhysicalProps()){return false;}
    InitialSceneState=SceneState;return SceneState.HasUniqueMoney();
}

void UStorySessionComponent::ClearScene()
{
    ClearPhysicalProps();
    if(CreativeSource.IsValid()){CreativeSource->OnActingEventTriggeredNative.Remove(CreativeDelegate);}CreativeSource.Reset();CreativeDelegate.Reset();
    for(ACharacter* NPC:NPCActors){if(IsValid(NPC)){NPC->Destroy();}}NPCActors.Reset();
    if(IsValid(MoneyProp))
    {
        MoneyProp->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        if(bSpawnedMoney){MoneyProp->Destroy();}
        else
        {
            MoneyProp->PickupMesh->SetStaticMesh(OriginalMoneyMesh);for(int32 Index=0;Index<OriginalMoneyMaterials.Num();++Index){MoneyProp->PickupMesh->SetMaterial(Index,OriginalMoneyMaterials[Index]);}
            MoneyProp->ResetForTake();MoneyProp->SetActorTransform(OriginalMoneyTransform,false,nullptr,ETeleportType::TeleportPhysics);
        }
    }
    for(auto& Member:Cast){if(IsValid(Member.Pawn)){if(auto* W=Member.Pawn->FindComponentByClass<UWardrobeComponent>()){W->SetHandPropsSuppressed(false,false);}}}
    MoneyProp=nullptr;OriginalMoneyMesh=nullptr;OriginalMoneyMaterials.Reset();MoneyVisual=FCosmeticDefinition();bSpawnedMoney=false;bScenePlayback=false;SceneEventCursor=0;
    SceneState=InitialSceneState=FinalSceneState=FStoryWorldState();SceneEvents.Reset();CurrentObjective=FMovieObjectiveDefinition();
    InteractionPairs.Reset();CreativeIds.Reset();NPCInteractionIds.Reset();ThrownObjects.Reset();Outcome=EStoryOutcome::Unresolved;
}

bool UStorySessionComponent::CanAct() const
{
    return IsCurrent() && !bScenePlayback && !bPendingEnd && Movie->State==EMovieState::Recording && Movie->Recorder && Movie->Recorder->IsRecording()
        && Movie->GetSceneTimeSeconds()-TurnStartSeconds<Movie->SessionRecordingDuration && Movie->Controller.IsValid() && Movie->Controller->GetPawn()==Movie->LivePerformer;
}
bool UStorySessionComponent::IsReachable(FVector Location,AActor* Target) const
{
    if(!CanAct() || FVector::DistSquared(Movie->LivePerformer->GetActorLocation(),Location)>FMath::Square(InteractionDistance)){return false;}
    FCollisionQueryParams Query(SCENE_QUERY_STAT(StoryInteractionSight),true,Movie->LivePerformer);if(Target){Query.AddIgnoredActor(Target);}if(MoneyProp){Query.AddIgnoredActor(MoneyProp);}
    FHitResult Hit;return !GetWorld()->LineTraceSingleByChannel(Hit,Movie->LivePerformer->GetActorLocation()+FVector(0,0,60),Location+FVector(0,0,50),ECC_Visibility,Query);
}
FVector UStorySessionComponent::GetInteractionLocation(FName Id) const
{
    if(Id==TEXT("Scene.Exit")){return EscapeLocation;}if(Id==TEXT("Scene.Secure")){return SecureLocation;}
    if(Id==SceneState.MoneyObjectId && MoneyProp){return MoneyProp->GetActorLocation();}
    if(auto* Prop=PhysicalProps.FindRef(Id).Get()){return Prop->GetOwner()->GetActorLocation();}
    if(APawn* Pawn=FindActor(Id)){return Pawn->GetActorLocation();}
    for(int32 Index=0;Index<SceneState.NPCs.Num();++Index){if(SceneState.NPCs[Index].Id==Id && NPCActors.IsValidIndex(Index)){return NPCActors[Index]->GetActorLocation();}}
    return FVector(TNumericLimits<double>::Max()/4);
}
FName UStorySessionComponent::FindInteractionTarget() const
{
    if(!CanAct()){return NAME_None;}FName Best;double BestScore=-TNumericLimits<double>::Max();
    const auto Consider=[&](FName Id,double Priority,AActor* Actor=nullptr)
    {
        const FVector Location=GetInteractionLocation(Id);if(!IsReachable(Location,Actor)){return;}
        if((Id==TEXT("Scene.Exit") || Id==TEXT("Scene.Secure")) && FVector::DistSquared(Location,Movie->LivePerformer->GetActorLocation())>FMath::Square(160.0)){return;}
        const double Score=Priority-FVector::Distance(Location,Movie->LivePerformer->GetActorLocation());if(Score>BestScore){BestScore=Score;Best=Id;}
    };
    const FName Role=Movie->GetCurrentRoleId();const FName Owner=SceneState.MoneyOwner();
    const auto* Guard=SceneState.NPCs.FindByPredicate([](const auto& N){return N.Type==EStoryNPCType::Guard;});
    const bool Protected=SceneState.bMoneySecured && GetRoleIndex()!=1 && (!Guard || Guard->Mood!=EStoryNPCMood::Distracted);
    if(Owner!=Role && !Protected){Consider(SceneState.MoneyObjectId,800,MoneyProp);}
    if(!Owner.IsNone() && Owner!=Role){Consider(Owner,900,FindActor(Owner));}
    if(Owner==Role){Consider(GetRoleIndex()==1?FName(TEXT("Scene.Secure")):FName(TEXT("Scene.Exit")),700);}
    for(int32 Index=0;Index<SceneState.NPCs.Num();++Index){Consider(SceneState.NPCs[Index].Id,100,NPCActors.IsValidIndex(Index)?NPCActors[Index]:nullptr);}
    for(const auto& Member:Cast){if(Member.ActorId!=Role){Consider(Member.ActorId,150,Member.Pawn);}}
    if(GetHeldObject(Role).IsNone() || GetHeldObject(Role,true).IsNone())
    {
        for(const auto& Object:SceneState.Objects)
        {if(Object.ObjectId!=SceneState.MoneyObjectId && Object.OwnerId.IsNone()){if(auto* Prop=PhysicalProps.FindRef(Object.ObjectId).Get()){Consider(Object.ObjectId,420,Prop->GetOwner());}}}
    }
    return Best;
}
FText UStorySessionComponent::GetInteractionPrompt() const
{
    const FName Target=FindInteractionTarget();if(Target.IsNone()){return FText::GetEmpty();}
    if(Target==TEXT("Scene.Exit")){return NSLOCTEXT("FinalTake","PromptEscape","[E] Leave with the money");}
    if(Target==TEXT("Scene.Secure")){return NSLOCTEXT("FinalTake","PromptSecure","[E] Secure the money");}
    if(Target==SceneState.MoneyObjectId || (!SceneState.MoneyOwner().IsNone() && Target==SceneState.MoneyOwner()))
    {return GetRoleIndex()==1?NSLOCTEXT("FinalTake","PromptConfiscate","[E] Confiscate money"):NSLOCTEXT("FinalTake","PromptTakeMoney","[E] Take the money");}
    if(Target==TEXT("NPC.Guard")){return GetRoleIndex()==1?NSLOCTEXT("FinalTake","PromptGuardInfo","[E] Talk to the guard"):GetRoundNumber()==2 && (SceneState.bMoneySecured || SceneState.MoneyOwner()==Cast[1].ActorId)?NSLOCTEXT("FinalTake","PromptAssist","[E] Help the police"):NSLOCTEXT("FinalTake","PromptDistract","[E] Distract the guard");}
    if(Target==TEXT("NPC.Teller")){return NSLOCTEXT("FinalTake","PromptTeller","[E] Talk to the teller");}
    if(FindActor(Target)){return GetRoleIndex()==1?NSLOCTEXT("FinalTake","PromptIntercept","[E] Intercept"):NSLOCTEXT("FinalTake","PromptCoordinate","[E] Coordinate a distraction");}
    if(PhysicalProps.Contains(Target)){return NSLOCTEXT("FinalTake","PromptGrab","[E] Grab object · [Click] Throw · [Q] Drop");}
    return NSLOCTEXT("FinalTake","PromptWitness","[E] Ask the witness");
}

void UStorySessionComponent::AddSceneEvent(EStoryEventType Type,FName Target,FName Object,FName Payload,const FTransform& Transform,double Duration)
{
    if(!IsCurrent() || bScenePlayback || (Movie->State!=EMovieState::Recording && Type!=EStoryEventType::PresentationStop)){return;}
    auto& Event=SceneEvents.AddDefaulted_GetRef();Event.Sequence=SceneEvents.Num();Event.TimeSeconds=Type==EStoryEventType::PresentationStop?Movie->GetSceneTimeSeconds():FMath::Min(Movie->GetSceneTimeSeconds(),TurnStartSeconds+Movie->SessionRecordingDuration);
    Event.Type=Type;Event.InstigatorId=Movie->GetCurrentRoleId();Event.TargetId=Target;Event.ObjectId=Object;Event.Payload=Payload;Event.WorldTransform=Transform;Event.Duration=Duration;
    PlaySceneEventAudio(Event,false);
}
void UStorySessionComponent::ApplyMoneyPresentation(FName Owner,const FTransform& Transform,bool bPlayback)
{
    if(!IsValid(MoneyProp)){return;}
    if(auto* Prop=PhysicalProps.FindRef(SceneState.MoneyObjectId).Get())
    {
        AActor* Holder=bPlayback?static_cast<AActor*>(Movie->FindReplayForRole(Owner)):static_cast<AActor*>(FindActor(Owner));
        if(Holder){Prop->PresentHeld(Holder,false);}else if(bPlayback){Prop->PresentWorld(Transform);}else{Prop->RecoverTo(Transform);}
        MoneyProp->SetActorHiddenInGame(false);UpdateHandSuppression(bPlayback);return;
    }
    MoneyProp->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);MoneyProp->SetActorHiddenInGame(false);MoneyProp->SetActorEnableCollision(false);
    AActor* Holder=bPlayback?static_cast<AActor*>(Movie->FindReplayForRole(Owner)):static_cast<AActor*>(FindActor(Owner));
    auto* Mesh=Holder?Holder->FindComponentByClass<USkeletalMeshComponent>():nullptr;
    const FCosmeticDefinition* Definition=MoneyVisual.Id.IsNone()?nullptr:&MoneyVisual;
    if(Holder && Mesh && Mesh->DoesSocketExist(TEXT("RightHand")) && Definition)
    {MoneyProp->AttachToComponent(Mesh,FAttachmentTransformRules::KeepWorldTransform,TEXT("RightHand"));MoneyProp->SetActorRelativeTransform(Definition->RelativeTransform);}
    else if(Holder){MoneyProp->AttachToActor(Holder,FAttachmentTransformRules::KeepWorldTransform);MoneyProp->SetActorRelativeLocation(FVector(45,25,0));}
    else {MoneyProp->SetActorTransform(Transform,false,nullptr,ETeleportType::TeleportPhysics);}
    for(const auto& Member:Cast)
    {
        AActor* Actor=bPlayback?static_cast<AActor*>(Movie->FindReplayForRole(Member.ActorId)):static_cast<AActor*>(Member.Pawn.Get());
        if(auto* Wardrobe=Actor?Actor->FindComponentByClass<UWardrobeComponent>():nullptr)
        {const auto Selection=Wardrobe->GetSelection();Wardrobe->SetHandPropsSuppressed(Owner==Member.ActorId || Selection.PropId==TEXT("Prop.MoneyBag"),Selection.LeftHandPropId==TEXT("Prop.MoneyBag"));}
    }
}
bool UStorySessionComponent::SetMoneyOwner(FName Owner,EStoryEventType Reason,FName PreviousOwner)
{
    auto* Money=SceneState.FindObject(SceneState.MoneyObjectId);if(!Money || (!Owner.IsNone() && !FindActor(Owner)) || Owner==Money->OwnerId){return false;}
    const FName Old=Money->OwnerId;
    if(!PreviousOwner.IsNone() && PreviousOwner!=Old){return false;}
    if(!Owner.IsNone())
    {
        const FName Occupied=GetHeldObject(Owner);if(!Occupied.IsNone() && Occupied!=Money->ObjectId)
        {if(!ReleaseObject(Occupied,false)){return false;}}
        ObserveProp(Money->ObjectId,true);
    }
    Money->OwnerId=Owner;Money->bLeftHand=false;SceneState.bMoneySecured=Reason==EStoryEventType::MoneySecured;SceneState.bRobberEscaped=false;
    if(!Owner.IsNone())
    {
        if(!SceneState.bMoneyWasTaken){for(const auto& NPC:SceneState.NPCs){if(NPC.Type==EStoryNPCType::Witness && FVector::DistSquared(NPC.WorldTransform.GetLocation(),MoneyProp->GetActorLocation())<FMath::Square(700.0)){SetNPCMood(NPC.Id,EStoryNPCMood::Frightened,3,TEXT("Scared"));}}}
        SceneState.bMoneyWasTaken=true;
    }
    if(Owner.IsNone())
    {
        FVector Location=SecureLocation;
        if(Reason!=EStoryEventType::MoneySecured)
        {
            const FVector Origin=Movie->LivePerformer->GetActorLocation();
            const FVector Offsets[]={Movie->LivePerformer->GetActorForwardVector()*100,Movie->LivePerformer->GetActorRightVector()*120,-Movie->LivePerformer->GetActorRightVector()*120,-Movie->LivePerformer->GetActorForwardVector()*120};
            Location=InitialSceneState.FindObject(SceneState.MoneyObjectId)->WorldTransform.GetLocation();
            for(const FVector& Offset:Offsets)
            {
                const FVector Candidate=Origin+Offset;bool Blocked=false;
                for(const auto& Member:Cast){Blocked|=Member.Pawn!=Movie->LivePerformer && FVector::DistSquared2D(Member.Pawn->GetActorLocation(),Candidate)<FMath::Square(100.0);}
                for(ACharacter* NPC:NPCActors){Blocked|=FVector::DistSquared2D(NPC->GetActorLocation(),Candidate)<FMath::Square(100.0);}
                if(!Blocked){Location=GroundLocation(Candidate,30);break;}
            }
        }
        Money->WorldTransform=MoneyProp->GetActorTransform();Money->WorldTransform.SetLocation(Location);Money->WorldTransform.SetRotation(OriginalMoneyTransform.GetRotation());
        const FVector Floor=GroundLocation(Location,0);
        if(auto* Prop=PhysicalProps.FindRef(Money->ObjectId).Get()){Money->WorldTransform.SetLocation(FVector(Location.X,Location.Y,Floor.Z+Prop->PhysicsBody->GetScaledBoxExtent().Z));}
        else{const auto Bounds=MoneyProp->PickupMesh->CalcBounds(Money->WorldTransform);Money->WorldTransform.AddToTranslation(FVector(0,0,Floor.Z-(Bounds.Origin.Z-Bounds.BoxExtent.Z)));}
    }
    ApplyMoneyPresentation(Owner,Money->WorldTransform,false);Money->WorldTransform=MoneyProp->GetActorTransform();
    if(Owner.IsNone()){ObserveProp(Money->ObjectId,true);}
    AddSceneEvent(Reason,Old,Money->ObjectId,Owner,Money->WorldTransform);
    if(!Old.IsNone() && Old!=Owner && Old!=Movie->GetCurrentRoleId()){InteractionPairs.Add(FName(*(Movie->GetCurrentRoleId().ToString()+TEXT("|")+Old.ToString())));}
    if(!Old.IsNone() && Old!=Owner){ReactActor(Old,TEXT("Scared"));}
    return SceneState.HasUniqueMoney();
}

void UStorySessionComponent::ReactActor(FName Target,FName Gesture)
{
    APawn* Pawn=FindActor(Target);if(!Pawn || Pawn==Movie->LivePerformer){return;}
    if(auto* Character=::Cast<ACharacter>(Pawn)){Character->GetMesh()->bPauseAnims=false;}
    if(auto* Acting=Pawn->FindComponentByClass<UActingComponent>()){Acting->PresentSceneGesture(Gesture,Movie->GetSceneTimeSeconds(),true);}
    AddSceneEvent(EStoryEventType::NPCReaction,Target,NAME_None,Gesture);
}
void UStorySessionComponent::SetNPCMood(FName Id,EStoryNPCMood Mood,double Duration,FName Reaction)
{
    const int32 Index=SceneState.NPCs.IndexOfByPredicate([Id](const auto& NPC){return NPC.Id==Id;});if(Index==INDEX_NONE){return;}
    auto& State=SceneState.NPCs[Index];if(State.Mood==Mood && State.EffectUntilSeconds>Movie->GetSceneTimeSeconds()){return;}
    State.Mood=Mood;State.EffectUntilSeconds=Duration>0?Movie->GetSceneTimeSeconds()+Duration:0;
    AddSceneEvent(EStoryEventType::NPCStateChanged,Id,NAME_None,FName(*FString::FromInt(static_cast<int32>(Mood))),State.WorldTransform,Duration);
    if(!Reaction.IsNone() && NPCActors.IsValidIndex(Index))
    {
        NPCActors[Index]->GetMesh()->bPauseAnims=false;
        if(auto* Acting=NPCActors[Index]->FindComponentByClass<UActingComponent>()){Acting->PresentSceneGesture(Reaction,Movie->GetSceneTimeSeconds(),true);}
        AddSceneEvent(EStoryEventType::NPCReaction,Id,NAME_None,Reaction);
    }
}
bool UStorySessionComponent::Interact(){return InteractWith(FindInteractionTarget());}
bool UStorySessionComponent::InteractWith(FName Target)
{
    if(!CanAct() || Target.IsNone()){return false;}
    if(Target!=SceneState.MoneyObjectId && PhysicalProps.Contains(Target)){return GrabObject(Target);}
    AActor* TargetActor=Target==SceneState.MoneyObjectId?static_cast<AActor*>(MoneyProp.Get()):static_cast<AActor*>(FindActor(Target));
    for(int32 Index=0;Index<SceneState.NPCs.Num();++Index){if(SceneState.NPCs[Index].Id==Target && NPCActors.IsValidIndex(Index)){TargetActor=NPCActors[Index];}}
    if(!IsReachable(GetInteractionLocation(Target),TargetActor)){return false;}
    if((Target==TEXT("Scene.Exit") || Target==TEXT("Scene.Secure")) && FVector::DistSquared(GetInteractionLocation(Target),Movie->LivePerformer->GetActorLocation())>FMath::Square(160.0)){return false;}
    const FName Role=Movie->GetCurrentRoleId();const FName Owner=SceneState.MoneyOwner();bool Success=false;
    const auto* Guard=SceneState.NPCs.FindByPredicate([](const auto& N){return N.Type==EStoryNPCType::Guard;});
    const bool Distracted=Guard && Guard->Mood==EStoryNPCMood::Distracted;
    if(Target==SceneState.MoneyObjectId || (!Owner.IsNone() && Target==Owner && Owner!=Role))
    {
        if(Owner==Role || (SceneState.bMoneySecured && !Distracted && GetRoleIndex()!=1)){return false;}
        Success=SetMoneyOwner(Role,Owner.IsNone()?EStoryEventType::MoneyTaken:GetRoleIndex()==1?EStoryEventType::MoneyConfiscated:EStoryEventType::MoneyTransferred,Owner);
        if(Success){SceneState.bAlarmActive=true;AddSceneEvent(EStoryEventType::AlarmChanged,NAME_None,NAME_None,TEXT("On"));SetNPCMood(TEXT("NPC.Teller"),EStoryNPCMood::Frightened,4,TEXT("Scared"));if(!Distracted){SetNPCMood(TEXT("NPC.Guard"),EStoryNPCMood::Alert,0,TEXT("Point"));}}
    }
    else if(Target==TEXT("Scene.Secure") && GetRoleIndex()==1 && Owner==Role){Success=SetMoneyOwner(NAME_None,EStoryEventType::MoneySecured,Role);}
    else if(Target==TEXT("Scene.Exit") && Owner==Role && GetRoleIndex()!=1)
    {
        if(GetRoleIndex()==0 && SceneState.bAlarmActive && !Distracted){return false;}
        SceneState.bRobberEscaped=GetRoleIndex()==0;AddSceneEvent(EStoryEventType::Escape,Role,SceneState.MoneyObjectId);Success=true;
    }
    else if(Target==TEXT("NPC.Teller"))
    {
        NPCInteractionIds.Add(TEXT("Teller"));
        if(GetRoleIndex()==0 && Owner.IsNone() && !SceneState.bMoneySecured){Success=SetMoneyOwner(Role,EStoryEventType::MoneyTaken);SceneState.bAlarmActive=true;AddSceneEvent(EStoryEventType::AlarmChanged,Target,NAME_None,TEXT("On"));SetNPCMood(Target,EStoryNPCMood::Frightened,4,TEXT("Scared"));if(!Distracted){SetNPCMood(TEXT("NPC.Guard"),EStoryNPCMood::Alert,0,TEXT("Point"));}}
        else{AddSceneEvent(EStoryEventType::NPCInformation,Target);SetNPCMood(Target,EStoryNPCMood::Helpful,3,TEXT("Point"));Success=true;}
    }
    else if(Target==TEXT("NPC.Guard"))
    {
        NPCInteractionIds.Add(TEXT("Guard"));
        if(GetRoleIndex()==1 && Owner==Role){Success=SetMoneyOwner(NAME_None,EStoryEventType::MoneySecured,Role);}
        if(GetRoleIndex()==1 || (GetRoleIndex()==2 && GetRoundNumber()==2 && (SceneState.bMoneySecured || Owner==Cast[1].ActorId)))
        {SceneState.bAlarmActive=false;AddSceneEvent(EStoryEventType::AlarmChanged,Target,NAME_None,TEXT("Off"));SetNPCMood(Target,EStoryNPCMood::Helpful,0,TEXT("Point"));AddSceneEvent(EStoryEventType::NPCAssisted,Cast[1].ActorId,Target);if(Role!=Cast[1].ActorId){InteractionPairs.Add(FName(*(Role.ToString()+TEXT("|")+Cast[1].ActorId.ToString())));}}
        else{SetNPCMood(Target,EStoryNPCMood::Distracted,GuardDistractionSeconds,TEXT("Dance"));AddSceneEvent(EStoryEventType::GuardDistracted,Target);}
        Success=true;
    }
    else if(APawn* Other=FindActor(Target))
    {
        if(Other==Movie->LivePerformer){return false;}
        InteractionPairs.Add(FName(*(Role.ToString()+TEXT("|")+Target.ToString())));
        if(GetRoleIndex()==1){SceneState.DetainedUntil.Add(Target,Movie->GetSceneTimeSeconds()+DetentionSeconds);SceneState.bRobberEscaped=false;AddSceneEvent(EStoryEventType::Detain,Target,NAME_None,NAME_None,Other->GetActorTransform(),DetentionSeconds);ReactActor(Target,TEXT("Scared"));}
        else{AddSceneEvent(EStoryEventType::NPCAssisted,Target);ReactActor(Target,TEXT("Point"));}Success=true;
    }
    else
    {
        const auto* NPC=SceneState.NPCs.FindByPredicate([Target](const auto& N){return N.Id==Target && N.Type==EStoryNPCType::Witness;});
        if(NPC){NPCInteractionIds.Add(TEXT("Witness"));AddSceneEvent(EStoryEventType::NPCInformation,Target);SetNPCMood(Target,EStoryNPCMood::Helpful,3,TEXT("Point"));Success=true;}
    }
    if(Success){EvaluateContextObjective();}return Success;
}
bool UStorySessionComponent::DropHeldObject()
{
    if(!CanAct()){return false;}FName Id=GetHeldObject(Movie->GetCurrentRoleId());if(Id.IsNone()){Id=GetHeldObject(Movie->GetCurrentRoleId(),true);}return !Id.IsNone() && ReleaseObject(Id,false);
}

void UStorySessionComponent::PreviewContextObjective(bool bBindCreative)
{
    if(CreativeSource.IsValid()){CreativeSource->OnActingEventTriggeredNative.Remove(CreativeDelegate);}CreativeSource.Reset();CreativeDelegate.Reset();
    if(bBindCreative){CreativeSource=Movie->LiveActingComponent;CreativeDelegate=Movie->LiveActingComponent->OnActingEventTriggeredNative.AddUObject(this,&UStorySessionComponent::HandleCreativeAction);}
    CurrentObjective=FMovieObjectiveDefinition();CurrentObjective.Id=FName(*FString::Printf(TEXT("Story.%s.Round%d"),*Movie->GetCurrentRoleId().ToString(),GetRoundNumber()));CurrentObjective.Type=EMovieObjectiveType::SceneAction;CurrentObjective.Points=50;
    const FName Owner=SceneState.MoneyOwner();
    if(GetRoleIndex()==0)
    {
        if(Owner==Movie->GetCurrentRoleId()){ObjectiveKind=EStoryObjectiveKind::Escape;CurrentObjective.DisplayName=NSLOCTEXT("FinalTake","ObjectiveEscape","Escape with the money: distract the guard and reach the exit.");}
        else if(SceneState.bMoneySecured){ObjectiveKind=EStoryObjectiveKind::AskWitness;CurrentObjective.DisplayName=NSLOCTEXT("FinalTake","ObjectiveAfterRobbery","Ask a witness how your brilliant plan ended.");}
        else{ObjectiveKind=EStoryObjectiveKind::GetMoney;CurrentObjective.DisplayName=Owner.IsNone()?NSLOCTEXT("FinalTake","ObjectiveGetMoney","Get the money from the counter or talk to the teller."):NSLOCTEXT("FinalTake","ObjectiveRecoverMoney","Recover the money from whoever is carrying it.");}
    }
    else if(GetRoleIndex()==1)
    {
        if(Owner==Movie->GetCurrentRoleId()){ObjectiveKind=EStoryObjectiveKind::SecureMoney;CurrentObjective.DisplayName=NSLOCTEXT("FinalTake","ObjectiveSecureMoney","Give the money to the guard to secure it.");}
        else if(SceneState.bMoneySecured){ObjectiveKind=EStoryObjectiveKind::AssistPolice;CurrentObjective.DisplayName=NSLOCTEXT("FinalTake","ObjectiveGuardDebrief","Talk to the guard and secure the scene.");}
        else{ObjectiveKind=EStoryObjectiveKind::ConfiscateMoney;CurrentObjective.DisplayName=Owner.IsNone()?NSLOCTEXT("FinalTake","ObjectivePoliceCollect","Pick up the money before someone else takes it."):NSLOCTEXT("FinalTake","ObjectiveConfiscate","Approach the money holder and confiscate it.");}
    }
    else
    {
        if(Owner==Movie->GetCurrentRoleId()){ObjectiveKind=EStoryObjectiveKind::Escape;CurrentObjective.DisplayName=NSLOCTEXT("FinalTake","ObjectiveCivilEscape","Leave the bank with your unexpected loot.");}
        else if(GetRoundNumber()==1){ObjectiveKind=EStoryObjectiveKind::DistractGuard;CurrentObjective.DisplayName=NSLOCTEXT("FinalTake","ObjectiveDistract","Distract the guard. The next actor must deal with it.");}
        else{ObjectiveKind=EStoryObjectiveKind::AssistPolice;CurrentObjective.DisplayName=NSLOCTEXT("FinalTake","ObjectiveCivilAftermath","Help the guard close the case... or change the ending.");}
    }
    Movie->ObjectiveManager->PreviewRole(Movie,Movie->GetCurrentRoleId(),{CurrentObjective});
}
FText UStorySessionComponent::GetPrimaryObjectiveText() const{return CurrentObjective.DisplayName;}
void UStorySessionComponent::EvaluateContextObjective()
{
    if(!CanAct()){return;}const FName Role=Movie->GetCurrentRoleId();bool Complete=false;
    switch(ObjectiveKind)
    {
    case EStoryObjectiveKind::GetMoney:case EStoryObjectiveKind::RecoverMoney:case EStoryObjectiveKind::ConfiscateMoney:Complete=SceneState.MoneyOwner()==Role;break;
    case EStoryObjectiveKind::SecureMoney:Complete=SceneState.bMoneySecured;break;
    case EStoryObjectiveKind::Escape:for(const auto& E:SceneEvents){Complete|=E.Type==EStoryEventType::Escape && E.InstigatorId==Role && E.TimeSeconds>=TurnStartSeconds;}break;
    case EStoryObjectiveKind::DistractGuard:for(const auto& N:SceneState.NPCs){Complete|=N.Type==EStoryNPCType::Guard && N.Mood==EStoryNPCMood::Distracted;}break;
    case EStoryObjectiveKind::AskWitness:for(const auto& E:SceneEvents){Complete|=E.Type==EStoryEventType::NPCInformation && E.InstigatorId==Role && E.TimeSeconds>=TurnStartSeconds;}break;
    case EStoryObjectiveKind::AssistPolice:case EStoryObjectiveKind::ReactToEnding:for(const auto& E:SceneEvents){Complete|=E.Type==EStoryEventType::NPCAssisted && E.InstigatorId==Role && E.TimeSeconds>=TurnStartSeconds;}break;
    }
    if(Complete){Movie->ObjectiveManager->CompleteSceneObjective(CurrentObjective.Id,Movie->GetSceneTimeSeconds());}
}
void UStorySessionComponent::HandleCreativeAction(const FActingEvent& Event,bool bFromReplay)
{
    if(bFromReplay || !CanAct() || (Event.Type!=EActingEventType::Gesture && Event.Type!=EActingEventType::Sound)){return;}
    CreativeIds.Add(FName(*(FString::FromInt(static_cast<int32>(Event.Type))+TEXT("|")+Event.Identifier.ToString())));
    if(Event.Identifier==TEXT("Dance") && IsReachable(GetInteractionLocation(TEXT("NPC.Guard")),NPCActors.IsValidIndex(1)?NPCActors[1]:nullptr))
    {SetNPCMood(TEXT("NPC.Guard"),EStoryNPCMood::Distracted,GuardDistractionSeconds,TEXT("Dance"));NPCInteractionIds.Add(TEXT("Guard"));AddSceneEvent(EStoryEventType::GuardDistracted,TEXT("NPC.Guard"));EvaluateContextObjective();}
    if((Event.Identifier==TEXT("Point") || Event.Identifier==TEXT("EvilLaugh")) && GetRoleIndex()==0 && IsReachable(GetInteractionLocation(TEXT("NPC.Teller")),NPCActors.IsValidIndex(0)?NPCActors[0]:nullptr))
    {SetNPCMood(TEXT("NPC.Teller"),EStoryNPCMood::Frightened,4,TEXT("Scared"));NPCInteractionIds.Add(TEXT("Teller"));}
    if(Event.Type==EActingEventType::Sound && (Event.Identifier==TEXT("Scream") || Event.Identifier==TEXT("PoliceSiren")))
    {for(const auto& NPC:SceneState.NPCs){if(FVector::DistSquared(NPC.WorldTransform.GetLocation(),Movie->LivePerformer->GetActorLocation())<FMath::Square(500.0) && NPC.Mood!=EStoryNPCMood::Distracted){SetNPCMood(NPC.Id,NPC.Type==EStoryNPCType::Guard?EStoryNPCMood::Alert:EStoryNPCMood::Frightened,3,NPC.Type==EStoryNPCType::Guard?FName(TEXT("Point")):FName(TEXT("Scared")));}}}
}

void UStorySessionComponent::UpdateScene()
{
    UpdatePhysicalProps();
    const bool Running=Movie->SceneClock->IsClockRunning();const double Time=Movie->GetSceneTimeSeconds();
    for(int32 Index=0;Index<NPCActors.Num();++Index)
    {
        if(auto* Acting=NPCActors[Index]->FindComponentByClass<UActingComponent>()){Acting->UpdateScenePresentation(Time,Running);}
        NPCActors[Index]->GetMesh()->bPauseAnims=!Running;
        if(Running && !bPendingEnd && SceneState.NPCs[Index].EffectUntilSeconds>0 && Time>=SceneState.NPCs[Index].EffectUntilSeconds)
        {SetNPCMood(SceneState.NPCs[Index].Id,SceneState.bAlarmActive?EStoryNPCMood::Alert:EStoryNPCMood::Idle,0);}
    }
    for(int32 Index=0;Index<Cast.Num();++Index)
    {if(Index!=GetRoleIndex()){if(auto* Acting=Cast[Index].Pawn->FindComponentByClass<UActingComponent>()){Acting->UpdateScenePresentation(Time,Running);}}}
    if(MoneyProp){if(auto* Money=SceneState.FindObject(SceneState.MoneyObjectId)){Money->WorldTransform=MoneyProp->GetActorTransform();}}
}

void UStorySessionComponent::AppendSceneActingEvents(FName Id,FActingRecording& Recording) const
{
    for(const auto& Event:SceneEvents)
    {
        if(Event.TargetId!=Id || (Event.Type!=EStoryEventType::NPCReaction && Event.Type!=EStoryEventType::PresentationStop)){continue;}
        auto& Acting=Recording.Events.AddDefaulted_GetRef();Acting.TimeSeconds=Event.TimeSeconds;Acting.Type=Event.Type==EStoryEventType::NPCReaction?EActingEventType::Gesture:EActingEventType::PresentationStop;Acting.Identifier=Event.Payload;
    }
    Recording.Events.StableSort([](const auto& A,const auto& B)
    {
        if(A.TimeSeconds!=B.TimeSeconds){return A.TimeSeconds<B.TimeSeconds;}
        const int32 PA=A.Type==EActingEventType::PresentationStop?(A.Identifier==TEXT("TurnStart")?-1:1):0;
        const int32 PB=B.Type==EActingEventType::PresentationStop?(B.Identifier==TEXT("TurnStart")?-1:1):0;
        return PA<PB;
    });
}
void UStorySessionComponent::BeginScenePlayback()
{
    bScenePlayback=true;SceneEventCursor=0;SetPhysicsPaused(true);PlaybackOwners.Reset();PlaybackLeftHands.Reset();
    for(const auto& Object:InitialSceneState.Objects)
    {PlaybackOwners.Add(Object.ObjectId,Object.OwnerId);PlaybackLeftHands.Add(Object.ObjectId,Object.bLeftHand);if(auto* Prop=PhysicalProps.FindRef(Object.ObjectId).Get()){Prop->PresentWorld(Object.WorldTransform);Prop->GetOwner()->SetActorHiddenInGame(Object.CreatedAtSeconds>0);}}
    for(int32 Index=0;Index<NPCActors.Num();++Index)
    {
        NPCActors[Index]->SetActorTransform(InitialSceneState.NPCs[Index].WorldTransform,false,nullptr,ETeleportType::TeleportPhysics);NPCActors[Index]->GetMesh()->bPauseAnims=false;
        if(auto* A=NPCActors[Index]->FindComponentByClass<UActingComponent>()){A->ResetPlaybackState();}
    }
    const auto* InitialMoney=InitialSceneState.FindObject(InitialSceneState.MoneyObjectId);
    if(InitialMoney){ApplyMoneyPresentation(InitialMoney->OwnerId,InitialMoney->WorldTransform,true);}UpdateHandSuppression(true);
}
void UStorySessionComponent::ApplyPlaybackEvent(const FStorySceneEvent& Event)
{
    PlaySceneEventAudio(Event,true);
    ApplyPhysicalEvent(Event);
    if(Event.Type==EStoryEventType::NPCReaction)
    {
        for(int32 Index=0;Index<InitialSceneState.NPCs.Num();++Index)
        {if(InitialSceneState.NPCs[Index].Id==Event.TargetId){if(auto* Acting=NPCActors[Index]->FindComponentByClass<UActingComponent>()){Acting->PresentSceneGesture(Event.Payload,Movie->GetSceneTimeSeconds(),true);}}}
    }
}

void UStorySessionComponent::PlaySceneEventAudio(const FStorySceneEvent& Event,bool bPlayback)
{
    USoundBase* Sound=nullptr;float Volume=1;
    if(Event.Type==EStoryEventType::MoneyTaken || Event.Type==EStoryEventType::MoneyTransferred || Event.Type==EStoryEventType::MoneyConfiscated){Sound=MoneyVisual.PickupSound;Volume=MoneyVisual.SoundVolume;}
    if(auto* Prop=PhysicalProps.FindRef(Event.ObjectId).Get())
    {
        if(Event.Type==EStoryEventType::PropGrabbed){Sound=Prop->Visual.PickupSound;Volume=Prop->Visual.SoundVolume;}
        if(Event.Type==EStoryEventType::PropImpact)
        {Sound=Prop->CosmeticId==TEXT("Prop.Donut")?Prop->Visual.UseSound.Get():Movie->AudioProfile && !Movie->AudioProfile->FaintImpacts.IsEmpty()?Movie->AudioProfile->FaintImpacts[0].Get():nullptr;Volume=.45f;}
    }
    if(!Sound){return;}
    AActor* Actor=bPlayback?static_cast<AActor*>(Movie->FindReplayForRole(Event.InstigatorId)):static_cast<AActor*>(FindActor(Event.InstigatorId));
    if(auto* Acting=Actor?Actor->FindComponentByClass<UActingComponent>():nullptr)
    {Acting->PlaySceneSound(Sound,Event.TimeSeconds,Movie->GetSceneTimeSeconds(),Movie->SceneClock->IsClockRunning(),Volume);}
}
void UStorySessionComponent::UpdateScenePlayback()
{
    if(!bScenePlayback || Movie->PresentationPhase!=EMoviePresentationPhase::Playback){return;}
    const double Time=Movie->GetSceneTimeSeconds();
    while(SceneEvents.IsValidIndex(SceneEventCursor) && SceneEvents[SceneEventCursor].TimeSeconds<=Time){ApplyPlaybackEvent(SceneEvents[SceneEventCursor++]);}
    UpdatePropPlayback();
    for(ACharacter* NPC:NPCActors){if(auto* Acting=NPC->FindComponentByClass<UActingComponent>()){Acting->UpdateScenePresentation(Time,Movie->SceneClock->IsClockRunning());}}
}

FStoryScoreBreakdown UStorySessionComponent::GetScoreBreakdown() const
{
    FStoryScoreBreakdown Score;const EStoryOutcome Ending=SceneState.ResolveOutcome(Cast.IsValidIndex(0)?Cast[0].ActorId:NAME_None,Cast.IsValidIndex(1)?Cast[1].ActorId:NAME_None,Cast.IsValidIndex(2)?Cast[2].ActorId:NAME_None);
    Score.Story=SceneState.bMoneyWasTaken?10:0;Score.Story+=InteractionPairs.Num()>0?10:0;
    Score.Story+=SceneState.bMoneyWasTaken && Ending!=EStoryOutcome::Unresolved && Ending!=EStoryOutcome::FailedRobbery?20:0;
    Score.Interactions=FMath::Min(30,InteractionPairs.Num()*10);Score.Creativity=FMath::Min(20,CreativeIds.Num()*2);
    Score.PropsAndNPCs=FMath::Min(6,NPCInteractionIds.Num()*2)+FMath::Min(4,ThrownObjects.Num()*2);return Score;
}
void UStorySessionComponent::CalculateStoryScore()
{
    Outcome=SceneState.ResolveOutcome(Cast[0].ActorId,Cast[1].ActorId,Cast[2].ActorId);FinalSceneState=SceneState;
    Movie->ScoreSummary=FMovieScoreSummary();
    for(const auto& Film:Movie->Performances){auto& Role=Movie->ScoreSummary.RoleScores.AddDefaulted_GetRef();Role.RoleId=Film.RoleId;Role.Score=Film.ObjectiveResult.Score;}
    Movie->ScoreSummary.FinalMovieScore=GetScoreBreakdown().Total();
    Movie->ScoreSummary.bUseStoryScoring=true;Movie->ScoreSummary.StoryScore=GetScoreBreakdown();
    Movie->MovieResults=UMovieResultsLibrary::EvaluateMovie(Movie->Performances,Movie->SessionRoles,Movie->ScoreSummary,Movie->SessionResultsSettings);
    Movie->MovieResults.StoryEnding=StoryWorldPresentation::Ending(Outcome);Movie->MovieResults.StoryScore=GetScoreBreakdown();
    const auto Summary=Movie->ScoreSummary;Movie->OnMovieScoreCalculated.Broadcast(Summary);
}
FText UStorySessionComponent::GetLatestConsequence() const
{
    if(SceneEvents.IsEmpty()){return FText::GetEmpty();}
    for(int32 Index=SceneEvents.Num()-1;Index>=0;--Index)
    {
        switch(SceneEvents[Index].Type)
        {
        case EStoryEventType::MoneyConfiscated:return NSLOCTEXT("FinalTake","ConsequenceConfiscated","The police have the money. The robber will return without it.");
        case EStoryEventType::MoneyTransferred:return NSLOCTEXT("FinalTake","ConsequenceTransferred","The money changed hands.");
        case EStoryEventType::MoneyDropped:return NSLOCTEXT("FinalTake","ConsequenceDropped","The money is on the floor. Anyone can pick it up.");
        case EStoryEventType::MoneySecured:return NSLOCTEXT("FinalTake","ConsequenceSecured","The money is safe... while the guard is paying attention.");
        case EStoryEventType::GuardDistracted:return NSLOCTEXT("FinalTake","ConsequenceDistracted","The guard is distracted. The next performance will change.");
        case EStoryEventType::Escape:return NSLOCTEXT("FinalTake","ConsequenceEscape","Someone left with the money. There are still consequences.");
        default:break;
        }
    }
    return FText::GetEmpty();
}
