#include "StudioAudioProfile.h"
#include "ActingComponent.h"

void UStudioAudioProfile::ConfigureActing(UActingComponent* Acting) const
{
    if (!Acting) { return; }
    for (auto& Sound:Acting->Sounds)
    {
        if (!Sound.Sound)
        { if (const auto* Default=Soundboard.FindByPredicate([&](const auto& Item){return Item.Identifier==Sound.Identifier;})) { Sound=*Default; } }
    }
    if (Acting->Footsteps.IsEmpty()) { Acting->Footsteps=Footsteps; }
    if (!Acting->DefaultAttenuation) { Acting->DefaultAttenuation=ActorAttenuation; }
    for (auto& Gesture:Acting->Gestures)
    {
        if (Gesture.Identifier==ActingIds::Gesture(EActingGesture::Faint) && !Gesture.ImpactSound && !FaintImpacts.IsEmpty())
        { Gesture.ImpactSound=FaintImpacts[0]; Gesture.ImpactDelaySeconds=1.3f; }
    }
}
void UStudioAudioProfile::ConfigureFlow(FStudioFlowSettings& Settings) const
{
    if (!Settings.MenuMusic) { Settings.MenuMusic=FlowAudio.MenuMusic; }
    if (!Settings.IntroductionMusic) { Settings.IntroductionMusic=FlowAudio.IntroductionMusic; }
    if (!Settings.WardrobeMusic) { Settings.WardrobeMusic=FlowAudio.WardrobeMusic; }
    if (!Settings.RecordingMusic) { Settings.RecordingMusic=FlowAudio.RecordingMusic; }
    if (!Settings.PremiereMusic) { Settings.PremiereMusic=FlowAudio.PremiereMusic; }
    if (!Settings.ResultsMusic) { Settings.ResultsMusic=FlowAudio.ResultsMusic; }
    if (!Settings.BankAmbience) { Settings.BankAmbience=FlowAudio.BankAmbience; }
    if (!Settings.CinemaAmbience) { Settings.CinemaAmbience=FlowAudio.CinemaAmbience; }
    if (!Settings.ClapperSound) { Settings.ClapperSound=FlowAudio.ClapperSound; }
    if (!Settings.ButtonSound) { Settings.ButtonSound=FlowAudio.ButtonSound; }
    for (auto& Cue:Settings.Announcements)
    {
        if (!Cue.Sound)
        {
            if (const auto* Default=FlowAudio.Announcements.FindByPredicate([&](const auto& Item){return Item.Cue==Cue.Cue;}))
            { Cue.Sound=Default->Sound; Cue.DurationSeconds=FMath::Max(Cue.DurationSeconds,Default->DurationSeconds); }
        }
    }
}
void UStudioAudioProfile::ConfigurePremiere(FMoviePremiereSettings& Settings) const
{
    if (!Settings.PremiereMusic) { Settings.PremiereMusic=PremiereAudio.PremiereMusic; }
    if (!Settings.ResultsMusic) { Settings.ResultsMusic=PremiereAudio.ResultsMusic; }
    if (!Settings.PremiereSting) { Settings.PremiereSting=PremiereAudio.PremiereSting; }
    if (!Settings.ResultsSting) { Settings.ResultsSting=PremiereAudio.ResultsSting; }
    if (!Settings.ButtonSound) { Settings.ButtonSound=PremiereAudio.ButtonSound; }
}
void UStudioAudioProfile::ConfigureCatalog(TArray<FCosmeticDefinition>& Catalog) const
{
    for (auto& Item:Catalog)
    {
        if (Item.Id==TEXT("Prop.MoneyBag") && !Item.PickupSound && !MoneyPickups.IsEmpty()) { Item.PickupSound=MoneyPickups[0]; }
        if (Item.Id==TEXT("Prop.Donut") && !Item.UseSound) { Item.UseSound=DonutBite; }
    }
}
