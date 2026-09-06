[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ProfilePath,
    [Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9][A-Za-z0-9._:-]{0,127}$')][string]$TrialToken,
    [ValidateRange(25, 1000)][int]$PollMilliseconds = 100,
    [ValidateRange(0.001, 1000.0)][double]$MaxHorizontalMeters = 0.25,
    [ValidateRange(0.001, 1000.0)][double]$MaxVerticalMeters = 0.50,
    [ValidateRange(0.1, 30.0)][double]$MaxArmedSeconds = 3.0,
    [ValidateRange(0.1, 30.0)][double]$HeartbeatMaxSeconds = 1.0,
    [ValidateRange(0.1, 600.0)][double]$WaitForArmSeconds = 120.0,
    [ValidateRange(1, 30)][int]$ProcessExitWaitSeconds = 5,
    [ValidateRange(1024, 1048576)][int]$MaxLineBytes = 4096,
    [ValidateRange(4096, 16777216)][int]$MaxAppendedBytes = 1048576
)

$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$testRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot 'build/private-test'))
$testPrefix = $testRoot.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
# Process and NTFS creation timestamps may be coarsened differently. This tolerance
# applies only to ownership-window comparisons; all watchdog deadlines are monotonic.
$clockTolerance = [TimeSpan]::FromSeconds(2)
$watchId = [Guid]::NewGuid().ToString('N')
$watchStartedUtc = [DateTime]::UtcNow
$watchClock = [Diagnostics.Stopwatch]::StartNew()
$profileDirectory = $null
$readyPath = $null
$resultPath = $null
$boundIdentity = $null
$bound = $false
$readyWritten = $false
$logStream = $null
$pendingLine = [Collections.Generic.List[byte]]::new()
$appendedBytes = [long]0
$observedArmed = $false
$observedFinished = $false
$armedAt = $null
$lastHeartbeatAt = $null
$maximumHorizontal = 0.0
$maximumVertical = 0.0

$result = [ordered]@{
    SchemaVersion = 1
    WatchId = $watchId
    RunId = $null
    TrialToken = $TrialToken
    Status = 'Starting'
    Reason = $null
    Error = $null
    Bound = $false
    ReadyWritten = $false
    ReadyPath = $null
    ResultPath = $null
    Pid = $null
    ServerExe = $null
    ServerSha256 = $null
    ProcessStartedUtc = $null
    LogPath = $null
    InitialLogOffset = $null
    AppendedBytes = 0
    ObservedArmed = $false
    ObservedFinished = $false
    MaximumHorizontalMeters = 0.0
    MaximumVerticalMeters = 0.0
    StopAttempted = $false
    StopSucceeded = $false
    StopError = $null
    StartedUtc = $watchStartedUtc.ToString('o')
    ArmedAfterMilliseconds = $null
    LastHeartbeatAgeMilliseconds = $null
    CompletedUtc = $null
    ElapsedMilliseconds = $null
}

function Assert-NoReparseAncestor([string]$Path) {
    $cursor = [IO.Path]::GetFullPath($Path)
    while ($cursor) {
        if (Test-Path -LiteralPath $cursor) {
            $item = Get-Item -LiteralPath $cursor -Force
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Reparse-point path is not permitted: $cursor"
            }
        }
        $parent = Split-Path -Parent $cursor
        if (-not $parent -or $parent -eq $cursor) { break }
        $cursor = $parent
    }
}

