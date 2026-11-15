#include "WardrobeComponent.h"
#include "ActingRecorderComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ReplayActor.h"

namespace
{
    constexpr ECosmeticCategory Categories[] = {ECosmeticCategory::Head,ECosmeticCategory::Body,ECosmeticCategory::Face,ECosmeticCategory::Prop,ECosmeticCategory::Eyes,ECosmeticCategory::Mouth,ECosmeticCategory::Eyebrows,ECosmeticCategory::LeftProp};
    bool FiniteColor(FLinearColor Color)
    {
        return FMath::IsFinite(Color.R) && FMath::IsFinite(Color.G) && FMath::IsFinite(Color.B) && FMath::IsFinite(Color.A);
    }
}

UWardrobeComponent::UWardrobeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    Catalog = WardrobeDefaults::MakeCatalog();
    DefaultSelection = WardrobeDefaults::Casual();
}

const FCosmeticDefinition* UWardrobeComponent::FindItem(ECosmeticCategory Category, FName Id) const
{
    if(Category==ECosmeticCategory::LeftProp){Category=ECosmeticCategory::Prop;}
    return Catalog.FindByPredicate([&](const FCosmeticDefinition& Item) { return Item.Category == Category && Item.Id == Id; });
}

bool UWardrobeComponent::ValidateCatalog(FText& OutError) const
{
    TSet<FName> Ids;
    bool bHasBody = false;
    for (const FCosmeticDefinition& Item : Catalog)
    {
        if (Item.Id.IsNone() || Ids.Contains(Item.Id) || static_cast<uint8>(Item.Category) > static_cast<uint8>(ECosmeticCategory::LeftProp)
            || !Item.RelativeTransform.IsValid() || !Item.ActorRelativeTransform.IsValid() || (Item.bUseLeftHandTransform&&!Item.LeftHandRelativeTransform.IsValid()) || !FiniteColor(Item.Tint)
            || (Item.StaticMesh && Item.SkeletalMesh))
        {
            OutError = NSLOCTEXT("FinalTakeUI","WardrobeComponent.aaa721312d","Cosmetic IDs must be unique/non-empty; use valid transforms and either a static or skeletal mesh per item.");
            return false;
        }
        Ids.Add(Item.Id);
        bHasBody |= Item.Category == ECosmeticCategory::Body;
    }
    if (!bHasBody) { OutError = NSLOCTEXT("FinalTakeUI","WardrobeComponent.c79d0a967e","The cosmetic catalog needs at least one Body entry for a valid default."); return false; }
    OutError = FText::GetEmpty();
    return true;
}

FCosmeticSelection UWardrobeComponent::NormalizeSelection(const FCosmeticSelection& Selection) const
{
    FCosmeticSelection Result = Selection;
    for (ECosmeticCategory Category : Categories)
    {
        if (!Result.GetId(Category).IsNone() && !FindItem(Category, Result.GetId(Category))) { Result.SetId(Category, NAME_None); }
    }
    if (Result.BodyId.IsNone())
    {
        if (FindItem(ECosmeticCategory::Body, DefaultSelection.BodyId)) { Result.BodyId = DefaultSelection.BodyId; }
        else
        {
            // Stable fallback even if the director reorders the catalog.
            for (const FCosmeticDefinition& Entry : Catalog)
            {
                if (Entry.Category == ECosmeticCategory::Body && (Result.BodyId.IsNone() || Entry.Id.LexicalLess(Result.BodyId))) { Result.BodyId = Entry.Id; }
            }
        }
    }
    if (!FiniteColor(Result.BodyColor)) { Result.BodyColor = FLinearColor::White; Result.bUseBodyColor = false; }
    if (!FiniteColor(Result.SkinColor)) {Result.SkinColor=FLinearColor::White;}
    return Result;
}

bool UWardrobeComponent::CanEditSelection() const
{
    if (!bSelectionEnabled || bReplayReadOnly || !IsValid(GetOwner()) || GetOwner()->IsA<AReplayActor>()) { return false; }
    const UActingRecorderComponent* Recorder = GetOwner()->FindComponentByClass<UActingRecorderComponent>();
    return !Recorder || !Recorder->IsRecording();
}

