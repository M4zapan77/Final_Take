#include "PropInteractionComponent.h"
#include "ActingComponent.h"
#include "ActingRecorderComponent.h"
#include "InteractableProp.h"
#include "MovieManager.h"
#include "ReplayActor.h"
#include "WardrobeComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UPropInteractionComponent::UPropInteractionComponent() { PrimaryComponentTick.bCanEverTick = false; }

bool UPropInteractionComponent::CanInteract() const
{
    const APawn* Pawn = Cast<APawn>(GetOwner());
    const AMovieManager* Manager = Movie.Get();
    const UActingRecorderComponent* Recorder = Pawn ? Pawn->FindComponentByClass<UActingRecorderComponent>() : nullptr;
    return bActive && !bReplayOnly && Pawn && Cast<APlayerController>(Pawn->GetController())
        && Pawn->GetController()->GetPawn() == Pawn && Manager && Manager->HasActiveSession()
        && Manager->State == EMovieState::Recording && Manager->LivePerformer == Pawn && Recorder && Recorder->IsRecording();
}

void UPropInteractionComponent::BeginTake(AMovieManager* Manager)
{
    ResetState(); Movie = Manager; bActive = true; bReplayOnly = false;
    if (const UWardrobeComponent* Wardrobe = GetOwner()->FindComponentByClass<UWardrobeComponent>())
    {
        EquippedPropId = Wardrobe->GetSelection().PropId;
        const bool bLeft=EquippedPropId.IsNone();if(bLeft){EquippedPropId=Wardrobe->GetSelection().LeftHandPropId;}
        const FRecordedAppearance Appearance = Wardrobe->CaptureAppearance();
        for (const auto& Item : Appearance.Items) { if (Item.Category == (bLeft?ECosmeticCategory::LeftProp:ECosmeticCategory::Prop)) { EquippedVisual = Item;EquippedVisual.Category=ECosmeticCategory::Prop; } }
    }
}
void UPropInteractionComponent::EndTake() { bActive = false; }
void UPropInteractionComponent::ResetState() { bActive = false; EquippedPropId = NAME_None; EquippedVisual = FCosmeticDefinition(); LastInteraction = FActingEvent(); }
void UPropInteractionComponent::ConfigureReplay(AMovieManager* Manager, FName InitialPropId)
{
    ResetState(); Movie = Manager; bReplayOnly = true; EquippedPropId = InitialPropId;
    if (const auto* Wardrobe = GetOwner()->FindComponentByClass<UWardrobeComponent>())
    {
        const bool bLeft=EquippedPropId.IsNone();if(bLeft){EquippedPropId=Wardrobe->GetSelection().LeftHandPropId;}
        const FRecordedAppearance Appearance = Wardrobe->CaptureAppearance();
        for (const auto& Item : Appearance.Items) { if (Item.Category == (bLeft?ECosmeticCategory::LeftProp:ECosmeticCategory::Prop)) { EquippedVisual = Item;EquippedVisual.Category=ECosmeticCategory::Prop; } }
    }
}

AInteractableProp* UPropInteractionComponent::FindNearestProp() const
{
    if (!CanInteract() || !FMath::IsFinite(MaxInteractionDistance) || MaxInteractionDistance <= 0) { return nullptr; }
    AInteractableProp* Closest = nullptr;
    double Best = TNumericLimits<double>::Max();
    for (TActorIterator<AInteractableProp> It(GetWorld()); It; ++It)
    {
        const double Range = FMath::Min(MaxInteractionDistance, It->InteractionDistance);
        const double Distance = FVector::DistSquared(GetOwner()->GetActorLocation(), It->GetActorLocation());
        if (It->IsAvailable() && FMath::IsFinite(Range) && Range > 0 && Distance <= Range*Range && Distance < Best) { Closest = *It; Best = Distance; }
    }
    return Closest;
}

bool UPropInteractionComponent::Interact()
{
    if (AInteractableProp* Prop = FindNearestProp()) { return CollectProp(Prop); }
    return UseEquippedProp();
}

