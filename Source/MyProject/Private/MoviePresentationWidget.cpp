#include "MoviePresentationWidget.h"
#include "StudioUITheme.h"
#include "StudioFlowManager.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"

#include "MovieManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"

UMoviePresentationWidget::UMoviePresentationWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    SetIsFocusable(true);
    TitleFont=FCoreStyle::GetDefaultFontStyle(TEXT("Bold"),32);
    ScoreFont=FCoreStyle::GetDefaultFontStyle(TEXT("Bold"),24);
    BodyFont=FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),17);
    ReplayButtonText=NSLOCTEXT("MovieResults","ReplayButton","Replay Movie");
    NewMovieButtonText=NSLOCTEXT("MovieResults","NewButton","Record New Movie");
    MainMenuButtonText=NSLOCTEXT("FinalUI","BackToHub","Back to Studio");
    AwardsHeading=NSLOCTEXT("MovieResults","Awards","THE EXTREMELY PRESTIGIOUS AWARDS");
    NoAwardsText=NSLOCTEXT("MovieResults","NoAwards","The jury is still recovering.");
    NoObjectivesText=NSLOCTEXT("MovieResults","NoObjectives","No objectives completed. A bold artistic choice.");
}

void UMoviePresentationWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if(!UITheme){UITheme=UStudioUITheme::LoadDefault();}
    if(UITheme){TitleFont=UITheme->Heading(28);ScoreFont=UITheme->Heading(24);AccentColor=UITheme->Gold;ResultPanelColor=UITheme->Navy;ResultTextColor=UITheme->Cream;}
    if (!WidgetTree) { WidgetTree=NewObject<UWidgetTree>(this,TEXT("WidgetTree")); }
    if (!WidgetTree->RootWidget) { BuildLayout(); }
}

UTextBlock* UMoviePresentationWidget::Text(UVerticalBox* Parent, const FSlateFontInfo& Font)
{
    auto* Label=WidgetTree->ConstructWidget<UTextBlock>(); Label->SetFont(Font);
    Label->SetColorAndOpacity(FSlateColor(ResultTextColor)); Label->SetAutoWrapText(true);
    Parent->AddChildToVerticalBox(Label)->SetPadding(FMargin(0,0,0,8)); return Label;
}

UButton* UMoviePresentationWidget::Button(UHorizontalBox* Parent, const FText& Label)
{
    auto* Control=WidgetTree->ConstructWidget<UStudioArtButton>(); Control->SetBackgroundColor(AccentColor);
    auto* Caption=WidgetTree->ConstructWidget<UTextBlock>(); Caption->SetText(Label); Caption->SetFont(BodyFont);
    Caption->SetColorAndOpacity(FSlateColor::UseForeground()); Control->SetContent(Caption);
    if(UITheme){UITheme->StyleButton(Control,Label.EqualTo(ReplayButtonText)?TEXT("Primary"):TEXT("Secondary"));Caption->SetFont(UITheme->Heading(17));}
    auto* ControlSlot=Parent->AddChildToHorizontalBox(Control); ControlSlot->SetPadding(FMargin(6));
    ControlSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); return Control;
}

