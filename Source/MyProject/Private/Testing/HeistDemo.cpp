#if !UE_BUILD_SHIPPING
#include "ObjectiveZone.h"
#include "InteractableProp.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    void SpawnHeistDemo(UWorld* World)
    {
        if (!World || !World->IsGameWorld()) { return; }
        APawn* Pawn = UGameplayStatics::GetPlayerPawn(World,0);
        if (!Pawn) { return; }
        if (TActorIterator<AObjectiveZone> Existing(World); Existing)
        { UE_LOG(LogTemp, Display, TEXT("Heist.Demo: objective zones already exist; using your setup.")); return; }
        const FTransform Origin = Pawn->GetActorTransform();
        FActorSpawnParameters Parameters; Parameters.ObjectFlags |= RF_Transient;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        const auto Zone = [&](FName Tag, FVector Offset, FVector Extent)
        {
            AObjectiveZone* Result = World->SpawnActor<AObjectiveZone>(Origin.TransformPositionNoScale(Offset), Origin.Rotator(), Parameters);
            Result->ZoneTag = Tag; Result->Bounds->SetBoxExtent(Extent);
            Result->RefreshMarker();
        };
        Zone(TEXT("Bank.Entry"), FVector(350,0,50), FVector(150,300,150));
        Zone(TEXT("Bank.Counter"), FVector(650,-300,50), FVector(110,110,150));
        Zone(TEXT("Bank.Escape"), FVector(1100,0,50), FVector(140,140,150));
        World->SpawnActor<AMoneyBagProp>(Origin.TransformPositionNoScale(FVector(430,0,-45)), Origin.Rotator(), Parameters);
        World->SpawnActor<ADonutProp>(Origin.TransformPositionNoScale(FVector(430,180,-45)), Origin.Rotator(), Parameters);
        World->SpawnActor<ABananaProp>(Origin.TransformPositionNoScale(FVector(520,-180,-45)), Origin.Rotator(), Parameters);
        UE_LOG(LogTemp, Display, TEXT("Heist.Demo: three temporary bank zones and three props added. Run Movie.Test. E collects/uses props; no map was saved."));
    }
    FAutoConsoleCommandWithWorld HeistDemoCommand(TEXT("Heist.Demo"), TEXT("Add temporary bank zones and props near the live player, then use Movie.Test."),
        FConsoleCommandWithWorldDelegate::CreateStatic(&SpawnHeistDemo));
}
#endif