bool UWardrobeComponent::SelectItem(ECosmeticCategory Category, FName Id)
{
    if (!CanEditSelection() || static_cast<uint8>(Category) > static_cast<uint8>(ECosmeticCategory::LeftProp)
        || (!Id.IsNone() && !FindItem(Category, Id)) || (Category == ECosmeticCategory::Body && Id.IsNone())) { return false; }
    FCosmeticSelection Selection = CurrentSelection;
    Selection.SetId(Category, Id);
    return SetSelection(Selection);
}

bool UWardrobeComponent::SetBodyColor(bool bUseColor, FLinearColor Color)
{
    if (!FiniteColor(Color)) { return false; }
    FCosmeticSelection Selection = CurrentSelection;
    Selection.bUseBodyColor = bUseColor;
    Selection.BodyColor = Color;
    return SetSelection(Selection);
}

bool UWardrobeComponent::SetSkinColor(FLinearColor Color)
{
    if(!FiniteColor(Color)){return false;} FCosmeticSelection Selection=CurrentSelection;Selection.SkinColor=Color;return SetSelection(Selection);
}
TArray<FCosmeticDefinition> UWardrobeComponent::GetCategoryOptions(ECosmeticCategory Category) const
{
    TArray<FCosmeticDefinition> Result;
    for(const auto& Item:Catalog){if(Item.Category==(Category==ECosmeticCategory::LeftProp?ECosmeticCategory::Prop:Category)){auto Copy=Item;Copy.Category=Category;Result.Add(Copy);}}
    return Result;
}
bool UWardrobeComponent::ResetCustomization()
{
    FCosmeticSelection White;
    White.BodyId=FindItem(ECosmeticCategory::Body,TEXT("Body.None"))?FName(TEXT("Body.None")):DefaultSelection.BodyId;
    return SetSelection(White);
}
bool UWardrobeComponent::RandomizeCustomization()
{
    if(!CanEditSelection()){return false;}
    FCosmeticSelection Selection;
    for(ECosmeticCategory Category:Categories)
    {
        const auto Options=GetCategoryOptions(Category);const int32 Choice=FMath::RandRange(0,Options.Num());
        if(Choice<Options.Num()){Selection.SetId(Category,Options[Choice].Id);}
    }
    const FLinearColor Colors[]={FLinearColor::White,FLinearColor(.32,.65,1),FLinearColor(.45,.22,.85),FLinearColor(1,.35,.6),FLinearColor(1,.78,.18),FLinearColor(.25,.8,.4)};
    Selection.SkinColor=Colors[FMath::RandHelper(UE_ARRAY_COUNT(Colors))];return SetSelection(Selection);
}

bool UWardrobeComponent::BeginSelection(const FCosmeticSelection& DefaultOutfit)
{
    bReplayReadOnly = false;
    bSelectionEnabled = true;
    return SetSelection(DefaultOutfit);
}

bool UWardrobeComponent::SetSelection(const FCosmeticSelection& Selection)
{
    FText Error;
    if (!CanEditSelection() || !ValidateCatalog(Error)) { return false; }
    CacheBodyMaterials();
    RemoveVisuals();
    CurrentSelection = NormalizeSelection(Selection);
    // The face-slot alias allows a beanie and mask together. Keep the legacy
    // head ID usable by existing role presets, without drawing two masks.
    if (CurrentSelection.HeadId==TEXT("Head.RobberMask") && CurrentSelection.FaceId==TEXT("Face.RobberMask")) { CurrentSelection.HeadId=NAME_None; }
    AppliedItems.Reset();
    for (ECosmeticCategory Category : Categories)
    {
        if (const FCosmeticDefinition* Entry = FindItem(Category, CurrentSelection.GetId(Category)))
        {
            FCosmeticDefinition Copy=*Entry;Copy.Category=Category;
            if(Category==ECosmeticCategory::LeftProp){Copy.SocketName=TEXT("LeftHand");if(Copy.bUseLeftHandTransform){Copy.RelativeTransform=Copy.LeftHandRelativeTransform;}}
            AppliedItems.Add(Copy);
        }
    }
    bHasAppearance = true;
    BuildVisuals(AppliedItems);
    BuildFacialVisual();
    const FCosmeticSelection StableSelection = CurrentSelection;
    OnSelectionChanged.Broadcast(StableSelection);
    return true;
}

