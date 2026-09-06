[CmdletBinding()]
param(
    [string]$MakePboPath
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$buildRoot = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'build\@SentinelEvents'))
$stagingRoot = [System.IO.Path]::GetFullPath((Join-Path $buildRoot '.pbo-source'))
$addonsRoot = Join-Path $buildRoot 'addons'
$outputPath = Join-Path $addonsRoot 'sentinel_events.pbo'

$buildRootPrefix = $buildRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
if (-not $stagingRoot.StartsWith($buildRootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing staging path outside $buildRoot`: $stagingRoot"
}

function Assert-NoReparsePointInPath {
    param(
        [Parameter(Mandatory)]
        [string]$Path,

        [Parameter(Mandatory)]
        [string]$Boundary
    )

    $currentPath = [System.IO.Path]::GetFullPath($Path)
    $resolvedBoundary = [System.IO.Path]::GetFullPath($Boundary)

    while ($true) {
        $item = Get-Item -LiteralPath $currentPath -Force -ErrorAction SilentlyContinue
        if ($item -and ($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint)) {
            throw "Refusing recursive removal through reparse point: $currentPath"
        }
        if ([string]::Equals($currentPath, $resolvedBoundary, [System.StringComparison]::OrdinalIgnoreCase)) {
            break
        }

        $parentPath = [System.IO.Path]::GetFullPath((Split-Path -Parent $currentPath))
        if ([string]::Equals($parentPath, $currentPath, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Path $Path is not contained by boundary $Boundary."
        }
        $currentPath = $parentPath
    }
}

if ($MakePboPath) {
    $resolvedMakePbo = (Resolve-Path -LiteralPath $MakePboPath).Path
}
else {
    $makePboCommand = Get-Command 'MakePbo.exe' -ErrorAction SilentlyContinue
    if (-not $makePboCommand) {
        throw 'MakePbo.exe was not found on PATH. Pass -MakePboPath with its full path.'
    }
    $resolvedMakePbo = $makePboCommand.Source
}

& (Join-Path $PSScriptRoot 'check-source.ps1')
if (-not $?) {
    throw 'Source checks failed.'
}

Assert-NoReparsePointInPath -Path $stagingRoot -Boundary $repositoryRoot
if (Test-Path -LiteralPath $stagingRoot) {
    Remove-Item -LiteralPath $stagingRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $stagingRoot -Force | Out-Null
New-Item -ItemType Directory -Path $addonsRoot -Force | Out-Null

Copy-Item -LiteralPath (Join-Path $repositoryRoot 'config.cpp') -Destination $stagingRoot
Copy-Item -LiteralPath (Join-Path $repositoryRoot '$PBOPREFIX$') -Destination $stagingRoot
Copy-Item -LiteralPath (Join-Path $repositoryRoot 'scripts') -Destination $stagingRoot -Recurse

$layoutsPath = Join-Path $repositoryRoot 'layouts'
if (Test-Path -LiteralPath $layoutsPath -PathType Container) {
    Copy-Item -LiteralPath $layoutsPath -Destination $stagingRoot -Recurse
}

if (Test-Path -LiteralPath $outputPath) {
    Remove-Item -LiteralPath $outputPath -Force
}

Write-Host "MakePbo: $resolvedMakePbo"
Write-Host "Source:  $stagingRoot"
Write-Host "Output:  $outputPath"

& $resolvedMakePbo '-DPW' '-@=SentinelEvents' $stagingRoot $outputPath
if ($LASTEXITCODE -ne 0) {
    throw "MakePbo failed with exit code $LASTEXITCODE."
}
if (-not (Test-Path -LiteralPath $outputPath -PathType Leaf)) {
    throw "MakePbo did not create $outputPath."
}

Remove-Item -LiteralPath $stagingRoot -Recurse -Force
Write-Host "Built $outputPath"
