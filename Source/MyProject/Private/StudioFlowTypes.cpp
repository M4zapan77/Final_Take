#include "StudioFlowTypes.h"

FStudioFlowSettings::FStudioFlowSettings()
{
    GameTitle=NSLOCTEXT("Studio","GameTitle","FINAL TAKE");
    MovieSynopsis=NSLOCTEXT("Studio","SynopsisStory","One actor. Three characters. Six turns in a bank. Play the robber, police officer and civilian twice, changing one shared story. The money and consequences stay where you leave them. Then watch what actually happened.");
    HowToPlay=NSLOCTEXT("Studio","TutorialStory","THREE CHARACTERS. TWO ROUNDS. SIX TURNS.\nChoose each character's costume once: up to 30 seconds, or press Ready. Every turn lasts up to 10 seconds. Round two continues with the same positions, outfits and inventory. Follow one contextual objective, improvise, and change the ending. The scene clock pauses between actors. After six turns, watch the chronological movie.\n\nMOVE: WASD   LOOK: Mouse   JUMP: Space\nE: interact / grab   Q: drop   LEFT CLICK: throw   T: Cut / end turn early\nGESTURES: 1 Dance | 2 Point | 3 Scared | 4 Faint | 5 Arrest\nSOUNDS: Z Laugh | X Scream | C Cry | V Siren | B Kiss | N Failure\nR: record a new movie (clears the old story).\n\nThe guard controls the exit: distract him to escape. Give recovered money to him as the police. A thrown donut can distract him too. Anyone can collect dropped money. Your actions change the next actor's situation.\n\nResults: Replay Movie keeps the same story and score. Record New Movie clears it. Even a failed robbery gets a premiere. Language and audio are available in Settings.");
    Credits=NSLOCTEXT("Studio","CreditsFinalTake","FINAL TAKE\nAn extremely small production.\n\nCreated for the Epic AI MegaJam 2026.\nMade with Unreal Engine 5.6.\n\nNo stunt doubles. No spare actors. Questionable directing.");
    const FText Lines[] = {NSLOCTEXT("FinalTakeUI","StudioFlowTypes.Lines.ca645a3ac2","Choose your costume!"),NSLOCTEXT("FinalTakeUI","StudioFlowTypes.Lines.bba1d2f5bf","Action!"),NSLOCTEXT("FinalTakeUI","StudioFlowTypes.Lines.630eb78647","Cut!"),NSLOCTEXT("FinalTakeUI","StudioFlowTypes.Lines.fecb10209b","Next actor!"),NSLOCTEXT("FinalTakeUI","StudioFlowTypes.Lines.6ade312205","That's a wrap!")};
    for (int32 Index=0;Index<5;++Index)
    {
        FDirectorAnnouncement& Item=Announcements.AddDefaulted_GetRef();
        Item.Cue=static_cast<EDirectorCue>(Index); Item.Text=Lines[Index];
    }
}

bool FStudioFlowSettings::IsValid() const
{
    for (double Value:{PageFadeSeconds,BetweenActorsSeconds,WrapSeconds,NextActorCueDelay})
    { if (!FMath::IsFinite(Value) || Value<0.0) { return false; } }
    for (float Value:{DefaultAudio.Master,DefaultAudio.Music,DefaultAudio.Effects,DefaultAudio.Ambient,DefaultAudio.UI})
    { if (!FMath::IsFinite(Value) || Value<0 || Value>1) { return false; } }
    TSet<EDirectorCue> Seen;
    if (!FMath::IsFinite(MusicFadeSeconds) || MusicFadeSeconds<.05f) { return false; }
    for (const auto& Item:Announcements)
    {
        if (Seen.Contains(Item.Cue) || !FMath::IsFinite(Item.DurationSeconds) || Item.DurationSeconds<0
            || !FMath::IsFinite(Item.Volume) || Item.Volume<0) { return false; }
        Seen.Add(Item.Cue);
    }
    return true;
}
