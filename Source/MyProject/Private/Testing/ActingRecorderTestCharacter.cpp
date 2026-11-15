#include "Testing/ActingRecorderTestCharacter.h"

#include "ActingRecorderComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "UObject/ConstructorHelpers.h"

AActingRecorderTestCharacter::AActingRecorderTestCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = false;
    bUseControllerRotationYaw = false;
    AutoPossessAI = EAutoPossessAI::Disabled;
    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0, 540.0, 0.0);
    GetCharacterMovement()->MaxWalkSpeed = 450.0f;

    ActingRecorder = CreateDefaultSubobject<UActingRecorderComponent>(TEXT("ActingRecorder"));
    PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
    PlaceholderBody->SetupAttachment(RootComponent);
    PlaceholderBody->SetRelativeScale3D(FVector(0.7, 0.7, 1.6));
    PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PlaceholderBody->SetGenerateOverlapEvents(false);
    PlaceholderBody->SetCanEverAffectNavigation(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded())
    {
        PlaceholderBody->SetStaticMesh(Sphere.Object);
    }

    // A small nose makes live facing direction visible without an animation asset.
    UStaticMeshComponent* Nose = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacingMarker"));
    Nose->SetupAttachment(RootComponent);
    Nose->SetStaticMesh(Sphere.Object);
    Nose->SetRelativeLocation(FVector(40.0, 0.0, 35.0));
    Nose->SetRelativeScale3D(FVector(0.35, 0.18, 0.18));
    Nose->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Nose->SetGenerateOverlapEvents(false);
    Nose->SetCanEverAffectNavigation(false);

    CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
    CameraArm->SetupAttachment(RootComponent);
    CameraArm->TargetArmLength = 500.0f;
    CameraArm->TargetOffset = FVector(0.0, 0.0, 60.0);
    CameraArm->bUsePawnControlRotation = true;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
}

void AActingRecorderTestCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    if (NewController)
    {
        FRotator View = NewController->GetControlRotation();
        View.Pitch = -25.0;
        NewController->SetControlRotation(View);
    }
}

void AActingRecorderTestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &AActingRecorderTestCharacter::LookYaw);
    PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &AActingRecorderTestCharacter::LookPitch);
    PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Released, this, &ACharacter::StopJumping);
}

void AActingRecorderTestCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const APlayerController* Player = Cast<APlayerController>(GetController());
    if (Player && IsLocallyControlled())
    {
        const FRotator Yaw(0.0, GetControlRotation().Yaw, 0.0);
        const float Forward = (Player->IsInputKeyDown(EKeys::W) ? 1.0f : 0.0f)
            - (Player->IsInputKeyDown(EKeys::S) ? 1.0f : 0.0f);
        const float Right = (Player->IsInputKeyDown(EKeys::D) ? 1.0f : 0.0f)
            - (Player->IsInputKeyDown(EKeys::A) ? 1.0f : 0.0f);
        AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Forward);
        AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Right);
    }
}

void AActingRecorderTestCharacter::LookYaw(float Value)
{
    AddControllerYawInput(Value);
}

void AActingRecorderTestCharacter::LookPitch(float Value)
{
    AddControllerPitchInput(-Value);
}
