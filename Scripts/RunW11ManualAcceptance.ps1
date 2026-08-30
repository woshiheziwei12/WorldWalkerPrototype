[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$Operator,
    [string]$EngineRoot = "",
    [string]$SessionDirectory = "",
    [ValidateSet("Development", "DebugGame")]
    [string]$Configuration = "Development",
    [ValidateSet("Editor", "Cooked")]
    [string]$RuntimeMode = "Editor",
    [string]$PackageManifest = "",
    [int]$ResX = 1280,
    [int]$ResY = 720,
    [switch]$Fullscreen,
    [switch]$SkipBuild,
    [switch]$Resume,
    [switch]$PlanOnly,
    [string]$PythonCommand = "python"
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$RunW11 = Join-Path $PSScriptRoot "RunW11.ps1"
$Verifier = Join-Path $PSScriptRoot "verify_w11_manual_acceptance_logs.py"
$PacketBuilder = Join-Path $PSScriptRoot "build_w11_manual_acceptance_packet.py"
$SignoffFinalizer = Join-Path $PSScriptRoot "finalize_w11_manual_acceptance_signoff.py"
$PackageVerifier = Join-Path $PSScriptRoot "verify_w11_steam_package_manifest.py"
$Roles = @("Observe", "Evasion", "Dodge", "Recovery", "Feedback")
$FixedSeed = 424242
$RoleGuidance = [ordered]@{
    Observe = [ordered]@{
        SuggestedSect = "Sect.ThunderManor"
        TargetOutcome = "Defeat"
        Note = "只观察三敌出招，不攻击、不闪避；完成进度后允许自然战败。"
    }
    Evasion = [ordered]@{
        SuggestedSect = "Sect.ShadowMoonTower"
        TargetOutcome = "Defeat"
        Note = "只移动避开瘴气弹，不攻击、不按闪避；完成进度后允许自然战败。"
    }
    Dodge = [ordered]@{
        SuggestedSect = "Sect.CanglanPalace"
        TargetOutcome = "Defeat"
        Note = "用空格穿过残剑魂冲斩并取得免疫；完成后可自然战败。"
    }
    Recovery = [ordered]@{
        SuggestedSect = "Sect.DanxiaValley"
        TargetOutcome = "Victory"
        Note = "诱导石甲重砸后摇并造成正生命伤害，随后正常取胜。"
    }
    Feedback = [ordered]@{
        SuggestedSect = "Sect.ThunderManor"
        TargetOutcome = "Victory"
        Note = "持续推进构筑，覆盖普攻、多目标术法、治疗、护障、会心、吸收和击败后取胜。"
    }
}

function Test-W11CompletedManualRoleLog {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [string]$Role
    )
    if (-not (Test-Path -LiteralPath $Path)) {
        return $false
    }
    $Text = Get-Content -LiteralPath $Path -Raw
    if ([string]::IsNullOrEmpty($Text)) {
        return $false
    }
    return $Text.Contains("W11_MANUAL_EVIDENCE_SESSION Role=$Role ") `
        -and $Text.Contains("InputScript=None Interactive=1 Rendered=1 Audio=1 AutoStart=0 NetMode=0 TestOverride=0") `
        -and $Text.Contains("W11_COMBAT_SUMMARY ") `
        -and -not $Text.Contains("W11_MANUAL_EVIDENCE_SESSION_REJECTED")
}

foreach ($RequiredPath in @($RunW11, $Verifier, $PacketBuilder, $SignoffFinalizer)) {
    if (-not (Test-Path -LiteralPath $RequiredPath)) {
        throw "Required W11 manual-acceptance file not found: $RequiredPath"
    }
}
if ($RuntimeMode -eq "Cooked") {
    if ([string]::IsNullOrWhiteSpace($PackageManifest)) {
        throw "-RuntimeMode Cooked requires -PackageManifest."
    }
    $PackageManifest = [System.IO.Path]::GetFullPath($PackageManifest)
    foreach ($RequiredPath in @($PackageManifest, $PackageVerifier)) {
        if (-not (Test-Path -LiteralPath $RequiredPath -PathType Leaf)) {
            throw "Required cooked manual-acceptance file not found: $RequiredPath"
        }
    }
    & $PythonCommand $PackageVerifier $PackageManifest --require-full-inventory --require-process-target
    if ($LASTEXITCODE -ne 0) {
        throw "Cooked manual-acceptance package manifest failed with exit code $LASTEXITCODE."
    }
    $RuntimeKind = "CookedGame"
    $PackageHash = (Get-FileHash -LiteralPath $PackageManifest -Algorithm SHA256).Hash.ToLowerInvariant()
    $PackageRoot = Split-Path -Parent $PackageManifest
}
else {
    if (-not [string]::IsNullOrWhiteSpace($PackageManifest)) {
        throw "-PackageManifest is valid only with -RuntimeMode Cooked."
    }
    $RuntimeKind = "EditorStandalone"
    $PackageHash = "EDITOR"
}
$ProcessTarget = if ($RuntimeMode -eq "Cooked") { "RuntimeExecutable" } else { "Editor" }
if ($ResX -lt 640 -or $ResY -lt 360) {
    throw "Manual evidence resolution must be at least 640x360."
}
if ($Resume -and [string]::IsNullOrWhiteSpace($SessionDirectory)) {
    throw "-Resume requires an explicit -SessionDirectory."
}

if ([string]::IsNullOrWhiteSpace($SessionDirectory)) {
    $Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $SessionDirectory = Join-Path $ProjectRoot "Saved\ManualAcceptance\Session_$Timestamp"
}
else {
    $SessionDirectory = [System.IO.Path]::GetFullPath($SessionDirectory)
}
if ($RuntimeMode -eq "Cooked") {
    $PackageRootPrefix = $PackageRoot.TrimEnd("\", "/") + [System.IO.Path]::DirectorySeparatorChar
    if ($SessionDirectory.Equals($PackageRoot, [System.StringComparison]::OrdinalIgnoreCase) `
        -or $SessionDirectory.StartsWith(
            $PackageRootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Cooked manual-acceptance evidence must stay outside the sealed package root."
    }
}

