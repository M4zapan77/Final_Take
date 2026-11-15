#include "WardrobeTypes.h"
#include "ProductionArtCatalog.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"

FName FCosmeticSelection::GetId(ECosmeticCategory Category) const
{
    switch (Category)
    {
    case ECosmeticCategory::Head: return HeadId;
    case ECosmeticCategory::Body: return BodyId;
    case ECosmeticCategory::Face: return FaceId;
    case ECosmeticCategory::Prop: return PropId;
    case ECosmeticCategory::Eyes: return EyeId;
    case ECosmeticCategory::Mouth: return MouthId;
    case ECosmeticCategory::Eyebrows: return EyebrowId;
    case ECosmeticCategory::LeftProp: return LeftHandPropId;
    default: return NAME_None;
    }
}

void FCosmeticSelection::SetId(ECosmeticCategory Category, FName Id)
{
    switch (Category)
    {
    case ECosmeticCategory::Head: HeadId = Id; break;
    case ECosmeticCategory::Body: BodyId = Id; break;
    case ECosmeticCategory::Face: FaceId = Id; break;
    case ECosmeticCategory::Prop: PropId = Id; break;
    case ECosmeticCategory::Eyes: EyeId=Id; break;
    case ECosmeticCategory::Mouth: MouthId=Id; break;
    case ECosmeticCategory::Eyebrows: EyebrowId=Id; break;
    case ECosmeticCategory::LeftProp: LeftHandPropId=Id; break;
    default: break;
    }
}

bool FRecordedAppearance::IsValid() const
{
    if (!bHasAppearance) { return true; }
    if (Selection.BodyId.IsNone() || !FaceSettings.IsValid() || !FMath::IsFinite(Selection.SkinColor.R) || !FMath::IsFinite(Selection.SkinColor.G) || !FMath::IsFinite(Selection.SkinColor.B) || !FMath::IsFinite(Selection.SkinColor.A) || !FMath::IsFinite(Selection.BodyColor.R) || !FMath::IsFinite(Selection.BodyColor.G)
        || !FMath::IsFinite(Selection.BodyColor.B) || !FMath::IsFinite(Selection.BodyColor.A)) { return false; }
    TSet<ECosmeticCategory> Seen;
    for (const FCosmeticDefinition& Item : Items)
    {
        if (Item.Id.IsNone() || static_cast<uint8>(Item.Category) > static_cast<uint8>(ECosmeticCategory::LeftProp)
            || Seen.Contains(Item.Category) || Selection.GetId(Item.Category) != Item.Id
            || !Item.RelativeTransform.IsValid() || !Item.ActorRelativeTransform.IsValid() || (Item.bUseLeftHandTransform&&!Item.LeftHandRelativeTransform.IsValid())
            || !FMath::IsFinite(Item.Tint.R) || !FMath::IsFinite(Item.Tint.G) || !FMath::IsFinite(Item.Tint.B) || !FMath::IsFinite(Item.Tint.A)
            || (Item.StaticMesh && Item.SkeletalMesh)) { return false; }
        Seen.Add(Item.Category);
    }
    for (ECosmeticCategory Category : {ECosmeticCategory::Head, ECosmeticCategory::Body, ECosmeticCategory::Face, ECosmeticCategory::Prop,ECosmeticCategory::Eyes,ECosmeticCategory::Mouth,ECosmeticCategory::Eyebrows,ECosmeticCategory::LeftProp})
    {
        if (!Selection.GetId(Category).IsNone() && !Seen.Contains(Category)) { return false; }
    }
    return true;
}

FCosmeticSelection WardrobeDefaults::Casual()
{
    FCosmeticSelection Selection;
    Selection.BodyId = TEXT("Body.Casual");
    return Selection;
}

