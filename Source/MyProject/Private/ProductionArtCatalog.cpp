#include "ProductionArtCatalog.h"

void UProductionArtCatalog::ApplyTo(TArray<FCosmeticDefinition>& Catalog) const
{
    for (const FCosmeticDefinition& Art : Cosmetics)
    {
        if (Art.Id.IsNone())
        {
            continue;
        }
        if (FCosmeticDefinition* Existing =
                Catalog.FindByPredicate([&](const auto& Item) { return Item.Id == Art.Id; }))
        {
            if (Existing->Category != Art.Category)
            {
                continue;
            }
            FCosmeticDefinition Resolved = Art;
            for (FName Tag : Existing->CosmeticTags)
            {
                Resolved.CosmeticTags.AddUnique(Tag);
            }
            if (Resolved.UseGestureId.IsNone())
            {
                Resolved.UseGestureId = Existing->UseGestureId;
            }
            *Existing = MoveTemp(Resolved);
        }
        else
        {
            Catalog.Add(Art);
        }
    }
}
