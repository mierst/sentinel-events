# Sentinel Events v1 design

Status: **Accepted product baseline, 2026-09-05.** The owner accepted all previously
presented recommendations not superseded by their explicit choices. Where earlier
recommendations conflict, the latest recommendation applies. Technical design and
feasibility work remain; this is not yet a task-by-task implementation specification.

## Product and scope

Run configurable events within an existing DayZ survival server. Battle royale
is the first complete mode; scheduling, event ownership, return handling, and
reward claims should support later modes without making BR a universal ruleset.

Accepted v1 scope: one active BR event, reusable in-game templates, timed
registration and ready checks, kit replacement, safe random spawning, countdown,
boundary enforcement, elimination, optional spectator platform, winner podium,
return to origin, persistent reward claims, and cancellation/recovery. Multiple
simultaneous events are outside v1. DZE compositions, raid, and convoy are
later capabilities. A general-purpose event scripting language is not proposed.

## Confirmed behavior

### Scheduling and registration

1. Admin schedules a future event and specifies when registration opens and how
   long players can sign up. Registration may close before or at event start.
2. When registration opens, players can sign up.
3. At closing, lock registration and notify players that they are registered.
4. At event start, warn registered players to stash their survival kit before
   accepting transport.
5. Run an affirmative ready check. Signup alone does not authorize stripping or
   teleporting. Acceptance explicitly warns that all carried belongings will be
   destroyed. No escrow or restoration of original inventory in v1.
6. Prepare accepted players: strip, teleport to safe random positions within
   the admin's spawn radius, equip the specified kit, and freeze until the
   synchronized on-screen countdown ends.

Accepting ready records consent; transport happens together after the ready
window closes. Revalidate the roster, minimum player count, safe arena/spawns,
kit classes, and each player's eligibility before stripping anyone. Reject
unconscious, restrained, or vehicle-occupied entrants and prevent admission from
becoming a combat-escape mechanism. Use configurable ready duration and minimum
roster. Decline/no response causes no mutation. Cancel before destruction if
validation fails; after partial preparation, use the cancellation/return policy.
Event check-in time and combat start must be distinct in UI copy; the ready-check
deadline and preparation duration must not be hidden from participants.

### Combat and isolation

- Fight until a single eligible participant remains. Leaving the arena forfeits
  the event and kills the participant. Numerical boundary tolerance requires
  testing; it must not create an escape loophole.
- Normalize participants to the same healthy starting condition, including
  bleeding, disease, hunger, and thirst. Do not restore original survival status.
- Provide a configurable shrinking boundary and maximum match duration. Zero
  survivors, including simultaneous final deaths, means no winner. Multiple
  survivors at the final deadline also means no winner; no implicit tiebreak.
- Dead participants cannot retain event equipment through respawning.
- Surviving contestants may loot eliminated contestants during the match (D01,
  confirmed). Keep their corpses' event equipment available to active contestants;
  do not delete it merely because its original recipient died. Eliminated players,
  spectators, and outsiders cannot recover it. Track event ownership through
  transfers, attachments, and containers. At settlement only the winner's carried
  inventory is retained; remaining event-owned loot is cleaned up.
- Logging out cannot preserve an event kit for retrieval after the event ends.
  Record forfeiture and remove equipment before normal disconnect persistence;
  recover interrupted cleanup on reconnect/startup. Do not revoke equipment
  already legitimately looted by another active contestant when its original
  recipient later disconnects.
- Ordinary players do not receive participant equipment, health, respawn, or
  movement rules. A temporary arena necessarily restricts access locally.
- Outsiders inside the reserved arena are safely teleported at least 100 meters
  beyond the nearest border, not into the ocean. Apply this on activation and
  later intrusion. Search safe alternatives and use a configured fallback when
  the nearest outward point is unsafe. Validate a fallback before activation.
- Ignore existing player buildings when deciding whether an admin may place an
  event. Do not implement an occupied-property veto. Cleanup still owns only
  event-created objects and must not sweep unrelated buildings or items.
- Vehicle occupancy is a required admission/teleport case: a participant must
  not bring their vehicle or other occupants when accepting transport.
  Block ready acceptance while occupied and recheck immediately before mutation;
  prompt the player to exit within the existing deadline. No automatic extraction
  in v1. Safe outsider vehicle/aircraft handling requires prototype evidence.
- Active disconnect means immediate forfeit and no re-entry. On server restart,
  abort unfinished combat, clean up event gear, and resolve pending returns;
  preserve already-settled winnings and awards. A whole-server shutdown must not
  select a winner by disconnect callback order.

Full isolation must address damage and items crossing the perimeter, not merely
show a visual wall. Bullet, explosive, collision, fast-vehicle, aircraft, modded
damage, and dropped-item behavior need evidence before claims of protection.
Normal physics in the shared world is not assumed to be a separate instance.