TArray<FCosmeticDefinition> WardrobeDefaults::MakeCatalog()
{
    // Only engine primitives: no project art is created or edited. Replace these in Blueprint defaults.
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    TArray<FCosmeticDefinition> Catalog;
    const auto Add = [&](const TCHAR* Id, ECosmeticCategory Category, const TCHAR* Label, UStaticMesh* Mesh,
        FName Socket, FVector Scale, FVector FallbackLocation, FLinearColor Color, FVector LocalOffset = FVector::ZeroVector)
    {
        FCosmeticDefinition& Entry = Catalog.AddDefaulted_GetRef();
        Entry.Id = Id;
        Entry.Category = Category;
        Entry.DisplayName = FText::FromString(Label);
        Entry.StaticMesh = Mesh;
        Entry.SocketName = Socket;
        Entry.RelativeTransform = FTransform(FQuat::Identity, LocalOffset, Scale);
        Entry.ActorRelativeTransform = FTransform(FQuat::Identity, FallbackLocation, Scale);
        if (Material) { Entry.Materials.Add(Material); }
        Entry.bFitInActorSpace = Category != ECosmeticCategory::Prop;
        Entry.bUseTint = true;
        Entry.Tint = Color;
    };
    Add(TEXT("Head.PoliceCap"), ECosmeticCategory::Head, TEXT("Police cap"), Cylinder, TEXT("head"), FVector(.36,.36,.10), FVector(0,0,87), FLinearColor(.05,.16,.65), FVector(0,0,12));
    Add(TEXT("Head.RobberMask"), ECosmeticCategory::Head, TEXT("Robber mask"), Cube, TEXT("head"), FVector(.12,.32,.10), FVector(18,0,72), FLinearColor(.025,.025,.025), FVector(12,0,2));
    Add(TEXT("Head.Crown"), ECosmeticCategory::Head, TEXT("Crown"), Cone, TEXT("head"), FVector(.35,.35,.28), FVector(0,0,100), FLinearColor(1,.6,.03), FVector(0,0,20));
    Add(TEXT("Head.Chicken"), ECosmeticCategory::Head, TEXT("Chicken head"), Sphere, TEXT("head"), FVector(.44,.35,.45), FVector(0,0,85), FLinearColor(1,.9,.4), FVector(0,0,10));
    Add(TEXT("Body.Casual"), ECosmeticCategory::Body, TEXT("Casual"), Cube, TEXT("spine_03"), FVector(.32,.50,.42), FVector(0,0,20), FLinearColor(.10,.65,.3));
    Add(TEXT("Body.Uniform"), ECosmeticCategory::Body, TEXT("Uniform"), Cube, TEXT("spine_03"), FVector(.36,.56,.48), FVector(0,0,20), FLinearColor(.05,.15,.65));
    Add(TEXT("Body.Suit"), ECosmeticCategory::Body, TEXT("Suit"), Cube, TEXT("spine_03"), FVector(.38,.58,.56), FVector(0,0,14), FLinearColor(.08,.08,.10));
    Add(TEXT("Body.Dress"), ECosmeticCategory::Body, TEXT("Dress"), Cone, TEXT("pelvis"), FVector(.85,.85,.85), FVector(0,0,-45), FLinearColor(.65,.06,.3), FVector(0,0,-25));
    Add(TEXT("Prop.Banana"), ECosmeticCategory::Prop, TEXT("Banana"), Cube, TEXT("hand_r"), FVector(.08,.10,.32), FVector(30,35,-10), FLinearColor(1,.8,.05));
    Add(TEXT("Prop.Donut"), ECosmeticCategory::Prop, TEXT("Donut"), Cylinder, TEXT("hand_r"), FVector(.20,.20,.06), FVector(30,35,-10), FLinearColor(.65,.22,.06));
    Add(TEXT("Prop.MoneyBag"), ECosmeticCategory::Prop, TEXT("Money bag"), Sphere, TEXT("hand_r"), FVector(.30,.24,.38), FVector(30,35,-20), FLinearColor(.25,.5,.15));
    Add(TEXT("Prop.Microphone"), ECosmeticCategory::Prop, TEXT("Microphone"), Cylinder, TEXT("hand_r"), FVector(.06,.06,.30), FVector(30,35,-10), FLinearColor(.16,.16,.18));
    Add(TEXT("Face.Glasses"), ECosmeticCategory::Face, TEXT("Glasses"), Cube, TEXT("head"), FVector(.06,.30,.07), FVector(19,0,75), FLinearColor(.05,.1,.15), FVector(14,0,5));
    Add(TEXT("Face.Mustache"), ECosmeticCategory::Face, TEXT("Mustache"), Sphere, TEXT("head"), FVector(.06,.20,.05), FVector(20,0,65), FLinearColor(.15,.05,.015), FVector(15,0,-3));
    for (auto& Item : Catalog)
    {
        if (Item.Id==TEXT("Head.Chicken")) { Item.CosmeticTags.Add(TEXT("Costume.AnimalHead")); }
        if (Item.Id==TEXT("Body.Uniform")) { Item.CosmeticTags.Add(TEXT("Costume.OfficialUniform")); }
        if(Item.Category==ECosmeticCategory::Body)
        {
            Item.StaticMesh=nullptr;Item.bFitInActorSpace=false;
            if(auto* Skin=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/OneManStudio/Characters/Base3/Customization/Materials/M_Base3Skin"),nullptr,LOAD_NoWarn|LOAD_Quiet)){Item.Materials={Skin};}
            Item.ClothingStyle=Item.Id==TEXT("Body.Casual")?1:Item.Id==TEXT("Body.Uniform")?2:Item.Id==TEXT("Body.Suit")?3:4;
        }
    }
    FCosmeticDefinition NoneBody=*Catalog.FindByPredicate([](const auto& Item){return Item.Id==TEXT("Body.Casual");});NoneBody.Id=TEXT("Body.None");NoneBody.DisplayName=NSLOCTEXT("FinalTakeUI","WardrobeTypes.46efc14281","No clothing");NoneBody.ClothingStyle=0;NoneBody.Tint=FLinearColor::White;Catalog.Add(NoneBody);
    const auto Face=[&](ECosmeticCategory Category,const TCHAR* Folder,const TCHAR* Prefix,const TCHAR* Name)
    {
        FCosmeticDefinition Entry;Entry.Category=Category;Entry.Id=FName(*(FString(Prefix)+TEXT(".")+Name));Entry.DisplayName=FText::FromString(Name);
        const FString Path=FString::Printf(TEXT("/Game/OneManStudio/Cosmetics/%s/T_%s_%s"),Folder,Folder==FString(TEXT("Mouths"))?TEXT("Mouth"):Folder,Name);
        Entry.FaceTexture=LoadObject<UTexture2D>(nullptr,*Path,nullptr,LOAD_NoWarn|LOAD_Quiet);Entry.Icon=Entry.FaceTexture;Catalog.Add(Entry);
    };
    for(const TCHAR* Name:{TEXT("Classic"),TEXT("Happy"),TEXT("Angry"),TEXT("Sleepy"),TEXT("Surprised"),TEXT("Dizzy"),TEXT("Bored"),TEXT("Hypnotized"),TEXT("WideStare")}){Face(ECosmeticCategory::Eyes,TEXT("Eyes"),TEXT("Eyes"),Name);}
    for(const TCHAR* Name:{TEXT("Smile"),TEXT("Teeth"),TEXT("Sad"),TEXT("Open"),TEXT("Evil"),TEXT("Laugh")}){Face(ECosmeticCategory::Mouth,TEXT("Mouths"),TEXT("Mouth"),Name);}
    for(const TCHAR* Name:{TEXT("Neutral"),TEXT("Angry"),TEXT("Sad"),TEXT("Surprised"),TEXT("Suspicious"),TEXT("Blocky")}){Face(ECosmeticCategory::Eyebrows,TEXT("Eyebrows"),TEXT("Brows"),Name);}
    if(const auto* Art=LoadObject<UProductionArtCatalog>(nullptr,TEXT("/Game/OneManStudio/Cosmetics/ProductionArt/DA_ProductionCosmetics"),nullptr,LOAD_NoWarn|LOAD_Quiet)){Art->ApplyTo(Catalog);}
    for(auto& Item:Catalog){Item.DisplayName=LocalizedName(Item.Id,Item.DisplayName);}
    return Catalog;
}
