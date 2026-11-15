#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "StudioFlowTypes.h"
#include "StudioFlowWidget.generated.h"

class AStudioFlowManager;
class UBorder;
class UButton;
class UHorizontalBox;
class USlider;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UScrollBox;
class UStudioUITheme;

/** Native fallback. A UMG child with a Designer root keeps that root and consumes ViewData/events. */
UCLASS(Blueprintable)
class MYPROJECT_API UStudioFlowWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UStudioFlowWidget(const FObjectInitializer& Initializer);
    void BindFlow(AStudioFlowManager* Manager);
    UFUNCTION(BlueprintCallable, Category="Studio") void RefreshFlow();
    UFUNCTION(BlueprintCallable, Category="Studio") bool PlayMovie();
    UFUNCTION(BlueprintCallable, Category="Studio") bool BeginMovie();
    UFUNCTION(BlueprintCallable, Category="Studio") bool HowToPlay();
    UFUNCTION(BlueprintCallable, Category="Studio") bool Credits();
    UFUNCTION(BlueprintCallable, Category="Studio") bool AudioSettings();
    UFUNCTION(BlueprintCallable,Category="Studio|Language") bool SelectLanguage(const FString& Language);
    UFUNCTION(BlueprintCallable, Category="Studio") bool Back();
    UFUNCTION(BlueprintCallable, Category="Studio") bool Quit();
    UFUNCTION(BlueprintCallable, Category="Studio") void SetVolumes(float Master,float Music,float Effects);
    UFUNCTION(BlueprintPure, Category="Studio") AStudioFlowManager* GetFlowManager() const;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Studio") FStudioFlowView ViewData;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") FSlateFontInfo HeadingFont;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") FSlateFontInfo BodyFont;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") FLinearColor BackgroundColor = FLinearColor(.025f,.03f,.05f,.94f);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") FLinearColor TextColor = FLinearColor(.95f,.95f,1.f);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") FLinearColor AccentColor = FLinearColor(.9f,.62f,.2f);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") TObjectPtr<UTexture2D> BackgroundImage;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Style") TObjectPtr<UStudioUITheme> UITheme;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Text") FText MenuTagline;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Text", meta=(MultiLine=true)) FText IntroductionControls;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Text") FText PlayText;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Text") FText StartGameText;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Text") FText HowToPlayText;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Text") FText CreditsText;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Text") FText AudioText;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Text") FText QuitText;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Text") FText StartText;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Text") FText BackText;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Text") FText NextActorText;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Text") FText WrapText;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Text") FText MasterLabel;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Text") FText MusicLabel;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Text") FText EffectsLabel;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Text") FText AmbientLabel;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Text") FText UILabel;
    UFUNCTION(BlueprintImplementableEvent, Category="Studio") void OnStudioViewChanged(const FStudioFlowView& Data);

    UPROPERTY(BlueprintReadOnly, Transient, Category="Native Controls") TObjectPtr<UButton> PlayButton;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Native Controls") TObjectPtr<UButton> HowButton;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Native Controls") TObjectPtr<UButton> CreditsButton;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Native Controls") TObjectPtr<UButton> AudioButton;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Native Controls") TObjectPtr<UButton> QuitButton;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Native Controls") TObjectPtr<UButton> StartButton;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Native Controls") TObjectPtr<UButton> BackButton;
    UPROPERTY(BlueprintReadOnly,Transient,Category="Native Controls") TObjectPtr<UButton> HubSettingsButton;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Native Controls") TObjectPtr<USlider> MasterSlider;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Native Controls") TObjectPtr<USlider> MusicSlider;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Native Controls") TObjectPtr<USlider> EffectsSlider;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Native Controls") TObjectPtr<USlider> AmbientSlider;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Native Controls") TObjectPtr<USlider> UISlider;
    UPROPERTY(BlueprintReadOnly,Transient,Category="Native Controls") TObjectPtr<UButton> SpanishButton;
    UPROPERTY(BlueprintReadOnly,Transient,Category="Native Controls") TObjectPtr<UButton> EnglishButton;
    void FocusPage();
protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
private:
    void BuildLayout();
    UTextBlock* Text(UVerticalBox* Parent,const FSlateFontInfo& Font);
    UButton* Button(UHorizontalBox* Parent,const FText& Label);
    USlider* AudioRow(UVerticalBox* Parent,const FText& Label,TObjectPtr<UTextBlock>& ValueLabel);
    bool CanUse() const;
    UFUNCTION() void ClickPlay();
    UFUNCTION() void ClickHow();
    UFUNCTION() void ClickCredits();
    UFUNCTION() void ClickAudio();
    UFUNCTION() void ClickQuit();
    UFUNCTION() void ClickStart();
    UFUNCTION() void ClickBack();
    UFUNCTION() void ClickSpanish();
    UFUNCTION() void ClickEnglish();
    UFUNCTION() void MasterChanged(float Value);
    UFUNCTION() void MusicChanged(float Value);
    UFUNCTION() void EffectsChanged(float Value);
    UFUNCTION() void AmbientChanged(float Value);
    UFUNCTION() void UIChanged(float Value);
    UPROPERTY(Transient) TWeakObjectPtr<AStudioFlowManager> Flow;
    UPROPERTY(Transient) TObjectPtr<UBorder> Backdrop;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> PageColumn;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> MainMenuColumn;
    UPROPERTY(Transient) TObjectPtr<UBorder> PageFrame;
    UPROPERTY(Transient) TObjectPtr<UBorder> CoverPanel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CoverTitle;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Heading;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Detail;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Tutorial;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> RoleRows;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> AudioRows;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MasterValue;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MusicValue;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> EffectsValue;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> AmbientValue;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> UIValue;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> PageScroll;
    UPROPERTY(Transient) TObjectPtr<UBorder> DirectorPanel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> DirectorLabel;
    int32 LastRevision = INDEX_NONE;
    bool bUpdating = false;
};
