[CmdletBinding()]
param(
    [string]$EngineRoot = '',

    [string]$Map = '/ShooterMaps/Maps/L_Convolution_Blockout',

    [ValidateRange(0, 32)]
    [int]$NumBots = 2,

    [ValidateRange(1024, 65535)]
    [int]$Port = 7777,

    [ValidateRange(640, 3840)]
    [int]$ClientResX = 900,

    [ValidateRange(360, 2160)]
    [int]$ClientResY = 600,

    [ValidateRange(15, 300)]
    [int]$ReadyTimeoutSeconds = 120,

    [string]$Experience = '',

    [ValidateRange(0, 600)]
    [int]$AutomatedSmokeSeconds = 0,

    [switch]$ValidateOnly
)

$ErrorActionPreference = 'Stop'
$serverProcess = $null
$clientProcesses = [System.Collections.Generic.List[System.Diagnostics.Process]]::new()

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

function Stop-OwnedProcess {
    param([System.Diagnostics.Process]$Process)

    if (-not $Process) {
        return
    }

    $liveProcess = Get-Process -Id $Process.Id -ErrorAction SilentlyContinue
    if (-not $liveProcess) {
        return
    }

    if ($liveProcess.MainWindowHandle -ne 0) {
        $null = $liveProcess.CloseMainWindow()
        if ($liveProcess.WaitForExit(5000)) {
            return
        }
    }

    Stop-Process -Id $liveProcess.Id -ErrorAction SilentlyContinue
}

function Wait-ForLogPattern {
    param(
        [Parameter(Mandatory = $true)][string]$LogFile,
        [Parameter(Mandatory = $true)][string]$Pattern,
        [Parameter(Mandatory = $true)][int]$TimeoutSeconds,
        [System.Diagnostics.Process]$Process
    )

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        if ($Process -and $Process.HasExited) {
            return $false
        }

        if ((Test-Path -LiteralPath $LogFile -PathType Leaf) -and
            (Select-String -LiteralPath $LogFile -Pattern $Pattern -Quiet)) {
            return $true
        }

        Start-Sleep -Seconds 1
    }

    return $false
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
$serverLog = Join-Path $logRoot "Multiplayer-Server-$timestamp.log"
$client1Log = Join-Path $logRoot "Multiplayer-Client1-$timestamp.log"
$client2Log = Join-Path $logRoot "Multiplayer-Client2-$timestamp.log"
$mapUrl = "${Map}?NumBots=$NumBots"
if ($Experience) {
    $mapUrl += "?Experience=$Experience"
}

Write-Host 'Week 01: Dedicated Server plus two visible clients' -ForegroundColor Cyan
Write-Host "Engine : $resolvedEngineRoot"
Write-Host "Project: $projectFile"
Write-Host "Map    : $mapUrl"
Write-Host "Port   : $Port"
Write-Host "Logs   : $logRoot"

$existingEndpoint = Get-NetUDPEndpoint -LocalPort $Port -ErrorAction SilentlyContinue |
    Select-Object -First 1
if (-not $existingEndpoint) {
    $existingEndpoint = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue |
        Select-Object -First 1
}
if ($existingEndpoint) {
    throw "Port $Port is already used by PID $($existingEndpoint.OwningProcess). Stop it or specify another -Port."
}

if ($ValidateOnly) {
    Write-Host 'Validation passed. UE was not launched.' -ForegroundColor Green
    return
}

New-Item -ItemType Directory -Path $logRoot -Force | Out-Null

