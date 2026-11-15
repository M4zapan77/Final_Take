#if WITH_DEV_AUTOMATION_TESTS
#include "StudioBankBounds.h"
#include "StudioSetDoor.h"
#include "StorySessionComponent.h"
#include "MovieManager.h"
#include "ActingSceneClockSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "CoreGlobals.h"
#include "Misc/AutomationTest.h"
namespace BankSafetyTests
{
struct FWorld
{
    UWorld* World;APlayerController* PC;APawn* Pawn;AMovieManager* Movie;
    FWorld()
    {
        UWorld::InitializationValues V;V.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
        World=UWorld::CreateWorld(EWorldType::Game,false,MakeUniqueObjectName(GetTransientPackage(),UWorld::StaticClass(),TEXT("BankSafetyWorld")),nullptr,true,ERHIFeatureLevel::Num,&V);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);World->InitializeActorsForPlay(FURL());World->GetWorldSettings()->NotifyBeginPlay();
        PC=World->SpawnActor<APlayerController>();PC->SpawnPlayerCameraManager();Pawn=World->SpawnActor<APawn>();auto* Root=NewObject<USceneComponent>(Pawn);Pawn->AddInstanceComponent(Root);Pawn->SetRootComponent(Root);Root->RegisterComponent();PC->Possess(Pawn);PC->SetViewTarget(Pawn);
        Movie=World->SpawnActor<AMovieManager>();Movie->bUseChronologicalGameplay=true;Movie->bShowWardrobeUI=false;Movie->bShowObjectiveHUD=false;Movie->bShowMoviePresentationUI=false;Movie->bShowDebugOverlay=false;Movie->bUseTakeTransitions=false;Movie->StoryTurnDurationSeconds=2;Movie->StoryWardrobeDurationSeconds=2;Movie->CharacterProfile=nullptr;Movie->AudioProfile=nullptr;
    }
    void Tick(float D=.05f){++GFrameCounter;World->Tick(LEVELTICK_All,D);}
    ~FWorld(){World->EndPlay(EEndPlayReason::Quit);World->DestroyWorld(false);GEngine->DestroyWorldContext(World);}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBankPerimeterTest,"OneManStudio.FinalBank.InvisiblePerimeter",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBankPerimeterTest::RunTest(const FString&)
{
    BankSafetyTests::FWorld C;auto* B=C.World->SpawnActor<AStudioBankBounds>();B->OnConstruction(B->GetActorTransform());
    for(FVector Point:{FVector(1200,0,100),FVector(-1200,0,100),FVector(0,1300,100),FVector(0,-1300,100),FVector(0,0,-500)}){TestTrue(TEXT("Every unsafe edge and fall is detected"),B->NeedsRecovery(Point));}
    TestFalse(TEXT("Lobby is valid"),B->NeedsRecovery(FVector(0,0,100)));
    FHitResult Hit;FCollisionQueryParams Q;
    TestTrue(TEXT("Perimeter blocks players"),C.World->SweepSingleByChannel(Hit,FVector(800,0,100),FVector(1300,0,100),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,96),Q));
    TestFalse(TEXT("Invisible perimeter never catches a camera probe"),C.World->SweepSingleByChannel(Hit,FVector(800,0,100),FVector(1300,0,100),FQuat::Identity,ECC_Camera,FCollisionShape::MakeSphere(12),Q));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBankRecoveryTest,"OneManStudio.FinalBank.RecoverSameActor",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBankRecoveryTest::RunTest(const FString&)
{
    BankSafetyTests::FWorld C;C.World->SpawnActor<AStudioBankBounds>();TestTrue(TEXT("Start"),C.Movie->StartMovie());C.Movie->ReadyForTake(C.Movie->PreparationId);C.Tick();
    APawn* Same=C.Movie->LivePerformer;const FVector Safe=Same->GetActorLocation();const int32 Turn=C.Movie->GetCurrentTakeNumber();
    Same->SetActorLocation(FVector(Safe.X,Safe.Y,-1500));C.Tick();
    TestTrue(TEXT("Recovered existing actor to its safe pose"),C.Movie->LivePerformer==Same && Same->GetActorLocation().Equals(Safe,1));
    TestEqual(TEXT("Recovery does not restart the turn"),C.Movie->GetCurrentTakeNumber(),Turn);
    TestTrue(TEXT("Recovery keeps recording"),C.Movie->State==EMovieState::Recording);
    TestTrue(TEXT("Recovery is recorded on the scene timeline"),C.Movie->StorySession->SceneEvents.ContainsByPredicate([](const auto& E){return E.Type==EStoryEventType::ActorRecovered;}));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBankDoorTest,"OneManStudio.FinalBank.ClockDoorPersistence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBankDoorTest::RunTest(const FString&)
{
    BankSafetyTests::FWorld C;auto* Door=C.World->SpawnActor<AStudioSetDoor>();Door->SetActorLocation(FVector(500,0,0));
    TestTrue(TEXT("Start"),C.Movie->StartMovie());C.Movie->ReadyForTake(C.Movie->PreparationId);
    TestTrue(TEXT("E route opens the vault"),C.Movie->StorySession->InteractWith(Door->DoorId));C.Tick(.15f);
    const FQuat Pose=Door->Hinge->GetComponentQuat();TestTrue(TEXT("Door animates"),!Pose.Equals(FQuat::Identity));
    TestTrue(TEXT("Door state is authoritative"),C.Movie->StorySession->SceneState.DoorOpen.FindRef(Door->DoorId));
    auto* Clock=C.World->GetSubsystem<UActingSceneClockSubsystem>();Clock->PauseClock();C.Tick(.15f);TestTrue(TEXT("Preparation/pause cannot advance door motion"),Door->Hinge->GetComponentQuat().Equals(Pose));Clock->ResumeClock();
    TestTrue(TEXT("One door event is recorded"),C.Movie->StorySession->SceneEvents.ContainsByPredicate([](const auto& E){return E.Type==EStoryEventType::DoorChanged && E.Payload==TEXT("Open");}));
    C.Movie->StopMovie();TestFalse(TEXT("New session reset closes old door"),Door->bOpen);return true;
}
#endif
