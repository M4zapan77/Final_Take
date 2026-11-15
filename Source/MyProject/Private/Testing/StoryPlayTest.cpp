#if !UE_BUILD_SHIPPING
#include "MovieManager.h"
#include "StorySessionComponent.h"
#include "ActingRecorderComponent.h"
#include "ActingComponent.h"
#include "WardrobeComponent.h"
#include "ReplayActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace FinalTakeStoryPlay
{
struct FRun
{
    TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<AMovieManager> Movie;
    FTimerHandle Timer;
    FString Directory;
    double Started=FPlatformTime::Seconds();
    bool bConfigured=false,bDone=false,bActed=false,bMoved=false,bReleased=false,bCut=false,bPremiereShot=false,bResultsShot=false;
    int32 LastTurn=0,LastPreparation=INDEX_NONE,Wardrobes=0,StartedTurns=0;
    double PreparationClock=0;
    TWeakObjectPtr<APawn> Actors[3];
    void Finish(bool bSuccess,const TCHAR* Message)
    {
        if(bDone){return;} bDone=true;
        if(World.IsValid()){World->GetTimerManager().ClearTimer(Timer);}
        IFileManager::Get().MakeDirectory(*Directory,true);
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"turns\":%d,\"wardrobes\":%d,\"message\":\"%s\"}"),bSuccess?TEXT("true"):TEXT("false"),StartedTurns,Wardrobes,Message),*(Directory/TEXT("result.json")));
        UE_LOG(LogTemp,Display,TEXT("StoryPlayTest %s: %s"),bSuccess?TEXT("PASS"):TEXT("FAIL"),Message);
        if(Movie.IsValid()){Movie->StopMovie();}
        FPlatformMisc::RequestExitWithStatus(false,bSuccess?0:1);
    }
    bool Check(bool Condition,const TCHAR* Message){if(!Condition){Finish(false,Message);}return Condition;}
    void Shot(const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Directory/(FString(Name)+TEXT(".png")),true,false);}
    void Tick()
    {
        if(bDone || !World.IsValid()){return;}
        if(FPlatformTime::Seconds()-Started>120){Finish(false,TEXT("Story integration timeout"));return;}
        if(!Movie.IsValid()){for(TActorIterator<AMovieManager> It(World.Get());It;++It){Movie=*It;break;}if(!Movie.IsValid()){return;}}
        AMovieManager* M=Movie.Get();
        if(!bConfigured)
        {
            if(!M->HasActiveSession()){return;}
            M->StopMovie(); M->bUseChronologicalGameplay=true; M->StoryWardrobeDurationSeconds=.4; M->StoryTurnDurationSeconds=1.2;
            M->bUseTakeTransitions=true; M->BetweenActorsSeconds=.15; M->WrapSeconds=.15;
            M->PremiereSettings.TitleFadeInSeconds=.05; M->PremiereSettings.TitleHoldSeconds=.1; M->PremiereSettings.TitleFadeOutSeconds=.05;
            M->bShowDebugOverlay=false;
            if(!Check(M->StartMovie(),TEXT("Real bank story failed to start"))){return;}
            for(int32 Index=0;Index<3;++Index){Actors[Index]=M->StorySession->Cast[Index].Pawn;}
            bConfigured=true;
        }
        if(!Check(M->State!=EMovieState::Error,TEXT("Movie entered Error state"))){return;}
        if(M->State==EMovieState::Preparation)
        {
            if(M->PreparationId!=LastPreparation)
            {
                LastPreparation=M->PreparationId; ++Wardrobes; PreparationClock=M->GetSceneTimeSeconds();
                const int32 Role=M->StorySession->GetRoleIndex();
                const FName Heads[]={TEXT("Head.Beanie"),TEXT("Head.PoliceCap"),TEXT("Head.Crown")};
                if(!Check(M->SelectWardrobeItem(LastPreparation,ECosmeticCategory::Head,Heads[Role]),TEXT("Wardrobe selection failed"))){return;}
                M->SetWardrobeBodyColor(LastPreparation,true,FLinearColor(Role==0?.8f:.2f,Role==1?.8f:.2f,Role==2?.8f:.2f));
                if(Role==0){Shot(TEXT("Wardrobe"));}
            }
            if(!Check(FMath::IsNearlyEqual(M->GetSceneTimeSeconds(),PreparationClock,1.e-6),TEXT("Scene advanced during wardrobe"))){return;}
            // First role deliberately times out; other roles use the same guarded Ready path.
            if(Wardrobes>1 && M->GetPhaseElapsedSeconds()>.12)
            {
                if(!Check(M->ReadyForTake(LastPreparation),TEXT("Ready failed"))){return;}
                if(!Check(!M->ReadyForTake(LastPreparation),TEXT("Duplicate Ready started twice"))){return;}
            }
        }
        else if(M->State==EMovieState::Recording)
        {
            const int32 Turn=M->GetCurrentTakeNumber(); const int32 Role=(Turn-1)%3;
            APlayerController* PC=World->GetFirstPlayerController();
            if(Turn!=LastTurn)
            {
                ++StartedTurns; LastTurn=Turn; bActed=bMoved=bReleased=bCut=false;
                if(!Check(Turn==StartedTurns && Turn<=6,TEXT("Skipped or duplicated acting turn"))){return;}
                if(!Check(PC && PC->GetPawn()==Actors[Role].Get(),TEXT("Active role possession mismatch"))){return;}
                if(!Check(M->ActiveReplays.IsEmpty(),TEXT("Live story incorrectly spawned old replay actors"))){return;}
                if(Turn>3)
                {
                    const auto& Previous=M->StorySession->Segments[Role];
                    if(!Check(M->LivePerformer->GetActorLocation().Equals(Previous.FinalTransform.GetLocation(),2),TEXT("Returning character lost its position"))){return;}
                    if(!Check(M->LiveWardrobeComponent->CaptureAppearance().Selection.HeadId==M->StorySession->Cast[Role].Appearance.Selection.HeadId,TEXT("Returning character lost appearance"))){return;}
                }
                Shot(Turn==1?TEXT("FirstTurn"):Turn==4?TEXT("SecondRound"):TEXT("ActiveTurn"));
            }
            const double Time=M->GetPhaseElapsedSeconds();
            if(!bMoved && Time>.05){PC->InputKey(FInputKeyParams(EKeys::W,IE_Pressed,1.0,false));bMoved=true;}
            if(!bReleased && Time>.38){PC->InputKey(FInputKeyParams(EKeys::W,IE_Released,0.0,false));bReleased=true;}
            if(!bActed && Time>.45)
            {
                if(!Check(M->LiveActingComponent->TriggerGestureById(TEXT("Dance")),TEXT("Real character gesture failed"))){return;}
                if(!Check(M->LiveActingComponent->TriggerSoundById(TEXT("Scream")),TEXT("Real character sound failed"))){return;}
                bActed=true;
            }
            if(Turn==4 && Time>.75 && !bCut){bCut=true;if(!Check(M->EndCurrentTurn(),TEXT("Early Cut failed"))){return;}if(!Check(!M->EndCurrentTurn(),TEXT("Duplicate Cut accepted"))){return;}}
            for(int32 Index=0;Index<3;++Index)
            {
                if(Index==Role || M->StorySession->Segments.Num()<=Index){continue;}
                const FStorySegment* Last=nullptr;
                for(const auto& Segment:M->StorySession->Segments){if(Segment.ActorId==M->StorySession->Cast[Index].ActorId){Last=&Segment;}}
                if(Last && !Check(Actors[Index]->GetActorLocation().Equals(Last->FinalTransform.GetLocation(),.5),TEXT("Inactive actor moved during another role"))){return;}
                if(!Check(!Actors[Index]->FindComponentByClass<UActingRecorderComponent>()->IsRecording(),TEXT("Inactive actor recorded another role"))){return;}
            }
        }
        else if(M->State==EMovieState::MoviePremiere)
        {
            if(!Check(M->StorySession->Segments.Num()==6 && M->Performances.Num()==3,TEXT("Chronological movie did not contain six segments and three actors"))){return;}
            if(!Check(M->StorySession->GetTimelineDuration()<7.2 && M->StorySession->GetTimelineDuration()>6.5,TEXT("Early-cut actual timeline was not retained"))){return;}
            if(M->IsMoviePlayback() && !bPremiereShot){bPremiereShot=true;Shot(TEXT("Premiere"));}
        }
        else if(M->State==EMovieState::Finished)
        {
            if(M->GetPresentationView().ResultsOpacity<.999f){return;}
            if(!Check(StartedTurns==6 && Wardrobes==3,TEXT("Expected exactly six turns and first-only wardrobes"))){return;}
            for(int32 Index=0;Index<3;++Index)
            {
                if(!Check(M->ActiveReplays[Index]->GetNextEventIndex()==M->Performances[Index].Recording.Events.Num(),TEXT("Final movie missed recorded acting events"))){return;}
                if(!Check(M->ActiveReplays[Index]->Wardrobe->CaptureAppearance().Selection.HeadId==M->StorySession->Cast[Index].Appearance.Selection.HeadId,TEXT("Cinematic appearance mismatch"))){return;}
            }
            if(!bResultsShot){bResultsShot=true;Shot(TEXT("Results"));return;}
            if(!Check(M->RecordNewMovie(),TEXT("Record new movie failed"))){return;}
            if(!Check(M->StorySession->Segments.IsEmpty() && M->Performances.IsEmpty() && M->StorySession->Cast.Num()==3 && M->IsWardrobeOpen() && M->GetSceneTimeSeconds()==0,TEXT("New movie retained previous story data"))){return;}
            Finish(true,TEXT("Real bank: six turns, three persistent actors, first-only wardrobe, continuous clock, movement, gestures, early cut, chronological cinema and clean restart passed"));
        }
    }
};
void OnWorld(UWorld* World,const UWorld::InitializationValues Values)
{
    if(!World || !World->IsGameWorld() || !FParse::Param(FCommandLine::Get(),TEXT("game")) || !FParse::Param(FCommandLine::Get(),TEXT("FinalTakeStorySmoke"))){return;}
    auto Run=MakeShared<FRun>();Run->World=World;Run->Directory=FPaths::ProjectSavedDir()/TEXT("Automation/StoryPlayM1");
    FParse::Value(FCommandLine::Get(),TEXT("StoryReportDir="),Run->Directory);IFileManager::Get().MakeDirectory(*Run->Directory,true);
    World->GetTimerManager().SetTimer(Run->Timer,FTimerDelegate::CreateLambda([Run](){const TSharedRef<FRun> Keep=Run;Keep->Tick();}),.033f,true);
}
struct FRegistration
{
    FDelegateHandle Handle=FWorldDelegates::OnPostWorldInitialization.AddStatic(&OnWorld);
    ~FRegistration(){FWorldDelegates::OnPostWorldInitialization.Remove(Handle);}
} Registration;
}
#endif
