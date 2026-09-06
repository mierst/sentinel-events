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

## 2026-09-05 - Reviewed boot tooling and non-destructive persistence probes

The bootstrap and its packaging safeguards passed independent review. The build
uses explicit source/layout allowlists, rejects private-key and state files, and
checks reparse-point ancestry before staging or output mutation. An isolated boot
runner now captures a fresh run manifest tying the owned process, server/PBO
hashes, and immutable fixture log together. Focused process doubles reproduced
and verified fixes for finalization failures with a retained server and for a
process exit racing fixture completion. Only the runner's own process is stopped.

The session journal, pure recovery decisions, and character correlation probes
are implemented and independently reviewed for the non-destructive scope. DayZ
server 1.29.163709 passed 11 Admission, 9 Recovery, and 25 Store fixtures with no
script errors. The successful run's PBO SHA-256 is
`E9A0C60D706C316CA2F84FA42D0459A1E692DEF545F599B3E6C542C7F5814BA0`.
The corresponding immutable log SHA-256 is
`3E1501C85BCB1F9F61575C3D1D8448FB446928687315EF38BE210C3422D6E68F`.
Full contracts and explicit unrun tests are in `evidence/persistence.md`.

The journal bounds records to 16 KiB, manifests to eight sessions, and retained
history to 32 generations per session. Generated canonical serialization and an
integrity chain reject incomplete, inconsistent, or newer corrupt data without
falling back to an older state. This is not an atomic transaction with character
storage. Deletion of an entire latest suffix still needs an independent witness;
character markers and save/load compatibility have not been proven by boot tests.
A deliberately truncated JSON test initially emitted a native parser error despite
correct rejection. A root-envelope check now rejects that torn suffix before
parsing, preserving strict rejection of unexpected script errors in test logs.

Only one real client is currently available. Character save/load, legacy/inter-mod
compatibility, disconnect/restart boundaries, and multi-client isolation remain
unrun gates. Inventory destruction and automatic destructive recovery stay disabled.
Continue with non-destructive guard and spawn probes while those gates are open.

Before implementing the spawn planner, replaced its ambiguous synchronous Resolve
proposal with explicit incremental requests and statuses. Eight candidate checks
are shared across each coordinator tick; attempts persist and stop at 32. This
prevents treating a pending search as a successful or exhausted preflight. Clarified
that owner-facing administrative grants use Steam64 strings, while internal session
keys use the authenticated hashed identity; neither uses display names.

## 2026-09-05 - Native guard probes and first live marker roundtrip

Added bounded configuration, token-scoped non-destructive guards, and incremental
safe-location probes. Native compilation exposed two unsupported hook placements:
HandEventBase belongs in Game, and DayZPlayerInventory cannot be modded by the
native runtime. The latter hooks were removed; non-hands transfer authority remains
unimplemented. Independent review then found a Take completion path bypassing the
base hands hook. A narrow subtype hook now rechecks it, with real event-class RED
and GREEN fixtures. Native transaction correction remains unproven.

A real 44-character player identity exposed a session-directory path failure that
short synthetic IDs missed. Manifest schema 2 now resolves exact immutable members
to p0-p7 folders; old manifests retain their original layout for read/append. Record
and character marker schemas remain 1. Native regression verification passed all
149 fixtures, including the long-profile case. Both fixes passed independent code
review; this does not close gameplay or destructive-recovery gates.

One real client completed an untouched diagnostic marker save, normal logout/load,
and server-process restart/load. The loaded marker matched the journal both times;
item/clothing class names remained present. The restart run could not rewrite the
marker. No inventory mutation or recovery was performed. This establishes a narrow
marker roundtrip, not an atomic save contract or crash-at-transition recovery.
See evidence/persistence.md and evidence/guard-and-spawn.md for scope and hashes.
The marker server was stopped after evidence capture; the separate movement trial
is next. Full multiplayer isolation still needs more than the one available client.

## 2026-09-05 - Live server movement override failed

The first connected-client movement trial acquired the guard successfully but did
not immobilize the character. The server recorded displacement of 20.5126 m after
5 seconds, 82.2243 m after 20 seconds, and 120.205 m after 60 seconds. The tester
reported being dragged toward the ocean. The timer called Release successfully,
but restored control was not established; the lead stopped the isolated server.
No inventory stripping, teleport, or event admission ran during this probe.

The server-side HumanInputController disable/speed/angle override sequence is a
failed mechanism and must not be reused for admission. Its diagnostic entry is
being disabled. A replacement requires new source investigation, a bounded test
with immediate abort, and live verification before destructive work can proceed.
This trial does not establish inventory-transfer or outgoing-damage protection.
