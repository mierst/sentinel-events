# Guard and safe-location feasibility probes

Date: 2026-09-05. This is a non-destructive implementation probe. It does not
admit players, strip inventory, teleport characters, or authorize recovery.
Destructive entry remains disabled. Dedicated-server script compilation and pure
fixtures cannot establish native movement, transfer, damage, or physics safety.

**Live movement authority FAILED.** The server input-controller candidate was
withdrawn after a connected-client trial showed more than 120 m displacement and
the user reported uncontrolled movement toward the ocean. `BeginMovementDiagnostic`
now returns false unconditionally. It cannot access a player/controller or install
movement overrides; guard release no longer contains movement-controller writes.
`MovementProven()` remains false. No replacement is enabled.

## Contracts and boundaries

`SevHarnessConfig` has explicit absent center/fallback arrays, an empty Steam64
admin allowlist, and disabled offers by default. Validation rejects invalid
settings; bounded loading never rewrites an existing operator file. Internal
session identity remains `GetId()`; admin comparison uses server `GetPlainId()`.

`SevSpawnPlanner` creates incremental requests. Only READY exposes a position;
INVALID, PENDING, and EXHAUSTED never return a usable result. Each request retains
its attempt count, caps at 32, and consumes at most eight candidates per advance.
The caller must use the shared `AdvanceTick` helper once per 250 ms coordinator
tick, for at most 16 requests (arena and return for eight entrants). No coordinator
is implemented here. Return attempt 1 tests the origin;
attempts 2-31 search nearby, and attempt 32 validates the fallback itself.

`SevEntryGuard` is a token/identity/live-character scoped server probe. Incoming
damage state is captured once and restored on matching release; duplicate acquire
does not recapture the guard's own protection. A stale release cannot unlock
another token or character. A released token is not reusable. No guard is applied
to ordinary players merely because this package is loaded.

The process-local registry retains at most 256 token tombstones and at most eight
active identity leases; exhaustion fails new acquisition. This is a diagnostic
bound, not durable session recovery. A replacement character does not inherit
these side effects automatically. The missing reconnect/replacement quarantine
gate remains explicit. Released tokens cannot act on a later character.

## Native source seams

References are paths in the extracted vanilla script tree, used as interface
evidence rather than copied implementation:

- `scripts/3_Game/human.c:7-25,230-237`: input-controller disabled flag and movement
  overrides have setters but no state getters. The former diagnostic applied
  SetDisabled and speed/angle overrides using a supplied baseline. That candidate
  failed the live trial and all of those movement writes, including release-time
  writes, have been removed. The baseline data type remains only for caller and
  fixture compatibility; it does not authorize a controller operation.
- `scripts/3_Game/Entities/Object.c:1184-1192`: observable `GetAllowDamage` and
  `SetAllowDamage`. Another protection writer during a held lease is an unresolved
  ownership conflict; this probe only preserves the captured preexisting value.
- `scripts/4_World/Classes/Weapons/WeaponManager.c:77-88`: `CanFire` pre-fire veto.
- `scripts/4_World/Entities/ManBase/DayZPlayer/DayZPlayerMeleeFightLogic_LightHeavy.c:73-82`:
  `CanFight` veto before fight handling.
- `scripts/4_World/Classes/UserActionsComponent/ActionBase.c:912-917`: action veto
  checks actor, target hierarchy, and main-item hierarchy. Existing actions require
  interruption separately (`ActionManagerServer.c:20-33`).
- `scripts/4_World/Entities/DayZPlayerImplementThrowing.c:125-140`: cancellation
  of an existing throw is a transition side effect, not proof of all throw paths.
- `scripts/4_World/Systems/Inventory/DayZPlayerInventory.c:2820-3054`: source,
  destination, swap, and drop validators were source candidates for preserving
  native rejection/repair paths. However, the actual 1.29 dedicated server rejected
  the World `modded class DayZPlayerInventory` block with "Engine class cannot be
  modded". That block was removed. **Server non-hands transfer authority is
  unimplemented and remains a blocking gate.** Hands and action hooks do not prove
  cargo, attachment, swap, drop, magazine or crafting coverage.
- The same inventory file at `1767-1785` constructs request validation locally and
  assigns `InventoryMode.JUNCTURE`; that mode is not deserialized from a client.
  `scripts/3_Game/Systems/Inventory/Hand_Events.c:38-108` supplies the separate hands
  veto and source/destination getters. The veto applies to server, non-remote
  JUNCTURE operations. Trusted direct `HandEvent(SERVER)` retains native behavior;
  no mutable global bypass flag or client permission exists.
  `HandEventBase` must be modded in its defining Game module. A small Game policy
  interface forwards actor and all four locations to the World participant service,
  installed before the first guard's transition effects.
