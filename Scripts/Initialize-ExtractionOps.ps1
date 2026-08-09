[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string]$LyraProject = '',

    [string]$EngineRoot = '',

    [switch]$BuildEditor,

    [switch]$LaunchEditor
)

$ErrorActionPreference = 'Stop'
$script:InteractiveMode = [string]::IsNullOrWhiteSpace($LyraProject) -or [string]::IsNullOrWhiteSpace($EngineRoot)

trap {
    Write-Host ''
    Write-Host "[Extraction Ops] 初始化失败：$($_.Exception.Message)" -ForegroundColor Red
    if ($script:InteractiveMode) {
        Read-Host '按 Enter 键退出' | Out-Null
        exit 1
    }
    throw
}

function Resolve-FullPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    $Path = $Path.Trim()
    if ($Path.Length -ge 2) {
        $first = $Path[0]
        $last = $Path[$Path.Length - 1]
        if (($first -eq "'" -and $last -eq "'") -or ($first -eq '"' -and $last -eq '"')) {
            $Path = $Path.Substring(1, $Path.Length - 2)
        }
    }

    if (-not [System.IO.Path]::IsPathRooted($Path)) {
        $Path = Join-Path (Get-Location) $Path
    }

    return [System.IO.Path]::GetFullPath($Path)
}

function Get-NormalizedPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (Resolve-FullPath $Path).TrimEnd('\')
}

function Fail {
    param([Parameter(Mandatory = $true)][string]$Message)

    throw "[Extraction Ops] $Message"
}

function Read-ExistingPath {
    param(
        [Parameter(Mandatory = $true)][string]$Prompt,
        [string]$DefaultValue = ''
    )

    while ($true) {
        $promptText = $Prompt
        if ($DefaultValue) {
            $promptText = "$Prompt [$DefaultValue]"
        }

        $value = Read-Host $promptText
        if ([string]::IsNullOrWhiteSpace($value) -and $DefaultValue) {
            $value = $DefaultValue
        }

        if ($value) {
            $resolvedValue = Resolve-FullPath $value
            if ((Test-Path -LiteralPath $resolvedValue -PathType Container) -or
                ((Test-Path -LiteralPath $resolvedValue -PathType Leaf) -and
                    [System.IO.Path]::GetExtension($resolvedValue) -ieq '.uproject')) {
                return $value
            }
        }

        Write-Host '路径不存在，请重新输入。' -ForegroundColor Yellow
    }
}

function Read-YesNo {
    param(
        [Parameter(Mandatory = $true)][string]$Prompt,
        [bool]$Default = $false
    )

    $defaultHint = if ($Default) { 'Y/n' } else { 'y/N' }
    while ($true) {
        $answer = (Read-Host "$Prompt [$defaultHint]").Trim().ToLowerInvariant()
        if (-not $answer) {
            return $Default
        }
        if ($answer -in @('y', 'yes', '是')) {
            return $true
        }
        if ($answer -in @('n', 'no', '否')) {
            return $false
        }
        Write-Host '请输入 y 或 n。' -ForegroundColor Yellow
    }
}

function Get-ValidEngineRoot {
    param([Parameter(Mandatory = $true)][string]$Candidate)

    $candidatePath = Resolve-FullPath $Candidate
    if (-not (Test-Path -LiteralPath $candidatePath -PathType Container)) {
        return $null
    }

    $installedEditor = Join-Path $candidatePath 'Engine\Binaries\Win64\UnrealEditor.exe'
    if (Test-Path -LiteralPath $installedEditor -PathType Leaf) {
        return $candidatePath
    }

    if ((Split-Path -Leaf $candidatePath) -ieq 'Engine') {
        $engineEditor = Join-Path $candidatePath 'Binaries\Win64\UnrealEditor.exe'
        if (Test-Path -LiteralPath $engineEditor -PathType Leaf) {
            return (Split-Path -Parent $candidatePath)
        }
    }

    return $null
}

