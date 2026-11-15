#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StudioFlowTypes.h"
#include "StudioAudioSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStudioAudioChanged, const FStudioAudioLevels&, Levels);

/** Session preferences. Applied to owned music/SFX, never written into a recorded performance. */
UCLASS()
class MYPROJECT_API UStudioAudioSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Studio|Audio") void SetLevels(FStudioAudioLevels NewLevels);
    UFUNCTION(BlueprintPure, Category="Studio|Audio") FStudioAudioLevels GetLevels() const { return Levels; }
    UPROPERTY(BlueprintAssignable, Category="Studio|Audio") FStudioAudioChanged OnAudioChanged;
    void InitializeDefaults(const FStudioAudioLevels& Defaults);
    static UStudioAudioSubsystem* Find(const UObject* Context);
    static float MusicMultiplier(const UObject* Context);
    static float EffectsMultiplier(const UObject* Context);
    UFUNCTION(BlueprintPure, Category="Studio|Audio") static float AmbientMultiplier(const UObject* Context);
    UFUNCTION(BlueprintPure, Category="Studio|Audio") static float UIMultiplier(const UObject* Context);
private:
    UPROPERTY(Transient) FStudioAudioLevels Levels;
    bool bConfigured = false;
};
