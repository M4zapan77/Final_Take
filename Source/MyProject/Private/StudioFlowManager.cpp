#include "StudioFlowManager.h"
#include "StorySessionComponent.h"
#include "StudioFlowWidget.h"
#include "StudioAudioSubsystem.h"
#include "StudioAudioProfile.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "StartupVideoComponent.h"
#include "StartupTravelSubsystem.h"
#include "ActingComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "Camera/CameraActor.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"

AStudioFlowManager::AStudioFlowManager()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickGroup=TG_PostUpdateWork;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("StudioRoot")));
    StartupVideos=CreateDefaultSubobject<UStartupVideoComponent>(TEXT("StartupVideos"));
    BackgroundMusic=CreateDefaultSubobject<UAudioComponent>(TEXT("StudioMusic"));
    DirectorVoice=CreateDefaultSubobject<UAudioComponent>(TEXT("DirectorVoice"));
    ButtonAudio=CreateDefaultSubobject<UAudioComponent>(TEXT("StudioButtons"));
    AmbientAudio=CreateDefaultSubobject<UAudioComponent>(TEXT("StudioAmbience"));
    SetEffectsAudio=CreateDefaultSubobject<UAudioComponent>(TEXT("ClapperAndAudience"));
    for (UAudioComponent* Audio:{BackgroundMusic.Get(),DirectorVoice.Get(),ButtonAudio.Get(),AmbientAudio.Get(),SetEffectsAudio.Get()})
    { Audio->SetupAttachment(RootComponent); Audio->bAutoActivate=false; Audio->bAllowSpatialization=false; Audio->bIsUISound=true; }
    WidgetClass=UStudioFlowWidget::StaticClass();
}

AStudioFlowManager* AStudioFlowManager::Find(const UObject* Context)
{
    UWorld* World=Context?Context->GetWorld():nullptr;
    if (!World) { return nullptr; }
    for (TActorIterator<AStudioFlowManager> It(World);It;++It) { if (It->bInitialized && !It->IsActorBeingDestroyed()) { return *It; } }
    return nullptr;
}

bool AStudioFlowManager::InitializeFlow(AMovieManager* Manager,APlayerController* Player)
{
    if (bInitialized || !IsValid(Manager) || !IsValid(Player) || !Player->GetPawn() || !Settings.IsValid()
        || Manager->GetWorld()!=GetWorld() || Player->GetWorld()!=GetWorld()) { return false; }
    Movie=Manager; Controller=Player; bInitialized=true;
    if (Movie->AudioProfile) { Movie->AudioProfile->ConfigureFlow(Settings); }
    if (Settings.MenuMusic) { StartupVideos->Settings.bMuteHubAudio=true; }
    AddTickPrerequisiteActor(Movie);
    Movie->bUseTakeTransitions=true; Movie->BetweenActorsSeconds=Settings.BetweenActorsSeconds; Movie->WrapSeconds=Settings.WrapSeconds;
    Movie->OnStateChanged.AddUniqueDynamic(this,&AStudioFlowManager::HandleMovieState);
    Movie->OnPreparationStarted.AddUniqueDynamic(this,&AStudioFlowManager::HandlePreparation);
    Movie->OnRecordingStarted.AddUniqueDynamic(this,&AStudioFlowManager::HandleRecording);
    Movie->OnTakeRecorded.AddUniqueDynamic(this,&AStudioFlowManager::HandleTakeRecorded);
    Movie->OnMovieError.AddUniqueDynamic(this,&AStudioFlowManager::HandleMovieError);
    if (auto* Audio=UStudioAudioSubsystem::Find(this)) { Audio->InitializeDefaults(Settings.DefaultAudio); }
    if (auto* Clock=GetWorld()->GetSubsystem<UActingSceneClockSubsystem>()) { Clock->StopClock(); }
    StartupVideos->InitializeVideos(this,Player);
    bStartupPresentation=StartupVideos->IsEnabledForFlow() && Player->GetLocalPlayer()!=nullptr;
    StartupVideos->OnHubPrepared.AddUniqueDynamic(this,&AStudioFlowManager::HandleHubPrepared);
    StartupVideos->OnHubInteractive.AddUniqueDynamic(this,&AStudioFlowManager::HandleHubInteractive);
    StartupVideos->OnGameplayBlack.AddUniqueDynamic(this,&AStudioFlowManager::HandleGameplayBlack);
    HoldFrontEnd();
    UStartupTravelSubsystem* Travel=UStartupTravelSubsystem::Find(this);
    if (Travel && Travel->ConsumeGameplayIntent())
    {
        SetScreen(EStudioScreen::MovieIntroduction);
        if(Movie->bUseChronologicalGameplay)
        {SetMusic(Settings.IntroductionMusic?Settings.IntroductionMusic.Get():Settings.MenuMusic.Get());Travel->MarkGameplayReady();return EnsureWidget();}
        const bool bStarted=BeginIntroducedMovie();
        Travel->MarkGameplayReady();
        return bStarted;
    }
    if (bStartupPresentation)
    {
        SetScreen(EStudioScreen::StartupVideos);
        StartupVideos->BeginSequence(Travel && Travel->HaveIntroductionsPlayed());
    }
    else { SetScreen(EStudioScreen::MainMenu); SetMusic(Settings.MenuMusic); }
    return EnsureWidget();
}

