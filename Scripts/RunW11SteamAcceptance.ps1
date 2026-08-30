[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("Host", "Client")]
    [string]$Role,
    [Parameter(Mandatory = $true)]
    [string]$AppId,
    [Parameter(Mandatory = $true)]
    [string]$EvidenceToken,
    [ValidateSet("Integration", "Release")]
    [string]$Mode = "Integration",
    [ValidateSet("BasicJoin", "FriendInvite", "HostExit", "ClientDisconnect", "PlayerSync")]
    [string]$Scenario = "BasicJoin",
    [ValidateRange(1, 4)]
    [int]$ExpectedPlayers = 2,
    [string]$PackagedExecutable = "",
    [string]$PackageManifest = "",
    [string]$EngineRoot = "",
    [string]$SessionDirectory = "",
    [ValidateSet("Development", "DebugGame")]
    [string]$Configuration = "Development",
    [int]$ResX = 1280,
    [int]$ResY = 720,
    [switch]$Fullscreen,
    [switch]$SkipBuild,
    [switch]$PlanOnly,
    [string]$PythonCommand = "python"
)

$ErrorActionPreference = "Stop"

if ($AppId -notmatch "^[1-9][0-9]+$" -or $AppId -eq "480") {
    throw "-AppId must be a non-placeholder numeric project Steam App ID; 480 is not accepted."
}
if ($EvidenceToken -notmatch "^[A-Za-z0-9_-]{8,64}$") {
    throw "-EvidenceToken must match [A-Za-z0-9_-]{8,64}."
}
if ($ResX -lt 640 -or $ResY -lt 360) {
    throw "Steam acceptance resolution must be at least 640x360."
}
if ($Scenario -ne "PlayerSync" -and $ExpectedPlayers -ne 2) {
    throw "-ExpectedPlayers can differ from 2 only with -Scenario PlayerSync."
}
if ($Scenario -eq "PlayerSync" -and $ExpectedPlayers -lt 2) {
    throw "PlayerSync evidence requires at least 2 players; solo is covered by the combat acceptance flow."
}
if ($Mode -eq "Integration" -and $Scenario -ne "BasicJoin") {
    throw "Non-BasicJoin Steam scenarios require -Mode Release and a cooked package."
}

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot "WorldWalkerPrototype.uproject"
$W11Map = "/Game/WorldWalker/Worlds/W11_RogueSurvival/Maps/L_W11_RogueSurvival"
$Verifier = Join-Path $PSScriptRoot "verify_w11_steam_acceptance_logs.py"
$PackageVerifier = Join-Path $PSScriptRoot "verify_w11_steam_package_manifest.py"
$KitVerifier = Join-Path $PSScriptRoot "verify_w11_steam_acceptance_kit.py"
$PortableKitManifest = Join-Path $ProjectRoot "W11SteamAcceptanceKitManifest.json"
$PortableKitVerification = "NotPortable"
$PortableLaunchContract = "ExternalPackage"

if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
    if (-not [string]::IsNullOrWhiteSpace($env:UE_ROOT)) {
        $EngineRoot = $env:UE_ROOT
    }
    else {
        $EngineRoot = "E:\app\ue\UE_5.8"
    }
}