$Logs = [ordered]@{}
foreach ($Role in $Roles) {
    $Logs[$Role] = Join-Path $SessionDirectory "W11Manual$Role.log"
}
$PacketPath = Join-Path $SessionDirectory "W11人工单人验收记录.md"
$SignoffPath = Join-Path $SessionDirectory "W11人工单人验收签字.json"
$SignoffEvidencePath = Join-Path $SessionDirectory "W11人工单人验收签字证据.json"
$ManifestPath = Join-Path $SessionDirectory "session.json"
$VisualEvidenceResolutions = @("3840x2160", "2560x1440", "1920x1080", "1280x720")
$RoleStatus = [ordered]@{}
foreach ($Role in $Roles) {
    $RoleStatus[$Role] = "PENDING"
}

function Write-W11ManualSessionManifest {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ObjectiveGate,
        [Parameter(Mandatory = $true)]
        [string]$SubjectiveStatus
    )
    $Manifest = [ordered]@{
        schema = 3
        seed = $FixedSeed
        operator = $Operator
        runtimeMode = $RuntimeMode
        runtime = $RuntimeKind
        processTarget = $ProcessTarget
        packageManifest = $PackageManifest
        packageHash = $PackageHash
        resolution = "${ResX}x${ResY}"
        roles = $Roles
        roleStatus = $RoleStatus
        logs = $Logs
        objectiveGate = $ObjectiveGate
        subjectiveStatus = $SubjectiveStatus
        packet = $PacketPath
        packetCreated = Test-Path -LiteralPath $PacketPath
        signoff = $SignoffPath
        signoffCreated = Test-Path -LiteralPath $SignoffPath
        signoffEvidence = $SignoffEvidencePath
        visualEvidenceResolutions = $VisualEvidenceResolutions
        visualEvidenceStatus = "PENDING"
        updatedAt = (Get-Date).ToString("o")
    }
    $Manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $ManifestPath -Encoding UTF8
}

