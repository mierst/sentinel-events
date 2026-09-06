[CmdletBinding()]
param(
    [string]$MakePboPath
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = [System.IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$buildRoot = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'build\@SentinelEvents'))
$stagingRoot = [System.IO.Path]::GetFullPath((Join-Path $buildRoot '.pbo-source'))
$addonsRoot = [System.IO.Path]::GetFullPath((Join-Path $buildRoot 'addons'))
$outputPath = [System.IO.Path]::GetFullPath((Join-Path $addonsRoot 'sentinel_events.pbo'))

function Assert-ContainedPathWithoutReparsePoint {
    param(
        [Parameter(Mandatory)]
        [string]$Path,

        [Parameter(Mandatory)]
        [string]$Boundary
    )

    $currentPath = [System.IO.Path]::GetFullPath($Path)
    $resolvedBoundary = [System.IO.Path]::GetFullPath($Boundary)
    $boundaryPrefix = $resolvedBoundary.TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar

    if (-not [string]::Equals($currentPath, $resolvedBoundary, [System.StringComparison]::OrdinalIgnoreCase) -and
        -not $currentPath.StartsWith($boundaryPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Path $currentPath is not contained by boundary $resolvedBoundary."
    }

    while ($true) {
        $item = Get-Item -LiteralPath $currentPath -Force -ErrorAction SilentlyContinue
        if ($item -and ($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint)) {
            throw "Refusing path through reparse point: $currentPath"
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

function Get-PermittedPackageFiles {
    param(
        [Parameter(Mandatory)]
        [string]$Root,

        [Parameter(Mandatory)]
        [string[]]$AllowedExtensions,

        [switch]$Optional
    )

    if (-not (Test-Path -LiteralPath $Root -PathType Container)) {
        if ($Optional) {
            return @()
        }
        throw "Required package source directory is missing: $Root"
    }

    Assert-ContainedPathWithoutReparsePoint -Path $Root -Boundary $repositoryRoot
    $files = [System.Collections.Generic.List[System.IO.FileInfo]]::new()
    $privateKeyExtensions = @('.biprivatekey', '.pem', '.pfx', '.p12', '.key')
    $profileDirectoryNames = @('profile', 'profiles', 'storage', 'logs')

    foreach ($item in Get-ChildItem -LiteralPath $Root -Force -Recurse) {
        if ($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
            throw "Package source contains a reparse point: $($item.FullName)"
        }
        if ($item.PSIsContainer) {
            if ($item.Name.ToLowerInvariant() -in $profileDirectoryNames) {
                throw "Package source contains a profile/state directory: $($item.FullName)"
            }
            continue
        }

        $extension = $item.Extension.ToLowerInvariant()
        if ($extension -in $privateKeyExtensions) {
            throw "Package source contains a private-key file: $($item.FullName)"
        }
        if ($extension -notin $AllowedExtensions) {
            throw "Unexpected package source file type '$extension': $($item.FullName)"
        }
        $files.Add($item)
    }

    return $files
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

$metadataPaths = @(
    (Join-Path $repositoryRoot 'config.cpp'),
    (Join-Path $repositoryRoot '$PBOPREFIX$')
)
$packageFiles = [System.Collections.Generic.List[System.IO.FileInfo]]::new()
foreach ($metadataPath in $metadataPaths) {
    Assert-ContainedPathWithoutReparsePoint -Path $metadataPath -Boundary $repositoryRoot
    $metadataFile = Get-Item -LiteralPath $metadataPath -Force
    if (-not $metadataFile.PSIsContainer) {
        $packageFiles.Add($metadataFile)
    }
    else {
        throw "Package metadata path is not a file: $metadataPath"
    }
}

$scriptsPath = Join-Path $repositoryRoot 'scripts'
foreach ($sourceFile in Get-PermittedPackageFiles -Root $scriptsPath -AllowedExtensions @('.c')) {
    $packageFiles.Add($sourceFile)
}

$layoutsPath = Join-Path $repositoryRoot 'layouts'
foreach ($layoutFile in Get-PermittedPackageFiles -Root $layoutsPath -AllowedExtensions @('.layout') -Optional) {
    $packageFiles.Add($layoutFile)
}

Assert-ContainedPathWithoutReparsePoint -Path $buildRoot -Boundary $repositoryRoot
Assert-ContainedPathWithoutReparsePoint -Path $stagingRoot -Boundary $buildRoot
Assert-ContainedPathWithoutReparsePoint -Path $addonsRoot -Boundary $buildRoot
Assert-ContainedPathWithoutReparsePoint -Path $outputPath -Boundary $buildRoot
if (Test-Path -LiteralPath $stagingRoot) {
    Remove-Item -LiteralPath $stagingRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $stagingRoot -Force | Out-Null
New-Item -ItemType Directory -Path $addonsRoot -Force | Out-Null

foreach ($packageFile in $packageFiles) {
    $repositoryPrefix = $repositoryRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $packageFile.FullName.StartsWith($repositoryPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Package source file is outside the repository: $($packageFile.FullName)"
    }
    $relativePath = $packageFile.FullName.Substring($repositoryPrefix.Length)
    $destinationPath = Join-Path $stagingRoot $relativePath
    New-Item -ItemType Directory -Path (Split-Path -Parent $destinationPath) -Force | Out-Null
    Copy-Item -LiteralPath $packageFile.FullName -Destination $destinationPath
}

Assert-ContainedPathWithoutReparsePoint -Path $addonsRoot -Boundary $buildRoot
Assert-ContainedPathWithoutReparsePoint -Path $outputPath -Boundary $buildRoot
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

Assert-ContainedPathWithoutReparsePoint -Path $stagingRoot -Boundary $buildRoot
Remove-Item -LiteralPath $stagingRoot -Recurse -Force
Write-Host "Built $outputPath"
