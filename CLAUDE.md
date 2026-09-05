# Sentinel Events

Public DayZ event mod repository: https://github.com/mierst/sentinel-events.
Current phase: repository bootstrap and iterative v1 design, not implementation.

## Sources of truth

- `docs/v1/design.md`: confirmed behavior, proposals, and open decisions.
- `docs/v1/roadmap.md`: draft delivery sequence and acceptance gates.
- `docs/v1/implementation-log.md`: dated decisions and work completed.
- `LICENSE.md`: current terms, adapted only by name from Sentinel Deathmatch.
- `CONTRIBUTING.md`: engineering and validation requirements.

Keep accepted decisions separate from recommendations. Update the design and log
when the owner resolves an open decision. Do not call an unbuilt capability
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
