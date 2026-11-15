#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StudioGameMode.generated.h"

class AMovieManager;
class AStudioFlowManager;

/** Optional packaged-game entry point. Uses the existing Third Person character and creates one movie manager. */
UCLASS(Blueprintable)
class MYPROJECT_API AStudioGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AStudioGameMode();
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Movie") TSubclassOf<AMovieManager> MovieManagerClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Movie") bool bStartMovieAutomatically = true;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Studio") bool bUseFrontEnd = true;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Studio") TSubclassOf<AStudioFlowManager> StudioFlowClass;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Studio") TObjectPtr<AStudioFlowManager> ActiveStudioFlow;
    UFUNCTION(BlueprintPure, Category="Studio") bool ShouldUseFrontEnd() const;
protected:
    virtual void Tick(float DeltaSeconds) override;
private:
    bool bMovieInitialized = false;
};
