# Contributing

This repository currently contains a design baseline. Work through the open
decisions in [the v1 design](docs/v1/design.md) before treating the roadmap as an
implementation specification.

## Engineering requirements for implementation

- Server authority over roster, timing, inventory, rewards, permissions, and
  elimination. Client UI requests actions; it does not grant them.
- Fast participant/event guards before allocation or expensive world queries.
  No per-frame script polling by default. Bound and measure periodic work,
  including nearby outsider checks and cleanup.
- One owned hook block per modded class, delegating to focused services. Preserve
  vanilla and other mods' lifecycle chains. Review hook ordering explicitly when
  inventory must be removed before a vanilla character save.
- Prefix modded-class members and RPC identifiers uniquely. Validate admin input
  and bound work from configuration before activating an event.
- Use ASCII in Enforce Script sources. Match override parameter names to vanilla,
  avoid duplicate local declarations within a method, and do not put a leading
  `+` on a continued string expression.
- Keep config changes backward compatible once a schema is published; use
  explicit migrations rather than silently repurposing existing fields.
- Persist enough information to recover participant returns and reward delivery.
  Do not assume inventory changes and journal writes are atomic together.
- Do not commit credentials, signing private keys, live player records, server
  profiles, generated PBOs, or third-party game/mod assets.

## Verification

Documentation-only changes need link, consistency, and diff review. Do not claim
gameplay validation for this bootstrap.

For code, add focused pure decision fixtures where useful and run an actual
dedicated-server boot: packaging a PBO does not prove Enforce Script compiles.
Client UI, vehicle behavior, transfer restrictions, and multiplayer timing require
an actual client/server test. Lifecycle changes require disconnect and restart
tests. Reward delivery requires crash-at-transition and replay tests. Record
commands, logs, mod versions, results, and limitations beside the relevant work.

Measure changes on frequent hooks or recurring scans under representative load;
idle-server behavior alone is insufficient. A build/test harness will be added
with the first prototype, rather than documenting nonexistent commands now.

## Source and releases

Use independently authored implementation; do not copy code or assets from other
mods. Credit design references and interoperation formats. Contributions must be
owned by the contributor and offered under the project's current license; this
does not expand redistribution rights granted by [LICENSE.md](LICENSE.md).

Before the first release, add a concrete build/signing/Workshop checklist and
verify license text, attribution, client dependencies, recovery behavior, and
compatibility evidence. Release work is separate from repository setup.
