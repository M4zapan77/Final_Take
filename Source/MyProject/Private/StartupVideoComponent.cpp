#include "StartupVideoComponent.h"
#include "StartupVideoWidget.h"
#include "StartupTravelSubsystem.h"
#include "StudioFlowManager.h"
#include "StudioAudioSubsystem.h"
#include "FileMediaSource.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "MediaSoundComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "UObject/ConstructorHelpers.h"
#include "InputCoreTypes.h"

bool FStartupVideoSettings::IsValid() const
{
    return FMath::IsFinite(FadeSeconds) && FadeSeconds>=0 && FMath::IsFinite(OpenTimeoutSeconds) && OpenTimeoutSeconds>0
        && FMath::IsFinite(PrimeSeconds) && PrimeSeconds>=0 && FMath::IsFinite(HubDimmerOpacity) && HubDimmerOpacity>=0 && HubDimmerOpacity<=1
        && FMath::IsFinite(AudioVolume) && AudioVolume>=0;
}

UStartupVideoComponent::UStartupVideoComponent()
{
    PrimaryComponentTick.bCanEverTick=true; PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
    static ConstructorHelpers::FObjectFinder<UFileMediaSource> Studio(TEXT("/Game/OneManStudio/UI/StartupVideos/MS_StudioIntro"));
    static ConstructorHelpers::FObjectFinder<UFileMediaSource> Game(TEXT("/Game/OneManStudio/UI/StartupVideos/MS_GameIntro"));
    static ConstructorHelpers::FObjectFinder<UFileMediaSource> Hub(TEXT("/Game/OneManStudio/UI/StartupVideos/MS_HubLoop"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/OneManStudio/UI/StartupVideos/M_StartupVideoUI"));
    Settings.StudioIntro=Studio.Object; Settings.GameIntroduction=Game.Object; Settings.HubBackground=Hub.Object; Settings.VideoMaterial=Material.Object;
    Settings.StartingGameplayLevel=TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/OneManStudio/Maps/Lvl_Bank.Lvl_Bank")));
}

bool UStartupVideoComponent::IsEnabledForFlow() const
{
    if (!Settings.bEnabled) { return false; }
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(),TEXT("StudioSkipStartupVideos"))) { return false; }
#endif
    return true;
}
void UStartupVideoComponent::InitializeVideos(AStudioFlowManager* Manager,APlayerController* PlayerController)
{ Flow=Manager; Controller=PlayerController; AddTickPrerequisiteActor(Manager); }
UMaterialInstanceDynamic* UStartupVideoComponent::GetVideoMaterial() const { return VideoMID.Get(); }
bool UStartupVideoComponent::IsIntro() const { return Stage==EStartupVideoStage::StudioIntro || Stage==EStartupVideoStage::GameIntro; }
bool UStartupVideoComponent::OwnsMenuBackground() const { return IsEnabledForFlow() && Stage!=EStartupVideoStage::Inactive; }
bool UStartupVideoComponent::IsHubInteractive() const
{
    return Stage==EStartupVideoStage::Hub && Phase==EStartupVideoPhase::Playing && !bWaitForRelease && !bSkipClickHeld
        && GetWorld() && GetWorld()->GetTimeSeconds()-ReleaseTime>.08 && GFrameCounter>ReleaseFrame;
}
bool UStartupVideoComponent::IsInputBlocked() const { return Stage!=EStartupVideoStage::Inactive && !IsHubInteractive(); }

void UStartupVideoComponent::EnsurePresentation()
{
    if (!MediaTexture)
    {
        MediaTexture=NewObject<UMediaTexture>(this); MediaTexture->AutoClear=true; MediaTexture->ClearColor=FLinearColor::Black; MediaTexture->NewStyleOutput=false; MediaTexture->UpdateResource();
        if (Settings.VideoMaterial) { VideoMID=UMaterialInstanceDynamic::Create(Settings.VideoMaterial,this); VideoMID->SetTextureParameterValue(TEXT("VideoTexture"),MediaTexture); }
    }
    if (!MediaSound && GetOwner())
    {
        MediaSound=NewObject<UMediaSoundComponent>(GetOwner()); GetOwner()->AddInstanceComponent(MediaSound);
        MediaSound->SetupAttachment(GetOwner()->GetRootComponent()); MediaSound->bAutoActivate=false; MediaSound->bAllowSpatialization=false; MediaSound->bIsUISound=true;
        MediaSound->Channels=EMediaSoundChannels::Stereo; MediaSound->RegisterComponent();
    }
    APlayerController* Player=Controller.Get(); if (!Player || !Player->GetLocalPlayer()) { return; }
    if (!BackdropWidget)
    {
        BackdropWidget=CreateWidget<UStartupVideoWidget>(Player,UStartupVideoWidget::StaticClass());
        BackdropWidget->BindVideos(this,false); BackdropWidget->AddToViewport(140);
    }
    if (!OverlayWidget)
    {
        OverlayWidget=CreateWidget<UStartupVideoWidget>(Player,UStartupVideoWidget::StaticClass());
        OverlayWidget->BindVideos(this,true); OverlayWidget->AddToViewport(200);
    }
    FInputModeUIOnly Input; Input.SetWidgetToFocus(OverlayWidget->TakeWidget()); Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Player->SetInputMode(Input); Player->bShowMouseCursor=true; OverlayWidget->SetKeyboardFocus();
}

