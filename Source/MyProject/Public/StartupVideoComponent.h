#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StartupVideoComponent.generated.h"

class AStudioFlowManager;
class APlayerController;
class UFileMediaSource;
class UMediaPlayer;
class UMediaTexture;
class UMediaSoundComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStartupVideoWidget;

UENUM(BlueprintType)
enum class EStartupVideoStage : uint8 { Inactive, StudioIntro, GameIntro, Hub, StartingGame };
UENUM(BlueprintType)
enum class EStartupVideoPhase : uint8 { Stopped, Opening, Priming, FadeIn, Playing, FadeOut };

USTRUCT(BlueprintType)
struct MYPROJECT_API FStartupVideoSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Videos") bool bEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Videos") TObjectPtr<UFileMediaSource> StudioIntro;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Videos") TObjectPtr<UFileMediaSource> GameIntroduction;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Videos") TObjectPtr<UFileMediaSource> HubBackground;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Videos") TObjectPtr<UMaterialInterface> VideoMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Videos") TSoftObjectPtr<UWorld> StartingGameplayLevel;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Videos", meta=(ClampMin="0")) double FadeSeconds = .3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Videos", meta=(ClampMin="0.1")) double OpenTimeoutSeconds = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Videos", meta=(ClampMin="0")) double PrimeSeconds = .1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Videos", meta=(ClampMin="0",ClampMax="1")) float HubDimmerOpacity = .25f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Videos", meta=(ClampMin="0")) float AudioVolume = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Videos") bool bMuteHubAudio = false;
    bool IsValid() const;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStartupVideoEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStartupVideoStageChanged,EStartupVideoStage,Stage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStartupVideoIssue,FText,Message);

/** One media player at a time; the hub player survives all menu-page changes. */
UCLASS(ClassGroup=(Studio), Blueprintable, meta=(BlueprintSpawnableComponent))
class MYPROJECT_API UStartupVideoComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStartupVideoComponent();
    void InitializeVideos(AStudioFlowManager* Manager,APlayerController* PlayerController);
    UFUNCTION(BlueprintCallable, Category="Startup Videos") void BeginSequence(bool bHubOnly=false);
    UFUNCTION(BlueprintCallable, Category="Startup Videos") bool SkipIntroductions();
    UFUNCTION(BlueprintCallable, Category="Startup Videos") bool RequestStartGame();
    UFUNCTION(BlueprintCallable, Category="Startup Videos") void StopVideos();
    UFUNCTION(BlueprintPure, Category="Startup Videos") bool IsEnabledForFlow() const;
    UFUNCTION(BlueprintPure, Category="Startup Videos") bool IsHubInteractive() const;
    UFUNCTION(BlueprintPure, Category="Startup Videos") bool IsIntro() const;
    UFUNCTION(BlueprintPure, Category="Startup Videos") bool IsInputBlocked() const;
    UFUNCTION(BlueprintPure, Category="Startup Videos") bool OwnsMenuBackground() const;
    UFUNCTION(BlueprintPure, Category="Startup Videos") float GetAspectRatio() const { return AspectRatio; }
    UFUNCTION(BlueprintPure, Category="Startup Videos") UMaterialInstanceDynamic* GetVideoMaterial() const;
    void IntroPointerDown();
    void IntroPointerUp();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Startup Videos") FStartupVideoSettings Settings;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Startup Videos") EStartupVideoStage Stage=EStartupVideoStage::Inactive;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Startup Videos") EStartupVideoPhase Phase=EStartupVideoPhase::Stopped;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Startup Videos") float BlackOpacity=1.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Startup Videos") bool bHasVideoFrame=false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Startup Videos") bool bSkipped=false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Startup Videos") int32 HubLoops=0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Startup Videos") int32 MediaFailures=0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Startup Videos") TObjectPtr<UMediaPlayer> MediaPlayer;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Startup Videos") TObjectPtr<UMediaTexture> MediaTexture;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Startup Videos") TObjectPtr<UMediaSoundComponent> MediaSound;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Startup Videos") TObjectPtr<UStartupVideoWidget> BackdropWidget;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Startup Videos") TObjectPtr<UStartupVideoWidget> OverlayWidget;
    UPROPERTY(BlueprintAssignable, Category="Startup Videos") FStartupVideoEvent OnHubPrepared;
    UPROPERTY(BlueprintAssignable, Category="Startup Videos") FStartupVideoEvent OnHubInteractive;
    UPROPERTY(BlueprintAssignable, Category="Startup Videos") FStartupVideoEvent OnGameplayBlack;
    UPROPERTY(BlueprintAssignable, Category="Startup Videos") FStartupVideoStageChanged OnStageChanged;
    UPROPERTY(BlueprintAssignable, Category="Startup Videos") FStartupVideoIssue OnMediaIssue;
protected:
    virtual void TickComponent(float Delta,ELevelTick TickType,FActorComponentTickFunction* Function) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void OpenStage(EStartupVideoStage NewStage);
    void FadeTo(EStartupVideoStage Target);
    void FailedMedia(const TCHAR* Reason);
    void CloseMedia();
    void EnsurePresentation();
    void StartFadeIn();
    void UpdatePresentation();
    UFUNCTION() void MediaOpened(FString Url);
    UFUNCTION() void MediaFailed(FString Url);
    UFUNCTION() void MediaEnded();
    UPROPERTY(Transient) TWeakObjectPtr<AStudioFlowManager> Flow;
    UPROPERTY(Transient) TWeakObjectPtr<APlayerController> Controller;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> VideoMID;
    EStartupVideoStage NextStage=EStartupVideoStage::Inactive;
    double PhaseStarted=0,FadeStartOpacity=1,LastMediaTime=0,LastProgress=0,LastLoopTime=-100,ReleaseTime=-100;
    float AspectRatio=16.f/9.f;
    uint64 ReleaseFrame=0;
    uint32 PlaybackGeneration=0;
    bool bOpened=false,bSkipClickHeld=false,bWaitForRelease=false,bHubAnnounced=false,bClosing=false,bEnding=false;
};
