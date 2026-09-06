# Public testing alpha

Server: **Sentinel Events Alpha**, `144.76.35.18:2322`, no password.
Chernarus, 16 slots, first person, vanilla survival economy.

[Download the signed alpha](https://github.com/mierst/sentinel-events/releases/tag/alpha-2026-09-06)
and [join the community Discord](https://discord.gg/5NKHAA8Wb3).

Extract the ZIP into a permanent folder. In the official DayZ launcher, choose
**Mods > Local mod**, select the extracted `@SentinelEvents` directory, and enable
it. Direct-connect to the address above. Load only Sentinel Events for this
server. The launcher cannot subscribe or update this build through Workshop yet;
replace the local mod folder when a new signed download is announced.

## What this build tests

This is a read-only readiness rehearsal, not playable battle royale. Normal
Chernarus survival is available. Rehearsals do not strip equipment, teleport,
freeze, create event kits, start combat, or award rewards. Persistent-session
eligibility remains conservative and may refuse Accept; that restriction is
expected until recovery verification is implemented.

An authorized administrator can use `/event rehearse` or `/event cancel`.
Players can use `/event ready` to reopen a current fresh offer. Registration,
scheduled events, template editing, role tiers, combat and spectator platforms
remain planned work. See [the release gates](../docs/release-v0.0.1.md).

## First group test

Coordinate in **events-dev**. Use two participating testers plus one ordinary
player when possible. A server boot or automated fixture pass does not establish
these connected-client results; record each separately:

1. All clients join with the matching signed build and can move and use inventory.
2. An ordinary player cannot start or cancel a rehearsal as an administrator.
3. The administrator opens a rehearsal. Offered players see the warning and
   countdown; Close returns control, and `/event ready` reopens an unexpired offer.
4. Decline and Accept display the server's actual response. Record refusal as
   refusal, not successful admission. No choice should alter position or gear.
5. Expiry and cancellation disable acceptance. A stale or disconnected offer
   must not become active after reconnecting.
6. Ordinary survival controls still work after closing the menu. Report any
   unexpected movement, inventory change, or persistent input lock immediately.

Do not conduct movement-freeze or destructive-admission experiments on this build.

## Contributing

Use **Events Contributor** through Discord onboarding to find **events-dev**.
Use **Events Updates** and **Events Alerts** for optional notices. Discuss ideas
there, then put reproducible bugs and scoped implementation proposals in GitHub
issues so they remain searchable. Pull requests should identify their test
evidence and any unverified native behavior. Read [CONTRIBUTING](../CONTRIBUTING.md)
before changing gameplay or persistence.

Include your build, steps, expected and actual result, and relevant screenshots.
Redact player identifiers, credentials and unrelated log content. Do not upload
full server profiles. Discord discussion does not replace the accepted design or
the release gate evidence.
