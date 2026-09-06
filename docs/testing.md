# Testing

This file records reproducible validation for the pre-release implementation.
Generated PBOs, server profiles, logs, and mission storage stay under ignored
`build/` paths and are not release artifacts.

## Admission policy bootstrap (0.0.1-pre)

Validated on 2026-09-05 with:

- DayZ dedicated server `1.29.0.163709` at
  `C:\Users\alexl\dev\dayz-server-win\steamcmd\steamapps\common\DayZServer\DayZServer_x64.exe`.
- MakePbo `2.16.9.36` at
  `C:\Program Files (x86)\Mikero\DePboTools\bin\MakePbo.exe`.
- PBO output
  `build/@SentinelEvents/addons/sentinel_events.pbo`, SHA-256
  `36A07B9B91C46C915B634E0C7B1E2E073ACBF7AA2A4C98A747AFFEE2C8BD4780`.

Run the source check and build from the repository root:

```powershell
.\tools\check-source.ps1
.\tools\build.ps1 -MakePboPath 'C:\Program Files (x86)\Mikero\DePboTools\bin\MakePbo.exe'
```

The private boot used the generated mod directory through
`-mod=<checkout>\build\@SentinelEvents`, an isolated profile, and a copied
vanilla Chernarus mission. The initial fixture-only package intentionally omitted
the policy and failed compilation at `SevAdmissionTests.c:39` with unknown
`SevEligibility`. Its script log is:

```text
build/private-test/profiles/20260905-215946-137/script_2026-09-05_21-59-48.log
```

The first implementation boot then caught an Enforce multiline-expression syntax
error before fixtures could run. Moving the boolean expression to one line fixed
that engine-specific compile error. After hardening the build and log tools, the
exact final artifact was booted again. Its log is:

```text
build/private-test/profiles/20260905-220850-592/script_2026-09-05_22-08-52.log
```

Validate that log with:

```powershell
.\tools\check-fixtures.ps1 -LogPath .\build\private-test\profiles\20260905-220850-592\script_2026-09-05_22-08-52.log -Suite Admission
```

Result: the Game, World, and Mission modules loaded without script compilation
errors, and all 11 admission fixtures passed: `eligible`, `vehicle`,
`unconscious`, `restrained`, `recent-combat`, `pending-session`, `stale-offer`,
`deadline-equality`, `below-minimum`, `invalid-arena`, and `invalid-kit`.

The prototype setting remains disabled (`CfgSentinelEvents.enabled = 0`). This
slice adds only pure policy decisions and boot-time fixture logging; it has no
runtime entry or player-state mutation path. The boot did not connect clients or
exercise gameplay, multiplayer timing, persistence, recovery, inventory, or UI.
Fixtures currently run on every `MissionServer.OnInit` for pre-release boot
evidence and must become opt-in or be removed before a public release build.
