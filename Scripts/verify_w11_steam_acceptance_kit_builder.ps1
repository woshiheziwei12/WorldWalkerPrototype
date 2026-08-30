[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$Builder = Join-Path $PSScriptRoot "BuildW11SteamAcceptanceKit.ps1"
foreach ($RequiredPath in @(
    $Builder,
    (Join-Path $PSScriptRoot "verify_w11_steam_acceptance_kit.py"))) {
    if (-not (Test-Path -LiteralPath $RequiredPath -PathType Leaf)) {
        throw "Required W11 Steam kit builder file not found: $RequiredPath"
    }
}
$Tokens = $null
$Errors = $null
[System.Management.Automation.Language.Parser]::ParseFile(
    (Resolve-Path -LiteralPath $Builder), [ref]$Tokens, [ref]$Errors) | Out-Null
if ($Errors.Count -gt 0) {
    throw "PowerShell syntax errors in $Builder`: $($Errors.Message -join '; ')"
}

$PlanDirectory = Join-Path ([System.IO.Path]::GetTempPath()) (
    "W11SteamKitPlan_" + [guid]::NewGuid().ToString("N"))
$Plan = @(& $Builder -OutputDirectory $PlanDirectory -PlanOnly)
$PlanLine = @($Plan | Where-Object {
    $_ -is [string] -and $_ -like "W11_STEAM_ACCEPTANCE_KIT_PLAN *"
})
if ($PlanLine.Count -ne 1) {
    throw "Expected exactly one W11 Steam acceptance kit plan line."
}
foreach ($Fact in @(
    "Runtime=CookedGame",
    "Platform=Win64",
    "PackageHash=",
    "PortableRelease=1",
    "SourceProjectRequired=0",
    "UnrealEngineRequired=0",
    "ProcessTarget=RuntimeExecutable",
    "Integrity=FullInventory",
    "LaunchContract=RunnerOnlyUserDirOutsidePackage",
    "VerificationContract=PreLaunchAndPostExitFullInventory",
    "Overwrite=0")) {
    if ($PlanLine[0] -notlike "*$Fact*") {
        throw "W11 Steam acceptance kit plan is missing $Fact."
    }
}
if (Test-Path -LiteralPath $PlanDirectory) {
    throw "W11 Steam acceptance kit PlanOnly unexpectedly created its output directory."
}

$Source = Get-Content -LiteralPath $Builder -Raw
foreach ($Required in @(
    'Copy-Item -LiteralPath $PackageRoot',
    'RunW11SteamAcceptance.ps1',
    'TestW11SteamAcceptanceEnvironment.ps1',
    'verify_w11_steam_acceptance_logs.py',
    'verify_w11_steam_package_manifest.py',
    'verify_w11_steam_acceptance_kit.py',
    'README_W11_STEAM_ACCEPTANCE.md',
    'Copy-Item -LiteralPath $ReadmeTemplate',
    'W11SteamAcceptanceKitManifest.json',
    'schema = 3',
    'supportedScenarios = @("BasicJoin", "FriendInvite", "HostExit", "ClientDisconnect", "PlayerSync")',
    'fullInventorySealed = $true',
    'launchContract = "RunnerOnlyUserDirOutsidePackage"',
    'verificationContract = "PreLaunchAndPostExitFullInventory"',
    'artifactCount = $ToolArtifacts.Count',
    'mutablePaths = @("Saved/")',
    '"FirewallPolicyReview"',
    'sourceProjectRequired = $false',
    'unrealEngineRequired = $false')) {
    if (-not $Source.Contains($Required)) {
        throw "W11 Steam acceptance kit builder is missing contract: $Required"
    }
}
if (-not $Source.Contains('--require-process-target')) {
    throw "W11 Steam acceptance kit builder does not require a sealed runtime process target."
}
foreach ($Forbidden in @("Remove-Item", "-clean", "steam_appid.txt")) {
    if ($Source.Contains($Forbidden)) {
        throw "W11 Steam acceptance kit builder contains forbidden behavior: $Forbidden"
    }
}

Write-Output (
    "W11_STEAM_ACCEPTANCE_KIT_BUILDER_GATE_PASSED " +
    "PortableRelease=1 NestedPackage=1 ToolHashes=1 Overwrite=0 " +
    "EnvironmentPreflight=1 FirewallMutation=0 " +
    "SourceProjectRequired=0 UnrealEngineRequired=0 Scenarios=5 Schema=3 " +
    "FullInventory=1 ProcessTarget=RuntimeExecutable LaunchContract=RunnerOnly " +
    "VerificationContract=DualInventory"
)
