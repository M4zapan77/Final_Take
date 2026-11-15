#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WardrobeTypes.h"
#include "WardrobeComponent.generated.h"

class UMeshComponent;
class USkeletalMeshComponent;
class UMaterialInstanceDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWardrobeSelectionChanged, const FCosmeticSelection&, Selection);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWardrobePresentationIssue, FName, Id, FText, Message);

/** Builds only actor-owned cosmetic components and material instances. It never replaces the base rig. */
UCLASS(ClassGroup = (Acting), BlueprintType, meta = (BlueprintSpawnableComponent))
class MYPROJECT_API UWardrobeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWardrobeComponent();

    UFUNCTION(BlueprintCallable, Category = "Wardrobe")
    bool SelectItem(ECosmeticCategory Category, FName Id);

    UFUNCTION(BlueprintCallable, Category = "Wardrobe")
    bool SetSelection(const FCosmeticSelection& Selection);

    UFUNCTION(BlueprintCallable, Category = "Wardrobe")
    bool SetBodyColor(bool bUseColor, FLinearColor Color);

    UFUNCTION(BlueprintCallable, Category="Wardrobe") bool SetSkinColor(FLinearColor Color);
    UFUNCTION(BlueprintCallable, Category="Wardrobe") bool ResetCustomization();
    UFUNCTION(BlueprintCallable, Category="Wardrobe") bool RandomizeCustomization();
    UFUNCTION(BlueprintPure, Category="Wardrobe") TArray<FCosmeticDefinition> GetCategoryOptions(ECosmeticCategory Category) const;

    UFUNCTION(BlueprintCallable, Category = "Wardrobe")
    void SetSelectionEnabled(bool bEnabled) { bSelectionEnabled = bEnabled; }

    UFUNCTION(BlueprintPure, Category = "Wardrobe")
    bool CanEditSelection() const;

    UFUNCTION(BlueprintPure, Category = "Wardrobe")
    bool IsSelectionEnabled() const { return bSelectionEnabled; }

    UFUNCTION(BlueprintPure, Category = "Wardrobe")
    FCosmeticSelection GetSelection() const { return CurrentSelection; }

    UFUNCTION(BlueprintPure, Category = "Wardrobe")
    FRecordedAppearance CaptureAppearance() const;

    UFUNCTION(BlueprintPure, Category = "Wardrobe")
    FCosmeticSelection NormalizeSelection(const FCosmeticSelection& Selection) const;

    UFUNCTION(BlueprintPure, Category = "Wardrobe")
    bool ValidateCatalog(FText& OutError) const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe")
    TArray<FCosmeticDefinition> Catalog;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe")
    FCosmeticSelection DefaultSelection;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Face") FFacialSurfaceSettings FacialSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe")
    TObjectPtr<USkeletalMeshComponent> TargetMesh;

    /** Optional static body for a non-skeletal placeholder actor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe")
    TObjectPtr<UMeshComponent> FallbackBodyMesh;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Wardrobe")
    FCosmeticSelection CurrentSelection;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Wardrobe")
    TArray<TObjectPtr<UMeshComponent>> CosmeticMeshes;

    UPROPERTY(BlueprintAssignable, Category = "Wardrobe|Events")
    FWardrobeSelectionChanged OnSelectionChanged;

    UPROPERTY(BlueprintAssignable, Category = "Wardrobe|Events")
    FWardrobePresentationIssue OnPresentationIssue;

    // Coordinator entry points. Replay snapshots do not resolve against a mutable live catalog.
    bool ApplySnapshot(const FRecordedAppearance& Snapshot, USkeletalMeshComponent* Mesh, UMeshComponent* Fallback, bool bForReplay = true);
    void ClearAppearance();
    void SetHandPropsSuppressed(bool bRight,bool bLeft);
    bool BeginSelection(const FCosmeticSelection& DefaultOutfit);
    /** Authoritative recorded interaction; changes only the prop, retaining the confirmed outfit/lock. */
    bool ApplyPropVisual(const FCosmeticDefinition& Definition);
    bool FindPropVisual(FName PropId, FCosmeticDefinition& OutDefinition) const;
    static FCosmeticDefinition FreezeItem(const FCosmeticDefinition& Definition, UObject* Outer);
    void SetReadOnly(bool bReadOnly) { bReplayReadOnly = bReadOnly; }

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    const FCosmeticDefinition* FindItem(ECosmeticCategory Category, FName Id) const;
    USkeletalMeshComponent* ResolveSkeletalMesh() const;
    UMeshComponent* ResolveBodyMesh() const;
    void CacheBodyMaterials();
    void RemoveVisuals();
    void BuildVisuals(const TArray<FCosmeticDefinition>& Items);
    void ApplyMaterials(UMeshComponent* Mesh, const FCosmeticDefinition& Definition);
    void BuildFacialVisual();
    static UMaterialInterface* CopyMaterialReference(UMaterialInterface* Material, UObject* Outer);

    UPROPERTY(Transient)
    TArray<FCosmeticDefinition> AppliedItems;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInterface>> OriginalBodyMaterials;

    UPROPERTY(Transient)
    TWeakObjectPtr<UMeshComponent> CachedBody;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> OwnedMaterials;

    bool bSelectionEnabled = true;
    bool bReplayReadOnly = false;
    bool bHasAppearance = false;
    bool bSuppressRightProp=false;
    bool bSuppressLeftProp=false;
};
