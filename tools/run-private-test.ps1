[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ServerExe,
    [Parameter(Mandatory)][string]$ServerConfig,
    [Parameter(Mandatory)][string]$MissionPath,
    [Parameter(Mandatory)][string]$ProfilePath,
    [Parameter(Mandatory)][string]$ModPath,
    [ValidateRange(1024, 65531)][int]$Port = 2402,
    [ValidateRange(10, 180)][int]$TimeoutSeconds = 90,
    [string]$Suite = 'Admission',
    [string]$CompletionPattern = '\[SEV\] fixture invalid-kit:',
    [switch]$KeepRunning
)

$ErrorActionPreference = 'Stop'
[void][regex]::new($CompletionPattern)
$repoRoot = [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$fixtureChecker = Join-Path $PSScriptRoot 'check-fixtures.ps1'
$suiteValidators = @((Get-Command $fixtureChecker).Parameters['Suite'].Attributes | Where-Object { $_ -is [System.Management.Automation.ValidateSetAttribute] })
if ($suiteValidators.Count -ne 1 -or $Suite -notin $suiteValidators[0].ValidValues) {
    throw "Unsupported fixture suite '$Suite' for the installed checker"
}
$testRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot 'build/private-test'))
$testPrefix = $testRoot.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar

function Assert-NoReparseAncestor([string]$Path) {
    $cursor = [IO.Path]::GetFullPath($Path)
    while ($cursor) {
        if (Test-Path -LiteralPath $cursor) {
            if ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Reparse-point path is not permitted: $cursor"
            }
        }
        $parent = Split-Path -Parent $cursor
        if ($parent -eq $cursor) { break }
        $cursor = $parent
    }
}

