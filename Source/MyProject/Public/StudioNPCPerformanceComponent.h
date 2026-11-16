#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActingRecording.h"
#include "StudioNPCPerformanceComponent.generated.h"
class ACharacter;
class UAnimSequence;
/** Small scene-clock-driven background performances; no AI, navigation or scoring. */
UCLASS(ClassGroup=(Acting),BlueprintType,meta=(BlueprintSpawnableComponent))
class MYPROJECT_API UStudioNPCPerformanceComponent:public UActorComponent
{
    GENERATED_BODY()
public:
    UStudioNPCPerformanceComponent();
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="NPC") TObjectPtr<UAnimSequence> IdleAnimation;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="NPC") TObjectPtr<UAnimSequence> WalkAnimation;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="NPC",meta=(ClampMin="0",ClampMax="80")) float PaceDistance=45;
    UPROPERTY(BlueprintReadOnly,Transient,Category="NPC") FActingRecording Performance;
    UPROPERTY(BlueprintReadOnly,Transient,Category="NPC") int32 VariationSeed=0;
    void Initialize(ACharacter* Actor,int32 Seed,bool bMayPace);
    void UpdateLive(double Time,bool bRunning,bool bCalm);
    void ObserveBoundary(double Time);
    void BeginPlayback();
    void UpdatePlayback(double Time,bool bRunning);
    FName ConsumeAmbientGesture(double Time,bool bCalm);
    static FCosmeticSelection VaryFace(const FCosmeticSelection& Base,const TArray<FCosmeticDefinition>& Catalog,int32 Seed);
private:
    void PresentAnimation(double Time,bool bRunning,bool bMoving);
    UPROPERTY(Transient) TObjectPtr<ACharacter> Character;
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> CurrentAmbient;
    FTransform InitialTransform=FTransform::Identity;
    double NextSample=0,PendingHold=0,NextGesture=3,LastLiveTime=0;
    int32 GestureIndex=0;
    bool bPacing=false;
};
