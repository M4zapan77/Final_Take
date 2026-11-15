#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActingTypes.h"
#include "PropInteractionComponent.generated.h"
class AMovieManager;
class AInteractableProp;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRecordedPropInteraction, const FActingEvent&, Event, bool, bFromReplay);

UCLASS(ClassGroup=(Acting), BlueprintType, meta=(BlueprintSpawnableComponent))
class MYPROJECT_API UPropInteractionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPropInteractionComponent();
    /** E: collect a nearby available prop; otherwise use the equipped prop. */
    UFUNCTION(BlueprintCallable, Category="Props") bool Interact();
    UFUNCTION(BlueprintCallable, Category="Props") bool CollectProp(AInteractableProp* Prop);
    UFUNCTION(BlueprintCallable, Category="Props") bool UseEquippedProp(FName ActionId = NAME_None);
    UFUNCTION(BlueprintPure, Category="Props") bool CanInteract() const;
    UFUNCTION(BlueprintPure, Category="Props") AInteractableProp* FindNearestProp() const;
    UFUNCTION(BlueprintPure, Category="Props") FText GetInteractionPrompt() const;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Props", meta=(ClampMin="1")) double MaxInteractionDistance = 180.0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Props") FName EquippedPropId;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Props") FActingEvent LastInteraction;
    UPROPERTY(BlueprintAssignable, Category="Props") FRecordedPropInteraction OnPropInteraction;

    void BeginTake(AMovieManager* Manager);
    void EndTake();
    void ResetState();
    void ConfigureReplay(AMovieManager* Manager, FName InitialPropId);
    void ApplyEvent(const FActingEvent& Event, bool bFromReplay);
private:
    UPROPERTY(Transient) TWeakObjectPtr<AMovieManager> Movie;
    UPROPERTY(Transient) FCosmeticDefinition EquippedVisual;
    bool bActive = false;
    bool bReplayOnly = false;
};
