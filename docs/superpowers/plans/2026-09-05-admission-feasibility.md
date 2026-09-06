# Admission and Recovery Feasibility Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce a private, guarded multiplayer rehearsal that demonstrates
consented admission, safe placement, kit replacement, countdown, and reliable return.

**Architecture:** Pure admission/recovery decisions live separately from engine
adapters. One coordinator owns the roster and transition sequence; player sessions
and persisted receipts carry recovery obligations across entity lifetimes. Destructive
work remains disabled until the input/recovery experiments pass.

**Tech Stack:** DayZ Enforce Script, client layouts, profile JSON, PowerShell build
and evidence tools, installed DayZ dedicated server and two or more test clients.

**Spec:** [Admission specification](../../v1/admission-spec.md), with
[accepted v1 design](../../v1/design.md) and [source notes](../../v1/source-notes.md).

## Global constraints

- One active rehearsal; disabled by default and enabled explicitly on a private test server.
- Client/server mod, no backend, paid service, or third-party mod dependency.
- Server identity, consent, inventory, permissions, and state are authoritative.
- No inventory mutation without current-run affirmative consent and verified preflight.
- No original-inventory escrow or restoration; preserve untouched originals on abort.
- No per-frame script polling; use a 250 ms coordinator timer in this slice.
- Source prefix `Sev`; modded-class fields use `m_Sev`/`s_Sev`; ASCII Enforce sources.
- One modded hook block per owned class, with the vanilla chain preserved.
- No public server deployment or Workshop publishing from this plan.

The complete v1 is not implemented by this plan. Later roadmap stages remain
required. File names and APIs below are new design contracts, not existing code.
Native adapters are implementation probes: report observed limitations instead of
inventing successful runtime guarantees. Do not copy vanilla/other mod code.

## Ownership and dependency map

| Task | New files | Depends on |
| --- | --- | --- |
| 1: boot and pure admission | `config.cpp`, `$PBOPREFIX$`, `scripts/3_Game/SevAdmission.c`, `scripts/5_Mission/SevMissionServer.c`, `scripts/5_Mission/tests/SevAdmissionTests.c`, `tools/build.ps1`, `tools/check-source.ps1`, `tools/check-fixtures.ps1` | none |
| 2: records and non-destructive persistence probe | `scripts/3_Game/SevSessionRecord.c`, `scripts/3_Game/SevRecovery.c`, `scripts/4_World/SevSessionStore.c`, `scripts/4_World/SevPlayerHook.c`, `scripts/5_Mission/tests/SevRecoveryTests.c`, `docs/v1/evidence/persistence.md` | 1 |
| 3: admission guard and safe location probes | `scripts/4_World/SevEntryGuard.c`, `scripts/4_World/SevSpawnPlanner.c`, `scripts/3_Game/SevHarnessConfig.c`, `scripts/5_Mission/tests/SevSpawnTests.c`, `docs/v1/evidence/guard-and-spawn.md` | 1-2 |
| 4: ready UI and server protocol | `scripts/3_Game/SevNet.c`, `scripts/4_World/SevCoordinator.c`, `scripts/5_Mission/SevMissionGameplay.c`, `scripts/5_Mission/gui/SevReadyMenu.c`, `layouts/sev_ready.layout`, `layouts/sev_countdown.layout` | 1-3 |
| 5: staged mutation and safe recovery | `scripts/4_World/SevEntryTransaction.c`, `scripts/4_World/SevReturnService.c`, `scripts/5_Mission/tests/SevTransactionTests.c` | 1-4 and probe gates |
| 6: multiplayer interruption evidence | `tools/run-private-test.ps1`, `docs/v1/evidence/admission-matrix.md`, `docs/testing.md` | 1-5 |

Tasks may modify previously created hook/coordinator/test-registration files.
Serialize those edits. Independent source review or pure fixtures can be delegated;
do not run two writers on the hook blocks. Public source omits private profile data.

## Task 1: Bootable package and pure admission policy

