#include "WardrobePreviewActor.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "WardrobeComponent.h"

AWardrobePreviewActor::AWardrobePreviewActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SetActorEnableCollision(false);
    bReplicates = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("PreviewRoot")));
    Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewBody"));
    Body->SetupAttachment(RootComponent);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetGenerateOverlapEvents(false);
    Body->SetCanEverAffectNavigation(false);
    Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Wardrobe = CreateDefaultSubobject<UWardrobeComponent>(TEXT("PreviewAppearance"));
    Wardrobe->SetReadOnly(true);
    Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PortraitCapture"));
    Capture->SetupAttachment(RootComponent);
    Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    Capture->PostProcessSettings.bOverride_AutoExposureMethod = true;
    Capture->PostProcessSettings.AutoExposureMethod = AEM_Manual;
    Capture->PostProcessSettings.bOverride_AutoExposureBias = true;
    Capture->PostProcessSettings.AutoExposureBias = ExposureCompensation;
    Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
    Capture->bCaptureEveryFrame = true;
    Capture->bCaptureOnMovement = false;
    Capture->bAlwaysPersistRenderingState = true;
    Capture->ShowFlags.SetFog(false);
    Capture->ShowFlags.SetAtmosphere(false);
    Capture->ShowFlags.SetBloom(false);
    KeyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortraitKey"));
    KeyLight->SetupAttachment(RootComponent);
    KeyLight->SetRelativeLocation(FVector(180, -130, 190));
    KeyLight->SetAttenuationRadius(650);
    KeyLight->SetCastShadows(false);
    FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortraitFill"));
    FillLight->SetupAttachment(RootComponent);
    FillLight->SetRelativeLocation(FVector(130, 150, 60));
    FillLight->SetAttenuationRadius(550);
    FillLight->SetCastShadows(false);
    KeyLight->SetIntensityUnits(ELightUnits::Candelas);
    KeyLight->SetIntensity(100);
    FillLight->SetIntensityUnits(ELightUnits::Candelas);
    FillLight->SetIntensity(80);
}
bool AWardrobePreviewActor::Present(const FRecordedAppearance& Appearance, const FActingPlaybackSettings& Acting)
{
    Capture->PostProcessSettings.AutoExposureBias = ExposureCompensation;
    if (!Acting.SkeletalMesh || !Appearance.IsValid())
    {
        return false;
    }
    const bool bChanged = Body->GetSkeletalMeshAsset() != Acting.SkeletalMesh;
    if (bChanged)
    {
        Body->SetSkeletalMesh(Acting.SkeletalMesh);
    }
    Body->SetRelativeTransform(Acting.MeshRelativeTransform);
    if (Acting.AnimationClass && Body->GetAnimClass() != Acting.AnimationClass)
    {
        Body->SetAnimInstanceClass(Acting.AnimationClass);
    }
    if (!Wardrobe->ApplySnapshot(Appearance, Body, nullptr, true))
    {
        return false;
    }
    if (!RenderTarget)
    {
        RenderTarget = NewObject<UTextureRenderTarget2D>(this);
        RenderTarget->RenderTargetFormat = RTF_RGBA8;
        RenderTarget->ClearColor = FLinearColor(0, 0, 0, 1);
        RenderTarget->InitAutoFormat(1024, 1024);
        RenderTarget->UpdateResourceImmediate(true);
        Capture->TextureTarget = RenderTarget;
    }
    Capture->ClearShowOnlyComponents();
    Capture->ShowOnlyActorComponents(this);
    UpdateView();
    return true;
}
void AWardrobePreviewActor::RotateView(float DeltaDegrees)
{
    ViewYaw = FMath::Fmod(ViewYaw + DeltaDegrees, 360.f);
    UpdateView();
}
void AWardrobePreviewActor::UpdateView()
{
    FBox Frame = Body->Bounds.GetBox();
    for (const UMeshComponent* Part : Wardrobe->CosmeticMeshes)
    {
        if (IsValid(Part) && Part->IsVisible()) { Frame += Part->Bounds.GetBox(); }
    }
    const FVector Focus = Frame.IsValid ? Frame.GetCenter() - GetActorLocation() : FVector::ZeroVector;
    double Distance=FMath::Max(20.0,static_cast<double>(CameraDistance));
    if(Frame.IsValid)
    {
        const FVector Direction=FRotator(0,ViewYaw,0).Vector();
        const FVector Right=FRotationMatrix(FRotator(0,ViewYaw,0)).GetUnitAxis(EAxis::Y);
        const double HalfTan=FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(FieldOfView,10.f,90.f)*.5f));
        const FVector Extent=Frame.GetExtent();
        for(int32 I=0;I<8;++I)
        {
            const FVector Corner=Extent*FVector((I&1)?1:-1,(I&2)?1:-1,(I&4)?1:-1);
            const double Span=FMath::Max(FMath::Abs(FVector::DotProduct(Corner,Right)),FMath::Abs(Corner.Z));
            Distance=FMath::Max(Distance,Span*FMath::Clamp(FramingPadding,1.f,2.f)/HalfTan+FVector::DotProduct(Corner,Direction));
        }
    }
    const FVector Location = Focus + FRotator(0, ViewYaw, 0).RotateVector(FVector(Distance, 0, 12));
    Capture->SetRelativeLocation(Location);
    Capture->SetRelativeRotation((Focus - Location).Rotation());
    Capture->FOVAngle = FieldOfView;
}
