#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MovieManager.h"
#include "MovieResultsLibrary.generated.h"

/** Pure evaluation of the retained movie. Never drives playback or awards gameplay points. */
UCLASS()
class MYPROJECT_API UMovieResultsLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Movie|Results") static int32 GetStarsForScore(double UnroundedScore);
    UFUNCTION(BlueprintPure, Category="Movie|Results")
    static FMovieResults EvaluateMovie(const TArray<FMoviePerformance>& Performances, const TArray<FMovieRoleDefinition>& Roles,
        const FMovieScoreSummary& Scores, const FMovieResultsSettings& Settings);
};
