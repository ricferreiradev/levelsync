[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string] $BuildDirectory,

    [Parameter(Mandatory = $true)]
    [string] $OutputDirectory,

    [Parameter(Mandatory = $true)]
    [ValidatePattern('^\d+\.\d+\.\d+([.-][0-9A-Za-z.-]+)?$')]
    [string] $Version
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$buildRoot = [IO.Path]::GetFullPath($BuildDirectory)
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
$standaloneSource = Join-Path $buildRoot 'LevelSync_artefacts\Release\Standalone\LevelSync.exe'
$vstSource = Join-Path $buildRoot 'LevelSync_artefacts\Release\VST3\LevelSync.vst3'

if (-not (Test-Path -LiteralPath $standaloneSource -PathType Leaf)) {
    throw "Standalone executable not found: $standaloneSource"
}
if (-not (Test-Path -LiteralPath $vstSource -PathType Container)) {
    throw "VST3 bundle not found: $vstSource"
}

New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
$stagingRoot = Join-Path $outputRoot ('.staging-LevelSync-' + $Version)

# Destructive cleanup is restricted to the explicitly named staging directory.
$resolvedOutput = [IO.Path]::GetFullPath($outputRoot).TrimEnd('\') + '\'
$resolvedStaging = [IO.Path]::GetFullPath($stagingRoot)
if (-not $resolvedStaging.StartsWith($resolvedOutput, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Unsafe staging path: $resolvedStaging"
}
if (Test-Path -LiteralPath $resolvedStaging) {
    Remove-Item -LiteralPath $resolvedStaging -Recurse -Force
}

$standaloneStage = Join-Path $resolvedStaging 'standalone'
$vstStage = Join-Path $resolvedStaging 'vst3'
New-Item -ItemType Directory -Path $standaloneStage, $vstStage -Force | Out-Null

$legalFiles = @('LICENSE', 'NOTICE', 'THIRD_PARTY_NOTICES.md', 'README.md')
foreach ($file in $legalFiles) {
    $source = Join-Path $projectRoot $file
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "Required distribution file not found: $source"
    }
    Copy-Item -LiteralPath $source -Destination $standaloneStage
    Copy-Item -LiteralPath $source -Destination $vstStage
}

Copy-Item -LiteralPath $standaloneSource -Destination $standaloneStage
Copy-Item -LiteralPath $vstSource -Destination $vstStage -Recurse

$standaloneZip = Join-Path $outputRoot "LevelSync-$Version-Standalone-Windows-x64.zip"
$vstZip = Join-Path $outputRoot "LevelSync-$Version-VST3-Windows-x64.zip"
foreach ($archive in @($standaloneZip, $vstZip)) {
    if (Test-Path -LiteralPath $archive) {
        Remove-Item -LiteralPath $archive -Force
    }
}

Compress-Archive -Path (Join-Path $standaloneStage '*') -DestinationPath $standaloneZip -CompressionLevel Optimal
Compress-Archive -Path (Join-Path $vstStage '*') -DestinationPath $vstZip -CompressionLevel Optimal
Remove-Item -LiteralPath $resolvedStaging -Recurse -Force

Write-Output "Standalone package: $standaloneZip"
Write-Output "VST3 package: $vstZip"
