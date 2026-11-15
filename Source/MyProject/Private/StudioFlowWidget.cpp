#include "StudioFlowWidget.h"
#include "StudioUITheme.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "StudioFlowManager.h"
#include "StudioLanguageSubsystem.h"
#include "StartupVideoComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"

UStudioFlowWidget::UStudioFlowWidget(const FObjectInitializer& Initializer):Super(Initializer)
{
    SetIsFocusable(true); HeadingFont=FCoreStyle::GetDefaultFontStyle(TEXT("Bold"),32); BodyFont=FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),18);
    MenuTagline=NSLOCTEXT("StudioUI","Tagline","One actor. Three roles. One questionable movie.");
    IntroductionControls=NSLOCTEXT("StudioUI","IntroControlsStory","BEFORE THE CAMERA ROLLS\nWardrobe: up to 30 seconds per character, only in round one. Ready starts early.\nSix turns of up to 10 seconds; the next actor inherits your consequences.\nWASD / Mouse: move / look | Space: jump | E: interact / grab\nQ: drop | Left click: throw | T: Cut | 1-5: gestures | Z X C V B N: sounds\nYour current objective and full controls stay visible while recording.");
    PlayText=NSLOCTEXT("StudioUI","Play","Play"); StartGameText=NSLOCTEXT("StudioUI","StartGame","Start Game"); HowToPlayText=NSLOCTEXT("StudioUI","How","How to Play");
    CreditsText=NSLOCTEXT("StudioUI","Credits","Credits"); AudioText=NSLOCTEXT("StudioUI","Settings","Settings");
    QuitText=NSLOCTEXT("StudioUI","Quit","Quit"); StartText=NSLOCTEXT("StudioUI","Start","Choose My Costume");
    BackText=NSLOCTEXT("StudioUI","Back","Back"); NextActorText=NSLOCTEXT("StudioUI","Next","NEXT ACTOR");
    WrapText=NSLOCTEXT("StudioUI","Wrap","THAT'S A WRAP!");
    MasterLabel=NSLOCTEXT("StudioUI","Master","Master"); MusicLabel=NSLOCTEXT("StudioUI","Music","Music"); EffectsLabel=NSLOCTEXT("StudioUI","Effects","Effects / Director");
    AmbientLabel=NSLOCTEXT("StudioUI","Ambient","Ambience"); UILabel=NSLOCTEXT("StudioUI","InterfaceAudio","Interface");
}
void UStudioFlowWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if(!UITheme){UITheme=UStudioUITheme::LoadDefault();}
    if(UITheme){HeadingFont=UITheme->Heading(28);TextColor=UITheme->Cream;AccentColor=UITheme->Gold;}
    if (!WidgetTree) { WidgetTree=NewObject<UWidgetTree>(this,TEXT("WidgetTree")); }
    if (!WidgetTree->RootWidget) { BuildLayout(); }
}
UTextBlock* UStudioFlowWidget::Text(UVerticalBox* Parent,const FSlateFontInfo& Font)
{
    auto* Label=WidgetTree->ConstructWidget<UTextBlock>(); Label->SetFont(Font); Label->SetAutoWrapText(true);
    Label->SetColorAndOpacity(FSlateColor(TextColor)); Parent->AddChildToVerticalBox(Label)->SetPadding(FMargin(0,0,0,12)); return Label;
}
UButton* UStudioFlowWidget::Button(UHorizontalBox* Parent,const FText& Label)
{
    auto* Control=WidgetTree->ConstructWidget<UStudioArtButton>(); Control->SetBackgroundColor(AccentColor);
    FButtonStyle Style=Control->GetStyle(); Style.NormalPadding=FMargin(12,14); Style.PressedPadding=FMargin(12,15,12,13); Control->SetStyle(Style);
    auto* Caption=WidgetTree->ConstructWidget<UTextBlock>(); Caption->SetFont(BodyFont); Caption->SetText(Label);Caption->SetAutoWrapText(true);Caption->SetJustification(ETextJustify::Center);
    Caption->SetColorAndOpacity(FSlateColor::UseForeground()); Control->SetContent(Caption);
    if(UITheme){UITheme->StyleButton(Control);Caption->SetFont(UITheme->Heading(18));}
    auto* ControlSlot=Parent->AddChildToHorizontalBox(Control); ControlSlot->SetPadding(FMargin(5,8)); ControlSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    return Control;
}
USlider* UStudioFlowWidget::AudioRow(UVerticalBox* Parent,const FText& Label,TObjectPtr<UTextBlock>& ValueLabel)
{
    auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>(); Parent->AddChildToVerticalBox(Row)->SetPadding(FMargin(0,8));
    ValueLabel=WidgetTree->ConstructWidget<UTextBlock>(); ValueLabel->SetFont(BodyFont); ValueLabel->SetColorAndOpacity(FSlateColor(TextColor));
    ValueLabel->SetText(Label); auto* NameSlot=Row->AddChildToHorizontalBox(ValueLabel); NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Control=WidgetTree->ConstructWidget<USlider>(); Control->SetStepSize(.05f);
    auto* ControlSlot=Row->AddChildToHorizontalBox(Control); ControlSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); ControlSlot->SetVerticalAlignment(VAlign_Center);
    return Control;
}
void UStudioFlowWidget::BuildLayout()
{
    auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Canvas;
    const auto Full=[](UCanvasPanel* Parent,UWidget* Child){auto* S=Parent->AddChildToCanvas(Child);S->SetAnchors(FAnchors(0,0,1,1));S->SetOffsets(FMargin(0));return S;};
    Backdrop=WidgetTree->ConstructWidget<UBorder>();Backdrop->SetBrushColor(BackgroundColor);Full(Canvas,Backdrop);
    auto* Scale=WidgetTree->ConstructWidget<UScaleBox>();Scale->SetStretch(EStretch::ScaleToFit);
    auto* ScaleSlot=Full(Canvas,Scale);ScaleSlot->SetAnchors(FAnchors(.035,.035,.965,.965));
    auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(1240);Size->SetHeightOverride(680);Scale->SetContent(Size);
    auto* Pages=WidgetTree->ConstructWidget<UCanvasPanel>();Size->SetContent(Pages);
    MainMenuColumn=WidgetTree->ConstructWidget<UVerticalBox>();
    auto* MenuSlot=Pages->AddChildToCanvas(MainMenuColumn);MenuSlot->SetAnchors(FAnchors(.5));MenuSlot->SetAlignment(FVector2D(.5));MenuSlot->SetOffsets(FMargin(0,0,400,610));
    if(UITheme && UITheme->Texture(TEXT("Logo")))
    {
        auto* Image=WidgetTree->ConstructWidget<UImage>();Image->SetBrushFromTexture(UITheme->Texture(TEXT("Logo")),true);
        auto* LogoScale=WidgetTree->ConstructWidget<UScaleBox>();LogoScale->SetStretch(EStretch::ScaleToFit);LogoScale->SetContent(Image);
        auto* LogoSize=WidgetTree->ConstructWidget<USizeBox>();LogoSize->SetWidthOverride(460);LogoSize->SetHeightOverride(250);LogoSize->SetContent(LogoScale);
        MainMenuColumn->AddChildToVerticalBox(LogoSize)->SetPadding(FMargin(-30,0,-30,12));
    }
    else{Text(MainMenuColumn,HeadingFont)->SetText(FText::FromString(TEXT("FINAL TAKE")));}
    const auto MenuButton=[&](const FText& Label,bool Primary)
    {
        auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();MainMenuColumn->AddChildToVerticalBox(Row);
        auto* B=Button(Row,Label);if(UITheme && Primary){UITheme->StyleButton(B,TEXT("Primary"));}return B;
    };
    PlayButton=MenuButton(PlayText,true);AudioButton=MenuButton(AudioText,false);CreditsButton=MenuButton(CreditsText,false);QuitButton=MenuButton(QuitText,false);
    PlayButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickPlay);AudioButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickAudio);
    CreditsButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickCredits);QuitButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickQuit);

    PageFrame=WidgetTree->ConstructWidget<UBorder>();PageFrame->SetPadding(FMargin(48,42,48,30));
    if(UITheme){PageFrame->SetBrush(UITheme->Brush(TEXT("Wardrobe.GridPanel"),FMargin(.08,.12,.08,.12)));}else{PageFrame->SetBrushColor(BackgroundColor);}
    Full(Pages,PageFrame);PageColumn=WidgetTree->ConstructWidget<UVerticalBox>();PageFrame->SetContent(PageColumn);
    Heading=Text(PageColumn,HeadingFont);Heading->SetColorAndOpacity(AccentColor);
    auto* PageRow=WidgetTree->ConstructWidget<UHorizontalBox>();PageColumn->AddChildToVerticalBox(PageRow)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* CoverSize=WidgetTree->ConstructWidget<USizeBox>();CoverSize->SetWidthOverride(360);
    CoverPanel=WidgetTree->ConstructWidget<UBorder>();CoverSize->SetContent(CoverPanel);CoverPanel->SetPadding(FMargin(28,42,28,44));
    if(UITheme){CoverPanel->SetBrush(UITheme->Brush(TEXT("Wardrobe.ActorPanel"),FMargin(.1,.18,.1,.14)));}else{CoverPanel->SetBrushColor(AccentColor);}
    PageRow->AddChildToHorizontalBox(CoverSize)->SetPadding(FMargin(0,0,24,0));
    auto* CoverColumn=WidgetTree->ConstructWidget<UVerticalBox>();CoverPanel->SetContent(CoverColumn);
    auto* CoverImage=WidgetTree->ConstructWidget<UImage>();
    if(UITheme){CoverImage->SetBrushFromTexture(UITheme->MovieCover?UITheme->MovieCover.Get():UITheme->Texture(TEXT("Logo")),true);}
    auto* CoverScale=WidgetTree->ConstructWidget<UScaleBox>();CoverScale->SetStretch(EStretch::ScaleToFit);CoverScale->SetContent(CoverImage);
    CoverColumn->AddChildToVerticalBox(CoverScale)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CoverTitle=Text(CoverColumn,HeadingFont);CoverTitle->SetJustification(ETextJustify::Center);CoverTitle->SetColorAndOpacity(UITheme?UITheme->Navy:FLinearColor::Black);
    auto* MovieInfo=Text(CoverColumn,BodyFont);MovieInfo->SetText(NSLOCTEXT("FinalUI","OneMovieInfo","BANK COMEDY\n1 player · 3 roles · 6 turns"));auto InfoFont=BodyFont;InfoFont.Size=14;MovieInfo->SetFont(InfoFont);MovieInfo->SetJustification(ETextJustify::Center);MovieInfo->SetColorAndOpacity(UITheme?UITheme->Navy:FLinearColor::Black);
    PageScroll=WidgetTree->ConstructWidget<UScrollBox>();PageRow->AddChildToHorizontalBox(PageScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Details=WidgetTree->ConstructWidget<UVerticalBox>();PageScroll->AddChild(Details);
    Detail=Text(Details,BodyFont);RoleRows=WidgetTree->ConstructWidget<UVerticalBox>();Details->AddChildToVerticalBox(RoleRows);
    Tutorial=Text(Details,BodyFont);AudioRows=WidgetTree->ConstructWidget<UVerticalBox>();Details->AddChildToVerticalBox(AudioRows);
    MasterSlider=AudioRow(AudioRows,MasterLabel,MasterValue);MusicSlider=AudioRow(AudioRows,MusicLabel,MusicValue);EffectsSlider=AudioRow(AudioRows,EffectsLabel,EffectsValue);
    MasterSlider->OnValueChanged.AddDynamic(this,&UStudioFlowWidget::MasterChanged);MusicSlider->OnValueChanged.AddDynamic(this,&UStudioFlowWidget::MusicChanged);EffectsSlider->OnValueChanged.AddDynamic(this,&UStudioFlowWidget::EffectsChanged);
    AmbientSlider=AudioRow(AudioRows,AmbientLabel,AmbientValue);UISlider=AudioRow(AudioRows,UILabel,UIValue);
    AmbientSlider->OnValueChanged.AddDynamic(this,&UStudioFlowWidget::AmbientChanged);UISlider->OnValueChanged.AddDynamic(this,&UStudioFlowWidget::UIChanged);
    Text(AudioRows,BodyFont)->SetText(NSLOCTEXT("StudioUI","LanguageLabel","Language / Idioma"));
    auto* Languages=WidgetTree->ConstructWidget<UHorizontalBox>();AudioRows->AddChildToVerticalBox(Languages);
    SpanishButton=Button(Languages,NSLOCTEXT("StudioUI","SpanishLanguage","Español"));EnglishButton=Button(Languages,NSLOCTEXT("StudioUI","EnglishLanguage","English"));
    SpanishButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickSpanish);EnglishButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickEnglish);
    auto* Controls=WidgetTree->ConstructWidget<UHorizontalBox>();PageColumn->AddChildToVerticalBox(Controls)->SetPadding(FMargin(0,10,0,0));
    BackButton=Button(Controls,BackText);HowButton=Button(Controls,HowToPlayText);HubSettingsButton=Button(Controls,AudioText);StartButton=Button(Controls,NSLOCTEXT("FinalUI","StartMovie","Start Movie"));
    FSlateChildSize StartSize(ESlateSizeRule::Fill);StartSize.Value=2;CastChecked<UHorizontalBoxSlot>(StartButton->Slot)->SetSize(StartSize);
    if(UITheme){UITheme->StyleButton(StartButton,TEXT("Primary"));}
    HowButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickHow);HubSettingsButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickAudio);
    StartButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickStart);BackButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickBack);
    DirectorPanel=WidgetTree->ConstructWidget<UBorder>();DirectorPanel->SetBrushColor(FLinearColor(.01,.01,.015,.85));DirectorPanel->SetPadding(FMargin(12));
    auto* DirectorSlot=Canvas->AddChildToCanvas(DirectorPanel);DirectorSlot->SetAnchors(FAnchors(.5,.025));DirectorSlot->SetAlignment(FVector2D(.5,0));DirectorSlot->SetOffsets(FMargin(0,0,520,62));
    DirectorLabel=WidgetTree->ConstructWidget<UTextBlock>();DirectorLabel->SetFont(HeadingFont);DirectorLabel->SetJustification(ETextJustify::Center);DirectorLabel->SetColorAndOpacity(AccentColor);DirectorPanel->SetContent(DirectorLabel);
}
void UStudioFlowWidget::BindFlow(AStudioFlowManager* Manager)
{
    Flow=Manager; LastRevision=INDEX_NONE;
    if (!WidgetTree) { WidgetTree=NewObject<UWidgetTree>(this,TEXT("WidgetTree")); }
    if (!WidgetTree->RootWidget) { BuildLayout(); }
    RefreshFlow();
}
AStudioFlowManager* UStudioFlowWidget::GetFlowManager() const { return Flow.Get(); }
bool UStudioFlowWidget::CanUse() const { return Flow.IsValid() && Flow->ActiveWidget==this && IsInViewport() && GetIsEnabled(); }
bool UStudioFlowWidget::PlayMovie() { return CanUse() && Flow->PlayMovie(); }
bool UStudioFlowWidget::BeginMovie() { return CanUse() && Flow->BeginIntroducedMovie(); }
bool UStudioFlowWidget::HowToPlay() { return CanUse() && Flow->ShowHowToPlay(); }
bool UStudioFlowWidget::Credits() { return CanUse() && Flow->ShowCredits(); }
bool UStudioFlowWidget::AudioSettings() { return CanUse() && Flow->ShowAudioSettings(); }
bool UStudioFlowWidget::SelectLanguage(const FString& Language)
{if(!CanUse()){return false;}auto* Settings=UStudioLanguageSubsystem::Find(this);if(!Settings || !Settings->SetLanguage(Language)){return false;}LastRevision=INDEX_NONE;RefreshFlow();return true;}
void UStudioFlowWidget::ClickSpanish(){SelectLanguage(TEXT("es"));}
void UStudioFlowWidget::ClickEnglish(){SelectLanguage(TEXT("en"));}
bool UStudioFlowWidget::Back() { return CanUse() && Flow->BackToMenu(); }
bool UStudioFlowWidget::Quit() { return CanUse() && Flow->QuitGame(); }
void UStudioFlowWidget::SetVolumes(float Master,float Music,float Effects)
{ if (CanUse() && !bUpdating) { FStudioAudioLevels Levels=Flow->GetView().Audio; Levels.Master=Master; Levels.Music=Music; Levels.Effects=Effects; Flow->SetAudioLevels(Levels); } }
void UStudioFlowWidget::ClickPlay() { PlayMovie(); } void UStudioFlowWidget::ClickHow() { HowToPlay(); }
void UStudioFlowWidget::ClickCredits() { Credits(); } void UStudioFlowWidget::ClickAudio() { AudioSettings(); }
void UStudioFlowWidget::ClickQuit() { Quit(); } void UStudioFlowWidget::ClickStart() { BeginMovie(); } void UStudioFlowWidget::ClickBack() { Back(); }
void UStudioFlowWidget::MasterChanged(float Value) { const auto Levels=Flow.IsValid()?Flow->GetView().Audio:ViewData.Audio; SetVolumes(Value,Levels.Music,Levels.Effects); }
void UStudioFlowWidget::MusicChanged(float Value) { const auto Levels=Flow.IsValid()?Flow->GetView().Audio:ViewData.Audio; SetVolumes(Levels.Master,Value,Levels.Effects); }
void UStudioFlowWidget::EffectsChanged(float Value) { const auto Levels=Flow.IsValid()?Flow->GetView().Audio:ViewData.Audio; SetVolumes(Levels.Master,Levels.Music,Value); }
void UStudioFlowWidget::AmbientChanged(float Value) { if (CanUse() && !bUpdating) { auto Levels=Flow->GetView().Audio; Levels.Ambient=Value; Flow->SetAudioLevels(Levels); } }
void UStudioFlowWidget::UIChanged(float Value) { if (CanUse() && !bUpdating) { auto Levels=Flow->GetView().Audio; Levels.UI=Value; Flow->SetAudioLevels(Levels); } }