bool AStudioFlowManager::EnsureWidget()
{
    APlayerController* Player=Controller.Get();
    if (!Player || !Player->GetLocalPlayer() || !GEngine || !GEngine->GameViewport) { return true; }
    if (!ActiveWidget)
    {
        ActiveWidget=CreateWidget<UStudioFlowWidget>(Player,WidgetClass?WidgetClass.Get():UStudioFlowWidget::StaticClass());
        if (!ActiveWidget) { return false; }
        ActiveWidget->BindFlow(this); ActiveWidget->AddToViewport(150);
    }
    ActiveWidget->RefreshFlow();
    if (IsFrontEndOpen() && Screen!=EStudioScreen::StartupVideos && Screen!=EStudioScreen::StartingGame)
    {
        FInputModeUIOnly Input; Input.SetWidgetToFocus(ActiveWidget->TakeWidget()); Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Player->SetInputMode(Input); Player->bShowMouseCursor=true; Player->bEnableClickEvents=true; Player->bEnableMouseOverEvents=true;
        ActiveWidget->FocusPage();
    }
    return true;
}

bool AStudioFlowManager::IsFrontEndOpen() const
{
    return Screen==EStudioScreen::MainMenu || Screen==EStudioScreen::HowToPlay || Screen==EStudioScreen::Credits
        || Screen==EStudioScreen::AudioSettings || Screen==EStudioScreen::MovieIntroduction || Screen==EStudioScreen::Error
        || Screen==EStudioScreen::StartupVideos || Screen==EStudioScreen::StartingGame;
}

void AStudioFlowManager::HoldFrontEnd()
{
    APlayerController* Player=Controller.Get();
    if (bMenuHeld || !Player || !Player->GetPawn()) { return; }
    MenuPawn=Player->GetPawn(); SavedView=Player->GetViewTarget(); SavedRotation=Player->GetControlRotation();
    bSavedCursor=Player->bShowMouseCursor; bSavedClick=Player->bEnableClickEvents; bSavedMouseOver=Player->bEnableMouseOverEvents;
    bSavedHidden=MenuPawn->IsHidden(); bSavedCollision=MenuPawn->GetActorEnableCollision();
    SavedHUD=Player->GetHUD(); bSavedHUDVisible=SavedHUD.IsValid()?SavedHUD->bShowHUD:true;
    SavedMovement=MenuPawn->GetMovementComponent(); bSavedMovementActive=SavedMovement.IsValid() && SavedMovement->IsActive();
    if (auto* Character=Cast<ACharacter>(MenuPawn.Get())) { Character->StopJumping(); }
    MenuPawn->ConsumeMovementInputVector();
    if (auto* Move=Cast<UCharacterMovementComponent>(SavedMovement.Get()))
    { SavedMovementMode=Move->MovementMode; SavedCustomMode=Move->CustomMovementMode; Move->DisableMovement(); }
    if (SavedMovement.IsValid()) { SavedMovement->StopMovementImmediately(); SavedMovement->Deactivate(); }
    SavedActing=MenuPawn->FindComponentByClass<UActingComponent>();
    if (SavedActing.IsValid()) { bSavedActions=SavedActing->AreLiveActionsEnabled(); SavedActing->StopAllActions(); SavedActing->SetLiveActionsEnabled(false); }
    Player->SetIgnoreMoveInput(true); Player->SetIgnoreLookInput(true); bMenuHeld=true;
    MenuPawn->SetActorHiddenInGame(true); MenuPawn->SetActorEnableCollision(false);
    if (SavedHUD.IsValid()) { SavedHUD->bShowHUD=false; }
    if (Movie && IsValid(Movie->PremiereCamera)) { Player->SetViewTargetWithBlend(Movie->PremiereCamera,0.f); }
}

