#include "ObjectiveManagerComponent.h"
#include "ActingComponent.h"
#include "InteractableProp.h"
#include "MovieManager.h"
#include "ObjectiveZone.h"
#include "PropInteractionComponent.h"
#include "ReplayActor.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UObjectiveManagerComponent::UObjectiveManagerComponent() { PrimaryComponentTick.bCanEverTick = false; }

bool UObjectiveManagerComponent::ValidateDefinitions(const TArray<FMovieObjectiveDefinition>& Definitions, FText& Error)
{
    TMap<FName, FName> Prerequisites;
    for (const auto& Definition : Definitions)
    {
        if (Definition.Id.IsNone() || Prerequisites.Contains(Definition.Id) || Definition.Points < 0 || Definition.Points > 100
            || static_cast<uint8>(Definition.Type) > static_cast<uint8>(EMovieObjectiveType::SceneAction)
            || !FMath::IsFinite(Definition.MaxInteractionDistance) || Definition.MaxInteractionDistance < 0)
        { Error = NSLOCTEXT("FinalTakeUI","ObjectiveManagerComponent.4d2d93ed21","Objectives need unique IDs, valid types, 0-100 points and finite nonnegative distances."); return false; }
        Prerequisites.Add(Definition.Id, Definition.PrerequisiteId);
    }
    for (const auto& Pair : Prerequisites)
    {
        TSet<FName> Visited;
        FName Id = Pair.Key;
        while (!Id.IsNone())
        {
            if (Visited.Contains(Id) || !Prerequisites.Contains(Id))
            { Error = NSLOCTEXT("FinalTakeUI","ObjectiveManagerComponent.8e0cd9e216","An objective prerequisite is missing or forms a cycle."); return false; }
            Visited.Add(Id); Id = Prerequisites[Id];
        }
    }
    Error = FText::GetEmpty(); return true;
}

void UObjectiveManagerComponent::UnbindActing()
{
    if (LiveActing.IsValid()) { LiveActing->OnActingEventTriggeredNative.Remove(ActingDelegate); }
    ActingDelegate.Reset(); LiveActing.Reset();
}

void UObjectiveManagerComponent::ResetObjectives()
{
    ++ObjectiveGeneration; bActive = false; UnbindActing(); LivePerformer.Reset();
    Objectives.Reset(); CurrentScore = 0; RoleId = NAME_None;
    LastCompletionFeedback = ConfigurationIssue = FText::GetEmpty(); LastCompletionWorldTime = -100.0;
}

bool UObjectiveManagerComponent::PreviewRole(AMovieManager* Manager, FName InRoleId, const TArray<FMovieObjectiveDefinition>& Definitions)
{
    ResetObjectives(); Movie = Manager; RoleId = InRoleId;
    if (!ValidateDefinitions(Definitions, ConfigurationIssue)) { return false; }
    for (const auto& Definition : Definitions) { Objectives.AddDefaulted_GetRef().Definition = Definition; }
    return true;
}

void UObjectiveManagerComponent::BeginTake(APawn* Performer, UActingComponent* Acting)
{
    UnbindActing(); LivePerformer = Performer; LiveActing = Acting; bActive = true;
    if (Acting) { ActingDelegate = Acting->OnActingEventTriggeredNative.AddUObject(this, &UObjectiveManagerComponent::HandleActingEvent); }
}

bool UObjectiveManagerComponent::CanScore() const
{
    const AMovieManager* Manager = Movie.Get();
    const APawn* Pawn = LivePerformer.Get();
    return bActive && Manager && Manager->HasActiveSession() && Manager->State == EMovieState::Recording
        && Pawn && Manager->LivePerformer == Pawn && Cast<APlayerController>(Pawn->GetController())
        && Pawn->GetController()->GetPawn() == Pawn && Manager->GetCurrentRoleId() == RoleId;
}

bool UObjectiveManagerComponent::IsObjectiveCompleted(FName Id) const
{
    const auto* Status = Objectives.FindByPredicate([Id](const FMovieObjectiveStatus& Item) { return Item.Definition.Id == Id; });
    return Status && Status->bCompleted;
}

bool UObjectiveManagerComponent::CompleteSceneObjective(FName Id,double SceneTime)
{
    const AMovieManager* Manager=Movie.Get();
    const auto* Status=Objectives.FindByPredicate([Id](const auto& Item){return Item.Definition.Id==Id;});
    if(!Manager || !Manager->IsChronologicalMovie() || !CanScore() || !Status || Status->Definition.Type!=EMovieObjectiveType::SceneAction){return false;}
    const FText Label=Status->Definition.DisplayName; const uint32 Before=ObjectiveGeneration;
    CompleteObjective(Id,SceneTime);
    if(Before==ObjectiveGeneration){LastCompletionFeedback=FText::Format(NSLOCTEXT("FinalTake","ObjectiveComplete","Done! {0}"),Label);}
    return Before==ObjectiveGeneration && IsObjectiveCompleted(Id);
}

