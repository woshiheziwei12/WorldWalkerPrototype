[CmdletBinding()]
param(
    [string]$EngineRoot = "",
    [ValidateSet("Development", "DebugGame")]
    [string]$Configuration = "Development",
    [string]$PythonCommand = "python",
    [string]$OutputDirectory = "",
    [int]$RunSeed = 424242,
    [int]$TimeoutSecondsPerHero = 180,
    [switch]$SkipBuild,
    [switch]$PlanOnly
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot "WorldWalkerPrototype.uproject"
$W11Map = "/Game/WorldWalker/Worlds/W11_RogueSurvival/Maps/L_W11_RogueSurvival"
$Verifier = Join-Path $PSScriptRoot "verify_w11_all_heroes_logs.py"

if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
    $EngineRoot = if ([string]::IsNullOrWhiteSpace($env:UE_ROOT)) { "E:\app\ue\UE_5.8" } else { $env:UE_ROOT }
}
$BuildScript = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"
$EditorExecutable = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
foreach ($RequiredPath in @($ProjectFile, $BuildScript, $EditorExecutable, $Verifier)) {
    if (-not (Test-Path -LiteralPath $RequiredPath -PathType Leaf)) {
        throw "Required W11 all-hero file not found: $RequiredPath"
    }
}
if ($TimeoutSecondsPerHero -lt 30) {
    throw "-TimeoutSecondsPerHero must be at least 30 seconds."
}

$Matrix = @(
    [pscustomobject]@{ Hero = "Hero.Wei";   Sect = "Sect.CanglanPalace" },
    [pscustomobject]@{ Hero = "Hero.Dong";  Sect = "Sect.DanxiaValley" },
    [pscustomobject]@{ Hero = "Hero.Tian";  Sect = "Sect.ThunderManor" },
    [pscustomobject]@{ Hero = "Hero.Xiang"; Sect = "Sect.AzureCloudSword" },
    [pscustomobject]@{ Hero = "Hero.Pu";    Sect = "Sect.TianjiFormation" },
    [pscustomobject]@{ Hero = "Hero.Ying";  Sect = "Sect.DanxiaValley" },
    [pscustomobject]@{ Hero = "Hero.Li";    Sect = "Sect.TianjiFormation" },
    [pscustomobject]@{ Hero = "Hero.Jia";   Sect = "Sect.ThunderManor" },
    [pscustomobject]@{ Hero = "Hero.Yu";    Sect = "Sect.ShadowMoonTower" },
    [pscustomobject]@{ Hero = "Hero.Meng";  Sect = "Sect.AzureCloudSword" }
)

if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $OutputDirectory = Join-Path $ProjectRoot "Saved\AllHeroes\$Timestamp"
}
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $OutputDirectory) {
    throw "All-hero output directory already exists; refusing to overwrite: $OutputDirectory"
}

Write-Host "[W11] Ten-hero matrix (automated smoke, not manual acceptance):" -ForegroundColor Cyan
foreach ($Entry in $Matrix) {
    Write-Host "  $($Entry.Hero) + $($Entry.Sect)"
}
if ($PlanOnly) {
    Write-Output "W11_ALL_HEROES_PLAN Heroes=10 Sects=6 Seed=$RunSeed InputScript=PacingCombatLoop AutoQuitOnOutcome=1 Output=$OutputDirectory"
    return
}

if (-not $SkipBuild) {
    Write-Host "[W11] Building WorldWalkerPrototypeEditor Win64 $Configuration once..." -ForegroundColor Cyan
    & $BuildScript WorldWalkerPrototypeEditor Win64 $Configuration $ProjectFile -WaitMutex -NoHotReloadFromIDE
    if ($LASTEXITCODE -ne 0) {
        throw "W11 all-hero build failed with exit code $LASTEXITCODE."
    }
}
else {
    Write-Host "[W11] Build skipped by request." -ForegroundColor Yellow
}

New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
$RunStatePath = Join-Path $OutputDirectory "run_state.json"
$RunState = [ordered]@{
    schemaVersion = 1
    evidenceType = "W11AllHeroesAutomatedCombatSmokeRun"
    status = "RUNNING"
    manualAcceptance = $false
    runSeed = $RunSeed
    inputScript = "PacingCombatLoop"
    entries = @()
}
$RunState | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $RunStatePath -Encoding utf8

$ProcessFailures = @()
foreach ($Entry in $Matrix) {
    $ShortHero = $Entry.Hero.Substring("Hero.".Length)
    $LogPath = Join-Path $OutputDirectory "$ShortHero.log"
    $Arguments = @(
        "`"$ProjectFile`"",
        $W11Map,
        "-game",
        "-NullRHI",
        "-Unattended",
        "-NoSound",
        "-NoSplash",
        "-DDC-ForceMemoryCache",
        "-BENCHMARK",
        "-FPS=60",
        "-log",
        "-W11AutoStart",
        "-W11Hero=$($Entry.Hero)",
        "-W11Sect=$($Entry.Sect)",
        "-W11RunSeed=$RunSeed",
        "-W11InputScript=PacingCombatLoop",
        "-W11AutoQuitOnOutcome",
        "-AbsLog=`"$LogPath`""
    )
    Write-Host "[W11] Running $($Entry.Hero) + $($Entry.Sect)..." -ForegroundColor Cyan
    $StartedAt = Get-Date
    $Process = Start-Process -FilePath $EditorExecutable -ArgumentList $Arguments -WorkingDirectory $ProjectRoot -PassThru
    $Exited = $Process.WaitForExit($TimeoutSecondsPerHero * 1000)
    if (-not $Exited) {
        Stop-Process -Id $Process.Id -Force
        $Process.WaitForExit()
        $ProcessFailures += "$($Entry.Hero): timeout after $TimeoutSecondsPerHero seconds"
        $ExitCode = -1
    }
    else {
        $ExitCode = $Process.ExitCode
        if ($ExitCode -ne 0) {
            $ProcessFailures += "$($Entry.Hero): exit code $ExitCode"
        }
    }
    $RunState.entries += [ordered]@{
        hero = $Entry.Hero
        sect = $Entry.Sect
        log = [System.IO.Path]::GetFileName($LogPath)
        processExitCode = $ExitCode
        wallSeconds = [Math]::Round(((Get-Date) - $StartedAt).TotalSeconds, 3)
    }
    $RunState | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $RunStatePath -Encoding utf8
}

if ($ProcessFailures.Count -gt 0) {
    $RunState.status = "PROCESS_FAILED"
    $RunState.failures = $ProcessFailures
    $RunState | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $RunStatePath -Encoding utf8
    throw "One or more hero processes failed: $($ProcessFailures -join '; ')"
}

$EvidencePath = Join-Path $OutputDirectory "W11AllHeroesEvidence.json"
& $PythonCommand $Verifier $OutputDirectory --output $EvidencePath --seed $RunSeed
if ($LASTEXITCODE -ne 0) {
    $RunState.status = "EVIDENCE_FAILED"
    $RunState | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $RunStatePath -Encoding utf8
    throw "W11 all-hero evidence verification failed with exit code $LASTEXITCODE."
}
$RunState.status = "PASSED"
$RunState.evidence = [System.IO.Path]::GetFileName($EvidencePath)
$RunState | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $RunStatePath -Encoding utf8
Write-Host "[W11] Ten-hero automated combat matrix passed: $EvidencePath" -ForegroundColor Green
