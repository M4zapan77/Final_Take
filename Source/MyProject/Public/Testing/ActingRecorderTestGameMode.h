#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ActingRecorderTestGameMode.generated.h"

/** Only the milestone test: placeholder pawn plus one runtime test actor. */
UCLASS()
class MYPROJECT_API AActingRecorderTestGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AActingRecorderTestGameMode();
    virtual void StartPlay() override;
};
