#include "Testing/ActingRecorderTestActor.h"

#include "ActingRecorderComponent.h"
#include "ActingSceneClockSubsystem.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "ReplayActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogActingRecorderTest, Log, All);

AActingRecorderTestActor::AActingRecorderTestActor()
{
    PrimaryActorTick.bCanEverTick = true;
    ReplayActorClass = AReplayActor::StaticClass();
}

void AActingRecorderTestActor::BeginPlay()
{
    Super::BeginPlay();
    TryBindInput();
}

void AActingRecorderTestActor::TryBindInput()
{
    if (TestController.IsValid() || !GetWorld())
    {
        return;
    }
    APlayerController* Player = GetWorld()->GetFirstPlayerController();
    if (Player && Player->IsLocalController())
    {
        EnableInput(Player);
        InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AActingRecorderTestActor::HandleRecordKey);
        InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AActingRecorderTestActor::HandleStopKey);
        InputComponent->BindKey(EKeys::P, IE_Pressed, this, &AActingRecorderTestActor::HandleRepeatKey);
        TestController = Player;
    }
}

void AActingRecorderTestActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    TryBindInput();
    if (bShowInstructions && GEngine)
    {
        FString CurrentStatus = Status;
        if (IsValid(Recorder) && Recorder->IsRecording())
        {
            CurrentStatus = FString::Printf(TEXT("Recording take 1: %.2f / %.2f seconds"),
                Recorder->GetRecordedDurationSeconds(), RecordingDurationSeconds);
        }
        else if (IsValid(ActiveReplay))
        {
            CurrentStatus = ActiveReplay->IsPlaying()
                ? FString::Printf(TEXT("Take 1 replay: %.2f / %.2f seconds. You control the second pawn."),
                    GetWorld()->GetSubsystem<UActingSceneClockSubsystem>()->GetSceneTimeSeconds(), SavedRecording.GetDurationSeconds())
                : TEXT("Replay finished; final pose held. P repeats the replay. R starts a new take.");
        }
        GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.1f, FColor::Cyan,
            FString::Printf(TEXT("WASD move | Mouse look | Space jump | R record | Enter stop early | P repeat replay\n%s"), *CurrentStatus));
    }
}

void AActingRecorderTestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(Recorder))
    {
        Recorder->OnRecordingFinished.RemoveDynamic(this, &AActingRecorderTestActor::OnTakeRecorded);
    }
    if (IsValid(ActiveReplay))
    {
        ActiveReplay->Destroy();
    }
    if (GEngine)
    {
        GEngine->RemoveOnScreenDebugMessage(GetUniqueID());
    }
    Super::EndPlay(EndPlayReason);
}

void AActingRecorderTestActor::HandleRecordKey() { StartTest(); }
void AActingRecorderTestActor::HandleStopKey() { StopAndReplay(); }
void AActingRecorderTestActor::HandleRepeatKey() { RepeatReplay(); }

bool AActingRecorderTestActor::StartTest()
{
    TryBindInput();
    APlayerController* Player = TestController.Get();
    APawn* Pawn = Player ? Player->GetPawn() : nullptr;
    if (!IsValid(Pawn) || (IsValid(Recorder) && Recorder->IsRecording())
        || !FMath::IsFinite(RecordingDurationSeconds) || RecordingDurationSeconds <= 0.0)
    {
        return false;
    }

    UActingRecorderComponent* NewRecorder = Pawn->FindComponentByClass<UActingRecorderComponent>();
    if (!NewRecorder)
    {
        NewRecorder = NewObject<UActingRecorderComponent>(Pawn);
        Pawn->AddInstanceComponent(NewRecorder);
        NewRecorder->RegisterComponent();
    }
    if (IsValid(Recorder))
    {
        Recorder->OnRecordingFinished.RemoveDynamic(this, &AActingRecorderTestActor::OnTakeRecorded);
    }
    Recorder = NewRecorder;
    StartTransform = Pawn->GetActorTransform();
    StartControlRotation = Player->GetControlRotation();
    RecordingPawn = Pawn;
    Recorder->OnRecordingFinished.AddUniqueDynamic(this, &AActingRecorderTestActor::OnTakeRecorded);
    if (!Recorder->StartRecording(RecordingDurationSeconds))
    {
        Recorder->OnRecordingFinished.RemoveDynamic(this, &AActingRecorderTestActor::OnTakeRecorded);
        Status = TEXT("Could not start recording. Check the pawn transform and sample rate.");
        return false;
    }

    if (IsValid(ActiveReplay))
    {
        ActiveReplay->Destroy();
        ActiveReplay = nullptr;
    }
    GetWorld()->GetSubsystem<UActingSceneClockSubsystem>()->StopClock();
    Status = TEXT("Recording take 1.");
    return true;
}

