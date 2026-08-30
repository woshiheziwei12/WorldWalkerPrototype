[CmdletBinding()]
param(
    [string]$EngineRoot = "",
    [string]$OutputDirectory = "",
    [ValidateSet("Development", "Shipping")]
    [string]$Configuration = "Development",
    [switch]$FinalizeExisting,
    [switch]$PlanOnly,
    [string]$PythonCommand = "python"
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot "WorldWalkerPrototype.uproject"
$DefaultEngineIni = Join-Path $ProjectRoot "Config\DefaultEngine.ini"
$W11Map = "/Game/WorldWalker/Worlds/W11_RogueSurvival/Maps/L_W11_RogueSurvival"
$ManifestVerifier = Join-Path $PSScriptRoot "verify_w11_steam_package_manifest.py"

if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
    if (-not [string]::IsNullOrWhiteSpace($env:UE_ROOT)) {
        $EngineRoot = $env:UE_ROOT
    }
    else {
        $EngineRoot = "E:\app\ue\UE_5.8"
    }
}
$RunUAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
foreach ($RequiredPath in @($ProjectFile, $DefaultEngineIni, $RunUAT, $ManifestVerifier)) {
    if (-not (Test-Path -LiteralPath $RequiredPath)) {
        throw "Required W11 packaging file not found: $RequiredPath"
    }
}

if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $OutputDirectory = Join-Path $ProjectRoot "Saved\SteamAcceptance\Packages\W11Steam_$Timestamp"
}
else {
    $OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
}

$UatArguments = @(
    "BuildCookRun",
    "-project=$ProjectFile",
    "-target=WorldWalkerPrototype",
    "-noP4",
    "-platform=Win64",
    "-clientconfig=$Configuration",
    "-build",
    "-cook",
    "-stage",
    "-pak",
    "-iostore",
    "-archive",
    "-compressed",
    "-unversionedcookedcontent",
    "-utf8output",
    "-NoCodeSign",
    "-map=$W11Map",
    "-archivedirectory=$OutputDirectory"
)

Write-Host "[W11 Package] Configuration: $Configuration" -ForegroundColor Cyan
Write-Host "[W11 Package] Map: $W11Map" -ForegroundColor Cyan
Write-Host "[W11 Package] Output: $OutputDirectory" -ForegroundColor Cyan
if ($PlanOnly) {
    Write-Output (
        "W11_STEAM_PACKAGE_PLAN Configuration=$Configuration Platform=Win64 " +
        "Build=1 Cook=1 Stage=1 Pak=1 IoStore=1 Archive=1 Map=$W11Map " +
        "Output=$OutputDirectory Runtime=CookedGame Integrity=FullInventory " +
        "ProcessTarget=RuntimeExecutable Overwrite=0"
    )
    return
}

$ManifestPath = Join-Path $OutputDirectory "W11SteamPackageManifest.json"
if ($FinalizeExisting) {
    if (-not (Test-Path -LiteralPath $OutputDirectory)) {
        throw "Cannot finalize a W11 Steam package that does not exist: $OutputDirectory"
    }
    if (Test-Path -LiteralPath $ManifestPath) {
        throw "W11 Steam package manifest already exists and will not be overwritten: $ManifestPath"
    }
    Write-Host "[W11 Package] Finalizing existing successful BuildCookRun output..." -ForegroundColor Cyan
}
else {
    if (Test-Path -LiteralPath $OutputDirectory) {
        throw "W11 Steam package output already exists and will not be overwritten: $OutputDirectory"
    }
    $OutputParent = Split-Path -Parent $OutputDirectory
    New-Item -ItemType Directory -Path $OutputParent -Force | Out-Null

    Write-Host "[W11 Package] Running BuildCookRun..." -ForegroundColor Cyan
    & $RunUAT @UatArguments
    if ($LASTEXITCODE -ne 0) {
        throw "W11 Steam BuildCookRun failed with exit code $LASTEXITCODE."
    }
}

