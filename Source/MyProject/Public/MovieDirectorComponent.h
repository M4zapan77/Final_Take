#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StoryWorldTypes.h"
#include "MovieDirectorComponent.generated.h"
class AMovieManager;
class ACameraActor;
class ACineCameraActor;
class AStudioRailRig;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMovieShotChanged,FName,ShotId,FName,FocusRoleId);

/** Read-only film presentation. Never advances the scene clock or executes story interactions. */
UCLASS(ClassGroup=(Acting),BlueprintType,meta=(BlueprintSpawnableComponent))
class MYPROJECT_API UMovieDirectorComponent:public UActorComponent
{
    GENERATED_BODY()
public:
    UMovieDirectorComponent();
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Director") bool bEnabled=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Director",meta=(ClampMin="1")) double MinimumShotSeconds=2.4;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Director",meta=(ClampMin="1")) double CutCooldownSeconds=2.4;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Director",meta=(ClampMin="2")) double TrackingShotSeconds=5;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Director") FVector TrackingOffset=FVector(320,-360,280);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Director",meta=(ClampMin="0")) float FollowSpeed=3;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Director") bool bUseExistingRail=true;
    UPROPERTY(BlueprintReadOnly,Transient,Category="Director") FName CurrentShot;
    UPROPERTY(BlueprintReadOnly,Transient,Category="Director") FName FocusRoleId;
    UPROPERTY(BlueprintReadOnly,Transient,Category="Director") int32 ShotCount=0;
    UPROPERTY(BlueprintAssignable,Category="Director") FMovieShotChanged OnShotChanged;
    UFUNCTION(BlueprintPure,Category="Director") ACameraActor* GetCamera() const;
    void Begin(AMovieManager* InMovie,ACameraActor* WideCamera);
    void Update(float DeltaSeconds);
    void Stop();
    static int32 EventPriority(EStoryEventType Type);
private:
    void SelectShot(FName Id,FName Role,FName Target=NAME_None);
    bool IsVisible(FVector Camera,FVector Focus) const;
    UPROPERTY(Transient) TObjectPtr<AMovieManager> Movie;
    UPROPERTY(Transient) TObjectPtr<ACineCameraActor> Camera;
    TWeakObjectPtr<ACameraActor> Wide;
    TWeakObjectPtr<AStudioRailRig> Rail;
    double LastCutTime=0;
    int32 EventCursor=0,PendingPriority=0;
    FStorySceneEvent Pending;
    FName TargetId;
    bool bFinalShotChosen=false;
};