void AStudioFlowManager::ReleaseFrontEnd()
{
    if (!bMenuHeld) { return; }
    if (APlayerController* Player=Controller.Get())
    {
        Player->SetIgnoreMoveInput(false); Player->SetIgnoreLookInput(false);
        Player->bShowMouseCursor=bSavedCursor; Player->bEnableClickEvents=bSavedClick; Player->bEnableMouseOverEvents=bSavedMouseOver;
        if (Player->GetLocalPlayer()) { Player->SetInputMode(FInputModeGameOnly()); }
        if (SavedView.IsValid()) { Player->SetViewTargetWithBlend(SavedView.Get(),0.f); }
        Player->SetControlRotation(SavedRotation);
    }
    if (MenuPawn.IsValid()) { MenuPawn->SetActorHiddenInGame(bSavedHidden); MenuPawn->SetActorEnableCollision(bSavedCollision); }
    if (auto* Move=Cast<UCharacterMovementComponent>(SavedMovement.Get())) { Move->SetMovementMode(static_cast<EMovementMode>(SavedMovementMode),SavedCustomMode); }
    if (SavedMovement.IsValid() && bSavedMovementActive) { SavedMovement->Activate(true); }
    if (SavedHUD.IsValid()) { SavedHUD->bShowHUD=bSavedHUDVisible; }
    if (SavedActing.IsValid()) { SavedActing->SetLiveActionsEnabled(bSavedActions); }
    bMenuHeld=false; MenuPawn.Reset(); SavedView.Reset(); SavedMovement.Reset(); SavedHUD.Reset(); SavedActing.Reset();
}

void AStudioFlowManager::SetScreen(EStudioScreen NewScreen)
{
    if (Screen==NewScreen && Revision>0) { return; }
    const EStudioScreen Previous=Screen; Screen=NewScreen; Revision=Revision==MAX_int32?1:Revision+1;
    PageStarted=GetWorld()?GetWorld()->GetTimeSeconds():0.0;
    EnsureWidget(); OnScreenChanged.Broadcast(Previous,Screen);
}

