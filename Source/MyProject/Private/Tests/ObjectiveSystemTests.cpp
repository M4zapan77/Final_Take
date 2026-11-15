#if WITH_DEV_AUTOMATION_TESTS
#include "ObjectiveTypes.h"
#include "ObjectiveManagerComponent.h"
#include "ObjectiveHUDWidget.h"
#include "ObjectiveZone.h"
#include "InteractableProp.h"
#include "PropInteractionComponent.h"
#include "MovieManager.h"
#include "ActingComponent.h"
#include "ActingRecorderComponent.h"
#include "WardrobeComponent.h"
#include "ReplayActor.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"

namespace ObjectiveTests
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
            World = UWorld::CreateWorld(EWorldType::Game,false,
                MakeUniqueObjectName(GetTransientPackage(),UWorld::StaticClass(),TEXT("ObjectiveWorld")),
                nullptr,true,ERHIFeatureLevel::Num,&Values);
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            World->InitializeActorsForPlay(FURL()); World->GetWorldSettings()->NotifyBeginPlay();
        }
        ~FWorld() { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); }
        void Tick(float Delta=.025f) { ++GFrameCounter; World->Tick(LEVELTICK_All,Delta); }
        void UntilFinished()
        {
            for (int32 Frame=0;Frame<800 && Manager->State!=EMovieState::Finished && Manager->State!=EMovieState::Error;++Frame) { Tick(); }
        }
        void FinishTake()
        {
            const int32 Index = Manager->CurrentTakeIndex;
            for (int32 Frame=0;Frame<150 && Manager->CurrentTakeIndex==Index && Manager->State==EMovieState::Recording;++Frame) { Tick(); }
        }
        void Move(FVector Location,float Delta=.1f) { Pawn->SetActorLocation(Location); Tick(Delta); }
        AMovieManager* Setup(bool bBank=true)
        {
            Player=World->SpawnActor<APlayerController>();
            if (!Player->PlayerCameraManager) { Player->SpawnPlayerCameraManager(); }
            Pawn=World->SpawnActor<APawn>();
            auto* Root=NewObject<USceneComponent>(Pawn); Pawn->AddInstanceComponent(Root); Pawn->SetRootComponent(Root); Root->RegisterComponent();
            Player->Possess(Pawn); Player->SetViewTarget(Pawn);
            Manager=World->SpawnActor<AMovieManager>();
            Manager->PreparationDurationSeconds=.05; Manager->RecordingDurationSeconds=1.0;
            Manager->bShowDebugOverlay=false; Manager->bBindActingKeys=false; Manager->bBindRestartKey=false; Manager->bBindInteractionKey=false;
            if (bBank)
            {
                Entry=Zone(TEXT("Bank.Entry"),FVector(200,0,0),FVector(65,300,100));
                Escape=Zone(TEXT("Bank.Escape"),FVector(600,0,0),FVector(60,60,100));
                Counter=Zone(TEXT("Bank.Counter"),FVector(300,-180,0),FVector(40,40,100));
                Money=World->SpawnActor<AMoneyBagProp>(FVector(200,0,0),FRotator::ZeroRotator);
                Donut=World->SpawnActor<ADonutProp>(FVector(200,180,0),FRotator::ZeroRotator);
                Banana=World->SpawnActor<ABananaProp>(FVector(400,-180,0),FRotator::ZeroRotator);
            }
            return Manager;
        }
        AObjectiveZone* Zone(FName Tag,FVector Location,FVector Extent)
        {
            auto* Result=World->SpawnActor<AObjectiveZone>(Location,FRotator::ZeroRotator);
            Result->ZoneTag=Tag; Result->Bounds->SetBoxExtent(Extent); Result->RefreshMarker(); return Result;
        }
        bool Ready() { return Manager->ReadyForTake(Manager->PreparationId); }
        UWorld* World=nullptr; APlayerController* Player=nullptr; APawn* Pawn=nullptr; AMovieManager* Manager=nullptr;
        AObjectiveZone* Entry=nullptr; AObjectiveZone* Escape=nullptr; AObjectiveZone* Counter=nullptr;
        AMoneyBagProp* Money=nullptr; ADonutProp* Donut=nullptr; ABananaProp* Banana=nullptr;
    };
    FMovieObjectiveDefinition Action(FName Id,int32 Points,EMovieObjectiveType Type,FName EventId)
    {
        FMovieObjectiveDefinition Result; Result.Id=Id; Result.DisplayName=FText::FromName(Id);
        Result.Points=Points; Result.Type=Type; Result.RequiredEventId=EventId; return Result;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FObjectiveDefinitionTest,"OneManStudio.Objectives.DefinitionsAndValidation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FObjectiveDefinitionTest::RunTest(const FString& Parameters)
{
    FText Error;
    for (int32 Role=0;Role<3;++Role)
    {
        const auto Definitions=MovieObjectiveDefaults::BankHeist(Role,TEXT("Robber"),TEXT("PoliceOfficer"));
        TestTrue(TEXT("Default definitions valid"),UObjectiveManagerComponent::ValidateDefinitions(Definitions,Error));
        TestEqual(TEXT("Five challenges per role"),Definitions.Num(),5);
        int32 Total=0; for (const auto& Item:Definitions) { Total+=Item.Points; }
        TestEqual(TEXT("Default role has exactly 100 available points"),Total,100);
    }
    auto Definitions=MovieObjectiveDefaults::BankHeist(0,TEXT("Robber"),TEXT("PoliceOfficer"));
    Definitions[0].PrerequisiteId=Definitions[2].Id;
    TestFalse(TEXT("Prerequisite cycle rejected"),UObjectiveManagerComponent::ValidateDefinitions(Definitions,Error));
    Definitions[0].PrerequisiteId=TEXT("Missing");
    TestFalse(TEXT("Missing prerequisite rejected"),UObjectiveManagerComponent::ValidateDefinitions(Definitions,Error));
    Definitions[0].PrerequisiteId=NAME_None; Definitions[1].Id=Definitions[0].Id;
    TestFalse(TEXT("Duplicate objective IDs rejected"),UObjectiveManagerComponent::ValidateDefinitions(Definitions,Error));
    FActingEvent Event; Event.Type=EActingEventType::CollectProp; Event.Identifier=TEXT("Collect");
    TestFalse(TEXT("Incomplete interaction payload rejected"),Event.HasValidPayload());
    Event.PropId=TEXT("Prop.MoneyBag"); Event.TargetId=TEXT("TestPickup");
    for (const auto& Item:WardrobeDefaults::MakeCatalog()) { if (Item.Id==Event.PropId) { Event.PropVisual=Item; } }
    TestTrue(TEXT("Interaction retains a valid asset/settings snapshot"),Event.HasValidPayload());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRobberObjectiveTest,"OneManStudio.Objectives.RobberRules",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRobberObjectiveTest::RunTest(const FString& Parameters)
{
    ObjectiveTests::FWorld Context; auto* Movie=Context.Setup(); Movie->StartMovie();
    TestEqual(TEXT("Objectives are available during wardrobe"),Movie->ObjectiveManager->Objectives.Num(),5);
    TestFalse(TEXT("No scoring during wardrobe"),Movie->ObjectiveManager->IsScoringActive());
    TestFalse(TEXT("No interaction during wardrobe"),Movie->LivePropInteraction->CollectProp(Context.Money));
    Movie->SelectWardrobeItem(Movie->PreparationId,ECosmeticCategory::Prop,NAME_None); Context.Ready();
    TestFalse(TEXT("Cannot collect from outside interaction range"),Movie->LivePropInteraction->CollectProp(Context.Money));
    TestTrue(TEXT("Gesture accepted without asset assignment"),Movie->LiveActingComponent->TriggerGesture(EActingGesture::Dance));
    TestEqual(TEXT("Dance without money and prerequisite never awards"),Movie->GetCurrentScore(),0);
    Context.Move(FVector(200,0,0)); TestEqual(TEXT("Entering bank awards 25"),Movie->GetCurrentScore(),25);
    TestTrue(TEXT("E path collects nearby money bag"),Movie->LivePropInteraction->Interact());
    TestEqual(TEXT("Collection awards 25 once"),Movie->GetCurrentScore(),50);
    TestEqual(TEXT("Collected prop is equipped"),Movie->LivePropInteraction->EquippedPropId,FName(TEXT("Prop.MoneyBag")));
    TestFalse(TEXT("Same world pickup cannot be collected twice"),Movie->LivePropInteraction->CollectProp(Context.Money));
    Movie->LiveActingComponent->TriggerGesture(EActingGesture::Dance);
    TestEqual(TEXT("Dance with money awards 15"),Movie->GetCurrentScore(),65);
    Movie->LiveActingComponent->TriggerGesture(EActingGesture::Dance);
    TestEqual(TEXT("Repeated dance cannot award twice"),Movie->GetCurrentScore(),65);
    Movie->LiveActingComponent->TriggerSound(EActingSound::EvilLaugh);
    Context.Move(FVector(600,0,0)); TestEqual(TEXT("Robber reaches 100"),Movie->GetCurrentScore(),100);
    Context.FinishTake();
    TestEqual(TEXT("Final role score retained in performance"),Movie->Performances[0].ObjectiveResult.Score,100);
    TestEqual(TEXT("Completed IDs retained in performance"),Movie->Performances[0].ObjectiveResult.CompletedObjectiveIds.Num(),5);
    TestEqual(TEXT("New role starts at zero"),Movie->GetCurrentScore(),0);
    TestTrue(TEXT("World prop reset for next performer"),Context.Money->IsAvailable());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAllRolesObjectiveTest,"OneManStudio.Objectives.AllRolesAndScores",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAllRolesObjectiveTest::RunTest(const FString& Parameters)
{
    ObjectiveTests::FWorld Context; auto* Movie=Context.Setup();
    for (auto& Item:Movie->CosmeticCatalog) { if (Item.Id==TEXT("Prop.Donut")) { Item.UseGestureId=TEXT("EatDonut"); } }
    Movie->StartMovie();
    FActingGestureDefinition Eat; Eat.Identifier=TEXT("EatDonut"); Eat.DurationSeconds=1.0;
    Movie->LiveActingComponent->Gestures.Add(Eat); Context.Ready();
    Context.Move(FVector(200,0,0)); Movie->LivePropInteraction->CollectProp(Context.Money);
    Movie->LiveActingComponent->TriggerGesture(EActingGesture::Dance); Movie->LiveActingComponent->TriggerSound(EActingSound::EvilLaugh);
    Context.Tick(.4f); // Remain in the bank while the other roles act at scene times 0.1-0.15.
    Context.Move(FVector(600,0,0),.05f); Context.FinishTake(); Context.Ready();
    Context.Move(FVector(200,0,0));
    TestEqual(TEXT("Police enters bank and approaches zero-independent recorded role"),Movie->GetCurrentScore(),50);
    Movie->LiveActingComponent->TriggerGesture(EActingGesture::Arrest);
    TestTrue(TEXT("Police uses its wardrobe Donut"),Movie->LivePropInteraction->UseEquippedProp());
    TestEqual(TEXT("Using a prop can present its configured gesture"),Movie->LiveActingComponent->ActiveGestureId,FName(TEXT("EatDonut")));
    Movie->LiveActingComponent->TriggerSound(EActingSound::PoliceSiren);
    TestEqual(TEXT("Police earns 100"),Movie->GetCurrentScore(),100);
    Context.FinishTake(); Context.Ready();
    const auto& PoliceEvents=Movie->Performances[1].Recording.Events;
    TestEqual(TEXT("Use presentation records only one logical interaction"),PoliceEvents.FilterByPredicate([](const FActingEvent& Event) { return Event.Type==EActingEventType::UseProp; }).Num(),1);
    TestFalse(TEXT("Use presentation does not record a duplicate gesture"),PoliceEvents.ContainsByPredicate([](const FActingEvent& Event) { return Event.Identifier==TEXT("EatDonut"); }));
    Context.Move(FVector(300,-180,0),.05f); Context.Move(FVector(200,0,0));
    Movie->LiveActingComponent->TriggerGesture(EActingGesture::Scared);
    Movie->LiveActingComponent->TriggerGesture(EActingGesture::Dance);
    Movie->LiveActingComponent->TriggerSound(EActingSound::Scream);
    TestEqual(TEXT("Police replay presents the saved use gesture"),Movie->FindReplayForRole(Movie->Roles[1].RoleId)->Acting->ActiveGestureId,FName(TEXT("EatDonut")));
    TestEqual(TEXT("Customer earns 100 through its five challenges"),Movie->GetCurrentScore(),100);
    Context.UntilFinished();
    TestTrue(TEXT("Scoring does not interfere with premiere completion"),Movie->State==EMovieState::Finished);
    TestEqual(TEXT("All three role scores retained"),Movie->ScoreSummary.RoleScores.Num(),3);
    TestTrue(TEXT("Movie average is 100"),FMath::IsNearlyEqual(Movie->ScoreSummary.FinalMovieScore,100.0));
    for (const auto& Performance:Movie->Performances)
    {
        TestEqual(TEXT("Every role score saved"),Performance.ObjectiveResult.Score,100);
        TestEqual(TEXT("Every role completed five objective IDs"),Performance.ObjectiveResult.CompletedObjectiveIds.Num(),5);
        TestTrue(TEXT("Movement/outfit/events remain valid"),Performance.Recording.IsValid());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FObjectiveProximityTest,"OneManStudio.Objectives.ProximityAtActionTime",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FObjectiveProximityTest::RunTest(const FString& Parameters)
{
    ObjectiveTests::FWorld Context; auto* Movie=Context.Setup(false); Movie->bUseDefaultHeistObjectivesIfEmpty=false;
    Movie->Roles[0].Objectives.Reset();
    auto Approach=ObjectiveTests::Action(TEXT("Approach"),25,EMovieObjectiveType::ReachActor,NAME_None);
    Approach.TargetRoleId=Movie->Roles[0].RoleId; Approach.MaxInteractionDistance=200;
    auto Arrest=ObjectiveTests::Action(TEXT("Arrest"),25,EMovieObjectiveType::ActionNearActor,TEXT("Arrest"));
    Arrest.TargetRoleId=Movie->Roles[0].RoleId; Arrest.MaxInteractionDistance=200; Arrest.PrerequisiteId=Approach.Id;
    Movie->Roles[1].Objectives={Approach,Arrest};
    Movie->Roles[1].InitialTransform.SetLocation(FVector(1000,0,0));
    Movie->StartMovie(); Context.Ready();
    Context.Move(FVector(100,0,0)); Context.FinishTake();
    TestEqual(TEXT("Previous character may score zero"),Movie->Performances[0].ObjectiveResult.Score,0);
    Context.Ready(); Context.Move(FVector(1000,0,0)); Movie->LiveActingComponent->TriggerGesture(EActingGesture::Arrest);
    TestEqual(TEXT("An arrest from too far does not award or cache completion"),Movie->GetCurrentScore(),0);
    AReplayActor* Robber=Movie->FindReplayForRole(Movie->Roles[0].RoleId);
    if (!TestNotNull(TEXT("Zero-score Robber still exists as a target"),Robber)) { return false; }
    Robber->SetActorLocation(FVector(10000,0,0)); // simulate the visual actor having a stale transform this frame
    Context.Pawn->SetActorLocation(FVector(100,0,0)); Movie->LiveActingComponent->TriggerGesture(EActingGesture::Arrest);
    TestEqual(TEXT("Action queries recorded target pose at event time"),Movie->GetCurrentScore(),50);
    TestTrue(TEXT("Theatrical arrest never changes the target actor"),Robber->GetActorLocation().Equals(FVector(10000,0,0)) && Robber->IsPlaying());
    Movie->LiveActingComponent->TriggerGesture(EActingGesture::Arrest); TestEqual(TEXT("Repeated arrest does not award"),Movie->GetCurrentScore(),50);
    TestEqual(TEXT("Replay score never changes"),Movie->Performances[0].ObjectiveResult.Score,0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPropReplayObjectiveTest,"OneManStudio.Objectives.PropReplayAndReset",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPropReplayObjectiveTest::RunTest(const FString& Parameters)
{
    ObjectiveTests::FWorld Context; auto* Movie=Context.Setup(); Movie->StartMovie();
    Movie->SelectWardrobeItem(Movie->PreparationId,ECosmeticCategory::Prop,NAME_None); Context.Ready(); Context.Move(FVector(200,0,0));
    Movie->LivePropInteraction->CollectProp(Context.Money);
    UActingRecorderComponent* Recorder=Context.Pawn->FindComponentByClass<UActingRecorderComponent>();
    const auto& Events=Recorder->GetRecordingData().Events;
    TestEqual(TEXT("Pickup recorded once on acting timeline"),Events.Num(),1);
    TestTrue(TEXT("Pickup timestamp uses the shared clock"),FMath::IsNearlyEqual(Events[0].TimeSeconds,Movie->GetSceneTimeSeconds()));
    const UStaticMesh* FrozenMesh=Events[0].PropVisual.StaticMesh;
    Context.FinishTake(); Context.Ready();
    for (auto& Item:Movie->LiveWardrobeComponent->Catalog) { if (Item.Id==TEXT("Prop.MoneyBag")) { Item.StaticMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")); } }
    Context.Tick(.2f);
    AReplayActor* Replay=Movie->ActiveReplays[0];
    TestEqual(TEXT("Replay equips its recorded pickup"),Replay->PropInteraction->EquippedPropId,FName(TEXT("Prop.MoneyBag")));
    TestEqual(TEXT("Live player's inventory remains its own Donut"),Movie->LivePropInteraction->EquippedPropId,FName(TEXT("Prop.Donut")));
    TestTrue(TEXT("Replay cannot consume the live world's pickup"),Context.Money->IsAvailable());
    TestEqual(TEXT("Replay interactions cannot score for current player"),Movie->GetCurrentScore(),0);
    TestEqual(TEXT("Recorded character score stays fixed"),Movie->Performances[0].ObjectiveResult.Score,50);
    bool bFrozenVisual=false;
    for (UMeshComponent* Mesh:Replay->Wardrobe->CosmeticMeshes)
    { if (const auto* Static=Cast<UStaticMeshComponent>(Mesh); Static && Static->ComponentHasTag(TEXT("Prop.MoneyBag"))) { bFrozenVisual=Static->GetStaticMesh()==FrozenMesh; } }
    TestTrue(TEXT("Replay uses captured mesh, not the edited live catalog"),bFrozenVisual);
    TestFalse(TEXT("Replay cannot use interaction input"),Replay->PropInteraction->Interact());
    Replay->StopPlayback(true);
    TestTrue(TEXT("Returning a replay to its first pose restores initial prop visuals and inventory"),Replay->Wardrobe->GetSelection().PropId.IsNone() && Replay->PropInteraction->EquippedPropId.IsNone());
    Context.Money->SetActorLocation(FVector(9000,0,0)); Context.FinishTake();
    TestTrue(TEXT("Every new take restores prop initial transform"),Context.Money->GetActorLocation().Equals(FVector(200,0,0)));
    TestFalse(TEXT("Use cannot fire during preparation"),Movie->LivePropInteraction->UseEquippedProp());
    Movie->RestartMovie(); TestTrue(TEXT("Restart restores source props"),Context.Money->IsAvailable());
    TestTrue(TEXT("Restart clears interaction events and all saved scores"),Movie->Performances.IsEmpty() && Recorder->GetRecordingData().Events.IsEmpty() && Movie->ScoreSummary.RoleScores.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FObjectiveScoreMathTest,"OneManStudio.Objectives.ZeroAverageAndCap",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FObjectiveScoreMathTest::RunTest(const FString& Parameters)
{
    {
        ObjectiveTests::FWorld Context; auto* Movie=Context.Setup(false); Movie->StartMovie(); Context.UntilFinished();
        TestTrue(TEXT("Movie continues through three zero-score roles"),Movie->State==EMovieState::Finished);
        TestEqual(TEXT("Zero-scored takes still retained"),Movie->Performances.Num(),3);
        TestTrue(TEXT("Zero average is exact"),Movie->ScoreSummary.FinalMovieScore==0.0);
    }
    {
        ObjectiveTests::FWorld Context; auto* Movie=Context.Setup(false); const int32 Scores[]={100,50,25};
        for (int32 Index=0;Index<3;++Index) { Movie->Roles[Index].Objectives={ObjectiveTests::Action(FName(*FString::Printf(TEXT("Sound%d"),Index)),Scores[Index],EMovieObjectiveType::PlaySound,TEXT("EvilLaugh"))}; }
        Movie->StartMovie();
        for (int32 Index=0;Index<3;++Index) { Context.Ready(); Movie->LiveActingComponent->TriggerSound(EActingSound::EvilLaugh); Context.FinishTake(); }
        TestTrue(TEXT("Movie score averages all three, including fractions"),FMath::IsNearlyEqual(Movie->ScoreSummary.FinalMovieScore,175.0/3.0,.0001));
        for (int32 Index=0;Index<3;++Index) { TestEqual(TEXT("Individual score query returns retained score"),Movie->GetScoreForRole(Movie->Roles[Index].RoleId),Scores[Index]); }
    }
    {
        ObjectiveTests::FWorld Context; auto* Movie=Context.Setup(false); Movie->Roles[0].Objectives.Reset();
        for (int32 Index=0;Index<5;++Index) { Movie->Roles[0].Objectives.Add(ObjectiveTests::Action(FName(*FString::Printf(TEXT("Gesture%d"),Index)),30,EMovieObjectiveType::PerformGesture,TEXT("Dance"))); }
        Movie->StartMovie(); Context.Ready(); Movie->LiveActingComponent->TriggerGesture(EActingGesture::Dance);
        TestEqual(TEXT("A custom oversized budget can never exceed 100"),Movie->GetCurrentScore(),100);
        TestEqual(TEXT("Completion remains once per objective even after cap"),Movie->ObjectiveManager->GetResult().CompletedObjectiveIds.Num(),5);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FObjectiveHUDTest,"OneManStudio.Objectives.NativeHUD",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FObjectiveHUDTest::RunTest(const FString& Parameters)
{
    ObjectiveTests::FWorld Context; auto* Movie=Context.Setup(); Movie->StartMovie();
    auto* HUD=NewObject<UObjectiveHUDWidget>(Context.World); HUD->Initialize(); HUD->BindMovie(Movie);
    TestNotNull(TEXT("Functional native objective UMG tree exists"),HUD->GetRootWidget());
    TestTrue(TEXT("Wardrobe HUD previews objectives without scoring"),HUD->ViewData.bPreparation && HUD->ViewData.Objectives.Num()==5 && HUD->ViewData.Score==0);
    TestEqual(TEXT("Role displayed"),HUD->ViewData.RoleName.ToString(),FString(TEXT("Robber")));
    Context.Ready(); Context.Move(FVector(200,0,0)); HUD->RefreshObjectives();
    TestEqual(TEXT("HUD updates awarded points"),HUD->ViewData.Score,25);
    TestTrue(TEXT("HUD exposes completed indicator"),HUD->ViewData.Objectives[0].bCompleted && HUD->ViewData.Objectives[0].PointsAwarded==25);
    TestFalse(TEXT("Completion feedback exposed"),HUD->ViewData.CompletionFeedback.IsEmpty());
    TestTrue(TEXT("HUD uses recording timer"),HUD->ViewData.RemainingSeconds < 1.0 && !HUD->ViewData.bPreparation);
    TestFalse(TEXT("HUD exposes E interaction prompt"),HUD->ViewData.InteractionPrompt.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FObjectiveZoneConfigurationTest,"OneManStudio.Objectives.ZonesAndBadConfiguration",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FObjectiveZoneConfigurationTest::RunTest(const FString& Parameters)
{
    ObjectiveTests::FWorld Context; auto* Movie=Context.Setup(false);
    auto* Zone=Context.Zone(TEXT("Rotated"),FVector(500,200,0),FVector(100,20,80)); Zone->SetActorRotation(FRotator(0,90,0));
    TestTrue(TEXT("Rotated zone uses local bounds"),Zone->ContainsPoint(FVector(500,280,0)));
    TestFalse(TEXT("Outside rotated zone rejected"),Zone->ContainsPoint(FVector(580,200,0)));
    auto Reach=ObjectiveTests::Action(TEXT("Reach"),100,EMovieObjectiveType::ReachZone,NAME_None); Reach.TargetActor=Zone;
    Movie->Roles[0].Objectives={Reach}; Movie->StartMovie(); Context.Ready(); Context.Move(FVector(500,280,0));
    TestEqual(TEXT("Direct zone actor reference supported"),Movie->GetCurrentScore(),100);
    Movie->StopMovie();
    Movie->Roles[0].Objectives={Reach,Reach};
    TestTrue(TEXT("Invalid objective config cannot block movie start"),Movie->StartMovie());
    TestFalse(TEXT("Invalid config exposed to UI"),Movie->ObjectiveManager->ConfigurationIssue.IsEmpty());
    Context.UntilFinished(); TestTrue(TEXT("Invalid objectives still allow full movie"),Movie->State==EMovieState::Finished);
    return true;
}
#endif
