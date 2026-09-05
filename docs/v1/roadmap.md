# V1 delivery roadmap

Status: accepted product direction and delivery sequence, 2026-09-05. Read
[the design](design.md) first. Detailed first-slice implementation planning is
next; this is not a task-by-task execution plan. No gameplay milestone is implemented.

## 0. Settle the first design slice

- [x] Allow surviving contestants to loot eliminated contestants; only the
  winner's carried inventory exits (D01).
- [x] Accept ready/transport timing, configurable minimum roster, and partial preparation
  recovery (D02-D03).
- [x] Accept health/return clothing policy, vehicle entry rules, and restart/cancellation
  treatment before writing destructive lifecycle code (D04-D08).
- [x] Approve launch scope and standalone client/server direction (D09-D12).
- [ ] Resolve concrete first-slice defaults and API/recovery contracts from the
  design's implementation investigations; write a bounded execution plan.

The owner has accepted the product recommendations. Do not repeat product
approval questions. Next deliverable: concrete first-slice technical specification
and execution plan, preserving the accepted flow and identifying prototype gates.

## 1. Feasibility harness and participant lifecycle

Build the smallest dedicated-server/client harness that can admit two test
participants while a third remains a normal survival player. Exercise consent,
origin capture, safe spawn, staged kit replacement, freeze/release, and return.

Evidence required: in-engine compilation; nonparticipant inventory/respawn remain
normal; declining/timing out causes no mutation; vehicle entry is rechecked;
water/obstructed spawn candidates are rejected; all clients release on the same
server deadline. Interrupt each preparation stage and document actual recovery.

Gate: inability to return players or to isolate participant mutations blocks
expanding to a full match. A green packer does not pass this gate.

## 2. Arena isolation and elimination

Add owned arena enforcement, death/logout handling, roster accounting, and the
chosen corpse/item transfer rules. Validate outsider relocation 100 meters beyond
the boundary, fallback safety, environmental interactions, and platform feasibility.

Evidence required: foot crossings, player logout inside arena, driver/passenger
cases, supported aircraft/high-speed traversal, outside gunfire and explosives,
simultaneous final deaths, thrown/dropped items, nested containers, and corpse
cleanup. Verify a surviving contestant can loot a corpse, the original owner's
later disconnect does not delete transferred loot, spectators/outsiders cannot
take it, and only the winner's carried inventory survives final cleanup.
Test with the actual supported mod stack; do not generalize vanilla
results to all vehicle or damage mods.

Include shrinking-boundary progression, final-deadline no-winner resolution, and
boundary changes affecting outsiders; safe relocation must remain valid as the
combat boundary changes.

Gate: record the supported isolation envelope and any blocking incompatibility;
do not market a universal physical barrier from position-check evidence.

## 3. Scheduling, registration, and usable administration

Implement reusable templates and a server-authorized in-game editor over the
proven mechanics, with future scheduling, registration notifications, ready
consent, previews, minimum roster validation, live status, and cancellation.

Evidence required: registration closing at/before start, late/no responses,
duplicate commands, unavailable players, invalid kit classes, invalid geometry,
unauthorized requests, and edits to a template while a run is active. Verify
exactly which changes affect future runs and persist across restarts.

## 4. Spectating, winner podium, and return

Implement optional spectator role/platform, winner-only podium, no outgoing
damage, containment, voluntary exit, and persisted return obligations. Winner
inventory is retained; losers follow the approved clothing/cleanup policy.

Evidence required: platform-disabled completion, fall/escape attempts, outsider
access, no spectator combat/item interference, winner inventory intact, occupied
origin fallback, player death/respawn during return, disconnected spectators,
cancellation, and restart before platforms are removed.

## 5. Durable reward claim window

Implement winner entitlements, private claim UI, partial inventory delivery,
later claims, exact expiry, and durable recovery. Award state remains separate
from event/platform cleanup. Reward claims are a standalone capability of this mod.

Evidence required: zero/partial space, attachments and quantities, duplicate and
concurrent claims, invalid classes, expiry during claim, disconnect, and a forced
crash at each inventory/journal/save boundary. Verify both no duplication and no
silent entitlement loss. Test uncertain outcomes via reconciliation rather than
assuming a journal is sufficient.

Gate: reward crash consistency is a release blocker, not an optional hardening
task. Escalate the design if the prototype cannot substantiate the guarantee.

## 6. Release readiness

Run repeated full events under representative load and supported mod combinations.
Capture normal-survival regression results, lifecycle failures, and event cost.
Review configuration bounds, permissions/RPCs, cleanup ownership, persistent
recovery, and attribution placement. Add operator documentation and an actual
build/sign/publish checklist; review license status before public distribution.

Gate: client/server boot and multiplayer evidence, crash/claim tests, bounded
performance, and an explicit supported compatibility list. Repository setup does
not include a Workshop release or license finalization.

## Next iterations

After BR acceptance, write separate specs for DZE compositions and raid targets,
then convoy routes/objectives. Keep the shared core limited to demonstrated needs.
