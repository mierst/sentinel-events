[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$LogPath,

    [ValidateSet('Admission')]
    [string]$Suite = 'Admission'
)

$ErrorActionPreference = 'Stop'
$resolvedLogPath = (Resolve-Path -LiteralPath $LogPath).Path
$expectedBySuite = @{
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
$fixturePattern = '\[SEV\] fixture (?<name>[a-z0-9-]+): expected=(?<expected>true|false) got=(?<got>true|false) (?<result>PASS|FAIL)\s*$'
$compileErrorPattern = '(?i)SCRIPT\s+\(E\):|Can''t compile\s+"[^"]+"\s+script module|Compilation error|FIX-ME:\s+Syntax error'
$logLines = @(Get-Content -LiteralPath $resolvedLogPath)

$compileError = $logLines | Where-Object { $_ -match $compileErrorPattern } | Select-Object -First 1
if ($compileError) {
    throw "Script compilation error in $resolvedLogPath`: $($compileError.Trim())"
}

foreach ($line in $logLines) {
    $match = [regex]::Match($line, $fixturePattern)
    if (-not $match.Success) {
        continue
    }

    $name = $match.Groups['name'].Value
    if ($name -notin $expectedNames) {
        throw "Unexpected $Suite fixture '$name' in $resolvedLogPath."
    }
    if ($results.ContainsKey($name)) {
        throw "Duplicate $Suite fixture '$name' in $resolvedLogPath."
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
