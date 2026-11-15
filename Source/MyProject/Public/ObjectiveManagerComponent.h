#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ObjectiveTypes.h"
#include "ObjectiveManagerComponent.generated.h"
class AMovieManager;
class APawn;
class UActingComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMovieObjectiveCompleted, const FMovieObjectiveStatus&, Objective);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMovieScoreChanged, int32, Score);

/** Only the current live performer can award points; replay actors are read-only spatial targets. */
UCLASS(ClassGroup=(Acting), BlueprintType, meta=(BlueprintSpawnableComponent))
class MYPROJECT_API UObjectiveManagerComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UObjectiveManagerComponent();
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Objectives") TArray<FMovieObjectiveStatus> Objectives;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Objectives") int32 CurrentScore = 0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Objectives") FName RoleId;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Objectives") FText ConfigurationIssue;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Objectives") FText LastCompletionFeedback;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Objectives") double LastCompletionWorldTime = -100.0;
    UPROPERTY(BlueprintAssignable, Category="Objectives") FMovieObjectiveCompleted OnObjectiveCompleted;
    UPROPERTY(BlueprintAssignable, Category="Objectives") FMovieScoreChanged OnScoreChanged;
    UFUNCTION(BlueprintPure, Category="Objectives") bool IsObjectiveCompleted(FName Id) const;
    UFUNCTION(BlueprintPure, Category="Objectives") bool IsPrerequisiteSatisfied(FName Id) const;
    UFUNCTION(BlueprintPure, Category="Objectives") bool IsScoringActive() const { return bActive; }
    UFUNCTION(BlueprintPure, Category="Objectives") FTakeObjectiveResult GetResult() const;
    UFUNCTION(BlueprintPure, Category="Objectives") static bool ValidateDefinitions(const TArray<FMovieObjectiveDefinition>& Definitions, FText& Error);

    bool PreviewRole(AMovieManager* Manager, FName InRoleId, const TArray<FMovieObjectiveDefinition>& Definitions);
    void BeginTake(APawn* Performer, UActingComponent* Acting);
    void EvaluateContinuous(double SceneTime, FVector LiveLocation);
    FTakeObjectiveResult FinishTake(double SceneTime, FVector FinalLocation);
    void ResetObjectives();
    bool CompleteSceneObjective(FName Id,double SceneTime);
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void HandleActingEvent(const FActingEvent& Event, bool bFromReplay);
    bool CanScore() const;
    bool InZone(const FMovieObjectiveDefinition& Definition, FVector Location) const;
    bool NearTarget(const FMovieObjectiveDefinition& Definition, FVector Location, double SceneTime) const;
    void CompleteObjective(FName Id, double SceneTime);
    void UnbindActing();
    UPROPERTY(Transient) TWeakObjectPtr<AMovieManager> Movie;
    UPROPERTY(Transient) TWeakObjectPtr<APawn> LivePerformer;
    UPROPERTY(Transient) TWeakObjectPtr<UActingComponent> LiveActing;
    FDelegateHandle ActingDelegate;
    uint32 ObjectiveGeneration = 0;
    bool bActive = false;
};
