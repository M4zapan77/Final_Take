#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Fonts/SlateFontInfo.h"
#include "MovieResultsTypes.h"
#include "MoviePresentationWidget.generated.h"

class AMovieManager;
class UBorder;
class UButton;
class UCanvasPanel;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UTexture2D;
class UStudioUITheme;
class UImage;

/** Native title card, letterbox and results. A Widget Blueprint with a Designer root keeps its own layout. */
UCLASS(Blueprintable)
class MYPROJECT_API UMoviePresentationWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UMoviePresentationWidget(const FObjectInitializer& ObjectInitializer);
    void BindMovie(AMovieManager* Manager, int32 ExpectedPresentationId);
    void FocusResults();
    UFUNCTION(BlueprintCallable, Category="Movie|UI") void RefreshPresentation();
    UFUNCTION(BlueprintCallable, Category="Movie|UI") bool ReplayMovie();
    UFUNCTION(BlueprintCallable, Category="Movie|UI") bool RecordNewMovie();
    UFUNCTION(BlueprintCallable, Category="Movie|UI") bool ReturnToMainMenu();
    UPROPERTY(BlueprintReadOnly, Transient, Category="Movie|UI") FMoviePresentationView ViewData;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Movie|UI") TObjectPtr<UButton> ReplayButton;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Movie|UI") TObjectPtr<UButton> NewMovieButton;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Movie|UI") TObjectPtr<UButton> MainMenuButton;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") FSlateFontInfo TitleFont;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") FSlateFontInfo ScoreFont;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") FSlateFontInfo BodyFont;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") FLinearColor ResultPanelColor = FLinearColor(.02f,.025f,.04f,.97f);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") FLinearColor ResultTextColor = FLinearColor::White;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") FLinearColor AccentColor = FLinearColor(1,.65f,.15f);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") FLinearColor LetterboxColor = FLinearColor::Black;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") TObjectPtr<UTexture2D> TitleBackgroundImage;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") TObjectPtr<UTexture2D> ResultsBackgroundImage;
    UPROPERTY(EditDefaultsOnly,BlueprintReadWrite,Category="Style") TObjectPtr<UStudioUITheme> UITheme;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Texts") FText ReplayButtonText;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Texts") FText NewMovieButtonText;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Texts") FText MainMenuButtonText;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Texts") FText AwardsHeading;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Texts") FText NoAwardsText;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Texts") FText NoObjectivesText;
    UFUNCTION(BlueprintImplementableEvent, Category="Movie|UI") void OnPresentationChanged(const FMoviePresentationView& Data);
protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
private:
    void BuildLayout();
    void BuildResultRows();
    UTextBlock* Text(UVerticalBox* Parent, const FSlateFontInfo& Font);
    UButton* Button(UHorizontalBox* Parent, const FText& Label);
    bool CanUseResults() const;
    UFUNCTION() void HandleReplay();
    UFUNCTION() void HandleNewMovie();
    UFUNCTION() void HandleMainMenu();
    UPROPERTY(Transient) TWeakObjectPtr<AMovieManager> Movie;
    UPROPERTY(Transient) TObjectPtr<UBorder> TitleBackdrop;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> TitleColumn;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> TaglineLabel;
    UPROPERTY(Transient) TObjectPtr<UBorder> TopBar;
    UPROPERTY(Transient) TObjectPtr<UBorder> BottomBar;
    UPROPERTY(Transient) TObjectPtr<UBorder> ResultsBackdrop;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> ResultsColumn;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ResultsTitle;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ScoreLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RatingLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ReviewLabel;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> RoleColumns;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> AwardRows;
    UPROPERTY(Transient) TArray<TObjectPtr<UImage>> RatingStars;
    int32 LastStars=INDEX_NONE;
    int32 BoundPresentationId = 0;
    bool bResultRowsBuilt = false;
};
