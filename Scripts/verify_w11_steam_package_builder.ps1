[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$Builder = Join-Path $PSScriptRoot "BuildW11SteamAcceptancePackage.ps1"
if (-not (Test-Path -LiteralPath $Builder)) {
    throw "W11 Steam package builder not found: $Builder"
}
$Tokens = $null
$Errors = $null
[System.Management.Automation.Language.Parser]::ParseFile(
    (Resolve-Path -LiteralPath $Builder), [ref]$Tokens, [ref]$Errors) | Out-Null
if ($Errors.Count -gt 0) {
    throw "PowerShell syntax errors in $Builder`: $($Errors.Message -join '; ')"
}

$PlanDirectory = Join-Path ([System.IO.Path]::GetTempPath()) (
    "W11SteamPackagePlan_" + [guid]::NewGuid().ToString("N"))
$Plan = @(& $Builder -OutputDirectory $PlanDirectory -PlanOnly)
$PlanLine = @($Plan | Where-Object {
    $_ -is [string] -and $_ -like "W11_STEAM_PACKAGE_PLAN *"
})
if ($PlanLine.Count -ne 1) {
    throw "Expected exactly one W11 Steam package plan line."
}
foreach ($Fact in @(
    "Configuration=Development",
    "Platform=Win64",
    "Build=1",
    "Cook=1",
    "Stage=1",
    "Pak=1",
    "IoStore=1",
    "Archive=1",
    "Map=/Game/WorldWalker/Worlds/W11_RogueSurvival/Maps/L_W11_RogueSurvival",
    "Runtime=CookedGame",
    "Integrity=FullInventory",
    "ProcessTarget=RuntimeExecutable",
    "Overwrite=0")) {
    if ($PlanLine[0] -notlike "*$Fact*") {
        throw "W11 Steam package plan is missing $Fact."
    }
}
if (Test-Path -LiteralPath $PlanDirectory) {
    throw "W11 Steam package PlanOnly unexpectedly created its output directory."
}

$Source = Get-Content -LiteralPath $Builder -Raw
foreach ($Required in @(
    '"-target=WorldWalkerPrototype"',
    '"-platform=Win64"',
    '"-build"',
    '"-cook"',
    '"-stage"',
    '"-pak"',
    '"-iostore"',
    '"-archive"',
    'acceptanceSteamAppIdEmbedded = $false',
    'packagedDefaultPlatformService = $PlatformServiceMatch.Groups[1].Value',
    'packagedDefaultSteamDevAppId = $SteamDevAppIdMatch.Groups[1].Value',
    '$LauncherPath = Join-Path $OutputDirectory "Windows\WorldWalkerPrototype.exe"',
    '$RuntimeExecutable = $RuntimeExecutables[0]',
    'schema = 4',
    'fullInventorySealed = $true',
    'processTargetSealed = $true',
    'bootstrapExecutable = $BootstrapExecutableRelative',
    'runtimeExecutable = $RuntimeExecutableRelative',
    'artifactCount = $Artifacts.Count',
    'Get-ChildItem -LiteralPath $OutputDirectory -Recurse -File',
    '--require-full-inventory',
    '--require-process-target',
    'Get-FileHash')) {
    if (-not $Source.Contains($Required)) {
        throw "W11 Steam package builder is missing contract: $Required"
    }
}
foreach ($Forbidden in @(
    "Remove-Item",
    "SteamDevAppId=480",
    "steam_appid.txt",
    "-clean")) {
    if ($Source.Contains($Forbidden)) {
        throw "W11 Steam package builder contains forbidden behavior: $Forbidden"
    }
}

Write-Output (
    "W11_STEAM_PACKAGE_BUILDER_GATE_PASSED " +
    "BuildCookStage=1 PakIoStore=1 W11Map=1 Overwrite=0 AcceptanceAppIdEmbedded=0 DefaultProvenance=1 ManifestHashes=1 FullInventory=1 ProcessTarget=RuntimeExecutable"
)
