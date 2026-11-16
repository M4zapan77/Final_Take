#include "StudioCinemaPresenter.h"
#include "StudioRailRig.h"
#include "MovieDirectorComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/TargetPoint.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

AStudioCinemaPresenter::AStudioCinemaPresenter()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    bReplicates = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("CinemaRoot")));
    Screen = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MovieScreen"));
    Screen->SetupAttachment(RootComponent);
    Screen->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane")));
    Screen->SetRelativeRotation(FRotator(0, 90, 90));
    Screen->SetRelativeScale3D(FVector(8, 4.5, 1));
    Screen->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Screen->CastShadow = false;
    Screen->SetCanEverAffectNavigation(false);
    FilmCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("ExistingMovieCapture"));
    FilmCapture->SetupAttachment(RootComponent);
    FilmCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    FilmCapture->bCaptureEveryFrame = false;
    FilmCapture->bCaptureOnMovement = false;
    FilmCapture->bAlwaysPersistRenderingState = true;
    FilmCapture->MaxViewDistanceOverride = 4000;
    AudienceCamera = CreateDefaultSubobject<UChildActorComponent>(TEXT("AudienceView"));
    AudienceCamera->SetupAttachment(RootComponent);
    AudienceCamera->SetChildActorClass(ACameraActor::StaticClass());
    AudienceCamera->SetRelativeLocation(FVector(-1200, 0, -70));
    AudienceCamera->SetRelativeRotation(FRotator(3.34, 0, 0));
    PremiereFill=CreateDefaultSubobject<UPointLightComponent>(TEXT("PremiereSoftFill"));PremiereFill->SetupAttachment(RootComponent);
    VaultFill=CreateDefaultSubobject<UPointLightComponent>(TEXT("PremiereVaultFill"));VaultFill->SetupAttachment(RootComponent);
    for(UPointLightComponent* Light:{PremiereFill.Get(),VaultFill.Get()})
    {
        Light->SetMobility(EComponentMobility::Movable);Light->SetCastShadows(false);Light->SetVisibility(false);
        Light->IntensityUnits=ELightUnits::Lumens;Light->SetAttenuationRadius(1700);Light->SetLightColor(FLinearColor(1,.91,.77));
    }
}
void AStudioCinemaPresenter::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if(ScreenMaterial && !bPresenting){Screen->SetMaterial(0,ScreenMaterial);}
}
void AStudioCinemaPresenter::BeginPlay()
{
    Super::BeginPlay();
    if (auto* View = GetAudienceCamera())
    {
        UCameraComponent* Camera = View->GetCameraComponent();
        Camera->SetFieldOfView(AudienceFieldOfView);
        Camera->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
        Camera->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
        Camera->PostProcessSettings.AutoExposureMinBrightness = AudienceExposureEV;
        Camera->PostProcessSettings.AutoExposureMaxBrightness = AudienceExposureEV;
        Camera->PostProcessSettings.bOverride_AutoExposureBias=true;
        Camera->PostProcessSettings.AutoExposureBias=AudienceExposureCompensation;
        Camera->PostProcessSettings.bOverride_BloomIntensity = true;
        Camera->PostProcessSettings.BloomIntensity = 0;
        Camera->PostProcessSettings.bOverride_MotionBlurAmount = true;
        Camera->PostProcessSettings.MotionBlurAmount = 0;
    }
    BindMovie();
}
ACameraActor* AStudioCinemaPresenter::GetAudienceCamera() const
{
    return Cast<ACameraActor>(AudienceCamera->GetChildActor());
}
void AStudioCinemaPresenter::BindMovie()
{
    if (BoundMovie.IsValid() || !GetWorld())
    {
        return;
    }
    for (TActorIterator<AMovieManager> It(GetWorld()); It; ++It)
    {
        BoundMovie = *It;
        BoundMovie->OnStateChanged.AddUniqueDynamic(this, &AStudioCinemaPresenter::MovieStateChanged);
        if (BoundMovie->State == EMovieState::MoviePremiere)
        {
            StartPresentation(BoundMovie.Get());
        }
        break;
    }
}
void AStudioCinemaPresenter::MovieStateChanged(EMovieState Previous, EMovieState Current)
{
    if (Current == EMovieState::MoviePremiere)
    {
        StartPresentation(BoundMovie.Get());
    }
    else if (Current != EMovieState::Finished)
    {
        StopPresentation();
    }
}
bool AStudioCinemaPresenter::StartPresentation(AMovieManager* Movie)
{
    StopPresentation();
    if (!bEnabled || !Movie || !ScreenMaterial || !GetAudienceCamera() || !Movie->GetPremiereCamera())
    {
        return false;
    }
    APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
    if (!Controller || !Controller->IsLocalController())
    {
        return false;
    }
    FilmCamera = Movie->GetPremiereCamera();
    Player = Controller;
    if (bUseOptionalRailAngle)
    {
        for (TActorIterator<AStudioRailRig> It(GetWorld()); It; ++It)
        {
            if (It->GetRailCamera())
            {
                FilmCamera = It->GetRailCamera();
                break;
            }
        }
    }
    if (!ScreenTexture)
    {
        ScreenTexture = NewObject<UTextureRenderTarget2D>(this);
        ScreenTexture->RenderTargetFormat = RTF_RGBA8;
        ScreenTexture->ClearColor = FLinearColor::Black;
        ScreenTexture->InitAutoFormat(FMath::Clamp(CaptureWidth, 256, 3840),
                                      FMath::Clamp(CaptureHeight, 144, 2160));
        ScreenTexture->UpdateResourceImmediate(true);
    }
    ScreenInstance = UMaterialInstanceDynamic::Create(ScreenMaterial, this);
    Screen->SetMaterial(0, ScreenInstance);
    ScreenInstance->SetTextureParameterValue(TEXT("MovieTexture"), ScreenTexture);
    ScreenInstance->SetScalarParameterValue(TEXT("FlipU"), bFlipScreenU ? 1.f : 0.f);
    ScreenInstance->SetScalarParameterValue(TEXT("FlipV"), bFlipScreenV ? 1.f : 0.f);
    FilmCapture->TextureTarget = ScreenTexture;
    FilmCapture->HiddenActors.Reset();
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        if (It->ActorHasTag(TEXT("CinemaSet")) || It->ActorHasTag(TEXT("FinalBankCeiling")))
        {
            FilmCapture->HideActorComponents(*It);
        }
    }
    FilmCapture->SetWorldTransform(FilmCamera->GetCameraComponent()->GetComponentTransform());
    FilmCapture->FOVAngle = FilmCamera->GetCameraComponent()->FieldOfView;
    FilmCapture->PostProcessSettings = FilmCamera->GetCameraComponent()->PostProcessSettings;
    ConfigureFilmExposure();
    FVector Anchor=FVector(0,0,140);
    for(TActorIterator<ATargetPoint> It(GetWorld());It;++It){if(It->ActorHasTag(TEXT("StorySceneAnchor"))){Anchor=It->GetActorLocation();break;}}
    PremiereFill->SetWorldLocation(Anchor+FVector(0,-100,280));VaultFill->SetWorldLocation(Anchor+FVector(-60,650,200));
    PremiereFill->SetIntensity(PremiereFillLumens);VaultFill->SetIntensity(PremiereFillLumens*.7f);
    PremiereFill->SetVisibility(true);VaultFill->SetVisibility(true);
    FilmCapture->bCameraCutThisFrame = true;
    LastDirectorShot = Movie->MovieDirector ? Movie->MovieDirector->ShotCount : INDEX_NONE;
    FilmCapture->bCaptureEveryFrame = true;
    FilmCapture->SetComponentTickEnabled(true);
    CapturedFrames = 0;
    bPresenting = true;
    if (bUseFilmAudioListener)
    {
        Controller->SetAudioListenerOverride(FilmCamera->GetRootComponent(), FVector::ZeroVector,
                                             FRotator::ZeroRotator);
        bOwnsListener = true;
    }
    Controller->SetViewTargetWithBlend(GetAudienceCamera(), 0);
    OnCinemaStarted();
    return true;
}
void AStudioCinemaPresenter::StopPresentation()
{
    const bool WasPresenting = bPresenting;
    bPresenting = false;
    PremiereFill->SetVisibility(false);VaultFill->SetVisibility(false);
    FilmCapture->bCaptureEveryFrame = false;
    FilmCapture->SetComponentTickEnabled(false);
    if (bOwnsListener && Player.IsValid())
    {
        Player->ClearAudioListenerOverride();
    }
    bOwnsListener = false;
    Player.Reset();
    FilmCamera.Reset();
    if (WasPresenting)
    {
        OnCinemaStopped();
    }
}
void AStudioCinemaPresenter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    BindMovie();
    if (bPresenting)
    {
        if(BoundMovie.IsValid() && !bUseOptionalRailAngle){FilmCamera=BoundMovie->GetPremiereCamera();}
        if (!FilmCamera.IsValid())
        {
            StopPresentation();
            return;
        }
        FilmCapture->SetWorldTransform(FilmCamera->GetCameraComponent()->GetComponentTransform());
        FilmCapture->FOVAngle = FilmCamera->GetCameraComponent()->FieldOfView;
        const int32 DirectorShot=BoundMovie.IsValid() && BoundMovie->MovieDirector ? BoundMovie->MovieDirector->ShotCount : INDEX_NONE;
        if(DirectorShot!=LastDirectorShot)
        {
            // The capture owns a separate rendering history from the film camera.
            // Mark cuts explicitly so old occlusion/motion history is not reused.
            FilmCapture->bCameraCutThisFrame=true;LastDirectorShot=DirectorShot;
        }
        ++CapturedFrames;
    }
}
void AStudioCinemaPresenter::EndPlay(const EEndPlayReason::Type Reason)
{
    StopPresentation();
    if (BoundMovie.IsValid())
    {
        BoundMovie->OnStateChanged.RemoveDynamic(this, &AStudioCinemaPresenter::MovieStateChanged);
    }
    Super::EndPlay(Reason);
}
void AStudioCinemaPresenter::ConfigureFilmExposure()
{
    auto& PP=FilmCapture->PostProcessSettings;
    PP.bOverride_AutoExposureMinBrightness=PP.bOverride_AutoExposureMaxBrightness=true;
    PP.AutoExposureMinBrightness=PP.AutoExposureMaxBrightness=FilmExposureEV;
    PP.bOverride_AutoExposureBias=true;PP.AutoExposureBias=0;
    PP.bOverride_AutoExposureApplyPhysicalCameraExposure=true;PP.AutoExposureApplyPhysicalCameraExposure=false;
    PP.bOverride_MotionBlurAmount=true;PP.MotionBlurAmount=0;
    FilmCapture->PostProcessBlendWeight=1;
}
