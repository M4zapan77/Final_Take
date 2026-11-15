#include "MovieManager.h"
#include "StorySessionComponent.h"
#include "MovieDirectorComponent.h"
#include "StudioFlowManager.h"
#include "StudioAudioSubsystem.h"

#include "ActingComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "MoviePresentationWidget.h"
#include "ObjectiveHUDWidget.h"
#include "ReplayActor.h"
#include "Camera/CameraActor.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"

bool AMovieManager::IsResultsOpen() const
{
    return bSessionActive && State==EMovieState::Finished && PresentationPhase==EMoviePresentationPhase::Results
        && MovieResults.bAvailable && !bStartingPremiere && !bClearingSession;
}

bool AMovieManager::ReplayMovie()
{
    if (!IsResultsOpen() || Performances.Num()!=3) { return false; }
    const int32 PreviousId=PresentationId;
    BeginPremiere();
    return bSessionActive && State==EMovieState::MoviePremiere && PresentationId!=PreviousId;
}

bool AMovieManager::RecordNewMovie() { return IsResultsOpen() && RestartMovie(); }

bool AMovieManager::CanReturnToMainMenu() const
{
    if (const auto* Flow=AStudioFlowManager::Find(this); Flow && Flow->Movie==this) { return true; }
    const FString Package=MainMenuMap.ToSoftObjectPath().GetLongPackageName();
    return !Package.IsEmpty() && FPackageName::IsValidLongPackageName(Package) && FPackageName::DoesPackageExist(Package);
}

bool AMovieManager::ReturnToMainMenu()
{
    if (!IsResultsOpen() || !CanReturnToMainMenu()) { return false; }
    if (auto* Flow=AStudioFlowManager::Find(this); Flow && Flow->Movie==this) { return Flow->ReturnToMainMenu(); }
    const TSoftObjectPtr<UWorld> Menu=MainMenuMap;
    StopMovie();
    UGameplayStatics::OpenLevelBySoftObjectPtr(this,Menu);
    return true;
}

void AMovieManager::SetPresentationPhase(EMoviePresentationPhase NewPhase)
{
    if (PresentationPhase==NewPhase) { return; }
    const EMoviePresentationPhase Previous=PresentationPhase; PresentationPhase=NewPhase;
    OnPresentationPhaseChanged.Broadcast(Previous,NewPhase);
}

void AMovieManager::HideGameplayHUD()
{
    if (ActiveObjectiveHUD) { ActiveObjectiveHUD->RemoveFromParent(); }
    if (AHUD* HUD=Controller.IsValid()?Controller->GetHUD():nullptr)
    {
        if (!bGameplayHUDHidden) { OriginalHUD=HUD; bOriginalHUDVisible=HUD->bShowHUD; }
        HUD->bShowHUD=false; bGameplayHUDHidden=true;
    }
}

void AMovieManager::CloseMoviePresentation()
{
    if (ActiveMoviePresentation) { ActiveMoviePresentation->SetIsEnabled(false); ActiveMoviePresentation->RemoveFromParent(); }
    ActiveMoviePresentation=nullptr;
    if (bPresentationInputHeld && Controller.IsValid())
    {
        if (Controller->GetLocalPlayer()) { Controller->SetInputMode(FInputModeGameOnly()); }
        Controller->bShowMouseCursor=bOriginalCursorVisible;
        Controller->bEnableClickEvents=bOriginalClickEvents;
        Controller->bEnableMouseOverEvents=bOriginalMouseOverEvents;
    }
    bPresentationInputHeld=false;
}

bool AMovieManager::EnsureMoviePresentation()
{
    APlayerController* Player=Controller.Get();
    if (!bShowMoviePresentationUI || !Player || !Player->GetLocalPlayer() || !GEngine || !GEngine->GameViewport) { return true; }
    const uint32 Generation=SessionGeneration;
    const int32 Id=PresentationId;
    if (!ActiveMoviePresentation)
    {
        UClass* WidgetClass=MoviePresentationWidgetClass?MoviePresentationWidgetClass.Get():UMoviePresentationWidget::StaticClass();
        auto* Created=CreateWidget<UMoviePresentationWidget>(Player,WidgetClass);
        if (!Created) { return false; }
        if (Generation!=SessionGeneration || Id!=PresentationId || !bSessionActive) { return true; }
        ActiveMoviePresentation=Created;
        Created->BindMovie(this,Id);
        if (Generation!=SessionGeneration || Id!=PresentationId || !bSessionActive || ActiveMoviePresentation!=Created) { return true; }
        Created->AddToViewport(100);
    }
    return true;
}

void AMovieManager::StopPresentationAudio()
{
    for (UAudioComponent* Audio : {PresentationMusic.Get(),PresentationSting.Get(),PresentationClick.Get()})
    { if (Audio) { Audio->Stop(); Audio->SetSound(nullptr); } }
}

