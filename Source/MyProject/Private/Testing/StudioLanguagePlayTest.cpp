#if !UE_BUILD_SHIPPING
#include "StudioLanguageSubsystem.h"
#include "StudioFlowManager.h"
#include "StudioFlowWidget.h"
#include "StartupVideoComponent.h"
#include "MovieManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/Button.h"
#include "MediaPlayer.h"
#include "TimerManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"
namespace StudioLanguagePlay
{
struct FRun
{
    TWeakObjectPtr<UWorld> World;FTimerHandle Timer;FString Expected=TEXT("en"),Directory;double Started=FPlatformTime::Seconds(),LastChange=0;int32 Step=0;
    TWeakObjectPtr<UMediaPlayer> OriginalPlayer;FTimespan VideoTime;bool bDone=false;
    void Finish(bool Success,const TCHAR* Message)
    {
        if(bDone){return;}bDone=true;World->GetTimerManager().ClearTimer(Timer);IFileManager::Get().MakeDirectory(*Directory,true);
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"expectedSavedLanguage\":\"%s\",\"message\":\"%s\"}"),Success?TEXT("true"):TEXT("false"),*Expected,Message),*(Directory/TEXT("result.json")));
        UE_LOG(LogTemp,Display,TEXT("LanguagePlay %s: %s"),Success?TEXT("PASS"):TEXT("FAIL"),Message);FPlatformMisc::RequestExitWithStatus(false,Success?0:1);
    }
    bool Check(bool Value,const TCHAR* Message){if(!Value){Finish(false,Message);}return Value;}
    void Tick()
    {
        if(bDone || !World.IsValid()){return;}if(FPlatformTime::Seconds()-Started>70){Finish(false,TEXT("Language menu test timed out"));return;}
        AStudioFlowManager* Flow=nullptr;for(TActorIterator<AStudioFlowManager> It(World.Get());It;++It){Flow=*It;break;}
        if(!Flow || !Flow->ActiveWidget){return;}auto* Language=UStudioLanguageSubsystem::Find(Flow);
        if(!Check(Language && Language->GetLanguage()==(Step>=2?TEXT("es"):Expected),TEXT("Saved language was not restored or selection did not persist"))){return;}
        if(Flow->StartupVideos->IsIntro()){Flow->StartupVideos->SkipIntroductions();return;}
        if(!Flow->StartupVideos->IsHubInteractive()){return;}auto* Widget=Flow->ActiveWidget.Get();
        if(Step==0)
        {
            if(!Check(Flow->Screen==EStudioScreen::MainMenu && Widget->StartGameText.ToString()==(Expected==TEXT("en")?TEXT("Start Game"):TEXT("Comenzar")),TEXT("Menu did not load translated Start Game text"))){return;}
            OriginalPlayer=Flow->StartupVideos->MediaPlayer;VideoTime=OriginalPlayer.IsValid()?OriginalPlayer->GetTime():FTimespan::Zero();
            if(!Check(Flow->ShowAudioSettings(),TEXT("Settings did not open"))){return;}Step=1;LastChange=FPlatformTime::Seconds();return;
        }
        if(Step==1 && FPlatformTime::Seconds()-LastChange>.4)
        {
            if(!Check(Widget->SpanishButton && Widget->EnglishButton,TEXT("Native language buttons missing"))){return;}
            Widget->SpanishButton->OnClicked.Broadcast();
            if(!Check(Language->GetLanguage()==TEXT("es") && Widget->HowToPlayText.ToString()==TEXT("Cómo jugar"),TEXT("Settings button did not refresh existing FText"))){return;}
            Step=2;LastChange=FPlatformTime::Seconds();FScreenshotRequest::RequestScreenshot(Directory/TEXT("SettingsSpanish.png"),true,false);return;
        }
        if(Step==2 && FPlatformTime::Seconds()-LastChange>.4)
        {
            if(!Check(Flow->StartupVideos->MediaPlayer==OriginalPlayer.Get() && (!OriginalPlayer.IsValid() || Flow->StartupVideos->MediaFailures>0 || OriginalPlayer->IsPlaying()),TEXT("Changing language replaced or stopped HUB video"))){return;}
            if(!Check(!Flow->Movie->HasActiveSession() && Flow->BackToMenu(),TEXT("Language changed gameplay state or menu navigation"))){return;}
            Step=3;FScreenshotRequest::RequestScreenshot(Directory/TEXT("MenuSpanish.png"),true,false);return;
        }
        if(Step==3){Finish(true,TEXT("Saved English restored in a new process; existing Settings buttons switch to Spanish, save it and refresh UI while HUB video continues without starting gameplay"));}
    }
};
void OnWorld(UWorld* World,const UWorld::InitializationValues Values)
{
    if(!World || !World->IsGameWorld() || !FParse::Param(FCommandLine::Get(),TEXT("FinalTakeLanguageSmoke"))){return;}
    auto Run=MakeShared<FRun>();Run->World=World;FParse::Value(FCommandLine::Get(),TEXT("ExpectedLanguage="),Run->Expected);Run->Directory=FPaths::ProjectSavedDir()/TEXT("Automation/LanguageMenu");
    World->GetTimerManager().SetTimer(Run->Timer,FTimerDelegate::CreateLambda([Run](){const auto Keep=Run;Keep->Tick();}),.05f,true);
}
struct FRegistration{FDelegateHandle Handle=FWorldDelegates::OnPostWorldInitialization.AddStatic(&OnWorld);~FRegistration(){FWorldDelegates::OnPostWorldInitialization.Remove(Handle);}} Registration;
}
#endif