void AStudioFlowManager::SetMusic(USoundBase* Sound)
{
    if (bStartupPresentation && (Screen==EStudioScreen::StartupVideos || Screen==EStudioScreen::StartingGame)) { Sound=nullptr; }
    DesiredMusic=Sound;
    MusicStateVolume=.72f;
}
bool AStudioFlowManager::UsesUnifiedAudio() const { return bInitialized && Movie && Movie->AudioProfile; }
void AStudioFlowManager::PlayPresentationMusic(USoundBase* Sound,float Volume)
{
    SetMusic(Sound);MusicStateVolume=.72f*FMath::Max(0.f,Volume);
}
void AStudioFlowManager::UpdateMusic(float DeltaSeconds)
{
    const float Step=DeltaSeconds/FMath::Max(.05f,Settings.MusicFadeSeconds);
    const bool Changing=BackgroundMusic->Sound!=DesiredMusic;
    MusicEnvelope=FMath::Clamp(MusicEnvelope+(Changing?-Step:Step),0.f,1.f);
    if (Changing && MusicEnvelope<=0)
    {
        if (BackgroundMusic->Sound && BackgroundMusic->Sound==Settings.WardrobeMusic)
        {
            const auto* Wave=Cast<USoundWave>(BackgroundMusic->Sound);
            const float Length=Wave?Wave->Duration:BackgroundMusic->Sound->GetDuration();
            const float Position=MusicStartPosition+static_cast<float>(GetWorld()->GetTimeSeconds()-MusicStartedAt);
            MusicPositions.Add(BackgroundMusic->Sound,Length>0?FMath::Fmod(Position,Length):0.f);
        }
        BackgroundMusic->Stop(); BackgroundMusic->SetSound(DesiredMusic);
        if (DesiredMusic)
        {
            MusicStartPosition=DesiredMusic==Settings.WardrobeMusic?MusicPositions.FindRef(DesiredMusic):0.f;
            MusicStartedAt=GetWorld()->GetTimeSeconds(); BackgroundMusic->SetVolumeMultiplier(0);
            BackgroundMusic->Play(MusicStartPosition);
        }
    }
    const float Duck=DirectorVoice->IsPlaying()?.48f:1.f;
    const float VideoFade=bStartupPresentation && Screen==EStudioScreen::StartingGame?1.f-StartupVideos->BlackOpacity:1.f;
    BackgroundMusic->SetVolumeMultiplier(MusicStateVolume*MusicEnvelope*Duck*VideoFade*UStudioAudioSubsystem::MusicMultiplier(this));
}
void AStudioFlowManager::SetAmbience(USoundBase* Sound)
{
    if (AmbientAudio->Sound==Sound) { return; }
    AmbientAudio->Stop(); AmbientAudio->SetSound(Sound);
    if (Sound) { AmbientAudio->FadeIn(.4f,1.f); }
}
void AStudioFlowManager::StopDirector()
{
    DirectorVoice->Stop(); DirectorVoice->SetSound(nullptr); DirectorLine=FText::GetEmpty(); CueStarted=-1; CueDuration=0;
}
void AStudioFlowManager::Announce(EDirectorCue Cue)
{
    StopDirector(); LastAnnouncement=Cue; ++AnnouncementSerial;
    const auto* Item=Settings.Announcements.FindByPredicate([Cue](const FDirectorAnnouncement& Entry){return Entry.Cue==Cue;});
    if (Item)
    {
        DirectorLine=Item->Text; CueStarted=GetWorld()->GetTimeSeconds(); CueDuration=Item->DurationSeconds; CueVolume=Item->Volume;
        if (Item->Sound && Item->DurationSeconds>0) { DirectorVoice->SetSound(Item->Sound); DirectorVoice->SetVolumeMultiplier(CueVolume*UStudioAudioSubsystem::EffectsMultiplier(this)); DirectorVoice->Play(); }
    }
    OnDirectorAnnouncement.Broadcast(Cue,DirectorLine);
}
void AStudioFlowManager::PlayClick()
{
    if (Settings.ButtonSound) { ButtonAudio->SetSound(Settings.ButtonSound); ButtonAudio->SetVolumeMultiplier(UStudioAudioSubsystem::UIMultiplier(this)); ButtonAudio->Play(); }
}
bool AStudioFlowManager::PlayMovie()
{
    if (!bInitialized || bNavigating || Screen!=EStudioScreen::MainMenu) { return false; }
    if (bStartupPresentation)
    {
        if (!StartupVideos->RequestStartGame()) { return false; }
        SetScreen(EStudioScreen::StartingGame); return true;
    }
    TGuardValue<bool> Guard(bNavigating,true); FlowError=FText::GetEmpty();
    SetScreen(EStudioScreen::MovieIntroduction); SetMusic(Settings.IntroductionMusic?Settings.IntroductionMusic.Get():Settings.MenuMusic.Get()); PlayClick(); return true;
}
bool AStudioFlowManager::BeginIntroducedMovie()
{
    if (!bInitialized || bNavigating || Screen!=EStudioScreen::MovieIntroduction || !Movie) { return false; }
    TGuardValue<bool> Guard(bNavigating,true); StopDirector(); SetMusic(nullptr); ReleaseFrontEnd();
    const bool bStarted=Movie->StartMovie();
    if (!bStarted && Screen!=EStudioScreen::Error) { HandleMovieError(Movie->LastError); }
    if (bStarted) { PlayClick(); } return bStarted;
}
bool AStudioFlowManager::ShowMenuPage(EStudioScreen NewScreen)
{
    if (!bInitialized || bNavigating || Screen!=EStudioScreen::MainMenu) { return false; }
    if (bStartupPresentation && !StartupVideos->IsHubInteractive()) { return false; }
    TGuardValue<bool> Guard(bNavigating,true); SetScreen(NewScreen); PlayClick(); return true;
}
bool AStudioFlowManager::ShowHowToPlay() { return ShowMenuPage(EStudioScreen::HowToPlay); }
bool AStudioFlowManager::ShowCredits() { return ShowMenuPage(EStudioScreen::Credits); }
bool AStudioFlowManager::ShowAudioSettings() { return ShowMenuPage(EStudioScreen::AudioSettings); }
bool AStudioFlowManager::BackToMenu()
{
    if (!bInitialized || bNavigating || !IsFrontEndOpen() || Screen==EStudioScreen::MainMenu) { return false; }
    if(Screen==EStudioScreen::MovieIntroduction && bStartupPresentation){return ReturnToMainMenu();}
    if (bStartupPresentation && !StartupVideos->IsHubInteractive()) { return false; }
    TGuardValue<bool> Guard(bNavigating,true); SetScreen(EStudioScreen::MainMenu); SetMusic(Settings.MenuMusic); PlayClick(); return true;
}
bool AStudioFlowManager::ReturnToMainMenu()
{
    if (!bInitialized || bNavigating || !Movie) { return false; }
    TGuardValue<bool> Guard(bNavigating,true);
    Movie->StopMovie(); StopDirector(); ButtonAudio->Stop(); ButtonAudio->SetSound(nullptr);
    SetAmbience(nullptr); SetEffectsAudio->Stop();
    if (auto* Clock=GetWorld()->GetSubsystem<UActingSceneClockSubsystem>()) { Clock->StopClock(); }
    FlowError=FText::GetEmpty(); bTransitionCueSent=false; HoldFrontEnd();
    if (bStartupPresentation) { SetScreen(EStudioScreen::StartupVideos); StartupVideos->BeginSequence(true); }
    else { SetScreen(EStudioScreen::MainMenu); SetMusic(Settings.MenuMusic); }
    return true;
}
bool AStudioFlowManager::QuitGame()
{
    if (!bInitialized || bNavigating || Screen!=EStudioScreen::MainMenu || !Controller.IsValid()) { return false; }
    if (bStartupPresentation && !StartupVideos->IsHubInteractive()) { return false; }
    TGuardValue<bool> Guard(bNavigating,true); Movie->StopMovie(); SetMusic(nullptr); StopDirector(); ButtonAudio->Stop();
    StartupVideos->StopVideos();
    UKismetSystemLibrary::QuitGame(this,Controller.Get(),EQuitPreference::Quit,false); return true;
}
void AStudioFlowManager::SetAudioLevels(FStudioAudioLevels Levels)
{ if (Screen==EStudioScreen::AudioSettings) { if (auto* Audio=UStudioAudioSubsystem::Find(this)) { Audio->SetLevels(Levels); } } }