bool UObjectiveManagerComponent::IsPrerequisiteSatisfied(FName Id) const
{
    const auto* Status = Objectives.FindByPredicate([Id](const FMovieObjectiveStatus& Item) { return Item.Definition.Id == Id; });
    return Status && (Status->Definition.PrerequisiteId.IsNone() || IsObjectiveCompleted(Status->Definition.PrerequisiteId));
}

bool UObjectiveManagerComponent::InZone(const FMovieObjectiveDefinition& Definition, FVector Location) const
{
    const auto Contains = [Location](const AActor* Actor)
    {
        if (const auto* Zone = Cast<AObjectiveZone>(Actor)) { return Zone->ContainsPoint(Location); }
        if (const UBoxComponent* Box = Actor ? Actor->FindComponentByClass<UBoxComponent>() : nullptr)
        {
            const FVector Local = Box->GetComponentTransform().InverseTransformPosition(Location);
            const FVector Extent = Box->GetUnscaledBoxExtent();
            return FMath::Abs(Local.X)<=Extent.X && FMath::Abs(Local.Y)<=Extent.Y && FMath::Abs(Local.Z)<=Extent.Z;
        }
        return false;
    };
    if (IsValid(Definition.TargetActor)) { return Contains(Definition.TargetActor); }
    if (Definition.TargetTag.IsNone()) { return false; }
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        const AObjectiveZone* Zone = Cast<AObjectiveZone>(*It);
        if ((It->ActorHasTag(Definition.TargetTag) || (Zone && Zone->ZoneTag == Definition.TargetTag)) && Contains(*It)) { return true; }
    }
    return false;
}

bool UObjectiveManagerComponent::NearTarget(const FMovieObjectiveDefinition& Definition, FVector Location, double SceneTime) const
{
    const double Distance = Definition.MaxInteractionDistance > 0.0 ? Definition.MaxInteractionDistance : 200.0;
    const auto Near = [&](AActor* Target)
    {
        if (!IsValid(Target) || Target == LivePerformer.Get() || Target->IsHidden()) { return false; }
        FVector TargetLocation = Target->GetActorLocation();
        if (AReplayActor* Replay = Cast<AReplayActor>(Target))
        {
            FTransform Pose;
            if (!Replay->EvaluateScenePose(SceneTime, Pose)) { return false; }
            TargetLocation = Pose.GetLocation();
        }
        return FVector::DistSquared(Location, TargetLocation) <= Distance*Distance;
    };
    if (IsValid(Definition.TargetActor)) { return Near(Definition.TargetActor); }
    const AMovieManager* Manager = Movie.Get();
    if (!Definition.TargetRoleId.IsNone()) { return Manager && Near(Manager->FindReplayForRole(Definition.TargetRoleId)); }
    if (Definition.bAnyRecordedRole && Manager)
    {
        for (AReplayActor* Replay : Manager->ActiveReplays) { if (Near(Replay)) { return true; } }
    }
    if (!Definition.TargetTag.IsNone())
    {
        for (TActorIterator<AActor> It(GetWorld()); It; ++It) { if (It->ActorHasTag(Definition.TargetTag) && Near(*It)) { return true; } }
    }
    return false;
}

void UObjectiveManagerComponent::CompleteObjective(FName Id, double SceneTime)
{
    if (!CanScore() || !IsPrerequisiteSatisfied(Id)) { return; }
    auto* Status = Objectives.FindByPredicate([Id](const FMovieObjectiveStatus& Item) { return Item.Definition.Id == Id; });
    if (!Status || Status->bCompleted) { return; }
    Status->bCompleted = true;
    Status->PointsAwarded = FMath::Clamp(Status->Definition.Points, 0, 100-CurrentScore);
    Status->CompletedAtSeconds = SceneTime;
    CurrentScore += Status->PointsAwarded;
    LastCompletionWorldTime = GetWorld()->GetTimeSeconds();
    LastCompletionFeedback = FText::Format(NSLOCTEXT("MovieHUD","PointsFeedback","+{0}  {1}"),FText::AsNumber(Status->PointsAwarded),Status->Definition.DisplayName.IsEmpty()?FText::FromName(Status->Definition.Id):Status->Definition.DisplayName);
    const uint32 Generation = ObjectiveGeneration;
    const FMovieObjectiveStatus Copy = *Status;
    OnObjectiveCompleted.Broadcast(Copy);
    if (Generation == ObjectiveGeneration) { OnScoreChanged.Broadcast(CurrentScore); }
}