void UStartupVideoComponent::BeginSequence(bool bHubOnly)
{
    if (!IsEnabledForFlow() || bEnding) { return; }
    StopVideos(); bSkipped=false; bHubAnnounced=false; bWaitForRelease=bSkipClickHeld=false; ReleaseTime=-100; ReleaseFrame=0;
    if (!Settings.IsValid())
    {
        UE_LOG(LogTemp,Warning,TEXT("Startup video settings invalid; using safe timings."));
        Settings.FadeSeconds=.3; Settings.OpenTimeoutSeconds=8; Settings.PrimeSeconds=.1; Settings.HubDimmerOpacity=.25f; Settings.AudioVolume=1;
    }
    EnsurePresentation(); OpenStage(bHubOnly?EStartupVideoStage::Hub:EStartupVideoStage::StudioIntro);
}

void UStartupVideoComponent::CloseMedia()
{
    TGuardValue<bool> Guard(bClosing,true);
    if (MediaSound) { MediaSound->SetVolumeMultiplier(0); MediaSound->Stop(); MediaSound->SetMediaPlayer(nullptr); MediaSound->UpdatePlayer(); }
    if (MediaPlayer)
    {
        MediaPlayer->OnMediaOpened.RemoveDynamic(this,&UStartupVideoComponent::MediaOpened);
        MediaPlayer->OnMediaOpenFailed.RemoveDynamic(this,&UStartupVideoComponent::MediaFailed);
        MediaPlayer->OnEndReached.RemoveDynamic(this,&UStartupVideoComponent::MediaEnded);
        MediaPlayer->Close(); MediaPlayer=nullptr;
    }
    if (MediaTexture) { MediaTexture->SetMediaPlayer(nullptr); }
    bOpened=false; bHasVideoFrame=false;
}

void UStartupVideoComponent::OpenStage(EStartupVideoStage NewStage)
{
    CloseMedia(); ++PlaybackGeneration; const uint32 Generation=PlaybackGeneration;
    Stage=NewStage; Phase=EStartupVideoPhase::Opening; BlackOpacity=1; PhaseStarted=GetWorld()->GetTimeSeconds(); LastProgress=PhaseStarted; LastMediaTime=0;
    OnStageChanged.Broadcast(Stage); if (Generation!=PlaybackGeneration || bEnding) { return; }
    if (Stage==EStartupVideoStage::Hub)
    { if (auto* Travel=UStartupTravelSubsystem::Find(this)) { Travel->MarkIntroductionsSeen(); } }
    UFileMediaSource* Source=Stage==EStartupVideoStage::StudioIntro?Settings.StudioIntro.Get():Stage==EStartupVideoStage::GameIntro?Settings.GameIntroduction.Get():Settings.HubBackground.Get();
    if (!Source || !Source->Validate() || !VideoMID) { FailedMedia(TEXT("Missing/invalid source or UI material")); return; }
    MediaPlayer=NewObject<UMediaPlayer>(this); MediaPlayer->PlayOnOpen=false; MediaPlayer->NativeAudioOut=false;
    MediaPlayer->OnMediaOpened.AddDynamic(this,&UStartupVideoComponent::MediaOpened);
    MediaPlayer->OnMediaOpenFailed.AddDynamic(this,&UStartupVideoComponent::MediaFailed);
    MediaPlayer->OnEndReached.AddDynamic(this,&UStartupVideoComponent::MediaEnded);
    MediaTexture->SetMediaPlayer(MediaPlayer); MediaSound->SetMediaPlayer(MediaPlayer); MediaSound->UpdatePlayer();
    UE_LOG(LogTemp,Display,TEXT("StartupVideo: opening stage %d: %s"),static_cast<int32>(Stage),*Source->GetUrl());
    if (!MediaPlayer->OpenSource(Source)) { FailedMedia(TEXT("OpenSource rejected the file")); }
    UpdatePresentation();
}