bool AActingRecorderTestActor::StopAndReplay()
{
    return IsValid(Recorder) && Recorder->StopRecording();
}

void AActingRecorderTestActor::OnTakeRecorded(const FActingRecording& InRecording)
{
    // Copy before possession/destruction: the delegate argument belongs to the old recorder.
    SavedRecording = InRecording;
    APlayerController* Player = TestController.Get();
    APawn* FirstPawn = RecordingPawn.Get();
    UActingSceneClockSubsystem* Clock = GetWorld()->GetSubsystem<UActingSceneClockSubsystem>();
    if (!Player || !IsValid(FirstPawn) || Player->GetPawn() != FirstPawn || !Clock
        || !SavedRecording.IsValid() || !ReplayActorClass)
    {
        Status = TEXT("Take captured, but replay setup failed. Check possession and replay class.");
        UE_LOG(LogActingRecorderTest, Warning, TEXT("%s"), *Status);
        return;
    }

    FActorSpawnParameters ReplayParameters;
    ReplayParameters.Owner = this;
    ReplayParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AReplayActor* NewReplay = GetWorld()->SpawnActor<AReplayActor>(ReplayActorClass, StartTransform, ReplayParameters);

    FActorSpawnParameters PawnParameters;
    PawnParameters.Owner = Player;
    PawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    UClass* PawnClass = SecondPawnClass ? SecondPawnClass.Get() : FirstPawn->GetClass();
    APawn* SecondPawn = GetWorld()->SpawnActor<APawn>(PawnClass, StartTransform, PawnParameters);
    if (!NewReplay || !SecondPawn)
    {
        if (NewReplay) { NewReplay->Destroy(); }
        if (SecondPawn) { SecondPawn->Destroy(); }
        Status = TEXT("Could not spawn the replay or second pawn. See Output Log.");
        return;
    }

    Player->Possess(SecondPawn);
    if (Player->GetPawn() != SecondPawn)
    {
        Player->Possess(FirstPawn);
        NewReplay->Destroy();
        SecondPawn->Destroy();
        Status = TEXT("Could not possess the second pawn. See Output Log.");
        return;
    }
    Player->SetControlRotation(StartControlRotation);
    if (UPawnMovementComponent* Movement = SecondPawn->GetMovementComponent())
    {
        Movement->StopMovementImmediately();
    }

    Recorder->OnRecordingFinished.RemoveDynamic(this, &AActingRecorderTestActor::OnTakeRecorded);
    Recorder = nullptr;
    RecordingPawn.Reset();
    FirstPawn->Destroy();
    ActiveReplay = NewReplay;
    Clock->StartClock(0.0);
    ActiveReplay->PlayRecording(SavedRecording, 0.0);
    Status = FString::Printf(TEXT("Take 1 replaying (%d samples, %.3f s). You control a NEW pawn at the original start. P repeats; R records again."),
        SavedRecording.Samples.Num(), SavedRecording.GetDurationSeconds());
    UE_LOG(LogActingRecorderTest, Display, TEXT("%s"), *Status);
}

bool AActingRecorderTestActor::RepeatReplay()
{
    if (!IsValid(ActiveReplay) || !SavedRecording.IsValid() || (IsValid(Recorder) && Recorder->IsRecording()))
    {
        return false;
    }
    UActingSceneClockSubsystem* Clock = GetWorld()->GetSubsystem<UActingSceneClockSubsystem>();
    return Clock && Clock->StartClock(0.0) && ActiveReplay->PlayRecording(SavedRecording, 0.0);
}
