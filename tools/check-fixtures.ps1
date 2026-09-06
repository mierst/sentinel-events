[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$LogPath,

    [ValidateSet('Admission', 'Recovery', 'Store')]
    [string]$Suite = 'Admission'
)

$ErrorActionPreference = 'Stop'
$resolvedLogPath = (Resolve-Path -LiteralPath $LogPath).Path
$expectedBySuite = @{
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
    throw "Script compilation error in $resolvedLogPath`: $($compileError.Trim())"
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