void AMovieManager::PlayPresentationAudio(bool bResults)
{
    StopPresentationAudio();
    USoundBase* Music=bResults?SessionPremiereSettings.ResultsMusic:SessionPremiereSettings.PremiereMusic;
    USoundBase* Sting=bResults?SessionPremiereSettings.ResultsSting:SessionPremiereSettings.PremiereSting;
    UpdatePresentationAudioMix();
    auto* Flow=AStudioFlowManager::Find(this);
    if(Flow && Flow->Movie==this && Flow->UsesUnifiedAudio()) { Flow->PlayPresentationMusic(Music,SessionPremiereSettings.MusicVolume); }
    else if (Music)
    { PresentationMusic->SetSound(Music); PresentationMusic->FadeIn(.35f,1.f); }
    if (Sting) { PresentationSting->SetSound(Sting); PresentationSting->Play(); }
}

void AMovieManager::UpdatePresentationAudioMix()
{
    PresentationMusic->SetVolumeMultiplier(SessionPremiereSettings.MusicVolume*UStudioAudioSubsystem::MusicMultiplier(this));
    PresentationSting->SetVolumeMultiplier(SessionPremiereSettings.EffectsVolume*UStudioAudioSubsystem::EffectsMultiplier(this));
    PresentationClick->SetVolumeMultiplier(SessionPremiereSettings.EffectsVolume*UStudioAudioSubsystem::UIMultiplier(this));
}

void AMovieManager::PlayPresentationClick()
{
    if (SessionPremiereSettings.ButtonSound)
    {
        PresentationClick->SetSound(SessionPremiereSettings.ButtonSound);
        UpdatePresentationAudioMix(); PresentationClick->Play();
    }
}

void AMovieManager::BeginPremiere()
{
    if (!bSessionActive || bStartingPremiere || Performances.Num()!=3 || !MovieResults.bAvailable) { return; }
    TGuardValue<bool> Starting(bStartingPremiere,true);
    const uint32 Generation=SessionGeneration;
    CloseMoviePresentation(); StopPresentationAudio();
    PresentationId=PresentationId==MAX_int32?1:PresentationId+1;
    const int32 Id=PresentationId;
    const auto StillCurrent=[&]() { return Generation==SessionGeneration && Id==PresentationId && bSessionActive; };
    LiveActingComponent->StopAllActions();
    if (!StillCurrent()) { return; }
    LiveActingComponent->SetLiveActionsEnabled(false);
    if (!StillCurrent()) { return; }
    SceneClock->StopClock();
    if(IsChronologicalMovie()) { StorySession->PreparePremiere(); } else { ResetWorldProps(); }
    SetPerformerFrozen(true); SetLookLocked(true); HideGameplayHUD();
    if(!IsChronologicalMovie()) { LivePerformer->SetActorTransform(OriginalPawnTransform,false,nullptr,ETeleportType::TeleportPhysics); }
    LivePerformer->SetActorHiddenInGame(true); LivePerformer->SetActorEnableCollision(false);
    const bool bPrepared=PrepareReplays(false);
    if (!StillCurrent()) { return; }
    if (!bPrepared || !PrepareCamera()) { FailMovie(NSLOCTEXT("FinalTakeUI","MovieManagerPresentation.84c59920d3","Could not prepare the movie premiere.")); return; }
    PremiereDuration=0.0;
    for (const auto& Performance : Performances) { PremiereDuration=FMath::Max(PremiereDuration,Performance.Recording.GetDurationSeconds()); }
    if(IsChronologicalMovie() && MovieDirector){MovieDirector->Begin(this,SessionCamera);}
    Controller->SetViewTargetWithBlend(GetPremiereCamera(),0.0f);
    Controller->bShowMouseCursor=false; Controller->bEnableClickEvents=false; Controller->bEnableMouseOverEvents=false;
    if (Controller->GetLocalPlayer()) { Controller->SetInputMode(FInputModeGameOnly()); }
    bPresentationInputHeld=true;
    PhaseStartWorldTime=GetWorld()->GetTimeSeconds();
    ChangeState(EMovieState::MoviePremiere);
    if (!StillCurrent() || State!=EMovieState::MoviePremiere) { return; }
    SetPresentationPhase(EMoviePresentationPhase::TitleCard);
    if (!StillCurrent() || State!=EMovieState::MoviePremiere) { return; }
    if (!EnsureMoviePresentation()) { FailMovie(NSLOCTEXT("FinalTakeUI","MovieManagerPresentation.3876dd6ded","Could not create movie presentation UI.")); return; }
    if (!StillCurrent()) { return; }
    PlayPresentationAudio(false);
    OnPremiereTitleStarted.Broadcast();
    if (!StillCurrent()) { return; }
    if (SessionPremiereSettings.GetTitleDuration()<=0.0) { StartPremierePlayback(); }
}

