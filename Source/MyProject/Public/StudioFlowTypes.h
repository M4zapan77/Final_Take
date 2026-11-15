#pragma once

#include "CoreMinimal.h"
#include "StudioFlowTypes.generated.h"

class USoundBase;

UENUM(BlueprintType)
enum class EStudioScreen : uint8
{
    MainMenu, HowToPlay, Credits, AudioSettings, MovieIntroduction,
    Wardrobe, Recording, NextCharacter, Premiere, Results, Error,
    StartupVideos, StartingGame
};

UENUM(BlueprintType)
enum class EDirectorCue : uint8 { ChooseCostume, Action, Cut, NextActor, Wrap };

USTRUCT(BlueprintType)
struct MYPROJECT_API FStudioAudioLevels
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0",ClampMax="1")) float Master = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0",ClampMax="1")) float Music = .65f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0",ClampMax="1")) float Effects = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0",ClampMax="1")) float Ambient = .8f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0",ClampMax="1")) float UI = .8f;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FDirectorAnnouncement
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Director") EDirectorCue Cue = EDirectorCue::Action;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Director") FText Text;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Director") TObjectPtr<USoundBase> Sound;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Director", meta=(ClampMin="0")) float DurationSeconds = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Director", meta=(ClampMin="0")) float Volume = 1.f;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FStudioFlowSettings
{
    GENERATED_BODY()
    FStudioFlowSettings();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Text") FText GameTitle;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Text", meta=(MultiLine=true)) FText MovieSynopsis;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Text", meta=(MultiLine=true)) FText HowToPlay;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Text", meta=(MultiLine=true)) FText Credits;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Timing", meta=(ClampMin="0")) double PageFadeSeconds = .2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Timing", meta=(ClampMin="0")) double BetweenActorsSeconds = 2.5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Timing", meta=(ClampMin="0")) double WrapSeconds = 2.25;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Timing", meta=(ClampMin="0")) double NextActorCueDelay = .6;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Director") TArray<FDirectorAnnouncement> Announcements;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") FStudioAudioLevels DefaultAudio;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> MenuMusic;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> IntroductionMusic;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> WardrobeMusic;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> RecordingMusic;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> ButtonSound;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> PremiereMusic;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> ResultsMusic;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> BankAmbience;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> CinemaAmbience;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio") TObjectPtr<USoundBase> ClapperSound;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin=".05",ClampMax="2")) float MusicFadeSeconds = .35f;
    bool IsValid() const;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FStudioRoleBrief
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Studio") FName RoleId;
    UPROPERTY(BlueprintReadOnly, Category="Studio") FText Name;
    UPROPERTY(BlueprintReadOnly, Category="Studio") FText Script;
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FStudioFlowView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Studio") EStudioScreen Screen = EStudioScreen::MainMenu;
    UPROPERTY(BlueprintReadOnly, Category="Studio") int32 Revision = 0;
    UPROPERTY(BlueprintReadOnly, Category="Studio") FText GameTitle;
    UPROPERTY(BlueprintReadOnly, Category="Studio") FText MovieTitle;
    UPROPERTY(BlueprintReadOnly, Category="Studio") FText Synopsis;
    UPROPERTY(BlueprintReadOnly, Category="Studio") FText HowToPlay;
    UPROPERTY(BlueprintReadOnly, Category="Studio") FText Credits;
    UPROPERTY(BlueprintReadOnly, Category="Studio") TArray<FStudioRoleBrief> Roles;
    UPROPERTY(BlueprintReadOnly, Category="Studio") FText CurrentRole;
    UPROPERTY(BlueprintReadOnly, Category="Studio") FText CurrentScript;
    UPROPERTY(BlueprintReadOnly, Category="Studio") int32 TakeNumber = 0;
    UPROPERTY(BlueprintReadOnly, Category="Studio") int32 RoundNumber = 1;
    UPROPERTY(BlueprintReadOnly, Category="Studio") bool bChronologicalMovie=false;
    UPROPERTY(BlueprintReadOnly, Category="Studio") double RemainingSeconds = 0;
    UPROPERTY(BlueprintReadOnly, Category="Studio") FText DirectorText;
    UPROPERTY(BlueprintReadOnly, Category="Studio") float DirectorOpacity = 0;
    UPROPERTY(BlueprintReadOnly, Category="Studio") float PageOpacity = 1;
    UPROPERTY(BlueprintReadOnly, Category="Studio") FStudioAudioLevels Audio;
    UPROPERTY(BlueprintReadOnly, Category="Studio") FText Error;
};
