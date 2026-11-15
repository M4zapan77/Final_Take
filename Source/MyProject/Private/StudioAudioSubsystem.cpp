#include "StudioAudioSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UStudioAudioSubsystem::SetLevels(FStudioAudioLevels NewLevels)
{
    const auto Clamp=[](float Value) { return FMath::IsFinite(Value)?FMath::Clamp(Value,0.f,1.f):0.f; };
    NewLevels.Master=Clamp(NewLevels.Master); NewLevels.Music=Clamp(NewLevels.Music); NewLevels.Effects=Clamp(NewLevels.Effects);
    NewLevels.Ambient=Clamp(NewLevels.Ambient); NewLevels.UI=Clamp(NewLevels.UI);
    bConfigured=true;
    if (Levels.Master==NewLevels.Master && Levels.Music==NewLevels.Music && Levels.Effects==NewLevels.Effects && Levels.Ambient==NewLevels.Ambient && Levels.UI==NewLevels.UI) { return; }
    Levels=NewLevels; const FStudioAudioLevels Stable=Levels; OnAudioChanged.Broadcast(Stable);
}
void UStudioAudioSubsystem::InitializeDefaults(const FStudioAudioLevels& Defaults) { if (!bConfigured) { SetLevels(Defaults); } }
UStudioAudioSubsystem* UStudioAudioSubsystem::Find(const UObject* Context)
{
    const UWorld* World=Context?Context->GetWorld():nullptr;
    UGameInstance* Instance=World?World->GetGameInstance():nullptr;
    return Instance?Instance->GetSubsystem<UStudioAudioSubsystem>():nullptr;
}
float UStudioAudioSubsystem::MusicMultiplier(const UObject* Context)
{
    const auto* Audio=Find(Context); return Audio?Audio->Levels.Master*Audio->Levels.Music:1.f;
}
float UStudioAudioSubsystem::EffectsMultiplier(const UObject* Context)
{
    const auto* Audio=Find(Context); return Audio?Audio->Levels.Master*Audio->Levels.Effects:1.f;
}
float UStudioAudioSubsystem::AmbientMultiplier(const UObject* Context)
{
    const auto* Audio=Find(Context); return Audio?Audio->Levels.Master*Audio->Levels.Ambient:1.f;
}
float UStudioAudioSubsystem::UIMultiplier(const UObject* Context)
{
    const auto* Audio=Find(Context); return Audio?Audio->Levels.Master*Audio->Levels.UI:1.f;
}
