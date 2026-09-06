[CmdletBinding()]
param(
    [string]$SourceRoot = (Join-Path (Split-Path -Parent $PSScriptRoot) 'scripts')
)

$ErrorActionPreference = 'Stop'
$resolvedSourceRoot = (Resolve-Path -LiteralPath $SourceRoot).Path
$sourceFiles = @(Get-ChildItem -LiteralPath $resolvedSourceRoot -Filter '*.c' -File -Recurse)
$errors = [System.Collections.Generic.List[string]]::new()
$hookClasses = @{}

foreach ($sourceFile in $sourceFiles) {
    $bytes = [System.IO.File]::ReadAllBytes($sourceFile.FullName)
    if ($bytes.Where({ $_ -gt 127 }).Count -gt 0) {
        $errors.Add("Non-ASCII Enforce source: $($sourceFile.FullName)")
    }

    $text = [System.IO.File]::ReadAllText($sourceFile.FullName)
    if ($text -match '(?m)^\s*\+\s*"') {
        $errors.Add("Leading-plus string continuation: $($sourceFile.FullName)")
    }

    foreach ($match in [regex]::Matches($text, '(?m)^\s*modded\s+class\s+(?<name>[A-Za-z_][A-Za-z0-9_]*)\b')) {
        $className = $match.Groups['name'].Value
        if (-not $hookClasses.ContainsKey($className)) {
            $hookClasses[$className] = [System.Collections.Generic.List[string]]::new()
        }
        $hookClasses[$className].Add($sourceFile.FullName)
    }
}

foreach ($className in $hookClasses.Keys) {
    if ($hookClasses[$className].Count -gt 1) {
        $locations = $hookClasses[$className] -join ', '
        $errors.Add("Duplicate owned hook block for $className`: $locations")
    }
}

if ($errors.Count -gt 0) {
    $errors | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Host "Source checks passed for $($sourceFiles.Count) Enforce file(s)."