### Spectating, winner, and return

- Optional transparent floating spectator platform overlooking the arena.
  Eliminated participants respawn there with godmode and no damage dealing.
- Spectators are a separate role: never counted alive in the match or handled
  as ordinary outsider intruders. They cannot recover event kit through respawn.
- Winner goes to a separate podium above spectators with carried winnings intact;
  never apply loser stripping to the winner. Reward collection is private.
- Everyone participating returns to the position from which they entered the
  event. Capture that origin immediately before event teleport. Never return a
  player to an obsolete vehicle seat; use a safe world position near the origin.
- Return before removing platforms. Early spectator departure should also use
  the saved origin. Offline return obligations must survive event cleanup and
  be resolved before normal interaction on reconnect.
- Check return destinations for safety; prefer nearby safe alternatives, then
  the configured fallback. Original belongings are not restored.
- Admin cancellation ends an active event early with no winner or reward.
  Before durable winner settlement, strip temporary equipment and return players
  safely without restoring original belongings or granting automatic compensation.
  This also governs partially prepared entrants. Entrants not yet stripped keep
  their originals. After durable settlement, ending the presentation does not
  revoke the winner's retained inventory or earned award.
- Winner announcement to the server is optional.

Platform rules: contain spectators, prevent item transfers and world
interactions, recover falls, and allow voluntary return. Validate that platform
collision and spectators do not interfere with combat. The platform is optional;
winner and return flows must still work when spectator mode is disabled. Use
safe direct return for the winner when the podium is disabled; rewards remain
claimable. Use a minimal explicitly defined spectator/return clothing kit for
eliminated players and cancelled stripped entrants, never their combat kit.
Document that spectators can relay information through external voice chat;
admins may disable spectating for competitive events.

### Reward claim window

- Save an award entitlement before delivery. The winner's carried inventory is
  retained separately from the admin-specified reward.
- A winner-only podium box opens a claim UI. Do not treat altitude as access
  control. Avoid simultaneously spawning prizes into a normal box and keeping
  those same prizes claimable in a ledger.
- Player selects what to claim; deliver only what fits. Remaining items stay
  pending. No automatic ground drops, forced replacement, or silent loss.
- Pending awards survive disconnection, server restart, and podium removal.
- Permit later claims within an admin-configurable window. Commands:
  `/event rewards` lists outstanding awards; `/event claim` opens the claim UI.
- Later claims do not teleport or protect the player. Show remaining quantities
  and exact expiry. Expiration and interrupted claims need deterministic handling.
- Persist per-award delivery progress; retries and interrupted inventory saves
  must not duplicate rewards or consume an undelivered entitlement. Prototype
  reconciliation; a journal alone does not make inventory delivery atomic.
- Deferred storage is for awarded rewards, not arbitrary discarded combat loot.

Accepted configurable defaults: two-minute podium period and 24-hour claim
window.

### In-game administration and attribution

Templates need scheduling, roster limits, kit definition, arena geometry/spawn
radius, safe fallback, timers, rewards, optional platforms, and announcements.
Support validation/preview and live event status/cancellation. Server-side
permissions govern all administrative actions, using local admin identities
initially. Reusable templates persist across restarts, with immutable configuration
snapshots for active runs and export/import using the template format. Exact
editor layout remains implementation design work. The accepted role and
server-identity contract is in [admin permissions](admin-permissions.md).

Use licensing matching Sentinel Deathmatch (see root LICENSE.md). Keep innocuous
attribution: a small "Powered by Sentinel" footer on event menus and Workshop
listing, with a discreet dayz.fyi link where appropriate. No interrupting ads,
chat advertising, or permanent promotional HUD during ordinary survival play.
This is intended UI behavior, not an implemented feature.

## Accepted architecture direction

Keep these responsibilities separate; concrete file/API contracts follow after
the first feasibility results:

| Component | Owns |
| --- | --- |
| Templates and admin service | Permissions, validation, schedules, immutable run configuration |
| Event coordinator | Phase transitions, roster, ready deadlines, winner/cancellation settlement |
| Participant sessions | Stable identity, role, eligibility, saved origin, pending return |
| Arena service | Geometry, safe spawning, outsider access, platform ownership |
| Kit and item ownership | Stripping, issued item tracking, corpse/transfer policy, cleanup |
| Reward service | Entitlements, partial delivery, expiry, restart reconciliation |
| Client UI | Signup, consent, countdown, editor, spectator/claim screens |
| Durable recovery | Transition records, cleanup and return obligations, uncertain delivery |

Phases: Draft -> Scheduled -> RegistrationOpen -> RegistrationClosed ->
ReadyCheck -> Preparing -> Countdown -> Active -> Resolving -> Returning -> Closed.
Cancellation routes to cleanup/return without a win. Registration-close and start
at the same instant still process closing before ready check. Reward entitlements
outlive Closed; event closure must not expire them. Participant roles (contestant,
eliminated, spectator, winner, returned, forfeited) are distinct from event phases.

