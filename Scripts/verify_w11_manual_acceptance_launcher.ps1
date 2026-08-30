[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$RunW11 = Join-Path $PSScriptRoot "RunW11.ps1"
$Launcher = Join-Path $PSScriptRoot "RunW11ManualAcceptance.ps1"
$Finalizer = Join-Path $PSScriptRoot "finalize_w11_manual_acceptance_signoff.py"
$PackageManifest = Join-Path $ProjectRoot (
    "Saved\SteamAcceptance\Packages\W11Steam_Final43_Development\W11SteamPackageManifest.json")
$ExpectedRoles = @("Observe", "Evasion", "Dodge", "Recovery", "Feedback")
$ExpectedResolutions = @("3840x2160", "2560x1440", "1920x1080", "1280x720")

foreach ($RequiredPath in @($Finalizer, $PackageManifest)) {
    if (-not (Test-Path -LiteralPath $RequiredPath -PathType Leaf)) {
        throw "Required manual acceptance contract file not found: $RequiredPath"
    }
}

foreach ($File in @($RunW11, $Launcher)) {
    $Tokens = $null
    $Errors = $null
    [System.Management.Automation.Language.Parser]::ParseFile(
        (Resolve-Path -LiteralPath $File), [ref]$Tokens, [ref]$Errors) | Out-Null
    if ($Errors.Count -gt 0) {
        throw "PowerShell syntax errors in $File`: $($Errors.Message -join '; ')"
    }
}

# This must return to the caller rather than terminating the five-role driver.
& $RunW11 -SkipBuild -BuildOnly
Write-Output "W11_MANUAL_LAUNCHER_NESTED_RETURN_OK"

$PlanDirectory = Join-Path ([System.IO.Path]::GetTempPath()) (
    "W11ManualAcceptancePlanOnly_" + [guid]::NewGuid().ToString("N"))
$Plan = @(& $Launcher `
    -Operator "ContractTest" `
    -SessionDirectory $PlanDirectory `
    -PlanOnly)
$RoleLines = @($Plan | Where-Object {
    $_ -is [string] -and $_ -like "W11_MANUAL_ACCEPTANCE_PLAN Role=*"
})
if ($RoleLines.Count -ne $ExpectedRoles.Count) {
    throw "Expected $($ExpectedRoles.Count) role plans, found $($RoleLines.Count)."
}

$LogPaths = @()
for ($Index = 0; $Index -lt $ExpectedRoles.Count; ++$Index) {
    $Role = $ExpectedRoles[$Index]
    $Line = $RoleLines[$Index]
    if ($Line -notmatch "Role=$Role ") {
        throw "Manual role order mismatch at index $Index`: expected $Role."
    }
    foreach ($Fact in @(
        "Seed=424242", "Interactive=1", "Rendered=1", "Audio=1",
        "InputScript=None", "AutoStart=0", "TestOverride=0",
        "ProcessTarget=Editor", "WaitsForGameExit=1",
        "SuggestedSect=Sect.", "TargetOutcome=")) {
        if ($Line -notlike "*$Fact*") {
            throw "Manual role $Role is missing plan fact $Fact."
        }
    }
    $LogPaths += ($Line -split " Log=", 2)[1]
}
if (($LogPaths | Sort-Object -Unique).Count -ne $ExpectedRoles.Count) {
    throw "The five manual roles do not have unique evidence log paths."
}
$SuggestedSects = @($RoleLines | ForEach-Object {
    if ($_ -match "SuggestedSect=([^ ]+)") { $Matches[1] }
})
$SuggestedSectCount = ($SuggestedSects | Sort-Object -Unique).Count
if ($SuggestedSectCount -lt 3) {
    throw "The launcher suggestions do not cover at least three sects."
}
$TargetOutcomes = @($RoleLines | ForEach-Object {
    if ($_ -match "TargetOutcome=([^ ]+)") { $Matches[1] }
})
if (-not $TargetOutcomes.Contains("Victory") -or -not $TargetOutcomes.Contains("Defeat")) {
    throw "The launcher suggestions do not cover both Victory and Defeat."
}
if (Test-Path -LiteralPath $PlanDirectory) {
    throw "PlanOnly unexpectedly created a session directory."
}

