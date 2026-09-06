# Sentinel Events

Public DayZ event mod repository: https://github.com/mierst/sentinel-events.
Current phase: first-slice implementation toward a v0.0.1 release candidate.
The package compiles on a dedicated server and 149 admission, recovery, journal,
guard, and spawn fixtures pass. A diagnostic marker survived one client's logout
and a server restart. Destructive recovery, gameplay guards, and multiplayer
isolation remain unverified. A bootable package is not a playable event or a
release gate pass. See `docs/v1/evidence/persistence.md` for the remaining gates.

## Sources of truth

- `docs/v1/design.md`: accepted behavior and remaining technical investigations.
- `docs/v1/roadmap.md`: draft delivery sequence and acceptance gates.
- `docs/v1/implementation-log.md`: dated decisions and work completed.
- `LICENSE.md`: current terms, adapted only by name from Sentinel Deathmatch.
- `CONTRIBUTING.md`: engineering and validation requirements.
- `docs/v1/admission-spec.md`: private rehearsal scope, defaults, and recovery gates.
- `docs/superpowers/plans/2026-09-05-admission-feasibility.md`: first execution plan.
- `docs/v1/source-notes.md`: inspected vanilla surfaces and unproven native behavior.
- `docs/v1/admin-permissions.md`: standalone server-owner role assignment contract.

The owner accepted previously presented recommendations on 2026-09-05, with
explicit user choices taking precedence and the latest recommendation resolving
conflicting earlier proposals. Do not reopen those approvals. Document remaining
technical choices and update the design/log as evidence resolves them.
Do not call an unbuilt capability
implemented or tested. Do not publish a Workshop release as part of planning.

## Boundaries

Battle royale is the first complete mode. Raid, convoy, and DZE compositions are
later work. Event participation must be explicit; normal players must not enter
a global deathmatch spawn/equipment loop. Inventory removal requires the player's
ready acceptance. Original belongings are destroyed, not escrowed or restored.

Events and event-owned entities have explicit ownership. Cleanup must not remove
unrelated world entities. Existing player buildings do not block arena selection;
the admin chooses the location at their own discretion.

Private cross-project context must remain local. Consult the untracked
`.git/local-context/README.md` when present; never publish its contents.

Do not edit other Sentinel repositories or copy their gameplay code incidentally
while working here. Read their guidance if a task explicitly involves them.