try {
    $serverArguments = @(
        "`"$projectFile`"",
        $mapUrl,
        '-server',
        "-port=$Port",
        '-unattended',
        '-NoSound',
        '-NoSplash',
        '-NullRHI',
        "-AbsLog=`"$serverLog`""
    )

    Write-Host 'Starting the hidden Dedicated Server...' -ForegroundColor Yellow
    $serverProcess = Start-Process `
        -FilePath $editorExe `
        -ArgumentList $serverArguments `
        -WindowStyle Hidden `
        -PassThru

    $serverReady = Wait-ForLogPattern `
        -LogFile $serverLog `
        -Pattern "IpNetDriver listening on port $Port" `
        -TimeoutSeconds $ReadyTimeoutSeconds `
        -Process $serverProcess

    if (-not $serverReady) {
        $tail = if (Test-Path -LiteralPath $serverLog) {
            (Get-Content -LiteralPath $serverLog -Tail 30) -join [Environment]::NewLine
        }
        else {
            'The Server has not created a log file.'
        }
        throw "The Server did not listen on port $Port within $ReadyTimeoutSeconds seconds.`n$tail"
    }

    Write-Host "The Server is listening on $Port. PID=$($serverProcess.Id)" -ForegroundColor Green

    $clientDefinitions = @(
        @{
            Number = 1
            Log = $client1Log
            WinX = 0
            ExtraArguments = @()
        },
        @{
            Number = 2
            Log = $client2Log
            WinX = $ClientResX + 20
            ExtraArguments = @('-NoSound')
        }
    )

    foreach ($clientDefinition in $clientDefinitions) {
        $clientArguments = @(
            "`"$projectFile`"",
            "127.0.0.1:$Port",
            '-game',
            '-log',
            '-windowed',
            "-ResX=$ClientResX",
            "-ResY=$ClientResY",
            "-WinX=$($clientDefinition.WinX)",
            '-WinY=0',
            '-NoSplash',
            "-AbsLog=`"$($clientDefinition.Log)`""
        ) + $clientDefinition.ExtraArguments

        if ($AutomatedSmokeSeconds -gt 0) {
            $clientArguments += @('-unattended', '-NullRHI', '-NoSound')
        }

        $clientProcess = Start-Process `
            -FilePath $editorExe `
            -ArgumentList $clientArguments `
            -WindowStyle $(if ($AutomatedSmokeSeconds -gt 0) { 'Hidden' } else { 'Normal' }) `
            -PassThru
        $clientProcesses.Add($clientProcess)
        Write-Host "Client $($clientDefinition.Number) started. PID=$($clientProcess.Id)"
    }

    Write-Host 'Waiting for both clients to join...' -ForegroundColor Yellow
    $joinDeadline = (Get-Date).AddSeconds($ReadyTimeoutSeconds)
    $joinCount = 0
    while ((Get-Date) -lt $joinDeadline) {
        if ($serverProcess.HasExited) {
            break
        }

        if (Test-Path -LiteralPath $serverLog) {
            $joinCount = @(Select-String -LiteralPath $serverLog -Pattern 'LogNet: Join succeeded:').Count
            if ($joinCount -ge 2) {
                break
            }
        }
        Start-Sleep -Seconds 1
    }

    if ($joinCount -ge 2) {
        Write-Host 'Both clients completed Login/Join.' -ForegroundColor Green
    }
    else {
        if ($AutomatedSmokeSeconds -gt 0) {
            throw "Only $joinCount joins were detected within $ReadyTimeoutSeconds seconds."
        }
        Write-Warning "Only $joinCount joins were detected within $ReadyTimeoutSeconds seconds. Check the client windows and logs."
    }

    if ($AutomatedSmokeSeconds -gt 0) {
        if ($Experience -and -not (Wait-ForLogPattern `
                -LogFile $serverLog `
                -Pattern ([regex]::Escape($Experience)) `
                -TimeoutSeconds $ReadyTimeoutSeconds `
                -Process $serverProcess)) {
            throw "The Server log did not confirm Extraction Experience '$Experience'."
        }

        if ($Experience -and -not (Wait-ForLogPattern `
                -LogFile $serverLog `
                -Pattern 'Granted Extraction rifle and shotgun' `
                -TimeoutSeconds $ReadyTimeoutSeconds `
                -Process $serverProcess)) {
            throw 'The Server did not grant the Extraction default loadout.'
        }

        Write-Host "Holding the two-client session for $AutomatedSmokeSeconds seconds..." -ForegroundColor Yellow
        $smokeDeadline = (Get-Date).AddSeconds($AutomatedSmokeSeconds)
        while ((Get-Date) -lt $smokeDeadline) {
            if ($serverProcess.HasExited -or @($clientProcesses | Where-Object HasExited).Count -gt 0) {
                throw 'An owned UE process exited during the automated smoke interval.'
            }
            Start-Sleep -Seconds 1
        }

        Stop-OwnedProcess -Process $clientProcesses[0]
        Start-Sleep -Seconds 3
        if ($serverProcess.HasExited -or $clientProcesses[1].HasExited) {
            throw 'Server or remaining client exited after Client 1 disconnected.'
        }
        Write-Host 'Client 1 disconnected; Server and Client 2 remained alive.' -ForegroundColor Green

        Stop-OwnedProcess -Process $serverProcess
        if (-not (Wait-ForLogPattern `
                -LogFile $client2Log `
                -Pattern 'Network Failure|Connection.*lost|Host closed the connection' `
                -TimeoutSeconds 90 `
                -Process $clientProcesses[1])) {
            throw 'Client 2 did not log a clear disconnect after the Server stopped.'
        }
        Write-Host 'Client 2 recorded the Server disconnect.' -ForegroundColor Green
        return
    }

    Write-Host ''
    Write-Host 'Manual acceptance checklist:' -ForegroundColor Cyan
    Write-Host '  1. Both windows enter the Convolution map.'
    Write-Host '  2. Move Client 1 and confirm Client 2 sees that movement.'
    Write-Host '  3. Verify firing, damage, HUD, death, and respawn.'
    Write-Host '  4. Close one client and confirm the Server and other client continue.'
    Write-Host ''
    Write-Host "Server log : $serverLog" -ForegroundColor DarkGray
    Write-Host "Client 1 log: $client1Log" -ForegroundColor DarkGray
    Write-Host "Client 2 log: $client2Log" -ForegroundColor DarkGray
    Read-Host 'Press Enter when finished; only processes started by this script will be closed' | Out-Null
}
finally {
    foreach ($clientProcess in $clientProcesses) {
        Stop-OwnedProcess -Process $clientProcess
    }
    Stop-OwnedProcess -Process $serverProcess
    Write-Host 'Week 01 multiplayer processes have been cleaned up.' -ForegroundColor Green
}
