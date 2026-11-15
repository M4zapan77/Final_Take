#pragma once
#include "ActingTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WardrobePreviewActor.generated.h"
class USkeletalMeshComponent;
class USceneCaptureComponent2D;
class UWardrobeComponent;
class UTextureRenderTarget2D;
class UPointLightComponent;

/** Presentation-only proxy. It has no recorder, acting events, possession, collision or scoring path. */
UCLASS(Transient)
class MYPROJECT_API AWardrobePreviewActor : public AActor
{
    GENERATED_BODY()
public:
    AWardrobePreviewActor();
    bool Present(const FRecordedAppearance& Appearance, const FActingPlaybackSettings& Acting);
    void RotateView(float DeltaDegrees);
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USkeletalMeshComponent> Body;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UWardrobeComponent> Wardrobe;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneCaptureComponent2D> Capture;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient) TObjectPtr<UTextureRenderTarget2D> RenderTarget;
    /** Minimum distance; the visible body and equipped items determine automatic framing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview") float CameraDistance = 80;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Preview",meta=(ClampMin="1",ClampMax="2")) float FramingPadding=1.12f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview") float FieldOfView = 32;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview", meta = (ClampMin = "-20", ClampMax = "10"))
    float ExposureCompensation = -12.5f;

private:
    void UpdateView();
    UPROPERTY() TObjectPtr<UPointLightComponent> KeyLight;
    UPROPERTY() TObjectPtr<UPointLightComponent> FillLight;
    float ViewYaw = 0;
};
