#if WITH_DEV_AUTOMATION_TESTS
#include "GrabbablePropComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "CoreGlobals.h"
namespace GrabbableTests
{
struct FWorld
{
    UWorld* World;AStaticMeshActor* Prop;AActor* Holder;UGrabbablePropComponent* Component;
    FWorld()
    {
        UWorld::InitializationValues Values;Values.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).ShouldSimulatePhysics(true).SetTransactional(false);
        World=UWorld::CreateWorld(EWorldType::Game,false,MakeUniqueObjectName(GetTransientPackage(),UWorld::StaticClass(),TEXT("GrabbableWorld")),nullptr,true,ERHIFeatureLevel::Num,&Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);World->InitializeActorsForPlay(FURL());World->GetWorldSettings()->NotifyBeginPlay();
        Prop=World->SpawnActor<AStaticMeshActor>();Prop->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);Prop->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        Prop->SetActorTransform(FTransform(FQuat::Identity,FVector(0,0,200),FVector(.4)));
        Holder=World->SpawnActor<AActor>();auto* Root=NewObject<USceneComponent>(Holder);Holder->AddInstanceComponent(Root);Holder->SetRootComponent(Root);Root->RegisterComponent();Holder->SetActorLocation(FVector(50,0,100));
        Component=NewObject<UGrabbablePropComponent>(Prop);Prop->AddInstanceComponent(Component);Component->RegisterComponent();Component->ObjectId=TEXT("Test.Cube");
    }
    void Tick(float Time=.05f){++GFrameCounter;World->Tick(LEVELTICK_All,Time);}
    ~FWorld(){World->EndPlay(EEndPlayReason::Quit);World->DestroyWorld(false);GEngine->DestroyWorldContext(World);}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrabbableOwnershipPresentationTest,"OneManStudio.PhysicalProps.HoldPresentationAndRestore",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGrabbableOwnershipPresentationTest::RunTest(const FString& Parameters)
{
    GrabbableTests::FWorld C;USceneComponent* OriginalRoot=C.Prop->GetRootComponent();const FTransform Original=C.Prop->GetActorTransform();
    if(!TestTrue(TEXT("Existing mesh gains simple body"),C.Component->InitializeBody())){return false;}
    TestTrue(TEXT("Original actor identity preserved"),C.Prop->GetRootComponent()==C.Component->PhysicsBody);
    TestTrue(TEXT("Grab succeeds"),C.Component->Hold(C.Holder,false));
    TestTrue(TEXT("Held prop attaches to holder"),C.Component->PhysicsBody->GetAttachParent()==C.Holder->GetRootComponent());
    TestTrue(TEXT("Held prop cannot interfere physically"),!C.Component->PhysicsBody->IsSimulatingPhysics() && C.Component->PhysicsBody->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
    const FVector Before=C.Prop->GetActorLocation();C.Holder->AddActorWorldOffset(FVector(100,0,0));
    TestTrue(TEXT("Carry follows holder"),C.Prop->GetActorLocation().Equals(Before+FVector(100,0,0),.01));
    const FTransform Recorded(FQuat::Identity,FVector(300,120,220));C.Component->PresentWorld(Recorded);C.Tick();
    TestTrue(TEXT("Playback uses recorded pose without simulation"),C.Prop->GetActorTransform().Equals(Recorded,.01) && !C.Component->PhysicsBody->IsSimulatingPhysics());
    C.Component->RestoreOriginal();
    TestTrue(TEXT("Original root restored"),C.Prop->GetRootComponent()==OriginalRoot);
    TestTrue(TEXT("Original transform restored"),C.Prop->GetActorTransform().Equals(Original,.01));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrabbablePhysicsPauseTest,"OneManStudio.PhysicalProps.ThrowAndPause",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGrabbablePhysicsPauseTest::RunTest(const FString& Parameters)
{
    GrabbableTests::FWorld C;if(!TestTrue(TEXT("Physical test holder initializes"),C.Component->Hold(C.Holder,false))){return false;}
    TestTrue(TEXT("Configured throw releases body"),C.Component->Release(C.Holder,true,FVector(1,0,0),0));
    const FVector Start=C.Prop->GetActorLocation();C.Tick();C.Tick();
    TestTrue(TEXT("Physics moves thrown object forward"),C.Prop->GetActorLocation().X>Start.X+10);
    C.Component->SetScenePaused(true);const FTransform Paused=C.Prop->GetActorTransform();C.Tick(.1f);
    TestTrue(TEXT("Wardrobe pause freezes transform"),C.Prop->GetActorTransform().Equals(Paused,.01));
    C.Component->SetScenePaused(false);C.Tick();C.Tick();
    TestTrue(TEXT("Resume preserves motion"),C.Prop->GetActorLocation().X>Paused.GetLocation().X+10);
    C.Component->bCanThrow=false;C.Component->Hold(C.Holder,false);
    TestFalse(TEXT("Per-item throw restriction respected"),C.Component->Release(C.Holder,true,FVector(1,0,0),1));
    return true;
}
#endif
