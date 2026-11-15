#include "StudioGameMode.h"
#include "MovieManager.h"
#include "StudioFlowManager.h"
#include "StudioCharacterProfile.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

AStudioGameMode::AStudioGameMode()
{
    PrimaryActorTick.bCanEverTick=true;
    MovieManagerClass=AMovieManager::StaticClass();
    StudioFlowClass=AStudioFlowManager::StaticClass();
    // Reuse the project's character Blueprint; this does not modify it or its movement/rig/input.
    static ConstructorHelpers::FClassFinder<APawn> Character(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
    if (Character.Succeeded()) { DefaultPawnClass=Character.Class; }
    static ConstructorHelpers::FClassFinder<APlayerController> PlayerControl(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController"));
    if (PlayerControl.Succeeded()) { PlayerControllerClass=PlayerControl.Class; }
}

void AStudioGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bMovieInitialized || !bStartMovieAutomatically) { return; }
    APlayerController* Player=UGameplayStatics::GetPlayerController(this,0);
    if (!Player || !Player->GetPawn()) { return; }
    TActorIterator<AMovieManager> Existing(GetWorld());
    AMovieManager* Manager=Existing?*Existing:GetWorld()->SpawnActor<AMovieManager>(MovieManagerClass);
    if (!Manager) { return; }
    if(Manager->CharacterProfile){Manager->CharacterProfile->ApplyModel(Cast<ACharacter>(Player->GetPawn()));}
    bMovieInitialized=true;
    if (ShouldUseFrontEnd())
    {
        Manager->StopMovie();
        TActorIterator<AStudioFlowManager> ExistingFlow(GetWorld());
        ActiveStudioFlow=ExistingFlow?*ExistingFlow:GetWorld()->SpawnActor<AStudioFlowManager>(StudioFlowClass);
        if (!ActiveStudioFlow || !ActiveStudioFlow->InitializeFlow(Manager,Player))
        { UE_LOG(LogTemp,Error,TEXT("Could not initialize the studio menu.")); }
        return;
    }
    if (!Manager->HasActiveSession()) { Manager->StartMovie(); }
}

bool AStudioGameMode::ShouldUseFrontEnd() const
{
    if (!bUseFrontEnd || !bStartMovieAutomatically) { return false; }
#if !UE_BUILD_SHIPPING
    // Legacy isolated smoke tests opt out explicitly. Normal launch always shows the menu.
    if (FParse::Param(FCommandLine::Get(),TEXT("StudioSkipMenu"))) { return false; }
#endif
    return true;
}