Write-Host "[W11 Manual] Operator: $Operator" -ForegroundColor Cyan
Write-Host "[W11 Manual] Fixed seed: $FixedSeed" -ForegroundColor Cyan
Write-Host "[W11 Manual] Runtime: $RuntimeKind" -ForegroundColor Cyan
Write-Host "[W11 Manual] Package identity: $PackageHash" -ForegroundColor Cyan
Write-Host "[W11 Manual] Session: $SessionDirectory" -ForegroundColor Cyan

if ($PlanOnly) {
    foreach ($Role in $Roles) {
        Write-Output (
            "W11_MANUAL_ACCEPTANCE_PLAN Role=$Role Seed=$FixedSeed " +
            "Runtime=$RuntimeKind PackageHash=$PackageHash " +
            "Interactive=1 Rendered=1 Audio=1 InputScript=None AutoStart=0 TestOverride=0 " +
            "ProcessTarget=$ProcessTarget WaitsForGameExit=1 " +
            "SuggestedSect=$($RoleGuidance[$Role].SuggestedSect) " +
            "TargetOutcome=$($RoleGuidance[$Role].TargetOutcome) " +
            "Log=$($Logs[$Role])"
        )
    }
    Write-Output "W11_MANUAL_ACCEPTANCE_PLAN_PACKET Output=$PacketPath Subjective=PENDING"
    foreach ($Resolution in $VisualEvidenceResolutions) {
        Write-Output (
            "W11_MANUAL_ACCEPTANCE_PLAN_VISUAL Resolution=$Resolution " +
            "Artifact=$(Join-Path $SessionDirectory "VisualEvidence\W11_$Resolution.png")"
        )
    }
    Write-Output (
        "W11_MANUAL_ACCEPTANCE_PLAN_SIGNOFF Template=$SignoffPath " +
        "Finalizer=$SignoffFinalizer Subjective=PENDING"
    )
    return
}

New-Item -ItemType Directory -Path $SessionDirectory -Force | Out-Null
foreach ($Role in $Roles) {
    if (Test-W11CompletedManualRoleLog -Path $Logs[$Role] -Role $Role) {
        $RoleStatus[$Role] = "COMPLETED"
    }
}
Write-W11ManualSessionManifest -ObjectiveGate "COLLECTING" -SubjectiveStatus "PENDING"

