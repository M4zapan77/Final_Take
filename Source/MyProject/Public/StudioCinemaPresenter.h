#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MovieManager.h"
#include "StudioCinemaPresenter.generated.h"
class UStaticMeshComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UChildActorComponent;
class ACameraActor;
class APlayerController;
class UPointLightComponent;

/** Presents the existing bank camera on a theater screen; owns no recording or movie clock. */
UCLASS(Blueprintable)
class MYPROJECT_API AStudioCinemaPresenter : public AActor
{
    GENERATED_BODY()
public:
    AStudioCinemaPresenter();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaSeconds) override;
    UFUNCTION(BlueprintCallable, Category = "Cinema") bool StartPresentation(AMovieManager* Movie);
    UFUNCTION(BlueprintCallable, Category = "Cinema") void StopPresentation();
    UFUNCTION(BlueprintPure, Category = "Cinema") bool IsPresenting() const
    {
        return bPresenting;
    }
    UFUNCTION(BlueprintPure, Category = "Cinema") ACameraActor* GetFilmCamera() const
    {
        return FilmCamera.Get();
    }
    UFUNCTION(BlueprintPure, Category = "Cinema") ACameraActor* GetAudienceCamera() const;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinema") bool bEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinema") bool bUseFilmAudioListener = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinema") bool bUseOptionalRailAngle = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinema|Lighting",
              meta = (ClampMin = "-5", ClampMax = "20"))
    float AudienceExposureEV = 9;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cinema|Lighting",meta=(ClampMin="-3",ClampMax="4")) float AudienceExposureCompensation=1.5f;
    /** Fixed exposure for the actual film render target, independent from the theater. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cinema|Lighting",meta=(ClampMin="5",ClampMax="16")) float FilmExposureEV=11.75f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cinema|Lighting",meta=(ClampMin="0",ClampMax="20000")) float PremiereFillLumens=6500.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cinema|Lighting") TObjectPtr<UPointLightComponent> PremiereFill;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cinema|Lighting") TObjectPtr<UPointLightComponent> VaultFill;
    void ConfigureFilmExposure();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cinema|Camera", meta=(ClampMin="20",ClampMax="120"))
    float AudienceFieldOfView=55;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinema")
    TObjectPtr<UMaterialInterface> ScreenMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinema",
              meta = (ClampMin = "256", ClampMax = "3840"))
    int32 CaptureWidth = 1920;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinema",
              meta = (ClampMin = "144", ClampMax = "2160"))
    int32 CaptureHeight = 1080;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinema") bool bFlipScreenU = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinema") bool bFlipScreenV = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinema")
    TObjectPtr<UStaticMeshComponent> Screen;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinema")
    TObjectPtr<USceneCaptureComponent2D> FilmCapture;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cinema")
    TObjectPtr<UChildActorComponent> AudienceCamera;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Cinema")
    TObjectPtr<UTextureRenderTarget2D> ScreenTexture;
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Cinema") int32 CapturedFrames = 0;
    UFUNCTION(BlueprintImplementableEvent, Category = "Cinema") void OnCinemaStarted();
    UFUNCTION(BlueprintImplementableEvent, Category = "Cinema") void OnCinemaStopped();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    void BindMovie();
    UFUNCTION() void MovieStateChanged(EMovieState Previous, EMovieState Current);
    UPROPERTY(Transient) TWeakObjectPtr<AMovieManager> BoundMovie;
    UPROPERTY(Transient) TWeakObjectPtr<ACameraActor> FilmCamera;
    UPROPERTY(Transient) TWeakObjectPtr<APlayerController> Player;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> ScreenInstance;
    bool bPresenting = false;
    bool bOwnsListener = false;
    int32 LastDirectorShot = INDEX_NONE;
};