- `scripts/3_Game/Global/Game.c:1162-1182,1312` and
  `scripts/3_Game/Global/DayZPhysics.c:199` supply terrain/water/normal, geometry
  clearance, and contact-ray probes. `World.c:85` supplies the map bound.

All retained unguarded overrides delegate to `super`. Denied request hooks return false at
the native decision seam, preserving the surrounding failure/repair handling.
Existing `PlayerBase` and `MissionServer` blocks are extended; character creation
and save/load chains are not replaced.

Client input exclusions are a separate local `MissionGameplay` diagnostic, requiring
the guarded local character, a known unused baseline for movement/aiming/menu
groups, and an exclusive caller during the test. Those native groups are shared
and not reference counted. It is unsafe to infer ownership from a missing group
getter or to remove someone else's restriction. The caller must explicitly end
the trial only when that exclusive baseline still holds. Normal guard release
does not unconditionally reset these groups. Client inventory affordances use a
synced participant flag; the server always consults its own registry.
The local input diagnostic also fences start/end by token and retains bounded
token tombstones, so a stale trial callback cannot end a later trial on the same
character.

The terrain adapter admits conservative open terrain only: a dry center and four
footprint edges, a normal with vertical component at least 0.94, bounded height
variation, a ground contact ray, and a 1 by 1.8 by 1 m geometry clearance box. Each
candidate performs at most five footprint checks, one terrain normal query, one
contact ray, and one box query. The saved-origin attempt rejects a validated height
that differs by more than 0.5 m, rather than silently accepting terrain below a
roof. Interiors, puddles/water volumes not reported by surface APIs, navigation
reachability, terrain box collision semantics, and occupied/moving locations still
need actual trials. The predicate does not reserve world space against outsiders.

## Verification record

The lead ran the native dedicated server against the packaged RED stubs. The
2026-09-05 22:53 run contained all 44 initial GuardSpawn fixtures and completion
marker; positive contract cases failed as expected, while the prior 45 fixtures
passed. No native script error occurred. The first GREEN attempt at 23:00 failed
World compilation because the `HandEventBase` modded hook was placed in World
instead of its defining Game layer. That failed boot is not a fixture or gameplay
pass. The corrected candidate uses the Game/World bridge described above.
The 23:09 native run then rejected the DayZPlayerInventory extension as an engine
class that cannot be modded; removing that unsupported hook permits further
diagnostics but does not satisfy the inventory transfer requirement.
Two further compiler corrections replaced compound vector-index assignments with
explicit assignments and matched a fixture subclass constructor to its inherited
signature. These failures are retained as part of the native verification history.

Local validation command: `./tools/build.ps1` (source checks plus MakePbo packaging).
Fixture suite: `GuardSpawn`; final marker: `[SEV] GuardSpawn fixtures complete`.
Expanded fixture cases cover all configured defaults/bounds, loaded default
clothing classes, unchanged invalid configuration, damage/token lease policy,
pure surface rejection, area-uniform sampling, persistent cap/budget behavior,
fallback failure, and known-baseline requirements. Native GREEN results follow
below. The hands-policy fixtures distinguish blocked JUNCTURE from preserved
SERVER/remote paths; these are direct scripted calls, not network authority tests.

At 23:14 on 2026-09-05, the actual DayZ 1.29 dedicated server compiled the corrected
package and passed **82/82 GuardSpawn**, plus **11/11 Admission**, **9/9 Recovery**,
and **25/25 Store** fixtures, with no native script errors. The worker independently
ran `tools/check-fixtures.ps1` for all four suites against the lead's captured
`guard-live-20260905-231435-104/fixture-evidence.log`. MakePbo 2.16 / DePbo DLL 10.21
packaged the artifact; packaging itself was never treated as compilation evidence.

- Tested PBO SHA256: `F215B3D2A139CF67C34AED4C1987BF6AD1CD4DCE2DBA566BB51942B5A647A9C0`.
- Captured fixture log SHA256: `4800887912AAAEA2253C65A9A6995E36ACE0205C7769921147AA70D1FBAB025D`.

The server was retained by the lead for connected-client diagnostics. At that
checkpoint no connected-client Task3 result was claimed. **Task3 acceptance remains
incomplete**, including the unimplemented non-hands transfer authority gate.

## Failed connected-client movement trial

The subsequent `p/g1` private diagnostic used artifact SHA256
`83B720895C75CC6CDE6EDF2A9E0B9675185A56CF0220BB74F52ACAA20ADA4E9A`.
The server log recorded guard acquisition and movement-diagnostic activation as
true, followed by these measured displacements from the anchor:

| Elapsed | Displacement |
| --- | --- |
| 5 seconds | 20.5126 m |
| 20 seconds | 82.2243 m |
| 60 seconds | 120.205 m |

