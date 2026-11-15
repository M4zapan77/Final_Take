#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ActingRecording.h"
#include "ObjectiveTypes.h"
#include "MovieResultsTypes.h"
#include "InputCoreTypes.h"
#include "MovieManager.generated.h"

class ACameraActor;
class APawn;
class APlayerController;
class AReplayActor;
class UActingRecorderComponent;
class UActingComponent;
class UActingSceneClockSubsystem;
class UWardrobeComponent;
class UWardrobeWidget;
class UPointLightComponent;
class UPawnMovementComponent;
class UObjectiveManagerComponent;
class UObjectiveHUDWidget;
class UPropInteractionComponent;
class AInteractableProp;
class AHUD;
class UAudioComponent;
class UMoviePresentationWidget;
class UStudioCharacterProfile;
class UStudioAudioProfile;
class UStorySessionComponent;
class UMovieDirectorComponent;
class UStoryScenarioData;
struct FKey;

UENUM(BlueprintType)
enum class EMovieState : uint8
{
    Idle,
    Preparation,
    Recording,
    MoviePremiere,
    Finished,
    Error,
    TakeTransition
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FMovieRoleDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie")
    FName RoleId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie", meta = (MultiLine = true))
    FText ScriptInstruction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie")
    FCosmeticSelection DefaultOutfit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie")
    TArray<FMovieObjectiveDefinition> Objectives;

    /** Local to the movie origin: the player's initial pose, or the manager transform. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie")
    FTransform InitialTransform = FTransform::Identity;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FMoviePerformance
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Movie")
    FName RoleId;

    /** Resolved world-space starting pose for this role. */
    UPROPERTY(BlueprintReadOnly, Category = "Movie")
    FTransform InitialTransform = FTransform::Identity;

    UPROPERTY(BlueprintReadOnly, Category = "Movie")
    FActingRecording Recording;

    UPROPERTY(BlueprintReadOnly, Category="Movie")
    FTakeObjectiveResult ObjectiveResult;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMovieStateChanged, EMovieState, PreviousState, EMovieState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMovieTakeStarted, int32, TakeNumber, FName, RoleId, double, DurationSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMovieTakeRecorded, const FMoviePerformance&, Performance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMovieEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMovieError, FText, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMovieScoreCalculated, const FMovieScoreSummary&, Summary);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMoviePresentationChanged, EMoviePresentationPhase, PreviousPhase, EMoviePresentationPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMovieResultsReady, const FMovieResults&, Results);

