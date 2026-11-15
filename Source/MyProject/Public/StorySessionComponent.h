#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActingRecording.h"
#include "StoryWorldTypes.h"
#include "StoryScenarioData.h"
#include "ObjectiveTypes.h"
#include "StorySessionComponent.generated.h"

class AMovieManager;
class APawn;
class AInteractableProp;
class ACharacter;
class UStaticMesh;
class UMaterialInterface;
class UActingComponent;
class UGrabbablePropComponent;
class UBoxComponent;
class ACameraActor;

USTRUCT(BlueprintType)
struct MYPROJECT_API FStoryPropTrack
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="Story") FName ObjectId;
    UPROPERTY(BlueprintReadOnly,Category="Story") TArray<FActingTransformSample> Samples;
    double PendingHoldTime=0;
    void Observe(double Time,const FTransform& Transform,bool bBoundary=false);
    bool Evaluate(double Time,FTransform& Out) const;
};

/** One local recording on a continuous scene timeline; appearance belongs to the cast. */
USTRUCT(BlueprintType)
struct MYPROJECT_API FStorySegment
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Story") FName ActorId;
    UPROPERTY(BlueprintReadOnly, Category="Story") int32 TurnNumber=0;
    UPROPERTY(BlueprintReadOnly, Category="Story") double StartSeconds=0;
    UPROPERTY(BlueprintReadOnly, Category="Story") double EndSeconds=0;
    UPROPERTY(BlueprintReadOnly, Category="Story") FTransform InitialTransform=FTransform::Identity;
    UPROPERTY(BlueprintReadOnly, Category="Story") FTransform FinalTransform=FTransform::Identity;
    UPROPERTY(BlueprintReadOnly, Category="Story") FActingRecording Recording;
    UPROPERTY(BlueprintReadOnly, Category="Story") FMovieObjectiveDefinition Objective;
    UPROPERTY(BlueprintReadOnly, Category="Story") FTakeObjectiveResult ObjectiveResult;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FStoryCastMember
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Story") FName ActorId;
    UPROPERTY(BlueprintReadOnly, Category="Story") TObjectPtr<APawn> Pawn;
    UPROPERTY(BlueprintReadOnly, Category="Story") FTransform InitialTransform=FTransform::Identity;
    UPROPERTY(BlueprintReadOnly, Category="Story") FRecordedAppearance Appearance;
    UPROPERTY(BlueprintReadOnly, Category="Story") FActingPlaybackSettings ActingSettings;
    UPROPERTY(BlueprintReadOnly, Category="Story") bool bCustomized=false;
    FRotator LastViewRotation=FRotator(-20,0,0);
    bool bHasView=false;
};

