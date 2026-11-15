#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ActingRecorderTestCharacter.generated.h"

class UActingRecorderComponent;
class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;

/** Disposable third-person placeholder for the recording milestone. No Content assets required. */
UCLASS()
class MYPROJECT_API AActingRecorderTestCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AActingRecorderTestCharacter();
    virtual void Tick(float DeltaSeconds) override;
    virtual void PossessedBy(AController* NewController) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Acting|Test")
    TObjectPtr<UActingRecorderComponent> ActingRecorder;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Acting|Test")
    TObjectPtr<UStaticMeshComponent> PlaceholderBody;

protected:
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
    void LookYaw(float Value);
    void LookPitch(float Value);

    UPROPERTY(VisibleAnywhere, Category = "Acting|Test")
    TObjectPtr<USpringArmComponent> CameraArm;

    UPROPERTY(VisibleAnywhere, Category = "Acting|Test")
    TObjectPtr<UCameraComponent> Camera;
};