/** Three roles performed by one live pawn, with earlier takes represented by collision-free replays. */
UCLASS(Blueprintable)
class MYPROJECT_API AMovieManager : public AActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Audio") TObjectPtr<UStudioAudioProfile> AudioProfile;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Character") TObjectPtr<UStudioCharacterProfile> CharacterProfile;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Wardrobe") bool bUseUnifiedCustomization=true;
    AMovieManager();

    /** Original simultaneous replay remains a selectable fallback during the redesign. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Story") bool bUseChronologicalGameplay=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Story", meta=(ClampMin="0")) double StoryWardrobeDurationSeconds=30;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Story", meta=(ClampMin="0.05",ClampMax="10")) double StoryTurnDurationSeconds=10;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Story") FKey EndTurnKey=EKeys::T;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Movie|Story") FKey DropObjectKey=EKeys::Q;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Movie|Story") FKey ThrowObjectKey=EKeys::LeftMouseButton;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movie|Story") TObjectPtr<UStorySessionComponent> StorySession;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Movie|Director") TObjectPtr<UMovieDirectorComponent> MovieDirector;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Movie|Scenarios") TArray<TObjectPtr<UStoryScenarioData>> ScenarioLibrary;
    /** None selects the first valid library entry, or the built-in verified bank scenario. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Movie|Scenarios") FName SelectedScenarioId;
    UFUNCTION(BlueprintPure, Category="Movie|Story") bool IsChronologicalMovie() const;
    UFUNCTION(BlueprintPure, Category="Movie|Story") int32 GetRoundNumber() const;
    UFUNCTION(BlueprintCallable, Category="Movie|Story") bool EndCurrentTurn();

    /** Acquires the player's current pawn. Returns false for invalid setup or an active session. */
    UFUNCTION(BlueprintCallable, Category = "Movie")
    bool StartMovie();

    /** Both Ready and timeout use this guarded transition. Stale widgets cannot confirm another take. */
    UFUNCTION(BlueprintCallable, Category = "Movie|Wardrobe")
    bool ReadyForTake(int32 ExpectedPreparationId);

    UFUNCTION(BlueprintCallable, Category = "Movie|Wardrobe")
    bool SelectWardrobeItem(int32 ExpectedPreparationId, ECosmeticCategory Category, FName CosmeticId);

    UFUNCTION(BlueprintCallable, Category = "Movie|Wardrobe")
    bool SetWardrobeBodyColor(int32 ExpectedPreparationId, bool bUseColor, FLinearColor Color);

    UFUNCTION(BlueprintPure, Category = "Movie|Wardrobe")
    bool IsWardrobeOpen() const;

    UFUNCTION(BlueprintPure, Category = "Movie|Wardrobe")
    FText GetCurrentRoleName() const;

    UFUNCTION(BlueprintPure, Category = "Movie|Wardrobe")
    FText GetCurrentScriptInstruction() const;

    UFUNCTION(BlueprintPure, Category = "Movie")
    int32 GetNumberOfTakes() const;

    UFUNCTION(BlueprintPure, Category="Movie|Objectives") int32 GetCurrentScore() const;
    UFUNCTION(BlueprintPure, Category="Movie|Objectives") int32 GetScoreForRole(FName RoleId) const;
    UFUNCTION(BlueprintPure, Category="Movie|Objectives") AReplayActor* FindReplayForRole(FName RoleId) const;
    AInteractableProp* FindPickupById(FName PickupId) const;
    bool IsMoviePlayback() const { return bSessionActive && PresentationPhase == EMoviePresentationPhase::Playback && Performances.Num() == 3; }

    /** Replays the retained film through its title card and fixed camera. Never clears or recalculates scores. */
    UFUNCTION(BlueprintCallable, Category="Movie|Results") bool ReplayMovie();
    UFUNCTION(BlueprintCallable, Category="Movie|Results") bool RecordNewMovie();
    UFUNCTION(BlueprintCallable, Category="Movie|Results") bool ReturnToMainMenu();
    UFUNCTION(BlueprintPure, Category="Movie|Results") bool CanReturnToMainMenu() const;
    UFUNCTION(BlueprintPure, Category="Movie|Results") bool IsResultsOpen() const;
    UFUNCTION(BlueprintPure, Category="Movie|Results") FMoviePresentationView GetPresentationView() const;
    void PlayPresentationClick();

    /** Cancels the current session, clears every recording, restores the player, and begins take 1. */
    UFUNCTION(BlueprintCallable, Category = "Movie")
    bool RestartMovie();

    /** Clears the session and returns the player/camera/input to their pre-movie state. */
    UFUNCTION(BlueprintCallable, Category = "Movie")
    void StopMovie();

    UFUNCTION(BlueprintPure, Category = "Movie")
    bool HasActiveSession() const { return bSessionActive; }

    UFUNCTION(BlueprintPure, Category = "Movie")
    int32 GetCurrentTakeNumber() const;

    UFUNCTION(BlueprintPure, Category = "Movie")
    FName GetCurrentRoleId() const;

    UFUNCTION(BlueprintPure, Category = "Movie")
    double GetPhaseDurationSeconds() const;

    UFUNCTION(BlueprintPure, Category = "Movie")
    double GetPhaseElapsedSeconds() const;

    UFUNCTION(BlueprintPure, Category = "Movie")
    double GetPhaseRemainingSeconds() const;

    UFUNCTION(BlueprintPure, Category = "Movie")
    double GetSceneTimeSeconds() const;

    UFUNCTION(BlueprintPure, Category = "Movie")
    ACameraActor* GetPremiereCamera() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Timing", meta = (ClampMin = "0.0", DisplayName = "Wardrobe Duration Seconds"))
    double PreparationDurationSeconds = 10.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Timing", meta = (ClampMin = "0.05"))
    double RecordingDurationSeconds = 30.0;

    /** Optional cards between takes. The studio front end enables these; standalone coordinators keep their old timing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Timing") bool bUseTakeTransitions = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Timing", meta=(ClampMin="0")) double BetweenActorsSeconds = 2.5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Timing", meta=(ClampMin="0")) double WrapSeconds = 2.25;

    /** Exactly three unique, non-empty identifiers. Snapshotted when a movie starts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Roles")
    TArray<FMovieRoleDefinition> Roles;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Objectives") bool bUseDefaultHeistObjectivesIfEmpty = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Objectives") bool bShowObjectiveHUD = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Objectives") TSubclassOf<UObjectiveHUDWidget> ObjectiveHUDClass;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movie|Objectives") TObjectPtr<UObjectiveManagerComponent> ObjectiveManager;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movie|Objectives") TObjectPtr<UObjectiveHUDWidget> ActiveObjectiveHUD;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movie|Objectives") TObjectPtr<UPropInteractionComponent> LivePropInteraction;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movie|Objectives") FMovieScoreSummary ScoreSummary;

    /** Stable item IDs and asset/settings definitions, copied into the live wardrobe at StartMovie. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Wardrobe")
    TArray<FCosmeticDefinition> CosmeticCatalog;

    /** Enable to configure a WardrobeComponent on the character instead of the manager's catalog. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Wardrobe")
    bool bUsePerformerCosmeticCatalog = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Wardrobe")
    TSubclassOf<UWardrobeWidget> WardrobeWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Wardrobe")
    bool bShowWardrobeUI = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Wardrobe")
    bool bUseWardrobePreviewCamera = true;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Movie|Wardrobe")
    TObjectPtr<ACameraActor> WardrobePreviewCamera;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Wardrobe")
    FVector WardrobeCameraOffset = FVector(450.0, 0.0, 30.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Wardrobe")
    FVector WardrobeCameraLookAtOffset = FVector(0.0, -90.0, 0.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Wardrobe", meta = (ClampMin = "10", ClampMax = "120"))
    float WardrobeCameraFieldOfView = 60.0f;

    /** Runtime fill for the preview only. Disabled before any performance starts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Wardrobe")
    bool bUseWardrobeFillLight = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Wardrobe", meta = (ClampMin = "0"))
    float WardrobeFillLightLumens = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Roles")
    bool bUsePlayerStartAsOrigin = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Playback")
    TSubclassOf<AReplayActor> ReplayActorClass;

    /** Optional level camera. It is used as-is and is never moved or destroyed by the manager. */
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Movie|Premiere")
    TObjectPtr<ACameraActor> PremiereCamera;

    /** Used for a runtime fixed camera when PremiereCamera is unset; local to the movie origin. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Premiere")
    FVector AutomaticCameraOffset = FVector(-900.0, -650.0, 500.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Premiere") FMoviePremiereSettings PremiereSettings;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Results") FMovieResultsSettings ResultsSettings;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Results") bool bShowMoviePresentationUI = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Results") TSubclassOf<UMoviePresentationWidget> MoviePresentationWidgetClass;
    /** Leave empty when no main menu exists. Include the assigned map when cooking your game. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Results") TSoftObjectPtr<UWorld> MainMenuMap;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movie|Results") FMovieResults MovieResults;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movie|Results") EMoviePresentationPhase PresentationPhase = EMoviePresentationPhase::None;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movie|Results") int32 PresentationId = 0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Movie|Results") TObjectPtr<UMoviePresentationWidget> ActiveMoviePresentation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Setup", meta = (ClampMin = "0"))
    int32 PlayerIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Setup")
    bool bAutoStart = false;

    /** Debug convenience; disable when the HUD/input action owns restart. No input assets are changed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Debug")
    bool bBindRestartKey = true;

    /** Optional demo bindings: 1-5 gestures, Z/X/C/V/B/N soundboard. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Debug")
    bool bBindActingKeys = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie|Debug") bool bBindInteractionKey = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movie|Debug")
    bool bShowDebugOverlay = true;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Movie|State")
    EMovieState State = EMovieState::Idle;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Movie|State")
    int32 CurrentTakeIndex = INDEX_NONE;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Movie|State")
    TArray<FMoviePerformance> Performances;

    /** Index matches Performances. Read-only; these actors belong to this manager. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Movie|State")
    TArray<TObjectPtr<AReplayActor>> ActiveReplays;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Movie|State")
    TObjectPtr<APawn> LivePerformer;

    /** Use this component for gesture/soundboard buttons and HUD delegates. */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Movie|State")
    TObjectPtr<UActingComponent> LiveActingComponent;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Movie|Wardrobe")
    TObjectPtr<UWardrobeComponent> LiveWardrobeComponent;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Movie|Wardrobe")
    TObjectPtr<UWardrobeWidget> ActiveWardrobeWidget;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Movie|Wardrobe")
    int32 PreparationId = 0;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Movie|State")
    FText LastError;

    UPROPERTY(BlueprintAssignable, Category = "Movie|Events")
    FMovieStateChanged OnStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Movie|Events")
    FMovieTakeStarted OnPreparationStarted;

    UPROPERTY(BlueprintAssignable, Category = "Movie|Events")
    FMovieTakeStarted OnRecordingStarted;

    UPROPERTY(BlueprintAssignable, Category = "Movie|Events")
    FMovieTakeRecorded OnTakeRecorded;

    UPROPERTY(BlueprintAssignable, Category = "Movie|Events")
    FMovieEvent OnPremiereStarted;

    UPROPERTY(BlueprintAssignable, Category = "Movie|Events")
    FMovieEvent OnMovieFinished;

    UPROPERTY(BlueprintAssignable, Category="Movie|Events") FMovieScoreCalculated OnMovieScoreCalculated;
    UPROPERTY(BlueprintAssignable, Category="Movie|Events") FMoviePresentationChanged OnPresentationPhaseChanged;
    UPROPERTY(BlueprintAssignable, Category="Movie|Events") FMovieEvent OnPremiereTitleStarted;
    UPROPERTY(BlueprintAssignable, Category="Movie|Events") FMovieResultsReady OnResultsReady;

    UPROPERTY(BlueprintAssignable, Category = "Movie|Events")
    FMovieEvent OnMovieReset;

    UPROPERTY(BlueprintAssignable, Category = "Movie|Events")
    FMovieEvent OnWardrobeClosed;

    UPROPERTY(BlueprintAssignable, Category = "Movie|Events")
    FMovieError OnMovieError;

