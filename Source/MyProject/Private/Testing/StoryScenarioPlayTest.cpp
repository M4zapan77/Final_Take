#if !UE_BUILD_SHIPPING
#include "MovieManager.h"
#include "StorySessionComponent.h"
#include "ActingComponent.h"
#include "ActingRecorderComponent.h"
#include "ReplayActor.h"
#include "WardrobeComponent.h"
#include "GrabbablePropComponent.h"
#include "MovieDirectorComponent.h"
#include "StudioLanguageSubsystem.h"
#include "StudioFlowManager.h"
#include "StudioFlowWidget.h"
#include "StartupVideoComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "InputKeyEventArgs.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformMemory.h"
#include "Misc/App.h"
#include "Misc/CoreDelegates.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace StoryScenarioPlay
{
bool bSawHub=false,bSawIntroduction=false;
struct FRun
{
    TWeakObjectPtr<UWorld> World;TWeakObjectPtr<AMovieManager> Movie;FTimerHandle Timer;
    FString Directory,Scenario=TEXT("Police");double Started=FPlatformTime::Seconds(),LastDiagnostic=-1;
    int32 LastTurn=0,Step=0,Turns=0,LastPreparation=INDEX_NONE,Wardrobes=0;
    bool bSetup=false,bDone=false,bWalking=false,bResultsShot=false,bPremiereShot=false,bDirectorShot=false,bRepeatedMovie=false,bLanguageChanged=false,bReturningMenu=false;double ThrowAt=-1;FName ThrownId;
    TArray<FStorySceneEvent> OriginalEvents;FStoryWorldState OriginalFinal;double OriginalScore=0;
    FDelegateHandle FrameHandle;TArray<double> RecordingFrames,PremiereFrames;uint64 PeakWorkingSet=0;int32 SegmentSamples=0,FilmSamples=0,PropSamples=0;int64 PoseBytes=0;
    ~FRun(){FCoreDelegates::OnEndFrame.Remove(FrameHandle);}
    void ObserveFrame()
    {
        if(bDone || !bSetup || !Movie.IsValid()){return;}const double Milliseconds=FApp::GetDeltaTime()*1000;if(!FMath::IsFinite(Milliseconds) || Milliseconds<=0){return;}
        if(Movie->State==EMovieState::Recording){RecordingFrames.Add(Milliseconds);}else if(Movie->IsMoviePlayback()){PremiereFrames.Add(Milliseconds);}
        PeakWorkingSet=FMath::Max(PeakWorkingSet,FPlatformMemory::GetStats().UsedPhysical);
    }
    FString FrameSummary(TArray<double> Values)
    {
        if(Values.IsEmpty()){return TEXT("{}");}Values.Sort();double Sum=0;for(double Value:Values){Sum+=Value;}const double Mean=Sum/Values.Num();
        return FString::Printf(TEXT("{\"frames\":%d,\"meanMs\":%.4f,\"fpsFromMean\":%.2f,\"p95Ms\":%.4f,\"p99Ms\":%.4f,\"maxMs\":%.4f}"),Values.Num(),Mean,1000/Mean,Values[FMath::Min(Values.Num()-1,FMath::FloorToInt(Values.Num()*.95))],Values[FMath::Min(Values.Num()-1,FMath::FloorToInt(Values.Num()*.99))],Values.Last());
    }
    void Key(bool Press){if(auto* PC=World->GetFirstPlayerController()){PC->InputKey(FInputKeyParams(EKeys::W,Press?IE_Pressed:IE_Released,Press?1.0:0.0,false));}bWalking=Press;}
    void Finish(bool Success,const TCHAR* Message)
    {
        if(bDone){return;}bDone=true;Key(false);if(World.IsValid()){World->GetTimerManager().ClearTimer(Timer);}
        IFileManager::Get().MakeDirectory(*Directory,true);
        if(Movie.IsValid())
        {
            FString Events=TEXT("sequence,time,type,instigator,target,payload\n");
            const auto& SavedEvents=OriginalEvents.IsEmpty()?Movie->StorySession->SceneEvents:OriginalEvents;
            for(const auto& E:SavedEvents){Events+=FString::Printf(TEXT("%d,%.6f,%d,%s,%s,%s\n"),E.Sequence,E.TimeSeconds,static_cast<int32>(E.Type),*E.InstigatorId.ToString(),*E.TargetId.ToString(),*E.Payload.ToString());}
            FFileHelper::SaveStringToFile(Events,*(Directory/TEXT("SceneEvents.csv")));
        }
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"scenario\":\"%s\",\"turns\":%d,\"wardrobes\":%d,\"physicalThrow\":%s,\"moviePlaybacks\":%d,\"message\":\"%s\"}"),Success?TEXT("true"):TEXT("false"),*Scenario,Turns,Wardrobes,ThrownId.IsNone()?TEXT("false"):TEXT("true"),bRepeatedMovie?2:1,Message),*(Directory/TEXT("result.json")));
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"runtime\":\"Development Editor game, offscreen\",\"segmentSamples\":%d,\"filmSamples\":%d,\"propSamples\":%d,\"allocatedPoseBytes\":%lld,\"peakWorkingSetMiB\":%.2f,\"recording\":%s,\"premiere\":%s}"),SegmentSamples,FilmSamples,PropSamples,PoseBytes,PeakWorkingSet/1048576.0,*FrameSummary(RecordingFrames),*FrameSummary(PremiereFrames)),*(Directory/TEXT("measurements.json")));
        UE_LOG(LogTemp,Display,TEXT("StoryScenario %s %s: %s"),*Scenario,Success?TEXT("PASS"):TEXT("FAIL"),Message);
        if(Movie.IsValid()){Movie->StopMovie();}FPlatformMisc::RequestExitWithStatus(false,Success?0:1);
    }
    bool Check(bool Value,const TCHAR* Message){if(!Value){Finish(false,Message);}return Value;}
    void Shot(const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Directory/(FString(Name)+TEXT(".png")),true,false);}
    bool Action(FName Target,FVector Offset=FVector::ZeroVector)
    {
        auto* S=Movie->StorySession.Get();
        if(S->InteractWith(Target)){UE_LOG(LogTemp,Display,TEXT("StoryScenario action turn=%d target=%s owner=%s"),LastTurn,*Target.ToString(),*S->SceneState.MoneyOwner().ToString());Key(false);return true;}
        FVector Goal=S->GetInteractionLocation(Target)+Offset;
        const FVector Delta=Goal-Movie->LivePerformer->GetActorLocation();
        auto* PC=World->GetFirstPlayerController();FRotator View=PC->GetControlRotation();View.Yaw=Delta.Rotation().Yaw;View.Pitch=-20;PC->SetControlRotation(View);
        if(Delta.Size2D()>45){if(!bWalking || !PC->IsInputKeyDown(EKeys::W)){Key(true);}}
        else{Key(false);}
        return false;
    }
    void EndTurn(){Key(false);if(Movie->GetPhaseElapsedSeconds()>.12){Movie->EndCurrentTurn();}}
    void Tick()
    {
        if(bDone || !World.IsValid()){return;}
        if(FPlatformTime::Seconds()-Started>300){Finish(false,TEXT("Scene playtest timed out"));return;}
        if(!Movie.IsValid()){for(TActorIterator<AMovieManager> It(World.Get());It;++It){Movie=*It;break;}if(!Movie.IsValid()){return;}}
        AMovieManager* M=Movie.Get();
        const bool Front=FParse::Param(FCommandLine::Get(),TEXT("StoryFrontEndCheck"));
        if(bReturningMenu)
        {
            auto* Flow=AStudioFlowManager::Find(M);if(!Flow || Flow->Screen!=EStudioScreen::MainMenu || !Flow->StartupVideos->IsHubInteractive()){return;}
            if(!Check(!M->HasActiveSession() && M->StorySession->Cast.IsEmpty() && M->StorySession->SceneEvents.IsEmpty(),TEXT("Returning to menu retained story actors or events"))){return;}
            Finish(true,TEXT("Normal HUB, script introduction, wardrobe, six turns, two films, new movie and return to interactive HUB verified"));return;
        }
        if(!bSetup)
        {
            if(Front && !M->HasActiveSession())
            {
                auto* Flow=AStudioFlowManager::Find(M);if(!Flow || !Flow->ActiveWidget){return;}
                if(Flow->StartupVideos->IsIntro()){Flow->StartupVideos->SkipIntroductions();return;}
                if(Flow->Screen==EStudioScreen::MainMenu && Flow->StartupVideos->IsHubInteractive())
                {bSawHub=true;Shot(TEXT("NormalHub"));Flow->ActiveWidget->PlayMovie();return;}
                if(Flow->Screen==EStudioScreen::MovieIntroduction)
                {
                    bSawIntroduction=true;Shot(TEXT("ScriptIntroduction"));
                    if(!FParse::Param(FCommandLine::Get(),TEXT("StoryDefaultTimes"))){M->StoryWardrobeDurationSeconds=.6;M->BetweenActorsSeconds=.15;M->WrapSeconds=.15;}
                    Flow->ActiveWidget->BeginMovie();return;
                }
                return;
            }
            if(!M->HasActiveSession()){return;}
            if(Front){if(!Check(M->IsChronologicalMovie() && bSawHub && bSawIntroduction,TEXT("Normal menu did not enter the new story mode through its introduction"))){return;}bSetup=true;}
            else{M->StopMovie();M->bUseChronologicalGameplay=true;
            if(!FParse::Param(FCommandLine::Get(),TEXT("StoryDefaultTimes"))){M->StoryWardrobeDurationSeconds=.6;M->StoryTurnDurationSeconds=10;}
            M->bUseTakeTransitions=true;M->BetweenActorsSeconds=.15;M->WrapSeconds=.15;M->bShowDebugOverlay=false;
            M->PremiereSettings.TitleFadeInSeconds=.1;M->PremiereSettings.TitleHoldSeconds=.15;M->PremiereSettings.TitleFadeOutSeconds=.1;
            if(!Check(M->StartMovie(),TEXT("Could not start contextual story"))){return;}bSetup=true;}
        }
        UStorySessionComponent* S=M->StorySession;
        if(!Check(M->State!=EMovieState::Error && S->SceneState.HasUniqueMoney(),TEXT("Story error or duplicate money"))){return;}
        if(M->State==EMovieState::Preparation)
        {
            if(LastPreparation!=M->PreparationId){LastPreparation=M->PreparationId;++Wardrobes;}
            if(M->GetPhaseElapsedSeconds()>.15){M->ReadyForTake(LastPreparation);}
        }
        else if(M->State==EMovieState::Recording)
        {
            const int32 Turn=M->GetCurrentTakeNumber();
            if(M->GetPhaseElapsedSeconds()-LastDiagnostic>.5 || LastDiagnostic>M->GetPhaseElapsedSeconds())
            {
                LastDiagnostic=M->GetPhaseElapsedSeconds();auto* PC=World->GetFirstPlayerController();auto* C=Cast<ACharacter>(M->LivePerformer);
                UE_LOG(LogTemp,Display,TEXT("StoryScenario movement turn=%d step=%d time=%.2f pos=%s velocity=%s ignored=%d mode=%d owner=%s"),Turn,Step,LastDiagnostic,*M->LivePerformer->GetActorLocation().ToString(),*M->LivePerformer->GetVelocity().ToString(),PC?PC->IsMoveInputIgnored():true,C?static_cast<int32>(C->GetCharacterMovement()->MovementMode):-1,*S->SceneState.MoneyOwner().ToString());
                if(C && bWalking && C->GetVelocity().Size2D()<1)
                {
                    FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(StoryMovementDiagnostic),false,C);
                    const FVector Forward=FRotationMatrix(FRotator(0,PC->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::X);
                    World->SweepSingleByChannel(Hit,C->GetActorLocation(),C->GetActorLocation()+Forward*150,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(C->GetCapsuleComponent()->GetScaledCapsuleRadius(),C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),Q);
                    UE_LOG(LogTemp,Display,TEXT("StoryScenario blocked key=%d input=%s actor=%s component=%s normal=%s"),PC->IsInputKeyDown(EKeys::W),*C->GetLastMovementInputVector().ToString(),*GetNameSafe(Hit.GetActor()),*GetNameSafe(Hit.GetComponent()),*Hit.Normal.ToString());
                }
            }
            if(Turn!=LastTurn)
            {
                LastTurn=Turn;++Turns;Step=0;Key(false);
                if(!Check(Turn==Turns && Turn<=6,TEXT("Turn order broken"))){return;}
                if(Turn==4 && Scenario==TEXT("Police") && !Check(S->SceneState.MoneyOwner()==S->Cast[1].ActorId,TEXT("Robber regained confiscated money on return"))){return;}
                Shot(Turn==1?TEXT("Robber"):Turn==4?TEXT("RoundTwo"):TEXT("Acting"));
            }
            if(Scenario==TEXT("Failure")){return;}
            if(Scenario==TEXT("Police"))
            {
                switch(Turn)
                {
                case 1:if(Step==0 && Action(TEXT("NPC.Teller"),FVector(0,270,0))){++Step;Shot(TEXT("MoneyAcquired"));}break;
                case 2:if(Step==0 && Action(S->Cast[0].ActorId)){++Step;Shot(TEXT("Confiscated"));}break;
                case 3:if(Step==0 && Action(TEXT("NPC.Guard"))){++Step;Shot(TEXT("DistractedGuard"));}break;
                case 4:if(Step==0 && Action(TEXT("NPC.Witness1"))){++Step;}break;
                case 5:if(Step==0 && Action(TEXT("Scene.Secure"))){++Step;Shot(TEXT("SecuredMoney"));}break;
                case 6:if(Step==0 && Action(TEXT("NPC.Guard"))){++Step;}break;
                }
                if(Step>0){EndTurn();}
            }
            else
            {
                switch(Turn)
                {
                case 1:
                    if(Step==0 && Action(TEXT("NPC.Teller"),FVector(0,270,0))){++Step;}
                    else if(Step==1 && S->DropHeldObject()){++Step;Shot(TEXT("DroppedMoney"));}break;
                case 2:
                    if(Step==0 && Action(S->SceneState.MoneyObjectId)){++Step;}
                    else if(Step==1 && S->DropHeldObject()){++Step;}break;
                case 3:
                    if(Step==0 && Action(TEXT("NPC.Guard"))){++Step;}
                    else if(Step==1)
                    {
                        const auto* Donut=S->SceneState.Objects.FindByPredicate([](const auto& O){return O.CosmeticId==TEXT("Prop.Donut") && O.OwnerId.IsNone();});
                        if(!Check(Donut!=nullptr,TEXT("No physical donut available"))){return;}ThrownId=Donut->ObjectId;if(Action(ThrownId)){++Step;}
                    }
                    else if(Step==2)
                    {
                        auto* PC=World->GetFirstPlayerController();const FVector Delta=S->GetInteractionLocation(TEXT("NPC.Guard"))-M->LivePerformer->GetActorLocation();PC->SetControlRotation(Delta.Rotation());
                        if(S->ThrowHeldObject()){ThrowAt=M->GetPhaseElapsedSeconds();++Step;Shot(TEXT("ActualThrow"));}
                    }
                    else if(Step==3 && M->GetPhaseElapsedSeconds()-ThrowAt>.8){if(Action(S->SceneState.MoneyObjectId)){++Step;}}
                    else if(Step==4 && S->DropHeldObject()){++Step;}break;
                case 4:if(Step==0 && Action(S->SceneState.MoneyObjectId)){++Step;}break;
                case 5:if(Step==0 && Action(S->Cast[0].ActorId)){++Step;}break;
                case 6:if(Step==0 && Action(S->Cast[1].ActorId)){++Step;Shot(TEXT("CivilianSteals"));}break;
                }
                const int32 Required=Turn<=2?2:Turn==3?5:1;if(Step>=Required){EndTurn();}
            }
        }
        else if(M->State==EMovieState::MoviePremiere)
        {
            const EStoryOutcome Expected=Scenario==TEXT("Police")?EStoryOutcome::PoliceRecovered:Scenario==TEXT("Civilian")?EStoryOutcome::CivilianBetrayal:EStoryOutcome::FailedRobbery;
            if(!Check(S->Outcome==Expected && S->Segments.Num()==6,TEXT("Final outcome does not match actual actions"))){return;}
            if(Scenario==TEXT("Civilian"))
            {
                const auto* Track=S->PropTracks.FindByPredicate([this](const auto& T){return T.ObjectId==ThrownId;});
                if(!Check(Track && Track->Samples.Num()>4 && S->SceneEvents.ContainsByPredicate([this](const auto& E){return E.Type==EStoryEventType::PropThrown && E.ObjectId==ThrownId;}),TEXT("Required physical throw was not recorded"))){return;}
            }
            if(OriginalEvents.IsEmpty())
            {
                OriginalEvents=S->SceneEvents;OriginalFinal=S->FinalSceneState;OriginalScore=M->MovieResults.FinalScore;
                for(const auto& Segment:S->Segments){SegmentSamples+=Segment.Recording.Samples.Num();PoseBytes+=Segment.Recording.Samples.GetAllocatedSize();}
                for(const auto& Film:M->Performances){FilmSamples+=Film.Recording.Samples.Num();PoseBytes+=Film.Recording.Samples.GetAllocatedSize();}
                for(const auto& Track:S->PropTracks){PropSamples+=Track.Samples.Num();PoseBytes+=Track.Samples.GetAllocatedSize();}
            }
            if(!bPremiereShot && M->IsMoviePlayback()){bPremiereShot=true;Shot(TEXT("Premiere"));}
            if(!bDirectorShot && M->IsMoviePlayback() && M->MovieDirector->ShotCount>1){bDirectorShot=true;Shot(TEXT("DirectedShot"));}
        }
        else if(M->State==EMovieState::Finished)
        {
            if(M->GetPresentationView().ResultsOpacity<.999f){return;}
            if(!Check(Turns==6 && Wardrobes==3 && M->MovieResults.bAvailable,TEXT("Results or complete six-turn flow missing"))){return;}
            if(!Check(M->MovieDirector->GetCamera()!=nullptr && (S->GetTimelineDuration()<2.5 || M->MovieDirector->ShotCount>1),TEXT("Cinematic director did not create or select shots"))){return;}
            if(!Check(S->FinalSceneState.MoneyOwner()==OriginalFinal.MoneyOwner() && S->SceneEvents.Num()==OriginalEvents.Num() && M->MovieResults.FinalScore==OriginalScore,TEXT("Movie playback changed ownership, recording or score"))){return;}
            for(int32 Index=0;Index<3;++Index){if(!Check(M->ActiveReplays[Index]->GetNextEventIndex()==M->Performances[Index].Recording.Events.Num(),TEXT("Chronological film missed actor events"))){return;}}
            for(TActorIterator<AActor> It(World.Get());It;++It)
            {
                auto* Prop=It->FindComponentByClass<UGrabbablePropComponent>();if(!Prop){continue;}
                if(!Check(!Prop->PhysicsBody->IsSimulatingPhysics(),TEXT("Movie resimulated physical prop"))){return;}
                const auto* State=S->FinalSceneState.FindObject(Prop->ObjectId);if(!State || !State->OwnerId.IsNone()){continue;}
                const auto* Track=S->PropTracks.FindByPredicate([Prop](const auto& T){return T.ObjectId==Prop->ObjectId;});FTransform Recorded;
                if(!Check(Track && Track->Evaluate(S->GetTimelineDuration(),Recorded) && Prop->GetBodyTransform().Equals(Recorded,.02),TEXT("Movie prop did not end at its recorded transform"))){return;}
            }
            if(!bResultsShot){bResultsShot=true;Shot(TEXT("Results"));return;}
            if(FParse::Param(FCommandLine::Get(),TEXT("StoryLanguageCheck")) && !bLanguageChanged)
            {
                auto* Language=UStudioLanguageSubsystem::Find(M);if(!Check(Language && Language->GetLanguage()==TEXT("es"),TEXT("Movie did not start in Spanish"))){return;}
                if(!Check(M->MovieResults.RatingName.ToString()!=TEXT("Cult Classic") && M->MovieResults.Roles[0].RoleName.ToString()==TEXT("Ladrón"),TEXT("Spanish results or role localization missing"))){return;}
                if(!Check(Language->SetLanguage(TEXT("en")) && M->State==EMovieState::Finished && M->MovieResults.FinalScore==OriginalScore && S->FinalSceneState.MoneyOwner()==OriginalFinal.MoneyOwner(),TEXT("Language switch changed movie state or failed"))){return;}
                if(!Check(M->MovieResults.Roles[0].RoleName.ToString()==TEXT("Robber"),TEXT("Existing results FText did not refresh"))){return;}
                bLanguageChanged=true;Shot(TEXT("ResultsEnglish"));return;
            }
            if(!bRepeatedMovie){bRepeatedMovie=true;if(!Check(M->ReplayMovie(),TEXT("Replay Movie failed"))){return;}return;}
            if(!Check(M->RecordNewMovie(),TEXT("New movie failed"))){return;}
            if(bLanguageChanged && !Check(UStudioLanguageSubsystem::Find(M)->GetLanguage()==TEXT("en") && M->GetCurrentRoleName().ToString()==TEXT("Robber"),TEXT("New movie lost English preference"))){return;}
            if(!Check(S->SceneEvents.IsEmpty() && S->Segments.IsEmpty() && S->SceneState.MoneyOwner().IsNone() && !S->SceneState.bMoneyWasTaken && !S->SceneState.bAlarmActive && !S->SceneState.bMoneySecured && S->SceneState.NPCs.Num()==4,TEXT("New movie kept old scene state"))){return;}
            if(Front)
            {auto* Flow=AStudioFlowManager::Find(M);if(!Check(Flow && Flow->ReturnToMainMenu(),TEXT("Return to main menu failed after new movie"))){return;}bReturningMenu=true;return;}
            Finish(true,TEXT("Six-turn bank story, real movement, unique money, NPC consequences, chronological event replay, outcome, score and clean new movie verified"));
        }
    }
};
void OnWorld(UWorld* World,const UWorld::InitializationValues Values)
{
    if(!World || !World->IsGameWorld() || !FParse::Param(FCommandLine::Get(),TEXT("game")) || !FParse::Param(FCommandLine::Get(),TEXT("FinalTakeSceneSmoke"))){return;}
    auto Run=MakeShared<FRun>();Run->World=World;FParse::Value(FCommandLine::Get(),TEXT("StoryScenario="),Run->Scenario);
    TWeakPtr<FRun> Weak=Run;Run->FrameHandle=FCoreDelegates::OnEndFrame.AddLambda([Weak](){if(auto Value=Weak.Pin()){Value->ObserveFrame();}});
    Run->Directory=FPaths::ProjectSavedDir()/TEXT("Automation")/(TEXT("StoryScene")+Run->Scenario);FParse::Value(FCommandLine::Get(),TEXT("StoryReportDir="),Run->Directory);
    IFileManager::Get().MakeDirectory(*Run->Directory,true);World->GetTimerManager().SetTimer(Run->Timer,FTimerDelegate::CreateLambda([Run](){const TSharedRef<FRun> Keep=Run;Keep->Tick();}),.033f,true);
}
struct FRegistration{FDelegateHandle Handle=FWorldDelegates::OnPostWorldInitialization.AddStatic(&OnWorld);~FRegistration(){FWorldDelegates::OnPostWorldInitialization.Remove(Handle);}} Registration;
}
#endif