**Produces:** `SevEligibility` fields `Connected`, `Alive`, `Unconscious`,
`Restrained`, `InVehicle`, `QuietSatisfied`, `HasPendingSession`; `SevAdmission`
static methods `CanAccept(SevEligibility e, bool currentOffer, bool beforeDeadline)`
and `CanPrepare(int eligibleCount, int minimumCount, bool arenaValid, bool kitsValid)`.

- [ ] Create the PBO metadata and a single MissionServer OnInit hook that calls
  super and registers fixtures. Prefix `SentinelEvents`; `$PBOPREFIX$` contains
  that exact value. Declare Game/World/Mission script modules and client-required
  loading; keep the package standalone. Keep prototype configuration disabled.
- [ ] Add PowerShell build/source/fixture tools. `build.ps1` resolves MakePbo from
  PATH or `-MakePboPath`, packages only mod metadata/scripts/layouts into
  `build/@SentinelEvents/addons/sentinel_events.pbo`, fails on tool errors, and
  never includes profile state/private keys. `check-source.ps1` rejects non-ASCII
  `.c` source, leading-plus string continuation, and duplicate owned hook blocks.
- [ ] Add fixtures before implementing policy. Use parseable exact lines
  `[SEV] fixture <name>: expected=<value> got=<value> PASS|FAIL` and register the
  expected names below. A missing fixture is a failure, not an empty success.

```c
SevEligibility e = new SevEligibility();
e.Connected = true;
e.Alive = true;
e.QuietSatisfied = true;
bool valid = SevAdmission.CanAccept(e, true, true); // expected true
e.InVehicle = true;
bool occupied = SevAdmission.CanAccept(e, true, true); // expected false
bool tooFew = SevAdmission.CanPrepare(1, 2, true, true); // expected false
```

  Cases: eligible, vehicle, unconscious, restrained, recent-combat, pending-session,
  stale-offer, deadline-equality, below-minimum, invalid-arena, invalid-kit.
- [ ] Build and boot on a private dedicated server with `-mod=@SentinelEvents`;
  establish failing/missing-policy output first, then implement this decision logic:

```c
static bool CanAccept(SevEligibility e, bool currentOffer, bool beforeDeadline)
{
    if (!e || !currentOffer || !beforeDeadline) return false;
    return e.Connected && e.Alive && !e.Unconscious && !e.Restrained
        && !e.InVehicle && e.QuietSatisfied && !e.HasPendingSession;
}

static bool CanPrepare(int eligibleCount, int minimumCount, bool arenaValid, bool kitsValid)
{
    return minimumCount >= 2 && eligibleCount >= minimumCount && arenaValid && kitsValid;
}
```

- [ ] Run `tools/check-source.ps1`, build, boot, and
  `tools/check-fixtures.ps1 -LogPath <actual-script-log> -Suite Admission`.
  Record resolved binary paths/build numbers and actual log path in `docs/testing.md`
  (created initially here, completed in task 6). Expected: all 11 named cases PASS,
  no script compilation errors, no gameplay changes when disabled.
- [ ] Commit the boot/policy slice with its evidence. Do not call a PBO build alone
  a passed test. No runtime entry mutation exists at this stage.

## Task 2: Versioned session records and persistence experiment

**Consumes:** admission policy. **Produces:** `SevSessionRecord` with `SchemaVersion`,
`Sequence`, `RunId`, `PlayerId`, `Token`, `Phase`, `Origin`, `OriginOrientation`,
`MutationReceipt`, `ReturnReceipt`, `ReturnPosition`; `SevSessionStore.WriteNext`
and `.LoadLatest`, both returning bool and an error string; `SevRecovery.Decide`.
Store `Origin`/orientation/return position as vectors; IDs/receipts/phases as strings;
schema/sequence as integers. Validate every field before use.

- [ ] Implement pure recovery fixtures against this decision table, first with
  `Decide` returning `BLOCK` so positive cases fail. The enum constants are
  `UNTOUCHED`, `NEEDS_RETURN`, `DONE`, `BLOCK`.