USkeletalMeshComponent* UWardrobeComponent::ResolveSkeletalMesh() const
{
    if (IsValid(TargetMesh) && TargetMesh->GetOwner() == GetOwner()) { return TargetMesh; }
    if (const ACharacter* Character = Cast<ACharacter>(GetOwner())) { return Character->GetMesh(); }
    return GetOwner() ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

bool UWardrobeComponent::FindPropVisual(FName PropId, FCosmeticDefinition& OutDefinition) const
{
    if (const FCosmeticDefinition* Definition = FindItem(ECosmeticCategory::Prop, PropId))
    {
        OutDefinition = FreezeItem(*Definition, const_cast<UWardrobeComponent*>(this));
        return true;
    }
    return false;
}

FCosmeticDefinition UWardrobeComponent::FreezeItem(const FCosmeticDefinition& Definition, UObject* Outer)
{
    FCosmeticDefinition Copy = Definition;
    for (TObjectPtr<UMaterialInterface>& Material : Copy.Materials) { Material = CopyMaterialReference(Material, Outer); }
    return Copy;
}

bool UWardrobeComponent::ApplyPropVisual(const FCosmeticDefinition& Definition)
{
    if (!IsValid(GetOwner()) || Definition.Id.IsNone() || Definition.Category != ECosmeticCategory::Prop
        || !Definition.RelativeTransform.IsValid() || !Definition.ActorRelativeTransform.IsValid()) { return false; }
    CacheBodyMaterials();
    RemoveVisuals();
    CurrentSelection.PropId = Definition.Id;
    AppliedItems.RemoveAll([](const FCosmeticDefinition& Item) { return Item.Category == ECosmeticCategory::Prop; });
    AppliedItems.Add(Definition);
    bHasAppearance = true;
    BuildVisuals(AppliedItems);
    BuildFacialVisual();
    return true;
}

UMeshComponent* UWardrobeComponent::ResolveBodyMesh() const
{
    if (USkeletalMeshComponent* Mesh = ResolveSkeletalMesh(); Mesh && Mesh->GetSkeletalMeshAsset()) { return Mesh; }
    if (IsValid(FallbackBodyMesh)) { return FallbackBodyMesh; }
    TInlineComponentArray<UMeshComponent*> Meshes;
    if (GetOwner()) { GetOwner()->GetComponents(Meshes); }
    for (UMeshComponent* Mesh : Meshes) { if (!CosmeticMeshes.Contains(Mesh)) { return Mesh; } }
    return nullptr;
}

void UWardrobeComponent::CacheBodyMaterials()
{
    if (CachedBody.IsValid()) { return; }
    CachedBody = ResolveBodyMesh();
    OriginalBodyMaterials.Reset();
    if (UMeshComponent* Mesh = CachedBody.Get())
    {
        for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index) { OriginalBodyMaterials.Add(Mesh->GetMaterial(Index)); }
    }
}

void UWardrobeComponent::RemoveVisuals()
{
    for (UMeshComponent* Mesh : CosmeticMeshes)
    {
        if (IsValid(Mesh)) { GetOwner()->RemoveInstanceComponent(Mesh); Mesh->DestroyComponent(); }
    }
    CosmeticMeshes.Reset();
    if (UMeshComponent* Body = CachedBody.Get())
    {
        Body->EmptyOverrideMaterials();
        for (int32 Index = 0; Index < OriginalBodyMaterials.Num(); ++Index) { Body->SetMaterial(Index, OriginalBodyMaterials[Index]); }
    }
    OwnedMaterials.Reset();
}

void UWardrobeComponent::ApplyMaterials(UMeshComponent* Mesh, const FCosmeticDefinition& Definition)
{
    if (!Mesh) { return; }
    for (int32 Index = 0; Index < Definition.Materials.Num() && Index < Mesh->GetNumMaterials(); ++Index)
    {
        if (Definition.Materials[Index]) { Mesh->SetMaterial(Index, Definition.Materials[Index]); }
    }
    const bool bBodyColor = Definition.Category == ECosmeticCategory::Body && CurrentSelection.bUseBodyColor;
    if ((Definition.bUseTint || bBodyColor || Definition.Category==ECosmeticCategory::Body) && !Definition.TintParameter.IsNone())
    {
        for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
        {
            if (UMaterialInterface* Source = Mesh->GetMaterial(Index))
            {
                UMaterialInstanceDynamic* Instance = Cast<UMaterialInstanceDynamic>(Source)
                    ? Cast<UMaterialInstanceDynamic>(CopyMaterialReference(Source,this)) : UMaterialInstanceDynamic::Create(Source, this);
                Instance->SetVectorParameterValue(Definition.TintParameter, bBodyColor ? CurrentSelection.BodyColor : Definition.Tint);
                if(Definition.Category==ECosmeticCategory::Body)
                {
                    Instance->SetVectorParameterValue(TEXT("SkinColor"),CurrentSelection.SkinColor);
                    Instance->SetScalarParameterValue(TEXT("OutfitStyle"),Definition.ClothingStyle);
                    Instance->SetScalarParameterValue(TEXT("UseClothingMask"),Definition.ClothingMask?1.f:0.f);
                    if(Definition.ClothingMask){Instance->SetTextureParameterValue(TEXT("ClothingMask"),Definition.ClothingMask);}
                }
                Mesh->SetMaterial(Index, Instance);
                OwnedMaterials.Add(Instance);
            }
        }
    }
}

