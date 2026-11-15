#include "Modules/ModuleManager.h"

#if !UE_BUILD_SHIPPING
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/DefaultValueHelper.h"
#include "MovieManager.h"
#include "Testing/ActingRecorderTestActor.h"

namespace
{
    void SpawnActingRecorderTest(UWorld* World)
    {
        if (!World || !World->IsGameWorld())
        {
            UE_LOG(LogTemp, Warning, TEXT("Acting.Test: start Play first, then run this command in the game viewport."));
            return;
        }
        TActorIterator<AMovieManager> ExistingMovie(World);
        if (ExistingMovie)
        {
            UE_LOG(LogTemp, Warning, TEXT("Acting.Test: use a fresh Play session without MovieManager for the phase-one test."));
            return;
        }
        TActorIterator<AActingRecorderTestActor> ExistingTest(World);
        if (!ExistingTest)
        {
            World->SpawnActor<AActingRecorderTestActor>();
        }
    }

    FAutoConsoleCommandWithWorld ActingTestCommand(
        TEXT("Acting.Test"), TEXT("Spawn the recording test harness in the current gameplay world. R starts a ten-second take."),
        FConsoleCommandWithWorldDelegate::CreateStatic(&SpawnActingRecorderTest));

    void StartMovieTest(const TArray<FString>& Args, UWorld* World)
    {
        if (!World || !World->IsGameWorld())
        {
            UE_LOG(LogTemp, Warning, TEXT("Movie.Test: enter Play first."));
            return;
        }
        TActorIterator<AActingRecorderTestActor> LegacyTest(World);
        if (LegacyTest)
        {
            UE_LOG(LogTemp, Warning, TEXT("Movie.Test: start a fresh Play session without Acting.Test; both coordinators use the scene clock."));
            return;
        }
        double Preparation = 10.0;
        double Recording = 30.0;
        if (!(Args.IsEmpty() || (Args.Num() == 2 && FDefaultValueHelper::ParseDouble(Args[0], Preparation)
            && FDefaultValueHelper::ParseDouble(Args[1], Recording) && FMath::IsFinite(Preparation) && Preparation >= 0.0
            && FMath::IsFinite(Recording) && Recording > 0.0)))
        {
            UE_LOG(LogTemp, Warning, TEXT("Usage: Movie.Test [PreparationSeconds RecordingSeconds], for example Movie.Test 1 3"));
            return;
        }
        TActorIterator<AMovieManager> Existing(World);
        AMovieManager* Manager = Existing ? *Existing : World->SpawnActor<AMovieManager>();
        if (!Manager) { return; }
        Manager->PreparationDurationSeconds = Preparation;
        Manager->RecordingDurationSeconds = Recording;
        if (Manager->HasActiveSession()) { Manager->RestartMovie(); }
        else { Manager->StartMovie(); }
    }

    FAutoConsoleCommandWithWorldAndArgs MovieTestCommand(
        TEXT("Movie.Test"), TEXT("Run three takes plus a fixed-camera premiere. Optional preparation/recording durations. R restarts."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&StartMovieTest));
}
#endif

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, MyProject, "MyProject");
