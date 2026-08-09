[CmdletBinding()]
param(
    [string]$EngineRoot = '',

    [string]$Map = '/ShooterMaps/Maps/L_Convolution_Blockout',

    [ValidateRange(0, 32)]
    [int]$NumBots = 5,

    [ValidateRange(640, 7680)]
    [int]$ResX = 1280,

    [ValidateRange(360, 4320)]
    [int]$ResY = 720,

    [switch]$ValidateOnly
)

$ErrorActionPreference = 'Stop'

function Resolve-EngineRoot {
    param(
        [string]$RequestedRoot,
        [Parameter(Mandatory = $true)][string]$EngineAssociation
    )

    $candidates = [System.Collections.Generic.List[string]]::new()
    if ($RequestedRoot) {
        $candidates.Add($RequestedRoot)
    }

    $candidates.Add("D:\Software\UE_$EngineAssociation")
    $candidates.Add((Join-Path $env:ProgramFiles "Epic Games\UE_$EngineAssociation"))
    if (${env:ProgramFiles(x86)}) {
        $candidates.Add((Join-Path ${env:ProgramFiles(x86)} "Epic Games\UE_$EngineAssociation"))
    }

    $buildsKey = 'HKCU:\Software\Epic Games\Unreal Engine\Builds'
    if (Test-Path -LiteralPath $buildsKey) {
        $properties = Get-ItemProperty -LiteralPath $buildsKey
        foreach ($property in $properties.PSObject.Properties) {
            if ($property.Name -notmatch '^PS' -and $property.Value -is [string]) {
                $candidates.Add([string]$property.Value)
            }
        }
    }

    $manifestRoot = Join-Path $env:ProgramData 'Epic\EpicGamesLauncher\Data\Manifests'
    if (Test-Path -LiteralPath $manifestRoot) {
        foreach ($manifestFile in Get-ChildItem -LiteralPath $manifestRoot -Filter '*.item' -File) {
            try {
                $manifest = Get-Content -LiteralPath $manifestFile.FullName -Raw -Encoding UTF8 |
                    ConvertFrom-Json
                $installLeaf = if ($manifest.InstallLocation) {
                    Split-Path ([string]$manifest.InstallLocation) -Leaf
                }
                if ($manifest.InstallLocation -and
                    (($manifest.AppName -match "UE_$([regex]::Escape($EngineAssociation))") -or
                        ($installLeaf -eq "UE_$EngineAssociation"))) {
                    $candidates.Add([string]$manifest.InstallLocation)
                }
            }
            catch {
                Write-Verbose "Skipping unreadable Epic manifest: $($manifestFile.FullName)"
            }
        }
    }

    foreach ($candidate in $candidates | Select-Object -Unique) {
        if (-not $candidate) {
            continue
        }

        $candidateRoot = [System.IO.Path]::GetFullPath($candidate.Trim('"').TrimEnd('\'))
        $editor = Join-Path $candidateRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
        if (Test-Path -LiteralPath $editor -PathType Leaf) {
            return $candidateRoot
        }
    }

    throw "UE $EngineAssociation was not found. Specify the engine root with -EngineRoot."
}

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$projectFile = Join-Path $repoRoot 'LyraStarterGame.uproject'
if (-not (Test-Path -LiteralPath $projectFile -PathType Leaf)) {
    throw "Project file not found: $projectFile"
}

if (-not $Map.StartsWith('/')) {
    throw "Map must be an Unreal package path beginning with '/': $Map"
}

$projectMetadata = Get-Content -LiteralPath $projectFile -Raw -Encoding UTF8 | ConvertFrom-Json
$engineAssociation = [string]$projectMetadata.EngineAssociation
if (-not $engineAssociation) {
    throw 'LyraStarterGame.uproject does not declare EngineAssociation.'
}

$resolvedEngineRoot = Resolve-EngineRoot -RequestedRoot $EngineRoot -EngineAssociation $engineAssociation
$editorExe = Join-Path $resolvedEngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$logRoot = Join-Path $repoRoot 'Saved\Logs\Week01'
$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$logFile = Join-Path $logRoot "LocalMatch-$timestamp.log"
$mapUrl = "${Map}?NumBots=$NumBots"

Write-Host 'Week 01: visible local match with bots' -ForegroundColor Cyan
Write-Host "Engine : $resolvedEngineRoot"
Write-Host "Project: $projectFile"
Write-Host "Map    : $mapUrl"
Write-Host "Window : ${ResX}x${ResY}"
Write-Host "Log    : $logFile"

if ($ValidateOnly) {
    Write-Host 'Validation passed. UE was not launched.' -ForegroundColor Green
    return
}

New-Item -ItemType Directory -Path $logRoot -Force | Out-Null

$arguments = @(
    "`"$projectFile`"",
    $mapUrl,
    '-game',
    '-log',
    '-windowed',
    "-ResX=$ResX",
    "-ResY=$ResY",
    '-NoSplash',
    "-AbsLog=`"$logFile`""
)

Write-Host ''
Write-Host 'Check WASD, mouse look, Space, fire, ADS, reload, HUD, death, and respawn.' -ForegroundColor Yellow
Write-Host 'Close the game window to finish the script.' -ForegroundColor DarkGray

$gameProcess = Start-Process `
    -FilePath $editorExe `
    -ArgumentList $arguments `
    -PassThru `
    -Wait

if ($gameProcess.ExitCode -ne 0) {
    throw "The local match exited with code $($gameProcess.ExitCode). Check: $logFile"
}

Write-Host "The local match closed normally. Log: $logFile" -ForegroundColor Green
