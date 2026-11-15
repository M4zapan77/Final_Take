#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StartupVideoWidget.generated.h"
class UStartupVideoComponent;
class UBorder;
class UImage;
class USizeBox;

/** Separate layers around the existing menu: aspect-fit video below, input/fade shield above. */
UCLASS()
class MYPROJECT_API UStartupVideoWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UStartupVideoWidget(const FObjectInitializer& Initializer);
    void BindVideos(UStartupVideoComponent* Component,bool bOverlay);
    void RefreshVideo();
protected:
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& Geometry,const FPointerEvent& Event) override;
    virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& Event) override;
private:
    UPROPERTY(Transient) TWeakObjectPtr<UStartupVideoComponent> Videos;
    UPROPERTY(Transient) TObjectPtr<UBorder> Black;
    UPROPERTY(Transient) TObjectPtr<UImage> VideoImage;
    UPROPERTY(Transient) TObjectPtr<USizeBox> AspectBox;
    bool bOverlayLayer=false;
};