protected:
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class UStorySessionComponent;
    void HandleEndTurnKey();
    void HandleDropKey();
    void HandleThrowKey();
    bool bEndTurnKeyBound=false;
    bool ValidateSetup(FText& OutError) const;
    void BeginPreparation();
    void BeginRecording();
    void BeginTakeTransition();
    void BeginPremiere();
    void StartPremierePlayback();
    void FinishPremiere();
    void SetPresentationPhase(EMoviePresentationPhase NewPhase);
    bool EnsureMoviePresentation();
    void CloseMoviePresentation();
    void HideGameplayHUD();
    void StopPresentationAudio();
    void PlayPresentationAudio(bool bResults);
    void UpdatePresentationAudioMix();
    bool PrepareReplays(bool bPlay);
    bool PrepareCamera();
    bool OpenWardrobePresentation();
    void CloseWardrobePresentation();
    void SetPerformerFrozen(bool bFrozen);
    void SetLookLocked(bool bLocked);
    void ChangeState(EMovieState NewState);
    void ClearSession(bool bRestorePlayer);
    void FailMovie(const FText& Error);
    void HandleRestartKey();
    void HandleActingKey(FKey Key);
    void HandleInteractKey();
    void ResetWorldProps();
    void CalculateMovieScore();
    void ShowObjectiveHUD();
    void UpdateDebugOverlay() const;

    UFUNCTION()
    void HandleRecordingFinished(const FActingRecording& Recording);

    UPROPERTY(Transient)
    TObjectPtr<UActingRecorderComponent> Recorder;

    UPROPERTY(Transient)
    TObjectPtr<UActingSceneClockSubsystem> SceneClock;

    UPROPERTY(Transient)
    TWeakObjectPtr<APlayerController> Controller;

    UPROPERTY(Transient)
    TWeakObjectPtr<AActor> OriginalViewTarget;

    UPROPERTY(Transient)
    TWeakObjectPtr<UPawnMovementComponent> Movement;

    UPROPERTY(Transient)
    TObjectPtr<ACameraActor> SessionCamera;

    UPROPERTY(Transient)
    TObjectPtr<ACameraActor> OwnedCamera;

    UPROPERTY(Transient)
    TObjectPtr<ACameraActor> OwnedWardrobeCamera;

    UPROPERTY(Transient)
    TObjectPtr<UPointLightComponent> WardrobeFillLight;

    UPROPERTY(Transient)
    TArray<FCosmeticDefinition> OriginalCosmeticCatalog;

    UPROPERTY(Transient)
    FRecordedAppearance OriginalAppearance;

    FCosmeticSelection OriginalDefaultOutfit;

    UPROPERTY(Transient)
    TArray<FMovieRoleDefinition> SessionRoles;

    UPROPERTY(Transient) FMoviePremiereSettings SessionPremiereSettings;
    UPROPERTY(Transient) FMovieResultsSettings SessionResultsSettings;
    UPROPERTY(VisibleAnywhere, Category="Movie|Audio") TObjectPtr<UAudioComponent> PresentationMusic;
    UPROPERTY(VisibleAnywhere, Category="Movie|Audio") TObjectPtr<UAudioComponent> PresentationSting;
    UPROPERTY(VisibleAnywhere, Category="Movie|Audio") TObjectPtr<UAudioComponent> PresentationClick;
    UPROPERTY(Transient) TWeakObjectPtr<AHUD> OriginalHUD;

    TArray<FTransform> InitialWorldTransforms;
    FTransform MovieOrigin = FTransform::Identity;
    FTransform OriginalPawnTransform = FTransform::Identity;
    FRotator OriginalControlRotation = FRotator::ZeroRotator;
    double SessionPreparationDuration = 0.0;
    double SessionRecordingDuration = 0.0;
    double SessionBetweenActors = 0.0;
    double SessionWrap = 0.0;
    double PremiereDuration = 0.0;
    double PhaseStartWorldTime = 0.0;
    uint32 SessionGeneration = 0;
    uint8 OriginalMovementMode = 0;
    uint8 OriginalCustomMovementMode = 0;
    bool bSessionActive = false;
    bool bClearingSession = false;
    bool bAutoStartAttempted = false;
    bool bRestartKeyBound = false;
    bool bActingKeysBound = false;
    bool bInteractionKeyBound = false;
    bool bOriginalLiveActionsEnabled = true;
    bool bOriginalRequireRecording = true;
    bool bPendingTakeFinished = false;
    bool bFrozen = false;
    bool bMoveLockHeld = false;
    bool bLookLockHeld = false;
    bool bOriginalMovementActive = false;
    bool bOriginalHidden = false;
    bool bOriginalCollision = true;
    bool bStartingTake = false;
    bool bWardrobeInputHeld = false;
    bool bOriginalCursorVisible = false;
    bool bOriginalClickEvents = false;
    bool bOriginalMouseOverEvents = false;
    bool bOriginalWardrobeEnabled = true;
    bool bOriginalHUDVisible = true;
    bool bGameplayHUDHidden = false;
    bool bPresentationInputHeld = false;
    bool bStartingPremiere = false;
};