if ($RuntimeMode -eq "Editor" -and -not $SkipBuild) {
    Write-Host "[W11 Manual] Building once before the five interactive roles..." -ForegroundColor Cyan
    & $RunW11 `
        -EngineRoot $EngineRoot `
        -Configuration $Configuration `
        -BuildOnly
}

foreach ($Role in $Roles) {
    $RoleLog = $Logs[$Role]
    if (Test-Path -LiteralPath $RoleLog) {
        if ($Resume -and (Test-W11CompletedManualRoleLog -Path $RoleLog -Role $Role)) {
            $RoleStatus[$Role] = "COMPLETED"
            Write-W11ManualSessionManifest -ObjectiveGate "COLLECTING" -SubjectiveStatus "PENDING"
            Write-Host "[W11 Manual] Resuming: keeping completed $Role log." -ForegroundColor Yellow
            continue
        }
        if (-not $Resume) {
            throw "Evidence log already exists and will not be overwritten: $RoleLog"
        }
        $ArchivePath = "$RoleLog.incomplete_$([guid]::NewGuid().ToString('N')).log"
        Move-Item -LiteralPath $RoleLog -Destination $ArchivePath
        $RoleStatus[$Role] = "PENDING"
        Write-W11ManualSessionManifest -ObjectiveGate "COLLECTING" -SubjectiveStatus "PENDING"
        Write-Host "[W11 Manual] Preserved incomplete/rejected log as $ArchivePath" -ForegroundColor Yellow
    }

    $Guidance = $RoleGuidance[$Role]
    Write-Host "[W11 Manual] Starting role $Role. Follow the in-game objective panel, then exit normally." -ForegroundColor Cyan
    Write-Host (
        "[W11 Manual] Coverage suggestion: Sect=$($Guidance.SuggestedSect) " +
        "TargetOutcome=$($Guidance.TargetOutcome) $($Guidance.Note)"
    ) -ForegroundColor Cyan
    $RoleStatus[$Role] = "RUNNING"
    Write-W11ManualSessionManifest -ObjectiveGate "COLLECTING" -SubjectiveStatus "PENDING"
    $RunArguments = @{
        EngineRoot = $EngineRoot
        Configuration = $Configuration
        RuntimeMode = $RuntimeMode
        PythonCommand = $PythonCommand
        SkipBuild = $true
        ResX = $ResX
        ResY = $ResY
        ManualEvidenceRole = $Role
        ManualEvidenceLog = $RoleLog
        RunSeed = $FixedSeed
        WaitForExit = $true
    }
    if ($RuntimeMode -eq "Cooked") {
        $RunArguments.PackageManifest = $PackageManifest
    }
    if ($Fullscreen) {
        $RunArguments.Fullscreen = $true
    }
    & $RunW11 @RunArguments
    if (-not (Test-Path -LiteralPath $RoleLog)) {
        throw "Interactive W11 role $Role did not create its evidence log: $RoleLog"
    }
    if (-not (Test-W11CompletedManualRoleLog -Path $RoleLog -Role $Role)) {
        $RoleStatus[$Role] = "INVALID"
        Write-W11ManualSessionManifest -ObjectiveGate "COLLECTING" -SubjectiveStatus "PENDING"
        throw (
            "Interactive W11 role $Role did not produce an accepted terminal manual log. " +
            "Re-run this session with -Resume; the invalid log will be preserved before retry."
        )
    }
    $RoleStatus[$Role] = "COMPLETED"
    Write-W11ManualSessionManifest -ObjectiveGate "COLLECTING" -SubjectiveStatus "PENDING"
}

$EvidenceArguments = @(
    "--observe", $Logs["Observe"],
    "--evasion", $Logs["Evasion"],
    "--dodge", $Logs["Dodge"],
    "--recovery", $Logs["Recovery"],
    "--extra", $Logs["Feedback"]
)

Write-Host "[W11 Manual] Verifying objective evidence across all five roles..." -ForegroundColor Cyan
& $PythonCommand $Verifier @EvidenceArguments
if ($LASTEXITCODE -ne 0) {
    throw "W11 manual objective evidence gate failed with exit code $LASTEXITCODE."
}

Write-Host "[W11 Manual] Creating the human sign-off packet..." -ForegroundColor Cyan
& $PythonCommand $PacketBuilder @EvidenceArguments `
    --operator $Operator `
    --output $PacketPath `
    --signoff-output $SignoffPath
if ($LASTEXITCODE -ne 0) {
    throw "W11 manual acceptance packet generation failed with exit code $LASTEXITCODE."
}
if (-not (Test-Path -LiteralPath $SignoffPath)) {
    throw "W11 manual acceptance packet builder did not create the structured sign-off template."
}

Write-W11ManualSessionManifest -ObjectiveGate "PASSED" -SubjectiveStatus "PENDING"

Write-Host "[W11 Manual] Objective gate passed. Human subjective fields remain PENDING." -ForegroundColor Green
Write-Host (
    "[W11 Manual] Capture the four exact-resolution PNGs, fill $SignoffPath, then run: " +
    "$PythonCommand `"$SignoffFinalizer`" --session `"$ManifestPath`""
) -ForegroundColor Yellow
Write-Output (
    "W11_MANUAL_ACCEPTANCE_SESSION_READY Packet=$PacketPath Signoff=$SignoffPath " +
    "Manifest=$ManifestPath VisualEvidence=PENDING Subjective=PENDING"
)
