# Milestone 1 — six-turn chronological foundation

Implemented within the existing MovieManager via StorySessionComponent. Three persistent instances of the actual player pawn class, exactly two rounds/six turns, same controller/input and shared scene clock. Only first visits open wardrobe (default30s); each turn defaults10s, Cut(T) can finish early. Ready/timeout share the existing guarded transition. Inactive actors stop movement/input; return visits preserve transform and appearance. Initial capsules are grounded against actual static floor. Exact recorded boundaries eliminate frame overshoot.

Local movement/event segments retain global start/end offsets. Appearance/settings are stored once per cast member. Final montage builds three continuous actor recordings, holds inactive positions and offsets events onto the common timeline. Existing replay actors, cinema/presentation/results/reset are reused. No second clock or playback system.

During development this mode is opt-in (`bUseChronologicalGameplay` or `-FinalTakeStory`); default remains the protected legacy game until interactions and native playtests are verified. Native settings and Blueprint accessors expose turns/rounds/Cut. HUD only adds functional round/turn labels; artwork/layout preserved.

Verified: Development Editor succeeded; all55 automation tests passed (52 regressions +3 story tests). Real-bank separate-process run passed six turns/three first-only wardrobes, player input, Dance/Scream, early-cut duration, inactive positions, unique pawn possession, cinema events/appearance and clean new movie. Report `Tests/StoryPlayM1.json`; screenshots under `Screenshots/M1`.

Not implemented at this milestone: contextual money/objective state, NPCs, physical throw trajectories, directed camera cuts, localization or final Shipping package. No completion is claimed for those pending milestones.
