#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ObjectiveTypes.h"
#include "ObjectiveHUDWidget.generated.h"
class AMovieManager;
class UBorder;
class UTextBlock;
class UVerticalBox;

USTRUCT(BlueprintType)
struct MYPROJECT_API FObjectiveHUDView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Objectives") FText RoleName;
    UPROPERTY(BlueprintReadOnly, Category="Objectives") int32 TakeNumber = 0;
    UPROPERTY(BlueprintReadOnly, Category="Objectives") int32 RoundNumber = 1;
    UPROPERTY(BlueprintReadOnly, Category="Objectives") int32 TurnCount = 3;
    UPROPERTY(BlueprintReadOnly, Category="Objectives") double RemainingSeconds = 0;
    UPROPERTY(BlueprintReadOnly, Category="Objectives") bool bPreparation = false;
    UPROPERTY(BlueprintReadOnly, Category="Objectives") int32 Score = 0;
    UPROPERTY(BlueprintReadOnly, Category="Objectives") TArray<FMovieObjectiveStatus> Objectives;
    UPROPERTY(BlueprintReadOnly, Category="Objectives") FText CompletionFeedback;
    UPROPERTY(BlueprintReadOnly, Category="Objectives") FText InteractionPrompt;
    UPROPERTY(BlueprintReadOnly, Category="Objectives") FMovieScoreSummary MovieScore;
    UPROPERTY(BlueprintReadOnly, Category="Objectives") bool bRecording = false;
    UPROPERTY(BlueprintReadOnly, Category="Objectives") FName ActiveGesture;
    UPROPERTY(BlueprintReadOnly, Category="Objectives") FText Controls;
};

/** Separate read-only HUD; does not change the wardrobe layout or gameplay input mode. */
UCLASS(Blueprintable)
class MYPROJECT_API UObjectiveHUDWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void BindMovie(AMovieManager* Manager);
    UFUNCTION(BlueprintCallable, Category="Objectives") void RefreshObjectives();
    UPROPERTY(BlueprintReadOnly, Transient, Category="Objectives") FObjectiveHUDView ViewData;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") FLinearColor PanelColor = FLinearColor(.025f,.03f,.05f,.9f);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") FLinearColor CompleteColor = FLinearColor(.4f,1.0f,.5f);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") float PanelWidth = 360;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") bool bShowControlReference = true;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style", meta=(MultiLine=true)) FText ControlReference = NSLOCTEXT("MovieHUD","Controls","WASD Move | Mouse Look | Space Jump\nE Pick up / use prop | R Restart movie\nGESTURES\n1 Dance | 2 Point | 3 Scared\n4 Faint | 5 Arrest\nSOUNDS\nZ Evil laugh | X Scream | C Cry\nV Siren | B Kiss | N Failure");
    UFUNCTION(BlueprintImplementableEvent, Category="Objectives") void OnObjectiveViewChanged(const FObjectiveHUDView& Data);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float Delta) override;
private:
    void BuildLayout();
    UTextBlock* AddText(UVerticalBox* Parent, int32 Size);
    UPROPERTY(Transient) TWeakObjectPtr<AMovieManager> Movie;
    UPROPERTY(Transient) TObjectPtr<UBorder> Panel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Title;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Timer;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Score;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Rows;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> RowLabels;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Feedback;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Prompt;
    UPROPERTY(Transient) TObjectPtr<UBorder> SummaryPanel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> SummaryText;
    UPROPERTY(Transient) TObjectPtr<UBorder> ControlsPanel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ControlsText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> GestureText;
};
