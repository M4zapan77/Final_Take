#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StudioFlowTypes.h"
#include "MovieManager.h"
#include "StudioFlowManager.generated.h"

class APlayerController;
class APawn;
class AHUD;
class UPawnMovementComponent;
class UAudioComponent;
class UActingComponent;
class UStudioFlowWidget;
class UStartupVideoComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStudioScreenChanged, EStudioScreen, Previous, EStudioScreen, Current);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStudioDirectorCue, EDirectorCue, Cue, FText, Text);

/** Front end and presentation only. MovieManager continues to own every take, clock and recording. */
UCLASS(Blueprintable)
class MYPROJECT_API AStudioFlowManager : public AActor
{
    GENERATED_BODY()
public:
    AStudioFlowManager();
    UFUNCTION(BlueprintCallable, Category="Studio") bool InitializeFlow(AMovieManager* Manager, APlayerController* Player);
    UFUNCTION(BlueprintCallable, Category="Studio") bool PlayMovie();
    UFUNCTION(BlueprintCallable, Category="Studio") bool BeginIntroducedMovie();
    UFUNCTION(BlueprintCallable, Category="Studio") bool ShowHowToPlay();
    UFUNCTION(BlueprintCallable, Category="Studio") bool ShowCredits();
    UFUNCTION(BlueprintCallable, Category="Studio") bool ShowAudioSettings();
    UFUNCTION(BlueprintCallable, Category="Studio") bool BackToMenu();
    UFUNCTION(BlueprintCallable, Category="Studio") bool ReturnToMainMenu();
    UFUNCTION(BlueprintCallable, Category="Studio") bool QuitGame();
    UFUNCTION(BlueprintCallable, Category="Studio|Audio") void SetAudioLevels(FStudioAudioLevels Levels);
    UFUNCTION(BlueprintCallable, Category="Studio|Audio") void PlayInterfaceSound() { PlayClick(); }
    UFUNCTION(BlueprintPure, Category="Studio") FStudioFlowView GetView() const;
    UFUNCTION(BlueprintPure, Category="Studio") bool IsFrontEndOpen() const;
    static AStudioFlowManager* Find(const UObject* Context);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Studio") FStudioFlowSettings Settings;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Studio") TSubclassOf<UStudioFlowWidget> WidgetClass;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Studio|Videos") TObjectPtr<UStartupVideoComponent> StartupVideos;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Studio|Videos") bool bStartupPresentation = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Studio") TObjectPtr<AMovieManager> Movie;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Studio") TObjectPtr<UStudioFlowWidget> ActiveWidget;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Studio") EStudioScreen Screen = EStudioScreen::MainMenu;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Studio") int32 Revision = 0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Studio") int32 AnnouncementSerial = 0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Studio") EDirectorCue LastAnnouncement = EDirectorCue::ChooseCostume;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Studio") FText FlowError;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Studio|Audio") TObjectPtr<UAudioComponent> BackgroundMusic;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Studio|Audio") TObjectPtr<UAudioComponent> DirectorVoice;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Studio|Audio") TObjectPtr<UAudioComponent> ButtonAudio;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Studio|Audio") TObjectPtr<UAudioComponent> AmbientAudio;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Studio|Audio") TObjectPtr<UAudioComponent> SetEffectsAudio;
    bool UsesUnifiedAudio() const;
    void PlayPresentationMusic(USoundBase* Sound,float Volume);
    UPROPERTY(BlueprintAssignable, Category="Studio") FStudioScreenChanged OnScreenChanged;
    UPROPERTY(BlueprintAssignable, Category="Studio") FStudioDirectorCue OnDirectorAnnouncement;

protected:
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    bool EnsureWidget();
    void SetScreen(EStudioScreen NewScreen);
    void HoldFrontEnd();
    void ReleaseFrontEnd();
    void SetMusic(USoundBase* Sound);
    void UpdateMusic(float DeltaSeconds);
    void SetAmbience(USoundBase* Sound);
    void StopDirector();
    void Announce(EDirectorCue Cue);
    void PlayClick();
    bool ShowMenuPage(EStudioScreen NewScreen);
    UFUNCTION() void HandleMovieState(EMovieState Previous, EMovieState Current);
    UFUNCTION() void HandlePreparation(int32 TakeNumber, FName RoleId, double Duration);
    UFUNCTION() void HandleRecording(int32 TakeNumber, FName RoleId, double Duration);
    UFUNCTION() void HandleTakeRecorded(const FMoviePerformance& Performance);
    UFUNCTION() void HandleMovieError(FText Error);
    UFUNCTION() void HandleHubPrepared();
    UFUNCTION() void HandleHubInteractive();
    UFUNCTION() void HandleGameplayBlack();

    UPROPERTY(Transient) TWeakObjectPtr<APlayerController> Controller;
    UPROPERTY(Transient) TWeakObjectPtr<APawn> MenuPawn;
    UPROPERTY(Transient) TWeakObjectPtr<AActor> SavedView;
    UPROPERTY(Transient) TWeakObjectPtr<AHUD> SavedHUD;
    UPROPERTY(Transient) TWeakObjectPtr<UPawnMovementComponent> SavedMovement;
    UPROPERTY(Transient) TWeakObjectPtr<UActingComponent> SavedActing;
    FRotator SavedRotation = FRotator::ZeroRotator;
    FText DirectorLine;
    double PageStarted = 0;
    double CueStarted = -1;
    double CueDuration = 0;
    float CueVolume = 1;
    uint8 SavedMovementMode = 0;
    uint8 SavedCustomMode = 0;
    bool bInitialized = false;
    bool bNavigating = false;
    bool bMenuHeld = false;
    bool bSavedCursor = false;
    bool bSavedClick = false;
    bool bSavedMouseOver = false;
    bool bSavedHidden = false;
    bool bSavedCollision = true;
    bool bSavedMovementActive = false;
    bool bSavedHUDVisible = true;
    bool bSavedActions = true;
    bool bTransitionCueSent = false;
    UPROPERTY(Transient) TObjectPtr<USoundBase> DesiredMusic;
    UPROPERTY(Transient) TMap<USoundBase*,float> MusicPositions;
    double MusicStartedAt=0;
    float MusicStartPosition=0;
    float MusicEnvelope=0;
    float MusicStateVolume=.72f;
};
