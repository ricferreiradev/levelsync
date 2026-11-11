[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string] $BuildDirectory,

    [Parameter(Mandatory = $true)]
    [string] $OutputDirectory,

    [Parameter(Mandatory = $true)]
    [ValidatePattern('^\d+\.\d+\.\d+([.-][0-9A-Za-z.-]+)?$')]
    [string] $Version,

    [string] $RepositoryUrl = 'https://github.com/',
    [string] $IsccPath
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$buildRoot = [IO.Path]::GetFullPath($BuildDirectory)
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
$vstBundle = Join-Path $buildRoot 'LevelSync_artefacts\Release\VST3\LevelSync.vst3'
$scriptPath = Join-Path $PSScriptRoot 'LevelSync.iss'

if (-not (Test-Path -LiteralPath $vstBundle -PathType Container)) {
    throw "VST3 bundle not found: $vstBundle"
}
New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null

if ([string]::IsNullOrWhiteSpace($IsccPath)) {
    $command = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($command) {
        $IsccPath = $command.Source
    } else {
        $candidates = @(
            "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
            "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
            "$env:ProgramFiles\Inno Setup 7\ISCC.exe"
        )
        $IsccPath = $candidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
    }
}

if ([string]::IsNullOrWhiteSpace($IsccPath) -or -not (Test-Path -LiteralPath $IsccPath -PathType Leaf)) {
    throw 'ISCC.exe not found. Install Inno Setup or pass -IsccPath.'
}

$arguments = @(
    "/DMyAppVersion=$Version",
    "/DBuildRoot=$buildRoot",
    "/DProjectRoot=$projectRoot",
    "/DOutputDir=$outputRoot",
    "/DRepositoryUrl=$RepositoryUrl",
    $scriptPath
)

& $IsccPath @arguments
if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup failed with exit code $LASTEXITCODE"
}

$installer = Join-Path $outputRoot "LevelSync-$Version-VST3-Setup-Windows-x64.exe"
if (-not (Test-Path -LiteralPath $installer -PathType Leaf)) {
    throw "Expected installer was not generated: $installer"
}
Write-Output "Installer: $installer"