$Executables = @(
    Get-ChildItem -LiteralPath $OutputDirectory -Recurse -File -Filter "WorldWalkerPrototype.exe"
)
$LauncherPath = Join-Path $OutputDirectory "Windows\WorldWalkerPrototype.exe"
$Launcher = @($Executables | Where-Object {
    $_.FullName -eq $LauncherPath
})
if ($Launcher.Count -ne 1) {
    throw "Expected the packaged bootstrap executable at $LauncherPath, found $($Launcher.Count)."
}
$RuntimeExecutables = @($Executables | Where-Object {
    $_.FullName -ne $LauncherPath
})
if ($RuntimeExecutables.Count -ne 1) {
    throw "Expected exactly one packaged runtime executable below the bootstrap, found $($RuntimeExecutables.Count)."
}
$Executable = $Launcher[0]
$RuntimeExecutable = $RuntimeExecutables[0]
$BootstrapExecutableRelative = "Windows/WorldWalkerPrototype.exe"
$RuntimeExecutableRelative = $RuntimeExecutable.FullName.Substring(
    $OutputDirectory.Length).TrimStart("\", "/").Replace("\", "/")
if ($RuntimeExecutableRelative -ne "Windows/WorldWalkerPrototype/Binaries/Win64/WorldWalkerPrototype.exe") {
    throw "Packaged runtime executable is not at the canonical wait target: $RuntimeExecutableRelative"
}
$Containers = @(
    Get-ChildItem -LiteralPath $OutputDirectory -Recurse -File |
        Where-Object { $_.Extension -in @(".pak", ".utoc", ".ucas") }
)
if ($Containers.Count -lt 3) {
    throw "Expected packaged .pak/.utoc/.ucas content containers, found $($Containers.Count)."
}

$EvidenceFiles = @(
    Get-ChildItem -LiteralPath $OutputDirectory -Recurse -File |
        Where-Object { $_.FullName -ne $ManifestPath } |
        Sort-Object FullName
)
$Artifacts = @(
    foreach ($File in $EvidenceFiles) {
        [ordered]@{
            path = $File.FullName.Substring($OutputDirectory.Length).TrimStart("\", "/").Replace("\", "/")
            bytes = $File.Length
            sha256 = (Get-FileHash -LiteralPath $File.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    }
)
$EngineConfigText = Get-Content -LiteralPath $DefaultEngineIni -Raw
$PlatformServiceMatch = [regex]::Match(
    $EngineConfigText, '(?m)^\s*DefaultPlatformService\s*=\s*([^\s;]+)')
$SteamDevAppIdMatch = [regex]::Match(
    $EngineConfigText, '(?m)^\s*SteamDevAppId\s*=\s*([^\s;]+)')
if (-not $PlatformServiceMatch.Success -or -not $SteamDevAppIdMatch.Success) {
    throw "DefaultEngine.ini must declare DefaultPlatformService and SteamDevAppId for package provenance."
}
$Manifest = [ordered]@{
    schema = 4
    createdAt = (Get-Date).ToString("o")
    configuration = $Configuration
    platform = "Win64"
    runtime = "CookedGame"
    map = $W11Map
    bootstrapExecutable = $BootstrapExecutableRelative
    runtimeExecutable = $RuntimeExecutableRelative
    contentContainerCount = $Containers.Count
    fullInventorySealed = $true
    processTargetSealed = $true
    artifactCount = $Artifacts.Count
    artifacts = $Artifacts
    acceptanceSteamAppIdEmbedded = $false
    packagedDefaultPlatformService = $PlatformServiceMatch.Groups[1].Value
    packagedDefaultSteamDevAppId = $SteamDevAppIdMatch.Groups[1].Value
    note = "The package retains project defaults for negative startup. A non-placeholder project App ID is supplied only by the acceptance runner; this package does not prove Steam connectivity."
}
$Manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $ManifestPath -Encoding UTF8

& $PythonCommand $ManifestVerifier $ManifestPath --require-full-inventory --require-process-target
if ($LASTEXITCODE -ne 0) {
    throw "Generated W11 package full-inventory manifest failed validation with exit code $LASTEXITCODE."
}

Write-Output (
    "W11_STEAM_PACKAGE_READY Runtime=CookedGame Configuration=$Configuration " +
    "Executable=$($Executable.FullName) Artifacts=$($Artifacts.Count) Containers=$($Containers.Count) " +
    "Integrity=FullInventory ProcessTarget=RuntimeExecutable Manifest=$ManifestPath SteamGate=PENDING"
)