function Mount-AssetDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    $sourcePath = Resolve-FullPath $Source
    $destinationPath = Resolve-FullPath $Destination

    if (-not (Test-Path -LiteralPath $sourcePath -PathType Container)) {
        Fail "资源目录不存在：$sourcePath"
    }

    if (Test-Path -LiteralPath $destinationPath) {
        $destinationItem = Get-Item -LiteralPath $destinationPath -Force
        $isReparsePoint = (($destinationItem.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0)

        if ($isReparsePoint) {
            if ($destinationItem.LinkType -eq 'Junction') {
                $target = [string]($destinationItem.Target | Select-Object -First 1)
                if ($target -and (Get-NormalizedPath $target) -ieq (Get-NormalizedPath $sourcePath)) {
                    [System.IO.Directory]::Delete($destinationPath, $false)
                    if (Test-Path -LiteralPath $destinationPath) {
                        Fail "无法移除旧资源 Junction：$destinationPath"
                    }
                    Write-Host "已移除旧资源联接，将改为复制：$destinationPath" -ForegroundColor Yellow
                }
                else {
                    Fail "目标资源目录是指向其他位置的 Junction：$destinationPath。为避免覆盖本地文件，请先检查并移除它。"
                }
            }
            else {
                Fail "目标资源目录是非 Junction 的重解析点：$destinationPath。为避免覆盖本地文件，请先检查并移除它。"
            }
        }
        elseif (-not $destinationItem.PSIsContainer) {
            Fail "目标资源路径已存在但不是目录：$destinationPath。为避免覆盖本地文件，请先检查并移除它。"
        }
    }

    $destinationParent = Split-Path -Parent $destinationPath
    New-Item -ItemType Directory -Path $destinationParent -Force | Out-Null

    New-Item -ItemType Directory -Path $destinationPath -Force | Out-Null
    Copy-Item -Path (Join-Path $sourcePath '*') -Destination $destinationPath -Recurse -Force
    Write-Host "已复制资源：$destinationPath" -ForegroundColor Green
}

$repoRoot = Resolve-FullPath (Join-Path $PSScriptRoot '..')
$projectFile = Join-Path $repoRoot 'LyraStarterGame.uproject'

if ($script:InteractiveMode) {
    Write-Host 'Extraction Ops 交互式初始化' -ForegroundColor Cyan
    Write-Host '直接双击此脚本即可；带齐参数运行时仍会保持自动化模式。' -ForegroundColor DarkGray
    Write-Host ''

    if ([string]::IsNullOrWhiteSpace($LyraProject)) {
        $siblingLyraProject = Join-Path (Split-Path -Parent $repoRoot) 'LyraStarterGame'
        $defaultLyraProject = if (Test-Path -LiteralPath $siblingLyraProject -PathType Container) {
            $siblingLyraProject
        }
        else {
            ''
        }
        $LyraProject = Read-ExistingPath 'Lyra 项目目录或 .uproject 文件' $defaultLyraProject
    }
}

if (-not (Test-Path -LiteralPath $projectFile -PathType Leaf)) {
    Fail "脚本必须从 Extraction Ops 仓库的 Scripts 目录运行，找不到 $projectFile。"
}

$sourceRoot = Resolve-FullPath $LyraProject
if (Test-Path -LiteralPath $sourceRoot -PathType Leaf) {
    if ([System.IO.Path]::GetExtension($sourceRoot) -ine '.uproject') {
        Fail "LyraProject 必须是 Lyra 项目目录或 .uproject 文件：$sourceRoot"
    }

    $sourceRoot = Split-Path -Parent $sourceRoot
}

if (-not (Test-Path -LiteralPath $sourceRoot -PathType Container)) {
    Fail "Lyra 项目目录不存在：$sourceRoot"
}

if ((Get-NormalizedPath $sourceRoot) -ieq (Get-NormalizedPath $repoRoot)) {
    Fail 'LyraProject 不能指向当前 Extraction Ops 仓库本身。'
}

$sourceContent = Join-Path $sourceRoot 'Content'
$sourcePlugins = Join-Path $sourceRoot 'Plugins'

$sourceProjectFile = Get-ChildItem -LiteralPath $sourceRoot -Filter '*.uproject' -File -Force |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $sourceProjectFile) {
    Fail "LyraProject 目录中找不到 .uproject 文件：$sourceRoot"
}

if (-not (Test-Path -LiteralPath $sourceContent -PathType Container)) {
    Fail "Lyra 项目 Content 目录不存在：$sourceContent。请先通过 Epic Games Launcher/Fab 创建 Lyra 项目。"
}

$projectMetadata = Get-Content -LiteralPath $projectFile -Raw -Encoding UTF8 | ConvertFrom-Json
$engineAssociation = [string]$projectMetadata.EngineAssociation
$sourceMetadata = Get-Content -LiteralPath $sourceProjectFile -Raw -Encoding UTF8 | ConvertFrom-Json
$sourceEngineAssociation = [string]$sourceMetadata.EngineAssociation
if ($sourceEngineAssociation -and $engineAssociation -and $sourceEngineAssociation -ne $engineAssociation) {
    Fail "当前工程需要 Unreal Engine $engineAssociation，但资源来源工程使用 $sourceEngineAssociation。请使用匹配版本的 Lyra 项目。"
}

