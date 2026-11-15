#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WardrobeTypes.h"
#include "ProductionArtCatalog.generated.h"

/** Director supplied assets override the existing stable IDs without changing recording data. */
UCLASS(BlueprintType)
class MYPROJECT_API UProductionArtCatalog : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe") TArray<FCosmeticDefinition> Cosmetics;
    void ApplyTo(TArray<FCosmeticDefinition>& Catalog) const;
};
