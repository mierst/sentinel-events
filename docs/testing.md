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
  `270B03E11E6F4036D571C461715153BFB2E8C7CC5141EA142A5544296B909AA4`.

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
build/private-test/profiles/runner-20260905-222053-333/fixture-evidence.log
```

Validate that log with:

```powershell
.\tools\check-fixtures.ps1 -LogPath .\build\private-test\profiles\runner-20260905-222053-333\fixture-evidence.log -Suite Admission
```

Result: the Game, World, and Mission modules loaded without script compilation
errors, and all 11 admission fixtures passed: `eligible`, `vehicle`,
`unconscious`, `restrained`, `recent-combat`, `pending-session`, `stale-offer`,
`deadline-equality`, `below-minimum`, `invalid-arena`, and `invalid-kit`.

The run manifest is
`build/private-test/profiles/runner-20260905-222053-333/run.json`. It records
result `Passed`, the PBO hash above, server binary SHA-256
`16CC3EE2A79AD726D0F4609824090F837BCD391FBA7B1A60213184E61CC0D9D9`, and
fixture-log SHA-256
`D92EBEEDE5DFB866EF4CA501182D456564F04FDFF5D78B1D21D837250E81D456`.
`check-fixtures.ps1` validates the contents of the supplied log, including
fixture completeness and script compilation errors; the launch manifest, not
the fixture parser alone, establishes which artifact produced that log.

Packaging safety probes also verified that:

- a dummy `.biprivatekey` below `scripts` is rejected before the existing PBO
  changes;
- a source-tree junction is rejected before the existing PBO changes; and
- an `addons` junction to an external probe directory is rejected while the
  external sentinel file remains byte-for-byte unchanged.

The prototype setting remains disabled (`CfgSentinelEvents.enabled = 0`). This
slice adds only pure policy decisions and boot-time fixture logging; it has no
runtime entry or player-state mutation path. The boot did not connect clients or
exercise gameplay, multiplayer timing, persistence, recovery, inventory, or UI.
Fixtures currently run on every `MissionServer.OnInit` for pre-release boot
evidence and must become opt-in or be removed before a public release build.