```text
Decide(recordValid=false, tokensMatch=true, mutationProven=true, returnProven=false) = BLOCK
Decide(true, false, true, false) = BLOCK
Decide(true, true, false, false) = UNTOUCHED only when no-mutation proof is present
Decide(true, true, true, false) = NEEDS_RETURN
Decide(true, true, true, true) = DONE
```

  Exact signature: `static int Decide(bool recordValid, bool tokensMatch,
  bool mutationProven, bool returnProven, bool untouchedProven)`. Missing or
  contradictory evidence always returns BLOCK. Do not infer untouched merely
  because `mutationProven` is false.
- [ ] Implement generation files and strict readback/schema validation. Keep the
  previous generation; never truncate the last usable record in place. Candidate
  method contracts: `bool WriteNext(SevSessionRecord record, out string error)` and
  `bool LoadLatest(string runId, string playerId, out SevSessionRecord record,
  out string error)`. Reject unsafe path characters, contradictory generations,
  missing manifest sessions, and unbounded file sizes. A newer corrupt generation
  triggers BLOCK rather than trusting an older state for destructive replay.
- [ ] Add the sole PlayerBase hook block. Probe versioned token/receipt persistence
  via OnStoreSave/OnStoreLoad, preserving super and actual vanilla signatures.
  No inventory removal. Add connect/reconnect/disconnect diagnostics with cached
  stable identity and redacted public evidence. Use a compile-time diagnostic
  switch for fault injection; do not expose fault controls in production RPCs.
- [ ] Probe writes/marker saves with process termination on an explicitly selected
  private test process. Record journal and character state before/after each stop:
  before write, partial/new generation, after readback, before/after player save,
  and before/after return receipt. Also load a truncated JSON and token mismatch.
- [ ] Implement the decision table, run Recovery fixtures, and record
  `docs/v1/evidence/persistence.md`: actual outcome, source/binary versions,
  observable guarantees, and unproven boundaries. Add check-fixtures suite names.
  Gate: if the engine cannot distinguish untouched/mutated/returned, task 5's
  destructive path remains disabled; do not guess from missing data.
- [ ] Commit store/probe work and its evidence. Do not claim generation JSON makes
  inventory and Hive persistence atomic.

## Task 3: Guard and safe-location probes without stripping

**Consumes:** records and eligibility. **Produces:** `SevEntryGuard.Acquire(PlayerBase
player, string token)`, `.Release(PlayerBase player, string token)`,
`.IsHeld(string token)`; `SevSpawnPlanner.Resolve(vector center, float radius,
array<vector> reserved, out vector position)`, `.ResolveReturn(vector origin,
vector fallback, out vector position)`; bounded `SevHarnessConfig` from spec.

- [ ] Add config validation with every default/bound in admission-spec. Missing
  center/fallback/admin identities disables offers. Never overwrite an operator's
  invalid file with defaults. Implement pure candidate/separation fixtures before
  wiring physics: zero-radius rejection, shoreline rejection, obstacle rejection,
  minimum-distance rejection, attempt cap, and fallback failure.
- [ ] Implement sampling and a native physics adapter. Core sampling contract:

```text
candidate.x = center.x + sqrt(u) * radius * cos(theta)
candidate.z = center.z + sqrt(u) * radius * sin(theta)
candidate.y = validated terrain/contact height
accept only if dry, walkable, clear, and distance >= minimum from each reserved point
after 32 attempts for one entrant: return false, never accept the last invalid candidate
```

  `u` is uniform [0,1], theta uniform [0,2*pi]. Check only eight candidates per
  coordinator tick; exact collision native calls are selected/tested here, not
  assumed from a terrain-height lookup. Keep source references in evidence.
- [ ] Implement token-scoped guard acquisition/release preserving the preexisting
  GetAllowDamage state. Test client input exclusions, server movement containment,
  outgoing action/damage suppression, and item transfers independently. Do not
  use developer-only SetInputSuppression as the shipped solution. Any added hook
  must preserve super and have an explicit participant-only contract.
- [ ] Run connected-client trials: walking/sprinting/jumping, firing/melee,
  inventory drop/transfer, vehicle seat transitions, incoming damage, disconnect
  and duplicate/stale release. One ordinary player receives none of these guards.
  Test the same profile with and without preexisting damage protection.