function Resolve-TestPath([string]$Path) {
    $resolved = [IO.Path]::GetFullPath($Path)
    if (-not $resolved.StartsWith($testPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Config, mission and profile must be isolated under $testRoot"
    }
    Assert-NoReparseAncestor $resolved
    return $resolved
}

function Save-LogSnapshot([string]$Source, [string]$Destination) {
    $inputStream = $null
    $outputStream = $null
    try {
        $inputStream = [IO.File]::Open($Source, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
        $outputStream = [IO.File]::Open($Destination, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
        $inputStream.CopyTo($outputStream)
    }
    finally {
        if ($outputStream) { $outputStream.Dispose() }
        if ($inputStream) { $inputStream.Dispose() }
    }
}

function Stop-OwnedTestServer($Process) {
    if ($Process -and -not $Process.HasExited) {
        Stop-Process -InputObject $Process -ErrorAction Stop
        if (-not $Process.WaitForExit(5000)) { throw 'Owned server has not exited after stop request' }
    }
}

$serverPath = (Resolve-Path -LiteralPath $ServerExe).Path
if (-not (Test-Path -LiteralPath $serverPath -PathType Leaf)) { throw 'ServerExe must be a file' }
$configPath = Resolve-TestPath $ServerConfig
$missionDirectory = Resolve-TestPath $MissionPath
$profileDirectory = Resolve-TestPath $ProfilePath
$modDirectory = (Resolve-Path -LiteralPath $ModPath).Path
Assert-NoReparseAncestor $modDirectory
if (-not (Test-Path -LiteralPath $configPath -PathType Leaf)) { throw 'Missing isolated server config' }
if (-not (Test-Path -LiteralPath (Join-Path $missionDirectory 'init.c') -PathType Leaf)) { throw 'Missing isolated mission init.c' }
foreach ($missionEntry in Get-ChildItem -LiteralPath $missionDirectory -Force -Recurse) {
    if ($missionEntry.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Mission contains a reparse point' }
}
if (Test-Path -LiteralPath $profileDirectory) { throw 'Use a new profile directory for each run; existing profiles are refused' }
foreach ($argumentPath in @($configPath, $missionDirectory, $profileDirectory, $modDirectory)) {
    if ($argumentPath -match '[\s";]') {
        throw 'This verified test launcher requires config, mission, profile and mod paths without whitespace, quotes or semicolons'
    }
}
$pboPath = Join-Path $modDirectory 'addons/sentinel_events.pbo'
Assert-NoReparseAncestor $pboPath
if (-not (Test-Path -LiteralPath $pboPath -PathType Leaf)) { throw 'Build sentinel_events.pbo first' }
if (@(Get-ChildItem -LiteralPath (Split-Path -Parent $pboPath) -Filter '*.pbo' -File).Count -ne 1) { throw 'Standalone test requires exactly one PBO in the mod addons directory' }
$pboHash = (Get-FileHash -LiteralPath $pboPath -Algorithm SHA256).Hash
$serverHash = (Get-FileHash -LiteralPath $serverPath -Algorithm SHA256).Hash
New-Item -ItemType Directory -Path $profileDirectory | Out-Null
$manifestPath = Join-Path $profileDirectory 'run.json'
$record = [ordered]@{
    SchemaVersion = 1
    RunId = [Guid]::NewGuid().ToString('N')
    StartedUtc = [DateTime]::UtcNow.ToString('o')
    CompletedUtc = $null
    ServerExe = $serverPath
    ServerSha256 = $serverHash
    PboPath = $pboPath
    PboSha256 = $pboHash
    ProfilePath = $profileDirectory
    MissionPath = $missionDirectory
    ServerConfig = $configPath
    Pid = $null
    Suite = $Suite
    LogPath = $null
    LiveLogPath = $null
    LogSha256 = $null
    Result = 'Starting'
    Error = $null
}
$record | ConvertTo-Json | Set-Content -LiteralPath $manifestPath -Encoding utf8
$ownedServer = $null
try {
    $launchArguments = @(
        ('-config=' + $configPath), ('-port=' + $Port),
        ('-profiles=' + $profileDirectory), ('-mod=' + $modDirectory),
        ('-mission=' + $missionDirectory), '-doLogs', '-adminLog',
        '-freezeCheck', '-NO_GUI', '-limitFPS=100'
    )
    $ownedServer = Start-Process -FilePath $serverPath -WorkingDirectory (Split-Path -Parent $serverPath) -ArgumentList $launchArguments -PassThru -WindowStyle Hidden -RedirectStandardOutput (Join-Path $profileDirectory 'stdout.txt') -RedirectStandardError (Join-Path $profileDirectory 'stderr.txt')
    $record.Pid = $ownedServer.Id
    $record.Result = 'Running'
    $record | ConvertTo-Json | Set-Content -LiteralPath $manifestPath -Encoding utf8
    Write-Host "Owned test PID $($ownedServer.Id); evidence $manifestPath"
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    $completed = $false
    while ([DateTime]::UtcNow -lt $deadline) {
        $scriptLogs = @(Get-ChildItem -LiteralPath $profileDirectory -Filter 'script_*.log' -File)
        if ($scriptLogs.Count -gt 1) { throw 'Multiple script logs in a fresh run; evidence is ambiguous' }
        if ($scriptLogs.Count -eq 1) {
            $record.LogPath = $scriptLogs[0].FullName
            $logText = Get-Content -LiteralPath $record.LogPath -Raw
            if ($logText -match 'SCRIPT\s+\(E\):|Can.t compile|FIX-ME:\s+Syntax error') { throw 'Native script error detected; see the captured script log' }
            if ($logText -match $CompletionPattern) { $completed = $true; break }
        }
        if ($ownedServer.HasExited) { throw "Owned test server exited with code $($ownedServer.ExitCode) before fixture completion" }
        Start-Sleep -Milliseconds 500
    }
    if (-not $completed) { throw 'Timed out before fixture completion' }
    if ((Get-FileHash -LiteralPath $pboPath -Algorithm SHA256).Hash -ne $pboHash) { throw 'PBO changed during the run' }
    $record.LiveLogPath = $record.LogPath
    $record.LogPath = Join-Path $profileDirectory 'fixture-evidence.log'
    Save-LogSnapshot $record.LiveLogPath $record.LogPath
    & $fixtureChecker -LogPath $record.LogPath -Suite $Suite
    if (-not $?) { throw 'Fixture checker failed' }
    if ($ownedServer.HasExited) { throw "Owned server exited with code $($ownedServer.ExitCode) before run completion" }
    $record.Result = 'Passed'
}
catch {
    $record.Result = 'Failed'
    $record.Error = $_.Exception.Message
    throw
}
finally {
    try {
        if ($record.Result -eq 'Passed' -and $ownedServer.HasExited) {
            throw "Owned server exited with code $($ownedServer.ExitCode) before evidence finalization"
        }
        if (-not $KeepRunning -or $record.Result -ne 'Passed') {
            Stop-OwnedTestServer $ownedServer
        }
        if ($record.LogPath -and (Test-Path -LiteralPath $record.LogPath)) {
            $record.LogSha256 = (Get-FileHash -LiteralPath $record.LogPath -Algorithm SHA256).Hash
        }
        if ($KeepRunning -and $record.Result -eq 'Passed' -and $ownedServer.HasExited) {
            throw "Owned server exited with code $($ownedServer.ExitCode) before retention"
        }
        $record.CompletedUtc = [DateTime]::UtcNow.ToString('o')
        $record | ConvertTo-Json | Set-Content -LiteralPath $manifestPath -Encoding utf8
    }
    catch {
        $finalizationError = $_
        $record.Result = 'Failed'
        $record.Error = $_.Exception.Message
        try { Stop-OwnedTestServer $ownedServer }
        catch { $record.Error += '; cleanup failed: ' + $_.Exception.Message }
        $record.CompletedUtc = [DateTime]::UtcNow.ToString('o')
        try { $record | ConvertTo-Json | Set-Content -LiteralPath $manifestPath -Encoding utf8 }
        catch { Write-Warning ('Could not record final failure: ' + $_.Exception.Message) }
        throw $finalizationError
    }
}
Write-Output ([pscustomobject]$record)
