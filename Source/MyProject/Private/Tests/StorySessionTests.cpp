#if WITH_DEV_AUTOMATION_TESTS
#include "StorySessionComponent.h"
#include "MovieManager.h"
#include "ActingSceneClockSubsystem.h"
#include "ActingRecorderComponent.h"
#include "WardrobeComponent.h"
#include "ObjectiveManagerComponent.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Components/SceneComponent.h"
#include "CoreGlobals.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

namespace StorySessionTests
{
struct FWorld
{
    UWorld* World=nullptr; APlayerController* Player=nullptr; APawn* Pawn=nullptr; AMovieManager* Movie=nullptr;
    FWorld()
    {
        UWorld::InitializationValues Values;
        Values.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
        World=UWorld::CreateWorld(EWorldType::Game,false,MakeUniqueObjectName(GetTransientPackage(),UWorld::StaticClass(),TEXT("StoryTestWorld")),nullptr,true,ERHIFeatureLevel::Num,&Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World); World->InitializeActorsForPlay(FURL()); World->GetWorldSettings()->NotifyBeginPlay();
        Player=World->SpawnActor<APlayerController>(); if(!Player->PlayerCameraManager){Player->SpawnPlayerCameraManager();}
        Pawn=World->SpawnActor<APawn>(); auto* Root=NewObject<USceneComponent>(Pawn); Pawn->AddInstanceComponent(Root); Pawn->SetRootComponent(Root); Root->RegisterComponent();
        Player->Possess(Pawn); Player->SetViewTarget(Pawn);
        Movie=World->SpawnActor<AMovieManager>(); Movie->bUseChronologicalGameplay=true;
        Movie->StoryWardrobeDurationSeconds=.2; Movie->StoryTurnDurationSeconds=.3;
        Movie->bUseTakeTransitions=false; Movie->bShowWardrobeUI=false; Movie->bUseWardrobePreviewCamera=false;
        Movie->bShowObjectiveHUD=false; Movie->bShowMoviePresentationUI=false; Movie->bShowDebugOverlay=false;
        Movie->CharacterProfile=nullptr; Movie->AudioProfile=nullptr;
    }
    void Tick(float Seconds=.025f){++GFrameCounter;World->Tick(LEVELTICK_All,Seconds);}
    ~FWorld(){World->EndPlay(EEndPlayReason::Quit);World->DestroyWorld(false);GEngine->DestroyWorldContext(World);}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryPersistentTurnsTest,"OneManStudio.Story.SixPersistentTurns",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStoryPersistentTurnsTest::RunTest(const FString& Parameters)
{
    StorySessionTests::FWorld C;
    TestTrue(TEXT("Story starts"),C.Movie->StartMovie());
    if(!C.Movie->IsChronologicalMovie() || C.Movie->StorySession->Cast.Num()!=3){AddError(TEXT("Three persistent actors were not initialized"));return false;}
    TestEqual(TEXT("Exactly six turns"),C.Movie->GetNumberOfTakes(),6);
    APawn* Actors[3]={C.Movie->StorySession->Cast[0].Pawn,C.Movie->StorySession->Cast[1].Pawn,C.Movie->StorySession->Cast[2].Pawn};
    FVector EndPositions[3]; int32 Wardrobes=0;
    for(int32 Turn=0;Turn<6;++Turn)
    {
        TestEqual(TEXT("No skipped turn"),C.Movie->GetCurrentTakeNumber(),Turn+1);
        TestEqual(TEXT("Two rounds only"),C.Movie->GetRoundNumber(),Turn/3+1);
        const int32 Role=Turn%3;
        TestTrue(TEXT("Possession follows role order"),C.Player->GetPawn()==Actors[Role]);
        if(Turn<3)
        {
            TestTrue(TEXT("First role visit shows wardrobe"),C.Movie->IsWardrobeOpen()); ++Wardrobes;
            const double Before=C.Movie->GetSceneTimeSeconds(); C.Tick(.05f); C.Tick(.05f);
            TestEqual(TEXT("Preparation never advances scene time"),C.Movie->GetSceneTimeSeconds(),Before);
            const int32 Id=C.Movie->PreparationId;
            TestTrue(TEXT("Ready starts one turn"),C.Movie->ReadyForTake(Id));
            TestFalse(TEXT("Duplicate Ready is rejected"),C.Movie->ReadyForTake(Id));
        }
        else
        {
            TestFalse(TEXT("Round two skips wardrobe"),C.Movie->IsWardrobeOpen());
            TestTrue(TEXT("Returning actor retained position"),Actors[Role]->GetActorLocation().Equals(EndPositions[Role],.01));
        }
        const FVector Goal=Actors[Role]->GetActorLocation()+FVector(25+Role*5,10,0);
        Actors[Role]->SetActorLocation(Goal); EndPositions[Role]=Goal;
        int32 Frames=0;
        while(C.Movie->GetCurrentTakeNumber()==Turn+1 && C.Movie->State==EMovieState::Recording && Frames++<40){C.Tick();}
        TestTrue(TEXT("Turn completed within budget"),Frames<40);
    }
    TestEqual(TEXT("Only three wardrobes"),Wardrobes,3);
    TestEqual(TEXT("Exactly six segments retained"),C.Movie->StorySession->Segments.Num(),6);
    TestEqual(TEXT("Three continuous actor tracks"),C.Movie->Performances.Num(),3);
    TestTrue(TEXT("Timeline concatenates six turns"),FMath::IsNearlyEqual(C.Movie->StorySession->GetTimelineDuration(),1.8,1.e-6));
    for(const auto& Film:C.Movie->Performances){TestTrue(TEXT("Assembled recording is valid"),Film.Recording.IsValid());TestTrue(TEXT("All actor tracks share final time"),FMath::IsNearlyEqual(Film.Recording.GetDurationSeconds(),1.8,1.e-6));}
    TestTrue(TEXT("Existing premiere starts"),C.Movie->State==EMovieState::MoviePremiere);
    TestFalse(TEXT("Premiere cannot end another turn"),C.Movie->EndCurrentTurn());
    TestTrue(TEXT("New movie starts cleanly"),C.Movie->RestartMovie());
    TestEqual(TEXT("Restart clears all segments"),C.Movie->StorySession->Segments.Num(),0);
    TestEqual(TEXT("Restart retains only new three-person cast"),C.Movie->StorySession->Cast.Num(),3);
    TestTrue(TEXT("Restart returns to first wardrobe"),C.Movie->IsWardrobeOpen() && C.Movie->GetCurrentTakeNumber()==1);
    C.Movie->StopMovie(); TestTrue(TEXT("Original pawn restored"),C.Player->GetPawn()==C.Pawn);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEarlyCutTest,"OneManStudio.Story.EarlyCutAndClock",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStoryEarlyCutTest::RunTest(const FString& Parameters)
{
    StorySessionTests::FWorld C; TestTrue(TEXT("Start"),C.Movie->StartMovie());
    C.Movie->ReadyForTake(C.Movie->PreparationId);
    TestFalse(TEXT("Zero-duration Cut cannot skip a turn"),C.Movie->EndCurrentTurn());
    C.Tick(.08f); TestTrue(TEXT("Early Cut succeeds"),C.Movie->EndCurrentTurn());
    TestFalse(TEXT("Second Cut rejected"),C.Movie->EndCurrentTurn()); C.Tick();
    TestEqual(TEXT("Exactly one segment"),C.Movie->StorySession->Segments.Num(),1);
    TestTrue(TEXT("Actual cut duration preserved"),FMath::IsNearlyEqual(C.Movie->StorySession->Segments[0].EndSeconds,.08,1.e-6));
    const double Before=C.Movie->GetSceneTimeSeconds(); C.Tick(.05f);
    TestTrue(TEXT("Continuous clock pauses between turns"),FMath::IsNearlyEqual(C.Movie->GetSceneTimeSeconds(),Before,1.e-8));
    auto* Clock=C.World->GetSubsystem<UActingSceneClockSubsystem>();
    TestFalse(TEXT("Cannot advance time with PauseClockAt"),Clock->PauseClockAt(100));
    TestEqual(TEXT("Rejected pause leaves time unchanged"),Clock->GetSceneTimeSeconds(),Before);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryChronologyTest,"OneManStudio.Story.ChronologicalAssembly",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStoryChronologyTest::RunTest(const FString& Parameters)
{
    FStoryCastMember Member; Member.ActorId=TEXT("Robber"); TArray<FStorySegment> Segments;
    for(int32 Index=0;Index<2;++Index)
    {
        auto& Segment=Segments.AddDefaulted_GetRef(); Segment.ActorId=Member.ActorId; Segment.StartSeconds=Index==0?0:5; Segment.EndSeconds=Segment.StartSeconds+1;
        Segment.InitialTransform.SetLocation(FVector(Index*100,0,0)); Segment.FinalTransform.SetLocation(FVector((Index+1)*100,0,0));
        auto& First=Segment.Recording.Samples.AddDefaulted_GetRef(); First.WorldTransform=Segment.InitialTransform;
        auto& Last=Segment.Recording.Samples.AddDefaulted_GetRef(); Last.TimeSeconds=1; Last.WorldTransform=Segment.FinalTransform;
        auto& Event=Segment.Recording.Events.AddDefaulted_GetRef(); Event.Type=EActingEventType::Sound; Event.Identifier=TEXT("Scream"); Event.TimeSeconds=.25;
    }
    FActingRecording Film; TestTrue(TEXT("Build valid continuous film"),UStorySessionComponent::AssembleRoleRecording(Member.ActorId,Member,Segments,8,Film));
    TestEqual(TEXT("Each recorded event appears once"),Film.Events.Num(),2);
    TestEqual(TEXT("Second-round event uses global timestamp"),Film.Events[1].TimeSeconds,5.25);
    FTransform Pose; Film.Evaluate(3,Pose); TestTrue(TEXT("Inactive actor stays where left"),Pose.GetLocation().Equals(FVector(100,0,0),.001));
    Film.Evaluate(5.5,Pose); TestTrue(TEXT("Second turn interpolates its own movement"),Pose.GetLocation().Equals(FVector(150,0,0),.001));
    Film.Evaluate(7,Pose); TestTrue(TEXT("Final inactive interval holds final pose"),Pose.GetLocation().Equals(FVector(200,0,0),.001));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryMoneyPersistenceTest,"OneManStudio.Story.MoneyOwnershipAndContext",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStoryMoneyPersistenceTest::RunTest(const FString& Parameters)
{
    StorySessionTests::FWorld C;C.Movie->StoryTurnDurationSeconds=2;
    TestTrue(TEXT("Start contextual scene"),C.Movie->StartMovie());UStorySessionComponent* S=C.Movie->StorySession;
    TestTrue(TEXT("Exactly one money object"),S->SceneState.HasUniqueMoney());
    TestFalse(TEXT("No interactions during wardrobe"),S->InteractWith(TEXT("NPC.Teller")));
    C.Movie->ReadyForTake(C.Movie->PreparationId);C.Tick(.08f);
    C.Movie->LivePerformer->SetActorLocation(S->GetInteractionLocation(TEXT("NPC.Teller"))+FVector(0,140,0));
    TestTrue(TEXT("Teller provides money"),S->InteractWith(TEXT("NPC.Teller")));
    const FName Robber=C.Movie->GetCurrentRoleId();const FVector RobberPosition=C.Movie->LivePerformer->GetActorLocation();
    TestEqual(TEXT("One authoritative robber owner"),S->SceneState.MoneyOwner(),Robber);
    TestTrue(TEXT("Contextual primary completed"),C.Movie->ObjectiveManager->IsObjectiveCompleted(TEXT("Story.Robber.Round1")));
    TestTrue(TEXT("Cut first turn"),C.Movie->EndCurrentTurn());C.Tick();
    C.Movie->ReadyForTake(C.Movie->PreparationId);C.Tick(.08f);
    C.Movie->LivePerformer->SetActorLocation(RobberPosition+FVector(120,0,0));
    TestTrue(TEXT("Police confiscates from inactive robber"),S->InteractWith(Robber));
    const FName Police=C.Movie->GetCurrentRoleId();
    TestEqual(TEXT("Confiscation changes exclusive owner"),S->SceneState.MoneyOwner(),Police);
    TestEqual(TEXT("Confiscation is one protagonist interaction"),S->GetScoreBreakdown().Interactions,10);
    TestTrue(TEXT("Owner may drop money"),S->DropHeldObject());
    TestEqual(TEXT("Dropping own bag does not farm protagonist points"),S->GetScoreBreakdown().Interactions,10);
    TestTrue(TEXT("Money remains recoverable"),S->InteractWith(S->SceneState.MoneyObjectId));
    TestEqual(TEXT("Picking up own drop adds no protagonist points"),S->GetScoreBreakdown().Interactions,10);
    TestTrue(TEXT("Money remains unique"),S->SceneState.HasUniqueMoney());
    TestTrue(TEXT("Cut second"),C.Movie->EndCurrentTurn());C.Tick();
    C.Movie->ReadyForTake(C.Movie->PreparationId);C.Tick(.08f);C.Movie->EndCurrentTurn();C.Tick();
    TestEqual(TEXT("Round two returns to robber"),C.Movie->GetCurrentRoleId(),Robber);
    TestTrue(TEXT("Robber returns in same position"),C.Movie->LivePerformer->GetActorLocation().Equals(RobberPosition,.001));
    TestEqual(TEXT("Lost inventory was not restored"),S->SceneState.MoneyOwner(),Police);
    TestTrue(TEXT("Current objective reacts to lost money"),!S->GetPrimaryObjectiveText().IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryOutcomesTest,"OneManStudio.Story.OutcomesAndNoActionFilm",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStoryOutcomesTest::RunTest(const FString& Parameters)
{
    FStoryWorldState State;State.MoneyObjectId=TEXT("UniqueMoney");auto& Money=State.Objects.AddDefaulted_GetRef();Money.ObjectId=State.MoneyObjectId;
    TestTrue(TEXT("Valid unique world location"),State.HasUniqueMoney());
    TestTrue(TEXT("No money action yields failed robbery"),State.ResolveOutcome(TEXT("Robber"),TEXT("Police"),TEXT("Civil"))==EStoryOutcome::FailedRobbery);
    State.bMoneyWasTaken=true;Money.OwnerId=TEXT("Police");
    TestTrue(TEXT("Police owner determines recovery"),State.ResolveOutcome(TEXT("Robber"),TEXT("Police"),TEXT("Civil"))==EStoryOutcome::PoliceRecovered);
    Money.OwnerId=TEXT("Civil");TestTrue(TEXT("Civilian can win without police or robber victory"),State.ResolveOutcome(TEXT("Robber"),TEXT("Police"),TEXT("Civil"))==EStoryOutcome::CivilianBetrayal);
    Money.OwnerId=TEXT("Robber");State.bRobberEscaped=true;TestTrue(TEXT("Escaped robber ending"),State.ResolveOutcome(TEXT("Robber"),TEXT("Police"),TEXT("Civil"))==EStoryOutcome::RobberEscaped);
    const FStoryObjectState Duplicate=Money;State.Objects.Add(Duplicate);TestFalse(TEXT("Duplicate money ID rejected"),State.HasUniqueMoney());
    StorySessionTests::FWorld C;TestTrue(TEXT("Start failure movie"),C.Movie->StartMovie());
    for(int32 Frame=0;Frame<250 && C.Movie->State!=EMovieState::MoviePremiere;++Frame){C.Tick();}
    TestTrue(TEXT("All six failed turns still reach premiere"),C.Movie->State==EMovieState::MoviePremiere && C.Movie->StorySession->Segments.Num()==6);
    TestTrue(TEXT("Actual failed outcome"),C.Movie->StorySession->Outcome==EStoryOutcome::FailedRobbery);
    TestTrue(TEXT("Zero-action film can still be rated"),C.Movie->MovieResults.bAvailable && C.Movie->MovieResults.FinalScore==0 && C.Movie->MovieResults.Stars==1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryMovementCameraLifecycle,"OneManStudio.Story.MovementCameraRestoration",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FStoryMovementCameraLifecycle::RunTest(const FString& Parameters)
{
    StorySessionTests::FWorld C;auto* Character=C.World->SpawnActor<ACharacter>();C.Player->Possess(Character);
    auto* Move=Character->GetCharacterMovement();Move->MaxWalkSpeed=640;Move->MaxAcceleration=1700;Move->BrakingDecelerationWalking=1800;Move->bEnablePhysicsInteraction=true;
    auto* Arm=NewObject<USpringArmComponent>(Character);Character->AddInstanceComponent(Arm);Arm->SetupAttachment(Character->GetRootComponent());Arm->RegisterComponent();Arm->TargetArmLength=520;Arm->bEnableCameraLag=false;Arm->SocketOffset=FVector(0,10,20);
    TestTrue(TEXT("Starts existing character"),C.Movie->StartMovie());
    TestEqual(TEXT("Indoor speed capped without accelerating project character"),Move->MaxWalkSpeed,500.f);
    TestFalse(TEXT("Character cannot explode thrown props through push forces"),Move->bEnablePhysicsInteraction);
    TestTrue(TEXT("Camera collision and bounded smooth follow"),Arm->bDoCollisionTest && Arm->bEnableCameraLag && Arm->CameraLagMaxDistance==45);
    C.Movie->StopMovie();
    TestEqual(TEXT("Leaving story restores original movement"),Move->MaxWalkSpeed,640.f);TestEqual(TEXT("Original stopping restored"),Move->BrakingDecelerationWalking,1800.f);
    TestTrue(TEXT("Original physics interaction restored"),Move->bEnablePhysicsInteraction);TestEqual(TEXT("Original arm length restored"),Arm->TargetArmLength,520.f);
    TestTrue(TEXT("Original camera setup preserved"),!Arm->bEnableCameraLag && Arm->SocketOffset.Equals(FVector(0,10,20),.001));return true;
}
#endif