void UStartupVideoComponent::MediaOpened(FString Url)
{
    if (bClosing || bEnding || !MediaPlayer || Phase!=EStartupVideoPhase::Opening) { return; }
    bOpened=true; MediaPlayer->SetLooping(Stage==EStartupVideoStage::Hub);
}
void UStartupVideoComponent::MediaFailed(FString Url) { if (!bClosing && !bEnding) { FailedMedia(TEXT("Decoder could not open the source")); } }
void UStartupVideoComponent::MediaEnded()
{
    if (bClosing || bEnding || !MediaPlayer) { return; }
    if (Stage==EStartupVideoStage::Hub)
    {
        const double Now=GetWorld()->GetTimeSeconds(); if (Now-LastLoopTime>.25) { ++HubLoops; LastLoopTime=Now; }
        if (!MediaPlayer->IsLooping()) { MediaPlayer->Seek(FTimespan::Zero()); MediaPlayer->Play(); }
    }
    // An end notification can arrive during a user-requested skip fade. Preserve that target.
    else if (IsIntro() && Phase!=EStartupVideoPhase::FadeOut) { FadeTo(Stage==EStartupVideoStage::StudioIntro?EStartupVideoStage::GameIntro:EStartupVideoStage::Hub); }
}

void UStartupVideoComponent::StartFadeIn()
{
    Phase=EStartupVideoPhase::FadeIn; PhaseStarted=GetWorld()->GetTimeSeconds(); BlackOpacity=1;
    if (Stage==EStartupVideoStage::Hub) { OnHubPrepared.Broadcast(); }
}
void UStartupVideoComponent::FailedMedia(const TCHAR* Reason)
{
    ++MediaFailures;
    UE_LOG(LogTemp,Warning,TEXT("StartupVideo: stage %d failed: %s. Continuing."),static_cast<int32>(Stage),Reason);
    const FText Message=FText::FromString(Reason); const uint32 Generation=PlaybackGeneration;
    OnMediaIssue.Broadcast(Message); if (Generation!=PlaybackGeneration || bEnding) { return; }
    if (Stage==EStartupVideoStage::StudioIntro && !bSkipped) { OpenStage(EStartupVideoStage::GameIntro); }
    else if (IsIntro()) { OpenStage(EStartupVideoStage::Hub); }
    else if (Stage==EStartupVideoStage::Hub) { CloseMedia(); StartFadeIn(); }
}
void UStartupVideoComponent::FadeTo(EStartupVideoStage Target)
{
    NextStage=Target; FadeStartOpacity=BlackOpacity; Phase=EStartupVideoPhase::FadeOut; PhaseStarted=GetWorld()->GetTimeSeconds();
}
bool UStartupVideoComponent::SkipIntroductions()
{
    if (!IsIntro() || bEnding) { return false; }
    if(bSkipped && Phase==EStartupVideoPhase::FadeOut && NextStage==EStartupVideoStage::Hub){return true;}
    bSkipped=true; FadeTo(EStartupVideoStage::Hub); return true;
}
void UStartupVideoComponent::IntroPointerDown()
{
    if (IsIntro()) { bSkipClickHeld=true; bWaitForRelease=true; SkipIntroductions(); }
}
void UStartupVideoComponent::IntroPointerUp()
{
    bSkipClickHeld=false; bWaitForRelease=false; ReleaseFrame=GFrameCounter; ReleaseTime=GetWorld()?GetWorld()->GetTimeSeconds():0;
}
bool UStartupVideoComponent::RequestStartGame()
{
    if (!IsHubInteractive()) { return false; }
    const FString Package=Settings.StartingGameplayLevel.ToSoftObjectPath().GetLongPackageName();
    if (Package.IsEmpty() || !FPackageName::IsValidLongPackageName(Package) || !FPackageName::DoesPackageExist(Package))
    { OnMediaIssue.Broadcast(NSLOCTEXT("FinalTakeUI","StartupVideoComponent.30552e20c9","Starting gameplay level is missing or not cooked.")); return false; }
    Stage=EStartupVideoStage::StartingGame; FadeTo(EStartupVideoStage::StartingGame); OnStageChanged.Broadcast(Stage); return true;
}

void UStartupVideoComponent::UpdatePresentation()
{
    if (MediaSound) { MediaSound->SetVolumeMultiplier((Settings.bMuteHubAudio && (Stage==EStartupVideoStage::Hub || Stage==EStartupVideoStage::StartingGame)?0.f:Settings.AudioVolume)*UStudioAudioSubsystem::MusicMultiplier(this)*(1.f-BlackOpacity)); }
    if (BackdropWidget) { BackdropWidget->RefreshVideo(); }
    if (OverlayWidget) { OverlayWidget->RefreshVideo(); }
}