void AStudioFlowManager::HandleMovieState(EMovieState Previous,EMovieState Current)
{
    if (Current==EMovieState::Preparation) { SetScreen(EStudioScreen::Wardrobe); SetMusic(Settings.WardrobeMusic); SetAmbience(nullptr); SetEffectsAudio->Stop(); }
    else if (Current==EMovieState::Recording) { SetScreen(EStudioScreen::Recording); SetMusic(Settings.RecordingMusic); SetAmbience(Settings.BankAmbience); }
    else if (Current==EMovieState::TakeTransition) { bTransitionCueSent=false; SetScreen(EStudioScreen::NextCharacter); SetAmbience(nullptr); }
    else if (Current==EMovieState::MoviePremiere) { StopDirector(); SetMusic(nullptr); SetAmbience(Settings.CinemaAmbience); SetScreen(EStudioScreen::Premiere); }
    else if (Current==EMovieState::Finished)
    {
        StopDirector(); SetMusic(nullptr); SetAmbience(Settings.CinemaAmbience); SetScreen(EStudioScreen::Results);
        if (Movie->AudioProfile && Movie->AudioProfile->Applause) { SetEffectsAudio->SetSound(Movie->AudioProfile->Applause); SetEffectsAudio->Play(); }
    }
    else if (Current==EMovieState::Idle) { StopDirector(); SetMusic(nullptr); SetAmbience(nullptr); SetEffectsAudio->Stop(); }
}
void AStudioFlowManager::HandlePreparation(int32 TakeNumber,FName RoleId,double Duration) { Announce(EDirectorCue::ChooseCostume); }
void AStudioFlowManager::HandleRecording(int32 TakeNumber,FName RoleId,double Duration)
{
    Announce(EDirectorCue::Action);
    if (Settings.ClapperSound) { SetEffectsAudio->SetSound(Settings.ClapperSound); SetEffectsAudio->Play(); }
}
void AStudioFlowManager::HandleTakeRecorded(const FMoviePerformance& Performance) { Announce(EDirectorCue::Cut); }
void AStudioFlowManager::HandleMovieError(FText Error)
{ FlowError=Error; StopDirector(); SetMusic(nullptr); HoldFrontEnd(); SetScreen(EStudioScreen::Error); }