void UObjectiveManagerComponent::EvaluateContinuous(double SceneTime, FVector LiveLocation)
{
    if (!CanScore()) { return; }
    const uint32 Generation = ObjectiveGeneration;
    const TArray<FMovieObjectiveStatus> Stable = Objectives;
    for (const auto& Status : Stable)
    {
        const auto& Definition = Status.Definition;
        if (!Status.bCompleted && IsPrerequisiteSatisfied(Definition.Id)
            && ((Definition.Type == EMovieObjectiveType::ReachZone && InZone(Definition, LiveLocation))
            || (Definition.Type == EMovieObjectiveType::ReachActor && NearTarget(Definition, LiveLocation, SceneTime))))
        { CompleteObjective(Definition.Id, SceneTime); }
        if (Generation != ObjectiveGeneration || !CanScore()) { return; }
    }
}

void UObjectiveManagerComponent::HandleActingEvent(const FActingEvent& Event, bool bFromReplay)
{
    if (bFromReplay || !CanScore()) { return; }
    const uint32 Generation = ObjectiveGeneration;
    const FVector Location = LivePerformer->GetActorLocation();
    EvaluateContinuous(Event.TimeSeconds, Location);
    if (Generation != ObjectiveGeneration || !CanScore()) { return; }
    const auto* Interaction = LivePerformer->FindComponentByClass<UPropInteractionComponent>();
    const TArray<FMovieObjectiveStatus> Stable = Objectives;
    for (const auto& Status : Stable)
    {
        const auto& Definition = Status.Definition;
        if (Status.bCompleted || !IsPrerequisiteSatisfied(Definition.Id)) { continue; }
        bool bMatches = false;
        switch (Definition.Type)
        {
        case EMovieObjectiveType::CollectProp:
            bMatches = Event.Type == EActingEventType::CollectProp
                && (Definition.RequiredPropId.IsNone() || Definition.RequiredPropId == Event.PropId);
            if (bMatches && Definition.MaxInteractionDistance > 0)
            { bMatches = FVector::DistSquared(Location, Event.InteractionLocation) <= FMath::Square(Definition.MaxInteractionDistance); }
            if (bMatches && (Definition.TargetActor || !Definition.TargetTag.IsNone()))
            {
                AInteractableProp* Target = Movie->FindPickupById(Event.TargetId);
                bMatches = Target && (!Definition.TargetActor || Target == Definition.TargetActor)
                    && (Definition.TargetTag.IsNone() || Target->ActorHasTag(Definition.TargetTag) || Target->PropId == Definition.TargetTag);
            }
            break;
        case EMovieObjectiveType::PerformGesture: bMatches = Event.Type == EActingEventType::Gesture; break;
        case EMovieObjectiveType::PlaySound: bMatches = Event.Type == EActingEventType::Sound; break;
        case EMovieObjectiveType::ActionNearActor:
            bMatches = Event.Type == EActingEventType::Gesture && NearTarget(Definition, Location, Event.TimeSeconds); break;
        case EMovieObjectiveType::UseProp:
            bMatches = Event.Type == Definition.RequiredEventType && Interaction && Interaction->EquippedPropId == Definition.RequiredPropId;
            if (bMatches && (Definition.TargetActor || !Definition.TargetRoleId.IsNone() || Definition.bAnyRecordedRole || !Definition.TargetTag.IsNone()))
            { bMatches = NearTarget(Definition, Location, Event.TimeSeconds); }
            break;
        default: break;
        }
        if (!Definition.RequiredEventId.IsNone()) { bMatches &= Event.Identifier == Definition.RequiredEventId; }
        if (bMatches) { CompleteObjective(Definition.Id, Event.TimeSeconds); }
        if (Generation != ObjectiveGeneration || !CanScore()) { return; }
    }
}

FTakeObjectiveResult UObjectiveManagerComponent::GetResult() const
{
    FTakeObjectiveResult Result; Result.Score = CurrentScore;
    for (const auto& Status : Objectives) { if (Status.bCompleted) { Result.CompletedObjectiveIds.Add(Status.Definition.Id); } }
    return Result;
}
FTakeObjectiveResult UObjectiveManagerComponent::FinishTake(double SceneTime, FVector FinalLocation)
{
    const uint32 Generation = ObjectiveGeneration;
    EvaluateContinuous(SceneTime, FinalLocation);
    if (Generation != ObjectiveGeneration) { return FTakeObjectiveResult(); }
    const FTakeObjectiveResult Result = GetResult();
    bActive = false; UnbindActing(); return Result;
}
void UObjectiveManagerComponent::EndPlay(const EEndPlayReason::Type Reason) { ResetObjectives(); Super::EndPlay(Reason); }
