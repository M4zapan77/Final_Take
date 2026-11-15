#if !UE_BUILD_SHIPPING
#include "MovieManager.h"
#include "ObjectiveManagerComponent.h"
#include "ObjectiveHUDWidget.h"
#include "MoviePresentationWidget.h"
#include "ObjectiveZone.h"
#include "InteractableProp.h"
#include "PropInteractionComponent.h"
#include "ActingComponent.h"
#include "WardrobeComponent.h"
#include "ReplayActor.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "InputCoreTypes.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace ObjectivePlayTest
{
    class FRun
    {
    public:
        TWeakObjectPtr<UWorld> World;
        TWeakObjectPtr<AMovieManager> Manager;
        TWeakObjectPtr<AMoneyBagProp> Money;
        FTransform Origin;
        FTimerHandle Timer;
        double Started = FPlatformTime::Seconds();
        int32 PreparationCount = 0;
        int32 LastPreparation = 0;
        double PreparationStart = 0;
        bool Acted[3] = {};
        bool Captured[5] = {};
        bool VerifiedReplay[3] = {};
        bool bPremiereChecked = false;
        bool bDone = false;
        bool bRender = false;
        bool bUseLevelSetup = false;
        bool bDoorInputHeld = false;
        bool bDoorWalkVerified = false;
        FVector EntryTarget,EscapeTarget,CounterTarget,PickupTarget;
        FString Directory=FPaths::ProjectSavedDir()/TEXT("Automation/HeistPlay");
        FMovieScoreSummary ReportSummary;

        FVector At(FVector Offset) const { return Origin.TransformPositionNoScale(Offset); }
        void Finish(bool bPass,const TCHAR* Message)
        {
            if (bDone) { return; } bDone = true;
            IFileManager::Get().MakeDirectory(*Directory,true);
            FString Scores;
            const FMovieScoreSummary Summary=ReportSummary.RoleScores.IsEmpty() && Manager.IsValid()?Manager->ScoreSummary:ReportSummary;
            for (const auto& Item:Summary.RoleScores) { if (!Scores.IsEmpty()) { Scores+=TEXT(", "); } Scores+=FString::FromInt(Item.Score); }
            const double Average=Summary.FinalMovieScore;
            const FString Json=FString::Printf(TEXT("{\n  \"passed\": %s,\n  \"preparations\": %d,\n  \"scores\": [%s],\n  \"finalMovieScore\": %.6f,\n  \"usedLevelSetup\": %s,\n  \"doorWalkVerified\": %s,\n  \"message\": \"%s\"\n}\n"),
                bPass?TEXT("true"):TEXT("false"),PreparationCount,*Scores,Average,bUseLevelSetup?TEXT("true"):TEXT("false"),bDoorWalkVerified?TEXT("true"):TEXT("false"),Message);
            FFileHelper::SaveStringToFile(Json,*(Directory/TEXT("result.json")));
            if (World.IsValid()) { World->GetTimerManager().ClearTimer(Timer); }
            if (Manager.IsValid()) { Manager->StopMovie(); }
            UE_LOG(LogTemp,Display,TEXT("HeistPlay: %s - %s"),bPass?TEXT("PASS"):TEXT("FAIL"),Message);
            FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
        }
        bool Check(bool bValue,const TCHAR* Message) { if (!bValue) { Finish(false,Message); } return bValue; }
        void Capture(int32 Index,const TCHAR* Name)
        {
            if (!bRender || Captured[Index]) { return; } Captured[Index]=true;
            IFileManager::Get().MakeDirectory(*Directory,true);
            FScreenshotRequest::RequestScreenshot(Directory/(FString(Name)+TEXT(".png")),true,false);
        }
        void Setup(APawn* Pawn)
        {
            Origin=Pawn->GetActorTransform();
            AMovieManager* LevelMovie=nullptr;
            for (TActorIterator<AMovieManager> It(World.Get());It;++It) { if (!LevelMovie) { LevelMovie=*It; } It->StopMovie(); }
            FActorSpawnParameters Spawn; Spawn.ObjectFlags|=RF_Transient; Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            const auto Zone=[&](FName Tag,FVector Offset,FVector Extent)
            {
                auto* Actor=World->SpawnActor<AObjectiveZone>(At(Offset),Origin.Rotator(),Spawn);
                Actor->ZoneTag=Tag; Actor->Bounds->SetBoxExtent(Extent); Actor->RefreshMarker();
            };
            if (!bUseLevelSetup)
            {
            Zone(TEXT("Bank.Entry"),FVector(200,0,0),FVector(70,300,150));
            Zone(TEXT("Bank.Escape"),FVector(600,0,0),FVector(70,70,150));
            Zone(TEXT("Bank.Counter"),FVector(300,-180,0),FVector(50,50,150));
            Money=World->SpawnActor<AMoneyBagProp>(At(FVector(200,0,-40)),Origin.Rotator(),Spawn);
            World->SpawnActor<ADonutProp>(At(FVector(200,180,-40)),Origin.Rotator(),Spawn);
            World->SpawnActor<ABananaProp>(At(FVector(400,-180,-40)),Origin.Rotator(),Spawn);
            }
            else
            {
                bool bEntry=false,bCounter=false,bEscape=false;
                for (TActorIterator<AObjectiveZone> It(World.Get());It;++It)
                {
                    if (It->ZoneTag==TEXT("Bank.Entry")) { EntryTarget=It->GetActorLocation(); bEntry=true; }
                    if (It->ZoneTag==TEXT("Bank.Counter")) { CounterTarget=It->GetActorLocation(); bCounter=true; }
                    if (It->ZoneTag==TEXT("Bank.Escape")) { EscapeTarget=It->GetActorLocation(); bEscape=true; }
                }
                for (TActorIterator<AMoneyBagProp> It(World.Get());It;++It) { Money=*It; break; }
                if (!Check(LevelMovie && bEntry && bCounter && bEscape && Money.IsValid(),TEXT("Placed bank manager, zones or props missing"))) { return; }
                PickupTarget=Money->GetActorLocation()-FVector(120,0,0); PickupTarget.Z=EntryTarget.Z;
            }
            AMovieManager* Movie=bUseLevelSetup?LevelMovie:World->SpawnActor<AMovieManager>(); Manager=Movie;
            Movie->PreparationDurationSeconds=3.0; Movie->RecordingDurationSeconds=bUseLevelSetup?5.0:3.0;
            if (bUseLevelSetup) { Movie->Roles[0].InitialTransform.AddToTranslation(FVector(-500,0,0)); }
            Movie->bShowDebugOverlay=false; Movie->bBindActingKeys=false; Movie->bBindRestartKey=false; Movie->bBindInteractionKey=false;
            bRender=FParse::Param(FCommandLine::Get(),TEXT("ObjectiveSmokeScreenshots"));
            Check(Movie->StartMovie(),TEXT("Could not start scored movie"));
        }
        void Tick()
        {
            if (bDone || !World.IsValid()) { return; }
            if (FPlatformTime::Seconds()-Started>120.0) { Finish(false,TEXT("Scored Play flow timed out")); return; }
            APawn* Pawn=UGameplayStatics::GetPlayerPawn(World.Get(),0); if (!Pawn) { return; }
            if (!Manager.IsValid()) { Setup(Pawn); return; }
            auto* Movie=Manager.Get();
            if (!Check(Movie->State!=EMovieState::Error,TEXT("Movie entered Error"))) { return; }
            if (Movie->IsWardrobeOpen())
            {
                if (!Check(Movie->ActiveObjectiveHUD && Movie->ActiveObjectiveHUD->IsInViewport(),TEXT("Objective HUD missing during wardrobe"))) { return; }
                if (!Check(Movie->ObjectiveManager->Objectives.Num()==5 && Movie->GetCurrentScore()==0,TEXT("Role objectives not previewed at zero"))) { return; }
                if (LastPreparation!=Movie->PreparationId)
                {
                    LastPreparation=Movie->PreparationId; ++PreparationCount; PreparationStart=World->GetTimeSeconds();
                    if (Movie->CurrentTakeIndex==0) { Movie->SelectWardrobeItem(LastPreparation,ECosmeticCategory::Prop,NAME_None); }
                }
                if (Movie->CurrentTakeIndex==0 && World->GetTimeSeconds()-PreparationStart>.15) { Capture(0,TEXT("Preparation")); }
                if (World->GetTimeSeconds()-PreparationStart>.45 && (!bRender || !FScreenshotRequest::IsScreenshotRequested())) { Movie->ReadyForTake(LastPreparation); }
            }
            else if (Movie->State==EMovieState::Recording && Movie->ObjectiveManager->IsScoringActive())
            {
                const int32 Role=Movie->CurrentTakeIndex; const double Time=Movie->GetSceneTimeSeconds();
                if (Role==0)
                {
                    if (bUseLevelSetup && Time<2.2)
                    {
                        if (!bDoorInputHeld)
                        {
                            UGameplayStatics::GetPlayerController(World.Get(),0)->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,EKeys::W,IE_Pressed,FPlatformTime::Cycles64()));
                            bDoorInputHeld=true;
                        }
                        return;
                    }
                    if (bDoorInputHeld)
                    {
                        UGameplayStatics::GetPlayerController(World.Get(),0)->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,EKeys::W,IE_Released,FPlatformTime::Cycles64()));
                        bDoorInputHeld=false; bDoorWalkVerified=Movie->ObjectiveManager->IsObjectiveCompleted(TEXT("Robber.EnterBank"));
                        UE_LOG(LogTemp,Display,TEXT("Bank entrance walk: %s, score %d"),*Pawn->GetActorLocation().ToString(),Movie->GetCurrentScore());
                        if (!Check(bDoorWalkVerified,TEXT("Actual bank entrance could not be reached through W movement"))) { return; }
                    }
                    Pawn->SetActorLocation(bUseLevelSetup?(Time<3.4?PickupTarget:EscapeTarget):At(Time<2.0?FVector(200,0,0):FVector(600,0,0)));
                    if (!Acted[0] && Time>(bUseLevelSetup?2.25:.25))
                    {
                        if (!Check(Movie->LivePropInteraction->CollectProp(Money.Get()),TEXT("Money bag pickup failed"))) { return; }
                        Movie->LiveActingComponent->TriggerGesture(EActingGesture::Dance); Movie->LiveActingComponent->TriggerSound(EActingSound::EvilLaugh); Acted[0]=true;
                    }
                }
                else if (Role==1)
                {
                    Pawn->SetActorLocation(bUseLevelSetup?(Time<.2?EntryTarget:PickupTarget+FVector(0,50,0)):At(FVector(200,0,0)));
                    if (!Acted[1] && Time>(bUseLevelSetup?2.7:.45))
                    {
                        Movie->LiveActingComponent->TriggerGesture(EActingGesture::Arrest);
                        Movie->LivePropInteraction->UseEquippedProp(); Movie->LiveActingComponent->TriggerSound(EActingSound::PoliceSiren); Acted[1]=true;
                    }
                }
                else
                {
                    Pawn->SetActorLocation(bUseLevelSetup?(Time<.35?CounterTarget:PickupTarget+FVector(0,50,0)):At(Time<.35?FVector(300,-180,0):FVector(200,0,0)));
                    if (!Acted[2] && Time>(bUseLevelSetup?2.7:.5))
                    {
                        Movie->LiveActingComponent->TriggerGesture(EActingGesture::Scared); Movie->LiveActingComponent->TriggerGesture(EActingGesture::Dance);
                        Movie->LiveActingComponent->TriggerSound(EActingSound::Scream); Acted[2]=true;
                    }
                }
                if (Movie->GetCurrentScore()==100 && Role<3) { Capture(Role==0?1:Role==1?2:3,Role==0?TEXT("RobberHUD"):Role==1?TEXT("PoliceHUD"):TEXT("CustomerHUD")); }
                if (Role>0 && Time>(bUseLevelSetup?3.0:.75) && !VerifiedReplay[Role])
                {
                    for (int32 Index=0;Index<Role;++Index)
                    {
                        const AReplayActor* Replay=Movie->ActiveReplays[Index];
                        const FName Expected=Index==0?FName(TEXT("Prop.MoneyBag")):FName(TEXT("Prop.Donut"));
                        if (!Check(Replay->PropInteraction->EquippedPropId==Expected && !Replay->GetActorEnableCollision()
                            && Movie->Performances[Index].ObjectiveResult.Score==100,TEXT("Replay prop, collision or retained score was incorrect"))) { return; }
                    }
                    if (!Check(Money->IsAvailable(),TEXT("A replay consumed the current player's money pickup"))) { return; }
                    VerifiedReplay[Role]=true;
                }
            }
            else if (Movie->State==EMovieState::MoviePremiere && Movie->GetSceneTimeSeconds()>(bUseLevelSetup?3.0:.75))
            {
                if (!bPremiereChecked)
                {
                    for (int32 Index=0;Index<3;++Index)
                    {
                        const AReplayActor* Replay=Movie->ActiveReplays[Index];
                        if (!Check(Replay->Acting->DispatchedEventCount==Movie->Performances[Index].Recording.Events.Num()
                            && Movie->Performances[Index].Recording.Events.FilterByPredicate([](const auto& Event){return Event.Type!=EActingEventType::Footstep;}).Num()==3,TEXT("Premiere did not dispatch each role's three independent events once"))) { return; }
                    }
                    if (!Check((!Movie->ActiveObjectiveHUD || !Movie->ActiveObjectiveHUD->IsInViewport())
                        && Movie->ActiveMoviePresentation && Movie->ActiveMoviePresentation->ViewData.Phase==EMoviePresentationPhase::Playback
                        && !Money->IsAvailable(),TEXT("Premiere HUD hiding or pickup visual was incorrect"))) { return; }
                    bPremiereChecked=true;
                }
                Capture(4,TEXT("PremiereScore"));
            }
            else if (Movie->State==EMovieState::Finished)
            {
                if (bRender && FScreenshotRequest::IsScreenshotRequested()) { return; }
                if (!Check(bPremiereChecked && VerifiedReplay[1] && VerifiedReplay[2],TEXT("Replay or premiere checks were not reached"))) { return; }
                if (!Check(PreparationCount==3 && Movie->Performances.Num()==3 && FMath::IsNearlyEqual(Movie->ScoreSummary.FinalMovieScore,100.0),TEXT("Scored movie average or role count was incorrect"))) { return; }
                for (const auto& Performance:Movie->Performances)
                { if (!Check(Performance.ObjectiveResult.Score==100 && Performance.ObjectiveResult.CompletedObjectiveIds.Num()==5,TEXT("A role did not complete its five objectives"))) { return; } }
                ReportSummary=Movie->ScoreSummary;
                Movie->RestartMovie();
                if (!Check(Movie->ScoreSummary.RoleScores.IsEmpty() && Movie->Performances.IsEmpty() && Movie->GetCurrentScore()==0,TEXT("Restart retained objective state"))) { return; }
                Finish(true,TEXT("Live HUD, all fifteen objectives, recorded interactions, three 100-point roles, premiere, and score reset passed"));
            }
        }
    };
    void Start(UWorld* World)
    {
        if (!World || !World->IsGameWorld() || !FParse::Param(FCommandLine::Get(),TEXT("game")) || !FParse::Param(FCommandLine::Get(),TEXT("ObjectiveSmoke"))) { return; }
        auto Run=MakeShared<FRun>(); Run->World=World;
        Run->bUseLevelSetup=FParse::Param(FCommandLine::Get(),TEXT("UseLevelHeistSetup"));
        FParse::Value(FCommandLine::Get(),TEXT("HeistReportDir="),Run->Directory);
        World->GetTimerManager().SetTimer(Run->Timer,FTimerDelegate::CreateLambda([Run]() { Run->Tick(); }),.05f,true);
    }
    FAutoConsoleCommandWithWorld Command(TEXT("Movie.ObjectiveSmoke"),TEXT("Separate -game -ObjectiveSmoke process only; checks scored movie, writes report and exits."),FConsoleCommandWithWorldDelegate::CreateStatic(&Start));
}
#endif
