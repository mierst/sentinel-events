# First slice: admission and recovery specification

Status: technical planning baseline, 2026-09-05. Implements the accepted direction
in [design.md](design.md); no runtime capability is claimed here.

## Deliverable and boundary

A private multiplayer feasibility build admits two consenting test players while
a third continues ordinary survival. It validates the roster, records origins,
replaces kits, safely places and freezes entrants, releases a synchronized
countdown into a short protected demonstration, then returns them. Cancellation,
disconnect, and process-restart paths must work before widening the slice.

This is a rehearsal harness, not a playable BR. No public matchmaking, combat,
corpse looting, prizes, shrinking-zone enforcement, spectator assets, or outsider
vehicle relocation ships in this slice. Those remain required v1 roadmap work.
Use an empty private test area; an outsider entering that area aborts the rehearsal
without moving or mutating that outsider. Do not call this full arena isolation.

**Global constraints:**

- One active rehearsal; disabled by default and enabled explicitly on a private test server.
- Client/server mod, no backend, paid service, or third-party mod dependency.
- Server identity, consent, inventory, permissions, and state are authoritative.
- No inventory mutation without current-run affirmative consent and verified preflight.
- No original-inventory escrow or restoration; preserve untouched originals on abort.
- No per-frame script polling; use a 250 ms coordinator timer in this slice.
- Source prefix `Sev`; modded-class fields use `m_Sev`/`s_Sev`; ASCII Enforce sources.
- One modded hook block per owned class, with the vanilla chain preserved.
- No public server deployment or Workshop publishing from this plan.

## Concrete prototype settings

These are engineering starting defaults under the owner's authorization to resolve
technical details, not claims of previously specified numerical requirements.

| Setting | Default / bound | Purpose |
| --- | --- | --- |
| Enabled | false | Explicit test-server activation |
| AdminIds | empty stable-ID allowlist | No implicit admin privilege |
| ReadySeconds | 120; 30-600 | Time to stash and accept |
| MinimumPlayers | 2; 2-8 | No one-player win/admission shortcut |
| MaximumPlayers | 8; minimum-8 | Bound prototype work |
| PreparationTimeoutSeconds | 30; 5-120 | Abort stalled preparation |
| CountdownSeconds | 10; 3-60 | Common server release deadline |
| DemonstrationSeconds | 15; 5-60 | Protected movement test, then return |
| CombatQuietSeconds | 60; 30-300 | Admission denied after dealt/received player damage |
| CoordinatorTickMs | 250, fixed in prototype | Bounded phase work |
| SpawnRadiusMeters | 50; 20-200 | Candidate disk around configured center |
| MinimumSeparationMeters | 10; 5-50 | Prevent stacked entrants |
| SpawnAttemptsPerPlayer | 32, fixed | Bounded failure instead of unsafe fallback |
| CandidateChecksPerTick | 8, fixed | Spread preflight work |
| ClientStateRefreshSeconds | 1 | Countdown recovery; transitions also push state |
| ReturnSearchRadiusMeters | 25; 5-100 | Safe alternative near recorded origin |

Center and fallback are required world positions, with no synthetic defaults such
as `0 0 0`. Invalid coordinates or insufficient safe candidates disable admission.
The default harness kit and return clothing are `TShirt_White`, `Jeans_Blue`, and
`AthleticShoes_Black`, with no weapons or cargo. Validate class availability against
the loaded server config before offering ready; record actual test classnames.
Return clothing replaces combat/rehearsal clothing only for already-stripped
entrants. Untouched entrants keep all original possessions.

## Identity, time, and command boundaries

Use the authenticated server-side stable player ID as the session key. A client
payload never supplies an authoritative recipient ID or arbitrary inventory list.
Session key: run ID + stable player ID; add a server-generated admission token
unique to this entry. Never key durable records on a reusable entity/network ID.

The private test entry point is `/event rehearse`, authorized against AdminIds.
It opens a ready offer to the current private-test players. `accept`/`decline` are
UI responses with run ID and current offer revision. `/event cancel` is admin-only.
Public scheduling/signup/editor flows belong to roadmap stage 3, not this harness.

