#include "ObjectiveHUDWidget.h"
#include "StudioUITheme.h"
#include "Components/SizeBox.h"
#include "WardrobeTypes.h"
#include "MovieManager.h"
#include "ObjectiveManagerComponent.h"
#include "PropInteractionComponent.h"
#include "StudioLanguageSubsystem.h"
#include "ActingComponent.h"
#include "StorySessionComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"

void UObjectiveHUDWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if(!UITheme){UITheme=UStudioUITheme::LoadDefault();}
    if(UITheme){PanelColor=UITheme->Navy;PanelColor.A=.94;}
    SetVisibility(ESlateVisibility::HitTestInvisible);
    if (!WidgetTree) { WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree")); }
    if (!WidgetTree->RootWidget) { BuildLayout(); }
}

UTextBlock* UObjectiveHUDWidget::AddText(UVerticalBox* Parent, int32 Size)
{
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
    auto Font = UITheme && Size>=22?UITheme->Heading(Size):Label->GetFont(); Font.Size = Size; Label->SetFont(Font);
    Label->SetColorAndOpacity(FSlateColor(FLinearColor::White)); Label->SetAutoWrapText(true);
    Parent->AddChildToVerticalBox(Label)->SetPadding(FMargin(0,0,0,8));
    return Label;
}

void UObjectiveHUDWidget::BuildLayout()
{
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget = Canvas;
    Panel = WidgetTree->ConstructWidget<UBorder>(); Panel->SetBrushColor(PanelColor); Panel->SetPadding(FMargin(16));
    auto* MainSize=WidgetTree->ConstructWidget<USizeBox>();MainSize->SetWidthOverride(PanelWidth);MainSize->SetContent(Panel);
    auto* PanelSlot=Canvas->AddChildToCanvas(MainSize);PanelSlot->SetOffsets(FMargin(20,20,0,0));PanelSlot->SetAutoSize(true);
    UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(); Panel->SetContent(Column);
    RecordingBadge=AddText(Column,15);RecordingBadge->SetText(NSLOCTEXT("FinalUI","RecBadge","● REC"));RecordingBadge->SetColorAndOpacity(FLinearColor(.95,.12,.13));
    Title=AddText(Column,19);Timer=AddText(Column,34);Score=AddText(Column,23);
    Rows = WidgetTree->ConstructWidget<UVerticalBox>(); Column->AddChildToVerticalBox(Rows);
    Feedback = AddText(Column,16); Feedback->SetColorAndOpacity(FSlateColor(CompleteColor));
    Prompt = AddText(Column,16);
    SummaryPanel = WidgetTree->ConstructWidget<UBorder>(); SummaryPanel->SetBrushColor(PanelColor); SummaryPanel->SetPadding(FMargin(12));
    auto* SummarySlot = Canvas->AddChildToCanvas(SummaryPanel);
    SummarySlot->SetAnchors(FAnchors(1,0)); SummarySlot->SetAlignment(FVector2D(1,0)); SummarySlot->SetOffsets(FMargin(-24,24,320,170));
    UVerticalBox* SummaryColumn = WidgetTree->ConstructWidget<UVerticalBox>(); SummaryPanel->SetContent(SummaryColumn);
    SummaryText = AddText(SummaryColumn,18);
    ControlsPanel=WidgetTree->ConstructWidget<UBorder>(); ControlsPanel->SetBrushColor(PanelColor); ControlsPanel->SetPadding(FMargin(14));
    auto* ControlsSlot=Canvas->AddChildToCanvas(ControlsPanel); ControlsSlot->SetAnchors(FAnchors(1,1)); ControlsSlot->SetAlignment(FVector2D(1,1)); ControlsSlot->SetOffsets(FMargin(-20,-20,350,190));
    auto* ControlsColumn=WidgetTree->ConstructWidget<UVerticalBox>(); ControlsPanel->SetContent(ControlsColumn);
    ControlsText=AddText(ControlsColumn,14); GestureText=AddText(ControlsColumn,14); GestureText->SetColorAndOpacity(FSlateColor(CompleteColor));
    HandsPanel=WidgetTree->ConstructWidget<UBorder>();HandsPanel->SetBrushColor(PanelColor);HandsPanel->SetPadding(FMargin(12));
    auto* HandsSize=WidgetTree->ConstructWidget<USizeBox>();HandsSize->SetWidthOverride(350);HandsSize->SetContent(HandsPanel);
    auto* HandsSlot=Canvas->AddChildToCanvas(HandsSize);HandsSlot->SetAnchors(FAnchors(0,1));HandsSlot->SetAlignment(FVector2D(0,1));HandsSlot->SetOffsets(FMargin(20,-20,0,0));HandsSlot->SetAutoSize(true);
    auto* HandsColumn=WidgetTree->ConstructWidget<UVerticalBox>();HandsPanel->SetContent(HandsColumn);HandsText=AddText(HandsColumn,15);
}

