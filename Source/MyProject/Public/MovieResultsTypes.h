#pragma once

#include "CoreMinimal.h"
#include "StoryWorldTypes.h"
#include "MovieResultsTypes.generated.h"

class USoundBase;
class UTexture2D;

UENUM(BlueprintType)
enum class EMoviePresentationPhase : uint8 { None, TitleCard, Playback, Results };

UENUM(BlueprintType)
enum class EMovieAwardType : uint8 { BestActor, DirectorsCut, DancingLegend, SoundChaos, CostumeDisaster };

USTRUCT(BlueprintType)
struct MYPROJECT_API FMovieRatingText
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rating", meta=(ClampMin="1", ClampMax="5")) int32 Stars = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rating") FText Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rating", meta=(MultiLine=true)) FText AudienceReview;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FMovieAwardStyle
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Award") EMovieAwardType Type = EMovieAwardType::BestActor;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Award") FText Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Award", meta=(MultiLine=true)) FText Description;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Award") TObjectPtr<UTexture2D> Icon;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FCostumeAwardRule
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Award") FName Id;
    /** All non-empty tags must occur in the recorded initial outfit. Empty rules never qualify. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Award") TArray<FName> RequiredTags;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Award") TArray<FName> ForbiddenTags;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FMovieResultsSettings
{
    GENERATED_BODY()
    FMovieResultsSettings();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie") FText MovieTitle;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie") FText Tagline;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie") TArray<FMovieRatingText> RatingTexts;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie") TArray<FMovieAwardStyle> AwardStyles;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movie") TArray<FCostumeAwardRule> CostumeDisasterRules;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FMoviePremiereSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition", meta=(ClampMin="0")) double TitleFadeInSeconds = .25;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition", meta=(ClampMin="0")) double TitleHoldSeconds = 2.0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition", meta=(ClampMin="0")) double TitleFadeOutSeconds = .4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition", meta=(ClampMin="0")) double ResultsFadeInSeconds = .25;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition") bool bLetterbox = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition", meta=(ClampMin="0", ClampMax="0.25")) float LetterboxHeightFraction = .08f;
    /** Optional background audio. Configure looping in the assigned SoundCue/Wave. Never changes recorded actor audio. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> PremiereMusic;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> PremiereSting;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> ResultsMusic;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> ResultsSting;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> ButtonSound;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0", ClampMax="2")) float MusicVolume = .5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0", ClampMax="2")) float EffectsVolume = 1.0f;

    double GetTitleDuration() const { return TitleFadeInSeconds + TitleHoldSeconds + TitleFadeOutSeconds; }
    bool IsValid() const;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FMovieRoleResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Results") FName RoleId;
    UPROPERTY(BlueprintReadOnly, Category="Results") FText RoleName;
    UPROPERTY(BlueprintReadOnly, Category="Results") int32 Score = 0;
    UPROPERTY(BlueprintReadOnly, Category="Results") int32 ObjectiveCount = 0;
    UPROPERTY(BlueprintReadOnly, Category="Results") TArray<FName> CompletedObjectiveIds;
    UPROPERTY(BlueprintReadOnly, Category="Results") TArray<FText> CompletedObjectiveNames;
    UPROPERTY(BlueprintReadOnly, Category="Results") int32 DanceEvents = 0;
    UPROPERTY(BlueprintReadOnly, Category="Results") TArray<FName> DistinctSoundIds;
    UPROPERTY(BlueprintReadOnly, Category="Results") TArray<FName> CostumeTags;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FMovieAward
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Results") EMovieAwardType Type = EMovieAwardType::BestActor;
    UPROPERTY(BlueprintReadOnly, Category="Results") FText Name;
    UPROPERTY(BlueprintReadOnly, Category="Results") FText Description;
    UPROPERTY(BlueprintReadOnly, Category="Results") TObjectPtr<UTexture2D> Icon;
    /** All tied roles are included; no winner is chosen by array order. */
    UPROPERTY(BlueprintReadOnly, Category="Results") TArray<FName> WinnerRoleIds;
    UPROPERTY(BlueprintReadOnly, Category="Results") int32 Statistic = 0;
    UPROPERTY(BlueprintReadOnly, Category="Results") bool bWholeMovie = false;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FMovieResults
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Results") bool bAvailable = false;
    UPROPERTY(BlueprintReadOnly, Category="Results") FText MovieTitle;
    UPROPERTY(BlueprintReadOnly, Category="Results") FText Tagline;
    /** Never rounded before applying the rating thresholds. */
    UPROPERTY(BlueprintReadOnly, Category="Results") double FinalScore = 0;
    UPROPERTY(BlueprintReadOnly, Category="Results") int32 Stars = 1;
    UPROPERTY(BlueprintReadOnly, Category="Results") FText RatingName;
    UPROPERTY(BlueprintReadOnly, Category="Results") FText AudienceReview;
      UPROPERTY(BlueprintReadOnly, Category="Results") FText StoryEnding;
      UPROPERTY(BlueprintReadOnly, Category="Results") FStoryScoreBreakdown StoryScore;
    UPROPERTY(BlueprintReadOnly, Category="Results") TArray<FMovieRoleResult> Roles;
    UPROPERTY(BlueprintReadOnly, Category="Results") TArray<FMovieAward> Awards;
    UPROPERTY(BlueprintReadOnly, Category="Results") bool bAllObjectivesCompleted = false;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FMoviePresentationView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Presentation") EMoviePresentationPhase Phase = EMoviePresentationPhase::None;
    UPROPERTY(BlueprintReadOnly, Category="Presentation") FMovieResults Results;
    UPROPERTY(BlueprintReadOnly, Category="Presentation") int32 PresentationId = 0;
    UPROPERTY(BlueprintReadOnly, Category="Presentation") float TitleTextOpacity = 1;
    UPROPERTY(BlueprintReadOnly, Category="Presentation") float TitleBackgroundOpacity = 1;
    UPROPERTY(BlueprintReadOnly, Category="Presentation") float ResultsOpacity = 1;
    UPROPERTY(BlueprintReadOnly, Category="Presentation") bool bLetterbox = true;
    UPROPERTY(BlueprintReadOnly, Category="Presentation") float LetterboxHeightFraction = .08f;
    UPROPERTY(BlueprintReadOnly, Category="Presentation") bool bCanReplay = false;
    UPROPERTY(BlueprintReadOnly, Category="Presentation") bool bCanRecordNew = false;
    UPROPERTY(BlueprintReadOnly, Category="Presentation") bool bCanReturnToMenu = false;
};