Deployment: standalone public client/server mod for this UI, with no mandatory
external service dependency or paid service. Packaging needs in-engine validation.
Modded aircraft/vehicles require an explicit supported compatibility matrix.

## Resolved decisions

- **D01 - Corpse looting (2026-09-05):** surviving contestants may loot eliminated
  contestants. Only the winner's carried inventory leaves the event; reward
  entitlements remain separate. See combat and isolation for ownership/cleanup.

## Accepted recommendations (2026-09-05)

| ID | Decision | Accepted direction |
| --- | --- | --- |
| D02 | Transport immediately on acceptance or after ready check closes? | Close ready check, validate roster and arena, then prepare together to reduce partial-admission failures. This supersedes the earlier tentative immediate-transport recommendation. |
| D03 | Ready-check timeout, minimum players, unsafe/incomplete preparation? | Configurable deadline/minimum; validate before destruction. Apply cancellation/return policy if preparation is interrupted. |
| D04 | Starting health/status and eliminated return equipment? | Equal healthy start; minimal spectator/return clothing, no event kit or original status restoration. |
| D05 | Vehicle handling for participants and outsiders? | Block acceptance while occupied and recheck before mutation; ask player to exit. Outsider vehicles/aircraft need safe whole-vehicle handling or a documented activation restriction. |
| D06 | Stalemate, time limit, simultaneous final deaths, boundary tolerance? | Configurable shrinking zone and max duration; no winner if zero or multiple remain at final resolution. Tune numerical tolerance in testing. |
| D07 | Disconnect and server restart policy? | Immediate forfeit on active disconnect; no re-entry. Abort unfinished combat after restart, resolve pending returns, retain already-settled awards. Distinguish whole-server shutdown from one-player logout. |
| D08 | Cancellation and settlement cutoff? | Before durable winner settlement: no award, strip temporary kit, safe return with minimal clothing. After settlement: close presentation without revoking earned inventory or award. No automatic partial-entry compensation. |
| D09 | Platforms in v1, disabled-mode finish, and spectator information sharing? | Include optional platform and podium behind feasibility gates; safe direct winner return when disabled. Inform admins about spectator voice-chat scouting. |
| D10 | Claim defaults and restrictions? | Two-minute podium, 24-hour claim window; `/event rewards` and `/event claim`; partial delivery and no ground drops. Resolve delivery races and eligibility details in implementation design. |
| D11 | Packaging, standalone admin identity, template import/export? | Standalone client/server mod, local admin identities initially, no mandatory paid service. |
| D12 | Launch scope and attribution placement? | One active BR with all safety/claim requirements; later DZE/raid/convoy. Small event-menu footer and discreet dayz.fyi link. |

The owner accepted the recommendations together; do not request approval for
these same choices again. Their explicit overrides remain authoritative: original
gear is destroyed, players return to origin, existing buildings do not veto an
arena, and contestants may loot eliminated players. Earlier suggestions to send
players to ordinary survival respawn locations or transport immediately on ready
acceptance are superseded.

## Remaining implementation design and evidence

- Safe outsider vehicle/aircraft relocation and tested compatibility; the claim
  that teleporting an occupant moves the vehicle is not assumed true.
- Exact inventory provenance, persistence, and recovery mechanism, including
  uncertain reward delivery and preventing corpse/transfer duplication.
- Claim expiry races, eligible-server scope, modded/oversized reward validation,
  and combat-time claim restrictions. No specific policy for these was previously
  recommended; choose and document coherent implementation defaults.
- Concrete ready/minimum/countdown bounds, shrinking-zone schedule, safe-spawn
  separation, return clothing classnames, and numerical boundary tolerance.
- Client/server packaging, admin permission format, RPC contracts, UI layouts,
  and measured isolation/performance under the supported mod stack.

Resolve ordinary technical choices during first-slice planning; ask the owner only
if evidence requires a material departure from the accepted experience. Acceptance
of the product baseline does not assert that any capability is implemented or
that engine limitations have been disproved.

## Later event modes and DZE compositions

Optional `.dze` loading adds temporary scenery or staff-built raid bases. Reserve
and clear the arena before loading; validate classes and footprint, track spawned
object ownership, then verify readiness before admission. Return/evacuate players
before cleanup; reconcile objects moved, destroyed, or persisted over restart.
Do not bundle third-party compositions/assets without permission.

Raid and convoy reuse lifecycle and administration, not BR's mandatory stripping
or sealed arena. A convoy has rewarded player targets, route stops, and a final
refuge; a raid has placed targets and staff defenders. Both require separate specs.
