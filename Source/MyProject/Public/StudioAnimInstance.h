#pragma once
#include "Animation/AnimInstance.h"
#include "CoreMinimal.h"
#include "StudioAnimInstance.generated.h"

/** Small locomotion input layer shared by the live mesh, preview and recorded actors. */
UCLASS(Blueprintable, BlueprintType)
class MYPROJECT_API UStudioAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    UPROPERTY(BlueprintReadOnly, Category = "Movement") float Speed = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Movement") bool bIsMoving = false;
    UPROPERTY(BlueprintReadOnly, Category = "Movement") bool bIsRunning = false;
    UPROPERTY(BlueprintReadOnly, Category = "Movement") bool bIsInAir = false;
    UPROPERTY(BlueprintReadOnly, Category = "Movement") bool bIsDescending = false;

private:
    FVector PreviousLocation = FVector::ZeroVector;
    double GroundZ = 0;
    bool bHasPrevious = false;
};
