#include "StartupVideoWidget.h"
#include "StartupVideoComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "InputCoreTypes.h"

UStartupVideoWidget::UStartupVideoWidget(const FObjectInitializer& Initializer):Super(Initializer) { SetIsFocusable(true); }
void UStartupVideoWidget::BindVideos(UStartupVideoComponent* Component,bool bOverlay)
{
    Videos=Component; bOverlayLayer=bOverlay;
    if (!WidgetTree) { WidgetTree=NewObject<UWidgetTree>(this,TEXT("WidgetTree")); }
    auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget=Canvas;
    Black=WidgetTree->ConstructWidget<UBorder>(); Black->SetPadding(FMargin(0)); Black->SetBrushColor(FLinearColor::Black);
    auto* Full=Canvas->AddChildToCanvas(Black); Full->SetAnchors(FAnchors(0,0,1,1)); Full->SetOffsets(FMargin(0));
    if (!bOverlayLayer)
    {
        auto* Scale=WidgetTree->ConstructWidget<UScaleBox>(); Scale->SetStretch(EStretch::ScaleToFit); Black->SetContent(Scale);
        AspectBox=WidgetTree->ConstructWidget<USizeBox>(); AspectBox->SetHeightOverride(1000); Scale->SetContent(AspectBox);
        VideoImage=WidgetTree->ConstructWidget<UImage>(); AspectBox->SetContent(VideoImage);
        if (Component->GetVideoMaterial()) { VideoImage->SetBrushFromMaterial(Component->GetVideoMaterial()); }
    }
    RefreshVideo();
}
void UStartupVideoWidget::RefreshVideo()
{
    const auto* Component=Videos.Get(); if (!Component) { return; }
    if (bOverlayLayer)
    {
        const bool bBlock=Component->IsInputBlocked();
        SetVisibility(bBlock?ESlateVisibility::Visible:ESlateVisibility::HitTestInvisible);
        if (Black) { Black->SetRenderOpacity(Component->BlackOpacity); }
    }
    else
    {
        SetVisibility(ESlateVisibility::HitTestInvisible);
        if (AspectBox) { AspectBox->SetWidthOverride(1000*Component->GetAspectRatio()); }
        if (VideoImage) { VideoImage->SetVisibility(Component->bHasVideoFrame?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed); }
    }
}
FReply UStartupVideoWidget::NativeOnMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event)
{
    if (bOverlayLayer && Videos.IsValid() && Videos->IsInputBlocked())
    {
        if (Event.GetEffectingButton()==EKeys::LeftMouseButton) { Videos->IntroPointerDown(); }
        return FReply::Handled().CaptureMouse(TakeWidget());
    }
    return Super::NativeOnMouseButtonDown(Geometry,Event);
}
FReply UStartupVideoWidget::NativeOnMouseButtonUp(const FGeometry& Geometry,const FPointerEvent& Event)
{
    if (bOverlayLayer && Videos.IsValid())
    { Videos->IntroPointerUp(); return FReply::Handled().ReleaseMouseCapture(); }
    return Super::NativeOnMouseButtonUp(Geometry,Event);
}
void UStartupVideoWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& Event)
{
    if (bOverlayLayer && Videos.IsValid()) { Videos->IntroPointerUp(); }
    Super::NativeOnMouseCaptureLost(Event);
}
