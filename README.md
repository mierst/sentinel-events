# Sentinel Events

Configurable, staff-run events on a live DayZ survival server.

**Status: implementation in progress. The package boots and 149 admission,
recovery, journal, guard, and spawn fixtures pass on a dedicated server. There is no
playable event or Workshop release yet.**

The first planned mode is an isolated battle royale: scheduled registration,
explicit ready checks, event kits, safe arena transport, a synchronized start,
elimination, optional spectating, and a winner's reward claim window. Participants
return to their original locations when they leave the event. Ordinary survival
continues outside the event, subject to access enforcement at the arena boundary.

Future modes include staff-defended raid bases and player convoys. Temporary
DayZ Editor (`.dze`) compositions are planned as a later shared capability.

## Planning

- [V1 design and decision register](docs/v1/design.md): confirmed requirements,
  accepted defaults and remaining implementation investigations.
- [V1 delivery roadmap](docs/v1/roadmap.md): implementation sequence and evidence
  required before advancing. Detailed implementation tasks are the next step.
- [Session and decision log](docs/v1/implementation-log.md): durable planning history.
- [First-slice admission specification](docs/v1/admission-spec.md): concrete
  defaults, consent/transport sequence, and recovery gates.
- [Admission implementation plan](docs/superpowers/plans/2026-09-05-admission-feasibility.md):
  six bounded tasks for a private feasibility rehearsal, not the full BR release.
- [Source investigation](docs/v1/source-notes.md): verified script surfaces and
  native behavior that still needs runtime evidence.
- [Administration permissions](docs/v1/admin-permissions.md): server-owner grants
  and the organizer, designer, and event-admin permission contract.

The intended experience includes a client UI. Packaging and compatibility must be
validated before release. The mod is designed to operate as a standalone client/server package.

## Attribution and license

Like Sentinel Deathmatch, this is a public **source-available** project under an
interim custom license, not an OSI open-source license. See [LICENSE.md](LICENSE.md)
for the exact current grant, including the alpha playtest terms, attribution,
monetized-server use, and redistribution restrictions.

The intended attribution is a small "Powered by Sentinel" footer on event menus
and the Workshop listing, with a discreet dayz.fyi link where appropriate. No
interrupting advertisements, chat spam, or persistent advertising over survival
gameplay. Final placement is part of UI design.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Please discuss feature changes in an issue
before implementation while the v1 design is still being settled.
