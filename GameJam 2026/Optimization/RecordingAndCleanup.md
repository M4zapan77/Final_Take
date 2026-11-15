# Recording storage and conservative cleanup — M7

The scene clock, event timestamps, 20 Hz moving poses, initial poses and exact final
endpoints remain authoritative. Stationary actor samples are compacted only in the
six-turn story mode. The reusable recorder keeps its original fixed-grid default.
A hold boundary before movement prevents interpolation from starting motion early.
Each persistent actor owns its appearance and animation configuration once; the
six temporary movement segments no longer duplicate those presentation snapshots.
Prop ownership remains an event and unowned moving props retain sampled trajectories.

Removed a duplicate refresh of the manager-owned objective HUD and limited physical
prop recovery traces to their existing sampling cadence. No NPC AI framework or
new runtime dependency was introduced. Test drivers are excluded from Shipping.

The first regression run exposed a stop-time precision bug: an event just after a
fixed-grid sample could exceed the final duration by a few ULPs. The exact endpoint
is now retained. The corrected Editor build and all 66 automated tests passed.

## Measurements and limits

Raw JSON and complete-scene reports are retained with the M7 tests. Compare the
same Failure scenario at 1280×720, DX12 SM6, Development Editor game, offscreen,
without sound. This is an idle-heavy movie, deliberately useful for measuring
stationary recording storage; it is not a representative action benchmark.
Frame-time means include state switches and capture outliers. These results do
not establish Shipping, audio, visible-presentation or minimum-hardware performance.
No claim of a general FPS increase or guaranteed 60 FPS is made.

## Audit policy

`AssetReferencesBeforeCleanup.json` includes hard/soft/management dependencies,
referencers, native/config/script roots (including directory loads), import sources
and script/plugin dependencies. Approved `/Game/OneManStudio` assets are protected.
Expanding dynamic directory roots reduced the initial 96 candidates to 16; the 80
mannequin assets remain available for retargeting/fallback workflows.

The reviewed candidates are template door/jump-pad/target/prototype resources with
no referencers outside their candidate set. They total 2,210,671 bytes. Separately,
45 ZIP/RAR input copies total 1,736,161,593 bytes; extracted FBX/OBJ/textures used
for reimport remain in SourceArt. The recovery tool validates absolute paths and
backs up every exact file with CRC and SHA256 before removing it. The archive is
outside the project and all generated caches. See CleanupReceipt for actual status.

GameplayStateTree has no project asset/script/native dependency in the audit.
Only that explicitly enabled unused gameplay plugin is eligible for disabling;
required media codecs, character animation, camera and editor tooling stay intact.

Final cache cleanup occurs only after the final package is verified. User settings,
evidence, original sources and useful runtime pipeline caches are retained.