$CookedPlanDirectory = Join-Path ([System.IO.Path]::GetTempPath()) (
    "W11ManualAcceptanceCookedPlan_" + [guid]::NewGuid().ToString("N"))
$CookedPlan = @(& $Launcher `
    -Operator "CookedContractTest" `
    -RuntimeMode Cooked `
    -PackageManifest $PackageManifest `
    -SessionDirectory $CookedPlanDirectory `
    -PlanOnly)
$CookedRoleLines = @($CookedPlan | Where-Object {
    $_ -is [string] -and $_ -like "W11_MANUAL_ACCEPTANCE_PLAN Role=*"
})
if ($CookedRoleLines.Count -ne $ExpectedRoles.Count) {
    throw "Cooked manual PlanOnly did not produce five role plans."
}
foreach ($Line in $CookedRoleLines) {
    if ($Line -notlike "*Runtime=CookedGame*" `
        -or $Line -notmatch "PackageHash=[0-9a-f]{64}" `
        -or $Line -notlike "*ProcessTarget=RuntimeExecutable*" `
        -or $Line -notlike "*WaitsForGameExit=1*") {
        throw "Cooked manual role plan lacks the sealed CookedGame identity."
    }
}
if (Test-Path -LiteralPath $CookedPlanDirectory) {
    throw "Cooked manual PlanOnly unexpectedly created a session directory."
}

$MissingCookedPackageRejected = $false
try {
    & $Launcher -Operator "MissingPackage" -RuntimeMode Cooked -PlanOnly 2>$null | Out-Null
}
catch {
    $MissingCookedPackageRejected = $_.Exception.Message -like "*-PackageManifest*"
}
if (-not $MissingCookedPackageRejected) {
    throw "Cooked manual acceptance did not require a package manifest."
}

$CookedRunPlanLog = Join-Path ([System.IO.Path]::GetTempPath()) (
    "W11CookedManualRunPlan_" + [guid]::NewGuid().ToString("N") + ".log")