- [ ] Record `guard-and-spawn.md`, including hook ordering and safe-area predicate
  limitations. Gate: inability to stop a guarded entrant moving or transferring
  items disables destructive entry; no periodic teleport-only claim of a secure
  freeze. Commit the verified adapter and fixtures.

## Task 4: Ready UI and guarded coordinator

**Consumes:** config, admission, guard, spawn and store. **Produces:**
`SevCoordinator.StartRehearsal(PlayerIdentity sender)`,
`.Respond(PlayerIdentity sender, string runId, int revision, bool accept)`,
`.Cancel(PlayerIdentity sender)`, `.Tick(float nowSeconds)`; client ready/countdown UI.

- [ ] Define the four bounded RPC envelopes from admission-spec in SevNet.c.
  Inspect the loaded test mods' ranges, choose one collision-free range, and
  document it in source. Resolve identity from sender, not body; reserve authority
  checks for the server. Add per-sender rate counters before expensive parsing.
- [ ] Add pure protocol/coordinator fixture cases: nonadmin start, stale run,
  stale revision, duplicate acceptance, late acceptance at equality, minimum after
  eligibility changes, cancel twice, and delayed callback from an earlier run.
  Stub `Respond` rejection first, observe valid-case failure, then implement:

```text
Respond(sender, run, revision, accept):
  if rate-limited or sender absent or wrong run/revision/phase: reject
  if now >= readyDeadline: reject
  if !accept: record declined; acknowledge; return
  eligibility = current server observation of sender's character
  if !CanAccept(eligibility, true, true): reject with reason
  record accepted exactly once; acknowledge waiting-for-ready-close
```

- [ ] Implement UI with full destruction warning, Accept/Decline, waiting state,
  authoritative countdown, exclusion/recovery reasons, and small attribution.
  A vehicle-occupied client gets an exit-vehicle prompt, but server verification
  still decides. No fake positive acceptance on a missing response.
- [ ] At deadline select and validate the roster/spawns, but in this task stop at
  the non-destructive preflight report. Push revisioned state at transitions and
  once per second. On reconnect resync the server state; old UI cannot reopen a run.
- [ ] Verify private clients see the same deadline, replayed requests do not change
  count, and absent/declined players remain untouched. Record protocol observations
  and commit. The full in-game template editor is not part of this slice.

## Task 5: Integrate staged mutation and return behind passed gates

**Consumes:** all previous contracts plus passed persistence and guard evidence.
**Produces:** `SevEntryTransaction.Begin(PlayerBase player, SevSessionRecord record)`,
`.Advance(string token)`, `.Abort(string token, string reason)`;
`SevReturnService.Begin(PlayerBase player, SevSessionRecord record)`,
`.Reconcile(PlayerBase player, string playerId)`.

- [ ] Add an engine-independent recording adapter for transaction fixtures. Its
  operations are WriteIntent, AcquireGuard, SaveMarker, ClearOriginals, IssueKit,
  Teleport, SavePrepared, ClearEventKit, IssueReturnClothes,
  SaveReturnMarkerAndCharacter, WriteReturnReceipt, ReleaseGuard.
  It records call order and permits failure at each operation. Use token/phase
  fencing for every continuation. Initial no-op transaction must fail ordering tests.
- [ ] Implement transitions against the spec; assert these required traces:

```text
success: preflight(all) -> intent(each) -> AcquireGuard -> SaveMarker -> ClearOriginals
         -> IssueKit -> Teleport -> prepared(all) -> common countdown -> protected demo
         -> ClearEventKit -> IssueReturnClothes -> return teleport
         -> SaveReturnMarkerAndCharacter -> WriteReturnReceipt -> ReleaseGuard
intent failure: no clear, no TP; abort untouched
guard failure: no SaveMarker or ClearOriginals; abort untouched
marker/save proof failure: no ClearOriginals; reconcile evidence before safe guard release
failure after first clear: abort whole roster; mutated entrants return; untouched keep originals
partial_roster: A PREPARED, B fails before clear -> A minimal clothes/origin; B originals intact
callback token mismatch: no engine operation
already returned proof: no clear, no TP, no new return clothing
corrupt or mismatched proof: BLOCK; no guessed clear or release into normal play
```