void UWardrobeComponent::BuildVisuals(const TArray<FCosmeticDefinition>& Items)
{
    USkeletalMeshComponent* Body = ResolveSkeletalMesh();
    // Copy so a HUD listener cannot invalidate the array by starting another movie.
    const TArray<FCosmeticDefinition> StableItems = Items;
    for (const FCosmeticDefinition& Entry : StableItems)
    {
        UMeshComponent* Part = nullptr;
        const bool bLeaderPose = Entry.SkeletalMesh && Entry.bUseLeaderPose;
        if (Entry.SkeletalMesh)
        {
            if (bLeaderPose && (!Body || !Body->GetSkeletalMeshAsset() || Entry.SkeletalMesh->GetSkeleton() != Body->GetSkeletalMeshAsset()->GetSkeleton()))
            {
                OnPresentationIssue.Broadcast(Entry.Id, NSLOCTEXT("FinalTakeUI","WardrobeComponent.ce56b2f6ed","A modular part must use the same skeleton as the existing character."));
                continue;
            }
            USkeletalMeshComponent* Garment = NewObject<USkeletalMeshComponent>(GetOwner());
            Garment->SetSkeletalMesh(Entry.SkeletalMesh);
            if (bLeaderPose) { Garment->SetLeaderPoseComponent(Body, true, false); }
            Part = Garment;
        }
        else if (Entry.StaticMesh)
        {
            UStaticMeshComponent* Attachment = NewObject<UStaticMeshComponent>(GetOwner());
            Attachment->SetStaticMesh(Entry.StaticMesh);
            Part = Attachment;
        }
        if (!Part)
        {
            if (Entry.Category == ECosmeticCategory::Body) { ApplyMaterials(CachedBody.Get(), Entry); }
            continue;
        }
        GetOwner()->AddInstanceComponent(Part);
        Part->ComponentTags.Add(Entry.Id);
        if(Entry.Category==ECosmeticCategory::Prop){Part->ComponentTags.Add(TEXT("Wardrobe.RightProp"));}
        if(Entry.Category==ECosmeticCategory::LeftProp){Part->ComponentTags.Add(TEXT("Wardrobe.LeftProp"));}
        CosmeticMeshes.Add(Part);
        FName Socket=Entry.SocketName;
        if(Body && Body->GetSkeletalMeshAsset() && !Body->DoesSocketExist(Socket))
        {
            if(Socket==TEXT("hand_r")){Socket=TEXT("RightHand");}
            else if(Socket==TEXT("hand_l")){Socket=TEXT("LeftHand");}
            else if(Socket==TEXT("spine_03")){Socket=TEXT("Spine1");}
            else if(Socket==TEXT("pelvis")){Socket=TEXT("Hips");}
        }
        const bool bSocket = Body && Body->GetSkeletalMeshAsset() && !Socket.IsNone() && Body->DoesSocketExist(Socket);
        USceneComponent* Parent = bLeaderPose || bSocket ? static_cast<USceneComponent*>(Body) : GetOwner()->GetRootComponent();
        Part->SetupAttachment(Parent, bLeaderPose ? NAME_None : bSocket ? Socket : NAME_None);
        FTransform AttachmentTransform = bLeaderPose || bSocket ? Entry.RelativeTransform : Entry.ActorRelativeTransform;
        if (bSocket && !bLeaderPose && Entry.bFitInActorSpace)
        {
            AttachmentTransform = (Entry.ActorRelativeTransform * GetOwner()->GetActorTransform()).GetRelativeTransform(Body->GetSocketTransform(Socket));
            // Freeze the fit, so replays use the live actor's resolved offset, even from a different idle pose.
            if (FCosmeticDefinition* Applied = AppliedItems.FindByPredicate([&](const FCosmeticDefinition& Item) { return Item.Id == Entry.Id; }))
            {
                Applied->RelativeTransform = AttachmentTransform;
                Applied->bFitInActorSpace = false;
                Applied->SocketName=Socket;
            }
        }
        else if(bSocket && !bLeaderPose && !Entry.bSocketScaleResolved && (Entry.Category==ECosmeticCategory::Prop || Entry.Category==ECosmeticCategory::LeftProp)
            && !Body->GetSocketTransform(Socket).GetScale3D().Equals(GetOwner()->GetActorScale3D(),.01))
        {
            // Imported Base3 bones carry a scale of 100. Keep existing prop dimensions in centimeters.
            const FTransform ParentTransform=Body->GetSocketTransform(Socket);
            const FTransform Desired(ParentTransform.GetRotation()*Entry.RelativeTransform.GetRotation(),ParentTransform.GetLocation()+ParentTransform.TransformVectorNoScale(Entry.RelativeTransform.GetLocation()),Entry.RelativeTransform.GetScale3D()*GetOwner()->GetActorScale3D());
            AttachmentTransform=Desired.GetRelativeTransform(ParentTransform);
            if(FCosmeticDefinition* Applied=AppliedItems.FindByPredicate([&](const auto& Item){return Item.Category==Entry.Category&&Item.Id==Entry.Id;}))
            {Applied->SocketName=Socket;Applied->RelativeTransform=AttachmentTransform;Applied->bSocketScaleResolved=true;}
        }
        Part->SetRelativeTransform(AttachmentTransform);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetCollisionResponseToAllChannels(ECR_Ignore);
        Part->SetGenerateOverlapEvents(false);
        Part->SetCanEverAffectNavigation(false);
        Part->RegisterComponent();
        Part->SetSimulatePhysics(false);
        ApplyMaterials(Part, Entry);
        if((Entry.Category==ECosmeticCategory::Prop && bSuppressRightProp) || (Entry.Category==ECosmeticCategory::LeftProp && bSuppressLeftProp)){Part->SetVisibility(false,true);}
    }
}