bool UPropInteractionComponent::CollectProp(AInteractableProp* Prop)
{
    if (!CanInteract() || !IsValid(Prop) || Prop->GetWorld() != GetWorld() || !Prop->IsAvailable()) { return false; }
    const double Range = FMath::Min(MaxInteractionDistance, Prop->InteractionDistance);
    if (!FMath::IsFinite(Range) || Range <= 0 || FVector::DistSquared(GetOwner()->GetActorLocation(), Prop->GetActorLocation()) > Range*Range) { return false; }
    UActingComponent* Acting = GetOwner()->FindComponentByClass<UActingComponent>();
    UWardrobeComponent* Wardrobe = GetOwner()->FindComponentByClass<UWardrobeComponent>();
    if (!Acting || !Wardrobe || Prop->PropId.IsNone()) { return false; }
    FActingEvent Event;
    Event.Type = EActingEventType::CollectProp; Event.Identifier = TEXT("Collect");
    Event.PropId = Prop->PropId; Event.TargetId = Prop->PickupId; Event.InteractionLocation = Prop->GetActorLocation();
    if (!Prop->bUsePerformerCatalogVisual || !Wardrobe->FindPropVisual(Prop->PropId, Event.PropVisual))
    {
        Event.PropVisual = UWardrobeComponent::FreezeItem(Prop->EquippedVisual, this);
        Event.PropVisual.Id = Prop->PropId; Event.PropVisual.Category = ECosmeticCategory::Prop;
    }
    const int32 Token = Movie->PreparationId;
    if (!Acting->TriggerPropEvent(Event)) { return false; }
    if (CanInteract() && Movie->PreparationId == Token && IsValid(Prop)) { Prop->MarkCollected(); }
    return true;
}

bool UPropInteractionComponent::UseEquippedProp(FName ActionId)
{
    if (!CanInteract() || EquippedPropId.IsNone()) { return false; }
    UActingComponent* Acting = GetOwner()->FindComponentByClass<UActingComponent>();
    if (!Acting) { return false; }
    FActingEvent Event;
    Event.Type = EActingEventType::UseProp;
    Event.PropId = EquippedPropId;
    Event.PropVisual = EquippedVisual;
    Event.Identifier = ActionId.IsNone() ? FName(*FString::Printf(TEXT("Use.%s"), *EquippedPropId.ToString().Replace(TEXT("Prop."), TEXT("")))) : ActionId;
    Event.InteractionLocation = GetOwner()->GetActorLocation();
    return Acting->TriggerPropEvent(Event);
}

void UPropInteractionComponent::ApplyEvent(const FActingEvent& Event, bool bFromReplay)
{
    if (Event.Type == EActingEventType::CollectProp)
    {
        if (UWardrobeComponent* Wardrobe = GetOwner()->FindComponentByClass<UWardrobeComponent>()) { Wardrobe->ApplyPropVisual(Event.PropVisual); }
        EquippedPropId = Event.PropId;
        EquippedVisual = Event.PropVisual;
        // Shared live pickups are never consumed by a replay. Premiere changes presentation only.
        if (bFromReplay && Movie.IsValid() && Movie->IsMoviePlayback())
        {
            for (TActorIterator<AInteractableProp> It(GetWorld()); It; ++It)
            {
                if (It->PickupId == Event.TargetId) { It->ShowCollectedInPremiere(); break; }
            }
        }
    }
    LastInteraction = Event;
    OnPropInteraction.Broadcast(Event, bFromReplay);
}

FText UPropInteractionComponent::GetInteractionPrompt() const
{
    if (!CanInteract()) { return FText::GetEmpty(); }
    if (const AInteractableProp* Prop = FindNearestProp()) { return FText::Format(NSLOCTEXT("Props","CollectPrompt","E - Collect {0}"),Prop->DisplayName); }
    return EquippedPropId.IsNone() ? NSLOCTEXT("FinalTakeUI","PropInteractionComponent.d4320b2f04","Approach a prop and press E")
        : FText::Format(NSLOCTEXT("Props","UsePrompt","E - Use {0}"),WardrobeDefaults::LocalizedName(EquippedPropId,FText::FromName(EquippedPropId)));
}
