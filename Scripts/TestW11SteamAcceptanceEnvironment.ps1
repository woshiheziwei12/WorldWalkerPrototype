[CmdletBinding()]
param(
    [string]$AppId = "",
    [string]$PackageManifest = "",
    [string]$KitManifest = "",
    [string]$PythonCommand = "python",
    [string]$Output = "",
    [switch]$SelfTest
)

$ErrorActionPreference = "Stop"

function Get-W11PreflightClassification {
    param(
        [bool]$PythonReady,
        [bool]$PackageReady,
        [bool]$KitReady,
        [bool]$SteamClientReady,
        [string]$FirewallInspection,
        [bool]$FirewallEnabled,
        [bool]$MatchingAllowRule
    )

    if (-not ($PythonReady -and $PackageReady -and $KitReady -and $SteamClientReady)) {
        return [ordered]@{
            status = "BLOCKED"
            firewallReview = "NOT_EVALUATED"
            exitCode = 1
        }
    }
    if (-not $FirewallEnabled) {
        return [ordered]@{
            status = "READY"
            firewallReview = "NOT_REQUIRED"
            exitCode = 0
        }
    }
    if ($FirewallInspection -ne "AVAILABLE") {
        return [ordered]@{
            status = "ADMIN_REVIEW_REQUIRED"
            firewallReview = $FirewallInspection
            exitCode = 2
        }
    }
    if (-not $MatchingAllowRule) {
        return [ordered]@{
            status = "FIREWALL_ACTION_REQUIRED"
            firewallReview = "NO_ENABLED_ALLOW_RULE"
            exitCode = 2
        }
    }
    return [ordered]@{
        status = "READY"
        firewallReview = "MATCHING_ALLOW_RULE"
        exitCode = 0
    }
}

function Test-W11AppId {
    param([string]$Value)
    return $Value -match "^[1-9][0-9]+$" -and $Value -ne "480"
}