void AMovieManager::StartPremierePlayback()
{
    if (!bSessionActive || State!=EMovieState::MoviePremiere || PresentationPhase!=EMoviePresentationPhase::TitleCard) { return; }
    const uint32 Generation=SessionGeneration; const int32 Id=PresentationId;
    SetPresentationPhase(EMoviePresentationPhase::Playback);
    if (Generation!=SessionGeneration || Id!=PresentationId || !bSessionActive) { return; }
    PhaseStartWorldTime=GetWorld()->GetTimeSeconds();
    SceneClock->StartClock(0.0);
    const bool bPrepared=PrepareReplays(true);
    if(IsChronologicalMovie()){StorySession->BeginScenePlayback();}
    if (Generation!=SessionGeneration || Id!=PresentationId || !bSessionActive) { return; }
    if (!bPrepared) { FailMovie(NSLOCTEXT("FinalTakeUI","MovieManagerPresentation.05f2fd043c","Could not start synchronized movie playback.")); return; }
    if (ActiveMoviePresentation) { ActiveMoviePresentation->RefreshPresentation(); }
    if (Generation!=SessionGeneration || Id!=PresentationId || !bSessionActive) { return; }
    OnPremiereStarted.Broadcast();
}

void AMovieManager::FinishPremiere()
{
    if (!bSessionActive || State!=EMovieState::MoviePremiere || PresentationPhase!=EMoviePresentationPhase::Playback) { return; }
    const uint32 Generation=SessionGeneration; const int32 Id=PresentationId;
    SceneClock->StartClock(PremiereDuration); SceneClock->PauseClock();
    if(IsChronologicalMovie()){StorySession->UpdateScenePlayback();if(MovieDirector){MovieDirector->Update(0);}}
    PhaseStartWorldTime=GetWorld()->GetTimeSeconds();
    SetPresentationPhase(EMoviePresentationPhase::Results);
    if (Generation!=SessionGeneration || Id!=PresentationId || !bSessionActive) { return; }
    ChangeState(EMovieState::Finished);
    if (Generation!=SessionGeneration || Id!=PresentationId || !IsResultsOpen()) { return; }
    if (!EnsureMoviePresentation()) { FailMovie(NSLOCTEXT("FinalTakeUI","MovieManagerPresentation.a96e1f1f0b","Could not create results UI.")); return; }
    if (Generation!=SessionGeneration || Id!=PresentationId || !IsResultsOpen()) { return; }
    PlayPresentationAudio(true);
    if (ActiveMoviePresentation)
    {
        ActiveMoviePresentation->RefreshPresentation();
        if (Generation!=SessionGeneration || Id!=PresentationId || !IsResultsOpen()) { return; }
        FInputModeUIOnly Input; Input.SetWidgetToFocus(ActiveMoviePresentation->TakeWidget()); Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Controller->SetInputMode(Input); Controller->bShowMouseCursor=true; Controller->bEnableClickEvents=true; Controller->bEnableMouseOverEvents=true;
        bPresentationInputHeld=true; ActiveMoviePresentation->FocusResults();
    }
    const FMovieResults StableResults=MovieResults;
    OnResultsReady.Broadcast(StableResults);
    if (Generation==SessionGeneration && Id==PresentationId && IsResultsOpen()) { OnMovieFinished.Broadcast(); }
}

ACameraActor* AMovieManager::GetPremiereCamera() const
{return MovieDirector && MovieDirector->GetCamera()?MovieDirector->GetCamera():SessionCamera.Get();}

FMoviePresentationView AMovieManager::GetPresentationView() const
{
    FMoviePresentationView View; View.Phase=PresentationPhase; View.PresentationId=PresentationId; View.Results=MovieResults;
    View.bLetterbox=SessionPremiereSettings.bLetterbox; View.LetterboxHeightFraction=SessionPremiereSettings.LetterboxHeightFraction;
    const double Elapsed=GetWorld()?FMath::Max(0.0,GetWorld()->GetTimeSeconds()-PhaseStartWorldTime):0.0;
    if (PresentationPhase==EMoviePresentationPhase::TitleCard)
    {
        const double FadeIn=SessionPremiereSettings.TitleFadeInSeconds;
        const double FadeOut=SessionPremiereSettings.TitleFadeOutSeconds;
        const double OutStart=FadeIn+SessionPremiereSettings.TitleHoldSeconds;
        View.TitleBackgroundOpacity=FadeOut>0?static_cast<float>(FMath::Clamp(1.0-(Elapsed-OutStart)/FadeOut,0.0,1.0)):(Elapsed<OutStart?1.f:0.f);
        View.TitleTextOpacity=View.TitleBackgroundOpacity*(FadeIn>0?static_cast<float>(FMath::Clamp(Elapsed/FadeIn,0.0,1.0)):1.f);
    }
    if (PresentationPhase==EMoviePresentationPhase::Results && SessionPremiereSettings.ResultsFadeInSeconds>0)
    { View.ResultsOpacity=static_cast<float>(FMath::Clamp(Elapsed/SessionPremiereSettings.ResultsFadeInSeconds,0.0,1.0)); }
    View.bCanReplay=IsResultsOpen(); View.bCanRecordNew=View.bCanReplay; View.bCanReturnToMenu=View.bCanReplay && CanReturnToMainMenu();
    return View;
}
