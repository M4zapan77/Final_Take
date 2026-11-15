#include "WardrobeWidget.h"
#include "MovieManager.h"
#include "WardrobeComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "InputCoreTypes.h"

void UWardrobeChoiceButton::InitializeChoice(UWardrobeWidget* InOwner)
{
    WardrobeOwner = InOwner;
    OnClicked.AddUniqueDynamic(this, &UWardrobeChoiceButton::HandleClick);
}

void UWardrobeChoiceButton::HandleClick()
{
    if (UWardrobeWidget* Owner = WardrobeOwner.Get())
    {
        if (bColorChoice) { Owner->ChooseBodyColor(bUseColor, Color); }
        else { Owner->ChooseItem(Category, CosmeticId); }
    }
}

UWardrobeWidget::UWardrobeWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    SetIsFocusable(true);
}

void UWardrobeWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    // A designer-authored Blueprint root is deliberately left intact.
    if (!WidgetTree) { WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree")); }
    if (!WidgetTree->RootWidget) { BuildNativeLayout(); }
}

UTextBlock* UWardrobeWidget::MakeText(const FText& Text, int32 Size)
{
    UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>();
    Block->SetText(Text);
    Block->SetColorAndOpacity(FSlateColor(TextColor));
    FSlateFontInfo Font = Block->GetFont();
    Font.Size = Size;
    Block->SetFont(Font);
    Block->SetAutoWrapText(true);
    return Block;
}

void UWardrobeWidget::BuildNativeLayout()
{
    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Root;
    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
    Panel->SetBrushColor(PanelColor);
    Panel->SetPadding(FMargin(20));
    UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
    PanelSlot->SetAnchors(FAnchors(1,0,1,1));
    PanelSlot->SetAlignment(FVector2D(1,0));
    PanelSlot->SetOffsets(FMargin(-24,24,PanelWidth,24));
    UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
    Panel->SetContent(Column);
    Header = MakeText(NSLOCTEXT("FinalTakeUI","WardrobeWidget.8d3cdaa374","WARDROBE"), FontSize + 8);
    Header->SetColorAndOpacity(FSlateColor(AccentColor));
    Column->AddChildToVerticalBox(Header)->SetPadding(FMargin(0,0,0,8));
    Role = MakeText(FText::GetEmpty(), FontSize + 4);
    Column->AddChildToVerticalBox(Role);
    Script = MakeText(FText::GetEmpty(), FontSize);
    Column->AddChildToVerticalBox(Script)->SetPadding(FMargin(0,8));
    Countdown = MakeText(FText::GetEmpty(), FontSize + 4);
    Column->AddChildToVerticalBox(Countdown);
    Progress = WidgetTree->ConstructWidget<UProgressBar>();
    Progress->SetFillColorAndOpacity(AccentColor);
    Column->AddChildToVerticalBox(Progress)->SetPadding(FMargin(0,8,0,12));
    UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    Column->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ChoicePanel = WidgetTree->ConstructWidget<UVerticalBox>();
    Scroll->AddChild(ChoicePanel);
    UTextBlock* Hint = MakeText(NSLOCTEXT("FinalTakeUI","WardrobeWidget.51f9bc1a45","Your character is the live preview. Choose an outfit, then get ready."), FontSize - 2);
    Column->AddChildToVerticalBox(Hint)->SetPadding(FMargin(0,10));
    ReadyButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ReadyButton"));
    if (bUseCustomButtonStyle) { ReadyButton->SetStyle(ButtonStyle); }
    ReadyButton->SetBackgroundColor(AccentColor);
    ReadyButton->SetContent(MakeText(NSLOCTEXT("FinalTakeUI","WardrobeWidget.65a95c6a68","READY - START TAKE"), FontSize + 2));
    ReadyButton->OnClicked.AddUniqueDynamic(this, &UWardrobeWidget::HandleReadyClicked);
    Column->AddChildToVerticalBox(ReadyButton)->SetPadding(FMargin(0,8,0,0));
}