void UMoviePresentationWidget::BuildLayout()
{
    auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget=Canvas;
    const auto Full=[Canvas](UWidget* Widget)
    {
        auto* CanvasSlot=Canvas->AddChildToCanvas(Widget); CanvasSlot->SetAnchors(FAnchors(0,0,1,1)); CanvasSlot->SetOffsets(FMargin(0)); return CanvasSlot;
    };
    TitleBackdrop=WidgetTree->ConstructWidget<UBorder>(); TitleBackdrop->SetBrushColor(TitleBackgroundImage?FLinearColor::White:FLinearColor::Black);
    if (TitleBackgroundImage) { TitleBackdrop->SetBrushFromTexture(TitleBackgroundImage); }
    Full(TitleBackdrop);
    TitleColumn=WidgetTree->ConstructWidget<UVerticalBox>();
    auto* TitleSlot=Canvas->AddChildToCanvas(TitleColumn); TitleSlot->SetAnchors(FAnchors(.5f)); TitleSlot->SetAlignment(FVector2D(.5f,.5f)); TitleSlot->SetOffsets(FMargin(0,0,1050,180));
    TitleLabel=Text(TitleColumn,TitleFont); TitleLabel->SetJustification(ETextJustify::Center); TitleLabel->SetColorAndOpacity(FSlateColor(AccentColor));
    TaglineLabel=Text(TitleColumn,ScoreFont); TaglineLabel->SetJustification(ETextJustify::Center);
    TopBar=WidgetTree->ConstructWidget<UBorder>(); TopBar->SetBrushColor(LetterboxColor); Full(TopBar);
    BottomBar=WidgetTree->ConstructWidget<UBorder>(); BottomBar->SetBrushColor(LetterboxColor); Full(BottomBar);
    ResultsBackdrop=WidgetTree->ConstructWidget<UBorder>(); ResultsBackdrop->SetBrushColor(ResultPanelColor);
    if (ResultsBackgroundImage) { ResultsBackdrop->SetBrushFromTexture(ResultsBackgroundImage); }
    Full(ResultsBackdrop);
    ResultsColumn=WidgetTree->ConstructWidget<UVerticalBox>();
    auto* ContentSlot=Full(ResultsColumn); ContentSlot->SetAnchors(FAnchors(.05f,.05f,.95f,.95f));
    auto* ResultHeader=WidgetTree->ConstructWidget<UHorizontalBox>();ResultsColumn->AddChildToVerticalBox(ResultHeader)->SetPadding(FMargin(0,0,0,6));
    if(UITheme)
    {
        auto* Logo=WidgetTree->ConstructWidget<UImage>();Logo->SetBrushFromTexture(UITheme->Texture(TEXT("Logo")),true);
        auto* LogoScale=WidgetTree->ConstructWidget<UScaleBox>();LogoScale->SetStretch(EStretch::ScaleToFit);LogoScale->SetContent(Logo);
        auto* LogoSize=WidgetTree->ConstructWidget<USizeBox>();LogoSize->SetWidthOverride(225);LogoSize->SetHeightOverride(124);LogoSize->SetContent(LogoScale);ResultHeader->AddChildToHorizontalBox(LogoSize)->SetPadding(FMargin(0,0,24,0));
    }
    auto* HeaderText=WidgetTree->ConstructWidget<UVerticalBox>();ResultHeader->AddChildToHorizontalBox(HeaderText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ResultsTitle=Text(HeaderText,TitleFont);ResultsTitle->SetColorAndOpacity(AccentColor);
    if(UITheme)
    {
        auto* Stars=WidgetTree->ConstructWidget<UHorizontalBox>();HeaderText->AddChildToVerticalBox(Stars);
        for(int32 Index=0;Index<5;++Index)
        {
            auto* Star=WidgetTree->ConstructWidget<UImage>();Star->SetBrushFromTexture(UITheme->Texture(TEXT("Star")));RatingStars.Add(Star);
            auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(35);Size->SetHeightOverride(35);Size->SetContent(Star);Stars->AddChildToHorizontalBox(Size)->SetPadding(FMargin(0,0,5,0));
        }
    }
    ScoreLabel=Text(ResultsColumn,ScoreFont); RatingLabel=Text(ResultsColumn,ScoreFont); ReviewLabel=Text(ResultsColumn,BodyFont);
    auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();
    auto* ScrollSlot=ResultsColumn->AddChildToVerticalBox(Scroll); ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Contents=WidgetTree->ConstructWidget<UVerticalBox>(); Scroll->AddChild(Contents);
    RoleColumns=WidgetTree->ConstructWidget<UHorizontalBox>(); Contents->AddChildToVerticalBox(RoleColumns)->SetPadding(FMargin(0,8));
    Text(Contents,ScoreFont)->SetText(AwardsHeading);
    AwardRows=WidgetTree->ConstructWidget<UVerticalBox>(); Contents->AddChildToVerticalBox(AwardRows);
    auto* Controls=WidgetTree->ConstructWidget<UHorizontalBox>(); ResultsColumn->AddChildToVerticalBox(Controls)->SetPadding(FMargin(0,12,0,0));
    ReplayButton=Button(Controls,ReplayButtonText); NewMovieButton=Button(Controls,NewMovieButtonText); MainMenuButton=Button(Controls,MainMenuButtonText);
    ReplayButton->OnClicked.AddDynamic(this,&UMoviePresentationWidget::HandleReplay);
    NewMovieButton->OnClicked.AddDynamic(this,&UMoviePresentationWidget::HandleNewMovie);
    MainMenuButton->OnClicked.AddDynamic(this,&UMoviePresentationWidget::HandleMainMenu);
}

void UMoviePresentationWidget::BindMovie(AMovieManager* Manager, int32 ExpectedPresentationId)
{
    Movie=Manager; BoundPresentationId=ExpectedPresentationId; bResultRowsBuilt=false;
    if (!WidgetTree) { WidgetTree=NewObject<UWidgetTree>(this,TEXT("WidgetTree")); }
    if (!WidgetTree->RootWidget) { BuildLayout(); }
    RefreshPresentation();
}

void UMoviePresentationWidget::BuildResultRows()
{
    if (!RoleColumns || !AwardRows) { return; }
    RoleColumns->ClearChildren(); AwardRows->ClearChildren();
    for (const auto& RoleResult : ViewData.Results.Roles)
    {
        auto* Column=WidgetTree->ConstructWidget<UVerticalBox>();
        auto* ColumnSlot=RoleColumns->AddChildToHorizontalBox(Column); ColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); ColumnSlot->SetPadding(FMargin(0,0,20,8));
        Text(Column,ScoreFont)->SetText(FText::Format(NSLOCTEXT("MovieResults","RoleScore","{0}: {1}/100"),RoleResult.RoleName,FText::AsNumber(RoleResult.Score)));
        Text(Column,BodyFont)->SetText(FText::Format(NSLOCTEXT("MovieResults","CompletedCount","Objectives: {0}/{1}"),FText::AsNumber(RoleResult.CompletedObjectiveIds.Num()),FText::AsNumber(RoleResult.ObjectiveCount)));
        if (RoleResult.CompletedObjectiveNames.IsEmpty()) { Text(Column,BodyFont)->SetText(NoObjectivesText); }
        for (const FText& Name : RoleResult.CompletedObjectiveNames) { Text(Column,BodyFont)->SetText(FText::Format(NSLOCTEXT("MovieResults","CompletedName","+ {0}"),Name)); }
    }
    if (ViewData.Results.Awards.IsEmpty()) { Text(AwardRows,BodyFont)->SetText(NoAwardsText); }
    for (const auto& Award : ViewData.Results.Awards)
    {
        TArray<FText> Winners;
        for (FName Id : Award.WinnerRoleIds)
        {
            const auto* Winner=ViewData.Results.Roles.FindByPredicate([Id](const FMovieRoleResult& RoleResult) { return RoleResult.RoleId==Id; });
            Winners.Add(Winner?Winner->RoleName:FText::FromName(Id));
        }
        auto* Heading=Text(AwardRows,BodyFont); Heading->SetColorAndOpacity(FSlateColor(AccentColor));
        Heading->SetText(FText::Format(NSLOCTEXT("MovieResults","AwardWinners","{0} - {1}"),Award.Name,FText::Join(FText::FromString(TEXT(", ")),Winners)));
        Text(AwardRows,BodyFont)->SetText(Award.Description);
    }
}

