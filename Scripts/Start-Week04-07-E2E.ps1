[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\Software\UE_5.8',

    [ValidateRange(5, 120)]
    [int]$SmokeSeconds = 8,

    [ValidateSet('Baseline', 'Lag100', 'Lag100Loss5', 'All')]
    [string]$Profile = 'All'
)

$ErrorActionPreference = 'Stop'
$runner = Join-Path $PSScriptRoot 'Start-Week01-Multiplayer.ps1'
$profiles = if ($Profile -eq 'All') {
    @('Baseline', 'Lag100', 'Lag100Loss5')
}
else {
    @($Profile)
}

$port = 7790
foreach ($networkProfile in $profiles) {
    Write-Host "Running Week 04-07 headless E2E profile: $networkProfile" -ForegroundColor Cyan
    & $runner `
        -EngineRoot $EngineRoot `
        -Map '/ExtractionOps/Maps/L_ExtractionTest' `
        -NumBots 0 `
        -Port $port `
        -Experience 'B_ExtractionExperience' `
        -AutomatedSmokeSeconds $SmokeSeconds `
        -NetworkProfile $networkProfile `
        -LogSubdirectory 'Week04-07' `
        -RequiredServerLogPatterns @(
            'event=raid_started',
            'event=inventory_component_ready',
            'event=raid_lifecycle_ready')
    ++$port
}

Write-Host "Week 04-07 E2E completed for: $($profiles -join ', ')" -ForegroundColor Green