void UWardrobeComponent::SetHandPropsSuppressed(bool bRight,bool bLeft)
{
    bSuppressRightProp=bRight;bSuppressLeftProp=bLeft;
    for(UMeshComponent* Part:CosmeticMeshes)
    {
        if(!Part){continue;}
        if(Part->ComponentHasTag(TEXT("Wardrobe.RightProp"))){Part->SetVisibility(!bRight,true);}
        if(Part->ComponentHasTag(TEXT("Wardrobe.LeftProp"))){Part->SetVisibility(!bLeft,true);}
    }
}

void UWardrobeComponent::BuildFacialVisual()
{
    USkeletalMeshComponent* Body=ResolveSkeletalMesh();
    if(!Body||!FacialSettings.SurfaceMesh||!FacialSettings.Material||!FacialSettings.NoneTexture||!FacialSettings.IsValid()||!Body->DoesSocketExist(FacialSettings.HeadBone)){return;}
    auto* Surface=NewObject<UStaticMeshComponent>(GetOwner());GetOwner()->AddInstanceComponent(Surface);CosmeticMeshes.Add(Surface);Surface->ComponentTags.Add(TEXT("Customization.Face"));
    Surface->SetStaticMesh(FacialSettings.SurfaceMesh);Surface->SetupAttachment(Body,FacialSettings.HeadBone);Surface->SetRelativeTransform(FacialSettings.RelativeTransform);
    Surface->SetCollisionEnabled(ECollisionEnabled::NoCollision);Surface->SetGenerateOverlapEvents(false);Surface->SetCanEverAffectNavigation(false);Surface->CastShadow=false;Surface->RegisterComponent();
    auto* Material=Cast<UMaterialInstanceDynamic>(FacialSettings.Material)?Cast<UMaterialInstanceDynamic>(CopyMaterialReference(FacialSettings.Material,this)):UMaterialInstanceDynamic::Create(FacialSettings.Material,this);OwnedMaterials.Add(Material);Surface->SetMaterial(0,Material);
    const auto Layer=[&](ECosmeticCategory Category,const TCHAR* Prefix,const FFacialLayerSettings& Settings)
    {
        UTexture2D* Texture=FacialSettings.NoneTexture;
        for(const auto& Item:AppliedItems){if(Item.Category==Category&&Item.FaceTexture){Texture=Item.FaceTexture;break;}}
        Material->SetTextureParameterValue(FName(*(FString(Prefix)+TEXT("Texture"))),Texture);
        Material->SetVectorParameterValue(FName(*(FString(Prefix)+TEXT("Layout"))),FLinearColor(Settings.Position.X,Settings.Position.Y,Settings.Scale.X,Settings.Scale.Y));
        Material->SetScalarParameterValue(FName(*(FString(Prefix)+TEXT("Visible"))),Settings.bVisible?1:0);
    };
    Layer(ECosmeticCategory::Eyes,TEXT("Eyes"),FacialSettings.Eyes);Layer(ECosmeticCategory::Mouth,TEXT("Mouth"),FacialSettings.Mouth);Layer(ECosmeticCategory::Eyebrows,TEXT("Eyebrows"),FacialSettings.Eyebrows);
}