$engineCandidates = @()
if ($EngineRoot) {
    $engineCandidates += $EngineRoot
}
if ($engineAssociation) {
    $engineCandidates += (Join-Path (Join-Path $env:ProgramFiles 'Epic Games') "UE_$engineAssociation")
    if (${env:ProgramFiles(x86)}) {
        $engineCandidates += (Join-Path (Join-Path ${env:ProgramFiles(x86)} 'Epic Games') "UE_$engineAssociation")
    }
}

$engineRootPath = $null
foreach ($engineCandidate in $engineCandidates) {
    $validEngineRoot = Get-ValidEngineRoot $engineCandidate
    if ($validEngineRoot) {
        $engineRootPath = $validEngineRoot
        break
    }
}
if (-not $engineRootPath) {
    if ($script:InteractiveMode -and [string]::IsNullOrWhiteSpace($EngineRoot)) {
        Write-Host "未找到默认 Unreal Engine $engineAssociation，正在等待你输入引擎目录。" -ForegroundColor Yellow
        while (-not $engineRootPath) {
            $EngineRoot = Read-Host "未自动找到 Unreal Engine $engineAssociation，请输入引擎目录"
            if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
                Write-Host '引擎目录不能为空。' -ForegroundColor Yellow
                continue
            }

            $engineRootPath = Get-ValidEngineRoot $EngineRoot
            if (-not $engineRootPath) {
                Write-Host '该目录不是有效的 Unreal Engine 根目录，请确认其中包含 Engine\Binaries\Win64\UnrealEditor.exe。' -ForegroundColor Yellow
            }
        }
    }
    else {
        Fail "未找到 Unreal Engine $engineAssociation。请通过 -EngineRoot 指定引擎目录。"
    }
}

if ($script:InteractiveMode) {
    Write-Host '资源将复制到 Extraction Ops 工作区，不使用 Junction。' -ForegroundColor DarkGray
    if (-not $PSBoundParameters.ContainsKey('BuildEditor')) {
        $BuildEditor = Read-YesNo '是否现在编译 LyraEditor？' $false
    }
    if (-not $PSBoundParameters.ContainsKey('LaunchEditor')) {
        $LaunchEditor = Read-YesNo '初始化完成后是否启动 Unreal Editor？' $false
    }
}

Write-Host "Extraction Ops: $repoRoot" -ForegroundColor Cyan
Write-Host "Lyra 资源来源：$sourceRoot" -ForegroundColor Cyan
Write-Host "Unreal Engine：$engineRootPath" -ForegroundColor Cyan
Write-Host '资源模式：复制到本地工作区' -ForegroundColor Cyan

Mount-AssetDirectory -Source $sourceContent -Destination (Join-Path $repoRoot 'Content')