void UWardrobeWidget::OpenWardrobe(AMovieManager* Manager, int32 InPreparationId)
{
    // Also permits isolated widget-tree tests and explicit creation before a LocalPlayer is assigned.
    if (!WidgetTree) { WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree")); }
    if (!WidgetTree->RootWidget) { BuildNativeLayout(); }
    MovieManager = Manager;
    PreparationId = InPreparationId;
    BuildChoices();
    RefreshWardrobe();
    OnWardrobeOpened();
}

TArray<FCosmeticDefinition> UWardrobeWidget::GetOptions(ECosmeticCategory Category) const
{
    TArray<FCosmeticDefinition> Options;
    const AMovieManager* Manager = MovieManager.Get();
    if (Manager && Manager->LiveWardrobeComponent)
    {
        Options=Manager->LiveWardrobeComponent->GetCategoryOptions(Category);
    }
    return Options;
}

void UWardrobeWidget::BuildChoices()
{
    if (!ChoicePanel) { return; }
    ChoicePanel->ClearChildren();
    ChoiceButtons.Reset();
    const ECosmeticCategory WidgetCategories[] = {ECosmeticCategory::Head, ECosmeticCategory::Body, ECosmeticCategory::Face, ECosmeticCategory::Prop};
    const FText Labels[] = {NSLOCTEXT("FinalTakeUI","WardrobeWidget.Labels.7138a51661","HEAD"), NSLOCTEXT("FinalTakeUI","WardrobeWidget.Labels.c4c51eeb13","BODY"), NSLOCTEXT("FinalTakeUI","WardrobeWidget.Labels.acc6652d65","ACCESSORY"), NSLOCTEXT("FinalTakeUI","WardrobeWidget.Labels.d4dba65759","PROP")};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(WidgetCategories); ++Index)
    {
        const ECosmeticCategory Category = WidgetCategories[Index];
        ChoicePanel->AddChildToVerticalBox(MakeText(Labels[Index], FontSize))->SetPadding(FMargin(0,12,0,5));
        UWrapBox* Row = WidgetTree->ConstructWidget<UWrapBox>();
        Row->SetExplicitWrapSize(true);
        Row->SetWrapSize(PanelWidth - 60.0f);
        Row->SetInnerSlotPadding(FVector2D(5,5));
        ChoicePanel->AddChildToVerticalBox(Row);
        TArray<FCosmeticDefinition> Options = GetOptions(Category);
        if (Category != ECosmeticCategory::Body)
        {
            FCosmeticDefinition None;
            None.Category = Category;
            None.DisplayName = NSLOCTEXT("FinalTakeUI","WardrobeWidget.6eef664840","None");
            Options.Insert(None, 0);
        }
        for (const FCosmeticDefinition& Option : Options)
        {
            UWardrobeChoiceButton* Button = WidgetTree->ConstructWidget<UWardrobeChoiceButton>();
            Button->Category = Category;
            Button->CosmeticId = Option.Id;
            Button->InitializeChoice(this);
            if (bUseCustomButtonStyle) { Button->SetStyle(ButtonStyle); }
            UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>();
            if (Option.Icon)
            {
                UImage* Icon = WidgetTree->ConstructWidget<UImage>();
                Icon->SetBrushFromTexture(Option.Icon);
                Icon->SetDesiredSizeOverride(FVector2D(24));
                Content->AddChild(Icon);
            }
            UBorder* ItemPadding = WidgetTree->ConstructWidget<UBorder>();
            ItemPadding->SetBrushColor(FLinearColor::Transparent);
            ItemPadding->SetPadding(FMargin(8,6));
            ItemPadding->SetContent(MakeText(Option.DisplayName.IsEmpty() ? FText::FromName(Option.Id) : Option.DisplayName, FontSize - 2));
            Content->AddChild(ItemPadding);
            Button->SetContent(Content);
            Row->AddChildToWrapBox(Button);
            ChoiceButtons.Add(Button);
        }
    }
    ChoicePanel->AddChildToVerticalBox(MakeText(NSLOCTEXT("FinalTakeUI","WardrobeWidget.eefb9319ca","BODY COLOR"), FontSize))->SetPadding(FMargin(0,12,0,5));
    UWrapBox* Colors = WidgetTree->ConstructWidget<UWrapBox>();
    Colors->SetInnerSlotPadding(FVector2D(5,5));
    ChoicePanel->AddChildToVerticalBox(Colors);
    const FLinearColor Presets[] = {FLinearColor::White, FLinearColor(.8,.05,.05), FLinearColor(.04,.25,.8), FLinearColor(1,.75,.05)};
    const FText Names[] = {NSLOCTEXT("FinalTakeUI","WardrobeWidget.Names.c0a8060f3b","Original"), NSLOCTEXT("FinalTakeUI","WardrobeWidget.Names.cc9db3a795","Red"), NSLOCTEXT("FinalTakeUI","WardrobeWidget.Names.7d44bc449c","Blue"), NSLOCTEXT("FinalTakeUI","WardrobeWidget.Names.3526f607bc","Yellow")};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Presets); ++Index)
    {
        UWardrobeChoiceButton* Button = WidgetTree->ConstructWidget<UWardrobeChoiceButton>();
        Button->bColorChoice = true;
        Button->bUseColor = Index != 0;
        Button->Color = Presets[Index];
        Button->InitializeChoice(this);
        if (bUseCustomButtonStyle) { Button->SetStyle(ButtonStyle); }
        Button->SetContent(MakeText(Names[Index], FontSize - 2));
        Colors->AddChildToWrapBox(Button);
        ChoiceButtons.Add(Button);
    }
}

