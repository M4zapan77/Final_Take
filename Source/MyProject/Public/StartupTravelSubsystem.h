#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "Engine/EngineBaseTypes.h"
#include "StartupTravelSubsystem.generated.h"
class SWidget;

/** Keeps the black input shield alive across OpenLevel; startup intros are per application session. */
UCLASS()
class MYPROJECT_API UStartupTravelSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    static UStartupTravelSubsystem* Find(const UObject* Context);
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override;
    virtual UWorld* GetTickableGameObjectWorld() const override;
    bool BeginGameplayTravel(const TSoftObjectPtr<UWorld>& Level,double FadeSeconds);
    bool ConsumeGameplayIntent();
    void MarkGameplayReady();
    void MarkIntroductionsSeen() { bIntroductionsSeen=true; }
    UFUNCTION(BlueprintPure, Category="Startup Videos") bool HaveIntroductionsPlayed() const { return bIntroductionsSeen; }
    UFUNCTION(BlueprintPure, Category="Startup Videos") bool IsGameplayTransitionActive() const { return bTravelPending; }
    UFUNCTION(BlueprintPure, Category="Startup Videos") float GetBlackOpacity() const { return BlackOpacity; }
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Startup Videos") int32 GameplayLoads=0;
private:
    void OnMapLoaded(UWorld* World);
    void OnTravelFailed(UWorld* World,ETravelFailure::Type Type,const FString& Error);
    void RemoveBlack(bool bRestoreFocus);
    TSharedPtr<SWidget> BlackShield;
    FDelegateHandle MapHandle,FailureHandle;
    double FadeDuration=.3,FadeElapsed=0;
    int32 ReadyFrames=0;
    float BlackOpacity=0;
    bool bIntroductionsSeen=false,bTravelPending=false,bIntentConsumed=false,bMapLoaded=false,bLevelReady=false;
};