void UStartupVideoComponent::TickComponent(float Delta,ELevelTick TickType,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,TickType,Function);
    if (Stage==EStartupVideoStage::Inactive || bEnding) { return; }
    const double Now=GetWorld()->GetTimeSeconds(),Elapsed=Now-PhaseStarted;
    if (bSkipClickHeld && FSlateApplication::IsInitialized() && !FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::LeftMouseButton)) { IntroPointerUp(); }
    if (Phase==EStartupVideoPhase::Opening)
    {
        if (bOpened && MediaPlayer && MediaPlayer->GetNumTracks(EMediaPlayerTrack::Video)>0)
        {
            const FIntPoint Dimensions=MediaPlayer->GetVideoTrackDimensions(0,INDEX_NONE);
            const float TrackAspect=MediaPlayer->GetVideoTrackAspectRatio(0,INDEX_NONE);
            if (TrackAspect>0 && FMath::IsFinite(TrackAspect)) { AspectRatio=TrackAspect; }
            else if (Dimensions.Y>0) { AspectRatio=static_cast<float>(Dimensions.X)/Dimensions.Y; }
            if (MediaPlayer->Play())
            {
                Phase=EStartupVideoPhase::Priming; PhaseStarted=Now; LastProgress=Now;
                MediaSound->Start();
            }
        }
        if (Phase==EStartupVideoPhase::Opening && Elapsed>=Settings.OpenTimeoutSeconds) { FailedMedia(TEXT("Media open/track timeout")); }
    }
    else if (Phase==EStartupVideoPhase::Priming)
    {
        if (MediaPlayer && MediaPlayer->IsPlaying() && MediaPlayer->GetTime().GetTotalSeconds()>.02 && Elapsed>=Settings.PrimeSeconds)
        { bHasVideoFrame=true; StartFadeIn(); }
        else if (Elapsed>=Settings.OpenTimeoutSeconds) { FailedMedia(TEXT("No decoded video progress")); }
    }
    else if (Phase==EStartupVideoPhase::FadeIn)
    {
        BlackOpacity=Settings.FadeSeconds>0?static_cast<float>(1-FMath::Clamp(Elapsed/Settings.FadeSeconds,0.0,1.0)):0.f;
        if (BlackOpacity<=0) { Phase=EStartupVideoPhase::Playing; }
    }
    else if (Phase==EStartupVideoPhase::FadeOut)
    {
        const double Alpha=Settings.FadeSeconds>0?FMath::Clamp(Elapsed/Settings.FadeSeconds,0.0,1.0):1.0;
        BlackOpacity=static_cast<float>(FMath::Lerp(FadeStartOpacity,1.0,Alpha));
        if (Alpha>=1)
        {
            if (NextStage==EStartupVideoStage::StartingGame) { CloseMedia(); Phase=EStartupVideoPhase::Stopped; OnGameplayBlack.Broadcast(); }
            else { OpenStage(NextStage); }
        }
    }
    if (MediaPlayer && Phase==EStartupVideoPhase::Playing)
    {
        const double Time=MediaPlayer->GetTime().GetTotalSeconds();
        if (Stage==EStartupVideoStage::Hub && Time+.1<LastMediaTime && Now-LastLoopTime>.25) { ++HubLoops; LastLoopTime=Now; }
        if (FMath::Abs(Time-LastMediaTime)>.001) { LastProgress=Now; }
        else if (Now-LastProgress>Settings.OpenTimeoutSeconds) { FailedMedia(TEXT("Playback stalled")); }
        LastMediaTime=Time;
    }
    if (IsHubInteractive() && !bHubAnnounced) { bHubAnnounced=true; OnHubInteractive.Broadcast(); }
    UpdatePresentation();
}

void UStartupVideoComponent::StopVideos()
{
    ++PlaybackGeneration; CloseMedia(); Stage=EStartupVideoStage::Inactive; Phase=EStartupVideoPhase::Stopped; bHubAnnounced=false;
    for (UStartupVideoWidget* Widget:{BackdropWidget.Get(),OverlayWidget.Get()}) { if (Widget) { Widget->SetIsEnabled(false); Widget->RemoveFromParent(); } }
    BackdropWidget=nullptr; OverlayWidget=nullptr;
}
void UStartupVideoComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    bEnding=true; StopVideos();
    if (MediaSound) { GetOwner()->RemoveInstanceComponent(MediaSound); MediaSound->DestroyComponent(); MediaSound=nullptr; }
    Super::EndPlay(Reason);
}
