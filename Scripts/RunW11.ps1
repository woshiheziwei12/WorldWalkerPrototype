[CmdletBinding()]
param(
    [string]$EngineRoot = "",
    [ValidateSet("Development", "DebugGame")]
    [string]$Configuration = "Development",
    [ValidateSet("Editor", "Cooked")]
    [string]$RuntimeMode = "Editor",
    [string]$PackageManifest = "",
    [string]$PythonCommand = "python",
    [switch]$SkipBuild,
    [switch]$BuildOnly,
    [int]$ResX = 0,
    [int]$ResY = 0,
    [switch]$Fullscreen,
    [switch]$Windowed,
    [ValidateSet("Observe", "Evasion", "Dodge", "Recovery", "Feedback")]
    [string]$ManualEvidenceRole,
    [int]$RunSeed = 424242,
    [string]$ManualEvidenceLog = "",
    [switch]$WaitForExit,
    [switch]$PlanOnly
)

$ErrorActionPreference = "Stop"

if ($Fullscreen -and $Windowed) {
    throw "-Fullscreen and -Windowed cannot be used together."
}
if (($ResX -le 0) -xor ($ResY -le 0)) {
    throw "Specify both -ResX and -ResY, or leave both at 0 for native resolution."
}
$UseNativeResolution = $ResX -le 0 -and $ResY -le 0
if ($UseNativeResolution) {
    try {
        Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class W11DpiAwareness {
    [DllImport("user32.dll")]
    public static extern bool SetProcessDpiAwarenessContext(IntPtr value);
}
"@
        [void][W11DpiAwareness]::SetProcessDpiAwarenessContext([IntPtr](-4))
        Add-Type -AssemblyName System.Windows.Forms
        $NativeBounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
        $ResX = [int]$NativeBounds.Width
        $ResY = [int]$NativeBounds.Height
    }
    catch {
        # The project-wide production baseline is 4K. Headless or restricted
        # shells that cannot query a monitor use that deterministic fallback.
        $ResX = 3840
        $ResY = 2160
        Write-Warning "Unable to query the primary display; using the 3840x2160 production baseline."
    }
}
if ($ResX -lt 640 -or $ResY -lt 360) {
    throw "W11 resolution must be at least 640x360."
}

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot "WorldWalkerPrototype.uproject"
$W11Map = "/Game/WorldWalker/Worlds/W11_RogueSurvival/Maps/L_W11_RogueSurvival"
$PackageVerifier = Join-Path $PSScriptRoot "verify_w11_steam_package_manifest.py"

if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
    if (-not [string]::IsNullOrWhiteSpace($env:UE_ROOT)) {
        $EngineRoot = $env:UE_ROOT
    }
    else {
        $EngineRoot = "E:\app\ue\UE_5.8"
    }
}

