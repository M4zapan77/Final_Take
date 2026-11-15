#include "UnifiedWardrobeWidget.h"
#include "StudioFlowManager.h"
#include "ActingComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MovieManager.h"
#include "ObjectiveManagerComponent.h"
#include "WardrobeComponent.h"
#include "WardrobePreviewActor.h"

void UCustomizationOptionButton::Bind(UUnifiedWardrobeWidget* Widget)
{
    OwnerWidget = Widget;
    OnClicked.AddUniqueDynamic(this, &UCustomizationOptionButton::Click);
}
void UCustomizationOptionButton::Click()
{
    if (OwnerWidget.IsValid())
    {
        OwnerWidget->Activate(this);
    }
}

UUnifiedWardrobeWidget::UUnifiedWardrobeWidget(const FObjectInitializer& Initializer) : Super(Initializer)
{
    PreviewActorClass = AWardrobePreviewActor::StaticClass();
    PreviewUIMaterial = LoadObject<UMaterialInterface>(
        nullptr, TEXT("/Game/OneManStudio/Characters/Base3/Customization/Materials/M_CharacterPreviewUI"), nullptr,
        LOAD_NoWarn | LOAD_Quiet);
    const TCHAR* Ids[] = {TEXT("White"), TEXT("Blue"), TEXT("Purple"), TEXT("Pink"), TEXT("Yellow"), TEXT("Green")};
    const FText Names[] = {NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.Names.37619fc130","White"), NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.Names.be96829112","Sky blue"), NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.Names.32576f4fed","Purple"),
                            NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.Names.54076359fd","Pink"),   NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.Names.3526f607bc","Yellow"),   NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.Names.933bf21afd","Green")};
    const FLinearColor Colors[] = {FLinearColor::White,      FLinearColor(.32, .65, 1), FLinearColor(.45, .22, .85),
                                   FLinearColor(1, .35, .6), FLinearColor(1, .78, .18), FLinearColor(.25, .8, .4)};
    for (int32 I = 0; I < 6; ++I)
    {
        auto& P = SkinPresets.AddDefaulted_GetRef();
        P.Id = Ids[I];
        P.Label = Names[I];
        P.Color = Colors[I];
    }
}
UBorder* UUnifiedWardrobeWidget::Card(FLinearColor Color, float Radius)
{
    auto* Border = WidgetTree->ConstructWidget<UBorder>();
    FSlateBrush Brush;
    Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
    Brush.TintColor = Color;
    Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
    Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    Border->SetBrush(Brush);
    Border->SetPadding(FMargin(16));
    return Border;
}
UTextBlock* UUnifiedWardrobeWidget::Label(const FText& Text, int32 Size, FLinearColor Color)
{
    auto* T = WidgetTree->ConstructWidget<UTextBlock>();
    auto Font = T->GetFont();
    if (HeadingFont.FontObject)
    {
        Font = HeadingFont;
    }
    Font.Size = Size;
    T->SetFont(Font);
    T->SetText(Text);
    T->SetColorAndOpacity(Color);
    T->SetAutoWrapText(true);
    return T;
}
UCustomizationOptionButton* UUnifiedWardrobeWidget::Button(const FText& Text, FName Id,
                                                           ECustomizationButtonAction Action)
{
    auto* B = WidgetTree->ConstructWidget<UCustomizationOptionButton>();
    B->ControlId = Id;
    B->Action = Action;
    B->Bind(this);
    FButtonStyle Style = B->GetStyle();
    const auto Brush = [&](FLinearColor C)
    {
        FSlateBrush R;
        R.DrawAs = ESlateBrushDrawType::RoundedBox;
        R.TintColor = C;
        R.OutlineSettings.CornerRadii = FVector4(10, 10, 10, 10);
        R.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        return R;
    };
    Style.SetNormal(Brush(FLinearColor::White));
    Style.SetHovered(Brush(FLinearColor(1, 1, 1, 1)));
    Style.SetPressed(Brush(Gold));
    Style.NormalPadding = FMargin(12, 9);
    Style.PressedPadding = FMargin(12, 9);
    B->SetStyle(bUseCustomButtonStyle ? ButtonStyle : Style);
    B->SetBackgroundColor(Cream);
    B->Caption = Label(Text, FontSize, Navy);
    B->Caption->SetJustification(ETextJustify::Center);
    B->SetContent(B->Caption);
    Controls.Add(B);
    return B;
}
void UUnifiedWardrobeWidget::BuildNativeLayout()
{
    auto* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Root;
    auto* Background = Card(Navy, 0);
    Background->SetPadding(FMargin(24));
    auto* BackgroundSlot = Root->AddChildToCanvas(Background);
    BackgroundSlot->SetAnchors(FAnchors(0, 0, 1, 1));
    BackgroundSlot->SetOffsets(FMargin(0));
    auto* Scale = WidgetTree->ConstructWidget<UScaleBox>();
    Scale->SetStretch(EStretch::ScaleToFit);
    Background->SetContent(Scale);
    auto* Size = WidgetTree->ConstructWidget<USizeBox>();
    Size->SetWidthOverride(CardSize.X);
    Size->SetHeightOverride(CardSize.Y);
    Scale->SetContent(Size);
    auto* Frame = Card(Purple, 24);
    Frame->SetPadding(FMargin(26, 20));
    Size->SetContent(Frame);
    auto* Main = WidgetTree->ConstructWidget<UVerticalBox>();
    Frame->SetContent(Main);
    auto* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    Main->AddChildToVerticalBox(HeaderRow)->SetPadding(FMargin(0, 0, 0, 16));
    if (Logo)
    {
        auto* LogoImage = WidgetTree->ConstructWidget<UImage>();
        LogoImage->SetBrushFromTexture(Logo);
        auto* LogoSize = WidgetTree->ConstructWidget<USizeBox>();
        LogoSize->SetWidthOverride(80);
        LogoSize->SetHeightOverride(80);
        LogoSize->SetContent(LogoImage);
        HeaderRow->AddChildToHorizontalBox(LogoSize)->SetPadding(FMargin(0, 0, 16, 0));
    }
    auto* Titles = WidgetTree->ConstructWidget<UVerticalBox>();
    HeaderRow->AddChildToHorizontalBox(Titles)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Titles->AddChildToVerticalBox(Label(NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.3510866e16","FINAL TAKE  /  CASTING SHEET"), 16, Gold));
    Titles->AddChildToVerticalBox(Label(NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.d9f0d7528a","ACTOR WARDROBE"), 30, Cream));
    MovieLabel = Label(FText::GetEmpty(), 16, Cream);
    Titles->AddChildToVerticalBox(MovieLabel);
    auto* Clock = WidgetTree->ConstructWidget<UVerticalBox>();
    HeaderRow->AddChildToHorizontalBox(Clock);
    TimerLabel = Label(FText::FromString(TEXT("00:10")), 40, Gold);
    Clock->AddChildToVerticalBox(TimerLabel);
    TakeLabel = Label(FText::GetEmpty(), 15, Cream);
    TakeLabel->SetJustification(ETextJustify::Right);
    Clock->AddChildToVerticalBox(TakeLabel);
    auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    Main->AddChildToVerticalBox(Row)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* LeftWidth = WidgetTree->ConstructWidget<USizeBox>();
    LeftWidth->SetWidthOverride(420);
    Row->AddChildToHorizontalBox(LeftWidth)->SetPadding(FMargin(0, 0, 22, 0));
    auto* Left = Card(FLinearColor(.10, .09, .18), 16);
    LeftWidth->SetContent(Left);
    auto* Photo = WidgetTree->ConstructWidget<UVerticalBox>();
    Left->SetContent(Photo);
    Photo->AddChildToVerticalBox(Label(NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.1302ceb294","●  CASTING"), 15, FLinearColor(1, .25, .25)));
    Portrait = WidgetTree->ConstructWidget<UImage>();
    auto* PortraitScale = WidgetTree->ConstructWidget<UScaleBox>();
    PortraitScale->SetStretch(EStretch::ScaleToFit);
    auto* PortraitSize = WidgetTree->ConstructWidget<USizeBox>();
    PortraitSize->SetWidthOverride(512);
    PortraitSize->SetHeightOverride(512);
    PortraitSize->SetContent(Portrait);
    PortraitScale->SetContent(PortraitSize);
    Photo->AddChildToVerticalBox(PortraitScale)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    RoleLabel = Label(FText::GetEmpty(), 26, Cream);
    RoleLabel->SetJustification(ETextJustify::Center);
    Photo->AddChildToVerticalBox(RoleLabel);
    auto* Rotate = WidgetTree->ConstructWidget<UHorizontalBox>();
    Photo->AddChildToVerticalBox(Rotate)->SetPadding(FMargin(0, 12, 0, 0));
    Rotate
        ->AddChildToHorizontalBox(
            Button(NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.969e2cc476","LEFT"), TEXT("Rotate.Left"), ECustomizationButtonAction::RotateLeft))
        ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Rotate
        ->AddChildToHorizontalBox(
            Button(NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.95253203f7","RIGHT"), TEXT("Rotate.Right"), ECustomizationButtonAction::RotateRight))
        ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Right = WidgetTree->ConstructWidget<UVerticalBox>();
    Row->AddChildToHorizontalBox(Right)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Tabs = WidgetTree->ConstructWidget<UWrapBox>();
    Tabs->SetExplicitWrapSize(true);
    Tabs->SetWrapSize(700);
    Tabs->SetInnerSlotPadding(FVector2D(6));
    Right->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(0, 0, 0, 10));
    const FText Names[] = {NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.Names.62be7f5142","Eyes"), NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.Names.8b89346ec1","Mouth"),      NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.Names.748d59f0b5","Eyebrows"),      NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.Names.8f57bd6132","Skin"),
                            NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.Names.d31d367bc5","Clothing"), NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.Names.9d3045a4d1","Hats"), NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.Names.f3b50cd53c","Accessories"), NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.Names.72a83add2c","Objects")};
    const TCHAR* Ids[] = {TEXT("Eyes"),     TEXT("Mouth"), TEXT("Eyebrows"),    TEXT("Skin"),
                          TEXT("Clothing"), TEXT("Hats"),  TEXT("Accessories"), TEXT("Objects")};
    for (int32 I = 0; I < 8; ++I)
    {
        auto* B = Button(Names[I], FName(*(FString(TEXT("Tab.")) + Ids[I])),
                         ECustomizationButtonAction::Tab);
        B->Tab = static_cast<ECustomizationTab>(I);
        auto* TabSize = WidgetTree->ConstructWidget<USizeBox>();
        TabSize->SetWidthOverride(160);
        TabSize->SetContent(B);
        Tabs->AddChildToWrapBox(TabSize);
    }
    HandRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    Right->AddChildToVerticalBox(HandRow)->SetPadding(FMargin(0, 0, 0, 8));
    HandRow
        ->AddChildToHorizontalBox(
            Button(NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.ac66d21b47","LEFT HAND"), TEXT("Hand.Left"), ECustomizationButtonAction::LeftHand))
        ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    HandRow
        ->AddChildToHorizontalBox(
            Button(NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.d6afc6a304","RIGHT HAND"), TEXT("Hand.Right"), ECustomizationButtonAction::RightHand))
        ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    Right->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Options = WidgetTree->ConstructWidget<UWrapBox>();
    Options->SetExplicitWrapSize(true);
    Options->SetWrapSize(690);
    Options->SetInnerSlotPadding(FVector2D(8));
    Scroll->AddChild(Options);
    auto* Brief = Card(Navy, 10);
    Right->AddChildToVerticalBox(Brief)->SetPadding(FMargin(0, 10, 0, 0));
    ObjectivesLabel = Label(FText::GetEmpty(), 15, Cream);
    Brief->SetContent(ObjectivesLabel);
    auto* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>();
    Main->AddChildToVerticalBox(Bottom)->SetPadding(FMargin(0, 18, 0, 0));
    Bottom
        ->AddChildToHorizontalBox(
            Button(NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.995f2d9ddf","RESET"), TEXT("Action.Reset"), ECustomizationButtonAction::Reset))
        ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Bottom
        ->AddChildToHorizontalBox(
            Button(NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.16c79c5fdd","RANDOMIZE"), TEXT("Action.Random"), ECustomizationButtonAction::Random))
        ->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Ready = Button(NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.aa7a094e7b","READY!"), TEXT("Action.Ready"), ECustomizationButtonAction::Ready);
    ReadyButton = Ready;
    Ready->SetBackgroundColor(Gold);
    Bottom->AddChildToHorizontalBox(Ready)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
}
bool UUnifiedWardrobeWidget::CanCustomize() const
{
    const auto* Manager = GetMovieManager();
    return Manager && Manager->IsWardrobeOpen() && PreparationId == Manager->PreparationId &&
           Manager->LiveWardrobeComponent;
}
void UUnifiedWardrobeWidget::OpenWardrobe(AMovieManager* Manager, int32 Id)
{
    Super::OpenWardrobe(Manager, Id);
    ActiveTab = ECustomizationTab::Eyes;
    bSelectLeftHand = false;
    BuildOptions();
    UpdatePreview();
    RefreshWardrobe();
}
void UUnifiedWardrobeWidget::SelectTab(ECustomizationTab Tab)
{
    if (!CanCustomize())
    {
        return;
    }
    ActiveTab = Tab;
    BuildOptions();
    RefreshWardrobe();
    OnCustomizationTabChanged(Tab);
}
void UUnifiedWardrobeWidget::SelectHand(bool bLeft)
{
    if (CanCustomize())
    {
        bSelectLeftHand = bLeft;
        BuildOptions();
        RefreshWardrobe();
    }
}
bool UUnifiedWardrobeWidget::ChooseSkin(FLinearColor Color)
{
    const bool Accepted = CanCustomize() && GetMovieManager()->LiveWardrobeComponent->SetSkinColor(Color);
    RefreshWardrobe();
    return Accepted;
}
bool UUnifiedWardrobeWidget::ResetAll()
{
    const bool Accepted = CanCustomize() && GetMovieManager()->LiveWardrobeComponent->ResetCustomization();
    RefreshWardrobe();
    return Accepted;
}
bool UUnifiedWardrobeWidget::RandomizeAll()
{
    if (!CanCustomize())
    {
        return false;
    }
    UWardrobeComponent* Wardrobe = GetMovieManager()->LiveWardrobeComponent;
    bool Accepted = Wardrobe->RandomizeCustomization();
    if (Accepted && !SkinPresets.IsEmpty())
    {
        Accepted = Wardrobe->SetSkinColor(SkinPresets[FMath::RandHelper(SkinPresets.Num())].Color);
    }
    RefreshWardrobe();
    return Accepted;
}
void UUnifiedWardrobeWidget::RotatePreview(float DeltaDegrees)
{
    if (CanCustomize() && PreviewActor)
    {
        PreviewActor->RotateView(DeltaDegrees);
    }
}
UCustomizationOptionButton* UUnifiedWardrobeWidget::FindControl(FName Id) const
{
    for (UCustomizationOptionButton* B : Controls)
    {
        if (B && B->ControlId == Id)
        {
            return B;
        }
    }
    return nullptr;
}
void UUnifiedWardrobeWidget::Activate(UCustomizationOptionButton* B)
{
    if (!B || !CanCustomize())
    {
        return;
    }
    if (auto* Flow=AStudioFlowManager::Find(this)) { Flow->PlayInterfaceSound(); }
    switch (B->Action)
    {
    case ECustomizationButtonAction::Item:
        ChooseItem(B->Category, B->CosmeticId);
        break;
    case ECustomizationButtonAction::Tab:
        SelectTab(B->Tab);
        break;
    case ECustomizationButtonAction::Skin:
        ChooseSkin(B->SkinColor);
        break;
    case ECustomizationButtonAction::Ready:
        Ready();
        break;
    case ECustomizationButtonAction::Random:
        RandomizeAll();
        break;
    case ECustomizationButtonAction::Reset:
        ResetAll();
        break;
    case ECustomizationButtonAction::LeftHand:
        SelectHand(true);
        break;
    case ECustomizationButtonAction::RightHand:
        SelectHand(false);
        break;
    case ECustomizationButtonAction::RotateLeft:
        RotatePreview(-25);
        break;
    case ECustomizationButtonAction::RotateRight:
        RotatePreview(25);
        break;
    }
}
FText UUnifiedWardrobeWidget::ItemLabel(const FCosmeticDefinition& Item)
{
    return WardrobeDefaults::LocalizedName(Item.Id,Item.DisplayName);
}
void UUnifiedWardrobeWidget::BuildOptions()
{
    if (!Options || !GetMovieManager() || !GetMovieManager()->LiveWardrobeComponent)
    {
        return;
    }
    Options->ClearChildren();
    Controls.RemoveAll(
        [](const auto& B)
        {
            return B &&
                   (B->Action == ECustomizationButtonAction::Item || B->Action == ECustomizationButtonAction::Skin);
        });
    if (HandRow)
    {
        HandRow->SetVisibility(ActiveTab == ECustomizationTab::Objects ? ESlateVisibility::Visible
                                                                       : ESlateVisibility::Collapsed);
    }
    const auto AddOption = [&](UCustomizationOptionButton* B, UTexture2D* Icon, FLinearColor Swatch, bool bSwatch)
    {
        auto CaptionFont=B->Caption->GetFont();CaptionFont.Size=OptionFontSize;B->Caption->SetFont(CaptionFont);
        auto* Size = WidgetTree->ConstructWidget<USizeBox>();
        Size->SetWidthOverride(206);
        Size->SetHeightOverride(ActiveTab == ECustomizationTab::Objects ? 100 : 126);
        auto* Column = WidgetTree->ConstructWidget<UVerticalBox>();
        auto* Picture = WidgetTree->ConstructWidget<UImage>();
        Picture->SetDesiredSizeOverride(FVector2D(76));
        if (Icon)
        {
            Picture->SetBrushFromTexture(Icon);
        }
        else
        {
            Picture->SetColorAndOpacity(bSwatch ? Swatch : FLinearColor::Transparent);
        }
        auto* IconSize = WidgetTree->ConstructWidget<USizeBox>();
        IconSize->SetWidthOverride(ActiveTab == ECustomizationTab::Objects ? 40 : 52);
        IconSize->SetHeightOverride(ActiveTab == ECustomizationTab::Objects ? 40 : 52);
        IconSize->SetContent(Picture);
        if (!Icon && !bSwatch)
        {
            static const TMap<FName, FString> Marks = {
                {TEXT("Prop.Banana"), TEXT("BAN")},    {TEXT("Prop.Donut"), TEXT("O")},
                {TEXT("Prop.MoneyBag"), TEXT("$")},    {TEXT("Prop.Microphone"), TEXT("MIC")},
                {TEXT("Head.PoliceCap"), TEXT("POL")}, {TEXT("Head.RobberMask"), TEXT("LAD")},
                {TEXT("Head.Crown"), TEXT("REY")},     {TEXT("Head.Chicken"), TEXT("PÍO")},
                {TEXT("Face.Glasses"), TEXT("OO")},    {TEXT("Face.Mustache"), TEXT("W")},
                {TEXT("Body.Casual"), TEXT("CAS")},    {TEXT("Body.Uniform"), TEXT("UNI")},
                {TEXT("Body.Suit"), TEXT("TRA")},      {TEXT("Body.Dress"), TEXT("VES")}};
            const FString* Mark = Marks.Find(B->CosmeticId);
            auto* Glyph = Label(FText::FromString(Mark ? *Mark : TEXT("—")), 28, Navy);
            Glyph->SetJustification(ETextJustify::Center);
            IconSize->SetContent(Glyph);
        }
        Column->AddChildToVerticalBox(IconSize)->SetHorizontalAlignment(HAlign_Center);
        Column->AddChildToVerticalBox(B->Caption)->SetPadding(FMargin(2, 4));
        B->SetContent(Column);
        Size->SetContent(B);
        Options->AddChildToWrapBox(Size);
    };
    if (ActiveTab == ECustomizationTab::Skin)
    {
        for (const auto& P : SkinPresets)
        {
            auto* B =
                Button(P.Label, FName(*(FString(TEXT("Skin.")) + P.Id.ToString())), ECustomizationButtonAction::Skin);
            B->SkinColor = P.Color;
            AddOption(B, nullptr, P.Color, true);
        }
        return;
    }
    const ECosmeticCategory Categories[] = {
        ECosmeticCategory::Eyes, ECosmeticCategory::Mouth, ECosmeticCategory::Eyebrows, ECosmeticCategory::Body,
        ECosmeticCategory::Body, ECosmeticCategory::Head,  ECosmeticCategory::Face,     ECosmeticCategory::Prop};
    ECosmeticCategory Category = Categories[static_cast<uint8>(ActiveTab)];
    if (ActiveTab == ECustomizationTab::Objects && bSelectLeftHand)
    {
        Category = ECosmeticCategory::LeftProp;
    }
    if (Category != ECosmeticCategory::Body)
    {
        auto* B = Button(NSLOCTEXT("FinalTakeUI","UnifiedWardrobeWidget.6eef664840","None"), TEXT("Item.None"), ECustomizationButtonAction::Item);
        B->Category = Category;
        AddOption(B, nullptr, FLinearColor::Transparent, false);
    }
    for (const auto& Item : GetMovieManager()->LiveWardrobeComponent->GetCategoryOptions(Category))
    {
        auto* B = Button(ItemLabel(Item), FName(*(FString(TEXT("Item.")) + Item.Id.ToString())),
                         ECustomizationButtonAction::Item);
        B->Category = Category;
        B->CosmeticId = Item.Id;
        AddOption(B, Item.Icon, Gold, false);
    }
}
void UUnifiedWardrobeWidget::UpdatePreview()
{
    auto* Manager = GetMovieManager();
    if (!Manager || !Manager->LiveActingComponent || !Manager->LiveWardrobeComponent || !GetWorld())
    {
        return;
    }
    const auto Selection = Manager->LiveWardrobeComponent->GetSelection();
    const bool Changed =
        !bHasPreviewSelection || Selection.HeadId != PreviewSelection.HeadId ||
        Selection.BodyId != PreviewSelection.BodyId || Selection.FaceId != PreviewSelection.FaceId ||
        Selection.PropId != PreviewSelection.PropId || Selection.LeftHandPropId != PreviewSelection.LeftHandPropId ||
        Selection.EyeId != PreviewSelection.EyeId || Selection.MouthId != PreviewSelection.MouthId ||
        Selection.EyebrowId != PreviewSelection.EyebrowId || !Selection.SkinColor.Equals(PreviewSelection.SkinColor) ||
        Selection.bUseBodyColor != PreviewSelection.bUseBodyColor ||
        !Selection.BodyColor.Equals(PreviewSelection.BodyColor);
    if (!Changed)
    {
        return;
    }
    if (!PreviewActor)
    {
        FActorSpawnParameters P;
        P.ObjectFlags |= RF_Transient;
        P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        PreviewActor = GetWorld()->SpawnActor<AWardrobePreviewActor>(
            PreviewActorClass ? PreviewActorClass.Get() : AWardrobePreviewActor::StaticClass(), FVector(0, 0, -100000),
            FRotator::ZeroRotator, P);
    }
    if (PreviewActor && PreviewActor->Present(Manager->LiveWardrobeComponent->CaptureAppearance(),
                                              Manager->LiveActingComponent->CapturePlaybackSettings()))
    {
        if (!PreviewMaterial && PreviewUIMaterial)
        {
            PreviewMaterial = UMaterialInstanceDynamic::Create(PreviewUIMaterial, this);
        }
        if (PreviewMaterial)
        {
            PreviewMaterial->SetTextureParameterValue(TEXT("PreviewTexture"), PreviewActor->RenderTarget);
            if (Portrait)
            {
                Portrait->SetBrushFromMaterial(PreviewMaterial);
            }
        }
        PreviewSelection = Selection;
        bHasPreviewSelection = true;
    }
}
void UUnifiedWardrobeWidget::RefreshWardrobe()
{
    Super::RefreshWardrobe();
    auto* Manager = GetMovieManager();
    if (!Manager)
    {
        return;
    }
    if (MovieLabel)
    {
        MovieLabel->SetText(Manager->ResultsSettings.MovieTitle);
    }
    if (TakeLabel)
    {
        TakeLabel->SetText(
            FText::Format(NSLOCTEXT("WardrobeUI","Turn","TURN {0} / {1}"),FText::AsNumber(ViewData.TakeNumber),FText::AsNumber(ViewData.TakeCount)));
    }
    if (TimerLabel)
    {
        TimerLabel->SetText(
            FText::FromString(FString::Printf(TEXT("00:%02d"), FMath::CeilToInt(ViewData.RemainingSeconds))));
        TimerLabel->SetColorAndOpacity(ViewData.RemainingSeconds <= 3 ? FLinearColor(1, .2, .2) : Gold);
    }
    if (RoleLabel)
    {
        RoleLabel->SetText(ViewData.RoleName);
    }
    if (ObjectivesLabel && Manager->ObjectiveManager)
    {
        TArray<FText> Names;
        for(const auto& Objective:Manager->ObjectiveManager->Objectives){Names.Add(Objective.Definition.DisplayName);}
        ObjectivesLabel->SetText(FText::Format(NSLOCTEXT("WardrobeUI","YourScene","YOUR SCENE\n{0}"),FText::Join(FText::FromString(TEXT(" · ")),Names)));
    }
    for (UCustomizationOptionButton* B : Controls)
    {
        if (!B)
        {
            continue;
        }
        B->SetIsEnabled(CanCustomize());
        bool Selected = false;
        if (B->Action == ECustomizationButtonAction::Tab)
        {
            Selected = B->Tab == ActiveTab;
        }
        else if (B->Action == ECustomizationButtonAction::Item)
        {
            Selected = ViewData.Selection.GetId(B->Category) == B->CosmeticId;
        }
        else if (B->Action == ECustomizationButtonAction::Skin)
        {
            Selected = ViewData.Selection.SkinColor.Equals(B->SkinColor);
        }
        else if (B->Action == ECustomizationButtonAction::LeftHand)
        {
            Selected = bSelectLeftHand;
        }
        else if (B->Action == ECustomizationButtonAction::RightHand)
        {
            Selected = !bSelectLeftHand;
        }
        B->SetBackgroundColor(Selected || B->Action == ECustomizationButtonAction::Ready ? Gold : Cream);
    }
    UpdatePreview();
}
void UUnifiedWardrobeWidget::NativeDestruct()
{
    if (PreviewActor)
    {
        PreviewActor->Destroy();
        PreviewActor = nullptr;
    }
    PreviewMaterial = nullptr;
    bHasPreviewSelection = false;
    Super::NativeDestruct();
}