function Test-W11PathWithin {
    param([string]$Path, [string]$Root)
    $ResolvedPath = [System.IO.Path]::GetFullPath($Path).TrimEnd('\')
    $ResolvedRoot = [System.IO.Path]::GetFullPath($Root).TrimEnd('\')
    return $ResolvedPath.Equals($ResolvedRoot, [System.StringComparison]::OrdinalIgnoreCase) -or
        $ResolvedPath.StartsWith(
            $ResolvedRoot + [System.IO.Path]::DirectorySeparatorChar,
            [System.StringComparison]::OrdinalIgnoreCase)
}

if ($SelfTest) {
    if (-not (Test-W11AppId "1234567") -or (Test-W11AppId "480") -or (Test-W11AppId "bad")) {
        throw "Steam environment preflight App ID self-test failed."
    }
    $SyntheticRoot = Join-Path ([System.IO.Path]::GetTempPath()) "W11PreflightRoot"
    if (-not (Test-W11PathWithin (Join-Path $SyntheticRoot "Saved\result.json") $SyntheticRoot) -or
        (Test-W11PathWithin ($SyntheticRoot + "Sibling\result.json") $SyntheticRoot)) {
        throw "Steam environment preflight path-boundary self-test failed."
    }
    $Ready = Get-W11PreflightClassification $true $true $true $true "AVAILABLE" $true $true
    $Managed = Get-W11PreflightClassification $true $true $true $true "ACCESS_DENIED" $true $false
    $MissingSteam = Get-W11PreflightClassification $true $true $true $false "AVAILABLE" $true $true
    $FirewallOff = Get-W11PreflightClassification $true $true $true $true "AVAILABLE" $false $false
    if ($Ready.status -ne "READY" -or $Ready.exitCode -ne 0) {
        throw "Steam environment preflight ready classification failed."
    }
    if ($Managed.status -ne "ADMIN_REVIEW_REQUIRED" -or $Managed.exitCode -ne 2) {
        throw "Steam environment preflight managed-firewall classification failed."
    }
    if ($MissingSteam.status -ne "BLOCKED" -or $MissingSteam.exitCode -ne 1) {
        throw "Steam environment preflight missing-Steam classification failed."
    }
    if ($FirewallOff.status -ne "READY" -or $FirewallOff.firewallReview -ne "NOT_REQUIRED") {
        throw "Steam environment preflight disabled-firewall classification failed."
    }
    Write-Output (
        "W11_STEAM_ENVIRONMENT_PREFLIGHT_SELF_TEST_PASSED " +
        "ProjectAppId=1 PlaceholderRejected=1 Ready=1 MissingSteamBlocked=1 " +
        "ManagedFirewallReview=1 FirewallDisabledReady=1 PathBoundary=1"
    )
    return
}

if (-not (Test-W11AppId $AppId)) {
    throw "-AppId must be a non-placeholder numeric project Steam App ID; 480 is not accepted."
}

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$PortableManifestCandidate = Join-Path $ProjectRoot "W11SteamAcceptanceKitManifest.json"
if ([string]::IsNullOrWhiteSpace($KitManifest) -and
    (Test-Path -LiteralPath $PortableManifestCandidate -PathType Leaf)) {
    $KitManifest = $PortableManifestCandidate
}
if ([string]::IsNullOrWhiteSpace($PackageManifest)) {
    if (-not [string]::IsNullOrWhiteSpace($KitManifest)) {
        $PackageManifest = Join-Path (Split-Path -Parent $KitManifest) (
            "Package\W11SteamPackageManifest.json")
    }
    else {
        $PackageManifest = Join-Path $ProjectRoot (
            "Saved\SteamAcceptance\Packages\W11Steam_Final43_Development\W11SteamPackageManifest.json")
    }
}

$PackageManifest = [System.IO.Path]::GetFullPath($PackageManifest)
if (-not [string]::IsNullOrWhiteSpace($KitManifest)) {
    $KitManifest = [System.IO.Path]::GetFullPath($KitManifest)
}
$PackageVerifier = Join-Path $PSScriptRoot "verify_w11_steam_package_manifest.py"
$KitVerifier = Join-Path $PSScriptRoot "verify_w11_steam_acceptance_kit.py"
foreach ($RequiredPath in @($PackageManifest, $PackageVerifier)) {
    if (-not (Test-Path -LiteralPath $RequiredPath -PathType Leaf)) {
        throw "Required W11 Steam preflight file not found: $RequiredPath"
    }
}
if (-not [string]::IsNullOrWhiteSpace($KitManifest) -and
    -not (Test-Path -LiteralPath $KitVerifier -PathType Leaf)) {
    throw "Required W11 Steam kit verifier not found: $KitVerifier"
}

$PythonText = @(& $PythonCommand --version 2>&1) -join " "
$PythonReady = $LASTEXITCODE -eq 0 -and $PythonText -match "Python\s+3\."
if (-not $PythonReady) {
    throw "Python 3 is required; detected: $PythonText"
}

& $PythonCommand -B $PackageVerifier $PackageManifest --require-full-inventory --require-process-target
$PackageReady = $LASTEXITCODE -eq 0
if (-not $PackageReady) {
    throw "W11 Steam package preflight failed with exit code $LASTEXITCODE."
}

$KitReady = $true
if (-not [string]::IsNullOrWhiteSpace($KitManifest)) {
    & $PythonCommand -B $KitVerifier $KitManifest
    $KitReady = $LASTEXITCODE -eq 0
    if (-not $KitReady) {
        throw "W11 Steam portable kit preflight failed with exit code $LASTEXITCODE."
    }
}

$PackageData = Get-Content -LiteralPath $PackageManifest -Raw | ConvertFrom-Json
$RuntimeExecutableRelative = [string]$PackageData.runtimeExecutable
if ([string]::IsNullOrWhiteSpace($RuntimeExecutableRelative)) {
    throw "Package manifest does not declare runtimeExecutable."
}
$RuntimeExecutable = [System.IO.Path]::GetFullPath((
    Join-Path (Split-Path -Parent $PackageManifest) $RuntimeExecutableRelative))
if (-not (Test-Path -LiteralPath $RuntimeExecutable -PathType Leaf)) {
    throw "Sealed W11 runtime executable is missing: $RuntimeExecutable"
}
$PackageHash = (Get-FileHash -LiteralPath $PackageManifest -Algorithm SHA256).Hash.ToLowerInvariant()
$KitHash = if ([string]::IsNullOrWhiteSpace($KitManifest)) {
    "NOT_PORTABLE"
}
else {
    (Get-FileHash -LiteralPath $KitManifest -Algorithm SHA256).Hash.ToLowerInvariant()
}

$SteamProcesses = @(Get-Process -Name steam -ErrorAction SilentlyContinue)
$SteamClientReady = $SteamProcesses.Count -gt 0

$FirewallInspection = "UNAVAILABLE"
$FirewallEnabled = $true
$MatchingAllowRule = $false
$FirewallProfiles = @()
$FirewallRules = @()
$NetworkProfiles = @()
try {
    $Profiles = @(Get-NetFirewallProfile -PolicyStore ActiveStore -ErrorAction Stop)
    $Connections = @(Get-NetConnectionProfile -ErrorAction Stop)
    $FirewallInspection = "AVAILABLE"
    $FirewallEnabled = @($Profiles | Where-Object { $_.Enabled }).Count -gt 0
    $ActiveProfileNames = @($Connections | ForEach-Object {
        [string]$_.NetworkCategory
    } | Where-Object { $_ -in @("DomainAuthenticated", "Private", "Public") } |
        ForEach-Object { if ($_ -eq "DomainAuthenticated") { "Domain" } else { $_ } } |
        Select-Object -Unique)
    $NetworkProfiles = @($Connections | ForEach-Object {
        [ordered]@{
            name = [string]$_.Name
            interfaceAlias = [string]$_.InterfaceAlias
            category = [string]$_.NetworkCategory
            ipv4Connectivity = [string]$_.IPv4Connectivity
            ipv6Connectivity = [string]$_.IPv6Connectivity
        }
    })
    $FirewallProfiles = @($Profiles | ForEach-Object {
        [ordered]@{
            name = [string]$_.Name
            enabled = [bool]$_.Enabled
            defaultInboundAction = [string]$_.DefaultInboundAction
            defaultOutboundAction = [string]$_.DefaultOutboundAction
            allowInboundRules = [string]$_.AllowInboundRules
            allowLocalFirewallRules = [string]$_.AllowLocalFirewallRules
        }
    })
    if ($FirewallEnabled) {
        if (@($Profiles | Where-Object {
            $_.Enabled -and [string]$_.DefaultInboundAction -eq "Allow" -and
            ([string]$_.Name -in $ActiveProfileNames)
        }).Count -gt 0) {
            $MatchingAllowRule = $true
        }
        $ApplicationFilters = @(Get-NetFirewallApplicationFilter `
            -PolicyStore ActiveStore -Program $RuntimeExecutable -ErrorAction Stop)
        foreach ($Filter in $ApplicationFilters) {
            $Rule = $Filter | Get-NetFirewallRule -ErrorAction Stop
            $FirewallRules += [ordered]@{
                displayName = [string]$Rule.DisplayName
                enabled = [string]$Rule.Enabled
                action = [string]$Rule.Action
                direction = [string]$Rule.Direction
                profile = [string]$Rule.Profile
            }
            $RuleProfiles = [string]$Rule.Profile
            $RuleMatchesActiveProfile = $RuleProfiles -eq "Any" -or @(
                $ActiveProfileNames | Where-Object { $RuleProfiles -match "(^|,\s*)$([regex]::Escape($_))(,|$)" }
            ).Count -gt 0
            if ($Rule.Enabled -eq "True" -and $Rule.Action -eq "Allow" -and
                $Rule.Direction -eq "Inbound" -and $RuleMatchesActiveProfile) {
                $MatchingAllowRule = $true
            }
        }
    }
}
catch [System.UnauthorizedAccessException] {
    $FirewallInspection = "ACCESS_DENIED"
}
catch {
    if ($_.Exception.Message -match "拒绝访问|Access.*denied") {
        $FirewallInspection = "ACCESS_DENIED"
    }
    else {
        $FirewallInspection = "ERROR"
    }
}

$Classification = Get-W11PreflightClassification `
    $PythonReady $PackageReady $KitReady $SteamClientReady `
    $FirewallInspection $FirewallEnabled $MatchingAllowRule
$Evidence = [ordered]@{
    schema = 1
    kind = "W11SteamEnvironmentPreflight"
    checkedAt = [DateTime]::UtcNow.ToString("o")
    status = $Classification.status
    formalSteamGate = "PENDING"
    appId = $AppId
    python = [ordered]@{
        ready = $PythonReady
        version = $PythonText
    }
    steamClient = [ordered]@{
        ready = $SteamClientReady
        processCount = $SteamProcesses.Count
    }
    package = [ordered]@{
        ready = $PackageReady
        manifest = $PackageManifest
        manifestSha256 = $PackageHash
        processTarget = "RuntimeExecutable"
        runtimeExecutable = $RuntimeExecutable
    }
    portableKit = [ordered]@{
        ready = $KitReady
        manifest = if ([string]::IsNullOrWhiteSpace($KitManifest)) { "NOT_PORTABLE" } else { $KitManifest }
        manifestSha256 = $KitHash
    }
    firewall = [ordered]@{
        inspection = $FirewallInspection
        enabled = $FirewallEnabled
        matchingAllowRule = $MatchingAllowRule
        review = $Classification.firewallReview
        networkProfiles = $NetworkProfiles
        profiles = $FirewallProfiles
        rules = $FirewallRules
    }
    note = "This is a read-only readiness diagnostic; it does not modify firewall policy or prove real Steam connectivity."
}

if (-not [string]::IsNullOrWhiteSpace($Output)) {
    $Output = [System.IO.Path]::GetFullPath($Output)
    $PackageRoot = (Split-Path -Parent $PackageManifest)
    if (Test-W11PathWithin $Output $PackageRoot) {
        throw "Steam environment preflight output must stay outside the sealed package root."
    }
    if (-not [string]::IsNullOrWhiteSpace($KitManifest)) {
        $KitRoot = Split-Path -Parent $KitManifest
        $MutableKitRoot = Join-Path $KitRoot "Saved"
        if ((Test-W11PathWithin $Output $KitRoot) -and
            -not (Test-W11PathWithin $Output $MutableKitRoot)) {
            throw "Portable preflight output inside the kit must use its mutable Saved directory."
        }
    }
    $OutputDirectory = Split-Path -Parent $Output
    if (-not (Test-Path -LiteralPath $OutputDirectory)) {
        New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
    }
    if (Test-Path -LiteralPath $Output) {
        throw "Steam environment preflight output already exists: $Output"
    }
    $Evidence | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $Output -Encoding utf8
}

Write-Output (
    "W11_STEAM_ENVIRONMENT_PREFLIGHT Status=$($Classification.status) " +
    "AppId=$AppId Python3=$(if ($PythonReady) { '1' } else { '0' }) " +
    "SteamClient=$(if ($SteamClientReady) { 'RUNNING' } else { 'NOT_RUNNING' }) " +
    "PackageHash=$PackageHash KitHash=$KitHash ProcessTarget=RuntimeExecutable " +
    "FirewallInspection=$FirewallInspection FirewallReview=$($Classification.firewallReview) " +
    "FormalSteamGate=PENDING Output=$(if ([string]::IsNullOrWhiteSpace($Output)) { 'NONE' } else { $Output })"
)
if ($Classification.exitCode -ne 0) {
    exit $Classification.exitCode
}
