#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StoryWorldTypes.h"
#include "StoryScenarioData.generated.h"

/** One bounded bank setup, copied once when a movie starts. Outcomes still follow real state. */
USTRUCT(BlueprintType)
struct MYPROJECT_API FStoryScenarioDefinition
{
    GENERATED_BODY()
    FStoryScenarioDefinition();
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Scenario") FName Id=TEXT("Bank.ThePerfectRobbery");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Scenario") FText Name;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Scenario",meta=(MultiLine=true)) FText Synopsis;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Positions") FVector SceneAnchorOffset=FVector(650,0,0);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Positions") TArray<FVector> RoleOffsets;
    /** Teller, guard, witness 1, witness 2. The validated core keeps these stable identities. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Positions") TArray<FVector> NPCOffsets;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Positions") FVector MoneyOffset=FVector(-350,-430,0);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Positions",meta=(ClampMin="0",ClampMax="200")) float MoneySurfaceHeight=110;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Positions") FVector ExitOffset=FVector(-600,0,0);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Positions") FVector SecureOffset=FVector(320,160,0);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Initial Conditions") bool bAlarmInitiallyActive=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Initial Conditions") bool bGuardInitiallyDistracted=false;
    /** Optional starting owner must match one of MovieManager's three stable role IDs. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Initial Conditions") FName InitialMoneyOwner;
    /** Core E interactions remain available so the six-turn story cannot lose its required actions. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Actions") TArray<FName> AvailablePropActions;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Objectives") TMap<EStoryObjectiveKind,FText> ObjectiveTextOverrides;
    /** Display text only: a scenario cannot invent a different winner from the authoritative money state. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Outcomes") TMap<EStoryOutcome,FText> OutcomeTexts;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Director") TMap<EStoryEventType,int32> CinematicPriorities;
    bool IsValid() const;
};

UCLASS(BlueprintType)
class MYPROJECT_API UStoryScenarioData:public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Scenario") FStoryScenarioDefinition Definition;
};
