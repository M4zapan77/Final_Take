# FINAL TAKE — GameJam 2026 progress

## Current verified gameplay

The game now defaults to three persistent characters, two rounds and six chronological turns. Wardrobe is first-only (30 seconds maximum); acting turns last up to 10 seconds with early Cut. Movement, gestures, sounds, physical props, NPC consequences, objectives and scores feed the actual recorded movie. The original simultaneous mode remains a temporary fallback. See Gameplay documents and Tests/Screenshots for evidence.

Protected Windows backup: `output/release/FINAL_TAKE_PreGameplayUpdate_2026-11-14/Windows` in the Codex workspace, outside project caches. ZIP `FINAL_TAKE_PreGameplayUpdate_Windows_2026-11-14.zip`, SHA256 `a834d542ce2d22dd5c687db90ab8bb4514cf93fd9088a42c356d2b987e6124f2`.

## Development status

1. Verified six-turn persistence, continuous shared clock and first-only wardrobe.
2. Verified contextual objectives, exclusive money ownership, reactive NPCs and 40/30/20/10 scoring.
3. Verified actual grab/drop/throw and deterministic recorded prop trajectories.
4. Verified camera/movement tuning and read-only cinematic direction inside the cinema.
5. Verified Spanish/English live localization and saved preference across process restart.
6. Verified configurable bank scenario and normal menu-to-movie-to-menu flow. Development Editor and 65 automated tests passed. Native police, civilian with actual throw, and failure movies each played twice with clean restart.
7. In progress: measured optimization, reference-checked cleanup, Shipping packaging and the newly supplied final UI artwork. These are not claimed complete.

Each milestone is checked before the next. The complete original backups remain protected outside caches. A recoverable six-turn gameplay package will be preserved before applying the newly approved final UI. No unrelated generated art or new maps are planned.

## Repository access

The user supplied [M4zapan77/Final_Take](https://github.com/M4zapan77/Final_Take). Source and verified evidence were pushed to main at 56bcd4faffa2b28130bac86b98fd804c83dcc06b from full-project local checkpoint c3b417734302430166b040d8eb335b7fae2b3655. The remote progress snapshot excludes multi-gigabyte art/media and generated/build files; complete assets remain in the local project and editable backups. LFS asset upload awaits account availability verification. The local full-project branch is final-take/gameplay-redesign; publication uses a separate source-only delivery branch and does not rewrite its history.
