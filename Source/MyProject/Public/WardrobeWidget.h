#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Styling/SlateTypes.h"
#include "WardrobeTypes.h"
#include "WardrobeWidget.generated.h"

class AMovieManager;
class UTextBlock;
class UVerticalBox;
class UProgressBar;
class UWardrobeWidget;

USTRUCT(BlueprintType)
struct MYPROJECT_API FWardrobeViewData
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category = "Wardrobe") int32 TakeNumber = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Wardrobe") int32 TakeCount = 3;
    UPROPERTY(BlueprintReadOnly, Category = "Wardrobe") FName RoleId;
    UPROPERTY(BlueprintReadOnly, Category = "Wardrobe") FText RoleName;
    UPROPERTY(BlueprintReadOnly, Category = "Wardrobe") FText ScriptInstruction;
    UPROPERTY(BlueprintReadOnly, Category = "Wardrobe") double RemainingSeconds = 0.0;
    UPROPERTY(BlueprintReadOnly, Category = "Wardrobe") double DurationSeconds = 0.0;
    UPROPERTY(BlueprintReadOnly, Category = "Wardrobe") bool bCanChoose = false;
    UPROPERTY(BlueprintReadOnly, Category = "Wardrobe") FCosmeticSelection Selection;
};

/** One button retains its own stable ID, rather than an index in a rebuilt UI. */
UCLASS()
class MYPROJECT_API UWardrobeChoiceButton : public UButton
{
    GENERATED_BODY()
public:
    void InitializeChoice(UWardrobeWidget* InOwner);
    UPROPERTY(BlueprintReadOnly, Category = "Wardrobe") ECosmeticCategory Category = ECosmeticCategory::Head;
    UPROPERTY(BlueprintReadOnly, Category = "Wardrobe") FName CosmeticId;
    bool bColorChoice = false;
    bool bUseColor = false;
    FLinearColor Color = FLinearColor::White;
private:
    UFUNCTION() void HandleClick();
    UPROPERTY(Transient) TWeakObjectPtr<UWardrobeWidget> WardrobeOwner;
};

/** Functional native UMG fallback. A Widget Blueprint subclass can supply its own designer tree. */
UCLASS(BlueprintType, Blueprintable)
class MYPROJECT_API UWardrobeWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UWardrobeWidget(const FObjectInitializer& ObjectInitializer);

    virtual void OpenWardrobe(AMovieManager* Manager, int32 InPreparationId);

    UFUNCTION(BlueprintCallable, Category = "Wardrobe") bool Ready();
    UFUNCTION(BlueprintCallable, Category = "Wardrobe") bool ChooseItem(ECosmeticCategory Category, FName Id);
    UFUNCTION(BlueprintCallable, Category = "Wardrobe") bool ChooseBodyColor(bool bUseColor, FLinearColor Color);
    UFUNCTION(BlueprintCallable, Category = "Wardrobe") virtual void RefreshWardrobe();
    UFUNCTION(BlueprintPure, Category = "Wardrobe") TArray<FCosmeticDefinition> GetOptions(ECosmeticCategory Category) const;
    UFUNCTION(BlueprintPure, Category = "Wardrobe") AMovieManager* GetMovieManager() const { return MovieManager.Get(); }

    UPROPERTY(BlueprintReadOnly, Transient, Category = "Wardrobe") FWardrobeViewData ViewData;
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Wardrobe") int32 PreparationId = 0;
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Wardrobe") TArray<TObjectPtr<UWardrobeChoiceButton>> ChoiceButtons;
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Wardrobe") TObjectPtr<UButton> ReadyButton;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wardrobe|Style") FLinearColor PanelColor = FLinearColor(.025f,.03f,.05f,.96f);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wardrobe|Style") FLinearColor TextColor = FLinearColor(.95f,.95f,.95f);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wardrobe|Style") FLinearColor AccentColor = FLinearColor(1.0f,.55f,.12f);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wardrobe|Style", meta = (ClampMin = "320", ClampMax = "900")) float PanelWidth = 480.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wardrobe|Style", meta = (ClampMin = "12", ClampMax = "36")) int32 FontSize = 18;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wardrobe|Style") bool bUseCustomButtonStyle = false;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wardrobe|Style") FButtonStyle ButtonStyle;

    UFUNCTION(BlueprintImplementableEvent, Category = "Wardrobe") void OnWardrobeOpened();
    UFUNCTION(BlueprintImplementableEvent, Category = "Wardrobe") void OnWardrobeViewChanged(const FWardrobeViewData& Data);

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
    virtual void BuildNativeLayout();
private:
    void BuildChoices();
    UTextBlock* MakeText(const FText& Text, int32 Size);
    UFUNCTION() void HandleReadyClicked();

    UPROPERTY(Transient) TWeakObjectPtr<AMovieManager> MovieManager;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> ChoicePanel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Header;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Role;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Script;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Countdown;
    UPROPERTY(Transient) TObjectPtr<UProgressBar> Progress;
};
