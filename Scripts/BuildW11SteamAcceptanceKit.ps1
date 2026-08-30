[CmdletBinding()]
param(
    [string]$PackageManifest = "",
    [string]$OutputDirectory = "",
    [switch]$PlanOnly,
    [string]$PythonCommand = "python"
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($PackageManifest)) {
    $PackageManifest = Join-Path $ProjectRoot (
        "Saved\SteamAcceptance\Packages\W11Steam_Final43_Development\W11SteamPackageManifest.json")
}
else {
    $PackageManifest = [System.IO.Path]::GetFullPath($PackageManifest)
}
$PackageVerifier = Join-Path $PSScriptRoot "verify_w11_steam_package_manifest.py"
$KitVerifier = Join-Path $PSScriptRoot "verify_w11_steam_acceptance_kit.py"
$ReadmeTemplate = Join-Path $PSScriptRoot "README_W11_STEAM_ACCEPTANCE.md"
$PortableTools = @(
    (Join-Path $PSScriptRoot "RunW11SteamAcceptance.ps1"),
    (Join-Path $PSScriptRoot "TestW11SteamAcceptanceEnvironment.ps1"),
    (Join-Path $PSScriptRoot "verify_w11_steam_acceptance_logs.py"),
    $PackageVerifier,
    $KitVerifier
)
foreach ($RequiredPath in @($PackageManifest, $PackageVerifier, $KitVerifier, $ReadmeTemplate) + $PortableTools) {
    if (-not (Test-Path -LiteralPath $RequiredPath -PathType Leaf)) {
        throw "Required W11 Steam kit file not found: $RequiredPath"
    }
}

& $PythonCommand -B $PackageVerifier $PackageManifest --require-full-inventory --require-process-target
if ($LASTEXITCODE -ne 0) {
    throw "Source W11 package manifest failed with exit code $LASTEXITCODE."
}
$PackageHash = (Get-FileHash -LiteralPath $PackageManifest -Algorithm SHA256).Hash.ToLowerInvariant()
$PackageRoot = Split-Path -Parent $PackageManifest

if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $OutputDirectory = Join-Path $ProjectRoot "Saved\SteamAcceptance\Kits\W11SteamKit_$Timestamp"
}
else {
    $OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
}

Write-Host "[W11 Steam Kit] Package: $PackageRoot" -ForegroundColor Cyan
Write-Host "[W11 Steam Kit] Package hash: $PackageHash" -ForegroundColor Cyan
Write-Host "[W11 Steam Kit] Output: $OutputDirectory" -ForegroundColor Cyan
if ($PlanOnly) {
    Write-Output (
        "W11_STEAM_ACCEPTANCE_KIT_PLAN Runtime=CookedGame Platform=Win64 " +
        "PackageHash=$PackageHash PortableRelease=1 SourceProjectRequired=0 " +
        "UnrealEngineRequired=0 ProcessTarget=RuntimeExecutable Integrity=FullInventory " +
        "LaunchContract=RunnerOnlyUserDirOutsidePackage " +
        "VerificationContract=PreLaunchAndPostExitFullInventory Overwrite=0 Output=$OutputDirectory"
    )
    return
}

if (Test-Path -LiteralPath $OutputDirectory) {
    throw "W11 Steam acceptance kit output already exists and will not be overwritten: $OutputDirectory"
}
$OutputParent = Split-Path -Parent $OutputDirectory
New-Item -ItemType Directory -Path $OutputParent -Force | Out-Null
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null

$KitPackageRoot = Join-Path $OutputDirectory "Package"
$KitToolsRoot = Join-Path $OutputDirectory "Tools"
Copy-Item -LiteralPath $PackageRoot -Destination $KitPackageRoot -Recurse
New-Item -ItemType Directory -Path $KitToolsRoot | Out-Null
foreach ($Tool in $PortableTools) {
    Copy-Item -LiteralPath $Tool -Destination (Join-Path $KitToolsRoot (Split-Path -Leaf $Tool))
}

$ReadmePath = Join-Path $OutputDirectory "README_W11_STEAM_ACCEPTANCE.md"
Copy-Item -LiteralPath $ReadmeTemplate -Destination $ReadmePath

$ToolArtifacts = @()
foreach ($File in @(
    Get-ChildItem -LiteralPath $KitToolsRoot -File
) + @(Get-Item -LiteralPath $ReadmePath)) {
    $ToolArtifacts += [ordered]@{
        path = $File.FullName.Substring($OutputDirectory.Length).TrimStart("\", "/").Replace("\", "/")
        bytes = $File.Length
        sha256 = (Get-FileHash -LiteralPath $File.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}
$KitManifest = [ordered]@{
    schema = 3
    kind = "W11SteamAcceptanceKit"
    createdAt = (Get-Date).ToString("o")
    runtime = "CookedGame"
    platform = "Win64"
    packageManifest = "Package/W11SteamPackageManifest.json"
    packageManifestSha256 = $PackageHash
    sourceProjectRequired = $false
    unrealEngineRequired = $false
    fullInventorySealed = $true
    launchContract = "RunnerOnlyUserDirOutsidePackage"
    verificationContract = "PreLaunchAndPostExitFullInventory"
    artifactCount = $ToolArtifacts.Count
    mutablePaths = @("Saved/")
    prerequisites = @("Windows", "SteamClient", "Python3", "ProjectSteamAccess", "FirewallPolicyReview")
    supportedScenarios = @("BasicJoin", "FriendInvite", "HostExit", "ClientDisconnect", "PlayerSync")
    artifacts = $ToolArtifacts
    note = "This kit proves file identity and runner readiness only; real Steam behavior requires two authorized machines."
}
$KitManifestPath = Join-Path $OutputDirectory "W11SteamAcceptanceKitManifest.json"
$KitManifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $KitManifestPath -Encoding UTF8

& $PythonCommand -B (Join-Path $KitToolsRoot "verify_w11_steam_acceptance_kit.py") $KitManifestPath
if ($LASTEXITCODE -ne 0) {
    throw "Generated W11 Steam acceptance kit failed validation with exit code $LASTEXITCODE."
}

Write-Output (
    "W11_STEAM_ACCEPTANCE_KIT_READY Runtime=CookedGame Platform=Win64 " +
    "PackageHash=$PackageHash Root=$OutputDirectory Manifest=$KitManifestPath " +
    "SourceProjectRequired=0 UnrealEngineRequired=0 ProcessTarget=RuntimeExecutable " +
    "Integrity=FullInventory LaunchContract=RunnerOnlyUserDirOutsidePackage " +
    "VerificationContract=PreLaunchAndPostExitFullInventory SteamGate=PENDING"
)
