#pragma once
#include "CoreMinimal.h"
#include "StoryWorldTypes.generated.h"

UENUM(BlueprintType)
enum class EStoryEventType:uint8 { MoneyTaken,MoneyTransferred,MoneyConfiscated,MoneyDropped,MoneySecured,Escape,Detain,GuardDistracted,AlarmChanged,NPCReaction,NPCInformation,NPCAssisted,PropGrabbed,PropDropped,PropThrown,PropImpact,NPCStateChanged,PresentationStop,PropCreated,PropRecovered };
UENUM(BlueprintType)
enum class EStoryNPCType:uint8 { Teller,Guard,Witness };
UENUM(BlueprintType)
enum class EStoryNPCMood:uint8 { Idle,Alert,Frightened,Distracted,Helpful };
UENUM(BlueprintType)
enum class EStoryObjectiveKind:uint8 { GetMoney,ConfiscateMoney,SecureMoney,Escape,DistractGuard,AssistPolice,AskWitness,RecoverMoney,ReactToEnding };
UENUM(BlueprintType)
enum class EStoryOutcome:uint8 { Unresolved,RobberEscaped,PoliceRecovered,CivilianBetrayal,FailedRobbery };

USTRUCT(BlueprintType)
struct MYPROJECT_API FStorySceneEvent
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="Story") int32 Sequence=0;
    UPROPERTY(BlueprintReadOnly,Category="Story") double TimeSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="Story") EStoryEventType Type=EStoryEventType::MoneyTaken;
    UPROPERTY(BlueprintReadOnly,Category="Story") FName InstigatorId;
    UPROPERTY(BlueprintReadOnly,Category="Story") FName TargetId;
    UPROPERTY(BlueprintReadOnly,Category="Story") FName ObjectId;
    UPROPERTY(BlueprintReadOnly,Category="Story") FName Payload;
    UPROPERTY(BlueprintReadOnly,Category="Story") FTransform WorldTransform=FTransform::Identity;
    UPROPERTY(BlueprintReadOnly,Category="Story") double Duration=0;
};
USTRUCT(BlueprintType)
struct MYPROJECT_API FStoryObjectState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="Story") FName ObjectId;
    UPROPERTY(BlueprintReadOnly,Category="Story") FName CosmeticId;
    /** None means one world location. This is the sole source of ownership. */
    UPROPERTY(BlueprintReadOnly,Category="Story") FName OwnerId;
    UPROPERTY(BlueprintReadOnly,Category="Story") bool bLeftHand=false;
    UPROPERTY(BlueprintReadOnly,Category="Story") double CreatedAtSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="Story") FTransform WorldTransform=FTransform::Identity;
};
USTRUCT(BlueprintType)
struct MYPROJECT_API FStoryNPCState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="Story") FName Id;
    UPROPERTY(BlueprintReadOnly,Category="Story") EStoryNPCType Type=EStoryNPCType::Witness;
    UPROPERTY(BlueprintReadOnly,Category="Story") EStoryNPCMood Mood=EStoryNPCMood::Idle;
    UPROPERTY(BlueprintReadOnly,Category="Story") FTransform WorldTransform=FTransform::Identity;
    UPROPERTY(BlueprintReadOnly,Category="Story") double EffectUntilSeconds=0;
};
USTRUCT(BlueprintType)
struct MYPROJECT_API FStoryWorldState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="Story") FName MoneyObjectId;
    UPROPERTY(BlueprintReadOnly,Category="Story") TArray<FStoryObjectState> Objects;
    UPROPERTY(BlueprintReadOnly,Category="Story") TArray<FStoryNPCState> NPCs;
    UPROPERTY(BlueprintReadOnly,Category="Story") bool bAlarmActive=false;
    UPROPERTY(BlueprintReadOnly,Category="Story") bool bMoneySecured=false;
    UPROPERTY(BlueprintReadOnly,Category="Story") bool bRobberEscaped=false;
    UPROPERTY(BlueprintReadOnly,Category="Story") bool bMoneyWasTaken=false;
    UPROPERTY(BlueprintReadOnly,Category="Story") TMap<FName,double> DetainedUntil;
    FStoryObjectState* FindObject(FName Id) { return Objects.FindByPredicate([Id](const auto& O){return O.ObjectId==Id;}); }
    const FStoryObjectState* FindObject(FName Id) const { return Objects.FindByPredicate([Id](const auto& O){return O.ObjectId==Id;}); }
    FName MoneyOwner() const { const auto* Money=FindObject(MoneyObjectId);return Money?Money->OwnerId:NAME_None; }
    bool HasUniqueMoney() const;
    EStoryOutcome ResolveOutcome(FName RobberId,FName PoliceId,FName CivilianId) const;
};
USTRUCT(BlueprintType)
struct MYPROJECT_API FStoryScoreBreakdown
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="Story") int32 Story=0;
    UPROPERTY(BlueprintReadOnly,Category="Story") int32 Interactions=0;
    UPROPERTY(BlueprintReadOnly,Category="Story") int32 Creativity=0;
    UPROPERTY(BlueprintReadOnly,Category="Story") int32 PropsAndNPCs=0;
    int32 Total() const { return Story+Interactions+Creativity+PropsAndNPCs; }
};
