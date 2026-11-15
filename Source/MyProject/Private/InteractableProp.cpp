#include "InteractableProp.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AInteractableProp::AInteractableProp()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;
    PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
    SetRootComponent(PickupMesh);
    PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PickupMesh->SetGenerateOverlapEvents(false);
    PickupMesh->SetCanEverAffectNavigation(false);
    ConfigureDefault(TEXT("Prop.MoneyBag"), TEXT("Money bag"));
    SetActorEnableCollision(false);
}

void AInteractableProp::ConfigureDefault(FName ItemId, const TCHAR* Label)
{
    PropId = ItemId;
    DisplayName = WardrobeDefaults::LocalizedName(ItemId,FText::FromString(Label));
    const TArray<FCosmeticDefinition> Catalog = WardrobeDefaults::MakeCatalog();
    if (const auto* Item = Catalog.FindByPredicate([ItemId](const FCosmeticDefinition& Entry) { return Entry.Id == ItemId; }))
    {
        EquippedVisual = *Item;
        PickupMesh->SetStaticMesh(Item->StaticMesh);
        PickupMesh->SetRelativeScale3D(Item->ActorRelativeTransform.GetScale3D());
        for (int32 Index = 0; Index < Item->Materials.Num(); ++Index) { PickupMesh->SetMaterial(Index, Item->Materials[Index]); }
    }
}

void AInteractableProp::BeginPlay()
{
    Super::BeginPlay();
    if (PickupId.IsNone()) { PickupId = FName(*FGuid::NewGuid().ToString(EGuidFormats::Digits)); }
    CaptureInitialState();
    if (EquippedVisual.bUseTint)
    {
        for (int32 Index=0; Index<PickupMesh->GetNumMaterials(); ++Index)
        { if (auto* Material = PickupMesh->CreateDynamicMaterialInstance(Index)) { Material->SetVectorParameterValue(EquippedVisual.TintParameter, EquippedVisual.Tint); } }
    }
}
void AInteractableProp::CaptureInitialState()
{
    InitialTransform = GetActorTransform();
    bInitialHidden = IsHidden();
    bAvailable = !bInitialHidden;
}

void AInteractableProp::ResetForTake()
{
    SetActorTransform(InitialTransform, false, nullptr, ETeleportType::TeleportPhysics);
    SetActorHiddenInGame(bInitialHidden);
    bAvailable = !bInitialHidden;
}
void AInteractableProp::MarkCollected() { bAvailable = false; SetActorHiddenInGame(true); }
void AInteractableProp::ShowCollectedInPremiere() { SetActorHiddenInGame(true); }
AMoneyBagProp::AMoneyBagProp() { ConfigureDefault(TEXT("Prop.MoneyBag"), TEXT("Money bag")); }
ADonutProp::ADonutProp() { ConfigureDefault(TEXT("Prop.Donut"), TEXT("Donut")); }
ABananaProp::ABananaProp() { ConfigureDefault(TEXT("Prop.Banana"), TEXT("Banana")); }