UMaterialInterface* UWardrobeComponent::CopyMaterialReference(UMaterialInterface* Material, UObject* Outer)
{
    UMaterialInstanceDynamic* Dynamic = Cast<UMaterialInstanceDynamic>(Material);
    if (!Dynamic) { return Material; }
    TArray<UMaterialInstanceDynamic*> Chain;
    UMaterialInterface* Parent = Dynamic;
    while (UMaterialInstanceDynamic* Node = Cast<UMaterialInstanceDynamic>(Parent)) { Chain.Add(Node); Parent = Node->Parent; }
    UMaterialInstanceDynamic* Copy = UMaterialInstanceDynamic::Create(Parent, Outer);
    for (int32 Index = Chain.Num() - 1; Index >= 0; --Index) { Copy->CopyParameterOverrides(Chain[Index]); }
    return Copy;
}

FRecordedAppearance UWardrobeComponent::CaptureAppearance() const
{
    FRecordedAppearance Snapshot;
    Snapshot.bHasAppearance = bHasAppearance;
    if (!bHasAppearance) { return Snapshot; }
    Snapshot.Selection = CurrentSelection;
    Snapshot.FaceSettings=FacialSettings;
    Snapshot.FaceSettings.Material=CopyMaterialReference(FacialSettings.Material,const_cast<UWardrobeComponent*>(this));
    Snapshot.Items = AppliedItems;
    for (FCosmeticDefinition& Item : Snapshot.Items)
    {
        for (TObjectPtr<UMaterialInterface>& Material : Item.Materials) { Material = CopyMaterialReference(Material, const_cast<UWardrobeComponent*>(this)); }
    }
    for (UMaterialInterface* Material : OriginalBodyMaterials) { Snapshot.BaseBodyMaterials.Add(CopyMaterialReference(Material, const_cast<UWardrobeComponent*>(this))); }
    return Snapshot;
}

bool UWardrobeComponent::ApplySnapshot(const FRecordedAppearance& Snapshot, USkeletalMeshComponent* Mesh, UMeshComponent* Fallback, bool bForReplay)
{
    if (!Snapshot.IsValid() || !IsValid(GetOwner())) { return false; }
    ClearAppearance();
    TargetMesh = Mesh;
    FallbackBodyMesh = Fallback;
    bReplayReadOnly = bForReplay;
    bSelectionEnabled = !bForReplay;
    if (!Snapshot.bHasAppearance) { return true; }
    CurrentSelection = Snapshot.Selection;
    AppliedItems = Snapshot.Items;
    FacialSettings=Snapshot.FaceSettings;
    CacheBodyMaterials();
    if (UMeshComponent* Body = CachedBody.Get())
    {
        if (!Snapshot.BaseBodyMaterials.IsEmpty())
        {
            OriginalBodyMaterials = Snapshot.BaseBodyMaterials;
            for (int32 Index = 0; Index < OriginalBodyMaterials.Num(); ++Index) { Body->SetMaterial(Index, OriginalBodyMaterials[Index]); }
        }
    }
    bHasAppearance = true;
    BuildVisuals(AppliedItems);
    BuildFacialVisual();
    return true;
}

void UWardrobeComponent::ClearAppearance()
{
    RemoveVisuals();
    CachedBody.Reset();
    OriginalBodyMaterials.Reset();
    AppliedItems.Reset();
    CurrentSelection = FCosmeticSelection();
    bHasAppearance = false;
}

void UWardrobeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearAppearance();
    Super::EndPlay(EndPlayReason);
}
