#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "ActingSceneClockSubsystem.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "MovieManager.h"
#include "ProductionArtCatalog.h"
#include "ReplayActor.h"
#include "StudioAnimInstance.h"
#include "StudioCharacterProfile.h"
#include "UnifiedWardrobeWidget.h"
#include "WardrobeComponent.h"

namespace UnifiedTests
{
UStudioCharacterProfile* Profile()
{
    return LoadObject<UStudioCharacterProfile>(
        nullptr, TEXT("/Game/OneManStudio/Characters/Base3/Customization/DA_Base3Character"));
}
struct FWorld
{
    UWorld* World = nullptr;
    APawn* Pawn = nullptr;
    USkeletalMeshComponent* Body = nullptr;
    UWardrobeComponent* Wardrobe = nullptr;
    FWorld()
    {
        UWorld::InitializationValues V;
        V.AllowAudioPlayback(false)
            .RequiresHitProxies(false)
            .CreatePhysicsScene(true)
            .CreateNavigation(false)
            .CreateAISystem(false)
            .CreateFXSystem(false)
            .ShouldSimulatePhysics(false)
            .SetTransactional(false);
        World = UWorld::CreateWorld(
            EWorldType::Game, false,
            MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("UnifiedTestWorld")), nullptr, true,
            ERHIFeatureLevel::Num, &V);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        World->InitializeActorsForPlay(FURL());
        World->GetWorldSettings()->NotifyBeginPlay();
        Pawn = World->SpawnActor<APawn>();
        auto* Root = NewObject<USceneComponent>(Pawn);
        Pawn->AddInstanceComponent(Root);
        Pawn->SetRootComponent(Root);
        Root->RegisterComponent();
        Body = NewObject<USkeletalMeshComponent>(Pawn);
        Pawn->AddInstanceComponent(Body);
        Body->SetupAttachment(Root);
        Body->RegisterComponent();
        Body->SetSkeletalMesh(Profile()->BodyMesh);
        Body->SetMaterial(0, Profile()->SkinMaterial);
        Wardrobe = NewObject<UWardrobeComponent>(Pawn);
        Pawn->AddInstanceComponent(Wardrobe);
        Wardrobe->RegisterComponent();
        Wardrobe->TargetMesh = Body;
        Wardrobe->FacialSettings = Profile()->Face;
    }
    ~FWorld()
    {
        World->EndPlay(EEndPlayReason::Quit);
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    }
    FActingRecording Clip(const FRecordedAppearance& Appearance)
    {
        FActingRecording R;
        R.Appearance = Appearance;
        R.ActingSettings.SkeletalMesh = Profile()->BodyMesh;
        R.ActingSettings.AnimationClass = Profile()->AnimationClass;
        R.Samples.AddDefaulted();
        auto& Last = R.Samples.AddDefaulted_GetRef();
        Last.TimeSeconds = .25;
        return R;
    }
    AMovieManager* Movie()
    {
        auto* Player = World->SpawnActor<APlayerController>();
        if (!Player->PlayerCameraManager)
        {
            Player->SpawnPlayerCameraManager();
        }
        Player->Possess(Pawn);
        Player->SetViewTarget(Pawn);
        auto* M = World->SpawnActor<AMovieManager>();
        M->bAutoStart = false;
        M->bShowWardrobeUI = false;
        M->bShowObjectiveHUD = false;
        M->bBindRestartKey = false;
        M->bBindActingKeys = false;
        M->bBindInteractionKey = false;
        M->bUsePerformerCosmeticCatalog = true;
        return M;
    }
    static UMaterialInstanceDynamic* FaceMaterial(UWardrobeComponent* W)
    {
        for (UMeshComponent* Part : W->CosmeticMeshes)
        {
            if (Part->ComponentHasTag(TEXT("Customization.Face")))
            {
                return Cast<UMaterialInstanceDynamic>(Part->GetMaterial(0));
            }
        }
        return nullptr;
    }
};
} // namespace UnifiedTests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnifiedProfileTest, "OneManStudio.Customization.ProfileAndCatalog",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUnifiedProfileTest::RunTest(const FString&)
{
    UStudioCharacterProfile* P = UnifiedTests::Profile();
    if (!TestNotNull(TEXT("Final character profile is assigned"), P))
    {
        return false;
    }
    TestTrue(TEXT("Reuses the existing imported mesh"),
             P->BodyMesh && P->BodyMesh->GetPathName().Contains(TEXT("/Base3/SK_Base3.")));
    TestTrue(TEXT("Animation Blueprint uses the shared native locomotion inputs"),
             P->AnimationClass && P->AnimationClass->IsChildOf(UStudioAnimInstance::StaticClass()));
    TestTrue(TEXT("Conforming head surface and transparent None are configured"),
             P->Face.SurfaceMesh && P->Face.Material && P->Face.NoneTexture && P->Face.IsValid());
    TestEqual(TEXT("Five gestures remain independently configurable"), P->DefaultGestures.Num(), 5);
    for (const auto& G : P->DefaultGestures)
    {
        TestTrue(TEXT("Every gesture has a target-skeleton animation"),
                 G.Animation && G.Animation->GetSkeleton() == P->BodyMesh->GetSkeleton());
    }
    UnifiedTests::FWorld W;
    FText Error;
    TestTrue(TEXT("Combined catalog valid"), W.Wardrobe->ValidateCatalog(Error));
    TestEqual(TEXT("Nine eye pairs"), W.Wardrobe->GetCategoryOptions(ECosmeticCategory::Eyes).Num(), 9);
    TestEqual(TEXT("Six mouths"), W.Wardrobe->GetCategoryOptions(ECosmeticCategory::Mouth).Num(), 6);
    TestEqual(TEXT("Six eyebrow pairs"), W.Wardrobe->GetCategoryOptions(ECosmeticCategory::Eyebrows).Num(), 6);
    for (const auto& I : W.Wardrobe->Catalog)
    {
        if (I.Category == ECosmeticCategory::Eyes || I.Category == ECosmeticCategory::Mouth ||
            I.Category == ECosmeticCategory::Eyebrows)
        {
            TestTrue(TEXT("Facial sprites and thumbnails reference the supplied 512px texture"),
                     I.FaceTexture && I.Icon == I.FaceTexture && I.FaceTexture->Source.GetSizeX() == 512 &&
                         I.FaceTexture->Source.GetSizeY() == 512);
        }
    }
    TestTrue(TEXT("Runtime default points to the unified UI"),
             GetDefault<AMovieManager>()->WardrobeWidgetClass->IsChildOf(UUnifiedWardrobeWidget::StaticClass()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnifiedSnapshotTest, "OneManStudio.Customization.FacialSnapshotIsolation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUnifiedSnapshotTest::RunTest(const FString&)
{
    if (!UnifiedTests::Profile())
    {
        AddError(TEXT("Character assets missing"));
        return false;
    }
    UnifiedTests::FWorld W;
    FCosmeticSelection First;
    First.BodyId = TEXT("Body.Suit");
    First.EyeId = TEXT("Eyes.Classic");
    First.MouthId = TEXT("Mouth.Smile");
    First.EyebrowId = TEXT("Brows.Neutral");
    First.SkinColor = FLinearColor(.45, .22, .85);
    TestTrue(TEXT("Select full first appearance"), W.Wardrobe->BeginSelection(First));
    const FRecordedAppearance Frozen = W.Wardrobe->CaptureAppearance();
    auto* Ghost = W.World->SpawnActor<AReplayActor>();
    TestTrue(TEXT("Replay restores the full snapshot"), Ghost->PrepareRecording(W.Clip(Frozen)));
    UMaterialInstanceDynamic* Face1 = UnifiedTests::FWorld::FaceMaterial(Ghost->Wardrobe);
    UMaterialInterface* Skin1 = Ghost->PerformanceMesh->GetMaterial(0);
    TestNotNull(TEXT("Independent facial MID exists"), Face1);
    TestNotNull(TEXT("Independent skin MID exists"), Cast<UMaterialInstanceDynamic>(Skin1));
    W.Wardrobe->SelectItem(ECosmeticCategory::Eyes, TEXT("Eyes.Angry"));
    W.Wardrobe->SetSkinColor(FLinearColor(.32, .65, 1));
    W.Wardrobe->SelectItem(ECosmeticCategory::Body, TEXT("Body.Uniform"));
    TestEqual(TEXT("Changing clothes retains the current face"), W.Wardrobe->GetSelection().EyeId,
              FName(TEXT("Eyes.Angry")));
    TestTrue(TEXT("Clothing leaves skin unchanged"),
             W.Wardrobe->GetSelection().SkinColor.Equals(FLinearColor(.32, .65, 1)));
    TestEqual(TEXT("Prior replay retains original eyes"), Ghost->Wardrobe->GetSelection().EyeId, First.EyeId);
    TestTrue(TEXT("Prior replay retains original skin"),
             Ghost->Wardrobe->GetSelection().SkinColor.Equals(First.SkinColor));
    TestTrue(TEXT("Actors never share skin/face MIDs"),
             Skin1 != W.Body->GetMaterial(0) && Face1 != UnifiedTests::FWorld::FaceMaterial(W.Wardrobe));
    FRecordedAppearance Bad = Frozen;
    Bad.FaceSettings.Eyes.Scale.X = 0;
    TestFalse(TEXT("Invalid layer scale rejected"), Bad.IsValid());
    TestFalse(TEXT("Invalid face cannot replace a valid replay"), Ghost->PrepareRecording(W.Clip(Bad)));
    TestFalse(TEXT("Replay cannot edit facial selections"),
              Ghost->Wardrobe->SelectItem(ECosmeticCategory::Mouth, TEXT("Mouth.Evil")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnifiedHandsTest, "OneManStudio.Customization.TwoHandsAndReset",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUnifiedHandsTest::RunTest(const FString&)
{
    if (!UnifiedTests::Profile())
    {
        AddError(TEXT("Character assets missing"));
        return false;
    }
    UnifiedTests::FWorld W;
    FCosmeticSelection S;
    S.BodyId = TEXT("Body.None");
    S.PropId = TEXT("Prop.MoneyBag");
    S.LeftHandPropId = TEXT("Prop.MoneyBag");
    W.Wardrobe->BeginSelection(S);
    const auto Frozen = W.Wardrobe->CaptureAppearance();
    TestTrue(TEXT("Same stable prop ID can equip both hands without duplicate catalog entries"), Frozen.IsValid());
    int32 Props = 0;
    for (const auto& Item : Frozen.Items)
    {
        if (Item.Category == ECosmeticCategory::Prop || Item.Category == ECosmeticCategory::LeftProp)
        {
            ++Props;
            TestTrue(TEXT("Imported scale normalization is frozen"), Item.bSocketScaleResolved);
        }
    }
    TestEqual(TEXT("Two independently equipped prop values"), Props, 2);
    auto* Ghost = W.World->SpawnActor<AReplayActor>();
    Ghost->PrepareRecording(W.Clip(Frozen));
    for (UMeshComponent* Part : Ghost->Wardrobe->CosmeticMeshes)
    {
        if (Part->ComponentHasTag(TEXT("Prop.MoneyBag")))
        {
            const UMeshComponent* LivePart = nullptr;
            for (UMeshComponent* Candidate : W.Wardrobe->CosmeticMeshes)
            {
                if (Candidate->ComponentHasTag(TEXT("Prop.MoneyBag")) &&
                    Candidate->GetAttachSocketName() == Part->GetAttachSocketName())
                {
                    LivePart = Candidate;
                    break;
                }
            }
            TestTrue(TEXT("Replay does not normalize the socket scale twice"),
                     LivePart && Part->GetComponentScale().Equals(LivePart->GetComponentScale(), .001));
            TestFalse(TEXT("Props cannot physically interfere"), Part->IsCollisionEnabled());
        }
    }
    TestTrue(TEXT("Randomize produces a valid complete appearance"),
             W.Wardrobe->RandomizeCustomization() && W.Wardrobe->CaptureAppearance().IsValid());
    W.Wardrobe->ResetCustomization();
    const auto White = W.Wardrobe->GetSelection();
    TestTrue(TEXT("Reset returns a completely white faceless character"),
             White.SkinColor.Equals(FLinearColor::White) && White.BodyId == TEXT("Body.None") && White.EyeId.IsNone() &&
                 White.MouthId.IsNone() && White.EyebrowId.IsNone() && White.HeadId.IsNone() && White.FaceId.IsNone() &&
                 White.PropId.IsNone() && White.LeftHandPropId.IsNone());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnifiedUITest, "OneManStudio.Customization.UnifiedInterfaceAndGuard",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUnifiedUITest::RunTest(const FString&)
{
    if (!UnifiedTests::Profile())
    {
        AddError(TEXT("Character assets missing"));
        return false;
    }
    UnifiedTests::FWorld W;
    AMovieManager* M = W.Movie();
    TestTrue(TEXT("Start existing movie coordinator"), M->StartMovie());
    auto* UI = NewObject<UUnifiedWardrobeWidget>();
    UI->Initialize();
    UI->OpenWardrobe(M, M->PreparationId);
    for (const TCHAR* Id : {TEXT("Tab.Eyes"), TEXT("Tab.Mouth"), TEXT("Tab.Eyebrows"), TEXT("Tab.Skin"),
                            TEXT("Tab.Clothing"), TEXT("Tab.Hats"), TEXT("Tab.Accessories"), TEXT("Tab.Objects"),
                            TEXT("Action.Ready"), TEXT("Action.Random"), TEXT("Action.Reset")})
    {
        TestNotNull(TEXT("All categories and actions live in one widget"), UI->FindControl(Id));
    }
    UI->ChooseItem(ECosmeticCategory::Eyes, TEXT("Eyes.Happy"));
    UI->ChooseSkin(FLinearColor(.45, .22, .85));
    UI->SelectTab(ECustomizationTab::Clothing);
    UI->ChooseItem(ECosmeticCategory::Body, TEXT("Body.Uniform"));
    UI->SelectTab(ECustomizationTab::Eyes);
    TestEqual(TEXT("Tabs retain facial selection"), UI->ViewData.Selection.EyeId, FName(TEXT("Eyes.Happy")));
    TestTrue(TEXT("One scene clock remains stopped during editing"),
             !W.World->GetSubsystem<UActingSceneClockSubsystem>()->IsClockRunning());
    TestTrue(TEXT("Ready calls the existing guarded transition"), UI->Ready());
    TestFalse(TEXT("Ready cannot start the take twice"), UI->Ready());
    TestFalse(TEXT("Recording locks all customization"), UI->ResetAll());
    M->StopMovie();
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProductionCosmeticsTest, "OneManStudio.Customization.ProductionAssetsAndHands",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FProductionCosmeticsTest::RunTest(const FString&)
{
    const auto* Art = LoadObject<UProductionArtCatalog>(
        nullptr, TEXT("/Game/OneManStudio/Cosmetics/ProductionArt/DA_ProductionCosmetics"));
    if (!TestNotNull(TEXT("Editable production catalog exists"), Art)) { return false; }
    TestEqual(TEXT("Supplied wearables, six clothing variants and compatible mask alias"), Art->Cosmetics.Num(), 22);
    TSet<FName> Ids;
    int32 Bodies=0, Props=0;
    for (const auto& Item : Art->Cosmetics)
    {
        TestFalse(TEXT("Production IDs remain unique"), Ids.Contains(Item.Id));
        Ids.Add(Item.Id);
        TestNotNull(TEXT("Each item has a selectable visual thumbnail"), Item.Icon.Get());
        if (Item.Category == ECosmeticCategory::Body)
        {
            ++Bodies;
            TestTrue(TEXT("Clothing reuses the existing rig through a material on UV0"),
                     !Item.StaticMesh && !Item.SkeletalMesh && Item.Materials.Num()==1 && Item.Materials[0]);
        }
        else
        {
            if(Item.Category==ECosmeticCategory::Head || Item.Category==ECosmeticCategory::Face)
            {
                TestTrue(TEXT("Head accessories use reference-pose bone offsets, independent of selection-time idle pose"),
                         Item.SocketName==TEXT("Head") && !Item.bFitInActorSpace && Item.bSocketScaleResolved);
            }
            TestTrue(TEXT("Wearables use supplied art rather than engine shapes"),
                     Item.StaticMesh && Item.StaticMesh->GetPathName().Contains(TEXT("/ProductionArt/")));
            if (Item.StaticMesh) { TestTrue(TEXT("Wearable triangle budget is respected"), Item.StaticMesh->GetNumTriangles(0)<=24000); }
        }
        if (Item.Category == ECosmeticCategory::Prop)
        {
            ++Props;
            TestTrue(TEXT("Both hands have valid independently authored transforms"),
                     Item.bSocketScaleResolved && Item.bUseLeftHandTransform && Item.RelativeTransform.IsValid() &&
                     Item.LeftHandRelativeTransform.IsValid() && Item.SocketName==TEXT("RightHand"));
        }
    }
    TestEqual(TEXT("Six clothing texture variants"), Bodies, 6);
    TestEqual(TEXT("Six reusable handheld models"), Props, 6);
    UnifiedTests::FWorld W;
    FCosmeticSelection S;S.BodyId=TEXT("Body.Uniform");S.PropId=TEXT("Prop.Microphone");S.LeftHandPropId=S.PropId;
    TestTrue(TEXT("Same prop can be equipped independently in both hands"), W.Wardrobe->BeginSelection(S));
    const auto Frozen=W.Wardrobe->CaptureAppearance();
    const auto* Right=Frozen.Items.FindByPredicate([](const auto& I){return I.Category==ECosmeticCategory::Prop;});
    const auto* Left=Frozen.Items.FindByPredicate([](const auto& I){return I.Category==ECosmeticCategory::LeftProp;});
    TestTrue(TEXT("Frozen hand values retain different attachment bones and authored offsets"), Right && Left &&
             Right->SocketName==TEXT("RightHand") && Left->SocketName==TEXT("LeftHand") &&
             Left->RelativeTransform.Equals(Left->LeftHandRelativeTransform));
    return true;
}
#endif
