# Sentinel Events planning and implementation log

## 2026-09-05 - Repository bootstrap and initial design draft

Owner created `mierst/sentinel-events` and requested setup, a license matching
Sentinel Deathmatch with innocuous attribution, and iteration on the v1 plan.

### Accepted direction carried from the planning conversation

- First mode: isolated BR within a live survival server; raid/convoy later.
- Separate scheduled registration, registration confirmation, stash warning,
  active ready acceptance, preparation, countdown, and combat.
- Players stash original belongings themselves; carried originals are destroyed.
- Safe random event spawns, assigned kits, frozen countdown, elimination on exit,
  and no gear retention through death/logout.
- Safely relocate outsiders at least 100 meters beyond the nearest arena border;
  avoid ocean/unsafe destinations. Ignore buildings when admins choose a location.
- Account for occupied vehicles at ready acceptance and immediately before TP.
- Optional protected spectator platform; protected winner podium above spectators.
- Winner keeps carried winnings; participants return to their pre-event origins.
- Cancelling an active event ends it without a winner.
- Reward claim window with private UI, partial claims, persistent outstanding
  rewards, and no automatic ground drops.
- Later DZE compositions share object-lifecycle needs with raid targets.

### Bootstrap decisions and evidence

The GitHub repository was empty. The current remote and local Deathmatch license
matched: interim all-rights-reserved/source-available terms with alpha playtest
grant, attribution, and no redistribution. Copy it with only the project name
changed; do not silently replace it with an OSI license or finalize new legal
terms. Deathmatch's existing vote/scoreboard layouts contain the discreet line
"Global stats - dayz.fyi/dm"; Events attribution placement is documented as intent,
not a claim that an Events statistics page exists.

Added repository overview, contributor/agent guidance, ignore rules, the v1 draft,
and a staged roadmap. No gameplay code, assets, PBO build, or Workshop publication
is part of this bootstrap. No other projects were changed.

### Resume here

Review `design.md` D02-D03: when accepted players enter and ready
deadline/minimum/partial preparation policy. D01 was resolved below. Then resolve the lifecycle decisions
before writing an executable first-slice implementation plan. Recommendations in
the draft are not owner approval; unresolved questions stay explicitly labeled.

## 2026-09-05 - D01: allow looting eliminated contestants

Owner confirmed that surviving contestants may loot eliminated players. Preserve
corpse equipment for active contestants, prevent recovery by eliminated players,
spectators, or outsiders, and retain only the winner's carried inventory at final
cleanup. Item ownership follows transfers rather than remaining tied solely to
the original kit recipient; that recipient's later disconnect must not delete
legitimately transferred loot. Reward entitlements remain a separate system.

Updated the design and roadmap acceptance evidence. Documentation only; no
gameplay implementation or in-engine test was performed. Next decision: D02,
ready acceptance versus transport timing.