void UStudioFlowWidget::RefreshFlow()
{
    if (!Flow.IsValid()) { return; }
    ViewData=Flow->GetView(); const EStudioScreen Page=ViewData.Screen;
    const bool bVideoOnly=Page==EStudioScreen::StartupVideos;
    const bool bFront=Flow->IsFrontEndOpen() && !bVideoOnly,bNext=Page==EStudioScreen::NextCharacter,bMain=Page==EStudioScreen::MainMenu || Page==EStudioScreen::StartingGame;
    const bool bIntro=Page==EStudioScreen::MovieIntroduction,bAudio=Page==EStudioScreen::AudioSettings;
    SetVisibility(bFront?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::HitTestInvisible);
    if (Backdrop) { Backdrop->SetVisibility(bFront||bNext?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed); }
    if (Backdrop && Flow->bStartupPresentation)
    {
        Backdrop->SetBrushFromTexture(nullptr);
        Backdrop->SetBrushColor(FLinearColor(0,0,0,Flow->StartupVideos->Settings.HubDimmerOpacity));
    }
    if(MainMenuColumn){MainMenuColumn->SetVisibility(bMain?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed);MainMenuColumn->SetRenderOpacity(ViewData.PageOpacity);}
    if(PageFrame){PageFrame->SetVisibility((bFront && !bMain)||bNext?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed);PageFrame->SetRenderOpacity(ViewData.PageOpacity);}
    if(CoverPanel){CoverPanel->GetParent()->SetVisibility(bIntro?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);}
    if(CoverTitle){CoverTitle->SetText(ViewData.MovieTitle);}
    if (Heading)
    {
        FText Label=ViewData.GameTitle;
        if (bIntro) { Label=NSLOCTEXT("FinalUI","StudioHeading","MOVIE STUDIO"); }
        else if (Page==EStudioScreen::HowToPlay) { Label=HowToPlayText; }
        else if (Page==EStudioScreen::Credits) { Label=CreditsText; }
        else if (bAudio) { Label=AudioText; }
        else if (bNext) { Label=ViewData.TakeNumber>0?NextActorText:WrapText; }
        else if (Page==EStudioScreen::Error) { Label=NSLOCTEXT("StudioUI","Error","The director needs a moment."); }
        Heading->SetText(Label);
    }
    if (Detail)
    {
        FText Body=MenuTagline;
        if (bIntro) { Body=FText::Format(NSLOCTEXT("FinalUI","HubBrief","{0}\n\n{1}"),UITheme && !UITheme->HubDescription.IsEmpty()?UITheme->HubDescription:NSLOCTEXT("FinalUI","HubDescription","One player, three roles, six chaotic acting turns. Create your own bank robbery movie."),ViewData.Synopsis); }
        else if (Page==EStudioScreen::HowToPlay) { Body=ViewData.HowToPlay; }
        else if (Page==EStudioScreen::Credits) { Body=ViewData.Credits; }
        else if (bAudio) { Body=NSLOCTEXT("StudioUI","AudioHint","Adjust the mix for this game session. Music, sound effects and the director use these levels."); }
        else if (Page==EStudioScreen::Error) { Body=ViewData.Error; }
        else if (bNext)
        {
            Body=ViewData.TakeNumber>0?FText::Format(NSLOCTEXT("StudioUI","NextRole","TAKE {0}/3 - {1}\n{2}\n\nWardrobe opens in {3} seconds."),FText::AsNumber(ViewData.TakeNumber),ViewData.CurrentRole,ViewData.CurrentScript,FText::AsNumber(FMath::CeilToInt(ViewData.RemainingSeconds)))
                :NSLOCTEXT("StudioUI","WrapBody","All three performances are in the can. Your movie premiere is about to begin.");
            if(ViewData.bChronologicalMovie)
            {
                Body=ViewData.TakeNumber>0?FText::Format(NSLOCTEXT("FinalTake","NextStoryRole","Round {0}/2 · Turn {1}/6 · {2}\n{3}\n\nNext character in {4} s."),FText::AsNumber(ViewData.RoundNumber),FText::AsNumber(ViewData.TakeNumber),ViewData.CurrentRole,ViewData.CurrentScript,FText::AsNumber(FMath::CeilToInt(ViewData.RemainingSeconds)))
                    :NSLOCTEXT("FinalTake","StoryWrap","All six performances are ready. Let the premiere begin!");
            }
        }
        Detail->SetText(Body);
    }
    if (RoleRows)
    {
        RoleRows->SetVisibility(bIntro?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
        if (LastRevision!=ViewData.Revision)
        {
            RoleRows->ClearChildren();
            if (bIntro) { for (int32 Index=0;Index<ViewData.Roles.Num();++Index)
            { auto* Label=Text(RoleRows,BodyFont); const auto& Brief=ViewData.Roles[Index]; Label->SetText(FText::Format(NSLOCTEXT("StudioUI","RoleBrief","{0}. {1}: {2}"),FText::AsNumber(Index+1),Brief.Name,Brief.Script)); } }
        }
    }
    if (Tutorial) { Tutorial->SetVisibility(bIntro?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed); Tutorial->SetText(IntroductionControls); }
    if (AudioRows) { AudioRows->SetVisibility(bAudio?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed); }
    if (MasterSlider)
    {
        TGuardValue<bool> Guard(bUpdating,true); MasterSlider->SetValue(ViewData.Audio.Master); MusicSlider->SetValue(ViewData.Audio.Music); EffectsSlider->SetValue(ViewData.Audio.Effects);
        if(AmbientSlider){AmbientSlider->SetValue(ViewData.Audio.Ambient);UISlider->SetValue(ViewData.Audio.UI);}
        const auto Caption=[](const FText& Label,float Level){return FText::Format(NSLOCTEXT("StudioUI","Level","{0}: {1}%"),Label,FText::AsNumber(FMath::RoundToInt(Level*100)));};
        MasterValue->SetText(Caption(MasterLabel,ViewData.Audio.Master)); MusicValue->SetText(Caption(MusicLabel,ViewData.Audio.Music)); EffectsValue->SetText(Caption(EffectsLabel,ViewData.Audio.Effects));
        if(AmbientValue){AmbientValue->SetText(Caption(AmbientLabel,ViewData.Audio.Ambient));UIValue->SetText(Caption(UILabel,ViewData.Audio.UI));}
    }
    if(SpanishButton && EnglishButton)
    {const auto* Language=UStudioLanguageSubsystem::Find(this);const bool Spanish=!Language || Language->GetLanguage()==TEXT("es");if(UITheme){if(LastRevision!=ViewData.Revision){UITheme->StyleButton(SpanishButton,Spanish?TEXT("Primary"):TEXT("Secondary"));UITheme->StyleButton(EnglishButton,!Spanish?TEXT("Primary"):TEXT("Secondary"));}}else{SpanishButton->SetBackgroundColor(Spanish?AccentColor:FLinearColor(.35,.35,.4));EnglishButton->SetBackgroundColor(!Spanish?AccentColor:FLinearColor(.35,.35,.4));}}
    if (PlayButton)
    {
        if (auto* Caption=Cast<UTextBlock>(PlayButton->GetContent())) { Caption->SetText(PlayText); }
        const bool bEnabled=!Flow->bStartupPresentation || Flow->StartupVideos->IsHubInteractive();
        for (UButton* Control:{PlayButton.Get(),HowButton.Get(),CreditsButton.Get(),AudioButton.Get(),QuitButton.Get(),BackButton.Get()}) { Control->SetIsEnabled(bEnabled); }
        BackButton->SetIsEnabled(bEnabled || bIntro);
        for(UButton* Control:{PlayButton.Get(),CreditsButton.Get(),AudioButton.Get(),QuitButton.Get()}){Control->SetVisibility(bMain?ESlateVisibility::Visible:ESlateVisibility::Collapsed);}
        HowButton->SetVisibility(bIntro?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
        if(HubSettingsButton){HubSettingsButton->SetVisibility(bIntro?ESlateVisibility::Visible:ESlateVisibility::Collapsed);HubSettingsButton->SetIsEnabled(bEnabled);}
        StartButton->SetVisibility(bIntro?ESlateVisibility::Visible:ESlateVisibility::Collapsed); BackButton->SetVisibility(bFront && !bMain?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    }
    if (DirectorPanel)
    {
        const bool bShow=!bFront && !(Page==EStudioScreen::Wardrobe && Flow->Movie && Flow->Movie->bUseUnifiedCustomization) && !ViewData.DirectorText.IsEmpty() && ViewData.DirectorOpacity>0;
        DirectorPanel->SetVisibility(bShow?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed); DirectorPanel->SetRenderOpacity(ViewData.DirectorOpacity); DirectorLabel->SetText(ViewData.DirectorText);
    }
    if (LastRevision!=ViewData.Revision && PageScroll) { PageScroll->SetScrollOffset(0); }
    LastRevision=ViewData.Revision; OnStudioViewChanged(ViewData);
}

void UStudioFlowWidget::FocusPage()
{
    UWidget* Target=ViewData.Screen==EStudioScreen::MainMenu?static_cast<UWidget*>(PlayButton.Get()):ViewData.Screen==EStudioScreen::MovieIntroduction?static_cast<UWidget*>(StartButton.Get()):static_cast<UWidget*>(BackButton.Get());
    if (Target) { Target->SetKeyboardFocus(); } else { SetKeyboardFocus(); }
}
FReply UStudioFlowWidget::NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if (!Event.IsRepeat() && Event.GetKey()==EKeys::Escape && Back()) { return FReply::Handled(); }
    return Super::NativeOnKeyDown(Geometry,Event);
}
