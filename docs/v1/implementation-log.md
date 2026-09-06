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

The product recommendations are accepted. The admission specification and six-task
execution plan are written (see latest entry). Resume at task 1 of
`docs/superpowers/plans/2026-09-05-admission-feasibility.md`; do not repeat D01-D12
product questions. Keep untested engine claims distinct from accepted behavior.

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

## 2026-09-05 - Accept remaining recommendations

Owner instruction: "please accept all the recommendations you presented that I
didn't comment on". Marked previously presented recommendations accepted while
preserving explicit user choices. The latest recommendation resolves conflicting
earlier proposals: transport everyone after the ready window closes; return to
origin rather than normal survival respawn; ignore player-building placement vetoes.

Accepted healthy normalized entry, minimal spectator/return clothing, vehicle
admission blocking and recheck, one active BR, optional platforms/podium,
configurable shrinking zone and max duration, no-winner final ties, abort/recovery
on restart, and cancellation stripping before durable winner settlement. Retain
settled winner inventory and awards when closing the presentation. Accepted
two-minute podium and 24-hour claim defaults, claim commands, reusable templates,
standalone client/server packaging, local admin identities, and discreet attribution.

The earlier discussion explicitly recommended shrinking-zone configuration and
no winner with multiple survivors at the final deadline; incorporated these into
the baseline rather than leaving them open because the initial draft omitted them.

Updated design, roadmap, README, and contributor guidance. Remaining work is
technical specification/prototype evidence, including outsider vehicles, durable
delivery, UI/RPC contracts, and concrete configuration values not previously
recommended. This update implements no gameplay and makes no runtime claims.

## 2026-09-05 - First-slice technical specification and execution plan

Continued the accepted planning work. Added `admission-spec.md`, vanilla
`source-notes.md`, and a six-task execution plan at
`docs/superpowers/plans/2026-09-05-admission-feasibility.md`.

Selected bounded prototype defaults: 120-second ready window, minimum 2/maximum 8
entrants, 10-second countdown, 15-second protected demonstration, 60-second combat
quiet requirement, 250 ms coordinator, 50 m spawn disk, 10 m separation, and
bounded candidate search. These are technical defaults under standing direction,
not changes to the accepted public event experience. Full scheduling/editor,
combat/looting, claims, platforms, and outsider enforcement remain later stages.

Source inspection confirms the developer teleport helper explicitly targets an
occupied transport, ClearInventory is server guarded, and OnDisconnect runs before
the normal disconnect save. JSON SaveFile provides no visible atomic coupling to
character persistence; UI input exclusions do not prove server-authoritative freeze.
The local source tree has no Git metadata. Execution must record game versions and
source hashes; no native guarantees are inferred from signatures alone.

Consequently the first rehearsal keeps public combat/rewards disabled and requires
non-destructive persistence and guard experiments before any stripping. Ambiguous
records must block automatic destructive replay instead of guessing which current
inventory to remove. Return obligations survive player entity replacement.

Planning artifacts only: no build, server run, destructive probe, or gameplay
code was executed. Documentation/source-contract review and local link/diff checks
are the validation for this step. Next task: build/boot harness and pure policy.

## 2026-09-05 - Implementation started and native admission checks

The owner authorized full implementation and preparation for a v0.0.1 publish.
Work proceeds in an isolated release branch. Publication is not part of this step;
all accepted BR requirements and runtime safety gates remain binding.

The initial standalone PBO was loaded by DayZ dedicated server 1.29.163709.
A fixture-only build first failed on the deliberately missing admission type.
The first implementation attempt exposed an Enforce line-continuation syntax
error, which was corrected. The subsequent fresh run compiled Game, World, and
Mission and passed all 11 admission decision fixtures; the log checker also
reported 11/11. These checks exercise pure decisions, not actual player admission,
vehicle extraction, inventory mutation, or recovery. No participant was stripped
or teleported during these boot tests.

Added the standalone organizer/designer/event-admin permission contract. The owner
assigns roles by stable identity in server configuration; each request must be
checked on the server. Permission UI and runtime enforcement remain implementation
work. Generated an Events Workshop icon using the public Deathmatch icon as a
style reference; texture conversion and package integration remain pending.