/** MovieManager's chronological mode. It never owns another clock or a parallel turn manager. */
UCLASS(ClassGroup=(Acting), meta=(BlueprintSpawnableComponent))
class MYPROJECT_API UStorySessionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStorySessionComponent();
    UFUNCTION(BlueprintPure, Category="Story") bool IsStoryActive() const { return bActive; }
    UFUNCTION(BlueprintPure, Category="Story") int32 GetRoundNumber() const;
    UFUNCTION(BlueprintPure, Category="Story") int32 GetTurnNumber() const;
    UFUNCTION(BlueprintPure, Category="Story") int32 GetRoleIndex() const;
    UFUNCTION(BlueprintPure, Category="Story") double GetTimelineDuration() const { return TimelineDuration; }
    UFUNCTION(BlueprintPure, Category="Story") double GetTurnElapsed() const;
    UFUNCTION(BlueprintPure, Category="Story") APawn* FindActor(FName ActorId) const;
    UFUNCTION(BlueprintPure,Category="Story") static FText RoleName(int32 RoleIndex);
    UFUNCTION(BlueprintPure,Category="Story") static FText RoleBrief(int32 RoleIndex);
    UFUNCTION(BlueprintCallable, Category="Story") bool CutTurn();
    UPROPERTY(BlueprintReadOnly, Transient, Category="Story") TArray<FStoryCastMember> Cast;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Story") TArray<FStorySegment> Segments;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Story") FStoryWorldState SceneState;
    UPROPERTY(BlueprintReadOnly,Transient,Category="Story|Scenario") FStoryScenarioDefinition SessionScenario;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Story") FStoryWorldState InitialSceneState;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Story") FStoryWorldState FinalSceneState;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Story") TArray<FStorySceneEvent> SceneEvents;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Story") TArray<FStoryPropTrack> PropTracks;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Story") EStoryOutcome Outcome=EStoryOutcome::Unresolved;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Story|Interactions",meta=(ClampMin="50")) double InteractionDistance=360;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Story|Interactions",meta=(ClampMin="1")) double GuardDistractionSeconds=12;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Story|Interactions",meta=(ClampMin="0.1",ClampMax="2")) double DetentionSeconds=.8;
    UFUNCTION(BlueprintPure, Category="Story") bool CanAct() const;
    UFUNCTION(BlueprintCallable, Category="Story") bool Interact();
    UFUNCTION(BlueprintCallable, Category="Story") bool InteractWith(FName TargetId);
    UFUNCTION(BlueprintCallable, Category="Story") bool DropHeldObject();
    UFUNCTION(BlueprintCallable, Category="Story") bool ThrowHeldObject();
    UFUNCTION(BlueprintPure,Category="Story") FName GetHeldObject(FName ActorId,bool bLeftHand=false) const;
    UFUNCTION(BlueprintPure, Category="Story") FText GetInteractionPrompt() const;
    UFUNCTION(BlueprintPure, Category="Story") FText GetPrimaryObjectiveText() const;
    UFUNCTION(BlueprintPure, Category="Story") FText GetLatestConsequence() const;
    UFUNCTION(BlueprintPure, Category="Story") FStoryScoreBreakdown GetScoreBreakdown() const;
    UFUNCTION(BlueprintPure, Category="Story") FVector GetInteractionLocation(FName TargetId) const;
    void BeginScenePlayback();
    void UpdateScenePlayback();
    void CalculateStoryScore();

    bool BeginSession();
    void UpdateStory(float DeltaSeconds);
    void BeginRecording();
    void CaptureFinishedSegment(const FActingRecording& Recording);
    void PreparePremiere();
    void ResetSession(bool bRestorePlayer);
    static bool AssembleRoleRecording(FName ActorId, const FStoryCastMember& Member,
        const TArray<FStorySegment>& SourceSegments, double Duration, FActingRecording& Out);

