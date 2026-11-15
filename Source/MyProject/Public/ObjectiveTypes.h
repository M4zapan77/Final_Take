#pragma once

#include "CoreMinimal.h"
#include "ActingTypes.h"
#include "StoryWorldTypes.h"
#include "ObjectiveTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EMovieObjectiveType : uint8 { ReachZone, CollectProp, PerformGesture, PlaySound, ReachActor, ActionNearActor, UseProp, SceneAction };

USTRUCT(BlueprintType)
struct MYPROJECT_API FMovieObjectiveDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective") FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective") FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective", meta=(MultiLine=true)) FText Description;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective") EMovieObjectiveType Type = EMovieObjectiveType::ReachZone;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective") FName TargetTag;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective") TObjectPtr<AActor> TargetActor;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective") FName TargetRoleId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective") bool bAnyRecordedRole = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective") FName RequiredEventId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective") EActingEventType RequiredEventType = EActingEventType::Gesture;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective") FName RequiredPropId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective", meta=(ClampMin="0", ClampMax="100")) int32 Points = 25;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective") FName PrerequisiteId;
    /** Zero uses the zone's bounds, or the default proximity distance (200) for actor objectives. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective", meta=(ClampMin="0")) double MaxInteractionDistance = 0.0;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FMovieObjectiveStatus
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Objective") FMovieObjectiveDefinition Definition;
    UPROPERTY(BlueprintReadOnly, Category="Objective") bool bCompleted = false;
    UPROPERTY(BlueprintReadOnly, Category="Objective") int32 PointsAwarded = 0;
    UPROPERTY(BlueprintReadOnly, Category="Objective") double CompletedAtSeconds = 0.0;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FTakeObjectiveResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Score") int32 Score = 0;
    UPROPERTY(BlueprintReadOnly, Category="Score") TArray<FName> CompletedObjectiveIds;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FMovieRoleScore
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Score") FName RoleId;
    UPROPERTY(BlueprintReadOnly, Category="Score") int32 Score = 0;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FMovieScoreSummary
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Score") TArray<FMovieRoleScore> RoleScores;
    UPROPERTY(BlueprintReadOnly, Category="Score") double FinalMovieScore = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Score") bool bUseStoryScoring=false;
    UPROPERTY(BlueprintReadOnly, Category="Score") FStoryScoreBreakdown StoryScore;
};

namespace MovieObjectiveDefaults
{
    MYPROJECT_API TArray<FMovieObjectiveDefinition> BankHeist(int32 RoleIndex, FName RobberRole, FName PoliceRole);
}