void AStudioFlowManager::HandleHubPrepared() { SetScreen(EStudioScreen::MainMenu); }
void AStudioFlowManager::HandleHubInteractive() { SetMusic(Settings.MenuMusic); EnsureWidget(); }
void AStudioFlowManager::HandleGameplayBlack()
{
    Movie->StopMovie(); StopDirector(); SetMusic(nullptr); ButtonAudio->Stop(); ButtonAudio->SetSound(nullptr);
    auto* Travel=UStartupTravelSubsystem::Find(this);
    if (Travel && Travel->BeginGameplayTravel(StartupVideos->Settings.StartingGameplayLevel,StartupVideos->Settings.FadeSeconds))
    { StartupVideos->StopVideos(); }
    else { FlowError=NSLOCTEXT("FinalTakeUI","StudioFlowManager.6bb6936865","Could not load the starting level."); SetScreen(EStudioScreen::StartupVideos); StartupVideos->BeginSequence(true); }
}

FStudioFlowView AStudioFlowManager::GetView() const
{
    FStudioFlowView View; View.Screen=Screen; View.Revision=Revision; View.GameTitle=Settings.GameTitle;
    View.Synopsis=Settings.MovieSynopsis; View.HowToPlay=Settings.HowToPlay; View.Credits=Settings.Credits; View.Error=FlowError;
    if (Movie)
    {
        if(Movie->bUseChronologicalGameplay){for(const UStoryScenarioData* Entry:Movie->ScenarioLibrary){if(Entry && (Movie->SelectedScenarioId.IsNone() || Entry->Definition.Id==Movie->SelectedScenarioId)){View.Synopsis=Entry->Definition.Synopsis;break;}}}
        View.MovieTitle=Movie->ResultsSettings.MovieTitle; View.CurrentRole=Movie->GetCurrentRoleName(); View.CurrentScript=Movie->GetCurrentScriptInstruction();
        View.RoundNumber=Movie->GetRoundNumber();View.bChronologicalMovie=Movie->IsChronologicalMovie();
        View.TakeNumber=Movie->GetCurrentTakeNumber(); View.RemainingSeconds=Movie->GetPhaseRemainingSeconds();
        for (int32 Index=0;Index<Movie->Roles.Num();++Index)
        {const auto& Definition=Movie->Roles[Index];FStudioRoleBrief& Brief=View.Roles.AddDefaulted_GetRef();Brief.RoleId=Definition.RoleId;Brief.Name=Movie->bUseChronologicalGameplay?UStorySessionComponent::RoleName(Index):Definition.DisplayName;Brief.Script=Movie->bUseChronologicalGameplay?UStorySessionComponent::RoleBrief(Index):Definition.ScriptInstruction;}
    }
    if (const auto* Audio=UStudioAudioSubsystem::Find(this)) { View.Audio=Audio->GetLevels(); }
    const double Now=GetWorld()?GetWorld()->GetTimeSeconds():0.0;
    View.PageOpacity=Settings.PageFadeSeconds>0?static_cast<float>(FMath::Clamp((Now-PageStarted)/Settings.PageFadeSeconds,0.0,1.0)):1.f;
    if (CueStarted>=0 && CueDuration>0 && Now<CueStarted+CueDuration)
    { View.DirectorText=DirectorLine; View.DirectorOpacity=static_cast<float>(FMath::Clamp((CueStarted+CueDuration-Now)/FMath::Min(.25,CueDuration),0.0,1.0)); }
    return View;
}