function Resolve-Beneath([string]$Path, [string]$Prefix, [string]$Description) {
    $resolved = [IO.Path]::GetFullPath($Path)
    if (-not $resolved.StartsWith($Prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "$Description must be beneath $testRoot"
    }
    Assert-NoReparseAncestor $resolved
    return $resolved
}

function Test-SamePath([string]$Left, [string]$Right) {
    return [string]::Equals([IO.Path]::GetFullPath($Left), [IO.Path]::GetFullPath($Right), [StringComparison]::OrdinalIgnoreCase)
}

function Require-Property($Object, [string]$Name) {
    if ($null -eq $Object -or $Name -notin @($Object.PSObject.Properties.Name)) {
        throw "Run manifest is missing $Name"
    }
    return $Object.$Name
}

function Require-String($Object, [string]$Name) {
    $value = Require-Property $Object $Name
    if ($value -isnot [string] -or [string]::IsNullOrWhiteSpace($value)) {
        throw "Run manifest $Name must be a non-empty string"
    }
    return $value
}

function Parse-UtcTimestamp($Object, [string]$Name) {
    $value = Require-String $Object $Name
    $parsed = [DateTime]::MinValue
    if (-not [DateTime]::TryParse($value, [Globalization.CultureInfo]::InvariantCulture, [Globalization.DateTimeStyles]::RoundtripKind, [ref]$parsed) -or $parsed.Kind -ne [DateTimeKind]::Utc) {
        throw "Run manifest $Name must be an ISO-8601 UTC timestamp"
    }
    return $parsed
}

function Assert-Hash([string]$Value, [string]$Name) {
    if ($Value -notmatch '^[A-Fa-f0-9]{64}$') { throw "Run manifest $Name must be a SHA-256 hash" }
}

function Write-JsonArtifact([string]$Path, $Value) {
    $json = $Value | ConvertTo-Json -Depth 6
    [IO.File]::WriteAllText($Path, $json + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
}

function Read-Identity([int]$ProcessId, [bool]$IncludeHash) {
    $process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
    if (-not $process -or $process.HasExited) { throw 'ProcessExited' }
    $path = $process.Path
    if ([string]::IsNullOrWhiteSpace($path)) { throw "Cannot read executable path for process $ProcessId" }
    $identity = [ordered]@{
        Process = $process
        Pid = $process.Id
        Path = [IO.Path]::GetFullPath($path)
        StartedUtc = $process.StartTime.ToUniversalTime()
        Sha256 = $null
    }
    if ($IncludeHash) {
        $identity.Sha256 = (Get-FileHash -LiteralPath $identity.Path -Algorithm SHA256).Hash
    }
    return [pscustomobject]$identity
}

function Assert-BoundIdentity([bool]$IncludeHash) {
    $current = Read-Identity $boundIdentity.Pid $IncludeHash
    if (-not (Test-SamePath $current.Path $boundIdentity.Path)) { throw 'Bound process executable path changed' }
    if ($current.StartedUtc.Ticks -ne $boundIdentity.StartedUtc.Ticks) { throw 'Bound process start time changed' }
    if ($IncludeHash -and -not [string]::Equals($current.Sha256, $boundIdentity.Sha256, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Bound process executable hash changed'
    }
    return $current
}

function Stop-BoundProcess {
    $result.StopAttempted = $true
    try {
        $current = Assert-BoundIdentity $true
        if (-not $current.Process.HasExited) { $current.Process.Kill() }
        if (-not $current.Process.WaitForExit($ProcessExitWaitSeconds * 1000)) {
            throw "Bound process did not exit within $ProcessExitWaitSeconds seconds"
        }
        $result.StopSucceeded = $true
    }
    catch {
        $stillPresent = Get-Process -Id $boundIdentity.Pid -ErrorAction SilentlyContinue
        if (-not $stillPresent) {
            $result.StopSucceeded = $true
        }
        else {
            $result.StopSucceeded = $false
            $result.StopError = $_.Exception.Message
        }
    }
}

function Complete-Result([string]$Status, [string]$Reason, [string]$ErrorText) {
    $result.Status = $Status
    $result.Reason = $Reason
    $result.Error = $ErrorText
    $result.Bound = $bound
    $result.ReadyWritten = $readyWritten
    $result.AppendedBytes = $appendedBytes
    $result.ObservedArmed = $observedArmed
    $result.ObservedFinished = $observedFinished
    $result.MaximumHorizontalMeters = $maximumHorizontal
    $result.MaximumVerticalMeters = $maximumVertical
    if ($armedAt) { $result.ArmedAfterMilliseconds = [Math]::Round($armedAt.TotalMilliseconds, 3) }
    if ($lastHeartbeatAt) { $result.LastHeartbeatAgeMilliseconds = [Math]::Round(($watchClock.Elapsed - $lastHeartbeatAt).TotalMilliseconds, 3) }
    $result.CompletedUtc = [DateTime]::UtcNow.ToString('o')
    $result.ElapsedMilliseconds = [Math]::Round($watchClock.Elapsed.TotalMilliseconds, 3)
    if ($resultPath) { Write-JsonArtifact $resultPath $result }
}

function Get-ProtocolEvent([string]$Line) {
    $prefixMatch = [regex]::Match($Line, '^\s*SCRIPT[ \t]+:[ \t]+(?<message>.*)$', [Text.RegularExpressions.RegexOptions]::CultureInvariant)
    if (-not $prefixMatch.Success) { return $null }
    $message = $prefixMatch.Groups['message'].Value
    $escapedToken = [regex]::Escape($TrialToken)
    $currentTokenPattern = "^\[SEV\] motion-probe token={0}(?:\s|$)" -f $escapedToken
    if (-not [regex]::IsMatch($message, $currentTokenPattern, [Text.RegularExpressions.RegexOptions]::CultureInvariant)) { return $null }

    $phaseMatch = [regex]::Match($message, ("^\[SEV\] motion-probe token={0} phase=(?<phase>armed|finished|failed)$" -f $escapedToken), [Text.RegularExpressions.RegexOptions]::CultureInvariant)
    if ($phaseMatch.Success) { return [pscustomobject]@{ Phase = $phaseMatch.Groups['phase'].Value; Horizontal = $null; Vertical = $null } }

    $number = '[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?'
    $sampleMatch = [regex]::Match($message, ("^\[SEV\] motion-probe token={0} phase=sample horizontal=(?<horizontal>{1}) vertical=(?<vertical>{1})$" -f $escapedToken, $number), [Text.RegularExpressions.RegexOptions]::CultureInvariant)
    if (-not $sampleMatch.Success) { return [pscustomobject]@{ Phase = 'invalid'; Horizontal = $null; Vertical = $null } }

    $horizontal = 0.0
    $vertical = 0.0
    if (-not [double]::TryParse($sampleMatch.Groups['horizontal'].Value, [Globalization.NumberStyles]::Float, [Globalization.CultureInfo]::InvariantCulture, [ref]$horizontal) -or
        -not [double]::TryParse($sampleMatch.Groups['vertical'].Value, [Globalization.NumberStyles]::Float, [Globalization.CultureInfo]::InvariantCulture, [ref]$vertical) -or
        [double]::IsNaN($horizontal) -or [double]::IsInfinity($horizontal) -or $horizontal -lt 0 -or
        [double]::IsNaN($vertical) -or [double]::IsInfinity($vertical) -or $vertical -lt 0) {
        return [pscustomobject]@{ Phase = 'invalid'; Horizontal = $null; Vertical = $null }
    }
    return [pscustomobject]@{ Phase = 'sample'; Horizontal = $horizontal; Vertical = $vertical }
}

function Get-ExpiredDeadline([TimeSpan]$Now) {
    if (-not $observedArmed) {
        if ($Now.TotalSeconds -ge $WaitForArmSeconds) { return 'ArmWaitTimeout' }
        return $null
    }
    if (($Now - $armedAt).TotalSeconds -ge $MaxArmedSeconds) { return 'ArmedDeadline' }
    if (($Now - $lastHeartbeatAt).TotalSeconds -ge $HeartbeatMaxSeconds) { return 'HeartbeatTimeout' }
    return $null
}

function Read-NewLogEvents {
    $events = [Collections.Generic.List[object]]::new()
    $length = $logStream.Length
    if ($length -lt $logStream.Position) { throw 'LogTruncated' }
    $available = $length - $logStream.Position
    if ($appendedBytes + $available -gt $MaxAppendedBytes) { throw 'LogAppendLimit' }
    $buffer = [byte[]]::new([Math]::Min(65536, [Math]::Max(1, $available)))
    while ($available -gt 0) {
        $requested = [int][Math]::Min($buffer.Length, $available)
        $read = $logStream.Read($buffer, 0, $requested)
        if ($read -le 0) { break }
        $appendedBytes += $read
        $available -= $read
        for ($index = 0; $index -lt $read; $index++) {
            $value = $buffer[$index]
            if ($value -eq 10) {
                $lineBytes = $pendingLine.ToArray()
                $pendingLine.Clear()
                if ($lineBytes.Count -gt 0 -and $lineBytes[-1] -eq 13) {
                    if ($lineBytes.Count -eq 1) { $lineBytes = [byte[]]::new(0) }
                    else { $lineBytes = $lineBytes[0..($lineBytes.Count - 2)] }
                }
                $line = [Text.Encoding]::UTF8.GetString($lineBytes)
                $event = Get-ProtocolEvent $line
                if ($event) { $events.Add($event) }
            }
            else {
                $pendingLine.Add($value)
                if ($pendingLine.Count -gt $MaxLineBytes) { throw 'LogLineLimit' }
            }
        }
    }
    return $events
}

try {
    $profileDirectory = Resolve-Beneath $ProfilePath $testPrefix 'ProfilePath'
    if (-not (Test-Path -LiteralPath $profileDirectory -PathType Container)) { throw 'ProfilePath must be an existing directory' }
    $readyPath = Join-Path $profileDirectory ("motion-watch-$watchId.ready.json")
    $resultPath = Join-Path $profileDirectory ("motion-watch-$watchId.result.json")
    $result.ReadyPath = $readyPath
    $result.ResultPath = $resultPath

    $manifestPath = Join-Path $profileDirectory 'run.json'
    Assert-NoReparseAncestor $manifestPath
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw 'Missing run.json in owned profile' }
    $manifestInfo = Get-Item -LiteralPath $manifestPath -Force
    if ($manifestInfo.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'run.json must not be a reparse point' }
    $manifestLength = $manifestInfo.Length
    $manifestLastWriteTicks = $manifestInfo.LastWriteTimeUtc.Ticks
    $manifestBytes = [IO.File]::ReadAllBytes($manifestPath)
    if ($manifestBytes.Length -gt 65536) { throw 'Run manifest exceeds 64 KiB' }
    $manifestJson = [Text.Encoding]::UTF8.GetString($manifestBytes).TrimStart([char]0xFEFF)
    if ((Get-Command ConvertFrom-Json).Parameters.ContainsKey('DateKind')) {
        $manifest = ConvertFrom-Json -InputObject $manifestJson -DateKind String
    }
    else {
        $manifest = ConvertFrom-Json -InputObject $manifestJson
    }
    if ($manifest -is [array]) { throw 'Run manifest must be one JSON object' }

    if ((Require-Property $manifest 'SchemaVersion') -ne 1) { throw 'Unsupported run manifest schema' }
    $runId = Require-String $manifest 'RunId'
    if ($runId -notmatch '^[A-Fa-f0-9]{32}$') { throw 'Run manifest RunId is invalid' }
    $result.RunId = $runId
    if ((Require-String $manifest 'Result') -ne 'Passed') { throw 'Watcher requires a retained passed run' }
    if ($null -ne (Require-Property $manifest 'Error')) { throw 'Passed run manifest must not contain an error' }
    $startedUtc = Parse-UtcTimestamp $manifest 'StartedUtc'
    $completedUtc = Parse-UtcTimestamp $manifest 'CompletedUtc'
    if ($completedUtc -lt $startedUtc -or $completedUtc -gt [DateTime]::UtcNow.Add($clockTolerance)) { throw 'Run manifest time window is invalid' }

    $profileFromManifest = Require-String $manifest 'ProfilePath'
    if (-not (Test-SamePath $profileFromManifest $profileDirectory)) { throw 'Run manifest ProfilePath does not match the selected profile' }
    $profileCreatedUtc = (Get-Item -LiteralPath $profileDirectory -Force).CreationTimeUtc
    if ($profileCreatedUtc -lt $startedUtc.Subtract($clockTolerance) -or $profileCreatedUtc -gt $completedUtc.Add($clockTolerance)) {
        throw 'Profile creation time is outside the retained run window'
    }

    $serverExe = Require-String $manifest 'ServerExe'
    $serverExe = (Resolve-Path -LiteralPath $serverExe).Path
    if (-not (Test-Path -LiteralPath $serverExe -PathType Leaf)) { throw 'Run manifest ServerExe is not a file' }
    $serverSha256 = Require-String $manifest 'ServerSha256'
    Assert-Hash $serverSha256 'ServerSha256'
    foreach ($name in @('PboPath', 'PboSha256', 'MissionPath', 'ServerConfig', 'Suite', 'LogPath', 'LogSha256', 'LiveLogPath')) {
        [void](Require-String $manifest $name)
    }
    Assert-Hash ([string]$manifest.PboSha256) 'PboSha256'
    Assert-Hash ([string]$manifest.LogSha256) 'LogSha256'

    $manifestPid = Require-Property $manifest 'Pid'
    $parsedPid = 0
    if (-not [int]::TryParse([string]$manifestPid, [ref]$parsedPid) -or $parsedPid -le 0) { throw 'Run manifest Pid is invalid' }

    $liveLogPath = Resolve-Beneath (Require-String $manifest 'LiveLogPath') ($profileDirectory.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar) 'LiveLogPath'
    if (-not (Test-Path -LiteralPath $liveLogPath -PathType Leaf) -or [IO.Path]::GetFileName($liveLogPath) -notlike 'script_*.log') {
        throw 'LiveLogPath must identify a script_*.log file in the owned profile'
    }
    $scriptLogs = @(Get-ChildItem -LiteralPath $profileDirectory -Filter 'script_*.log' -File)
    if ($scriptLogs.Count -ne 1 -or -not (Test-SamePath $scriptLogs[0].FullName $liveLogPath)) { throw 'Owned profile live script log is ambiguous' }

    $candidate = Read-Identity $parsedPid $true
    if (-not (Test-SamePath $candidate.Path $serverExe)) { throw 'Run manifest executable path does not match the live process' }
    if (-not [string]::Equals($candidate.Sha256, $serverSha256, [StringComparison]::OrdinalIgnoreCase)) { throw 'Run manifest executable hash does not match the live process' }
    if ($candidate.StartedUtc -lt $startedUtc.Subtract($clockTolerance) -or $candidate.StartedUtc -gt $completedUtc.Add($clockTolerance)) {
        throw 'Live process start time is outside the retained run window'
    }

    $manifestInfo = Get-Item -LiteralPath $manifestPath -Force
    if ($manifestInfo.Length -ne $manifestLength -or $manifestInfo.LastWriteTimeUtc.Ticks -ne $manifestLastWriteTicks) {
        throw 'Run manifest changed during ownership validation'
    }
    $boundIdentity = $candidate
    $bound = $true
    $result.Bound = $true
    $result.Pid = $candidate.Pid
    $result.ServerExe = $candidate.Path
    $result.ServerSha256 = $candidate.Sha256
    $result.ProcessStartedUtc = $candidate.StartedUtc.ToString('o')
    $result.LogPath = $liveLogPath

    $logInfo = Get-Item -LiteralPath $liveLogPath -Force
    $logCreationUtc = $logInfo.CreationTimeUtc
    $logStream = [IO.File]::Open($liveLogPath, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete)
    $initialOffset = $logStream.Length
    [void]$logStream.Seek($initialOffset, [IO.SeekOrigin]::Begin)
    $result.InitialLogOffset = $initialOffset

    $ready = [ordered]@{
        SchemaVersion = 1
        WatchId = $watchId
        RunId = $runId
        TrialToken = $TrialToken
        Status = 'Ready'
        Pid = $candidate.Pid
        ServerExe = $candidate.Path
        ServerSha256 = $candidate.Sha256
        ProcessStartedUtc = $candidate.StartedUtc.ToString('o')
        LogPath = $liveLogPath
        InitialLogOffset = $initialOffset
        PollMilliseconds = $PollMilliseconds
        MaxHorizontalMeters = $MaxHorizontalMeters
        MaxVerticalMeters = $MaxVerticalMeters
        MaxArmedSeconds = $MaxArmedSeconds
        HeartbeatMaxSeconds = $HeartbeatMaxSeconds
        WaitForArmSeconds = $WaitForArmSeconds
        OwnershipClockToleranceSeconds = $clockTolerance.TotalSeconds
        ReadyUtc = [DateTime]::UtcNow.ToString('o')
        ResultPath = $resultPath
    }
    Write-JsonArtifact $readyPath $ready
    $readyWritten = $true
    $result.ReadyWritten = $true
    Write-Output ([pscustomobject]$ready)

    $failureReason = $null
    while (-not $observedFinished -and -not $failureReason) {
        $failureReason = Get-ExpiredDeadline $watchClock.Elapsed
        if ($failureReason) { break }
        try {
            $manifestCurrent = Get-Item -LiteralPath $manifestPath -Force
            if ($manifestCurrent.Length -ne $manifestLength -or $manifestCurrent.LastWriteTimeUtc.Ticks -ne $manifestLastWriteTicks) { throw 'ManifestChanged' }
            $currentLogInfo = Get-Item -LiteralPath $liveLogPath -Force
            if ($currentLogInfo.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'LogReplaced' }
            if ($currentLogInfo.CreationTimeUtc.Ticks -ne $logCreationUtc.Ticks) { throw 'LogReplaced' }
            [void](Assert-BoundIdentity $false)
            $events = @(Read-NewLogEvents)
        }
        catch {
            $knownReason = $_.Exception.Message
            if ($knownReason -notin @('ProcessExited', 'ManifestChanged', 'LogReplaced', 'LogTruncated', 'LogAppendLimit', 'LogLineLimit')) { $knownReason = 'MonitorError' }
            $failureReason = $knownReason
            $result.Error = $_.Exception.Message
            break
        }

        $failureReason = Get-ExpiredDeadline $watchClock.Elapsed
        if ($failureReason) { break }

        foreach ($event in $events) {
            $failureReason = Get-ExpiredDeadline $watchClock.Elapsed
            if ($failureReason) { break }
            $now = $watchClock.Elapsed
            switch ($event.Phase) {
                'invalid' { $failureReason = 'InvalidTelemetry' }
                'failed' { $failureReason = 'ProbeFailed' }
                'armed' {
                    if ($observedArmed) { $failureReason = 'InvalidTelemetry' }
                    else {
                        $observedArmed = $true
                        $armedAt = $now
                        $lastHeartbeatAt = $now
                    }
                }
                'sample' {
                    if (-not $observedArmed) { $failureReason = 'InvalidTelemetry' }
                    else {
                        $lastHeartbeatAt = $now
                        $maximumHorizontal = [Math]::Max($maximumHorizontal, $event.Horizontal)
                        $maximumVertical = [Math]::Max($maximumVertical, $event.Vertical)
                        if ($event.Horizontal -gt $MaxHorizontalMeters) { $failureReason = 'HorizontalLimit' }
                        elseif ($event.Vertical -gt $MaxVerticalMeters) { $failureReason = 'VerticalLimit' }
                    }
                }
                'finished' {
                    if (-not $observedArmed) { $failureReason = 'InvalidTelemetry' }
                    else {
                        $observedFinished = $true
                        $lastHeartbeatAt = $now
                    }
                }
            }
            if ($failureReason -or $observedFinished) { break }
        }

        if ($failureReason -or $observedFinished) { break }
        $failureReason = Get-ExpiredDeadline $watchClock.Elapsed
        if (-not $failureReason) { Start-Sleep -Milliseconds $PollMilliseconds }
    }

    if ($observedFinished) {
        Complete-Result 'Finished' 'Finished' $null
        Write-Output ([pscustomobject]$result)
        exit 0
    }

    Stop-BoundProcess
    Complete-Result 'Failed' $failureReason $result.Error
    Write-Output ([pscustomobject]$result)
    exit 3
}
catch {
    $caughtMessage = $_.Exception.Message
    if ($bound) {
        Stop-BoundProcess
        Complete-Result 'Failed' 'MonitorError' $caughtMessage
        if ($resultPath) { Write-Output ([pscustomobject]$result) }
        exit 4
    }
    Complete-Result 'Rejected' 'OwnershipRejected' $caughtMessage
    if ($resultPath) { Write-Output ([pscustomobject]$result) }
    else { Write-Error $caughtMessage }
    exit 2
}
finally {
    if ($logStream) { $logStream.Dispose() }
}
