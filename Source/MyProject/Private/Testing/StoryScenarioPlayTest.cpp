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
#include "StudioNPCPerformanceComponent.h"
#include "StudioCinemaPresenter.h"
#include "Components/SceneCaptureComponent2D.h"
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
    bool bSetup=false,bDone=false,bWalking=false,bResultsShot=false,bPremiereShot=false,bDirectorShot=false,bRepeatedMovie=false,bLanguageChanged=false,bReturningMenu=false,bHudShot=false;double ThrowAt=-1;FName ThrownId;
    TArray<FCosmeticSelection> ExpectedLooks;int32 WardrobeShot=INDEX_NONE;
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
#if WITH_EDITOR
        const TCHAR* Runtime=TEXT("Development Editor game, offscreen");
#else
        const TCHAR* Runtime=TEXT("Packaged Windows Development, offscreen");
#endif
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"runtime\":\"%s\",\"segmentSamples\":%d,\"filmSamples\":%d,\"propSamples\":%d,\"allocatedPoseBytes\":%lld,\"peakWorkingSetMiB\":%.2f,\"recording\":%s,\"premiere\":%s}"),Runtime,SegmentSamples,FilmSamples,PropSamples,PoseBytes,PeakWorkingSet/1048576.0,*FrameSummary(RecordingFrames),*FrameSummary(PremiereFrames)),*(Directory/TEXT("measurements.json")));
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
    bool DropTowardRole(int32 Receiver)
    {
        // Stage a deliberate handoff on the lobby floor, rather than dropping
        // into the teller counter while still looking at the teller.
        auto* S=Movie->StorySession.Get();auto* PC=World->GetFirstPlayerController();
        const FVector Delta=S->Cast[Receiver].Pawn->GetActorLocation()-Movie->LivePerformer->GetActorLocation();
        PC->SetControlRotation(FRotator(-15,Delta.Rotation().Yaw,0));
        return S->DropHeldObject();
    }
    void EndTurn(){Key(false);const double Minimum=FParse::Param(FCommandLine::Get(),TEXT("StoryUICosmeticsCheck"))?.85:.12;if(Movie->GetPhaseElapsedSeconds()>Minimum){Movie->EndCurrentTurn();}}
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
            const bool Looks=FParse::Param(FCommandLine::Get(),TEXT("StoryUICosmeticsCheck"));
            if(LastPreparation!=M->PreparationId)
            {
                LastPreparation=M->PreparationId;++Wardrobes;
                if(Looks)
                {
                    const int32 Role=S->GetRoleIndex();ExpectedLooks.SetNum(3);
                    const FName Hats[]={TEXT("Head.RobberMask"),TEXT("Head.PoliceCap"),TEXT("Head.Crown")};
                    const FName Bodies[]={TEXT("Body.Robber"),TEXT("Body.Uniform"),TEXT("Body.Dress")};
                    const FLinearColor Colors[]={FLinearColor(.45,.22,.85),FLinearColor::White,FLinearColor(.25,.8,.4)};
                    if(!Check(M->SelectWardrobeItem(LastPreparation,ECosmeticCategory::Head,Hats[Role]) && M->SelectWardrobeItem(LastPreparation,ECosmeticCategory::Body,Bodies[Role]) && M->LiveWardrobeComponent->SetSkinColor(Colors[Role]),TEXT("Configured role cosmetics could not be selected"))){return;}
                    ExpectedLooks[Role]=M->LiveWardrobeComponent->GetSelection();
                }
            }
            if(Looks && WardrobeShot!=LastPreparation && M->GetPhaseElapsedSeconds()>.25){WardrobeShot=LastPreparation;Shot(*FString::Printf(TEXT("WardrobeRole%d"),S->GetRoleIndex()+1));return;}
            if(M->GetPhaseElapsedSeconds()>(Looks?.4:.15)){M->ReadyForTake(LastPreparation);}
        }
        else if(M->State==EMovieState::Recording)
        {
            if(M->GetPhaseElapsedSeconds()>.5 && !Check(World->GetFirstPlayerController()->GetViewTarget()==M->LivePerformer,TEXT("Gameplay camera did not follow the currently possessed role after wardrobe"))){return;}
            if(!bHudShot && M->GetPhaseElapsedSeconds()>.65){bHudShot=true;Shot(TEXT("RecordingHUD"));}
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
                if(FParse::Param(FCommandLine::Get(),TEXT("StoryUICosmeticsCheck")) && Turn>=4)
                {
                    const auto Current=M->LiveWardrobeComponent->GetSelection();const auto& Expected=ExpectedLooks[S->GetRoleIndex()];
                    if(!Check(Current.HeadId==Expected.HeadId && Current.BodyId==Expected.BodyId && Current.SkinColor.Equals(Expected.SkinColor),TEXT("Round two lost an actor's independent outfit"))){return;}
                }
                if(Turn==4 && Scenario==TEXT("Police") && !Check(S->SceneState.MoneyOwner()==S->Cast[1].ActorId,TEXT("Robber regained confiscated money on return"))){return;}
                if(FParse::Param(FCommandLine::Get(),TEXT("StoryUICosmeticsCheck")) && Scenario!=TEXT("Failure"))
                {
                    if(!Check(M->LiveActingComponent && M->LiveActingComponent->TriggerGesture(EActingGesture::Dance) && M->LiveActingComponent->TriggerSound(EActingSound::EvilLaugh),TEXT("Live role did not record configured gesture and sound"))){return;}
                }
                Shot(Turn==1?TEXT("Robber"):Turn==4?TEXT("RoundTwo"):TEXT("Acting"));
            }
            if(Scenario==TEXT("Failure")){return;}
            if(Scenario==TEXT("Escape"))
            {
                // Exercise the actual door, unique vault money and entrance
                // using the same movement/E routes as the player.
                if(Turn==1)
                {
                    if(Step==0 && Action(TEXT("Scene.VaultDoor"))){++Step;}
                    else if(Step==1 && Action(S->SceneState.MoneyObjectId)){++Step;Shot(TEXT("VaultMoney"));}
                    else if(Step==2 && Action(TEXT("Scene.Exit"))){++Step;Shot(TEXT("ActualEscape"));}
                    if(Step>=3){EndTurn();}
                }
                else if(Turn==3)
                {if(Step==0 && Action(TEXT("NPC.Guard"))){++Step;}if(Step>0){EndTurn();}}
                else if(Turn==4)
                {if(Step==0 && Action(TEXT("Scene.Exit"))){++Step;Shot(TEXT("ActualEscape"));}if(Step>0){EndTurn();}}
                else{EndTurn();}
                return;
            }
            if(Scenario==TEXT("Police"))
            {
                switch(Turn)
                {
                case 1:if(Step==0 && Action(TEXT("NPC.Teller"),FVector(270,0,0))){++Step;Shot(TEXT("MoneyAcquired"));}break;
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
                    if(Step==0 && Action(TEXT("NPC.Teller"),FVector(270,0,0))){++Step;}
                    else if(Step==1 && DropTowardRole(1)){++Step;Shot(TEXT("DroppedMoney"));}break;
                case 2:
                    if(Step==0 && Action(S->SceneState.MoneyObjectId)){++Step;}
                    else if(Step==1 && DropTowardRole(2)){++Step;}break;
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
                    else if(Step==4 && DropTowardRole(0)){++Step;}break;
                case 4:if(Step==0 && Action(S->SceneState.MoneyObjectId)){++Step;}break;
                case 5:if(Step==0 && Action(S->Cast[0].ActorId)){++Step;}break;
                case 6:if(Step==0 && Action(S->Cast[1].ActorId)){++Step;Shot(TEXT("CivilianSteals"));}break;
                }
                const int32 Required=Turn<=2?2:Turn==3?5:1;if(Step>=Required){EndTurn();}
            }
        }
        else if(M->State==EMovieState::MoviePremiere)
        {
            const EStoryOutcome Expected=Scenario==TEXT("Police")?EStoryOutcome::PoliceRecovered:Scenario==TEXT("Civilian")?EStoryOutcome::CivilianBetrayal:Scenario==TEXT("Escape")?EStoryOutcome::RobberEscaped:EStoryOutcome::FailedRobbery;
            if(!Check(S->Outcome==Expected && S->Segments.Num()==6,TEXT("Final outcome does not match actual actions"))){return;}
            if(FParse::Param(FCommandLine::Get(),TEXT("StoryUICosmeticsCheck")))
            {
                for(int32 I=0;I<M->Performances.Num();++I)
                {
                    const auto& Look=M->Performances[I].Recording.Appearance.Selection;const auto& ExpectedLook=ExpectedLooks[I];
                    if(!Check(Look.HeadId==ExpectedLook.HeadId && Look.BodyId==ExpectedLook.BodyId && Look.SkinColor.Equals(ExpectedLook.SkinColor),TEXT("Recorded performance lost independent cosmetic data"))){return;}
                    if(M->ActiveReplays.IsValidIndex(I))
                    {
                        const auto Ghost=M->ActiveReplays[I]->Wardrobe->GetSelection();
                        if(!Check(Ghost.HeadId==ExpectedLook.HeadId && Ghost.BodyId==ExpectedLook.BodyId && Ghost.SkinColor.Equals(ExpectedLook.SkinColor),TEXT("Premiere actor did not restore its own outfit"))){return;}
                    }
                }
            }
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
            {
                int32 NPCCount=0;TSet<FString> Faces;
                for(TActorIterator<ACharacter> It(World.Get());It;++It)
                {
                    auto* N=It->FindComponentByClass<UStudioNPCPerformanceComponent>();if(!N){continue;}++NPCCount;
                    FTransform Recorded;
                    if(!Check(N->Performance.Samples.Num()>2 && N->Performance.Evaluate(S->GetTimelineDuration(),Recorded) && It->GetActorTransform().Equals(Recorded,.05),TEXT("Background NPC did not end at its recorded pose"))){return;}
                    const auto Frozen=N->Performance.Appearance.Selection;
                    auto* W=It->FindComponentByClass<UWardrobeComponent>();const auto Current=W->GetSelection();
                    if(!Check(Current.EyeId==Frozen.EyeId && Current.MouthId==Frozen.MouthId && Current.EyebrowId==Frozen.EyebrowId,TEXT("NPC face changed while replaying the film"))){return;}
                    Faces.Add(Frozen.EyeId.ToString()+TEXT("|")+Frozen.MouthId.ToString()+TEXT("|")+Frozen.EyebrowId.ToString());
                }
                if(!Check(NPCCount==4 && Faces.Num()>1,TEXT("Expected four animated NPCs with varied faces"))){return;}
                for(TActorIterator<AStudioCinemaPresenter> It(World.Get());It;++It)
                {if(!Check(It->IsPresenting() && It->FilmCapture->PostProcessSettings.AutoExposureMaxBrightness==It->FilmExposureEV,TEXT("Cinema capture did not retain its fixed exposure"))){return;}}
            }
            if(!Check(Turns==6 && Wardrobes==3 && M->MovieResults.bAvailable,TEXT("Results or complete six-turn flow missing"))){return;}
            if(!Check(M->MovieDirector->GetCamera()!=nullptr && (S->GetTimelineDuration()<2.5 || M->MovieDirector->ShotCount>1),TEXT("Cinematic director did not create or select shots"))){return;}
            if(!Check(S->FinalSceneState.MoneyOwner()==OriginalFinal.MoneyOwner() && S->SceneEvents.Num()==OriginalEvents.Num() && M->MovieResults.FinalScore==OriginalScore,TEXT("Movie playback changed ownership, recording or score"))){return;}
            for(int32 Index=0;Index<3;++Index)
            {
                if(!Check(M->ActiveReplays[Index]->GetNextEventIndex()==M->Performances[Index].Recording.Events.Num(),TEXT("Chronological film missed actor events"))){return;}
                if(FParse::Param(FCommandLine::Get(),TEXT("StoryUICosmeticsCheck")) && Scenario!=TEXT("Failure") && !Check(M->ActiveReplays[Index]->Acting->StartedSoundCount>0,TEXT("A cinematic actor did not reproduce its recorded sound"))){return;}
            }
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