$BuildScript = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"
$EditorExecutable = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
foreach ($RequiredPath in @($Verifier, $PackageVerifier, $KitVerifier)) {
    if (-not (Test-Path -LiteralPath $RequiredPath)) {
        throw "Required W11 Steam acceptance file not found: $RequiredPath"
    }
}
if ($Mode -eq "Release") {
    if ([string]::IsNullOrWhiteSpace($PackageManifest)) {
        throw "-Mode Release requires -PackageManifest from a verified cooked package."
    }
    $PackageManifest = [System.IO.Path]::GetFullPath($PackageManifest)
    if (-not (Test-Path -LiteralPath $PackageManifest -PathType Leaf)) {
        throw "W11 package manifest not found: $PackageManifest"
    }
    & $PythonCommand $PackageVerifier $PackageManifest --require-full-inventory --require-process-target
    if ($LASTEXITCODE -ne 0) {
        throw "W11 package manifest gate failed with exit code $LASTEXITCODE."
    }
    if (Test-Path -LiteralPath $PortableKitManifest -PathType Leaf) {
        & $PythonCommand -B $KitVerifier $PortableKitManifest
        if ($LASTEXITCODE -ne 0) {
            throw "Portable W11 acceptance kit pre-launch gate failed with exit code $LASTEXITCODE."
        }
        $PortableKitVerification = "FullInventory"
        $PortableLaunchContract = "RunnerOnlyUserDirOutsidePackage"
    }
    $PackageHash = (Get-FileHash -LiteralPath $PackageManifest -Algorithm SHA256).Hash.ToLowerInvariant()
    $PackageRoot = Split-Path -Parent $PackageManifest
    $PackageData = Get-Content -LiteralPath $PackageManifest -Raw | ConvertFrom-Json
    $RuntimeExecutableRelative = [string]$PackageData.runtimeExecutable
    $Executable = Join-Path $PackageRoot $RuntimeExecutableRelative.Replace("/", "\")
    if (-not (Test-Path -LiteralPath $Executable -PathType Leaf)) {
        throw "Packaged W11 executable not found: $Executable"
    }
    if (-not [string]::IsNullOrWhiteSpace($PackagedExecutable)) {
        $RequestedExecutable = [System.IO.Path]::GetFullPath($PackagedExecutable)
        if ($RequestedExecutable -ne $Executable) {
            throw "-PackagedExecutable does not match the runtime process target sealed by -PackageManifest."
        }
    }
    $RuntimeKind = "CookedGame"
    $LaunchWorkingDirectory = Split-Path -Parent $Executable
}
else {
    if (-not [string]::IsNullOrWhiteSpace($PackageManifest) `
        -or -not [string]::IsNullOrWhiteSpace($PackagedExecutable)) {
        throw "PackageManifest/PackagedExecutable are valid only with -Mode Release."
    }
    foreach ($RequiredPath in @($ProjectFile, $BuildScript, $EditorExecutable)) {
        if (-not (Test-Path -LiteralPath $RequiredPath)) {
            throw "Required W11 Steam integration file not found: $RequiredPath"
        }
    }
    $Executable = $EditorExecutable
    $RuntimeKind = "EditorStandalone"
    $PackageHash = "EDITOR"
    $LaunchWorkingDirectory = $ProjectRoot
}

if ([string]::IsNullOrWhiteSpace($SessionDirectory)) {
    $SessionDirectory = Join-Path $ProjectRoot "Saved\SteamAcceptance\$EvidenceToken"
}
else {
    $SessionDirectory = [System.IO.Path]::GetFullPath($SessionDirectory)
}
$UserDataDirectory = [System.IO.Path]::GetFullPath(
    (Join-Path $SessionDirectory "UserData\$Role"))
if ($Mode -eq "Release") {
    $PackageRootPrefix = $PackageRoot.TrimEnd("\", "/") + [System.IO.Path]::DirectorySeparatorChar
    if ($UserDataDirectory.Equals($PackageRoot, [System.StringComparison]::OrdinalIgnoreCase) `
        -or $UserDataDirectory.StartsWith(
            $PackageRootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Steam acceptance user data must stay outside the sealed package root."
    }
}
$LogPath = Join-Path $SessionDirectory "W11Steam${Scenario}${Role}.log"
$ReceiptPath = Join-Path $SessionDirectory "W11Steam${Scenario}${Role}Receipt.json"
$WindowMode = if ($Fullscreen) { "-Fullscreen" } else { "-Windowed" }
$LaunchArguments = @()
if ($Mode -eq "Integration") {
    $LaunchArguments += "`"$ProjectFile`""
}
$LaunchArguments += $W11Map
if ($Mode -eq "Integration") {
    $LaunchArguments += "-game"
}
$LaunchArguments += @(
    $WindowMode,
    "-ResX=$ResX",
    "-ResY=$ResY",
    "-log",
    "-NoSplash",
    "-W11RunSeed=424242",
    "-W11SteamEvidenceRole=$Role",
    "-W11SteamEvidenceScenario=$Scenario",
    "-W11SteamEvidenceToken=$EvidenceToken",
    "-W11SteamPackageHash=$PackageHash",
    "-ini:Engine:[OnlineSubsystem]:DefaultPlatformService=Steam",
    "-ini:Engine:[OnlineSubsystemSteam]:bEnabled=true",
    "-ini:Engine:[OnlineSubsystemSteam]:SteamDevAppId=$AppId",
    "-UserDir=`"$UserDataDirectory`"",
    "-AbsLog=`"$LogPath`""
)

Write-Host "[W11 Steam] Role: $Role" -ForegroundColor Cyan
Write-Host "[W11 Steam] App ID: $AppId" -ForegroundColor Cyan
Write-Host "[W11 Steam] Evidence token: $EvidenceToken" -ForegroundColor Cyan
Write-Host "[W11 Steam] Scenario: $Scenario (expected players: $ExpectedPlayers)" -ForegroundColor Cyan
Write-Host "[W11 Steam] Mode/runtime: $Mode/$RuntimeKind" -ForegroundColor Cyan
Write-Host "[W11 Steam] Log: $LogPath" -ForegroundColor Cyan
switch ($Scenario) {
    "FriendInvite" {
        if ($Role -eq "Host") {
            Write-Host "[W11 Steam] Create the room, then use the Steam overlay to invite the Client account; keep the room open." -ForegroundColor Yellow
        }
        else {
            Write-Host "[W11 Steam] Accept the Steam friend invitation. Do not use the room browser; InviteAccepted must drive the join." -ForegroundColor Yellow
        }
    }
    "HostExit" {
        if ($Role -eq "Host") {
            Write-Host "[W11 Steam] Create the room, wait for the Client to join, then exit the host game." -ForegroundColor Yellow
        }
        else {
            Write-Host "[W11 Steam] Join through the room browser and keep the game open until it records the host-loss NetworkFailure." -ForegroundColor Yellow
        }
    }
    "ClientDisconnect" {
        if ($Role -eq "Host") {
            Write-Host "[W11 Steam] Create the room; after the Client joins, keep running until ParticipantLeft is recorded." -ForegroundColor Yellow
        }
        else {
            Write-Host "[W11 Steam] Join through the room browser, then close this client when both players are in combat." -ForegroundColor Yellow
        }
    }
    "PlayerSync" {
        if ($Role -eq "Host") {
            Write-Host "[W11 Steam] Create the room and wait until the host log reaches Players=$ExpectedPlayers before exiting." -ForegroundColor Yellow
        }
        else {
            Write-Host "[W11 Steam] Join through the room browser; gather $($ExpectedPlayers - 1) distinct Client logs for the final group gate." -ForegroundColor Yellow
        }
    }
    default {
        if ($Role -eq "Host") {
            Write-Host "[W11 Steam] Open Online Co-op and create a room; keep it open until the Client joins." -ForegroundColor Yellow
        }
        else {
            Write-Host "[W11 Steam] Open Online Co-op, refresh rooms, and join the token-filtered result." -ForegroundColor Yellow
        }
    }
}

if ($PlanOnly) {
    $PackageIntegrity = if ($Mode -eq "Release") { "FullInventory" } else { "Editor" }
    Write-Output (
        "W11_STEAM_ACCEPTANCE_PLAN Role=$Role Scenario=$Scenario ExpectedPlayers=$ExpectedPlayers AppId=$AppId Token=$EvidenceToken " +
        "PackageHash=$PackageHash PackageIntegrity=$PackageIntegrity OSS=Steam Placeholder=0 " +
        "Seed=424242 Interactive=1 Runtime=$RuntimeKind UserDirOutsidePackage=1 " +
        "ProcessTarget=$(if ($Mode -eq 'Release') { 'RuntimeExecutable' } else { 'Editor' }) " +
        "PostExitPackageVerification=$(if ($Mode -eq 'Release') { 'FullInventory' } else { 'EditorLogOnly' }) " +
        "DurableRoleReceipt=$(if ($Mode -eq 'Release') { '1' } else { '0' }) " +
        "ParticipantIdentity=$(if ($Mode -eq 'Release') { 'Steam64Required' } else { 'IntegrationOnly' }) " +
        "PreLaunchKitVerification=$PortableKitVerification " +
        "PostExitKitVerification=$(if ($PortableKitVerification -eq 'FullInventory') { 'FullInventory' } else { $PortableKitVerification }) " +
        "ReceiptKitIdentity=$(if ($PortableKitVerification -eq 'FullInventory') { 'SealedAfterExit' } else { $PortableKitVerification }) " +
        "LaunchContract=$PortableLaunchContract " +
        "WaitsForGameExit=1 Executable=$Executable Log=$LogPath Receipt=$ReceiptPath"
    )
    return
}

if (Test-Path -LiteralPath $LogPath) {
    throw "Steam evidence log already exists and will not be overwritten: $LogPath"
}
if ($Mode -eq "Release" -and (Test-Path -LiteralPath $ReceiptPath)) {
    throw "Steam role receipt already exists and will not be overwritten: $ReceiptPath"
}
New-Item -ItemType Directory -Path $SessionDirectory -Force | Out-Null
New-Item -ItemType Directory -Path $UserDataDirectory -Force | Out-Null

if ($Mode -eq "Integration" -and -not $SkipBuild) {
    Write-Host "[W11 Steam] Building WorldWalkerPrototypeEditor Win64 $Configuration..." -ForegroundColor Cyan
    & $BuildScript `
        WorldWalkerPrototypeEditor `
        Win64 `
        $Configuration `
        $ProjectFile `
        -WaitMutex `
        -NoHotReloadFromIDE
    if ($LASTEXITCODE -ne 0) {
        throw "W11 Steam acceptance build failed with exit code $LASTEXITCODE."
    }
}

$HadSteamAppId = Test-Path Env:\SteamAppId
$PreviousSteamAppId = $env:SteamAppId
$HadSteamGameId = Test-Path Env:\SteamGameId
$PreviousSteamGameId = $env:SteamGameId
try {
    $env:SteamAppId = $AppId
    $env:SteamGameId = $AppId
    $Process = Start-Process `
        -FilePath $Executable `
        -ArgumentList $LaunchArguments `
        -WorkingDirectory $LaunchWorkingDirectory `
        -PassThru
}
finally {
    if ($HadSteamAppId) { $env:SteamAppId = $PreviousSteamAppId } else { Remove-Item Env:\SteamAppId -ErrorAction SilentlyContinue }
    if ($HadSteamGameId) { $env:SteamGameId = $PreviousSteamGameId } else { Remove-Item Env:\SteamGameId -ErrorAction SilentlyContinue }
}

Write-Host "[W11 Steam] Waiting for the interactive process to exit..." -ForegroundColor Cyan
$Process.WaitForExit()
if ($Process.ExitCode -ne 0) {
    throw "W11 Steam acceptance process exited with code $($Process.ExitCode)."
}
if (-not (Test-Path -LiteralPath $LogPath)) {
    throw "W11 Steam acceptance process did not create its evidence log: $LogPath"
}
if ($Mode -eq "Release" -and $PortableKitVerification -eq "FullInventory") {
    & $PythonCommand -B $KitVerifier $PortableKitManifest
    if ($LASTEXITCODE -ne 0) {
        throw "Portable W11 acceptance kit post-exit gate failed with exit code $LASTEXITCODE."
    }
}

$VerifierArguments = @(
    "--role", $Role,
    "--log", $LogPath,
    "--scenario", $Scenario,
    "--expected-players", $ExpectedPlayers,
    "--app-id", $AppId,
    "--token", $EvidenceToken
)
if ($Mode -eq "Release") {
    $VerifierArguments += @(
        "--package-manifest", $PackageManifest,
        "--output", $ReceiptPath
    )
    if ($PortableKitVerification -eq "FullInventory") {
        $VerifierArguments += @("--kit-manifest", $PortableKitManifest)
    }
}
else {
    $VerifierArguments += @("--package-hash", $PackageHash, "--allow-editor-integration")
}
& $PythonCommand $Verifier @VerifierArguments
if ($LASTEXITCODE -ne 0) {
    throw "W11 Steam $Role evidence gate failed with exit code $LASTEXITCODE."
}

Write-Output (
    "W11_STEAM_ACCEPTANCE_ROLE_READY Role=$Role Scenario=$Scenario ExpectedPlayers=$ExpectedPlayers AppId=$AppId Token=$EvidenceToken " +
    "PackageHash=$PackageHash Runtime=$RuntimeKind " +
    "PostExitPackageVerified=$(if ($Mode -eq 'Release') { '1' } else { 'EDITOR' }) " +
    "PostExitKitVerified=$(if ($PortableKitVerification -eq 'FullInventory') { '1' } else { $PortableKitVerification }) " +
    "KitIdentity=$(if ($PortableKitVerification -eq 'FullInventory') { 'VerifiedInReceipt' } else { $PortableKitVerification }) " +
    "ParticipantIdentity=$(if ($Mode -eq 'Release') { 'VerifiedInReceipt' } else { 'EDITOR' }) " +
    "Receipt=$(if ($Mode -eq 'Release') { $ReceiptPath } else { 'EDITOR' }) " +
    "Log=$LogPath PairGate=PENDING"
)
