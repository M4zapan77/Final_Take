#if WITH_DEV_AUTOMATION_TESTS

#include "WardrobeComponent.h"
#include "WardrobeWidget.h"
#include "ActingComponent.h"
#include "ActingRecorderComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "MovieManager.h"
#include "ReplayActor.h"
#include "Algo/Reverse.h"
#include "Animation/AnimSequence.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"

namespace WardrobeTests
{
    class FWorld
    {
    public:
        FWorld()
        {
            UWorld::InitializationValues Values;
            Values.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true)
                .CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false)
                .ShouldSimulatePhysics(false).SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("WardrobeTestWorld")),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            World->InitializeActorsForPlay(FURL());
            World->GetWorldSettings()->NotifyBeginPlay();
        }

        ~FWorld()
        {
            World->EndPlay(EEndPlayReason::Quit);
            World->DestroyWorld(false);
            GEngine->DestroyWorldContext(World);
        }

        void Tick(float Delta = .025f) { ++GFrameCounter; World->Tick(LEVELTICK_All, Delta); }

        UWardrobeComponent* MakeLive()
        {
            Pawn = World->SpawnActor<APawn>();
            USceneComponent* Root = NewObject<USceneComponent>(Pawn);
            Pawn->AddInstanceComponent(Root);
            Pawn->SetRootComponent(Root);
            Root->RegisterComponent();
            Body = NewObject<UStaticMeshComponent>(Pawn);
            Pawn->AddInstanceComponent(Body);
            Body->SetupAttachment(Root);
            Body->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
            Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Body->RegisterComponent();
            Wardrobe = NewObject<UWardrobeComponent>(Pawn);
            Pawn->AddInstanceComponent(Wardrobe);
            Wardrobe->RegisterComponent();
            Wardrobe->FallbackBodyMesh = Body;
            return Wardrobe;
        }

        AMovieManager* SetupMovie()
        {
            MakeLive();
            Player = World->SpawnActor<APlayerController>();
            if (!Player->PlayerCameraManager) { Player->SpawnPlayerCameraManager(); }
            Player->Possess(Pawn);
            Player->SetViewTarget(Pawn);
            Manager = World->SpawnActor<AMovieManager>();
            Manager->PreparationDurationSeconds = .1;
            Manager->RecordingDurationSeconds = .3;
            Manager->bShowDebugOverlay = false;
            Manager->bBindActingKeys = false;
            Manager->bBindRestartKey = false;
            return Manager;
        }

        UWorld* World = nullptr;
        APawn* Pawn = nullptr;
        APlayerController* Player = nullptr;
        UStaticMeshComponent* Body = nullptr;
        UWardrobeComponent* Wardrobe = nullptr;
        AMovieManager* Manager = nullptr;
    };

    FActingRecording Clip(const FRecordedAppearance& Appearance)
    {
        FActingRecording Recording;
        Recording.Samples.AddDefaulted();
        Recording.Samples.AddDefaulted_GetRef().TimeSeconds = 1.0;
        Recording.Appearance = Appearance;
        return Recording;
    }

    FCosmeticSelection Outfit(FName Head, FName Body, FName Prop)
    {
        FCosmeticSelection Selection;
        Selection.HeadId = Head;
        Selection.BodyId = Body;
        Selection.PropId = Prop;
        return Selection;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWardrobeCatalogTest, "OneManStudio.Wardrobe.CatalogAndFallback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWardrobeCatalogTest::RunTest(const FString& Parameters)
{
    WardrobeTests::FWorld Context;
    UWardrobeComponent* Wardrobe = Context.MakeLive();
    FText Error;
    TestTrue(TEXT("Placeholder catalog valid"), Wardrobe->ValidateCatalog(Error));
    TestEqual(TEXT("Final cosmetics, removable clothing, facial variants and compatible mask slot"), Wardrobe->Catalog.Num(), 44);
    TestTrue(TEXT("An empty choice produces a valid default body"), Wardrobe->BeginSelection(FCosmeticSelection()));
    TestEqual(TEXT("Default is identified by stable ID"), Wardrobe->GetSelection().BodyId, FName(TEXT("Body.Casual")));
    TestTrue(TEXT("Hat selected"), Wardrobe->SelectItem(ECosmeticCategory::Head, TEXT("Head.Crown")));
    TestTrue(TEXT("Prop selected"), Wardrobe->SelectItem(ECosmeticCategory::Prop, TEXT("Prop.Banana")));
    FCosmeticSelection Saved = Wardrobe->GetSelection();
    Algo::Reverse(Wardrobe->Catalog);
    TestTrue(TEXT("Selection restores after catalog reorder"), Wardrobe->SetSelection(Saved));
    TestEqual(TEXT("Reordering never changes selected ID"), Wardrobe->GetSelection().HeadId, Saved.HeadId);
    TestFalse(TEXT("Wrong category rejected"), Wardrobe->SelectItem(ECosmeticCategory::Head, TEXT("Prop.Banana")));
    TestFalse(TEXT("Unknown ID rejected"), Wardrobe->SelectItem(ECosmeticCategory::Prop, TEXT("missing")));
    TestEqual(TEXT("Rejected selection leaves current prop intact"), Wardrobe->GetSelection().PropId, Saved.PropId);
    TestFalse(TEXT("Body cannot be unequipped"), Wardrobe->SelectItem(ECosmeticCategory::Body, NAME_None));
    Wardrobe->Catalog.RemoveAll([](const FCosmeticDefinition& Item) { return Item.Id == TEXT("Body.Casual"); });
    TestEqual(TEXT("Invalid default falls back deterministically, independent of order"), Wardrobe->NormalizeSelection(FCosmeticSelection()).BodyId, FName(TEXT("Body.Dress")));
    const TArray<FCosmeticDefinition> ValidCatalog = Wardrobe->Catalog;
    Wardrobe->Catalog.Add(ValidCatalog[0]);
    TestFalse(TEXT("Duplicate IDs rejected"), Wardrobe->ValidateCatalog(Error));
    Wardrobe->Catalog = ValidCatalog;
    Wardrobe->Catalog.RemoveAll([](const FCosmeticDefinition& Item) { return Item.Category == ECosmeticCategory::Body; });
    TestFalse(TEXT("Catalog without any valid default body rejected"), Wardrobe->ValidateCatalog(Error));
    // Check the actual material supports the placeholder color controls, not just a stored MID override.
    UMaterialInterface* PlaceholderMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    TArray<FMaterialParameterInfo> Infos;
    TArray<FGuid> Guids;
    PlaceholderMaterial->GetAllVectorParameterInfo(Infos, Guids);
    const bool bColorParameter = Infos.ContainsByPredicate([](const FMaterialParameterInfo& Info) { return Info.Name == TEXT("Color"); });
    if (!bColorParameter)
    {
        for (const FMaterialParameterInfo& Info : Infos) { AddInfo(FString::Printf(TEXT("BasicShapeMaterial vector parameter: %s"), *Info.Name.ToString())); }
    }
    TestTrue(TEXT("Placeholder material exposes its visible Color parameter"), bColorParameter);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWardrobeSnapshotTest, "OneManStudio.Wardrobe.SnapshotIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWardrobeSnapshotTest::RunTest(const FString& Parameters)
{
    WardrobeTests::FWorld Context;
    UWardrobeComponent* Wardrobe = Context.MakeLive();
    const FLinearColor Red(.9,.03,.03);
    const FLinearColor Blue(.03,.1,.9);
    Wardrobe->BeginSelection(WardrobeTests::Outfit(TEXT("Head.Crown"), TEXT("Body.Dress"), TEXT("Prop.Banana")));
    Wardrobe->SetBodyColor(true, Red);
    const FRecordedAppearance First = Wardrobe->CaptureAppearance();
    TestTrue(TEXT("Snapshot is complete and valid"), First.IsValid());
    Wardrobe->SetSelection(WardrobeTests::Outfit(TEXT("Head.PoliceCap"), TEXT("Body.Uniform"), TEXT("Prop.Donut")));
    Wardrobe->SetBodyColor(true, Blue);
    const FRecordedAppearance Second = Wardrobe->CaptureAppearance();
    AReplayActor* Ghost1 = Context.World->SpawnActor<AReplayActor>();
    AReplayActor* Ghost2 = Context.World->SpawnActor<AReplayActor>();
    Ghost1->PrepareRecording(WardrobeTests::Clip(First));
    Ghost2->PrepareRecording(WardrobeTests::Clip(Second));
    TestEqual(TEXT("First replay retains original hat"), Ghost1->Wardrobe->GetSelection().HeadId, First.Selection.HeadId);
    TestEqual(TEXT("Second replay retains its own body variant"), Ghost2->Wardrobe->GetSelection().BodyId, Second.Selection.BodyId);
    TestTrue(TEXT("Independent body colors retained"), Ghost1->Wardrobe->GetSelection().BodyColor.Equals(Red) && Ghost2->Wardrobe->GetSelection().BodyColor.Equals(Blue));
    TestFalse(TEXT("Replay is never editable"), Ghost1->Wardrobe->SelectItem(ECosmeticCategory::Head, TEXT("Head.Chicken")));
    TestEqual(TEXT("Each replay has independent hat and prop attachments; clothing uses a material"), Ghost1->Wardrobe->CosmeticMeshes.Num(), 2);
    TestEqual(TEXT("Second replay has its own attachments"), Ghost2->Wardrobe->CosmeticMeshes.Num(), 2);
    for (int32 Index = 0; Index < Ghost1->Wardrobe->CosmeticMeshes.Num(); ++Index)
    {
        UMeshComponent* Part = Ghost1->Wardrobe->CosmeticMeshes[Index];
        TestFalse(TEXT("Cosmetic parts cannot collide"), Part->IsCollisionEnabled());
        TestFalse(TEXT("Cosmetic parts cannot generate overlaps"), Part->GetGenerateOverlapEvents());
        TestTrue(TEXT("Parts are not shared between actors"), Part != Ghost2->Wardrobe->CosmeticMeshes[Index]);
        TestTrue(TEXT("Per-actor tint instances are not shared"), Part->GetMaterial(0) != Ghost2->Wardrobe->CosmeticMeshes[Index]->GetMaterial(0));
    }
    // Even externally supplied dynamic material references are frozen in recording data.
    UMaterialInterface* ExternalParent = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    UMaterialInstanceDynamic* External = UMaterialInstanceDynamic::Create(ExternalParent, Context.Pawn);
    External->SetVectorParameterValue(TEXT("Color"), Red);
    FCosmeticDefinition* BodyItem = Wardrobe->Catalog.FindByPredicate([](const FCosmeticDefinition& Item) { return Item.Id == TEXT("Body.Casual"); });
    BodyItem->StaticMesh = nullptr;
    BodyItem->bUseTint = false;
    BodyItem->Materials = {External};
    Wardrobe->SetSelection(WardrobeDefaults::Casual());
    const FRecordedAppearance Frozen = Wardrobe->CaptureAppearance();
    External->SetVectorParameterValue(TEXT("Color"), Blue);
    const FCosmeticDefinition* FrozenBody = Frozen.Items.FindByPredicate([](const FCosmeticDefinition& Item) { return Item.Category == ECosmeticCategory::Body; });
    UMaterialInstanceDynamic* FrozenMaterial = FrozenBody ? Cast<UMaterialInstanceDynamic>(FrozenBody->Materials[0]) : nullptr;
    TestTrue(TEXT("Recording owns a frozen dynamic material copy"), FrozenMaterial && FrozenMaterial != External && FrozenMaterial->K2_GetVectorParameterValue(TEXT("Color")).Equals(Red));
    FRecordedAppearance Bad = First;
    Bad.Selection.HeadId = TEXT("Head.Unknown");
    TestFalse(TEXT("Incomplete snapshot rejected"), Bad.IsValid());
    FActingRecording Invalid = WardrobeTests::Clip(Bad);
    TestFalse(TEXT("Invalid appearance makes a recording invalid"), Invalid.IsValid());
    TestFalse(TEXT("Invalid snapshot cannot replace an existing replay"), Ghost1->PrepareRecording(Invalid));
    TestEqual(TEXT("Rejected snapshot preserves previous outfit"), Ghost1->Wardrobe->GetSelection().HeadId, First.Selection.HeadId);
    const int32 OriginalOwnedCount = Context.Pawn->GetInstanceComponents().Num();
    for (int32 Change = 0; Change < 8; ++Change) { Wardrobe->SetSelection(WardrobeDefaults::Casual()); }
    TestTrue(TEXT("Changing outfit does not leak owned components"), Context.Pawn->GetInstanceComponents().Num() <= OriginalOwnedCount);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWardrobeReadyTest, "OneManStudio.Wardrobe.ReadyAndTimeout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWardrobeReadyTest::RunTest(const FString& Parameters)
{
    WardrobeTests::FWorld Context;
    AMovieManager* Manager = Context.SetupMovie();
    TestTrue(TEXT("Movie starts"), Manager->StartMovie());
    TestTrue(TEXT("Wardrobe is present in the state flow before Take 1"), Manager->IsWardrobeOpen() && Manager->GetCurrentTakeNumber() == 1);
    TestEqual(TEXT("First role supplied"), Manager->GetCurrentRoleName().ToString(), FString(TEXT("Robber")));
    TestFalse(TEXT("Role includes a script instruction"), Manager->GetCurrentScriptInstruction().IsEmpty());
    TestTrue(TEXT("Movement blocked while choosing"), Context.Player->IsMoveInputIgnored());
    TestTrue(TEXT("Look blocked while choosing"), Context.Player->IsLookInputIgnored());
    UActingRecorderComponent* Recorder = Context.Pawn->FindComponentByClass<UActingRecorderComponent>();
    TestFalse(TEXT("Wardrobe does not record transforms"), Recorder->IsRecording());
    TestFalse(TEXT("Wardrobe does not accept acting events"), Manager->LiveActingComponent->TriggerGesture(EActingGesture::Dance));
    const int32 FirstToken = Manager->PreparationId;
    Context.Tick(.05f);
    TestTrue(TEXT("Wardrobe countdown advances while scene clock stays at zero"), Manager->GetPhaseRemainingSeconds() < .1 && Manager->GetSceneTimeSeconds() == 0.0);
    TestTrue(TEXT("Hat can be selected"), Manager->SelectWardrobeItem(FirstToken, ECosmeticCategory::Head, TEXT("Head.Chicken")));
    TestTrue(TEXT("Body can be selected"), Manager->SelectWardrobeItem(FirstToken, ECosmeticCategory::Body, TEXT("Body.Dress")));
    TestTrue(TEXT("Prop can be selected"), Manager->SelectWardrobeItem(FirstToken, ECosmeticCategory::Prop, TEXT("Prop.Microphone")));
    TestTrue(TEXT("Ready confirms early"), Manager->ReadyForTake(FirstToken));
    TestFalse(TEXT("A second Ready is rejected"), Manager->ReadyForTake(FirstToken));
    TestTrue(TEXT("Recording begins at shared scene zero"), Manager->State == EMovieState::Recording && Manager->GetSceneTimeSeconds() == 0.0);
    TestFalse(TEXT("Gameplay movement restored"), Context.Player->IsMoveInputIgnored());
    TestFalse(TEXT("Gameplay look restored"), Context.Player->IsLookInputIgnored());
    TestEqual(TEXT("Recording contains confirmed selection"), Recorder->GetRecordingData().Appearance.Selection.HeadId, FName(TEXT("Head.Chicken")));
    TestFalse(TEXT("Cannot change outfit during a take"), Context.Wardrobe->SelectItem(ECosmeticCategory::Head, TEXT("Head.Crown")));
    Context.Tick(.1f);
    const double ExistingTime = Manager->GetSceneTimeSeconds();
    Manager->ReadyForTake(FirstToken);
    TestEqual(TEXT("Duplicate confirm never resets the active scene clock"), Manager->GetSceneTimeSeconds(), ExistingTime);
    for (int32 Frame = 0; Frame < 32 && Manager->State != EMovieState::Preparation; ++Frame) { Context.Tick(); }
    TestTrue(TEXT("Wardrobe appears again before Take 2"), Manager->IsWardrobeOpen() && Manager->GetCurrentTakeNumber() == 2);
    TestFalse(TEXT("Stale Ready cannot confirm next take"), Manager->ReadyForTake(FirstToken));
    TestFalse(TEXT("Stale widget cannot change next selection"), Manager->SelectWardrobeItem(FirstToken, ECosmeticCategory::Head, TEXT("Head.Crown")));
    TestEqual(TEXT("Previous actor retains the confirmed head"), Manager->ActiveReplays[0]->Wardrobe->GetSelection().HeadId, FName(TEXT("Head.Chicken")));
    TestEqual(TEXT("New role receives valid default without user input"), Context.Wardrobe->GetSelection().BodyId, FName(TEXT("Body.Uniform")));
    const int32 SecondToken = Manager->PreparationId;
    for (int32 Frame = 0; Frame < 12 && Manager->State == EMovieState::Preparation; ++Frame) { Context.Tick(); }
    TestTrue(TEXT("Timeout confirms through the same transition"), Manager->State == EMovieState::Recording);
    TestFalse(TEXT("Ready after timeout is rejected"), Manager->ReadyForTake(SecondToken));
    TestTrue(TEXT("Default timeout outfit is recorded"), Recorder->GetRecordingData().Appearance.IsValid());
    Manager->StopMovie();
    TestTrue(TEXT("Cancel clears appearance data from recorder"), !Recorder->GetRecordingData().Appearance.bHasAppearance);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWardrobeMovieTest, "OneManStudio.Wardrobe.ThreeTakeOutfits",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWardrobeMovieTest::RunTest(const FString& Parameters)
{
    WardrobeTests::FWorld Context;
    AMovieManager* Manager = Context.SetupMovie();
    UMaterialInterface* OriginalMaterial = Context.Body->GetMaterial(0);
    Manager->PreparationDurationSeconds = .05;
    Manager->RecordingDurationSeconds = .2;
    FCosmeticSelection Outfits[] = {
        WardrobeTests::Outfit(TEXT("Head.Crown"), TEXT("Body.Dress"), TEXT("Prop.Banana")),
        WardrobeTests::Outfit(TEXT("Head.RobberMask"), TEXT("Body.Suit"), TEXT("Prop.Microphone")),
        WardrobeTests::Outfit(TEXT("Head.Chicken"), TEXT("Body.Uniform"), TEXT("Prop.Donut"))};
    bool Chosen[3] = {};
    bool Acted[3] = {};
    int32 PreparationCount = 0;
    bool SawPremiere = false;
    Manager->StartMovie();
    for (int32 Frame = 0; Frame < 180 && Manager->State != EMovieState::Finished && Manager->State != EMovieState::Error; ++Frame)
    {
        const int32 Take = Manager->CurrentTakeIndex;
        if (Manager->IsWardrobeOpen() && !Chosen[Take])
        {
            ++PreparationCount;
            for (int32 Prior = 0; Prior < Take; ++Prior)
            {
                TestEqual(TEXT("Earlier outfits unchanged during new wardrobe"), Manager->ActiveReplays[Prior]->Wardrobe->GetSelection().HeadId, Outfits[Prior].HeadId);
                TestEqual(TEXT("Previous events stay silent during wardrobe"), Manager->ActiveReplays[Prior]->Acting->DispatchedEventCount, 0);
            }
            Context.Wardrobe->SetSelection(Outfits[Take]);
            Chosen[Take] = true;
            Manager->ReadyForTake(Manager->PreparationId);
        }
        if (Manager->State == EMovieState::Recording)
        {
            if (!Acted[Take])
            {
                Manager->LiveActingComponent->TriggerGesture(static_cast<EActingGesture>(Take));
                Manager->LiveActingComponent->TriggerSound(static_cast<EActingSound>(Take));
                Acted[Take] = true;
            }
            Context.Pawn->AddActorWorldOffset(FVector(3,Take + 1,0));
        }
        if (Manager->State == EMovieState::MoviePremiere)
        {
            SawPremiere = true;
            for (int32 ActorIndex = 0; ActorIndex < Manager->ActiveReplays.Num(); ++ActorIndex)
            {
                TestEqual(TEXT("Premiere restores each independent outfit"), Manager->ActiveReplays[ActorIndex]->Wardrobe->GetSelection().BodyId, Outfits[ActorIndex].BodyId);
            }
        }
        Context.Tick();
    }
    TestEqual(TEXT("Wardrobe occurred before all three takes"), PreparationCount, 3);
    TestTrue(TEXT("Full movie finishes"), SawPremiere && Manager->State == EMovieState::Finished);
    TestEqual(TEXT("Three appearances saved"), Manager->Performances.Num(), 3);
    for (int32 Index = 0; Index < Manager->Performances.Num(); ++Index)
    {
        const FActingRecording& Recording = Manager->Performances[Index].Recording;
        TestTrue(TEXT("Each performance keeps valid movement and appearance"), Recording.IsValid() && Recording.Appearance.IsValid());
        TestEqual(TEXT("Each recorded prop ID preserved"), Recording.Appearance.Selection.PropId, Outfits[Index].PropId);
        TestEqual(TEXT("Each final actor keeps its original hat"), Manager->ActiveReplays[Index]->Wardrobe->GetSelection().HeadId, Outfits[Index].HeadId);
        TestEqual(TEXT("Existing gesture and sound replay still run"), Manager->ActiveReplays[Index]->Acting->DispatchedEventCount, 2);
    }
    const int32 OldToken = Manager->PreparationId;
    Manager->RestartMovie();
    TestTrue(TEXT("Restart clears all saved performances and replay actors"), Manager->Performances.IsEmpty() && Manager->ActiveReplays.IsEmpty());
    TestEqual(TEXT("Restart begins with the role default, not previous user selection"), Context.Wardrobe->GetSelection().HeadId, FName(TEXT("Head.RobberMask")));
    TestFalse(TEXT("Old wardrobe session cannot confirm after restart"), Manager->ReadyForTake(OldToken));
    Manager->StopMovie();
    TestEqual(TEXT("Stop restores the original body material"), Context.Body->GetMaterial(0), OriginalMaterial);
    TestTrue(TEXT("Stop removes temporary cosmetics"), Context.Wardrobe->CosmeticMeshes.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWardrobeWidgetTest, "OneManStudio.Wardrobe.NativeWidget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWardrobeWidgetTest::RunTest(const FString& Parameters)
{
    WardrobeTests::FWorld Context;
    AMovieManager* Manager = Context.SetupMovie();
    Manager->PreparationDurationSeconds = 10.0;
    Manager->StartMovie();
    UWardrobeWidget* Widget = NewObject<UWardrobeWidget>(Context.World);
    Widget->Initialize();
    Widget->OpenWardrobe(Manager, Manager->PreparationId);
    TestNotNull(TEXT("Native UMG hierarchy exists without a Widget Blueprint"), Widget->GetRootWidget());
    TestEqual(TEXT("Native UI shows Take 1"), Widget->ViewData.TakeNumber, 1);
    TestEqual(TEXT("Native UI shows role"), Widget->ViewData.RoleName.ToString(), FString(TEXT("Robber")));
    TestTrue(TEXT("Native UI shows ten-second countdown"), FMath::IsNearlyEqual(Widget->ViewData.RemainingSeconds, 10.0));
    TestEqual(TEXT("All six hat options supplied to editable UI"), Widget->GetOptions(ECosmeticCategory::Head).Num(), 6);
    UWardrobeChoiceButton* Crown = nullptr;
    for (UWardrobeChoiceButton* Button : Widget->ChoiceButtons)
    {
        if (Button->CosmeticId == TEXT("Head.Crown")) { Crown = Button; }
    }
    if (!TestNotNull(TEXT("Native UI builds the crown button"), Crown)) { return false; }
    Crown->OnClicked.Broadcast();
    TestEqual(TEXT("Actual UMG button updates preview selection"), Context.Wardrobe->GetSelection().HeadId, FName(TEXT("Head.Crown")));
    TestTrue(TEXT("Preview components rebuilt immediately"), !Context.Wardrobe->CosmeticMeshes.IsEmpty());
    Context.Tick(.1f);
    Widget->RefreshWardrobe();
    TestTrue(TEXT("Native UI countdown reads advancing preparation time"), Widget->ViewData.RemainingSeconds < 10.0);
    if (!TestNotNull(TEXT("Ready button exists"), Widget->ReadyButton.Get())) { return false; }
    Widget->ReadyButton->OnClicked.Broadcast();
    TestTrue(TEXT("Actual UMG Ready button starts recording"), Manager->State == EMovieState::Recording);
    Widget->RefreshWardrobe();
    TestFalse(TEXT("Closed wardrobe disables further input"), Widget->ViewData.bCanChoose);
    Manager->RestartMovie();
    TestFalse(TEXT("A stale widget cannot confirm or edit a new take"), Widget->Ready());
    TestFalse(TEXT("A stale widget cannot select"), Widget->ChooseItem(ECosmeticCategory::Head, TEXT("Head.Chicken")));
    Manager->StopMovie();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWardrobeSkeletonTest, "OneManStudio.Wardrobe.ExistingSkeleton",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWardrobeSkeletonTest::RunTest(const FString& Parameters)
{
    USkeletalMesh* MeshAsset = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    UAnimSequence* Animation = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd"));
    if (!TestNotNull(TEXT("Existing template mesh"), MeshAsset) || !TestNotNull(TEXT("Existing template animation"), Animation)) { return false; }
    WardrobeTests::FWorld Context;
    UWardrobeComponent* Wardrobe = Context.MakeLive();
    USkeletalMeshComponent* Base = NewObject<USkeletalMeshComponent>(Context.Pawn);
    Context.Pawn->AddInstanceComponent(Base);
    Base->SetupAttachment(Context.Pawn->GetRootComponent());
    Base->RegisterComponent();
    Base->SetSkeletalMesh(MeshAsset);
    Base->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    Base->PlayAnimation(Animation, true);
    Wardrobe->TargetMesh = Base;
    UAnimInstance* OriginalAnimInstance = Base->GetAnimInstance();
    if(auto* Item=Wardrobe->Catalog.FindByPredicate([](const auto& I){return I.Id==TEXT("Head.PoliceCap");}))
    {
        Item->SocketName=TEXT("head");Item->bFitInActorSpace=true;
    }
    // The final catalog is authored on Base3. This generic compatibility fixture
    // deliberately authors the equivalent hand attachment on the template rig.
    if(auto* Item=Wardrobe->Catalog.FindByPredicate([](const auto& I){return I.Id==TEXT("Prop.Microphone");}))
    {
        Item->SocketName=TEXT("hand_r");Item->RelativeTransform=FTransform(FVector::ZeroVector);
        Item->bSocketScaleResolved=false;
    }
    Wardrobe->BeginSelection(WardrobeTests::Outfit(TEXT("Head.PoliceCap"), TEXT("Body.Casual"), TEXT("Prop.Microphone")));
    bool bHeadAttached = false;
    bool bPropInHand = false;
    UMeshComponent* HeadPart = nullptr;
    for (UMeshComponent* Mesh : Wardrobe->CosmeticMeshes)
    {
        bHeadAttached |= Mesh->GetAttachParent() == Base && Mesh->GetAttachSocketName() == TEXT("head");
        if (Mesh->ComponentHasTag(TEXT("Head.PoliceCap"))) { HeadPart = Mesh; }
        if (Mesh->ComponentHasTag(TEXT("Prop.Microphone")))
        {
            bPropInHand = Mesh->GetAttachSocketName() == TEXT("hand_r") && Mesh->GetRelativeLocation().IsNearlyZero();
        }
    }
    TestTrue(TEXT("Hat attaches to existing head bone"), bHeadAttached);
    TestTrue(TEXT("Props are anchored at the hand rather than a floating actor-space fit"), bPropInHand);
    const FCosmeticDefinition* HeadDefinition = Wardrobe->Catalog.FindByPredicate([](const FCosmeticDefinition& Item) { return Item.Id == TEXT("Head.PoliceCap"); });
    TestTrue(TEXT("Placeholder fit starts upright in actor space despite bone axes"), HeadPart
        && HeadPart->GetComponentTransform().Equals(HeadDefinition->ActorRelativeTransform * Context.Pawn->GetActorTransform(), .01));
    const FRecordedAppearance Fitted = Wardrobe->CaptureAppearance();
    const FCosmeticDefinition* ResolvedHead = Fitted.Items.FindByPredicate([](const FCosmeticDefinition& Item) { return Item.Id == TEXT("Head.PoliceCap"); });
    TestTrue(TEXT("The resolved socket offset is frozen into the take"), ResolvedHead && !ResolvedHead->bFitInActorSpace
        && HeadPart && ResolvedHead->RelativeTransform.Equals(HeadPart->GetRelativeTransform(), .001));
    FCosmeticDefinition Garment;
    Garment.Id = TEXT("Body.ModularTest");
    Garment.Category = ECosmeticCategory::Body;
    Garment.SkeletalMesh = MeshAsset; // transient fixture, never assigned to art assets
    Wardrobe->Catalog.Add(Garment);
    Wardrobe->SelectItem(ECosmeticCategory::Body, Garment.Id);
    bool bFollowerFound = false;
    for (UMeshComponent* Mesh : Wardrobe->CosmeticMeshes)
    {
        if (USkeletalMeshComponent* Skeletal = Cast<USkeletalMeshComponent>(Mesh)) { bFollowerFound |= Skeletal->LeaderPoseComponent.Get() == Base; }
        TestFalse(TEXT("Cosmetics remain nonphysical on a real rig"), Mesh->IsCollisionEnabled());
    }
    TestTrue(TEXT("Modular body follows the existing pose"), bFollowerFound);
    TestEqual(TEXT("Base skeletal mesh was never replaced"), Base->GetSkeletalMeshAsset(), MeshAsset);
    TestEqual(TEXT("Existing animation instance remains in place"), Base->GetAnimInstance(), OriginalAnimInstance);
    Context.Tick(.1f);
    TestEqual(TEXT("Animation instance survives wardrobe updates"), Base->GetAnimInstance(), OriginalAnimInstance);
    return true;
}

#endif