if (Test-Path -LiteralPath $sourcePlugins -PathType Container) {
    $pluginContentDirectories = Get-ChildItem -LiteralPath $sourcePlugins -Directory -Recurse -Force |
        Where-Object { $_.Name -eq 'Content' }

    foreach ($sourcePluginContent in $pluginContentDirectories) {
        $relativePluginContent = $sourcePluginContent.FullName.Substring($sourcePlugins.Length).TrimStart('\')
        $destinationPluginContent = Join-Path (Join-Path $repoRoot 'Plugins') $relativePluginContent
        $destinationPluginRoot = Split-Path -Parent $destinationPluginContent

        if (Test-Path -LiteralPath $destinationPluginRoot -PathType Container) {
            Mount-AssetDirectory -Source $sourcePluginContent.FullName -Destination $destinationPluginContent
        }
        else {
            Write-Warning "跳过未出现在当前仓库的插件资源：$relativePluginContent"
        }
    }
}

$selectorCandidates = @(
    (Join-Path $engineRootPath 'Engine\Binaries\Win64\UnrealVersionSelector-Win64-Shipping.exe'),
    (Join-Path $engineRootPath 'Engine\Binaries\Win64\UnrealVersionSelector.exe')
)
$selector = $selectorCandidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1

if ($selector) {
    Write-Host '正在生成项目文件...' -ForegroundColor Cyan
    & $selector '/projectfiles' $projectFile
    if ($LASTEXITCODE -ne 0) {
        Fail "UnrealVersionSelector 生成项目文件失败，退出码：$LASTEXITCODE"
    }
}
elseif (Test-Path -LiteralPath (Join-Path $engineRootPath 'Engine\Build\BatchFiles\GenerateProjectFiles.bat') -PathType Leaf) {
    $generateProjectFiles = Join-Path $engineRootPath 'Engine\Build\BatchFiles\GenerateProjectFiles.bat'
    Write-Host '正在使用 GenerateProjectFiles.bat 生成项目文件...' -ForegroundColor Cyan
    & $generateProjectFiles $projectFile
    if ($LASTEXITCODE -ne 0) {
        Fail "GenerateProjectFiles.bat 失败，退出码：$LASTEXITCODE"
    }
}
else {
    $ubtDirectory = Join-Path $engineRootPath 'Engine\Binaries\DotNET\UnrealBuildTool'
    $ubtDll = Join-Path $ubtDirectory 'UnrealBuildTool.dll'
    $ubt = Join-Path $ubtDirectory 'UnrealBuildTool.exe'
    $bundledDotnet = Join-Path $engineRootPath 'Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe'

    if (-not (Test-Path -LiteralPath $ubtDll -PathType Leaf) -and -not (Test-Path -LiteralPath $ubt -PathType Leaf)) {
        Fail '找不到 UnrealVersionSelector、GenerateProjectFiles.bat 或 UnrealBuildTool.exe。'
    }

    Write-Host '正在使用 UnrealBuildTool 生成 Rider 项目文件...' -ForegroundColor Cyan
    $ubtArguments = @('-ProjectFiles', '-Rider', "-Project=$projectFile", '-Game', '-Engine')

    if (Test-Path -LiteralPath $bundledDotnet -PathType Leaf) {
        $previousDotnetRoot = $env:DOTNET_ROOT
        $previousDotnetRootX64 = $env:DOTNET_ROOT_X64
        $env:DOTNET_ROOT = Split-Path -Parent $bundledDotnet
        $env:DOTNET_ROOT_X64 = Split-Path -Parent $bundledDotnet
        try {
            if (Test-Path -LiteralPath $ubtDll -PathType Leaf) {
                Push-Location $ubtDirectory
                try {
                    & $bundledDotnet $ubtDll @ubtArguments
                    $ubtExitCode = $LASTEXITCODE
                }
                finally {
                    Pop-Location
                }
            }
            else {
                & $ubt @ubtArguments
                $ubtExitCode = $LASTEXITCODE
            }
        }
        finally {
            $env:DOTNET_ROOT = $previousDotnetRoot
            $env:DOTNET_ROOT_X64 = $previousDotnetRootX64
        }
    }
    else {
        if (Test-Path -LiteralPath $ubtDll -PathType Leaf) {
            & dotnet $ubtDll @ubtArguments
        }
        else {
            & $ubt @ubtArguments
        }
        $ubtExitCode = $LASTEXITCODE
    }

    if ($ubtExitCode -ne 0) {
        Fail "UnrealBuildTool 生成项目文件失败，退出码：$ubtExitCode"
    }
}

if ($BuildEditor) {
    $buildBatch = Join-Path $engineRootPath 'Engine\Build\BatchFiles\Build.bat'
    if (-not (Test-Path -LiteralPath $buildBatch -PathType Leaf)) {
        Fail "找不到 Build.bat：$buildBatch"
    }

    Write-Host '正在编译 LyraEditor...' -ForegroundColor Cyan
    & $buildBatch 'LyraEditor' 'Win64' 'Development' "-Project=$projectFile" '-WaitMutex'
    if ($LASTEXITCODE -ne 0) {
        Fail "LyraEditor 编译失败，退出码：$LASTEXITCODE"
    }
}

if ($LaunchEditor) {
    $editor = Join-Path $engineRootPath 'Engine\Binaries\Win64\UnrealEditor.exe'
    if (-not (Test-Path -LiteralPath $editor -PathType Leaf)) {
        Fail "找不到 UnrealEditor.exe：$editor"
    }

    Start-Process -FilePath $editor -ArgumentList "`"$projectFile`"" -WorkingDirectory $repoRoot
}

Write-Host ''
Write-Host '初始化完成。' -ForegroundColor Green
Write-Host "项目文件：$projectFile"
Write-Host '如需编译：重新运行时添加 -BuildEditor；如需启动编辑器：添加 -LaunchEditor。'

if ($script:InteractiveMode) {
    Read-Host '初始化已完成，按 Enter 键退出' | Out-Null
}