bool UWardrobeWidget::Ready()
{
    AMovieManager* Manager = MovieManager.Get();
    return Manager && Manager->ReadyForTake(PreparationId);
}

void UWardrobeWidget::HandleReadyClicked() { Ready(); }

bool UWardrobeWidget::ChooseItem(ECosmeticCategory Category, FName Id)
{
    AMovieManager* Manager = MovieManager.Get();
    const bool bAccepted = Manager && Manager->SelectWardrobeItem(PreparationId, Category, Id);
    RefreshWardrobe();
    return bAccepted;
}

bool UWardrobeWidget::ChooseBodyColor(bool bUseColor, FLinearColor Color)
{
    AMovieManager* Manager = MovieManager.Get();
    const bool bAccepted = Manager && Manager->SetWardrobeBodyColor(PreparationId, bUseColor, Color);
    RefreshWardrobe();
    return bAccepted;
}

void UWardrobeWidget::RefreshWardrobe()
{
    AMovieManager* Manager = MovieManager.Get();
    if (!Manager) { return; }
    ViewData.TakeNumber = Manager->GetCurrentTakeNumber();
    ViewData.TakeCount = Manager->GetNumberOfTakes();
    ViewData.RoleId = Manager->GetCurrentRoleId();
    ViewData.RoleName = Manager->GetCurrentRoleName();
    ViewData.ScriptInstruction = Manager->GetCurrentScriptInstruction();
    ViewData.RemainingSeconds = Manager->GetPhaseRemainingSeconds();
    ViewData.DurationSeconds = Manager->GetPhaseDurationSeconds();
    ViewData.bCanChoose = Manager->IsWardrobeOpen() && PreparationId == Manager->PreparationId;
    if (Manager->LiveWardrobeComponent) { ViewData.Selection = Manager->LiveWardrobeComponent->GetSelection(); }
    if (Header) { Header->SetText(FText::Format(NSLOCTEXT("WardrobeUI","HeaderTurn","WARDROBE - TURN {0} / {1}"),FText::AsNumber(ViewData.TakeNumber),FText::AsNumber(ViewData.TakeCount))); }
    if (Role) { Role->SetText(ViewData.RoleName); }
    if (Script) { Script->SetText(ViewData.ScriptInstruction); }
    if (Countdown) { Countdown->SetText(FText::Format(NSLOCTEXT("WardrobeUI","ReadyCountdown","{0} seconds to get ready"),FText::AsNumber(FMath::CeilToInt(ViewData.RemainingSeconds)))); }
    if (Progress) { Progress->SetPercent(ViewData.DurationSeconds > 0.0 ? ViewData.RemainingSeconds / ViewData.DurationSeconds : 0.0); }
    if (ReadyButton) { ReadyButton->SetIsEnabled(ViewData.bCanChoose); }
    for (UWardrobeChoiceButton* Button : ChoiceButtons)
    {
        const bool bSelected = Button->bColorChoice
            ? Button->bUseColor == ViewData.Selection.bUseBodyColor && (!Button->bUseColor || Button->Color.Equals(ViewData.Selection.BodyColor))
            : Button->CosmeticId == ViewData.Selection.GetId(Button->Category);
        Button->SetIsEnabled(ViewData.bCanChoose);
        Button->SetBackgroundColor(bSelected ? AccentColor : FLinearColor(.16f,.18f,.22f));
    }
    OnWardrobeViewChanged(ViewData);
}

void UWardrobeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    RefreshWardrobe();
}

FReply UWardrobeWidget::NativeOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Enter) { Ready(); return FReply::Handled(); }
    if (AMovieManager* Manager = MovieManager.Get(); Manager && Manager->bBindRestartKey && InKeyEvent.GetKey() == EKeys::R)
    {
        Manager->RestartMovie();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(MyGeometry, InKeyEvent);
}