void UMoviePresentationWidget::RefreshPresentation()
{
    AMovieManager* Manager=Movie.Get();
    if (!Manager || Manager->PresentationId!=BoundPresentationId) { return; }
    ViewData=Manager->GetPresentationView();
    const bool bTitle=ViewData.Phase==EMoviePresentationPhase::TitleCard;
    const bool bResults=ViewData.Phase==EMoviePresentationPhase::Results;
    const bool bBars=ViewData.Phase==EMoviePresentationPhase::Playback && ViewData.bLetterbox;
    SetVisibility(bResults?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::HitTestInvisible);
    if (TitleBackdrop) { TitleBackdrop->SetVisibility(bTitle?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed); TitleBackdrop->SetRenderOpacity(ViewData.TitleBackgroundOpacity); }
    if (TitleColumn) { TitleColumn->SetVisibility(bTitle?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed); TitleColumn->SetRenderOpacity(ViewData.TitleTextOpacity); }
    if (TitleLabel) { TitleLabel->SetText(ViewData.Results.MovieTitle); }
    if (TaglineLabel) { TaglineLabel->SetText(ViewData.Results.Tagline); }
    if (TopBar)
    {
        TopBar->SetVisibility(bBars?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
        auto* BarSlot=CastChecked<UCanvasPanelSlot>(TopBar->Slot); BarSlot->SetAnchors(FAnchors(0,0,1,ViewData.LetterboxHeightFraction)); BarSlot->SetOffsets(FMargin(0));
        BottomBar->SetVisibility(TopBar->GetVisibility());
        auto* LowerSlot=CastChecked<UCanvasPanelSlot>(BottomBar->Slot); LowerSlot->SetAnchors(FAnchors(0,1-ViewData.LetterboxHeightFraction,1,1)); LowerSlot->SetOffsets(FMargin(0));
    }
    if (ResultsBackdrop) { ResultsBackdrop->SetVisibility(bResults?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed); ResultsBackdrop->SetRenderOpacity(ViewData.ResultsOpacity); }
    if (ResultsColumn) { ResultsColumn->SetVisibility(bResults?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed); ResultsColumn->SetRenderOpacity(ViewData.ResultsOpacity); }
    if (ResultsTitle) { ResultsTitle->SetText(ViewData.Results.MovieTitle); }
    if (ScoreLabel) { FNumberFormattingOptions Format;Format.MinimumFractionalDigits=2;Format.MaximumFractionalDigits=2;ScoreLabel->SetText(FText::Format(NSLOCTEXT("MovieResults","FinalScore","{0} / 100"),FText::AsNumber(ViewData.Results.FinalScore,&Format))); }
    if (RatingLabel)
    {
        const FString Stars=FString::ChrN(ViewData.Results.Stars,TCHAR(0x2605))+FString::ChrN(5-ViewData.Results.Stars,TCHAR(0x2606));
        RatingLabel->SetText(RatingStars.IsEmpty()?FText::Format(NSLOCTEXT("MovieResults","Rating","{0}  {1}"),FText::FromString(Stars),ViewData.Results.RatingName):ViewData.Results.RatingName);
        if(LastStars!=ViewData.Results.Stars){for(int32 I=0;I<RatingStars.Num();++I){RatingStars[I]->SetRenderOpacity(I<ViewData.Results.Stars?1.f:.18f);}LastStars=ViewData.Results.Stars;}
    }
    if (ReviewLabel) { ReviewLabel->SetText(ViewData.Results.AudienceReview); }
    if(ReviewLabel && !ViewData.Results.StoryEnding.IsEmpty())
    {
        const auto& S=ViewData.Results.StoryScore;
        ReviewLabel->SetText(FText::Format(NSLOCTEXT("FinalTake","StoryResultSummary","{0}\nStory {1}/40 · Interactions {2}/30 · Comedy {3}/20 · Props/NPCs {4}/10\n{5}"),
            ViewData.Results.StoryEnding,FText::AsNumber(S.Story),FText::AsNumber(S.Interactions),FText::AsNumber(S.Creativity),FText::AsNumber(S.PropsAndNPCs),ViewData.Results.AudienceReview));
    }
    if (ViewData.Results.bAvailable && !bResultRowsBuilt) { BuildResultRows(); bResultRowsBuilt=true; }
    if (ReplayButton) { ReplayButton->SetIsEnabled(ViewData.bCanReplay); NewMovieButton->SetIsEnabled(ViewData.bCanRecordNew); MainMenuButton->SetVisibility(ViewData.bCanReturnToMenu?ESlateVisibility::Visible:ESlateVisibility::Collapsed); }
    OnPresentationChanged(ViewData);
}

bool UMoviePresentationWidget::CanUseResults() const
{
    return Movie.IsValid() && Movie->PresentationId==BoundPresentationId && Movie->IsResultsOpen();
}
bool UMoviePresentationWidget::ReplayMovie()
{
    AMovieManager* Manager=Movie.Get(); if (!CanUseResults() || !Manager->ReplayMovie()) { return false; }
    Manager->PlayPresentationClick(); return true;
}
bool UMoviePresentationWidget::RecordNewMovie()
{
    AMovieManager* Manager=Movie.Get(); if (!CanUseResults() || !Manager->RecordNewMovie()) { return false; }
    Manager->PlayPresentationClick(); return true;
}
bool UMoviePresentationWidget::ReturnToMainMenu()
{
    if(!CanUseResults()){return false;}
    if(auto* Flow=AStudioFlowManager::Find(this)){return Flow->ReturnToHub();}
    return Movie->ReturnToMainMenu();
}
void UMoviePresentationWidget::HandleReplay() { ReplayMovie(); }
void UMoviePresentationWidget::HandleNewMovie() { RecordNewMovie(); }
void UMoviePresentationWidget::HandleMainMenu() { ReturnToMainMenu(); }
void UMoviePresentationWidget::FocusResults() { if (ReplayButton) { ReplayButton->SetKeyboardFocus(); } else { SetKeyboardFocus(); } }
FReply UMoviePresentationWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
    if (!KeyEvent.IsRepeat() && KeyEvent.GetKey()==EKeys::R && RecordNewMovie()) { return FReply::Handled(); }
    if (!KeyEvent.IsRepeat() && KeyEvent.GetKey()==EKeys::Escape && ReturnToMainMenu()) { return FReply::Handled(); }
    return Super::NativeOnKeyDown(Geometry,KeyEvent);
}
