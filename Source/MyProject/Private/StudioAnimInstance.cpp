#include "StudioAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ReplayActor.h"

void UStudioAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    bHasPrevious = false;
    Speed = 0;
    bIsMoving = bIsRunning = bIsInAir = bIsDescending = false;
    if (AActor* Owner = GetOwningActor())
    {
        PreviousLocation = Owner->GetActorLocation();
        GroundZ = PreviousLocation.Z;
    }
    SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
}
void UStudioAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    AActor* Owner = GetOwningActor();
    if (!Owner)
    {
        return;
    }
    FVector Velocity = FVector::ZeroVector;
    if (const ACharacter* Character = Cast<ACharacter>(Owner))
    {
        Velocity = Character->GetVelocity();
        bIsInAir = Character->GetCharacterMovement() && Character->GetCharacterMovement()->IsFalling();
    }
    else if (Owner->IsA<AReplayActor>())
    {
        if (bHasPrevious && DeltaSeconds > SMALL_NUMBER)
        {
            Velocity = (Owner->GetActorLocation() - PreviousLocation) / DeltaSeconds;
        }
        bIsInAir = Owner->GetActorLocation().Z > GroundZ + 8;
    }
    else
    {
        bIsInAir = false;
    }
    PreviousLocation = Owner->GetActorLocation();
    bHasPrevious = true;
    Speed = Velocity.Size2D();
    bIsMoving = Speed > 5;
    bIsRunning = Speed > 300;
    bIsDescending = bIsInAir && Velocity.Z < 0;
}
