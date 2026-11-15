# Physical props and chronological film

The story uses UGrabbablePropComponent on the existing money, donut, banana and one bank stool. The component creates reversible simple collision at runtime; it does not edit mesh assets. Structural doors, vault, tracks, camera equipment and cinema screen remain fixed. The selected first-round hand props become physical objects once; the second round never restores lost inventory.

SceneState.Objects is the only ownership source. E picks up a nearby eligible object, Q drops, left mouse throws, and T ends a turn early. Money has priority when a hand must be freed. DropObjectKey, ThrowObjectKey and EndTurnKey are editable on MovieManager. Per-object mass, throw speed, sockets, grip and visual references are editable on GrabbablePropComponent.

Bodies preserve velocity while the shared scene clock is paused between turns. Releases begin outside the holder capsule, use CCD and bounded speed, and avoid character physics pushing. Out-of-area objects recover inside the bank. Donuts distract the guard; significant impacts produce bounded NPC reactions and can make a money holder drop the bag. Distinct objects and NPC categories score once; cooldowns prevent repeated collision reactions.

Unowned prop trajectories sample near 20 Hz, compact idle spans, retain hold boundaries, and interpolate during the film. Ownership/creation/drop/throw/impact/recovery events use the same scene clock. Film bodies never simulate physics, and replay never grants points or changes the retained world state. Every replay resets its own ownership and event cursors.

Validation: 61 automated tests passed, including physical hold/throw/pause/restoration and compact trajectory interpolation. Native bank scenarios cover police recovery, civilian betrayal with an actual donut throw, and six failed natural ten-second turns. Each native run plays the movie twice and checks original outcomes/scores, final recorded prop transforms, disabled film physics, and a clean new movie. See the M3 JSON evidence for actual results. Human listening and final Shipping validation remain later milestones.