- [ ] Wire native inventory operations only after gates pass. ClearInventory is
  the candidate destruction primitive; verify actual hands/nested removal.
  Issue clothing with checked creation results across staged callbacks. Normalize
  required health/status using individually verified vanilla methods; confirm no
  healing/status change leaks to nonparticipants. Never force a vehicle exit.
- [ ] Save origin just before entry mutation and recheck safe position before TP.
  Abort if the fixed roster changes during preparation. Start countdown only when
  every entrant is verified prepared. Restore guards on abort; preserve unrelated
  protection instead of blindly setting damage allowed. Remove only known event
  clothing after mutation; final v1 item provenance is a separate later extension.
- [ ] Wire connect, reconnect, death/replacement, and disconnect recovery through
  the existing hook block. Cleanup before normal disconnect save, preserve super,
  and do not erase consent/origin/token merely because the player entity disappears.
  Unresolved sessions are gated from ordinary interaction until reconciled.
  Record OnConnect/new/ready/reconnect callback order on fresh and saved characters.
  Attempt movement, drop/transfer, and world actions from the earliest client frame;
  no window may precede quarantine. Passing a hook log alone does not pass this test.
- [ ] Run Transaction fixtures and ordinary multiplayer rehearsal. Run a duplicate
  end/cancel and a late callback after return. Record actual stage effects and
  recovery. Commit only with honest gate status; do not enable destruction by
  removing the checks because the prototype is difficult.

## Task 6: Reproducible private-server evidence and handoff

**Consumes:** completed prototype. **Produces:** test runner and evidence matrix.

- [ ] Implement `tools/run-private-test.ps1` taking explicit `ServerExe`,
  `ServerConfig`, `ProfilePath`, and mod path. Require a dedicated test profile,
  do not mutate production files or discover/stop arbitrary DayZ processes.
  Capture only its own PID; Start-Process uses hidden window. Fail on missing paths.
  Fault mode must require an explicit stage and owned test PID; no blanket kill.
- [ ] Execute two accepting clients plus one ordinary control player. Run all
  negative cases and eleven crash boundaries in admission-spec. Record each row:
  case, build, prestate, injection point, observed character/journal state, recovery,
  fixture/log evidence, outcome. Distinguish failed, inconclusive, and not run.
- [ ] Add negative assertions to check-fixtures: expected suite/fixture names must
  occur; any FAIL, compile error, or missing suite fails the command. Record log
  paths per run rather than searching an old profile for a convenient PASS.
- [ ] Measure coordinator/candidate checks for 2 and 8 admitted players plus the
  control player. Record sampling method, work count, timer delay, and cost. Do
  not infer live-load performance from boot fixtures. Confirm no per-frame loop.
- [ ] Finish docs/testing.md with exact commands, installed build versions,
  resolved paths, test fixture inventories, and limitations. Update README status
  only to what was demonstrated. Update roadmap and implementation log; commit
  the evidence with no raw private identities or server secrets.

Exit: review the admission specification against the completed matrix. Passing
permits planning the combat/arena/item-ownership slice; failure narrows the next
experiment. Neither result is a Workshop release or a claim full v1 is complete.

## Plan self-review

Coverage: consent/roster and deadlines (1,4); safe geometry and eligibility (3);
source/build evidence (1-3,6); persistence and quarantine (2,5,6); kit/health/TP
and recovery (5); private client UX (4); bounded performance and interruption
matrix (6). Later full-v1 requirements remain explicitly in roadmap stages 2-6.

Contract consistency: all Sev types/methods used by later tasks are declared above;
engine-native methods are candidate adapters, verified against source and runtime.
Code blocks are implementation guidance, not prevalidated Enforce compilation.
The concrete test log and process arguments are runtime inputs, not fabricated
machine paths or evidence. Documentation verification does not execute this plan.