void AStudioFlowManager::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); if (!bInitialized) { return; }
    if (Movie && Movie->State==EMovieState::TakeTransition && !bTransitionCueSent
        && Movie->GetPhaseElapsedSeconds()>=FMath::Min(Settings.NextActorCueDelay,Movie->GetPhaseDurationSeconds()*.5))
    { bTransitionCueSent=true; Announce(Movie->GetCurrentTakeNumber()>0?EDirectorCue::NextActor:EDirectorCue::Wrap); }
    if (CueStarted>=0 && GetWorld()->GetTimeSeconds()>=CueStarted+CueDuration) { StopDirector(); }
    UpdateMusic(DeltaSeconds);
    AmbientAudio->SetVolumeMultiplier(UStudioAudioSubsystem::AmbientMultiplier(this));
    SetEffectsAudio->SetVolumeMultiplier(.8f*UStudioAudioSubsystem::EffectsMultiplier(this));
    DirectorVoice->SetVolumeMultiplier(CueVolume*UStudioAudioSubsystem::EffectsMultiplier(this));
    ButtonAudio->SetVolumeMultiplier(UStudioAudioSubsystem::UIMultiplier(this));
    if (ActiveWidget) { ActiveWidget->RefreshFlow(); }
}

void AStudioFlowManager::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Movie)
    {
        Movie->OnStateChanged.RemoveDynamic(this,&AStudioFlowManager::HandleMovieState);
        Movie->OnPreparationStarted.RemoveDynamic(this,&AStudioFlowManager::HandlePreparation);
        Movie->OnRecordingStarted.RemoveDynamic(this,&AStudioFlowManager::HandleRecording);
        Movie->OnTakeRecorded.RemoveDynamic(this,&AStudioFlowManager::HandleTakeRecorded);
        Movie->OnMovieError.RemoveDynamic(this,&AStudioFlowManager::HandleMovieError);
    }
    if (ActiveWidget) { ActiveWidget->SetIsEnabled(false); ActiveWidget->RemoveFromParent(); ActiveWidget=nullptr; }
    StartupVideos->StopVideos();
    BackgroundMusic->Stop(); BackgroundMusic->SetSound(nullptr); StopDirector(); ButtonAudio->Stop(); ButtonAudio->SetSound(nullptr);
    AmbientAudio->Stop(); AmbientAudio->SetSound(nullptr); SetEffectsAudio->Stop(); SetEffectsAudio->SetSound(nullptr); DesiredMusic=nullptr; MusicPositions.Reset();
    ReleaseFrontEnd(); bInitialized=false;
    Super::EndPlay(Reason);
}
