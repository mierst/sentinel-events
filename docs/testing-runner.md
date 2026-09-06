# Isolated boot runner

`tools/run-private-test.ps1` runs the admission boot fixtures and writes a local
evidence manifest. It does not perform the connected-player rehearsal or the
crash/recovery matrix.

Prepare a disposable copy of a vanilla server mission and a test server config
under this checkout's `build/private-test`. Keep the installed game executable
and data in their normal installation directory. Do not commit copied game
assets or generated profiles. Supply a profile directory that does not yet exist;
each run creates it and refuses to reuse another run's logs.

Example from the repository root, after building the mod:

```powershell
$repo = (Get-Location).Path
$profile = Join-Path $repo ('build/private-test/profiles/' + [Guid]::NewGuid().ToString('N'))
.\tools\run-private-test.ps1 `
    -ServerExe 'D:\DayZServer\DayZServer_x64.exe' `
    -ServerConfig (Join-Path $repo 'build/private-test/serverDZ.cfg') `
    -MissionPath (Join-Path $repo 'build/private-test/mpmissions/dayzOffline.chernarusplus') `
    -ProfilePath $profile `
    -ModPath (Join-Path $repo 'build/@SentinelEvents')
```

Use your installed server path. The test config must select the disposable
mission and unused test ports. An unsigned development build requires a private
test config that permits it; signed-release installation is a separate gate.
The verified launcher currently requires config, mission, profile and mod paths
without spaces, quotes, or semicolons. The executable path may contain spaces.

The runner starts a hidden server, observes its script log, validates the expected
fixture sequence, and stops only its own process. Script compilation errors,
timeouts, missing fixtures, artifact changes during the run, and nonzero premature
exits fail the command. It waits for its process to exit before finishing cleanup.
It never discovers or stops unrelated DayZ processes.

`-KeepRunning` leaves a successfully verified instance running for subsequent
manual checks; failures still stop that instance. The returned PID identifies the
owned server. The mission's persistence storage is separate from the fresh log
profile: use a fresh mission copy when the scenario requires a fresh character
database, and retain the same disposable mission when testing recovery.

The profile's `run.json` records a unique run ID, UTC times, the owned PID,
server/PBO paths and SHA-256 hashes, result, and fixture evidence path/hash. The
fixture evidence is a fixed byte snapshot, so later client activity cannot change
the validated fixture log when `-KeepRunning` is used. Keep this manifest with its
snapshot and the tested PBO when collecting local evidence. It links observed
inputs and output; it is not a signed attestation or a gameplay test result.

Validation on DayZ server 1.29.163709: the runner passed all 11 admission fixtures,
wrote the completed manifest, and stopped its owned process. Negative probes
rejected an existing profile and a reparse-point addons path before launch. An
initial post-stop log-lock race was reproduced and fixed by capturing the snapshot
with shared-read access and explicitly waiting for exit.

Focused process doubles also exercised evidence-hash failure, final-manifest
failure with `-KeepRunning`, and an exit racing the last fixture. All now fail
the run, and finalization failures stop the owned process. Separate success
probes verify default shutdown and successful retention. These controlled
failure probes test launcher control flow, not DayZ behavior.

## Independent diagnostic watcher

`tools/watch-private-test.ps1` attaches to a successful retained run using its
profile's `run.json`. It validates the process identity, starts reading at the
current end of the live log, and writes a unique `motion-watch-*.ready.json` before
monitoring telemetry for the requested, case-sensitive trial token. Observe that
ready artifact before signalling a matching diagnostic producer. The watcher
does not start a producer, freeze a player, or launch a server itself.

```powershell
.\tools\watch-private-test.ps1 -ProfilePath $profile -TrialToken 'unique-trial-token'
```

Defaults are a 100 ms poll, 0.25 m horizontal and 0.50 m vertical displacement
limits, a 3 second armed limit, a 1 second heartbeat limit, and a 120 second wait
for arming. Expired monotonic deadlines take precedence over newly buffered
telemetry. Only exact SCRIPT telemetry for the current token is considered;
unrelated tokens, chat lines, and bytes predating attachment cannot satisfy it.

On failure, the watcher rechecks its bound process identity before stopping that
process. It never selects servers by name or kills a process tree. An observed
`finished` message leaves the server running and is only a transport result, not
proof of correct movement or restored controls. A unique result JSON records the
outcome. Polling, native log flushing, and OS scheduling limit reaction time; this
is not a real-time guarantee or a substitute for an in-engine abort path.

Twelve hidden PowerShell process-double scenarios pass, including late telemetry,
missing heartbeat, displacement limits, token isolation, cumulative log-byte
limits, and process ownership. Native log transport results and their narrower
scope are recorded in [watcher evidence](v1/evidence/watchdog.md).
Connected gameplay validation remains a separate release gate.
