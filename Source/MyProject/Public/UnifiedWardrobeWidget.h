#pragma once
#include "CoreMinimal.h"
#include "WardrobeWidget.h"
#include "UnifiedWardrobeWidget.generated.h"
class AWardrobePreviewActor;
class UMaterialInstanceDynamic;
class UImage;
class UWrapBox;
class UHorizontalBox;
class UBorder;

UENUM(BlueprintType)
enum class ECustomizationTab : uint8
{
    Eyes,
    Mouth,
    Eyebrows,
    Skin,
    Clothing,
    Hats,
    Accessories,
    Objects
};
UENUM()
enum class ECustomizationButtonAction : uint8
{
    Item,
    Tab,
    Skin,
    Ready,
    Random,
    Reset,
    RotateLeft,
    RotateRight,
    LeftHand,
    RightHand
};

USTRUCT(BlueprintType)
struct MYPROJECT_API FSkinColorPreset
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Label;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor Color = FLinearColor::White;
};
class UUnifiedWardrobeWidget;
UCLASS()
class MYPROJECT_API UCustomizationOptionButton : public UButton
{
    GENERATED_BODY()
public:
    void Bind(UUnifiedWardrobeWidget* Widget);
    UPROPERTY(BlueprintReadOnly) FName ControlId;
    UPROPERTY(BlueprintReadOnly) FName CosmeticId;
    ECustomizationButtonAction Action = ECustomizationButtonAction::Item;
    ECustomizationTab Tab = ECustomizationTab::Eyes;
    ECosmeticCategory Category = ECosmeticCategory::Eyes;
    FLinearColor SkinColor = FLinearColor::White;
    UPROPERTY() TObjectPtr<UTextBlock> Caption;

private:
    UFUNCTION() void Click();
    UPROPERTY() TWeakObjectPtr<UUnifiedWardrobeWidget> OwnerWidget;
};

/** The active preparation UI: one shared wardrobe state, eight tabs and an isolated 3D portrait. */
UCLASS(Blueprintable, BlueprintType)
class MYPROJECT_API UUnifiedWardrobeWidget : public UWardrobeWidget
{
    GENERATED_BODY()
public:
    UUnifiedWardrobeWidget(const FObjectInitializer& Initializer);
    virtual void OpenWardrobe(AMovieManager* Manager, int32 Id) override;
    virtual void RefreshWardrobe() override;
    UFUNCTION(BlueprintCallable, Category = "Customization") void SelectTab(ECustomizationTab Tab);
    UFUNCTION(BlueprintCallable, Category = "Customization") bool ChooseSkin(FLinearColor Color);
    UFUNCTION(BlueprintCallable, Category = "Customization") bool ResetAll();
    UFUNCTION(BlueprintCallable, Category = "Customization") bool RandomizeAll();
    UFUNCTION(BlueprintCallable, Category = "Customization") void SelectHand(bool bLeft);
    UFUNCTION(BlueprintCallable, Category = "Customization") void RotatePreview(float DeltaDegrees);
    UFUNCTION(BlueprintPure, Category = "Customization") UCustomizationOptionButton* FindControl(FName Id) const;
    UFUNCTION(BlueprintPure, Category = "Customization") UMaterialInstanceDynamic* GetPreviewMaterial() const
    {
        return PreviewMaterial;
    }
    void Activate(UCustomizationOptionButton* Button);
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Customization")
    ECustomizationTab ActiveTab = ECustomizationTab::Eyes;
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Customization") bool bSelectLeftHand = false;
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Customization")
    TArray<TObjectPtr<UCustomizationOptionButton>> Controls;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Customization|Style")
    FLinearColor Navy = FLinearColor(.018, .018, .046, 1);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Customization|Style")
    FLinearColor Purple = FLinearColor(.065, .06, .12, 1);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Customization|Style")
    FLinearColor Cream = FLinearColor(.92, .87, .74, 1);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Customization|Style")
    FLinearColor Gold = FLinearColor(1, .68, .12, 1);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Customization|Style")
    FLinearColor Red = FLinearColor(.62, .07, .10, 1);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Customization|Style") FSlateFontInfo HeadingFont;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Customization|Style") TObjectPtr<UTexture2D> Logo;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Customization|Style")
    TObjectPtr<UMaterialInterface> PreviewUIMaterial;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Customization|Style")
    TSubclassOf<AWardrobePreviewActor> PreviewActorClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Customization|Style")
    FVector2D CardSize = FVector2D(1240, 760);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Customization|Style", meta=(ClampMin="10",ClampMax="30"))
    int32 OptionFontSize=18;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Customization|Skin")
    TArray<FSkinColorPreset> SkinPresets;
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Customization") TObjectPtr<AWardrobePreviewActor> PreviewActor;
    UFUNCTION(BlueprintImplementableEvent, Category = "Customization")
    void OnCustomizationTabChanged(ECustomizationTab Tab);

protected:
    virtual void BuildNativeLayout() override;
    virtual void NativeDestruct() override;

private:
    UTextBlock* Label(const FText& Text, int32 Size, FLinearColor Color);
    UCustomizationOptionButton* Button(const FText& Text, FName Id, ECustomizationButtonAction Action);
    UBorder* Card(FLinearColor Color, float Radius = 14);
    void BuildOptions();
    void UpdatePreview();
    bool CanCustomize() const;
    static FText ItemLabel(const FCosmeticDefinition& Item);
    UPROPERTY() TObjectPtr<UWrapBox> Options;
    UPROPERTY() TObjectPtr<UHorizontalBox> HandRow;
    UPROPERTY() TObjectPtr<UTextBlock> MovieLabel;
    UPROPERTY() TObjectPtr<UTextBlock> RoleLabel;
    UPROPERTY() TObjectPtr<UTextBlock> TakeLabel;
    UPROPERTY() TObjectPtr<UTextBlock> TimerLabel;
    UPROPERTY() TObjectPtr<UTextBlock> ObjectivesLabel;
    UPROPERTY() TObjectPtr<UImage> Portrait;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> PreviewMaterial;
    FCosmeticSelection PreviewSelection;
    bool bHasPreviewSelection = false;
};