The user reported being dragged toward the ocean. This fails the freeze gate;
the evidence does not distinguish movement caused by the override sequence from
movement that the sequence failed to contain. The reported isolated baseline was
an assumption, not native state readback. At 60 seconds the log recorded release
true and allowDamage true, but neither proves restored control, a safe position,
or correct release under other preexisting protection. The lead stopped the test
server. No restoration or server-authority pass is claimed.

Failed-trial log SHA256:
`B393C94534E93048EBDC1203FAD2CAB550AEF87EF92B472A656917C9DF464F73`.
Its source file was `p/g1/script_2026-09-05_23-28-26.log`; no player identity or
character coordinates are reproduced here.

The immediate source backout preserves the callable diagnostic signature and
returns false without side effects. Stored movement baselines and controller
restoration writes were removed. Static checks verify an unconditional false
body and absence of SetDisabled/OverrideMovementSpeed/OverrideMovementAngle calls
in SevEntryGuard. A replacement requires design and evidence review before any
new connected-client execution; it is not part of this backout.

The lead's source-withdrawal artifact passed native compilation and all 149
fixtures in `p/withdraw1`, with no native script errors. The worker independently
checked GuardSpawn87 and the captured log hash. This verifies compilation and
unchanged fixture behavior, not restored live control or a replacement freeze.
No connected-client trial was run for the withdrawn path.

- Withdrawal artifact SHA256: `5E5B437181ADCDF88DDC98165BCD2186654C05726FCAB6D0E116E741DA4032CC`.
- Withdrawal log SHA256: `668A613CC7DDC5D559CCF507B4E2FAD29812B320B7DD7CD3E0172E691285392B`.

## Unrun acceptance gates

Independent review found an additional hands completion gap in the original
82-fixture candidate: vanilla `HandEventTake.CanPerformEventEx` returns true for
`m_IsJuncture=true` before calling its base (`Hand_Events.c:238-242`). The initial
generic HandEventBase fixture did not exercise that inherited path. A take
accepted before guard acquisition can reach acknowledged completion afterward.
New fixtures call the actual HandEventTake subtype with that flag, a denying
policy, and SERVER/remote/unguarded controls. This direct-method test performs no
inventory event and cannot establish client correction or juncture cleanup.
The lead's native RED run reproduced both missed-veto/policy-call failures with
all three controls passing and no script errors. A narrow Game-layer HandEventTake
hook now checks that completion before super using the shared server/non-remote/
JUNCTURE predicate. It checks the actor and source: vanilla Take's destination is
the actor's hands, so that owner is already included. Initial requests keep their
existing native/base path, and trusted SERVER/remote calls retain super behavior.

The corrected source passed native compilation and **87/87 GuardSpawn** fixtures
in `store-hands-green-20260905-232528-958` (2026-09-05 23:25), including the actual
Take completion cases. The combined run passed 149 fixtures across all four suites
with no native script errors. The worker independently checked GuardSpawn and the
captured log hash; the lead owned artifact building and native execution.

- Review-fix artifact SHA256: `83B720895C75CC6CDE6EDF2A9E0B9675185A56CF0220BB74F52ACAA20ADA4E9A`.
- Review-fix log SHA256: `B89380F3A92A8DBE50D1EC9055D41D14DBC8CFB366BCD66AF2D48C4D79352EFA`.

The native server's surrounding failed-condition path serializes the event,
removes movable overrides, sends destination repair, and keeps the transaction
unsuccessful (`DayZPlayerInventory.c:1204-1244,1298-1346`). Source inspection does
not prove that rejecting an already acknowledged transaction leaves the client
and inventory juncture consistent. The controlled completion-after-acquisition
trial remains **NOT RUN**, separately from non-hands transfer coverage.

Beyond the failed movement trial above, the following cases remain **NOT RUN**:

- The full walking/sprinting/jumping, stance, ladder, held-input transition, slopes,
  latency, and server/observer displacement with server-only and combined guards.
- Firearm modes, mid-burst activation, fists/melee/finisher, outgoing projectile
  attribution, and already-started actions or throws.
- Cargo/hands/attachment drop, nested containers, swaps, quickbar, split/combine,
  crafting, reload/magazines, outsider transfers, and stale request replay.
- Vehicle entry and seat transitions, disconnect/reconnect and replacement
  character timing, duplicate/stale release on live characters.
- Incoming firearm/melee/explosion/environment/fall/attachment damage, with and
  without preexisting damage protection.
- Terrain slopes, shoreline/puddle edges, roofs/interiors, obstacles, moving
  entities, and post-preflight changes. Geometry clearance is a candidate predicate,
  not a navigation/pathfinding or future occupancy guarantee.
- One ordinary observer receiving no restrictions; full two-participant plus
  outsider isolation requires three connected clients and remains a release gate.

An inability to stop movement or transfers keeps destructive entry disabled.
Periodic teleport correction is not implemented and would not prove a freeze.
Task2 character-save/crash/legacy/inter-mod gates remain unresolved independently.
