[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$LogPath,

    [ValidateSet('Admission', 'Recovery', 'Store', 'GuardSpawn')]
    [string]$Suite = 'Admission'
)

$ErrorActionPreference = 'Stop'
$resolvedLogPath = (Resolve-Path -LiteralPath $LogPath).Path
$expectedBySuite = @{
    GuardSpawn = @('config-disabled', 'config-missing', 'config-defaults', 'config-destruction-disabled', 'config-valid', 'config-ready-bound', 'config-roster-bound', 'config-roster-order', 'config-fixed-tick', 'config-fixed-budget', 'config-fixed-attempts', 'config-admin-format', 'config-fallback-required', 'config-kit-bounded', 'config-ready-high', 'config-minimum-low', 'config-preparation-low', 'config-preparation-high', 'config-countdown-low', 'config-countdown-high', 'config-demonstration-low', 'config-demonstration-high', 'config-quiet-low', 'config-quiet-high', 'config-radius-low', 'config-radius-high', 'config-separation-low', 'config-separation-high', 'config-return-radius-low', 'config-return-radius-high', 'config-fixed-refresh', 'config-schema', 'config-position-invalid', 'config-admin-required', 'config-loaded-classes', 'config-default-offers-disabled', 'config-invalid-file-rejected', 'config-invalid-file-preserved', 'guard-acquire', 'guard-duplicate', 'guard-preserves-damage', 'guard-other-token', 'guard-other-player', 'guard-stale-release', 'guard-held-after-stale', 'guard-release', 'guard-duplicate-release', 'guard-reacquire-released-token', 'guard-preserves-protection', 'guard-empty-token', 'spawn-dry-clear', 'spawn-shoreline', 'spawn-slope', 'spawn-obstacle', 'spawn-minimum-distance', 'spawn-separation-boundary', 'spawn-area-uniform', 'spawn-zero-radius', 'spawn-pending', 'spawn-zero-budget', 'spawn-eight-budget', 'spawn-still-pending', 'spawn-attempt-cap', 'spawn-no-unsafe-last', 'spawn-cap-no-retry', 'spawn-ready', 'spawn-ready-no-retry', 'spawn-fallback-failure', 'spawn-fallback-reserved-attempt', 'spawn-shared-tick-budget', 'spawn-sixteen-shared-requests', 'spawn-invalid-origin', 'spawn-invalid-fallback', 'guard-unknown-movement-baseline', 'guard-known-movement-baseline', 'guard-transient-baseline-rejected', 'guard-movement-not-proven', 'guard-hands-juncture-denied', 'guard-hands-all-locations', 'guard-hands-server-path-preserved', 'guard-hands-remote-path-preserved', 'guard-hands-unguarded-super')
    Store = @('store-manifest', 'store-manifest-immutable', 'store-first-write', 'store-missing-member', 'store-second-member', 'store-roundtrip', 'store-fields', 'store-next-generation', 'store-latest', 'store-latest-fields', 'store-repeat-sequence', 'store-token-change', 'store-unsafe-run', 'store-unknown-member', 'store-path-encoding', 'store-keeps-previous', 'store-truncated-newer', 'store-failure-clears-output', 'store-oversized', 'store-generation-gap', 'store-contradictory-generation', 'store-valid-after-fault-removal', 'store-unknown-schema', 'store-invalid-position', 'store-lost-member')
    Recovery = @('recovery-invalid', 'recovery-token-mismatch', 'recovery-missing-proof', 'recovery-untouched', 'recovery-needs-return', 'recovery-done', 'recovery-contradictory', 'recovery-return-without-mutation', 'recovery-return-and-untouched')
    Admission = @(
        'eligible',
        'vehicle',
        'unconscious',
        'restrained',
        'recent-combat',
        'pending-session',
        'stale-offer',
        'deadline-equality',
        'below-minimum',
        'invalid-arena',
        'invalid-kit'
    )
}

$expectedNames = $expectedBySuite[$Suite]
$results = @{}
$seen = @{}
$fixturePattern = '\[SEV\] fixture (?<name>[a-z0-9-]+): expected=(?<expected>true|false|[0-9]+) got=(?<got>true|false|[0-9]+) (?<result>PASS|FAIL)\s*$'
$compileErrorPattern = '(?i)SCRIPT\s+\(E\):|Can''t compile\s+"[^"]+"\s+script module|Compilation error|FIX-ME:\s+Syntax error'
$logLines = @(Get-Content -LiteralPath $resolvedLogPath)

$compileError = $logLines | Where-Object { $_ -match $compileErrorPattern } | Select-Object -First 1
if ($compileError) {
    throw "Native script error in $resolvedLogPath`: $($compileError.Trim())"
}

foreach ($line in $logLines) {
    if ($line -match '\[SEV\].*\bFAIL\b') {
        throw "Failed fixture output in $resolvedLogPath`: $($line.Trim())"
    }
    $match = [regex]::Match($line, $fixturePattern)
    if (-not $match.Success) {
        if ($line -match '\[SEV\] fixture ') {
            throw "Malformed fixture output in $resolvedLogPath`: $($line.Trim())"
        }
        continue
    }

    $name = $match.Groups['name'].Value
    if ($match.Groups['result'].Value -eq 'FAIL' -or $match.Groups['expected'].Value -ne $match.Groups['got'].Value) {
        throw "Failed fixture '$name' in $resolvedLogPath."
    }
    if ($seen.ContainsKey($name)) {
        throw "Duplicate fixture '$name' in $resolvedLogPath."
    }
    $seen[$name] = $true
    if ($name -notin $expectedNames) {
        if ($name -in @($expectedBySuite.Values | ForEach-Object { $_ })) { continue }
        throw "Unexpected $Suite fixture '$name' in $resolvedLogPath."
    }
    $expected = $match.Groups['expected'].Value
    $got = $match.Groups['got'].Value
    $status = $match.Groups['result'].Value
    $results[$name] = $status -eq 'PASS' -and $expected -eq $got
}

$failures = [System.Collections.Generic.List[string]]::new()
foreach ($name in $expectedNames) {
    if (-not $results.ContainsKey($name)) {
        $failures.Add("Missing $Suite fixture '$name'.")
    }
    elseif (-not $results[$name]) {
        $failures.Add("Failed $Suite fixture '$name'.")
    }
}

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Host "$Suite fixtures passed: $($expectedNames.Count)/$($expectedNames.Count) in $resolvedLogPath"