void UObjectiveHUDWidget::BindMovie(AMovieManager* Manager)
{
    Movie = Manager;
    if (!WidgetTree) { WidgetTree = NewObject<UWidgetTree>(this,TEXT("WidgetTree")); }
    if (!WidgetTree->RootWidget) { BuildLayout(); }
    SetVisibility(ESlateVisibility::HitTestInvisible);
    RefreshObjectives();
}

void UObjectiveHUDWidget::RefreshObjectives()
{
    const AMovieManager* Manager = Movie.Get();
    if (!Manager || !Manager->ObjectiveManager) { return; }
    const UObjectiveManagerComponent* Objectives = Manager->ObjectiveManager;
    ViewData.RoleName = Manager->GetCurrentRoleName(); ViewData.TakeNumber = Manager->GetCurrentTakeNumber();
    ViewData.RoundNumber=Manager->GetRoundNumber(); ViewData.TurnCount=Manager->GetNumberOfTakes();
    ViewData.bPreparation = Manager->State == EMovieState::Preparation;
    ViewData.RemainingSeconds = Manager->GetPhaseRemainingSeconds(); ViewData.Score = Objectives->CurrentScore;
    ViewData.bRecording=Manager->State==EMovieState::Recording;
    ViewData.ActiveGesture=Manager->LiveActingComponent?Manager->LiveActingComponent->ActiveGestureId:NAME_None; ViewData.Controls=ControlReference;
    ViewData.Objectives = Objectives->Objectives; ViewData.MovieScore = Manager->ScoreSummary;
    ViewData.CompletionFeedback = GetWorld() && GetWorld()->GetTimeSeconds() - Objectives->LastCompletionWorldTime < 2.0
        ? Objectives->LastCompletionFeedback : FText::GetEmpty();
    ViewData.InteractionPrompt = Manager->LivePropInteraction ? Manager->LivePropInteraction->GetInteractionPrompt() : FText::GetEmpty();
    if(Manager->IsChronologicalMovie())
    {
        ViewData.InteractionPrompt=Manager->StorySession->GetInteractionPrompt();
        if(ViewData.CompletionFeedback.IsEmpty()){ViewData.CompletionFeedback=Manager->StorySession->GetLatestConsequence();}
        ViewData.Controls=FText::Format(NSLOCTEXT("FinalUI","ActingControls","WASD Move · Mouse Look · Space Jump\n[{0}] Cut! · [E] Interact\n1 Dance · 2 Point · 3 Scared\n4 Faint · 5 Arrest\nZ Laugh · X Scream · C Cry\nV Siren · B Kiss · N Failure"),Manager->EndTurnKey.GetDisplayName());
        const auto* Story=Manager->StorySession.Get();const FName ActorId=Manager->GetCurrentRoleId();
        const FName Left=Story->GetHeldObject(ActorId,true),Right=Story->GetHeldObject(ActorId,false);
        const auto ObjectName=[Story](FName Id)
        {
            if(Id.IsNone()){return NSLOCTEXT("FinalUI","EmptyHand","Empty");}
            if(Id==Story->SceneState.MoneyObjectId){return NSLOCTEXT("FinalUI","MoneyBag","Money bag");}
            const auto* Object=Story->SceneState.FindObject(Id);
            return Object && !Object->CosmeticId.IsNone()?WardrobeDefaults::LocalizedName(Object->CosmeticId,NSLOCTEXT("FinalUI","HeldProp","Prop")):NSLOCTEXT("FinalUI","HeldProp","Prop");
        };
        ViewData.HeldObjects=FText::Format(NSLOCTEXT("FinalUI","HeldHands","LEFT: {0} · RIGHT: {1}"),ObjectName(Left),ObjectName(Right));
        if(!Left.IsNone() || !Right.IsNone())
        {ViewData.HeldObjects=FText::Format(NSLOCTEXT("FinalUI","HeldControls","{0}\n[{1}] Drop · [{2}] Throw"),ViewData.HeldObjects,Manager->DropObjectKey.GetDisplayName(),Manager->ThrowObjectKey.GetDisplayName());}
    }
    const bool bTake = Manager->State == EMovieState::Preparation || Manager->State == EMovieState::Recording;
    if (Panel) { Panel->SetVisibility(bTake ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
    if (Title)
    {
        Title->SetText(Manager->IsChronologicalMovie()
            ? FText::Format(NSLOCTEXT("FinalTake","TurnHUD","{0} — Round {1}/2\nTurn {2}/6"),ViewData.RoleName,FText::AsNumber(ViewData.RoundNumber),FText::AsNumber(ViewData.TakeNumber))
            : FText::Format(NSLOCTEXT("MovieHUD","LegacyTake","TAKE {0}/3 - {1}"),FText::AsNumber(ViewData.TakeNumber),ViewData.RoleName));
    }
    if (Timer)
    {
        Timer->SetText(ViewData.bPreparation ? NSLOCTEXT("MovieHUD","Challenges","Your acting challenges") : FText::Format(NSLOCTEXT("FinalUI","SecondsLeft","{0} s"),FText::AsNumber(FMath::CeilToInt(ViewData.RemainingSeconds))));
        Timer->SetColorAndOpacity(FSlateColor(ViewData.bRecording && ViewData.RemainingSeconds<=3?FLinearColor(1.f,.35f,.25f):FLinearColor::White));
    }
    if(RecordingBadge){RecordingBadge->SetVisibility(ViewData.bRecording?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);}
    if(HandsPanel){HandsPanel->SetVisibility(ViewData.bRecording && Manager->IsChronologicalMovie()?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);HandsText->SetText(ViewData.HeldObjects);}
    if (ControlsPanel) { ControlsPanel->SetVisibility(ViewData.bRecording && bShowControlReference?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed); }
    if (ControlsText) { ControlsText->SetText(ViewData.Controls); }
    if (GestureText) { GestureText->SetText(ViewData.ActiveGesture.IsNone()?FText::GetEmpty():FText::Format(NSLOCTEXT("MovieHUD","Gesture","Acting: {0}"),UStudioLanguageSubsystem::ActingEventName(ViewData.ActiveGesture))); }
    if (Score) { Score->SetText(FText::Format(NSLOCTEXT("MovieHUD","Score","{0} / 100"),FText::AsNumber(ViewData.Score))); }
    if(Score){Score->SetVisibility(Manager->IsChronologicalMovie()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);}
    if (Rows && RowLabels.Num() != ViewData.Objectives.Num())
    {
        Rows->ClearChildren(); RowLabels.Reset();
        for (int32 Index=0; Index<ViewData.Objectives.Num(); ++Index) { RowLabels.Add(AddText(Rows,16)); }
    }
    for (int32 Index=0; Index<RowLabels.Num(); ++Index)
    {
        const auto& Status = ViewData.Objectives[Index];
        const auto& Definition = Status.Definition;
        const FText Name = Definition.DisplayName.IsEmpty() ? FText::FromName(Definition.Id) : Definition.DisplayName;
        const bool bLocked = !Objectives->IsPrerequisiteSatisfied(Definition.Id);
        RowLabels[Index]->SetText(FText::Format(NSLOCTEXT("MovieHUD","ObjectiveRow","{0} {1} ({2})"),Status.bCompleted?NSLOCTEXT("FinalTake","CompleteCheck","[DONE]"):bLocked?NSLOCTEXT("MovieHUD","Locked","[LOCKED]"):FText::FromString(TEXT("[ ]")),Name,FText::AsNumber(Status.bCompleted?Status.PointsAwarded:Definition.Points)));
        if(Manager->IsChronologicalMovie())
        {RowLabels[Index]->SetText(FText::Format(NSLOCTEXT("FinalTake","PrimaryObjectiveRow","{0} {1}"),Status.bCompleted?NSLOCTEXT("FinalTake","CompleteCheck","[DONE]"):FText::FromString(TEXT("[ ]")),Definition.DisplayName));}
        RowLabels[Index]->SetColorAndOpacity(FSlateColor(Status.bCompleted ? CompleteColor : bLocked ? FLinearColor(.65f,.65f,.65f) : FLinearColor::White));
    }
    if (Feedback) { Feedback->SetText(Objectives->ConfigurationIssue.IsEmpty() ? ViewData.CompletionFeedback : Objectives->ConfigurationIssue); }
    if (Prompt) { Prompt->SetText(ViewData.InteractionPrompt); }
    const bool bSummary=false; // The cinematic and authoritative results have their own presentation; keep the movie unobstructed.
    if (SummaryPanel) { SummaryPanel->SetVisibility(bSummary ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
    if (SummaryText)
    {
        TArray<FText> Lines;Lines.Add(FText::Format(NSLOCTEXT("MovieHUD","MovieSummary","{0}\nMovie score: {1} / 100"),Manager->ResultsSettings.MovieTitle,FText::AsNumber(ViewData.MovieScore.FinalMovieScore)));
        for(const auto& Item:ViewData.MovieScore.RoleScores){const auto* Role=Manager->Roles.FindByPredicate([&Item](const auto& R){return R.RoleId==Item.RoleId;});Lines.Add(FText::Format(NSLOCTEXT("MovieResults","RoleScore","{0}: {1}/100"),Role?Role->DisplayName:FText::FromName(Item.RoleId),FText::AsNumber(Item.Score)));}
        SummaryText->SetText(FText::Join(FText::FromString(TEXT("\n")),Lines));
    }
    OnObjectiveViewChanged(ViewData);
}

void UObjectiveHUDWidget::NativeTick(const FGeometry& Geometry, float Delta)
{
    Super::NativeTick(Geometry,Delta);if(!Movie.IsValid() || Movie->ActiveObjectiveHUD!=this){RefreshObjectives();}
}