private:
    bool IsCurrent() const;
    bool ConfigureActor(APawn* Pawn);
    bool SwitchActor(int32 Index);
    void BeginTurn();
    void FinishTurn();
    void FreezeCast(bool bFreezeActive);
    bool AssembleMovie();
    bool InitializeScene();
    void ClearScene();
    void PreviewContextObjective(bool bBindCreative=true);
    void EvaluateContextObjective();
    void UpdateScene();
    FName FindInteractionTarget() const;
    bool IsReachable(FVector Location,AActor* Target=nullptr) const;
    bool SetMoneyOwner(FName Owner,EStoryEventType Reason,FName PreviousOwner=NAME_None);
    void ApplyMoneyPresentation(FName Owner,const FTransform& Transform,bool bPlayback);
    void SetNPCMood(FName Id,EStoryNPCMood Mood,double Duration,FName Reaction=NAME_None);
    void ReactActor(FName TargetId,FName Gesture);
    void AddSceneEvent(EStoryEventType Type,FName Target,FName Object=NAME_None,FName Payload=NAME_None,const FTransform& Transform=FTransform::Identity,double Duration=0);
    void HandleCreativeAction(const FActingEvent& Event,bool bFromReplay);
    void ApplyPlaybackEvent(const FStorySceneEvent& Event);
    void AppendSceneActingEvents(FName ActorId,FActingRecording& Recording) const;
    void PlaySceneEventAudio(const FStorySceneEvent& Event,bool bPlayback);
    bool InitializePhysicalProps();
    void ClearPhysicalProps();
    UGrabbablePropComponent* RegisterPhysicalProp(AActor* Actor,FName ObjectId,FName CosmeticId=NAME_None);
    void InitializeOutfitProps();
    void UpdatePhysicalProps();
    void SetPhysicsPaused(bool bPaused);
    void SetDetentionInput(bool bHeld);
    bool GrabObject(FName ObjectId);
    bool ReleaseObject(FName ObjectId,bool bThrow);
    void ObserveProp(FName ObjectId,bool bBoundary=false);
    void HandlePropImpact(UGrabbablePropComponent* Prop,AActor* Target,float Speed,const FVector& Location);
    void UpdatePropPlayback();
    void ApplyPhysicalEvent(const FStorySceneEvent& Event);
    void UpdateHandSuppression(bool bPlayback);
    UPROPERTY(Transient) TMap<FName,TObjectPtr<UGrabbablePropComponent>> PhysicalProps;
    UPROPERTY(Transient) TArray<TObjectPtr<AActor>> CreatedPropActors;
    UPROPERTY(Transient) TArray<TObjectPtr<UBoxComponent>> FurnitureCollision;
    TMap<FName,FName> PlaybackOwners;
    TMap<FName,bool> PlaybackLeftHands;
    TMap<FName,double> LastImpactTimes;
    double NextPropSampleTime=0;
    struct FQueuedImpact{FName ObjectId;TWeakObjectPtr<AActor> Target;float Speed;FVector Location;};
    TArray<FQueuedImpact> QueuedImpacts;
    FVector GroundLocation(FVector Location,double Height=96) const;
    UPROPERTY(Transient) TObjectPtr<AInteractableProp> MoneyProp;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> OriginalMoneyMesh;
    UPROPERTY(Transient) FCosmeticDefinition MoneyVisual;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInterface>> OriginalMoneyMaterials;
    UPROPERTY(Transient) TArray<TObjectPtr<ACharacter>> NPCActors;
    UPROPERTY(Transient) FMovieObjectiveDefinition CurrentObjective;
    EStoryObjectiveKind ObjectiveKind=EStoryObjectiveKind::GetMoney;
    FTransform OriginalMoneyTransform=FTransform::Identity;
    FVector SceneAnchor=FVector::ZeroVector;
    FVector EscapeLocation=FVector::ZeroVector;
    FVector SecureLocation=FVector::ZeroVector;
    FDelegateHandle CreativeDelegate;
    TWeakObjectPtr<UActingComponent> CreativeSource;
    TSet<FName> InteractionPairs;
    TSet<FName> CreativeIds;
    TSet<FName> NPCInteractionIds;
    TSet<FName> ThrownObjects;
    int32 SceneEventCursor=0;
    bool bScenePlayback=false;
    bool bSpawnedMoney=false;
    UPROPERTY(Transient) TObjectPtr<AMovieManager> Movie;
    UPROPERTY(Transient) TObjectPtr<APawn> OriginalPawn;
    uint32 Generation=0;
    double TurnStartSeconds=0;
    double TimelineDuration=0;
    bool bActive=false;
    bool bPendingEnd=false;
    bool bCutRequested=false;
    bool bOriginalAutoCamera=true;
    bool bOriginalActorTick=true;
    bool bOriginalPauseAnims=false;
    bool bOriginalCompact=false,bOriginalCaptureSnapshot=true;
    bool bDetentionInputHeld=false;
    UPROPERTY(Transient) TObjectPtr<ACameraActor> PossessionCamera;
    struct FOriginalMovementCamera
    {
        float Speed=0,Acceleration=0,Braking=0,RotationYaw=0,ArmLength=0,LagSpeed=0,LagDistance=0,ProbeSize=0;
        FVector SocketOffset=FVector::ZeroVector,TargetOffset=FVector::ZeroVector;
        bool bPhysicsInteraction=true,bCameraLag=false,bRotationLag=false,bCollision=true,bSubstep=false,bSaved=false,bArmSaved=false;
    } OriginalControls;
};