The server closes ready at `now >= deadline`, validates, and prepares the roster
together. Late acceptance does not extend deadlines. Duplicate acceptance is an
idempotent acknowledgement; stale run/revision and unauthorized requests do nothing.
At each boundary recheck connected/alive/not restrained/not unconscious/on foot,
combat-quiet history, and whether the player already has an unresolved session.
Missing combat-history observation after restart requires the full quiet interval.

Use monotonic server time for live durations. Persist wall timestamps for audit,
but never resume an interrupted countdown from wall time after restart. At start
of recovery, abort unfinished rehearsals. A 1-second state refresh is presentation
only; client clock/acknowledgement cannot authorize release or inventory mutation.

## Admission state and mutation boundaries

Per-player preparation states:

`OFFERED -> ACCEPTED -> VALIDATED -> INTENT_RECORDED -> MUTATING -> PREPARED -> RETURNING -> RETURNED`

`DECLINED`, `EXCLUDED`, and `RECOVERY_REQUIRED` are separate outcomes. Event phases
remain as described in the parent design; this harness replaces Active combat
with a protected demonstration phase.

1. Freeze the accepted roster at the deadline. Resolve safe spawn and return
   fallback for every eligible entrant before mutating any entrant.
2. Recheck roster count and eligibility. An excluded player is untouched; abort
   all if fewer than minimum remain. Fix the roster once mutation begins.
3. Capture each entrant's position/orientation at the entry boundary, before
   any event teleport. Persist intent with that origin and a unique token.
4. Apply a participant-only server interaction/damage guard and client input
   restriction. Recheck eligibility under the guard; a race aborts admission.
5. Correlate the intent with a character-persisted token and explicit preparation
   receipt. Prove the marker/save ordering in the persistence experiment before
   allowing destruction. A write return value is not a durability guarantee.
6. Remove originals recursively, including hands, clothing, attachments, and
   nested cargo. Never drop them into the world. Sequence clearing, dressing,
   and state normalization across callbacks; verify each stage's actual result.
7. Move to the reserved validated position, normalize health, issue the harness
   clothing, and record PREPARED only after server verification. All delayed work
   validates the token/phase and live character before acting.
8. When all are prepared, start one server countdown. Keep movement, attacks,
   inventory transfers, and incoming damage restricted during preparation and
   countdown. During protected demonstration release movement only; disallow
   outgoing damage and transfers, retain protection, and contain entrants in the
   test area. This is explicitly not final combat behavior.
9. End demonstration or cancel: strip only proven rehearsal inventory of mutated
   entrants, apply minimal return clothing, and safely return each to its origin.
   Never apply that stripping branch to an untouched entrant.
10. Persist the character's return token/state through the experimentally validated
    character-save path before recording final journal completion and releasing
    the guard. A journal-only receipt is not proof that the return persisted.

All movement/action/transfer/protection requirements above are prototype gates,
not assertions of a native all-input freeze. Test each separately. Client input
exclusions improve presentation but never establish authoritative enforcement.

Two participants cannot be mutated atomically as a group. "Together" means one
validated fixed roster and one release deadline. Any failure after the first
mutation aborts the rehearsal for everyone; previously destroyed originals are
not restored. Tell affected players the event ended and return them safely.

## Persistence experiment and recovery gate

Proposed storage under `$profile:SentinelEvents/`: `config.json`, `runs/<run-id>/`,
and per-session generation records. Write new generation files without truncating
the last verified generation. Include schema version, sequence, run/token/player
identity, phase, origin, mutation receipt, return destination/receipt, and integrity
metadata. Use a manifest listing the intended session records before any mutation.
Record the server/game build with evidence. No live identities appear in git logs.

Read back and validate writes and retain earlier generations; reject malformed,
missing, incompatible, or ambiguous state. Do not invent a JSON transaction,
atomic rename, fsync guarantee, or character-save acknowledgement from script APIs.
Generation selection and game persistence are separate problems to investigate.