$CookedRunPlan = @(& $RunW11 `
    -RuntimeMode Cooked `
    -PackageManifest $PackageManifest `
    -SkipBuild `
    -ManualEvidenceRole Observe `
    -ManualEvidenceLog $CookedRunPlanLog `
    -RunSeed 424242 `
    -PlanOnly)
$CookedRunLine = @($CookedRunPlan | Where-Object {
    $_ -is [string] -and $_ -like "W11_RUN_PLAN *"
})
if ($CookedRunLine.Count -ne 1 `
    -or $CookedRunLine[0] -notlike "*RuntimeMode=Cooked*" `
    -or $CookedRunLine[0] -notlike "*Runtime=CookedGame*" `
    -or $CookedRunLine[0] -notmatch "PackageHash=[0-9a-f]{64}" `
    -or $CookedRunLine[0] -notlike "*UserDirOutsidePackage=1*" `
    -or $CookedRunLine[0] -notlike "*ProcessTarget=RuntimeExecutable*" `
    -or $CookedRunLine[0] -notlike "*WaitsForGameExit=0*" `
    -or $CookedRunLine[0] -notlike "*Binaries\Win64\WorldWalkerPrototype.exe*" `
    -or $CookedRunLine[0] -notlike "*Interactive=1*Rendered=1*Audio=1*InputScript=None*AutoStart=0*") {
    throw "RunW11 Cooked PlanOnly lacks the sealed interactive manual contract."
}
if (Test-Path -LiteralPath $CookedRunPlanLog) {
    throw "RunW11 Cooked PlanOnly unexpectedly created an evidence log."
}

$PlanText = $Plan -join "`n"
foreach ($ForbiddenSwitch in @(
    "-W11InputScript", "-W11AutoStart", "-NullRHI", "-Unattended",
    "-NoSound", "-RenderOffscreen")) {
    if ($PlanText.Contains($ForbiddenSwitch)) {
        throw "PlanOnly contains forbidden switch $ForbiddenSwitch."
    }
}
if ($PlanText -notmatch "W11_MANUAL_ACCEPTANCE_PLAN_PACKET .*Subjective=PENDING") {
    throw "PlanOnly does not preserve the human subjective PENDING boundary."
}
$VisualLines = @($Plan | Where-Object {
    $_ -is [string] -and $_ -like "W11_MANUAL_ACCEPTANCE_PLAN_VISUAL Resolution=*"
})
if ($VisualLines.Count -ne $ExpectedResolutions.Count) {
    throw "Expected $($ExpectedResolutions.Count) visual evidence plans, found $($VisualLines.Count)."
}
for ($Index = 0; $Index -lt $ExpectedResolutions.Count; ++$Index) {
    $Resolution = $ExpectedResolutions[$Index]
    if ($VisualLines[$Index] -notmatch "Resolution=$Resolution .*W11_$Resolution\.png$") {
        throw "Visual evidence plan mismatch at index $Index`: expected $Resolution."
    }
}
if ($PlanText -notmatch "W11_MANUAL_ACCEPTANCE_PLAN_SIGNOFF .*finalize_w11_manual_acceptance_signoff.py .*Subjective=PENDING") {
    throw "PlanOnly does not expose the structured sign-off finalizer."
}

$LauncherSource = Get-Content -LiteralPath $Launcher -Raw
foreach ($RequiredSourceContract in @(
    "ManualEvidenceRole = `$Role",
    "ManualEvidenceLog = `$RoleLog",
    "RunSeed = `$FixedSeed",
    "WaitForExit = `$true",
    'Write-W11ManualSessionManifest -ObjectiveGate "COLLECTING"',
    'RoleStatus[$Role] = "RUNNING"',
    'RoleStatus[$Role] = "COMPLETED"',
    'RoleStatus[$Role] = "INVALID"',
    'Write-W11ManualSessionManifest -ObjectiveGate "PASSED"',
    'schema = 3',
    'runtimeMode = $RuntimeMode',
    'runtime = $RuntimeKind',
    'processTarget = $ProcessTarget',
    'packageHash = $PackageHash',
    '--require-full-inventory',
    '--require-process-target',
    'outside the sealed package root',
    'RuntimeMode = $RuntimeMode',
    '$RunArguments.PackageManifest = $PackageManifest',
    '--signoff-output $SignoffPath',
    'visualEvidenceResolutions = $VisualEvidenceResolutions',
    'visualEvidenceStatus = "PENDING"')) {
    if (-not $LauncherSource.Contains($RequiredSourceContract)) {
        throw "Launcher source is missing contract: $RequiredSourceContract"
    }
}
foreach ($ForbiddenSwitch in @(
    "-W11InputScript", "-W11AutoStart", "-NullRHI", "-Unattended",
    "-NoSound", "-RenderOffscreen")) {
    if ($LauncherSource.Contains($ForbiddenSwitch)) {
        throw "Launcher source contains forbidden switch $ForbiddenSwitch."
    }
}

Write-Output (
    "W11_MANUAL_ACCEPTANCE_LAUNCHER_GATE_PASSED " +
    "Roles=5 UniqueLogs=5 SuggestedSects=$SuggestedSectCount Outcomes=Victory,Defeat " +
    "Seed=424242 ManualOnly=1 WaitForExit=1 EditorMode=1 CookedMode=1 " +
    "PackageManifestRequired=1 FullInventory=1 UserDirOutsidePackage=1 " +
    "ProcessTarget=RuntimeExecutable WaitForRealGame=1 " +
    "Resolutions=4 StructuredSignoff=1 Subjective=PENDING"
)
