#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WardrobeTypes.h"
#include "InteractableProp.generated.h"
class UStaticMeshComponent;

/** Each take gets a fresh live pickup. Replays equip their own visual copy. */
UCLASS(Blueprintable)
class MYPROJECT_API AInteractableProp : public AActor
{
    GENERATED_BODY()
public:
    AInteractableProp();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prop") FName PropId = TEXT("Prop.MoneyBag");
    /** Optional persistent ID. Empty generates one once per gameplay world, retained across takes/restarts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prop") FName PickupId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prop") FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prop", meta=(ClampMin="1")) double InteractionDistance = 180.0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prop") bool bUsePerformerCatalogVisual = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prop") FCosmeticDefinition EquippedVisual;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Prop") TObjectPtr<UStaticMeshComponent> PickupMesh;
    UFUNCTION(BlueprintPure, Category="Prop") bool IsAvailable() const { return bAvailable && !IsHidden(); }
    UFUNCTION(BlueprintCallable, Category="Prop") void ResetForTake();
    UFUNCTION(BlueprintCallable, Category="Prop") void CaptureInitialState();
    void MarkCollected();
    void ShowCollectedInPremiere();
protected:
    virtual void BeginPlay() override;
    void ConfigureDefault(FName ItemId, const TCHAR* Label);
private:
    FTransform InitialTransform;
    bool bInitialHidden = false;
    bool bAvailable = true;
};

UCLASS(Blueprintable) class MYPROJECT_API AMoneyBagProp : public AInteractableProp { GENERATED_BODY() public: AMoneyBagProp(); };
UCLASS(Blueprintable) class MYPROJECT_API ADonutProp : public AInteractableProp { GENERATED_BODY() public: ADonutProp(); };
UCLASS(Blueprintable) class MYPROJECT_API ABananaProp : public AInteractableProp { GENERATED_BODY() public: ABananaProp(); };