$LaunchArguments = @()
$UserDataDirectory = ""
if ($RuntimeMode -eq "Cooked") {
    if ($BuildOnly) {
        throw "-BuildOnly is valid only with -RuntimeMode Editor."
    }
    if ([string]::IsNullOrWhiteSpace($PackageManifest)) {
        throw "-RuntimeMode Cooked requires -PackageManifest."
    }
    $PackageManifest = [System.IO.Path]::GetFullPath($PackageManifest)
    foreach ($RequiredPath in @($PackageManifest, $PackageVerifier)) {
        if (-not (Test-Path -LiteralPath $RequiredPath -PathType Leaf)) {
            throw "Required cooked W11 file not found: $RequiredPath"
        }
    }
    & $PythonCommand $PackageVerifier $PackageManifest --require-full-inventory --require-process-target
    if ($LASTEXITCODE -ne 0) {
        throw "Cooked W11 package manifest failed with exit code $LASTEXITCODE."
    }
    $PackageHash = (Get-FileHash -LiteralPath $PackageManifest -Algorithm SHA256).Hash.ToLowerInvariant()
    $PackageRoot = Split-Path -Parent $PackageManifest
    $PackageData = Get-Content -LiteralPath $PackageManifest -Raw | ConvertFrom-Json
    $RuntimeExecutableRelative = [string]$PackageData.runtimeExecutable
    $Executable = Join-Path $PackageRoot $RuntimeExecutableRelative.Replace("/", "\")
    if (-not (Test-Path -LiteralPath $Executable -PathType Leaf)) {
        throw "Cooked W11 executable not found: $Executable"
    }
    $WorkingDirectory = Split-Path -Parent $Executable
    $UserDataIdentity = if ([string]::IsNullOrWhiteSpace($ManualEvidenceRole)) {
        $PackageHash
    }
    else {
        "${PackageHash}_$ManualEvidenceRole"
    }
    $UserDataDirectory = [System.IO.Path]::GetFullPath(
        (Join-Path $ProjectRoot "Saved\CookedUserData\$UserDataIdentity"))
    $PackageRootPrefix = $PackageRoot.TrimEnd("\", "/") + [System.IO.Path]::DirectorySeparatorChar
    if ($UserDataDirectory.Equals($PackageRoot, [System.StringComparison]::OrdinalIgnoreCase) `
        -or $UserDataDirectory.StartsWith(
            $PackageRootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Cooked W11 user data must stay outside the sealed package root."
    }
    $LaunchArguments += $W11Map
    $LaunchArguments += "-UserDir=`"$UserDataDirectory`""
    Write-Host "[W11] Cooked package verified: $PackageHash" -ForegroundColor Green
}
else {
    if (-not [string]::IsNullOrWhiteSpace($PackageManifest)) {
        throw "-PackageManifest is valid only with -RuntimeMode Cooked."
    }
    $BuildScript = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"
    $EditorExecutable = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
    foreach ($RequiredPath in @($ProjectFile, $BuildScript, $EditorExecutable)) {
        if (-not (Test-Path -LiteralPath $RequiredPath)) {
            throw "Required file not found: $RequiredPath"
        }
    }
    if (-not $SkipBuild) {
        Write-Host "[W11] Building WorldWalkerPrototypeEditor Win64 $Configuration..." -ForegroundColor Cyan
        & $BuildScript `
            WorldWalkerPrototypeEditor `
            Win64 `
            $Configuration `
            $ProjectFile `
            -WaitMutex `
            -NoHotReloadFromIDE

        if ($LASTEXITCODE -ne 0) {
            throw "W11 build failed with exit code $LASTEXITCODE."
        }

        Write-Host "[W11] Build succeeded." -ForegroundColor Green
    }
    else {
        Write-Host "[W11] Build skipped." -ForegroundColor Yellow
    }
    if ($BuildOnly) {
        Write-Host "[W11] Build-only mode complete." -ForegroundColor Green
        return
    }
    $Executable = $EditorExecutable
    $WorkingDirectory = $ProjectRoot
    $LaunchArguments += @("`"$ProjectFile`"", $W11Map, "-game")
}

$WindowMode = if ($Windowed) {
    "-Windowed"
}
elseif ($Fullscreen -or $UseNativeResolution) {
    "-Fullscreen"
}
else {
    "-Windowed"
}
$LaunchArguments += @(
    $WindowMode,
    "-ResX=$ResX",
    "-ResY=$ResY",
    "-ExecCmds=`"r.ScreenPercentage 100`"",
    "-log",
    "-NoSplash"
)

if (-not [string]::IsNullOrWhiteSpace($ManualEvidenceRole)) {
    if ([string]::IsNullOrWhiteSpace($ManualEvidenceLog)) {
        $LogDirectory = Join-Path $ProjectRoot "Saved\Logs"
        New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null
        $Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
        $ManualEvidenceLog = Join-Path $LogDirectory "W11Manual${ManualEvidenceRole}_${Timestamp}.log"
    }
    else {
        $ManualEvidenceLog = [System.IO.Path]::GetFullPath($ManualEvidenceLog)
    }
    $LaunchArguments += "-W11ManualEvidenceRole=$ManualEvidenceRole"
    $LaunchArguments += "-W11RunSeed=$RunSeed"
    $LaunchArguments += "-AbsLog=`"$ManualEvidenceLog`""
    Write-Host "[W11] Manual evidence role: $ManualEvidenceRole" -ForegroundColor Cyan
    Write-Host "[W11] Evidence log: $ManualEvidenceLog" -ForegroundColor Cyan
}

Write-Host (
    "[W11] Launching $W11Map at ${ResX}x${ResY}, " +
    "mode=$($WindowMode.TrimStart('-')), screen percentage=100..."
) -ForegroundColor Cyan
if ($PlanOnly) {
    $PlanPackageHash = if ($RuntimeMode -eq "Cooked") { $PackageHash } else { "EDITOR" }
    $UserDirOutsidePackage = if ($RuntimeMode -eq "Cooked") { "1" } else { "NA" }
    Write-Output (
        "W11_RUN_PLAN RuntimeMode=$RuntimeMode Runtime=" +
        "$(if ($RuntimeMode -eq 'Cooked') { 'CookedGame' } else { 'EditorStandalone' }) " +
        "PackageHash=$PlanPackageHash Interactive=1 Rendered=1 Audio=1 " +
        "InputScript=None AutoStart=0 UserDirOutsidePackage=$UserDirOutsidePackage " +
        "Resolution=${ResX}x${ResY} WindowMode=$($WindowMode.TrimStart('-')) ScreenPercentage=100 " +
        "ProcessTarget=$(if ($RuntimeMode -eq 'Cooked') { 'RuntimeExecutable' } else { 'Editor' }) " +
        "WaitsForGameExit=$(if ($WaitForExit) { '1' } else { '0' }) Executable=$Executable"
    )
    return
}
if ($RuntimeMode -eq "Cooked") {
    New-Item -ItemType Directory -Path $UserDataDirectory -Force | Out-Null
}
$W11Process = Start-Process `
    -FilePath $Executable `
    -ArgumentList $LaunchArguments `
    -WorkingDirectory $WorkingDirectory `
    -PassThru

Write-Host "[W11] Launch request sent." -ForegroundColor Green
if ($WaitForExit) {
    Write-Host "[W11] Waiting for the interactive W11 process to exit..." -ForegroundColor Cyan
    $W11Process.WaitForExit()
    if ($W11Process.ExitCode -ne 0) {
        throw "Interactive W11 process exited with code $($W11Process.ExitCode)."
    }
    Write-Host "[W11] Process exited with code 0." -ForegroundColor Green
    return
}
