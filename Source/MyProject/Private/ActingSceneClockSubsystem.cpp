#include "ActingSceneClockSubsystem.h"

#include "Engine/World.h"

bool UActingSceneClockSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::GamePreview;
}

bool UActingSceneClockSubsystem::StartClock(double StartTimeSeconds)
{
    if (!GetWorld() || !FMath::IsFinite(StartTimeSeconds) || StartTimeSeconds < 0.0)
    {
        return false;
    }
    HeldTimeSeconds = StartTimeSeconds;
    EpochWorldTimeSeconds = GetWorld()->GetTimeSeconds();
    bRunning = true;
    return true;
}

double UActingSceneClockSubsystem::GetSceneTimeSeconds() const
{
    return HeldTimeSeconds + (bRunning && GetWorld()
        ? FMath::Max(0.0, GetWorld()->GetTimeSeconds() - EpochWorldTimeSeconds) : 0.0);
}

void UActingSceneClockSubsystem::PauseClock()
{
    HeldTimeSeconds = GetSceneTimeSeconds();
    bRunning = false;
}

bool UActingSceneClockSubsystem::PauseClockAt(double CompletedSceneTimeSeconds)
{
    if(!FMath::IsFinite(CompletedSceneTimeSeconds) || CompletedSceneTimeSeconds<0 || CompletedSceneTimeSeconds>GetSceneTimeSeconds()+1.e-6) { return false; }
    HeldTimeSeconds=CompletedSceneTimeSeconds; bRunning=false; return true;
}

void UActingSceneClockSubsystem::ResumeClock()
{
    if (!bRunning && GetWorld())
    {
        EpochWorldTimeSeconds = GetWorld()->GetTimeSeconds();
        bRunning = true;
    }
}

void UActingSceneClockSubsystem::StopClock()
{
    bRunning = false;
    HeldTimeSeconds = 0.0;
}
