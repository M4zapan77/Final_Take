#include "Testing/ActingRecorderTestGameMode.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Testing/ActingRecorderTestActor.h"
#include "Testing/ActingRecorderTestCharacter.h"

AActingRecorderTestGameMode::AActingRecorderTestGameMode()
{
    DefaultPawnClass = AActingRecorderTestCharacter::StaticClass();
}

void AActingRecorderTestGameMode::StartPlay()
{
    Super::StartPlay();
    TActorIterator<AActingRecorderTestActor> ExistingTest(GetWorld());
    if (!ExistingTest)
    {
        GetWorld()->SpawnActor<AActingRecorderTestActor>();
    }
}
