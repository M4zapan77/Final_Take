#include "StudioFlowWidget.h"
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
    auto* Control=WidgetTree->ConstructWidget<UButton>(); Control->SetBackgroundColor(AccentColor);
    FButtonStyle Style=Control->GetStyle(); Style.NormalPadding=FMargin(12,14); Style.PressedPadding=FMargin(12,15,12,13); Control->SetStyle(Style);
    auto* Caption=WidgetTree->ConstructWidget<UTextBlock>(); Caption->SetFont(BodyFont); Caption->SetText(Label);
    Caption->SetColorAndOpacity(FSlateColor(FLinearColor(.015f,.02f,.025f))); Control->SetContent(Caption);
    auto* ControlSlot=Parent->AddChildToHorizontalBox(Control); ControlSlot->SetPadding(FMargin(6,14)); ControlSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    return Control;
}
USlider* UStudioFlowWidget::AudioRow(UVerticalBox* Parent,const FText& Label,TObjectPtr<UTextBlock>& ValueLabel)
{
    auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>(); Parent->AddChildToVerticalBox(Row)->SetPadding(FMargin(0,20));
    ValueLabel=WidgetTree->ConstructWidget<UTextBlock>(); ValueLabel->SetFont(BodyFont); ValueLabel->SetColorAndOpacity(FSlateColor(TextColor));
    ValueLabel->SetText(Label); auto* NameSlot=Row->AddChildToHorizontalBox(ValueLabel); NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Control=WidgetTree->ConstructWidget<USlider>(); Control->SetStepSize(.05f);
    auto* ControlSlot=Row->AddChildToHorizontalBox(Control); ControlSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); ControlSlot->SetVerticalAlignment(VAlign_Center);
    return Control;
}
void UStudioFlowWidget::BuildLayout()
{
    auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget=Canvas;
    Backdrop=WidgetTree->ConstructWidget<UBorder>(); Backdrop->SetBrushColor(BackgroundImage?FLinearColor::White:BackgroundColor);
    if (BackgroundImage) { Backdrop->SetBrushFromTexture(BackgroundImage); }
    auto* BackdropSlot=Canvas->AddChildToCanvas(Backdrop); BackdropSlot->SetAnchors(FAnchors(0,0,1,1)); BackdropSlot->SetOffsets(FMargin(0));
    PageColumn=WidgetTree->ConstructWidget<UVerticalBox>();
    auto* PageSlot=Canvas->AddChildToCanvas(PageColumn); PageSlot->SetAnchors(FAnchors(.09f,.12f,.91f,.91f)); PageSlot->SetOffsets(FMargin(0));
    Heading=Text(PageColumn,HeadingFont); Heading->SetColorAndOpacity(FSlateColor(AccentColor));
    PageScroll=WidgetTree->ConstructWidget<UScrollBox>(); auto* ScrollSlot=PageColumn->AddChildToVerticalBox(PageScroll); ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* Details=WidgetTree->ConstructWidget<UVerticalBox>(); PageScroll->AddChild(Details);
    Detail=Text(Details,BodyFont); RoleRows=WidgetTree->ConstructWidget<UVerticalBox>(); Details->AddChildToVerticalBox(RoleRows);
    Tutorial=Text(Details,BodyFont); AudioRows=WidgetTree->ConstructWidget<UVerticalBox>(); Details->AddChildToVerticalBox(AudioRows);
    MasterSlider=AudioRow(AudioRows,MasterLabel,MasterValue); MusicSlider=AudioRow(AudioRows,MusicLabel,MusicValue); EffectsSlider=AudioRow(AudioRows,EffectsLabel,EffectsValue);
    MasterSlider->OnValueChanged.AddDynamic(this,&UStudioFlowWidget::MasterChanged); MusicSlider->OnValueChanged.AddDynamic(this,&UStudioFlowWidget::MusicChanged); EffectsSlider->OnValueChanged.AddDynamic(this,&UStudioFlowWidget::EffectsChanged);
    AmbientSlider=AudioRow(AudioRows,AmbientLabel,AmbientValue); UISlider=AudioRow(AudioRows,UILabel,UIValue);
    AmbientSlider->OnValueChanged.AddDynamic(this,&UStudioFlowWidget::AmbientChanged); UISlider->OnValueChanged.AddDynamic(this,&UStudioFlowWidget::UIChanged);
    Text(AudioRows,BodyFont)->SetText(NSLOCTEXT("StudioUI","LanguageLabel","Language / Idioma"));
    auto* Languages=WidgetTree->ConstructWidget<UHorizontalBox>();AudioRows->AddChildToVerticalBox(Languages);
    SpanishButton=Button(Languages,NSLOCTEXT("StudioUI","SpanishLanguage","Español"));EnglishButton=Button(Languages,NSLOCTEXT("StudioUI","EnglishLanguage","English"));
    SpanishButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickSpanish);EnglishButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickEnglish);
    auto* Controls=WidgetTree->ConstructWidget<UHorizontalBox>(); PageColumn->AddChildToVerticalBox(Controls);
    PlayButton=Button(Controls,PlayText); HowButton=Button(Controls,HowToPlayText); CreditsButton=Button(Controls,CreditsText);
    AudioButton=Button(Controls,AudioText); QuitButton=Button(Controls,QuitText); StartButton=Button(Controls,StartText); BackButton=Button(Controls,BackText);
    PlayButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickPlay); HowButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickHow);
    CreditsButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickCredits); AudioButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickAudio);
    QuitButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickQuit); StartButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickStart); BackButton->OnClicked.AddDynamic(this,&UStudioFlowWidget::ClickBack);
    DirectorPanel=WidgetTree->ConstructWidget<UBorder>(); DirectorPanel->SetBrushColor(FLinearColor(.01f,.01f,.015f,.85f)); DirectorPanel->SetPadding(FMargin(12));
    auto* DirectorSlot=Canvas->AddChildToCanvas(DirectorPanel); DirectorSlot->SetAnchors(FAnchors(.5f,.025f)); DirectorSlot->SetAlignment(FVector2D(.5f,0)); DirectorSlot->SetOffsets(FMargin(0,0,520,62));
    DirectorLabel=WidgetTree->ConstructWidget<UTextBlock>(); DirectorLabel->SetFont(HeadingFont); DirectorLabel->SetJustification(ETextJustify::Center); DirectorLabel->SetColorAndOpacity(FSlateColor(AccentColor)); DirectorPanel->SetContent(DirectorLabel);
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
    if (PageColumn) { PageColumn->SetVisibility(bFront||bNext?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed); PageColumn->SetRenderOpacity(Page==EStudioScreen::StartingGame?1.f:ViewData.PageOpacity); }
    if (Heading)
    {
        FText Label=ViewData.GameTitle;
        if (bIntro) { Label=ViewData.MovieTitle; }
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
        if (bIntro) { Body=ViewData.Synopsis; }
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
    {const auto* Language=UStudioLanguageSubsystem::Find(this);const bool Spanish=!Language || Language->GetLanguage()==TEXT("es");SpanishButton->SetBackgroundColor(Spanish?AccentColor:FLinearColor(.35,.35,.4));EnglishButton->SetBackgroundColor(!Spanish?AccentColor:FLinearColor(.35,.35,.4));}
    if (PlayButton)
    {
        if (auto* Caption=Cast<UTextBlock>(PlayButton->GetContent())) { Caption->SetText(Flow->bStartupPresentation?StartGameText:PlayText); }
        const bool bEnabled=!Flow->bStartupPresentation || Flow->StartupVideos->IsHubInteractive();
        for (UButton* Control:{PlayButton.Get(),HowButton.Get(),CreditsButton.Get(),AudioButton.Get(),QuitButton.Get(),BackButton.Get()}) { Control->SetIsEnabled(bEnabled); }
        BackButton->SetIsEnabled(bEnabled || bIntro);
        for (UButton* Control:{PlayButton.Get(),HowButton.Get(),CreditsButton.Get(),AudioButton.Get(),QuitButton.Get()}) { Control->SetVisibility(bMain?ESlateVisibility::Visible:ESlateVisibility::Collapsed); }
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