The critical experiment correlates journal records and character-saved tokens
across actual process termination at each boundary. Establish whether replay can
distinguish untouched originals, issued rehearsal inventory, and an already
returned character that has resumed survival. If it cannot, mark the affected
session RECOVERY_REQUIRED and keep automatic destructive replay disabled. An
admin-visible diagnostic/manual recovery path is required for ambiguity; do not
resolve it by deleting arbitrary current inventory or silently forgetting return.

| Observed state | Required recovery action |
| --- | --- |
| No consent or excluded/declined | No character mutation |
| Verified intent, proof mutation never began | Preserve originals; release guard |
| Verified mutated session on matching character | Remove rehearsal gear, return with minimal clothing |
| Session journal incomplete/corrupt or token mismatch | Quarantine only affected session; no guessed stripping |
| Verified return receipt matches saved character | Do not repeat stripping or teleport after normal play resumed |
| Character dead/replaced with unresolved return | Preserve obligation by identity; reconcile replacement before interaction |
| Participant offline | Keep pending return past event cleanup; reconcile on reconnect |
| Whole server shutting down/restarting | Abort rehearsal, never derive a winner from disconnect order |

While blocked, prevent event-item leakage and do not grant unrestricted play on a
possibly corrupted event character. Unrelated players remain unaffected. Design
the quarantine UX with the actual reconnect hook ordering; if the engine cannot
establish that gate reliably, the destructive harness stays disabled.

## UI and transport contracts

Client shows event name, ready deadline, irreversible gear warning, Accept and
Decline. Disable acceptance in a vehicle client-side for clarity; server rechecks
are mandatory. Server responses show accepted/waiting, excluded reason, preparation,
countdown, demonstration, return, or recovery-needed. Small attribution footer:
`Powered by Sentinel` and a discreet `dayz.fyi` reference.

Logical RPC operations are OFFER, READY_RESPONSE, STATE, and ADMIN_REQUEST. Pick
and document a noncolliding numeric range after inspecting loaded test mods.
Bound payload length and requests per sender (initial limit: 2/second, burst 4).
Validate direction, authenticated sender, schema, run/revision, and phase before
allocation or actions. Disconnect invalidates callbacks tied to the old entity.
Do not rely on hidden server code in a publicly downloaded PBO for authorization.

Spawn candidates use area-uniform radius sampling, minimum separation, terrain
height, water/slope checks, and character-sized clearance. Physics predicates are
an engine adapter that needs actual tests on slopes, shorelines, roofs/interiors,
and obstacles. No retry loop may silently accept an invalid candidate at its cap.
Return search first tests saved world position/orientation, then nearby safe points,
then the prevalidated fallback. Never reconstruct a vehicle seat or teleport a
vehicle as a side effect. Refuse admission if fallback safety cannot be established.

## Acceptance evidence

Private multiplayer: A and B accept; C does not. A/B enter after the window,
wear the test kit, remain restricted through countdown, move during the protected
demonstration, then return. C's inventory, spawn, damage, and movement remain normal.

Negative cases: no response, decline, stale/replayed offer, nonadmin start/cancel,
ready in driver/passenger seat, entering vehicle after acceptance, restraint,
unconsciousness, recent player damage, invalid clothing class, missing fallback,
blocked/water spawn, insufficient roster, and outsider entering the test area.

Fault matrix: terminate during intent write, character marker/save, first deletion,
kit issue, teleport, PREPARED write, countdown, return teleport, character return
save, and final receipt write. Also interrupt callbacks with disconnect and death.
For each record disk generations, character/token state, inventory before/after,
origin/destination, and observed recovery without publishing real player identity.

Include a named partial-roster failure: A has lost originals and is prepared; B
fails before clearing. A returns in minimal clothing, B keeps originals, and the
whole rehearsal ends. Record connect/new/ready/reconnect callback order and attempt
movement, inventory transfer, and world interaction at the earliest client frame;
no interaction may precede quarantine/recovery for an unresolved session.

The slice passes only with actual dedicated-server boot and client observations,
verified private-server recovery, and bounded timer/query cost. Any inconclusive
destructive replay remains a gate, not an assertion of exactly-once behavior.
