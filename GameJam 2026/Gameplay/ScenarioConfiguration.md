# Verified bank scenario and normal game flow

StudioGameMode now enables chronological gameplay by default. StudioLegacyMovie or disabling bUseChronologicalStory retains the temporary original mode. Standalone MovieManager defaults remain compatible with isolated legacy tests.

Create a Data Asset of class StoryScenarioData to author a compatible setup. Add it to MovieManager.ScenarioLibrary and select its stable Definition.Id with SelectedScenarioId. None chooses the first entry; an empty library uses Bank.ThePerfectRobbery. The definition is copied once at movie start, so editing the asset later cannot change an ongoing movie.

Expose/configure the scene anchor, three role offsets, teller/guard/two witness offsets, money/exit/secure positions, initial money owner/alarm/guard distraction, optional prop actions, contextual objective text, real outcome text and cinematic priorities. Validation rejects incompatible counts/actions/positions before spawning. Keep the known three role identities and four NPC slots. Outcomes always follow actual ownership/escape/security; wording overrides cannot invent another winner. Only one fully playable bank scenario is advertised.

The normal route was tested: HUB, script/role introduction after loading, wardrobe, six turns, two films, new movie and interactive HUB. Brief detention releases its input lock on scene-time timeout, Cut or exit. The teller cannot remotely retrieve dropped money. An already-distracted guard produces an appropriate alternative civilian objective.

Development Editor compiled and 65 automated tests passed, including independent scenario snapshots, invalid setup cleanup and detention input ownership. Native normal-flow police recovery, civilian betrayal with an actual throw, and six natural failed turns passed. Each movie played twice without changing scores/outcomes or resimulating props. Shipping/optimization and the newly supplied final artwork remain subsequent work.
