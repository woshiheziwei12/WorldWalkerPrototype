[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Launcher = Join-Path $PSScriptRoot "RunW11SteamAcceptance.ps1"
$EnvironmentPreflight = Join-Path $PSScriptRoot "TestW11SteamAcceptanceEnvironment.ps1"
$LogVerifier = Join-Path $PSScriptRoot "verify_w11_steam_acceptance_logs.py"
$PackageManifest = Join-Path $ProjectRoot "Saved\SteamAcceptance\Packages\W11Steam_Final43_Development\W11SteamPackageManifest.json"
$PortableKitRoot = Join-Path $ProjectRoot "Saved\SteamAcceptance\Kits\W11Steam_Final43_Portable_v8"
$PortableLauncher = Join-Path $PortableKitRoot "Tools\RunW11SteamAcceptance.ps1"
$PortableEnvironmentPreflight = Join-Path $PortableKitRoot "Tools\TestW11SteamAcceptanceEnvironment.ps1"
$PortablePackageManifest = Join-Path $PortableKitRoot "Package\W11SteamPackageManifest.json"

foreach ($File in @(
    $Launcher,
    $EnvironmentPreflight,
    $LogVerifier,
    $PackageManifest,
    $PortableLauncher,
    $PortableEnvironmentPreflight,
    $PortablePackageManifest)) {
    if (-not (Test-Path -LiteralPath $File)) {
        throw "Required Steam acceptance tool not found: $File"
    }
}

$Tokens = $null
$Errors = $null
[System.Management.Automation.Language.Parser]::ParseFile(
    (Resolve-Path -LiteralPath $Launcher), [ref]$Tokens, [ref]$Errors) | Out-Null
if ($Errors.Count -gt 0) {
    throw "PowerShell syntax errors in $Launcher`: $($Errors.Message -join '; ')"
}
$PreflightTokens = $null
$PreflightErrors = $null
[System.Management.Automation.Language.Parser]::ParseFile(
    (Resolve-Path -LiteralPath $EnvironmentPreflight),
    [ref]$PreflightTokens,
    [ref]$PreflightErrors) | Out-Null
if ($PreflightErrors.Count -gt 0) {
    throw "PowerShell syntax errors in $EnvironmentPreflight`: $($PreflightErrors.Message -join '; ')"
}
$PreflightSelfTest = @(& $EnvironmentPreflight -SelfTest)
if (@($PreflightSelfTest | Where-Object {
    $_ -is [string] -and $_ -like "W11_STEAM_ENVIRONMENT_PREFLIGHT_SELF_TEST_PASSED *"
}).Count -ne 1) {
    throw "Steam environment preflight self-test did not pass."
}

$Token = "W11_Contract_20260830"
$AppId = "1234567"
$HostDirectory = Join-Path ([System.IO.Path]::GetTempPath()) (
    "W11SteamHostPlan_" + [guid]::NewGuid().ToString("N"))
$ClientDirectory = Join-Path ([System.IO.Path]::GetTempPath()) (
    "W11SteamClientPlan_" + [guid]::NewGuid().ToString("N"))
$HostPlan = @(& $Launcher `
    -Role Host `
    -AppId $AppId `
    -EvidenceToken $Token `
    -SessionDirectory $HostDirectory `
    -PlanOnly)
$ClientPlan = @(& $Launcher `
    -Role Client `
    -AppId $AppId `
    -EvidenceToken $Token `
    -SessionDirectory $ClientDirectory `
    -PlanOnly)

$HostLine = @($HostPlan | Where-Object { $_ -is [string] -and $_ -like "W11_STEAM_ACCEPTANCE_PLAN *" })
$ClientLine = @($ClientPlan | Where-Object { $_ -is [string] -and $_ -like "W11_STEAM_ACCEPTANCE_PLAN *" })
if ($HostLine.Count -ne 1 -or $ClientLine.Count -ne 1) {
    throw "Steam PlanOnly must output exactly one plan line for each role."
}
foreach ($Entry in @(
    @{ Line = $HostLine[0]; Role = "Host" },
    @{ Line = $ClientLine[0]; Role = "Client" })) {
    foreach ($Fact in @(
        "Role=$($Entry.Role)", "AppId=$AppId", "Token=$Token",
        "Scenario=BasicJoin", "ExpectedPlayers=2",
        "OSS=Steam", "Placeholder=0", "Seed=424242", "Interactive=1",
        "Runtime=EditorStandalone", "PackageHash=EDITOR", "PackageIntegrity=Editor",
        "UserDirOutsidePackage=1", "ProcessTarget=Editor",
        "PostExitPackageVerification=EditorLogOnly", "DurableRoleReceipt=0",
        "ParticipantIdentity=IntegrationOnly",
        "PreLaunchKitVerification=NotPortable",
        "PostExitKitVerification=NotPortable",
        "LaunchContract=ExternalPackage",
        "WaitsForGameExit=1")) {
        if ($Entry.Line -notlike "*$Fact*") {
            throw "Steam $($Entry.Role) plan is missing $Fact."
        }
    }
}

$ReleaseDirectory = Join-Path ([System.IO.Path]::GetTempPath()) (
    "W11SteamReleasePlan_" + [guid]::NewGuid().ToString("N"))
$ReleasePlan = @(& $Launcher `
    -Role Host `
    -AppId $AppId `
    -EvidenceToken $Token `
    -Mode Release `
    -PackageManifest $PackageManifest `
    -SessionDirectory $ReleaseDirectory `
    -PlanOnly)
$ReleaseLine = @($ReleasePlan | Where-Object {
    $_ -is [string] -and $_ -like "W11_STEAM_ACCEPTANCE_PLAN *"
})
if ($ReleaseLine.Count -ne 1 `
    -or $ReleaseLine[0] -notlike "*Scenario=BasicJoin*ExpectedPlayers=2*" `
    -or $ReleaseLine[0] -notlike "*Runtime=CookedGame*" `
    -or $ReleaseLine[0] -notlike "*PackageIntegrity=FullInventory*" `
    -or $ReleaseLine[0] -notlike "*UserDirOutsidePackage=1*" `
    -or $ReleaseLine[0] -notlike "*ProcessTarget=RuntimeExecutable*" `
    -or $ReleaseLine[0] -notlike "*PostExitPackageVerification=FullInventory*" `
    -or $ReleaseLine[0] -notlike "*DurableRoleReceipt=1*" `
    -or $ReleaseLine[0] -notlike "*ParticipantIdentity=Steam64Required*" `
    -or $ReleaseLine[0] -notlike "*PreLaunchKitVerification=NotPortable*" `
    -or $ReleaseLine[0] -notlike "*PostExitKitVerification=NotPortable*" `
    -or $ReleaseLine[0] -notlike "*LaunchContract=ExternalPackage*" `
    -or $ReleaseLine[0] -notlike "*W11SteamBasicJoinHostReceipt.json*" `
    -or $ReleaseLine[0] -notlike "*WaitsForGameExit=1*" `
    -or $ReleaseLine[0] -notlike "*Binaries\Win64\WorldWalkerPrototype.exe*" `
    -or $ReleaseLine[0] -notmatch "PackageHash=[0-9a-f]{64}") {
    throw "Steam Release PlanOnly does not require a cooked-game runtime."
}
if (Test-Path -LiteralPath $ReleaseDirectory) {
    throw "Steam Release PlanOnly unexpectedly created an evidence directory."
}

$PortableDirectory = Join-Path ([System.IO.Path]::GetTempPath()) (
    "W11SteamPortablePlan_" + [guid]::NewGuid().ToString("N"))
$PortablePlan = @(& $PortableLauncher `
    -Role Host `
    -AppId $AppId `
    -EvidenceToken $Token `
    -Mode Release `
    -PackageManifest $PortablePackageManifest `
    -SessionDirectory $PortableDirectory `
    -PlanOnly)
$PortableLine = @($PortablePlan | Where-Object {
    $_ -is [string] -and $_ -like "W11_STEAM_ACCEPTANCE_PLAN *"
})
if ($PortableLine.Count -ne 1 `
    -or $PortableLine[0] -notlike "*PreLaunchKitVerification=FullInventory*" `
    -or $PortableLine[0] -notlike "*PostExitKitVerification=FullInventory*" `
    -or $PortableLine[0] -notlike "*ReceiptKitIdentity=SealedAfterExit*" `
    -or $PortableLine[0] -notlike "*LaunchContract=RunnerOnlyUserDirOutsidePackage*") {
    throw "Portable Steam Release plan does not automatically verify the full kit."
}
if (Test-Path -LiteralPath $PortableDirectory) {
    throw "Portable Steam Release PlanOnly unexpectedly created an evidence directory."
}
if ($HostLine[0] -notlike "*W11SteamBasicJoinHost.log*" `
    -or $ClientLine[0] -notlike "*W11SteamBasicJoinClient.log*") {
    throw "Steam role plans do not use unique role-specific log names."
}

$ScenarioPlans = @{}
foreach ($Scenario in @("FriendInvite", "HostExit", "ClientDisconnect", "PlayerSync")) {
    $ExpectedPlayers = if ($Scenario -eq "PlayerSync") { 4 } else { 2 }
    $ScenarioDirectory = Join-Path ([System.IO.Path]::GetTempPath()) (
        "W11SteamScenarioPlan_" + [guid]::NewGuid().ToString("N"))
    $Plan = @(& $Launcher `
        -Role Host `
        -AppId $AppId `
        -EvidenceToken $Token `
        -Mode Release `
        -Scenario $Scenario `
        -ExpectedPlayers $ExpectedPlayers `
        -PackageManifest $PackageManifest `
        -SessionDirectory $ScenarioDirectory `
        -PlanOnly)
    $PlanLine = @($Plan | Where-Object {
        $_ -is [string] -and $_ -like "W11_STEAM_ACCEPTANCE_PLAN *"
    })
    if ($PlanLine.Count -ne 1 `
        -or $PlanLine[0] -notlike "*Scenario=$Scenario*" `
        -or $PlanLine[0] -notlike "*ExpectedPlayers=$ExpectedPlayers*" `
        -or $PlanLine[0] -notlike "*Runtime=CookedGame*") {
        throw "Steam $Scenario Release plan is missing its scenario contract."
    }
    if (Test-Path -LiteralPath $ScenarioDirectory) {
        throw "Steam $Scenario PlanOnly unexpectedly created an evidence directory."
    }
    $ScenarioPlans[$Scenario] = $PlanLine[0]
}

$IntegrationScenarioRejected = $false
try {
    & $Launcher -Role Host -AppId $AppId -EvidenceToken $Token `
        -Scenario FriendInvite -PlanOnly 2>$null | Out-Null
}
catch {
    $IntegrationScenarioRejected = $_.Exception.Message -like "*require -Mode Release*"
}
if (-not $IntegrationScenarioRejected) {
    throw "Steam launcher accepted a non-BasicJoin scenario in editor integration mode."
}
if ((Test-Path -LiteralPath $HostDirectory) -or (Test-Path -LiteralPath $ClientDirectory)) {
    throw "Steam PlanOnly unexpectedly created an evidence directory."
}

$PlaceholderRejected = $false
try {
    & $Launcher -Role Host -AppId 480 -EvidenceToken $Token -PlanOnly 2>$null | Out-Null
}
catch {
    $PlaceholderRejected = $_.Exception.Message -like "*480 is not accepted*"
}
if (-not $PlaceholderRejected) {
    throw "Steam launcher did not reject placeholder App ID 480."
}

$UnsealedReleaseRejected = $false
try {
    & $Launcher -Role Host -AppId $AppId -EvidenceToken $Token `
        -Mode Release -PackagedExecutable (Join-Path $ProjectRoot "Binaries\Win64\WorldWalkerPrototype.exe") `
        -PlanOnly 2>$null | Out-Null
}
catch {
    $UnsealedReleaseRejected = $_.Exception.Message -like "*-PackageManifest*"
}
if (-not $UnsealedReleaseRejected) {
    throw "Steam launcher accepted an unsealed Release executable without a package manifest."
}

$PackageRoot = Split-Path -Parent $PackageManifest
$PackageMutationRejected = $false
try {
    & $Launcher -Role Host -AppId $AppId -EvidenceToken $Token `
        -Mode Release -PackageManifest $PackageManifest `
        -SessionDirectory (Join-Path $PackageRoot "ForbiddenEvidence") `
        -PlanOnly 2>$null | Out-Null
}
catch {
    $PackageMutationRejected = $_.Exception.Message -like "*outside the sealed package root*"
}
if (-not $PackageMutationRejected) {
    throw "Steam launcher accepted evidence/user data inside the sealed package root."
}

$Source = Get-Content -LiteralPath $Launcher -Raw
foreach ($Required in @(
    '-W11SteamEvidenceRole=$Role',
    '-W11SteamEvidenceScenario=$Scenario',
    '-W11SteamEvidenceToken=$EvidenceToken',
    '-W11SteamPackageHash=$PackageHash',
    'DefaultPlatformService=Steam',
    'SteamDevAppId=$AppId',
    '$env:SteamAppId = $AppId',
    '$env:SteamGameId = $AppId',
    '"--role", $Role',
    '"--scenario", $Scenario',
    '"--expected-players", $ExpectedPlayers',
    '"--app-id", $AppId',
    '"--token", $EvidenceToken',
    '"--package-manifest", $PackageManifest',
    '$VerifierArguments += @("--kit-manifest", $PortableKitManifest)',
    '"--output", $ReceiptPath',
    '$VerifierArguments += @("--package-hash", $PackageHash, "--allow-editor-integration")',
    'verify_w11_steam_package_manifest.py',
    'verify_w11_steam_acceptance_kit.py',
    '$PortableKitManifest = Join-Path $ProjectRoot "W11SteamAcceptanceKitManifest.json"',
    'PortableKitVerification',
    'Get-FileHash -LiteralPath $PackageManifest -Algorithm SHA256',
    '--require-full-inventory',
    '--require-process-target',
    '$RuntimeExecutableRelative = [string]$PackageData.runtimeExecutable',
    '-UserDir=`"$UserDataDirectory`"',
    'UserDirOutsidePackage=1',
    'Mode -eq "Release"',
    'RuntimeKind = "CookedGame"',
    'PostExitPackageVerification=')) {
    if (-not $Source.Contains($Required)) {
        throw "Steam launcher source is missing contract: $Required"
    }
}
foreach ($Forbidden in @(
    "Config\\DefaultEngine.ini",
    "steam_appid.txt",
    "SteamDevAppId=480",
    "-W11AutoStart",
    "-W11InputScript",
    "-NullRHI",
    "-Unattended")) {
    if ($Source.Contains($Forbidden)) {
        throw "Steam launcher source contains forbidden behavior: $Forbidden"
    }
}
$KitVerificationCalls = [regex]::Matches(
    $Source,
    [regex]::Escape('& $PythonCommand -B $KitVerifier $PortableKitManifest')).Count
if ($KitVerificationCalls -lt 2) {
    throw "Steam launcher must verify the portable kit both before launch and after exit."
}

$PreflightSource = Get-Content -LiteralPath $EnvironmentPreflight -Raw
foreach ($Required in @(
    'Get-Process -Name steam',
    'Get-NetConnectionProfile -ErrorAction Stop',
    'Get-NetFirewallProfile -PolicyStore ActiveStore',
    'Get-NetFirewallApplicationFilter `',
    '-PolicyStore ActiveStore -Program $RuntimeExecutable',
    'formalSteamGate = "PENDING"',
    'ProcessTarget=RuntimeExecutable',
    'ADMIN_REVIEW_REQUIRED',
    'ACCESS_DENIED')) {
    if (-not $PreflightSource.Contains($Required)) {
        throw "Steam environment preflight source is missing contract: $Required"
    }
}
foreach ($Forbidden in @(
    'New-NetFirewallRule',
    'Set-NetFirewallProfile',
    'Set-NetFirewallRule',
    'Remove-NetFirewallRule',
    'netsh advfirewall')) {
    if ($PreflightSource.Contains($Forbidden)) {
        throw "Steam environment preflight must remain read-only: $Forbidden"
    }
}

Write-Output (
    "W11_STEAM_ACCEPTANCE_LAUNCHER_GATE_PASSED " +
    "Roles=Host,Client ProjectAppIdRequired=1 Placeholder480Rejected=1 " +
    "SharedToken=1 SharedPackageHash=1 PackageManifestRequired=1 UnsealedReleaseRejected=1 " +
    "ConfigMutation=0 PackageMutationRejected=1 FullInventory=1 UserDirOutsidePackage=1 " +
    "ProcessTarget=RuntimeExecutable WaitsForGameExit=1 PostExitPackageVerified=1 " +
    "PortableKitDualVerification=1 PortableKitReceiptIdentity=1 LaunchContract=RunnerOnly " +
    "EnvironmentPreflight=ReadOnly FirewallMutation=0 " +
    "DurableRoleReceipt=1 " +
    "DistinctSteamIdentity=1 " +
    "IntegrationRuntime=EditorStandalone ReleaseRuntime=CookedGame " +
    "Scenarios=BasicJoin,FriendInvite,HostExit,ClientDisconnect,PlayerSync " +
    "PlayerSync4=1 IntegrationScenarioRejected=1 Interactive=1 PairGate=PENDING"
)
