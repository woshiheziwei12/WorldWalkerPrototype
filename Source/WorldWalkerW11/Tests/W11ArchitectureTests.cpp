#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include <limits>

#include "Core/W11StatLibrary.h"
#include "Core/W11ArenaBounds.h"
#include "Components/W11AttributeComponent.h"
#include "Components/W11CombatComponent.h"
#include "Data/W11Definitions.h"
#include "Engine/Texture2D.h"
#include "Game/W11Enemy.h"
#include "Game/W11EnemyProjectile.h"
#include "Game/W11Character.h"
#include "Game/W11ChapterRouteSubsystem.h"
#include "Game/W11GameMode.h"
#include "Game/W11GameState.h"
#include "Game/W11RunDirector.h"
#include "Materials/MaterialInterface.h"
#include "Online/W11SessionSubsystem.h"
#include "PaperFlipbook.h"
#include "Sound/SoundWave.h"
#include "UI/W11CombatHUDWidget.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11SteamEvidenceContractsTest,
	"WorldWalker.W11.Online.SteamEvidenceContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11SteamEvidenceContractsTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("host is a valid Steam evidence role"),
		UW11SessionSubsystem::IsValidSteamEvidenceRole(TEXT("Host")));
	TestTrue(TEXT("client is a valid Steam evidence role"),
		UW11SessionSubsystem::IsValidSteamEvidenceRole(TEXT("Client")));
	TestFalse(TEXT("role matching remains case-sensitive"),
		UW11SessionSubsystem::IsValidSteamEvidenceRole(TEXT("host")));
	TestFalse(TEXT("unfrozen migration behavior is not an evidence role"),
		UW11SessionSubsystem::IsValidSteamEvidenceRole(TEXT("HostMigration")));
	for (const FString Scenario : {
		TEXT("BasicJoin"), TEXT("FriendInvite"), TEXT("HostExit"),
		TEXT("ClientDisconnect"), TEXT("PlayerSync")})
	{
		TestTrue(*FString::Printf(TEXT("%s is a valid evidence scenario"), *Scenario),
			UW11SessionSubsystem::IsValidSteamEvidenceScenario(Scenario));
	}
	TestFalse(TEXT("host migration is not a frozen acceptance scenario"),
		UW11SessionSubsystem::IsValidSteamEvidenceScenario(TEXT("HostMigration")));
	TestFalse(TEXT("scenario matching remains case-sensitive"),
		UW11SessionSubsystem::IsValidSteamEvidenceScenario(TEXT("basicjoin")));

	TestTrue(TEXT("bounded ASCII evidence tokens are accepted"),
		UW11SessionSubsystem::IsValidSteamEvidenceToken(TEXT("W11_20260830-ABCD")));
	TestFalse(TEXT("short evidence tokens are rejected"),
		UW11SessionSubsystem::IsValidSteamEvidenceToken(TEXT("short")));
	TestFalse(TEXT("tokens with whitespace are rejected"),
		UW11SessionSubsystem::IsValidSteamEvidenceToken(TEXT("W11 invalid token")));
	TestFalse(TEXT("tokens with non-ASCII characters are rejected"),
		UW11SessionSubsystem::IsValidSteamEvidenceToken(TEXT("W11_验收_20260830")));

	TestTrue(TEXT("a SHA-256 package identity is accepted"),
		UW11SessionSubsystem::IsValidSteamEvidencePackageHash(
			TEXT("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef")));
	TestTrue(TEXT("editor integration has an explicit non-release identity"),
		UW11SessionSubsystem::IsValidSteamEvidencePackageHash(TEXT("EDITOR")));
	TestFalse(TEXT("a short package identity is rejected"),
		UW11SessionSubsystem::IsValidSteamEvidencePackageHash(TEXT("deadbeef")));
	TestFalse(TEXT("non-hex package identity characters are rejected"),
		UW11SessionSubsystem::IsValidSteamEvidencePackageHash(
			TEXT("g123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef")));

	TestTrue(TEXT("a Steam64 account identity is accepted"),
		UW11SessionSubsystem::IsValidSteamAccountId(TEXT("76561198012345678")));
	TestFalse(TEXT("a missing Steam account identity is rejected"),
		UW11SessionSubsystem::IsValidSteamAccountId(TEXT("NONE")));
	TestFalse(TEXT("an all-zero account identity is rejected"),
		UW11SessionSubsystem::IsValidSteamAccountId(TEXT("00000000000000000")));
	TestFalse(TEXT("a short numeric identity is rejected"),
		UW11SessionSubsystem::IsValidSteamAccountId(TEXT("1234567")));

	TestTrue(TEXT("Steam with a project App ID is release-evidence eligible"),
		UW11SessionSubsystem::IsReleaseSteamEvidenceEnvironment(
			FName(TEXT("STEAM")), TEXT("1234567")));
	TestFalse(TEXT("the public Spacewar App ID cannot prove release readiness"),
		UW11SessionSubsystem::IsReleaseSteamEvidenceEnvironment(
			FName(TEXT("STEAM")), TEXT("480")));
	TestFalse(TEXT("Null OSS cannot prove Steam release readiness"),
		UW11SessionSubsystem::IsReleaseSteamEvidenceEnvironment(
			FName(TEXT("NULL")), TEXT("1234567")));
	TestFalse(TEXT("a nonnumeric App ID is rejected"),
		UW11SessionSubsystem::IsReleaseSteamEvidenceEnvironment(
			FName(TEXT("STEAM")), TEXT("ProjectApp")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11MemoryRunLifecycleTest,
	"WorldWalker.W11.Combat.MemoryRunLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11MemoryRunLifecycleTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("the first authority sequence is one"),
		AW11GameState::AdvanceAuthoritySequence(0), 1);
	TestEqual(TEXT("authority sequences advance monotonically"),
		AW11GameState::AdvanceAuthoritySequence(41), 42);
	TestEqual(TEXT("authority sequence overflow avoids reserved zero"),
		AW11GameState::AdvanceAuthoritySequence(MAX_int32), 1);

	int32 EncounterSequence = 77;
	int32 OutcomeSequence = 33;
	TestEqual(TEXT("first memory run starts at sequence one"),
		AW11GameState::AdvanceMemoryRunSequence(0, EncounterSequence, OutcomeSequence), 1);
	TestEqual(TEXT("first memory run initializes encounter sequence"), EncounterSequence, 0);
	TestEqual(TEXT("first memory run initializes outcome sequence"), OutcomeSequence, 0);
	EncounterSequence = 4;
	OutcomeSequence = 3;
	TestEqual(TEXT("next memory run advances its run identity"),
		AW11GameState::AdvanceMemoryRunSequence(1, EncounterSequence, OutcomeSequence), 2);
	TestEqual(TEXT("next memory run preserves the global encounter sequence"), EncounterSequence, 4);
	TestEqual(TEXT("next memory run preserves the global outcome sequence"), OutcomeSequence, 3);

	const FProperty* MemoryRunProperty = FindFProperty<FProperty>(
		AW11GameState::StaticClass(), TEXT("MemoryRunSequence"));
	TestNotNull(TEXT("memory run identity is reflected"), MemoryRunProperty);
	if (MemoryRunProperty)
	{
		TestTrue(TEXT("memory run identity is replicated"),
			MemoryRunProperty->HasAnyPropertyFlags(CPF_Net));
		TestEqual(TEXT("memory run identity notifies runtime subscribers"),
			MemoryRunProperty->RepNotifyFunc, FName(TEXT("OnRep_RuntimeState")));
	}
	TestNotNull(TEXT("game mode exposes a pre-reset persistence boundary"),
		FindFProperty<FMulticastDelegateProperty>(AW11GameMode::StaticClass(), TEXT("OnBeforeMemoryRunReset")));
	TestNotNull(TEXT("reset context exposes the completed run identity"),
		FindFProperty<FIntProperty>(FW11MemoryRunResetContext::StaticStruct(), TEXT("CompletedMemoryRunSequence")));
	TestNotNull(TEXT("reset context carries the terminal node"),
		FindFProperty<FStructProperty>(FW11MemoryRunResetContext::StaticStruct(), TEXT("TerminalNode")));
	TestNotNull(TEXT("character exposes an authoritative per-run transient reset"),
		AW11Character::StaticClass()->FindFunctionByName(TEXT("ResetForNewMemoryRun")));
	TestNotNull(TEXT("combat component exposes an authoritative cooldown reset"),
		UW11CombatComponent::StaticClass()->FindFunctionByName(TEXT("ResetForNewMemoryRun")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11ChapterTransitionOwnershipTest,
	"WorldWalker.W11.Combat.ChapterTransitionOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11ChapterTransitionOwnershipTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("no owner and no observer uses the compatibility transition"),
		AW11GameMode::ShouldChapterRouterOwnTransition(false, false));
	TestFalse(TEXT("an observation-only listener cannot capture transition ownership"),
		AW11GameMode::ShouldChapterRouterOwnTransition(false, true));
	TestTrue(TEXT("an explicit owner controls the transition without observers"),
		AW11GameMode::ShouldChapterRouterOwnTransition(true, false));
	TestTrue(TEXT("an explicit owner remains authoritative when observers are present"),
		AW11GameMode::ShouldChapterRouterOwnTransition(true, true));
	TestNotNull(TEXT("chapter ownership has an explicit claim API"),
		AW11GameMode::StaticClass()->FindFunctionByName(TEXT("ClaimChapterTransitionOwnership")));
	TestNotNull(TEXT("chapter ownership has an explicit release API"),
		AW11GameMode::StaticClass()->FindFunctionByName(TEXT("ReleaseChapterTransitionOwnership")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11RecoveryPunishTelemetryTest,
	"WorldWalker.W11.Combat.RecoveryPunishTelemetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11RecoveryPunishTelemetryTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("positive damage during recovery is a punish hit"),
		AW11GameMode::IsRecoveryPunishHit(EW11EnemyCombatState::Recovery, 1.0f));
	TestFalse(TEXT("zero damage during recovery is not a punish hit"),
		AW11GameMode::IsRecoveryPunishHit(EW11EnemyCombatState::Recovery, 0.0f));
	TestFalse(TEXT("windup damage is not a recovery punish"),
		AW11GameMode::IsRecoveryPunishHit(EW11EnemyCombatState::Windup, 20.0f));
	TestFalse(TEXT("active-frame damage is not a recovery punish"),
		AW11GameMode::IsRecoveryPunishHit(EW11EnemyCombatState::Active, 20.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11ManualEvidenceSessionContractTest,
	"WorldWalker.W11.Combat.ManualEvidenceSessionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11ManualEvidenceSessionContractTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("automated outcome exit flag is recognized"),
		AW11GameMode::IsAutomatedOutcomeExitRequested(TEXT("-W11AutoQuitOnOutcome")));
	TestFalse(TEXT("automated outcome exit stays opt-in"),
		AW11GameMode::IsAutomatedOutcomeExitRequested(TEXT("-W11AutoStart")));
	TSet<FString> Instructions;
	const auto IsInteractiveStandaloneAllowed = [](const FName Role, const FName InputScript)
	{
		return AW11GameMode::IsManualEvidenceSessionAllowed(
			Role, InputScript, false, false, false, false, false, false, NM_Standalone);
	};
	for (const FName Role : {
		FName(TEXT("Observe")), FName(TEXT("Evasion")), FName(TEXT("Dodge")),
		FName(TEXT("Recovery")), FName(TEXT("Feedback")) })
	{
		TestTrue(FString::Printf(TEXT("manual role %s is accepted"), *Role.ToString()),
			AW11GameMode::IsValidManualEvidenceRole(Role));
		TestTrue(FString::Printf(TEXT("manual role %s accepts no input script"), *Role.ToString()),
			IsInteractiveStandaloneAllowed(Role, TEXT("None")));
		TestFalse(FString::Printf(TEXT("manual role %s rejects scripted input"), *Role.ToString()),
			IsInteractiveStandaloneAllowed(Role, TEXT("RecoveryPunish")));
		const FString DisplayName = AW11GameMode::GetManualEvidenceRoleDisplayName(Role);
		const FString Instruction = AW11GameMode::GetManualEvidenceRoleInstruction(Role);
		TestFalse(FString::Printf(TEXT("manual role %s has a display name"), *Role.ToString()),
			DisplayName.IsEmpty());
		TestTrue(FString::Printf(TEXT("manual role %s has actionable guidance"), *Role.ToString()),
			Instruction.Len() >= 20);
		Instructions.Add(Instruction);
	}
	TestEqual(TEXT("all five manual roles have distinct guidance"), Instructions.Num(), 5);
	TestFalse(TEXT("unknown manual roles are rejected"),
		AW11GameMode::IsValidManualEvidenceRole(TEXT("Other")));
	TestFalse(TEXT("empty manual roles are rejected"),
		AW11GameMode::IsValidManualEvidenceRole(NAME_None));
	TestEqual(TEXT("scripted input has an explicit rejection reason"),
		AW11GameMode::ResolveManualEvidenceSessionRejectionReason(
			TEXT("Recovery"), TEXT("RecoveryPunish"), false, false, false, false, false, false, NM_Standalone),
		FName(TEXT("ScriptedInput")));
	TestEqual(TEXT("unattended sessions are rejected"),
		AW11GameMode::ResolveManualEvidenceSessionRejectionReason(
			TEXT("Observe"), TEXT("None"), true, false, false, false, false, false, NM_Standalone),
		FName(TEXT("Unattended")));
	TestEqual(TEXT("NullRHI sessions are rejected"),
		AW11GameMode::ResolveManualEvidenceSessionRejectionReason(
			TEXT("Observe"), TEXT("None"), false, true, false, false, false, false, NM_Standalone),
		FName(TEXT("NullRHI")));
	TestEqual(TEXT("offscreen rendering sessions are rejected"),
		AW11GameMode::ResolveManualEvidenceSessionRejectionReason(
			TEXT("Observe"), TEXT("None"), false, false, true, false, false, false, NM_Standalone),
		FName(TEXT("RenderOffscreen")));
	TestEqual(TEXT("audio-disabled sessions are rejected"),
		AW11GameMode::ResolveManualEvidenceSessionRejectionReason(
			TEXT("Observe"), TEXT("None"), false, false, false, true, false, false, NM_Standalone),
		FName(TEXT("NoSound")));
	TestEqual(TEXT("automatic starts are rejected"),
		AW11GameMode::ResolveManualEvidenceSessionRejectionReason(
			TEXT("Observe"), TEXT("None"), false, false, false, false, true, false, NM_Standalone),
		FName(TEXT("AutoStart")));
	TestEqual(TEXT("test overrides are rejected"),
		AW11GameMode::ResolveManualEvidenceSessionRejectionReason(
			TEXT("Observe"), TEXT("None"), false, false, false, false, false, true, NM_Standalone),
		FName(TEXT("TestOverride")));
	TestEqual(TEXT("networked sessions cannot satisfy the single-player evidence role"),
		AW11GameMode::ResolveManualEvidenceSessionRejectionReason(
			TEXT("Observe"), TEXT("None"), false, false, false, false, false, false, NM_ListenServer),
		FName(TEXT("NonStandalone")));
	TestNotNull(TEXT("combat HUD exposes a manual evidence guide panel"),
		FindFProperty<FObjectProperty>(UW11CombatHUDWidget::StaticClass(), TEXT("ManualEvidencePanel")));
	TestNotNull(TEXT("combat HUD exposes manual evidence guide text"),
		FindFProperty<FObjectProperty>(UW11CombatHUDWidget::StaticClass(), TEXT("ManualEvidenceText")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11ManualEvidenceProgressTest,
	"WorldWalker.W11.Combat.ManualEvidenceProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11ManualEvidenceProgressTest::RunTest(const FString& Parameters)
{
	FW11CombatTelemetry Telemetry;
	Telemetry.Enemies.FindOrAdd(TEXT("Enemy.MiasmaWisp")).Attacks = 1;
	Telemetry.Enemies.FindOrAdd(TEXT("Enemy.SwordWraith")).Attacks = 1;
	Telemetry.Enemies.FindOrAdd(TEXT("Enemy.StoneFiend")).Attacks = 1;
	FW11ManualEvidenceProgress Progress = AW11GameMode::ResolveManualEvidenceProgress(TEXT("Observe"), Telemetry);
	TestEqual(TEXT("observe completes all three enemy identities"), Progress.CompletedSteps, 3);
	TestTrue(TEXT("observe is complete without forbidden actions"), Progress.bComplete);
	Telemetry.AbilityUses = 1;
	Progress = AW11GameMode::ResolveManualEvidenceProgress(TEXT("Observe"), Telemetry);
	TestTrue(TEXT("observe marks attacking as a role violation"), Progress.bViolation);
	TestFalse(TEXT("a violated observe role cannot complete"), Progress.bComplete);

	Telemetry = FW11CombatTelemetry();
	FW11EnemyCombatTelemetry& Miasma = Telemetry.Enemies.FindOrAdd(TEXT("Enemy.MiasmaWisp"));
	Miasma.Attacks = 2;
	Miasma.Hits = 1;
	Miasma.Evaded = 1;
	Progress = AW11GameMode::ResolveManualEvidenceProgress(TEXT("Evasion"), Telemetry);
	TestEqual(TEXT("movement evasion completes attack and miss requirements"), Progress.CompletedSteps, 2);
	TestTrue(TEXT("movement-only evasion completes"), Progress.bComplete);
	Telemetry.Dodges = 1;
	Progress = AW11GameMode::ResolveManualEvidenceProgress(TEXT("Evasion"), Telemetry);
	TestTrue(TEXT("using dodge violates movement-only evidence"), Progress.bViolation);

	Telemetry = FW11CombatTelemetry();
	Telemetry.SuccessfulDodgeImmunities = 1;
	Progress = AW11GameMode::ResolveManualEvidenceProgress(TEXT("Dodge"), Telemetry);
	TestTrue(TEXT("one authoritative immunity completes dodge evidence"), Progress.bComplete);
	Telemetry = FW11CombatTelemetry();
	Telemetry.RecoveryPunishHits = 1;
	Progress = AW11GameMode::ResolveManualEvidenceProgress(TEXT("Recovery"), Telemetry);
	TestTrue(TEXT("one recovery health hit completes recovery evidence"), Progress.bComplete);

	Telemetry = FW11CombatTelemetry();
	Telemetry.bBasicAttackEffectObserved = true;
	Telemetry.bMultiTargetActiveObserved = true;
	Telemetry.bActualHealObserved = true;
	Telemetry.bActualBarrierObserved = true;
	Telemetry.CriticalHits = 1;
	Telemetry.BarrierAbsorbed = 1.0f;
	Telemetry.bEnemyDefeatObserved = true;
	Progress = AW11GameMode::ResolveManualEvidenceProgress(TEXT("Feedback"), Telemetry);
	TestEqual(TEXT("feedback has seven independent objective facts"), Progress.TotalSteps, 7);
	TestEqual(TEXT("all seven feedback facts are complete"), Progress.CompletedSteps, 7);
	TestTrue(TEXT("complete feedback is reported"), Progress.bComplete);
	Telemetry.bActualHealObserved = false;
	Progress = AW11GameMode::ResolveManualEvidenceProgress(TEXT("Feedback"), Telemetry);
	TestEqual(TEXT("missing healing leaves one feedback fact incomplete"), Progress.CompletedSteps, 6);
	TestFalse(TEXT("partial feedback is not complete"), Progress.bComplete);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11CombatNodeParticipantSetsTest,
	"WorldWalker.W11.Combat.NodeParticipantSets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11CombatNodeParticipantSetsTest::RunTest(const FString& Parameters)
{
	TArray<int32> Defeated;
	TArray<int32> Surviving;
	AW11GameMode::ClassifyCombatParticipant(257, false, Defeated, Surviving);
	AW11GameMode::ClassifyCombatParticipant(256, true, Defeated, Surviving);
	AW11GameMode::ClassifyCombatParticipant(258, false, Defeated, Surviving);
	TestEqual(TEXT("one defeated participant is classified"), Defeated.Num(), 1);
	TestEqual(TEXT("defeated participant keeps its stable player id"), Defeated[0], 256);
	TestEqual(TEXT("two surviving participants are classified"), Surviving.Num(), 2);
	TestEqual(TEXT("surviving participants sort the lower stable id first"), Surviving[0], 257);
	TestEqual(TEXT("surviving participants sort the higher stable id last"), Surviving[1], 258);

	AW11GameMode::ClassifyCombatParticipant(257, true, Defeated, Surviving);
	AW11GameMode::ClassifyCombatParticipant(257, true, Defeated, Surviving);
	TestEqual(TEXT("reclassification remains deduplicated"), Defeated.Num(), 2);
	TestEqual(TEXT("reclassified participant joins the defeated set"), Defeated[1], 257);
	TestEqual(TEXT("participant sets remain mutually exclusive"), Surviving.Num(), 1);
	TestEqual(TEXT("unaffected survivor remains present"), Surviving[0], 258);
	AW11GameMode::ClassifyCombatParticipant(-1, false, Defeated, Surviving);
	TestEqual(TEXT("invalid player ids are ignored"), Surviving.Num(), 1);

	const FArrayProperty* DefeatedArray = FindFProperty<FArrayProperty>(
		FW11CombatNodeResult::StaticStruct(), TEXT("DefeatedPlayerIds"));
	const FArrayProperty* SurvivingArray = FindFProperty<FArrayProperty>(
		FW11CombatNodeResult::StaticStruct(), TEXT("SurvivingPlayerIds"));
	TestNotNull(TEXT("node payload reflects defeated player ids"), DefeatedArray);
	TestNotNull(TEXT("node payload reflects surviving player ids"), SurvivingArray);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11ImpactPresentationTierTest,
	"WorldWalker.W11.Combat.ImpactPresentationTiers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11ImpactPresentationTierTest::RunTest(const FString& Parameters)
{
	UW11SkillVisualDefinition* Profile = NewObject<UW11SkillVisualDefinition>();
	TestNotNull(TEXT("impact presentation profile can be created"), Profile);
	if (!Profile)
	{
		return false;
	}
	TestTrue(TEXT("a critical hit holds longer than a normal hit"),
		Profile->ResolveHitStopSeconds(EW11CombatCueType::Damage, true)
		> Profile->ResolveHitStopSeconds(EW11CombatCueType::Damage, false));
	TestTrue(TEXT("a defeat holds at least as long as a critical hit"),
		Profile->ResolveHitStopSeconds(EW11CombatCueType::Defeated, false)
		>= Profile->ResolveHitStopSeconds(EW11CombatCueType::Damage, true));
	TestTrue(TEXT("a critical camera impulse is stronger than a normal impact"),
		Profile->ResolveCameraImpactStrength(EW11CombatCueType::Damage, true)
		> Profile->ResolveCameraImpactStrength(EW11CombatCueType::Damage, false));
	TestTrue(TEXT("a defeat camera impulse is the strongest default tier"),
		Profile->ResolveCameraImpactStrength(EW11CombatCueType::Defeated, false)
		>= Profile->ResolveCameraImpactStrength(EW11CombatCueType::Damage, true));
	Profile->DefeatHitStopSeconds = 10.0f;
	Profile->DefeatCameraImpactStrength = 100.0f;
	TestEqual(TEXT("local hit stop has a hard presentation cap"),
		Profile->ResolveHitStopSeconds(EW11CombatCueType::Defeated, false), 0.12f);
	TestEqual(TEXT("local camera displacement has a hard presentation cap"),
		Profile->ResolveCameraImpactStrength(EW11CombatCueType::Defeated, false), 8.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11EncounterBudgetRefillTest,
	"WorldWalker.W11.Combat.EncounterBudgetRefill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11EncounterBudgetRefillTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("fifteen budget is divided evenly across three waves"),
		AW11RunDirector::CalculateCurrentWaveBudget(15, 3, false), 5);
	TestEqual(TEXT("ceil division preserves all budget across uneven waves"),
		AW11RunDirector::CalculateCurrentWaveBudget(11, 3, false), 4);
	TestEqual(TEXT("the final wave owns the complete remaining budget"),
		AW11RunDirector::CalculateCurrentWaveBudget(7, 1, true), 7);
	TestEqual(TEXT("invalid remaining wave count cannot create budget"),
		AW11RunDirector::CalculateCurrentWaveBudget(7, 0, false), 0);

	TestTrue(TEXT("an open concurrency slot refills the current wave"),
		AW11RunDirector::ShouldRefillCurrentWave(2, 4, 8, 3));
	TestFalse(TEXT("the concurrency cap prevents an extra simultaneous spawn"),
		AW11RunDirector::ShouldRefillCurrentWave(3, 4, 8, 3));
	TestFalse(TEXT("an exhausted current-wave budget cannot refill"),
		AW11RunDirector::ShouldRefillCurrentWave(0, 0, 8, 3));
	TestFalse(TEXT("an exhausted encounter budget cannot refill"),
		AW11RunDirector::ShouldRefillCurrentWave(0, 4, 0, 3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11HudRuntimeNotificationContractTest,
	"WorldWalker.W11.Combat.HudRuntimeNotificationContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11HudRuntimeNotificationContractTest::RunTest(const FString& Parameters)
{
	const FProperty* RunPhaseProperty = FindFProperty<FProperty>(
		AW11GameState::StaticClass(), TEXT("RunPhase"));
	TestNotNull(TEXT("run phase remains a reflected replicated fact"), RunPhaseProperty);
	if (RunPhaseProperty)
	{
		TestTrue(TEXT("run phase is replicated"), RunPhaseProperty->HasAnyPropertyFlags(CPF_Net));
		TestEqual(TEXT("run phase wakes subscribers through the shared RepNotify"),
			RunPhaseProperty->RepNotifyFunc, FName(TEXT("OnRep_RuntimeState")));
	}

	const FProperty* ObjectiveProperty = FindFProperty<FProperty>(
		AW11GameState::StaticClass(), TEXT("ObjectiveEndServerTime"));
	TestNotNull(TEXT("objective deadline remains reflected"), ObjectiveProperty);
	if (ObjectiveProperty)
	{
		TestEqual(TEXT("objective deadline uses the runtime-state notification"),
			ObjectiveProperty->RepNotifyFunc, FName(TEXT("OnRep_RuntimeState")));
	}

	TestNotNull(TEXT("game state exposes a multicast runtime-state event"),
		FindFProperty<FMulticastDelegateProperty>(AW11GameState::StaticClass(), TEXT("OnRuntimeStateChanged")));

	const FProperty* DodgeCooldownProperty = FindFProperty<FProperty>(
		AW11Character::StaticClass(), TEXT("DodgeCooldownEndTime"));
	TestNotNull(TEXT("dodge cooldown deadline remains reflected"), DodgeCooldownProperty);
	if (DodgeCooldownProperty)
	{
		TestTrue(TEXT("dodge cooldown deadline is replicated"),
			DodgeCooldownProperty->HasAnyPropertyFlags(CPF_Net));
		TestEqual(TEXT("dodge cooldown deadline has an owner notification"),
			DodgeCooldownProperty->RepNotifyFunc, FName(TEXT("OnRep_DodgeCooldownEndTime")));
	}
	TestNotNull(TEXT("character exposes a multicast dodge cooldown event"),
		FindFProperty<FMulticastDelegateProperty>(AW11Character::StaticClass(), TEXT("OnDodgeCooldownChanged")));

	TestNotNull(TEXT("combat component exposes authoritative cooldown notifications"),
		FindFProperty<FMulticastDelegateProperty>(UW11CombatComponent::StaticClass(), TEXT("OnCooldownChanged")));
	TestNotNull(TEXT("combat component exposes ability execution results"),
		FindFProperty<FMulticastDelegateProperty>(UW11CombatComponent::StaticClass(), TEXT("OnAbilityResult")));
	TestNotNull(TEXT("attribute component exposes barrier notifications"),
		FindFProperty<FMulticastDelegateProperty>(UW11AttributeComponent::StaticClass(), TEXT("OnBarrierChanged")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11CombatCueDeduplicationTest,
	"WorldWalker.W11.Combat.CombatCueDeduplication",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11CombatCueDeduplicationTest::RunTest(const FString& Parameters)
{
	FW11CombatCueDeduplicator Deduplicator;
	FW11CombatCue Cue;
	Cue.EncounterInstanceId = 1;
	Cue.SourceStableId = 11;
	Cue.AuthorityExecutionSequence = 7;
	Cue.EffectSequence = 1;
	TestTrue(TEXT("first presentation cue is accepted"), Deduplicator.Accept(Cue));
	TestFalse(TEXT("the same source/encounter/execution/effect cue is filtered"), Deduplicator.Accept(Cue));
	Cue.SourceStableId = 12;
	TestTrue(TEXT("another source with the same execution/effect remains visible"), Deduplicator.Accept(Cue));
	TestFalse(TEXT("the second source is independently deduplicated"), Deduplicator.Accept(Cue));
	Cue.EffectSequence = 2;
	TestTrue(TEXT("another effect from the same execution remains visible"), Deduplicator.Accept(Cue));
	Cue.EncounterInstanceId = 2;
	Cue.EffectSequence = 1;
	TestTrue(TEXT("a new encounter resets the presentation key space"), Deduplicator.Accept(Cue));
	for (int32 Index = 1; Index <= 300; ++Index)
	{
		Cue.AuthorityExecutionSequence = Index;
		Cue.EffectSequence = 3;
		Deduplicator.Accept(Cue);
	}
	TestEqual(TEXT("the transient cue cache stays bounded"), Deduplicator.NumBuffered(), 256);
	Cue.AuthorityExecutionSequence = 1;
	TestTrue(TEXT("an evicted old key no longer grows the bounded cache"), Deduplicator.Accept(Cue));
	TestEqual(TEXT("accepting after eviction preserves the bound"), Deduplicator.NumBuffered(), 256);
	FW11CombatCue InvalidCue;
	TestFalse(TEXT("zero-instance cues cannot collide in the presentation cache"),
		Deduplicator.Accept(InvalidCue));
	InvalidCue.EncounterInstanceId = 1;
	InvalidCue.AuthorityExecutionSequence = 1;
	InvalidCue.EffectSequence = 1;
	TestFalse(TEXT("cues without stable source identity are rejected"), Deduplicator.Accept(InvalidCue));

	FW11CombatAudioGate AudioGate;
	Cue.EncounterInstanceId = 3;
	Cue.SourceStableId = 21;
	Cue.AuthorityExecutionSequence = 4;
	Cue.EffectSequence = 1;
	TestTrue(TEXT("the first primary sound for an execution is accepted"),
		AudioGate.Accept(Cue, EW11CombatAudioLayer::Primary));
	Cue.EffectSequence = 2;
	TestFalse(TEXT("another target cannot multiply the primary sound"),
		AudioGate.Accept(Cue, EW11CombatAudioLayer::Primary));
	TestTrue(TEXT("a critical accent remains an independent restrained layer"),
		AudioGate.Accept(Cue, EW11CombatAudioLayer::Critical));
	TestFalse(TEXT("the critical accent is also limited to one per execution"),
		AudioGate.Accept(Cue, EW11CombatAudioLayer::Critical));
	Cue.SourceStableId = 22;
	TestTrue(TEXT("another source keeps an independent sound execution"),
		AudioGate.Accept(Cue, EW11CombatAudioLayer::Primary));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11NonDamageEffectsTest,
	"WorldWalker.W11.Combat.NonDamageEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11NonDamageEffectsTest::RunTest(const FString& Parameters)
{
	const FName AbilityId(TEXT("Ability.ResourceContract"));
	const FW11EffectResult Heal = UW11CombatComponent::BuildNonDamageEffectResult(
		EW11EffectResultType::Heal, nullptr, nullptr, AbilityId, 30.0f, 18.0f);
	const FW11EffectResult Barrier = UW11CombatComponent::BuildNonDamageEffectResult(
		EW11EffectResultType::Barrier, nullptr, nullptr, AbilityId, 35.0f, 35.0f);
	const FW11EffectResult Mana = UW11CombatComponent::BuildNonDamageEffectResult(
		EW11EffectResultType::Mana, nullptr, nullptr, AbilityId, 20.0f, 12.0f);
	TestEqual(TEXT("healing remains an explicit effect type"), Heal.Type, EW11EffectResultType::Heal);
	TestEqual(TEXT("healing records requested amount"), Heal.RequestedAmount, 30.0f);
	TestEqual(TEXT("healing records authoritative actual amount"), Heal.ActualAmount, 18.0f);
	TestEqual(TEXT("barrier gain remains an explicit effect type"), Barrier.Type, EW11EffectResultType::Barrier);
	TestEqual(TEXT("barrier gain records authoritative actual amount"), Barrier.ActualAmount, 35.0f);
	TestEqual(TEXT("mana restoration remains an explicit effect type"), Mana.Type, EW11EffectResultType::Mana);
	TestEqual(TEXT("mana restoration records authoritative actual amount"), Mana.ActualAmount, 12.0f);
	TestEqual(TEXT("all non-damage effects retain their ability source"), Mana.AbilityId, AbilityId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11AbilityRejectionTest,
	"WorldWalker.W11.Combat.AbilityRejection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11AbilityRejectionTest::RunTest(const FString& Parameters)
{
	FW11AbilityRequest Request;
	Request.EncounterInstanceId = 14;
	Request.ClientRequestSequence = 9;
	Request.Slot = EW11AbilitySlot::Active2;
	const FW11AbilityExecutionResult Result = UW11CombatComponent::BuildRejectedAbilityResult(
		Request, EW11AbilityRejectionReason::Cooldown, 23.5f, 6, nullptr,
		TEXT("Ability.Contract"));
	TestFalse(TEXT("rejection is never reported as accepted"), Result.bAccepted);
	TestEqual(TEXT("rejection preserves encounter identity"), Result.EncounterInstanceId, 14);
	TestEqual(TEXT("rejection preserves client request identity"), Result.ClientRequestSequence, 9);
	TestEqual(TEXT("rejection carries the latest authority sequence"), Result.AuthorityExecutionSequence, 6);
	TestEqual(TEXT("rejection exposes its reason"), Result.RejectionReason, EW11AbilityRejectionReason::Cooldown);
	TestEqual(TEXT("rejection exposes authoritative cooldown end"), Result.ServerCooldownEndTime, 23.5f);
	TestEqual(TEXT("rejection preserves the logical slot"), Result.Slot, EW11AbilitySlot::Active2);
	TestEqual(TEXT("rejection preserves the resolved ability"), Result.AbilityId, FName(TEXT("Ability.Contract")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11DamageBreakdownTest,
	"WorldWalker.W11.Combat.DamageBreakdown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11DamageBreakdownTest::RunTest(const FString& Parameters)
{
	const FW11DamageBreakdown Damage = UW11AttributeComponent::CalculateDamageBreakdown(
		160.0f, 100.0f, 20.0f, 55.0f, false);
	TestEqual(TEXT("raw damage is retained"), Damage.RawDamage, 160.0f);
	TestEqual(TEXT("armor mitigation is explicit"), Damage.ArmorMitigated, 80.0f);
	TestEqual(TEXT("barrier absorption is explicit"), Damage.BarrierAbsorbed, 20.0f);
	TestEqual(TEXT("health receives only the remainder"), Damage.HealthDamage, 55.0f);
	TestTrue(TEXT("lethal damage is explicit"), Damage.bDefeated);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11CooldownScalingTest,
	"WorldWalker.W11.Combat.CooldownScaling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11CooldownScalingTest::RunTest(const FString& Parameters)
{
	FW11StatBlock Stats;
	Stats.AttackSpeedScale = 2.0f;
	Stats.AbilityHaste = 1.0f;
	TestEqual(TEXT("attack speed scales attack cooldown"),
		UW11CombatComponent::CalculateAbilityCooldown(10.0f, EW11CooldownScaling::AttackSpeed, Stats), 5.0f);
	TestEqual(TEXT("ability haste scales ability cooldown"),
		UW11CombatComponent::CalculateAbilityCooldown(10.0f, EW11CooldownScaling::AbilityHaste, Stats), 5.0f);
	TestEqual(TEXT("none preserves authored cooldown"),
		UW11CombatComponent::CalculateAbilityCooldown(10.0f, EW11CooldownScaling::None, Stats), 10.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11StableMultiTargetOrderTest,
	"WorldWalker.W11.Combat.StableMultiTargetOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11StableMultiTargetOrderTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("nearer targets sort first"),
		UW11CombatComponent::IsStableTargetBefore(10.0f, 9, 20.0f, 1));
	TestFalse(TEXT("farther targets never sort first"),
		UW11CombatComponent::IsStableTargetBefore(20.0f, 1, 10.0f, 9));
	TestTrue(TEXT("equal-distance targets use stable spawn id"),
		UW11CombatComponent::IsStableTargetBefore(10.0f, 3, 10.0f, 7));
	TestFalse(TEXT("stable spawn id tie-break is antisymmetric"),
		UW11CombatComponent::IsStableTargetBefore(10.0f, 7, 10.0f, 3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11DodgeImmunityTest,
	"WorldWalker.W11.Combat.DodgeImmunity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11DodgeImmunityTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("damage immunity is active before its authoritative end"),
		UW11AttributeComponent::IsImmunityWindowActive(4.99f, 5.0f));
	TestFalse(TEXT("damage immunity ends exactly at its authoritative end"),
		UW11AttributeComponent::IsImmunityWindowActive(5.0f, 5.0f));
	TestFalse(TEXT("damage immunity remains inactive afterwards"),
		UW11AttributeComponent::IsImmunityWindowActive(5.01f, 5.0f));
	TestTrue(TEXT("normalized in-combat dodge request is valid"),
		AW11Character::IsValidDodgeRequest(
			FVector::RightVector, true, false, false, 3.0f, 2.0f));
	TestFalse(TEXT("dodge request is rejected outside pending combat"),
		AW11Character::IsValidDodgeRequest(
			FVector::RightVector, false, false, false, 3.0f, 2.0f));
	TestFalse(TEXT("dodge request is rejected during cooldown"),
		AW11Character::IsValidDodgeRequest(
			FVector::RightVector, true, false, false, 3.0f, 4.0f));
	TestFalse(TEXT("dodge request is rejected when defeated"),
		AW11Character::IsValidDodgeRequest(
			FVector::RightVector, true, false, true, 3.0f, 2.0f));
	TestFalse(TEXT("dodge request rejects non-finite direction"),
		AW11Character::IsValidDodgeRequest(
			FVector(std::numeric_limits<float>::infinity(), 0.0f, 0.0f),
			true, false, false, 3.0f, 2.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11VictoryOnceTest,
	"WorldWalker.W11.Combat.VictoryOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11VictoryOnceTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("pending encounter accepts its first victory"),
		AW11GameState::CanTransitionCombatOutcome(8, 8, EW11CombatOutcome::Pending, EW11CombatOutcome::Victory));
	TestFalse(TEXT("resolved victory cannot resolve again"),
		AW11GameState::CanTransitionCombatOutcome(8, 8, EW11CombatOutcome::Victory, EW11CombatOutcome::Victory));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11DeathOnceTest,
	"WorldWalker.W11.Combat.DeathOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11DeathOnceTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("pending encounter accepts its first defeat"),
		AW11GameState::CanTransitionCombatOutcome(8, 8, EW11CombatOutcome::Pending, EW11CombatOutcome::Defeat));
	TestFalse(TEXT("resolved defeat cannot resolve again"),
		AW11GameState::CanTransitionCombatOutcome(8, 8, EW11CombatOutcome::Defeat, EW11CombatOutcome::Defeat));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11TerminalPrecedenceTest,
	"WorldWalker.W11.Combat.TerminalPrecedence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11TerminalPrecedenceTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("solo defeat wins a same-batch victory race"),
		AW11GameMode::ResolveSoloTerminalOutcome(EW11CombatOutcome::Victory, true),
		EW11CombatOutcome::Defeat);
	TestEqual(TEXT("living solo player retains requested victory"),
		AW11GameMode::ResolveSoloTerminalOutcome(EW11CombatOutcome::Victory, false),
		EW11CombatOutcome::Victory);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11StaleEncounterCallbacksTest,
	"WorldWalker.W11.Combat.StaleEncounterCallbacks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11StaleEncounterCallbacksTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("matching pending combat encounter remains live"),
		AW11GameState::IsPendingCombatEncounter(
			EW11RunPhase::Combat, EW11CombatOutcome::Pending, 12, 12));
	TestFalse(TEXT("old encounter callbacks are rejected"),
		AW11GameState::IsPendingCombatEncounter(
			EW11RunPhase::Combat, EW11CombatOutcome::Pending, 12, 11));
	TestFalse(TEXT("callbacks are rejected after terminal outcome"),
		AW11GameState::IsPendingCombatEncounter(
			EW11RunPhase::Combat, EW11CombatOutcome::Victory, 12, 12));
	TestFalse(TEXT("callbacks are rejected outside combat"),
		AW11GameState::IsPendingCombatEncounter(
			EW11RunPhase::Results, EW11CombatOutcome::Pending, 12, 12));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11ProjectileSingleHitTest,
	"WorldWalker.W11.Combat.ProjectileSingleHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11ProjectileSingleHitTest::RunTest(const FString& Parameters)
{
	bool bResolved = false;
	TestTrue(TEXT("projectile claims its first valid hit"),
		AW11EnemyProjectile::TryClaimResolution(bResolved));
	TestTrue(TEXT("the claim latches projectile resolution"), bResolved);
	TestFalse(TEXT("projectile rejects every later hit"),
		AW11EnemyProjectile::TryClaimResolution(bResolved));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11EnemyTelegraphRecoveryTest,
	"WorldWalker.W11.Combat.EnemyTelegraphRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11EnemyTelegraphRecoveryTest::RunTest(const FString& Parameters)
{
	FW11ReplicatedAttackState Active;
	Active.State = EW11EnemyCombatState::Active;
	Active.TelegraphShape = EW11TelegraphShape::Line;
	Active.WindupStartServerTime = 1.0f;
	Active.ActiveStartServerTime = 2.0f;
	Active.RecoveryEndServerTime = 3.0f;
	const FW11ReplicatedAttackState Recovered = AW11Enemy::MakeInactiveAttackState(
		Active, EW11EnemyCombatState::AcquireTarget);
	TestEqual(TEXT("recovery returns to target acquisition"), Recovered.State, EW11EnemyCombatState::AcquireTarget);
	TestEqual(TEXT("recovery removes the telegraph"), Recovered.TelegraphShape, EW11TelegraphShape::None);
	TestEqual(TEXT("recovery clears windup time"), Recovered.WindupStartServerTime, 0.0f);
	TestEqual(TEXT("recovery clears active time"), Recovered.ActiveStartServerTime, 0.0f);
	TestEqual(TEXT("recovery clears recovery time"), Recovered.RecoveryEndServerTime, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11StageCleanupTest,
	"WorldWalker.W11.Combat.StageCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11StageCleanupTest::RunTest(const FString& Parameters)
{
	FW11ReplicatedAttackState Threat;
	Threat.EncounterInstanceId = 21;
	Threat.AttackInstanceId = 5;
	Threat.AttackDefinitionId = TEXT("EnemyAttack.Contract");
	Threat.LockedTargetStableId = 17;
	Threat.LockedDirection = FVector(0.0f, 1.0f, 0.0f);
	Threat.LockedTargetPoint = FVector(100.0f, 200.0f, 0.0f);
	Threat.TelegraphShape = EW11TelegraphShape::Circle;
	Threat.TelegraphRange = 450.0f;
	Threat.TelegraphRadius = 80.0f;
	const FW11ReplicatedAttackState Cleared = AW11Enemy::MakeInactiveAttackState(
		Threat, EW11EnemyCombatState::Defeated);
	TestEqual(TEXT("cleanup retains encounter identity for replication ordering"), Cleared.EncounterInstanceId, 21);
	TestEqual(TEXT("cleanup retains attack sequence identity"), Cleared.AttackInstanceId, 5);
	TestTrue(TEXT("cleanup removes attack definition"), Cleared.AttackDefinitionId.IsNone());
	TestEqual(TEXT("cleanup removes locked target"), Cleared.LockedTargetStableId, 0);
	TestTrue(TEXT("cleanup removes locked point"), FVector(Cleared.LockedTargetPoint).IsNearlyZero());
	TestEqual(TEXT("cleanup removes threat range"), Cleared.TelegraphRange, 0.0f);
	TestEqual(TEXT("cleanup removes threat radius"), Cleared.TelegraphRadius, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11DeterministicNameHashTest,
	"WorldWalker.W11.Unit.DeterministicNameHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11DeterministicNameHashTest::RunTest(const FString& Parameters)
{
	const FName EncounterId(TEXT("Encounter.Stage2"));
	const FName PurposeId(TEXT("CultivationOffer"));
	TestEqual(TEXT("encounter ids hash their stable text rather than runtime FName indices"),
		AW11RunDirector::GetStableNameHash(EncounterId), FCrc::StrCrc32(TEXT("Encounter.Stage2")));
	TestEqual(TEXT("random-purpose ids hash their stable text rather than runtime FName indices"),
		AW11RunDirector::GetStableNameHash(PurposeId), FCrc::StrCrc32(TEXT("CultivationOffer")));
	TestNotEqual(TEXT("different deterministic ids retain distinct hashes"),
		AW11RunDirector::GetStableNameHash(EncounterId), AW11RunDirector::GetStableNameHash(PurposeId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11HudIncomingDirectionTest,
	"WorldWalker.W11.Presentation.HudIncomingDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11HudIncomingDirectionTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("positive world X is above the fixed top-down camera"),
		UW11CombatHUDWidget::ResolveIncomingDirection(FVector(10.0f, 1.0f, 0.0f)),
		EW11FacingDirection::Up);
	TestEqual(TEXT("negative world X is below the fixed top-down camera"),
		UW11CombatHUDWidget::ResolveIncomingDirection(FVector(-10.0f, 1.0f, 0.0f)),
		EW11FacingDirection::Down);
	TestEqual(TEXT("positive world Y is right of the fixed top-down camera"),
		UW11CombatHUDWidget::ResolveIncomingDirection(FVector(1.0f, 10.0f, 0.0f)),
		EW11FacingDirection::Right);
	TestEqual(TEXT("negative world Y is left of the fixed top-down camera"),
		UW11CombatHUDWidget::ResolveIncomingDirection(FVector(1.0f, -10.0f, 0.0f)),
		EW11FacingDirection::Left);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11StatRuleTest,
	"WorldWalker.W11.Unit.StatRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11StatRuleTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("zero armor has no reduction"), UW11StatLibrary::GetArmorDamageReduction(0.0f), 0.0f);
	TestEqual(TEXT("100 armor halves damage"), UW11StatLibrary::GetArmorDamageReduction(100.0f), 0.5f);
	TestEqual(TEXT("armor reduction is capped"), UW11StatLibrary::GetArmorDamageReduction(10000.0f), 0.75f);
	TestEqual(TEXT("100 luck reaches fifty percent effective luck"), UW11StatLibrary::GetEffectiveLuck(100.0f), 0.5f);
	TestEqual(TEXT("double area yields sqrt radius"), UW11StatLibrary::GetFinalAreaRadius(100.0f, 4.0f), 200.0f);
	TestEqual(TEXT("one point haste halves cooldown"), UW11StatLibrary::GetFinalCooldown(10.0f, 1.0f), 5.0f);

	FW11StatBlock Stats;
	FW11StatModifier Health;
	Health.Stat = EW11StatType::MaxHealth;
	Health.Operation = EW11ModifierOperation::Add;
	Health.Magnitude = 25.0f;
	UW11StatLibrary::ApplyModifier(Stats, Health);
	TestEqual(TEXT("add modifier changes the selected stat"), Stats.MaxHealth, 125.0f);

	FW11StatModifier Area;
	Area.Stat = EW11StatType::Area;
	Area.Operation = EW11ModifierOperation::Multiply;
	Area.Magnitude = 1.5f;
	UW11StatLibrary::ApplyModifier(Stats, Area);
	TestEqual(TEXT("multiplicative area modifier composes"), Stats.AreaScale, 1.5f);

	FW11StatModifier InvalidSpeed;
	InvalidSpeed.Stat = EW11StatType::MoveSpeed;
	InvalidSpeed.Operation = EW11ModifierOperation::Override;
	InvalidSpeed.Magnitude = -4.0f;
	UW11StatLibrary::ApplyModifier(Stats, InvalidSpeed);
	TestEqual(TEXT("stat sanitation enforces movement floor"), Stats.MoveSpeedScale, 0.25f);

	const FW11DamageBreakdown Damage = UW11AttributeComponent::CalculateDamageBreakdown(
		100.0f, 100.0f, 20.0f, 100.0f, false);
	TestEqual(TEXT("damage breakdown records armor mitigation"), Damage.ArmorMitigated, 50.0f);
	TestEqual(TEXT("damage breakdown records barrier absorption"), Damage.BarrierAbsorbed, 20.0f);
	TestEqual(TEXT("damage breakdown records actual health damage"), Damage.HealthDamage, 30.0f);
	TestFalse(TEXT("surviving detailed damage is not defeated"), Damage.bDefeated);
	const FW11DamageBreakdown Immune = UW11AttributeComponent::CalculateDamageBreakdown(
		100.0f, 0.0f, 0.0f, 100.0f, true);
	TestTrue(TEXT("immunity is explicit in the damage result"), Immune.bImmune);
	TestEqual(TEXT("immune damage changes no health"), Immune.HealthDamage, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11CombatRequestSequenceTest,
	"WorldWalker.W11.Combat.ServerRequestValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11CombatRequestSequenceTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("zero is never a valid client request sequence"),
		UW11CombatComponent::IsNewerRequestSequence(0, 0));
	TestTrue(TEXT("first positive sequence is accepted"),
		UW11CombatComponent::IsNewerRequestSequence(1, 0));
	TestFalse(TEXT("duplicate sequence is rejected"),
		UW11CombatComponent::IsNewerRequestSequence(42, 42));
	TestFalse(TEXT("backwards sequence is rejected"),
		UW11CombatComponent::IsNewerRequestSequence(41, 42));
	TestTrue(TEXT("positive sequence increments are accepted"),
		UW11CombatComponent::IsNewerRequestSequence(43, 42));
	TestTrue(TEXT("MAX_int32 wraps explicitly to one"),
		UW11CombatComponent::IsNewerRequestSequence(1, MAX_int32));
	FW11AbilityRequest ValidRequest;
	ValidRequest.Slot = EW11AbilitySlot::Active4;
	ValidRequest.InputDirection = FVector2D(0.0f, 1.0f);
	ValidRequest.TargetPoint = FVector(100.0f, 20.0f, 0.0f);
	TestTrue(TEXT("finite normalized requests in a logical slot are accepted"),
		UW11CombatComponent::IsValidRequestInput(ValidRequest));
	FW11AbilityRequest BadSlot = ValidRequest;
	BadSlot.Slot = static_cast<EW11AbilitySlot>(255);
	TestFalse(TEXT("out-of-range logical slots are rejected"),
		UW11CombatComponent::IsValidRequestInput(BadSlot));
	FW11AbilityRequest BadDirection = ValidRequest;
	BadDirection.InputDirection = FVector2D(0.0f, 0.0f);
	TestFalse(TEXT("non-normalized directions are rejected"),
		UW11CombatComponent::IsValidRequestInput(BadDirection));
	FW11AbilityRequest NonFinite = ValidRequest;
	NonFinite.TargetPoint.X = std::numeric_limits<float>::infinity();
	TestFalse(TEXT("non-finite target points are rejected"),
		UW11CombatComponent::IsValidRequestInput(NonFinite));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11BasicAttackDefinitionTest,
	"WorldWalker.W11.Combat.BasicAttackDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11BasicAttackDefinitionTest::RunTest(const FString& Parameters)
{
	const UW11AbilityDefinition* BasicAttack = LoadObject<UW11AbilityDefinition>(nullptr,
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Abilities/DA_Ability_BasicAttack.DA_Ability_BasicAttack"));
	TestNotNull(TEXT("shared basic attack definition loads"), BasicAttack);
	if (BasicAttack)
	{
		TestFalse(TEXT("basic attack owns a stable definition id"), BasicAttack->DefinitionId.IsNone());
		TestEqual(TEXT("basic attack owns its logical slot"), BasicAttack->LogicalSlot, EW11AbilitySlot::BasicAttack);
		TestEqual(TEXT("basic attack owns attack-speed cooldown policy"),
			BasicAttack->CooldownScaling, EW11CooldownScaling::AttackSpeed);
		TestTrue(TEXT("basic attack owns damage instead of component constants"), BasicAttack->BaseDamage > 0.0f);
		TestTrue(TEXT("basic attack owns target geometry"),
			BasicAttack->BaseRange > 0.0f && BasicAttack->BaseRadius > 0.0f);
	}
	TestTrue(TEXT("capsule-edge basic attack target is retained"),
		UW11CombatComponent::IsBasicAttackTargetInForgivingSweep(
			FVector::ZeroVector, FVector::ForwardVector, FVector(250.0f, 135.0f, 500.0f),
			280.0f, 80.0f, 34.0f));
	TestFalse(TEXT("basic attack cannot hit a target behind the player"),
		UW11CombatComponent::IsBasicAttackTargetInForgivingSweep(
			FVector::ZeroVector, FVector::ForwardVector, FVector(-10.0f, 0.0f, 0.0f),
			280.0f, 80.0f, 34.0f));
	TestFalse(TEXT("basic attack cannot hit beyond authored reach"),
		UW11CombatComponent::IsBasicAttackTargetInForgivingSweep(
			FVector::ZeroVector, FVector::ForwardVector, FVector(360.0f, 0.0f, 0.0f),
			280.0f, 80.0f, 34.0f));
	TestEqual(TEXT("first combat camera is expanded"),
		AW11Character::ResolveArenaCameraWidth(1, 1), 3000.0f);
	TestEqual(TEXT("later chapter camera keeps the established width"),
		AW11Character::ResolveArenaCameraWidth(2, 1), 2200.0f);
	TestEqual(TEXT("first combat uses the expanded arena boundary"),
		FW11ArenaBounds::ResolveHalfExtent(1, 1), 1500.0f);
	TestEqual(TEXT("later chapters keep their authored arena boundary"),
		FW11ArenaBounds::ResolveHalfExtent(2, 1), 950.0f);
	const FVector ClampedArenaLocation = FW11ArenaBounds::ClampLocation(
		FVector(2000.0f, -2000.0f, 88.0f), 42.0f, 1500.0f);
	TestEqual(TEXT("arena boundary accounts for capsule radius on X"), ClampedArenaLocation.X, 1458.0);
	TestEqual(TEXT("arena boundary accounts for capsule radius on Y"), ClampedArenaLocation.Y, -1458.0);
	TestEqual(TEXT("arena boundary preserves the movement plane height"), ClampedArenaLocation.Z, 88.0);

	FString InputConfig;
	TestTrue(TEXT("DefaultInput.ini loads"), FFileHelper::LoadFileToString(
		InputConfig, *(FPaths::ProjectConfigDir() / TEXT("DefaultInput.ini"))));
	TestTrue(TEXT("F is bound to W11 basic attack"),
		InputConfig.Contains(TEXT("ActionName=\"W11BasicAttack\",bShift=False,bCtrl=False,bAlt=False,bCmd=False,Key=F")));
	const TCHAR* ActiveBindings[] = {
		TEXT("ActionName=\"W11Active1\",bShift=False,bCtrl=False,bAlt=False,bCmd=False,Key=Q"),
		TEXT("ActionName=\"W11Active2\",bShift=False,bCtrl=False,bAlt=False,bCmd=False,Key=W"),
		TEXT("ActionName=\"W11Active3\",bShift=False,bCtrl=False,bAlt=False,bCmd=False,Key=E"),
		TEXT("ActionName=\"W11Active4\",bShift=False,bCtrl=False,bAlt=False,bCmd=False,Key=R"),
	};
	for (const TCHAR* Binding : ActiveBindings)
	{
		TestTrue(TEXT("Q/W/E/R active binding exists"), InputConfig.Contains(Binding));
	}
	TestFalse(TEXT("legacy IJKL W11 attack bindings are removed"),
		InputConfig.Contains(TEXT("ActionName=\"W11AttackUp\""))
		|| InputConfig.Contains(TEXT("ActionName=\"W11AttackDown\""))
		|| InputConfig.Contains(TEXT("ActionName=\"W11AttackLeft\""))
		|| InputConfig.Contains(TEXT("ActionName=\"W11AttackRight\"")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11EnemyAttackContractTest,
	"WorldWalker.W11.Combat.EnemyTelegraphLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11EnemyAttackContractTest::RunTest(const FString& Parameters)
{
	struct FExpectedEnemyAttack
	{
		const TCHAR* EnemyPath;
		EW11TelegraphShape Telegraph;
		EW11HitDelivery Delivery;
		EW11HitShape HitShape;
	};
	const FExpectedEnemyAttack Expected[] = {
		{ TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Enemies/DA_Enemy_MiasmaWisp.DA_Enemy_MiasmaWisp"),
			EW11TelegraphShape::ProjectilePath, EW11HitDelivery::Projectile, EW11HitShape::Projectile },
		{ TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Enemies/DA_Enemy_SwordWraith.DA_Enemy_SwordWraith"),
			EW11TelegraphShape::Line, EW11HitDelivery::Immediate, EW11HitShape::Line },
		{ TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Enemies/DA_Enemy_StoneFiend.DA_Enemy_StoneFiend"),
			EW11TelegraphShape::Circle, EW11HitDelivery::Immediate, EW11HitShape::Circle },
	};
	TSet<FName> AbilityIds;
	for (const FExpectedEnemyAttack& Contract : Expected)
	{
		const UW11EnemyDefinition* Enemy = LoadObject<UW11EnemyDefinition>(nullptr, Contract.EnemyPath);
		TestNotNull(TEXT("enemy definition loads"), Enemy);
		if (!Enemy)
		{
			continue;
		}
		TestEqual(TEXT("enemy owns exactly one primary attack"), Enemy->Abilities.Num(), 1);
		const UW11AbilityDefinition* Attack = Enemy->Abilities.IsEmpty()
			? nullptr : Enemy->Abilities[0].LoadSynchronous();
		TestNotNull(TEXT("enemy primary attack loads"), Attack);
		if (!Attack)
		{
			continue;
		}
		AbilityIds.Add(Attack->DefinitionId);
		TestEqual(TEXT("enemy telegraph shape matches its role"), Attack->TelegraphShape, Contract.Telegraph);
		TestEqual(TEXT("enemy delivery matches its role"), Attack->Delivery, Contract.Delivery);
		TestEqual(TEXT("enemy hit shape matches its role"), Attack->HitShape, Contract.HitShape);
		TestTrue(TEXT("enemy attack has a visible windup"), Attack->WindupSeconds >= 0.45f);
		TestTrue(TEXT("enemy attack has a meaningful recovery"), Attack->RecoverySeconds >= 1.0f);
		TestTrue(TEXT("enemy attack has authored geometry"),
			Attack->BaseRange > 0.0f && Attack->BaseRadius > 0.0f);
	}
	TestEqual(TEXT("three enemy roles use distinct attack definitions"), AbilityIds.Num(), 3);

	FW11ReplicatedAttackState Snapshot;
	Snapshot.EncounterInstanceId = 7;
	Snapshot.AttackInstanceId = 3;
	Snapshot.State = EW11EnemyCombatState::Windup;
	Snapshot.WindupStartServerTime = 10.0f;
	Snapshot.ActiveStartServerTime = 10.7f;
	Snapshot.RecoveryEndServerTime = 12.0f;
	TestTrue(TEXT("replicated attack timestamps are monotonic"),
		Snapshot.WindupStartServerTime < Snapshot.ActiveStartServerTime
		&& Snapshot.ActiveStartServerTime < Snapshot.RecoveryEndServerTime);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11DefinitionIdentityTest,
	"WorldWalker.W11.Unit.DefinitionIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11DefinitionIdentityTest::RunTest(const FString& Parameters)
{
	UW11ManualDefinition* Manual = NewObject<UW11ManualDefinition>();
	Manual->DefinitionId = TEXT("Manual.TestSword");
	TestEqual(TEXT("manual primary type"), Manual->GetPrimaryAssetId().PrimaryAssetType, FPrimaryAssetType(TEXT("W11Manual")));
	TestEqual(TEXT("manual stable asset name"), Manual->GetPrimaryAssetId().PrimaryAssetName, FName(TEXT("Manual.TestSword")));

	UW11TreasureDefinition* Treasure = NewObject<UW11TreasureDefinition>();
	Treasure->DefinitionId = TEXT("Treasure.TestBell");
	TestEqual(TEXT("treasure primary type"), Treasure->GetPrimaryAssetId().PrimaryAssetType, FPrimaryAssetType(TEXT("W11Treasure")));

	UW11RunRuleSet* Rules = NewObject<UW11RunRuleSet>();
	Rules->DefinitionId = TEXT("RunRules.Test");
	TestEqual(TEXT("rules primary type"), Rules->GetPrimaryAssetId().PrimaryAssetType, FPrimaryAssetType(TEXT("W11RunRuleSet")));

	UW11AbilityDefinition* Ability = NewObject<UW11AbilityDefinition>();
	Ability->DefinitionId = TEXT("Ability.TestBasic");
	TestEqual(TEXT("ability primary type"), Ability->GetPrimaryAssetId().PrimaryAssetType,
		FPrimaryAssetType(TEXT("W11Ability")));

	UW11HeroDefinition* Hero = NewObject<UW11HeroDefinition>();
	Hero->DefinitionId = TEXT("Hero.Test");
	TestEqual(TEXT("hero primary type"), Hero->GetPrimaryAssetId().PrimaryAssetType, FPrimaryAssetType(TEXT("W11Hero")));

	UW11SectDefinition* Sect = NewObject<UW11SectDefinition>();
	Sect->DefinitionId = TEXT("Sect.Test");
	TestEqual(TEXT("sect primary type"), Sect->GetPrimaryAssetId().PrimaryAssetType, FPrimaryAssetType(TEXT("W11Sect")));

	UW11CharacterAnimationSet* CharacterAnimation = NewObject<UW11CharacterAnimationSet>();
	CharacterAnimation->DefinitionId = TEXT("CharacterAnimation.Hero.Test");
	TestEqual(TEXT("character animation primary type"), CharacterAnimation->GetPrimaryAssetId().PrimaryAssetType,
		FPrimaryAssetType(TEXT("W11CharacterAnimation")));

	UW11SkillVisualDefinition* SkillVisual = NewObject<UW11SkillVisualDefinition>();
	SkillVisual->DefinitionId = TEXT("SkillVisual.Test");
	TestEqual(TEXT("skill visual primary type"), SkillVisual->GetPrimaryAssetId().PrimaryAssetType,
		FPrimaryAssetType(TEXT("W11SkillVisual")));

	UW11ChapterDefinition* Chapter = NewObject<UW11ChapterDefinition>();
	Chapter->DefinitionId = TEXT("Chapter.Prologue");
	TestEqual(TEXT("chapter primary type"), Chapter->GetPrimaryAssetId().PrimaryAssetType,
		FPrimaryAssetType(TEXT("W11Chapter")));
	UW11ChapterRouteDefinition* ChapterRoute = NewObject<UW11ChapterRouteDefinition>();
	ChapterRoute->DefinitionId = TEXT("ChapterRoute.Standard");
	TestEqual(TEXT("chapter route primary type"), ChapterRoute->GetPrimaryAssetId().PrimaryAssetType,
		FPrimaryAssetType(TEXT("W11ChapterRoute")));
	FW11ChapterRouteNode RouteNode;
	RouteNode.NodeId = TEXT("Node.Prologue.Battle1");
	RouteNode.NodeType = EW11ChapterNodeType::Combat;
	TestFalse(TEXT("chapter route nodes carry stable ids"), RouteNode.NodeId.IsNone());
	for (int32 ChapterIndex = 0; ChapterIndex < 5; ++ChapterIndex)
	{
		UW11ChapterDefinition* RouteChapter = NewObject<UW11ChapterDefinition>(ChapterRoute);
		RouteChapter->DefinitionId = FName(*FString::Printf(TEXT("Chapter.%d"), ChapterIndex + 1));
		RouteChapter->ChapterIndex = ChapterIndex + 1;
		FW11ChapterRouteNode& BossNode = RouteChapter->Nodes.AddDefaulted_GetRef();
		BossNode.NodeId = FName(*FString::Printf(TEXT("Node.%d.Boss"), ChapterIndex + 1));
		BossNode.NodeType = EW11ChapterNodeType::SmallBoss;
		ChapterRoute->Chapters.Add(RouteChapter);
	}
	UW11EncounterDefinition* FinalEncounter = NewObject<UW11EncounterDefinition>(ChapterRoute);
	FinalEncounter->DefinitionId = TEXT("Encounter.Final.GuChangyuan");
	ChapterRoute->FinalBossEncounter = FinalEncounter;
	FW11ChapterRouteCursor Cursor;
	TestTrue(TEXT("initial five-chapter cursor is valid"),
		UW11ChapterRouteSubsystem::IsCursorValid(ChapterRoute, Cursor));
	for (int32 CompletedChapter = 0; CompletedChapter < 5; ++CompletedChapter)
	{
		TestTrue(TEXT("chapter cursor advances"),
			UW11ChapterRouteSubsystem::AdvanceCursor(ChapterRoute, Cursor));
	}
	TestTrue(TEXT("five small bosses advance to the final boss"), Cursor.bFinalBoss && !Cursor.bComplete);
	TestEqual(TEXT("final boss cursor sits after all chapter indices"), Cursor.ChapterIndex, 5);
	TestTrue(TEXT("final boss completion advances the route"),
		UW11ChapterRouteSubsystem::AdvanceCursor(ChapterRoute, Cursor));
	TestTrue(TEXT("route completes only after the final boss"), Cursor.bComplete);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11GeneratedContentContractTest,
	"WorldWalker.W11.Combat.ContentContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11GeneratedContentContractTest::RunTest(const FString& Parameters)
{
	const UMaterialInterface* TelegraphMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Materials/M_W11_EnemyTelegraph.M_W11_EnemyTelegraph"));
	TestNotNull(TEXT("enemy ground warnings use a dedicated readable telegraph material"), TelegraphMaterial);

	const UObject* WorldDefinition = LoadObject<UObject>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/DA_W11_RogueSurvival.DA_W11_RogueSurvival"));
	TestNotNull(TEXT("W11 registers a discoverable WorldDefinition"), WorldDefinition);

	const UTexture2D* ArenaTexture = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Prototype/T_W11_FirstArena_Continuous.T_W11_FirstArena_Continuous"));
	TestNotNull(TEXT("the active arena texture loads"), ArenaTexture);
	if (ArenaTexture)
	{
		const FIntPoint ImportedSize = ArenaTexture->GetImportedSize();
		TestEqual(TEXT("the active arena master is 8K wide"), ImportedSize.X, 8192);
		TestEqual(TEXT("the active arena master is 8K high"), ImportedSize.Y, 8192);
	}
	const UTexture2D* ArenaTileTexture = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Prototype/T_W11_Arena_JadeTile.T_W11_Arena_JadeTile"));
	TestNotNull(TEXT("the high-density arena tile loads"), ArenaTileTexture);
	if (ArenaTileTexture)
	{
		const FIntPoint ImportedSize = ArenaTileTexture->GetImportedSize();
		TestEqual(TEXT("the high-density arena tile preserves source width"), ImportedSize.X, 1254);
		TestEqual(TEXT("the high-density arena tile preserves source height"), ImportedSize.Y, 1254);
	}

	FString EngineConfig;
	TestTrue(TEXT("default engine config loads"), FFileHelper::LoadFileToString(
		EngineConfig, *FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultEngine.ini"))));
	TestTrue(TEXT("native-resolution launches retain a 100 percent render scale"),
		EngineConfig.Contains(TEXT("r.ScreenPercentage=100")));
	TestTrue(TEXT("Paper2D presentation disables motion blur"),
		EngineConfig.Contains(TEXT("r.DefaultFeature.MotionBlur=False"))
		&& EngineConfig.Contains(TEXT("r.MotionBlurQuality=0")));

	const UW11RunRuleSet* Rules = LoadObject<UW11RunRuleSet>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/DA_W11_RunRules.DA_W11_RunRules"));
	TestNotNull(TEXT("W11 standard run rules exist"), Rules);
	if (Rules)
	{
		TestEqual(TEXT("co-op cap is four players"), Rules->MaxPlayers, 4);
		TestEqual(TEXT("cultivation presents three choices"), Rules->CultivationOfferCount, 3);
		TestTrue(TEXT("rules include a meaningful stat pool"), Rules->StatChoices.Num() >= 10);
		TestTrue(TEXT("rules include Xianxia manual schools"), Rules->Manuals.Num() >= 9);
		TestTrue(TEXT("rules include treasures"), Rules->Treasures.Num() >= 6);
		TestEqual(TEXT("rules include the five M6 encounter archetypes"), Rules->Encounters.Num(), 5);
		TestEqual(TEXT("rules expose six active abilities"), Rules->ActiveAbilities.Num(), 6);
		TestEqual(TEXT("market reserves one ability offer"), Rules->AbilityShopSlots, 1);
		TestEqual(TEXT("rules expose exactly ten protagonists"), Rules->Heroes.Num(), 10);
		TestEqual(TEXT("rules expose six initial sects"), Rules->Sects.Num(), 6);
		TestFalse(TEXT("rules freeze a combat rule version"), Rules->RuleVersion.IsEmpty());
		TestEqual(TEXT("combat logs use the recovery-punish schema"), Rules->CombatLogSchemaVersion, 3);
		TestTrue(TEXT("run rules own positive dodge distance"), Rules->DodgeDistance > 0.0f);
		TestTrue(TEXT("run rules own positive dodge duration"), Rules->DodgeDuration > 0.0f);
		TestTrue(TEXT("run rules own positive dodge cooldown"), Rules->DodgeCooldown > 0.0f);
		TestTrue(TEXT("dodge immunity fits within its cooldown"),
			Rules->DodgeInvulnerabilityDuration >= 0.0f
			&& Rules->DodgeInvulnerabilityDuration <= Rules->DodgeCooldown);
		const UW11ChapterRouteDefinition* FormalRoute = Rules->ChapterRoute.LoadSynchronous();
		TestNotNull(TEXT("run rules connect the formal five-chapter route"), FormalRoute);
		if (FormalRoute)
		{
			TestEqual(TEXT("formal route contains exactly five chapter gates"),
				FormalRoute->Chapters.Num(), 5);
			TSet<FSoftObjectPath> ChapterMaps;
			TSet<FName> SmallBossIds;
			int32 MarketNodeCount = 0;
			int32 SafeNodeCount = 0;
			for (int32 ChapterIndex = 0; ChapterIndex < FormalRoute->Chapters.Num(); ++ChapterIndex)
			{
				const UW11ChapterDefinition* FormalChapter =
					FormalRoute->Chapters[ChapterIndex].LoadSynchronous();
				TestNotNull(TEXT("formal chapter definition loads"), FormalChapter);
				if (!FormalChapter)
				{
					continue;
				}
				TestEqual(TEXT("chapter indices are authored one-based and ordered"),
					FormalChapter->ChapterIndex, ChapterIndex + 1);
				TestFalse(TEXT("formal chapter owns an explicit map"), FormalChapter->ChapterMap.IsNull());
				TestNotNull(TEXT("formal chapter map loads"), FormalChapter->ChapterMap.LoadSynchronous());
				ChapterMaps.Add(FormalChapter->ChapterMap.ToSoftObjectPath());
				TestTrue(TEXT("formal chapter has a route ending in a boss"),
					!FormalChapter->Nodes.IsEmpty()
					&& FormalChapter->Nodes.Last().NodeType == EW11ChapterNodeType::SmallBoss);
				for (const FW11ChapterRouteNode& Node : FormalChapter->Nodes)
				{
					MarketNodeCount += Node.NodeType == EW11ChapterNodeType::ImmortalMarket ? 1 : 0;
					SafeNodeCount += Node.bSafeNode ? 1 : 0;
					if (Node.NodeType == EW11ChapterNodeType::SmallBoss)
					{
						const UW11EncounterDefinition* BossEncounter = Node.Encounter.LoadSynchronous();
						TestNotNull(TEXT("chapter boss encounter loads"), BossEncounter);
						if (BossEncounter && BossEncounter->EnemyPool.Num() == 1)
						{
							const UW11EnemyDefinition* FormalBoss =
								BossEncounter->EnemyPool[0].Enemy.LoadSynchronous();
							TestNotNull(TEXT("chapter boss enemy loads"), FormalBoss);
							if (FormalBoss)
							{
								SmallBossIds.Add(FormalBoss->DefinitionId);
								TestTrue(TEXT("chapter boss uses the formal multi-phase contract"),
									FormalBoss->bBoss && FormalBoss->bFormalBoss
									&& FormalBoss->BossPhases.Num() >= 2);
								for (const FW11BossPhaseDefinition& Phase : FormalBoss->BossPhases)
								{
									TestFalse(TEXT("boss phase has a mechanic identity"), Phase.MechanicId.IsNone());
									TestTrue(TEXT("boss phase changes its attack mechanics"), !Phase.Abilities.IsEmpty());
								}
							}
						}
					}
				}
			}
			TestEqual(TEXT("formal route references five unique maps"), ChapterMaps.Num(), 5);
			TestEqual(TEXT("formal route references five unique small bosses"), SmallBossIds.Num(), 5);
			TestEqual(TEXT("each chapter contains one map-owned immortal market node"), MarketNodeCount, 5);
			TestTrue(TEXT("route authors multiple safe resume nodes per chapter"), SafeNodeCount >= 10);
			const UW11EncounterDefinition* FinalBossEncounter =
				FormalRoute->FinalBossEncounter.LoadSynchronous();
			TestNotNull(TEXT("formal route connects the final boss encounter"), FinalBossEncounter);
			if (FinalBossEncounter && FinalBossEncounter->EnemyPool.Num() == 1)
			{
				const UW11EnemyDefinition* GuChangyuan =
					FinalBossEncounter->EnemyPool[0].Enemy.LoadSynchronous();
				TestNotNull(TEXT("Gu Changyuan final boss loads"), GuChangyuan);
				if (GuChangyuan)
				{
					TestEqual(TEXT("final boss has the confirmed Gu Changyuan identity"),
						GuChangyuan->DefinitionId, FName(TEXT("Boss.GuChangyuan")));
					TestEqual(TEXT("Gu Changyuan has two transitions for three phases"),
						GuChangyuan->BossPhases.Num(), 2);
					TestEqual(TEXT("Gu Changyuan begins as the demon-slaying hero"),
						GuChangyuan->InitialBossPhaseId,
						FName(TEXT("Phase.Gu.DemonSlayingHero")));
					TestEqual(TEXT("Gu Changyuan ends as the wishless dragon"),
						GuChangyuan->BossPhases.Last().PhaseId,
						FName(TEXT("Phase.Gu.WishlessDragon")));
				}
			}
		}
		TSet<EW11EncounterType> EncounterTypes;
		bool bFoundStageOne = false;
		for (const TSoftObjectPtr<UW11EncounterDefinition>& EncounterAsset : Rules->Encounters)
		{
			const UW11EncounterDefinition* Encounter = EncounterAsset.LoadSynchronous();
			TestNotNull(TEXT("encounter definition loads"), Encounter);
			if (!Encounter)
			{
				continue;
			}
			EncounterTypes.Add(Encounter->EncounterType);
			TestTrue(TEXT("encounter has a finite concurrency cap"), Encounter->MaxConcurrentEnemies > 0);
			TestTrue(TEXT("encounter has a positive baseline enemy health scale"),
				Encounter->EnemyBaseHealthScale > 0.0f);
			TestTrue(TEXT("encounter has a non-negative baseline enemy power scale"),
				Encounter->EnemyBasePowerScale >= 0.0f);
			TestTrue(TEXT("spawn annulus preserves a player safe radius"),
				Encounter->SpawnOuterRadius > Encounter->SpawnSafeRadius);
			TestTrue(TEXT("encounter has at least one wave"), Encounter->WaveCount > 0);
			if (Encounter->StageIndex == 1)
			{
				bFoundStageOne = true;
				TestTrue(TEXT("first combat trial authors a 45 to 90 second pacing target"),
					Encounter->DurationSeconds >= 45.0f && Encounter->DurationSeconds <= 90.0f);
				TestEqual(TEXT("first combat trial increases simultaneous enemies"),
					Encounter->MaxConcurrentEnemies, 5);
				TestEqual(TEXT("first combat trial doubles the encounter budget"),
					Encounter->SpawnBudget, 22);
				TestEqual(TEXT("first combat trial uses the expanded safe spawn radius"),
					Encounter->SpawnSafeRadius, 650.0f);
				TestEqual(TEXT("first combat trial uses the expanded outer spawn radius"),
					Encounter->SpawnOuterRadius, 1250.0f);
				TestEqual(TEXT("first combat trial lowers individual enemy durability"),
					Encounter->EnemyBaseHealthScale, 4.5f);
				TestEqual(TEXT("first combat trial offsets five-enemy pressure"),
					Encounter->EnemyBasePowerScale, 0.18f);
				TestTrue(TEXT("first combat trial separates lethality from durability"),
					Encounter->EnemyBasePowerScale > 0.0f && Encounter->EnemyBasePowerScale < 1.0f);
			}
		}
		TestTrue(TEXT("rules include the first combat trial"), bFoundStageOne);
		TestEqual(TEXT("clear waves survival elite and boss types are all authored"), EncounterTypes.Num(), 5);
		const UW11AbilityDefinition* BasicAttack = Rules->BasicAttackAbility.LoadSynchronous();
		TestNotNull(TEXT("rules provide a shared mandatory basic attack"), BasicAttack);
		if (BasicAttack)
		{
			TestEqual(TEXT("basic attack uses the non-active logical slot"),
				BasicAttack->LogicalSlot, EW11AbilitySlot::BasicAttack);
			TestEqual(TEXT("basic attack cooldown scales only with attack speed"),
				BasicAttack->CooldownScaling, EW11CooldownScaling::AttackSpeed);
			TestEqual(TEXT("basic attack crit is rolled once per execution"),
				BasicAttack->CriticalRollPolicy, EW11CriticalRollPolicy::PerExecution);
			TestEqual(TEXT("basic attack costs no mana"), BasicAttack->ManaCost, 0.0f);
			TestTrue(TEXT("basic attack has authored damage"), BasicAttack->BaseDamage > 0.0f);
		}
		TSet<FName> HeroIds;
		int32 MaleCount = 0;
		int32 FemaleCount = 0;
		for (const TSoftObjectPtr<UW11HeroDefinition>& HeroAsset : Rules->Heroes)
		{
			const UW11HeroDefinition* Hero = HeroAsset.LoadSynchronous();
			TestNotNull(TEXT("hero definition loads"), Hero);
			if (Hero)
			{
				HeroIds.Add(Hero->DefinitionId);
				MaleCount += Hero->Gender == EW11HeroGender::Male ? 1 : 0;
				FemaleCount += Hero->Gender == EW11HeroGender::Female ? 1 : 0;
				TestTrue(TEXT("hero has starting modifiers"), Hero->StartingModifiers.Num() >= 2);
				TestNotNull(TEXT("hero portrait loads"), Hero->Portrait.LoadSynchronous());
				TestNotNull(TEXT("hero avatar loads"), Hero->Avatar.LoadSynchronous());
				const UW11CharacterAnimationSet* AnimationSet = Hero->CombatAnimations.LoadSynchronous();
				TestNotNull(TEXT("hero has a separate combat animation set"), AnimationSet);
				if (AnimationSet)
				{
					TestNotNull(TEXT("hero right idle flipbook loads"), AnimationSet->Right.Idle.LoadSynchronous());
					TestNotNull(TEXT("hero right move flipbook loads"), AnimationSet->Right.Move.LoadSynchronous());
					TestNotNull(TEXT("hero right cast flipbook loads"), AnimationSet->Right.Cast.LoadSynchronous());
					TestNotNull(TEXT("hero left hit flipbook loads"), AnimationSet->Left.Hit.LoadSynchronous());
				}
			}
		}
		TestEqual(TEXT("hero ids are unique"), HeroIds.Num(), 10);
		TestEqual(TEXT("five male protagonists"), MaleCount, 5);
		TestEqual(TEXT("five female protagonists"), FemaleCount, 5);
		TSet<FName> InitialAbilityIds;
		for (const TSoftObjectPtr<UW11SectDefinition>& SectAsset : Rules->Sects)
		{
			const UW11SectDefinition* Sect = SectAsset.LoadSynchronous();
			TestNotNull(TEXT("sect definition loads"), Sect);
			if (Sect)
			{
				TestFalse(TEXT("sect exposes a selection style tagline"), Sect->StyleTagline.IsEmpty());
				const UTexture2D* SelectionArtwork = Sect->SelectionArtwork.LoadSynchronous();
				TestNotNull(TEXT("sect selection artwork loads"), SelectionArtwork);
				if (SelectionArtwork)
				{
					const FIntPoint ImportedSize = SelectionArtwork->GetImportedSize();
					TestEqual(TEXT("sect artwork matches the frozen 4K production width"),
						ImportedSize.X, 3840);
					TestEqual(TEXT("sect artwork matches the frozen 4K production height"),
						ImportedSize.Y, 2160);
				}
				const UW11AbilityDefinition* Ability = Sect->InitialAbility.LoadSynchronous();
				TestNotNull(TEXT("sect grants an initial ability"), Ability);
				if (Ability)
				{
					InitialAbilityIds.Add(Ability->DefinitionId);
					TestEqual(TEXT("sect ability occupies logical active slot one"),
						Ability->LogicalSlot, EW11AbilitySlot::Active1);
					TestEqual(TEXT("sect cooldown uses ability haste only"),
						Ability->CooldownScaling, EW11CooldownScaling::AbilityHaste);
					const UW11SkillVisualDefinition* Visuals = Ability->Visuals.LoadSynchronous();
					TestNotNull(TEXT("ability has character-independent visuals"), Visuals);
					if (Visuals)
					{
						TestNotNull(TEXT("ability visual effect flipbook loads"), Visuals->EffectFlipbook.LoadSynchronous());
					}
				}
			}
		}
		TestEqual(TEXT("each sect has a distinct initial ability"), InitialAbilityIds.Num(), 6);
		for (const TSoftObjectPtr<UW11AbilityDefinition>& AbilityAsset : Rules->ActiveAbilities)
		{
			const UW11AbilityDefinition* Ability = AbilityAsset.LoadSynchronous();
			TestNotNull(TEXT("active ability pool entry loads"), Ability);
			if (Ability)
			{
				TestTrue(TEXT("active ability has a stable id"), !Ability->DefinitionId.IsNone());
			}
		}
		const UW11AbilityDefinition* MiasmaBolt = LoadObject<UW11AbilityDefinition>(nullptr,
			TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Abilities/Enemies/DA_Ability_MiasmaBolt.DA_Ability_MiasmaBolt"));
		TestNotNull(TEXT("miasma projectile ability loads"), MiasmaBolt);
		if (MiasmaBolt)
		{
			TestEqual(TEXT("miasma has one status payload"), MiasmaBolt->StatusEffects.Num(), 1);
			if (!MiasmaBolt->StatusEffects.IsEmpty())
			{
				TestEqual(TEXT("miasma applies slow"), MiasmaBolt->StatusEffects[0].Type, EW11StatusType::Slow);
				TestTrue(TEXT("slow magnitude reduces movement"), MiasmaBolt->StatusEffects[0].Magnitude < 1.0f);
			}
		}
		const UW11ManualDefinition* ThunderManual = LoadObject<UW11ManualDefinition>(nullptr,
			TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Manuals/DA_Manual_ThunderSpell.DA_Manual_ThunderSpell"));
		TestNotNull(TEXT("thunder manual loads"), ThunderManual);
		if (ThunderManual && !ThunderManual->RankEffects.IsEmpty())
		{
			TestTrue(TEXT("thunder manual grants the active burn behavior"),
				ThunderManual->RankEffects[0].GrantedBehaviors.Contains(TEXT("ManualBehavior.ActiveBurn")));
		}
		const UW11EnemyDefinition* Elite = LoadObject<UW11EnemyDefinition>(nullptr,
			TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Enemies/DA_Enemy_MiasmaSwordElite.DA_Enemy_MiasmaSwordElite"));
		TestNotNull(TEXT("mechanic-combination elite loads"), Elite);
		if (Elite)
		{
			TestTrue(TEXT("elite flag is authored"), Elite->bElite);
			TestFalse(TEXT("elite is not mislabeled as boss"), Elite->bBoss);
			TestTrue(TEXT("elite combines at least three attack mechanics"), Elite->Abilities.Num() >= 3);
		}
		const UW11EnemyDefinition* Boss = LoadObject<UW11EnemyDefinition>(nullptr,
			TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Enemies/DA_Enemy_VoidWardenBoss.DA_Enemy_VoidWardenBoss"));
		TestNotNull(TEXT("boss contract enemy loads"), Boss);
		if (Boss)
		{
			TestTrue(TEXT("boss flag is authored"), Boss->bBoss);
			TestTrue(TEXT("boss combines reusable attack mechanics"), Boss->Abilities.Num() >= 3);
		}
		const UW11SkillVisualDefinition* BasicVisual = BasicAttack ? BasicAttack->Visuals.LoadSynchronous() : nullptr;
		TestNotNull(TEXT("basic attack shared visual loads"), BasicVisual);
		if (BasicVisual)
		{
			TestNotNull(TEXT("basic attack shared sound loads"), BasicVisual->CastSound.LoadSynchronous());
			TestNotNull(TEXT("shared impact sound loads"), BasicVisual->ImpactSound.LoadSynchronous());
			TestNotNull(TEXT("shared critical sound loads"), BasicVisual->CriticalSound.LoadSynchronous());
			TestNotNull(TEXT("shared defeat sound loads"), BasicVisual->DefeatSound.LoadSynchronous());
			TestTrue(TEXT("shared hit visual authors a local hit-stop tier"),
				BasicVisual->HitStopSeconds > 0.0f);
			TestTrue(TEXT("shared hit visual tiers critical feedback above normal"),
				BasicVisual->CriticalHitStopSeconds > BasicVisual->HitStopSeconds
				&& BasicVisual->CriticalCameraImpactStrength > BasicVisual->CameraImpactStrength);
			TestTrue(TEXT("shared hit visual keeps camera strength within the hard cap"),
				BasicVisual->DefeatCameraImpactStrength <= 8.0f);
		}
	}

	const UTexture2D* EntryBackground = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Entry/T_W11_TitleBackground.T_W11_TitleBackground"));
	TestNotNull(TEXT("W11 entry flow has a high-definition title background"), EntryBackground);
	const UTexture2D* EntryFog = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Entry/T_W11_TitleFog.T_W11_TitleFog"));
	TestNotNull(TEXT("W11 entry flow has a transparent animated fog layer"), EntryFog);
	const UTexture2D* EntryMenuPanel = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Entry/T_W11_EntryMenuPanel.T_W11_EntryMenuPanel"));
	TestNotNull(TEXT("W11 entry flow has an ornamental xianxia menu panel"), EntryMenuPanel);
	const UTexture2D* EntryButtonFrame = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Entry/T_W11_EntryButtonFrame.T_W11_EntryButtonFrame"));
	TestNotNull(TEXT("W11 entry buttons have a reusable transparent corner frame"), EntryButtonFrame);
	const USoundWave* EntryMusic = LoadObject<USoundWave>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Audio/Music/S_W11_TaixuMoonGate_MenuTheme.S_W11_TaixuMoonGate_MenuTheme"));
	TestNotNull(TEXT("W11 entry flow has original menu music"), EntryMusic);
	const USoundWave* HeroSelectionMusic = LoadObject<USoundWave>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Audio/Music/S_W11_MirrorOfLives_HeroSelectionTheme.S_W11_MirrorOfLives_HeroSelectionTheme"));
	TestNotNull(TEXT("W11 hero selection has distinct original music"), HeroSelectionMusic);
	for (const TCHAR* CombatSound : {
		TEXT("BasicAttack"), TEXT("AbilityCast"), TEXT("EnemyAttack"),
		TEXT("EnemyMiasma"), TEXT("EnemySwordDash"), TEXT("EnemyStoneSlam"),
		TEXT("Hit"), TEXT("Critical"), TEXT("Defeat")})
	{
		const FString Path = FString::Printf(
			TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Audio/Combat/S_W11_%s.S_W11_%s"),
			CombatSound, CombatSound);
		TestNotNull(FString::Printf(TEXT("shared combat sound %s loads"), CombatSound),
			LoadObject<USoundWave>(nullptr, *Path));
	}
	TSet<FSoftObjectPath> EnemyAttackSounds;
	for (const TCHAR* EnemyAbility : {
		TEXT("MiasmaBolt"), TEXT("SwordWraithDash"), TEXT("StoneFiendSlam")})
	{
		const FString AbilityPath = FString::Printf(
			TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Abilities/Enemies/DA_Ability_%s.DA_Ability_%s"),
			EnemyAbility, EnemyAbility);
		const UW11AbilityDefinition* Ability = LoadObject<UW11AbilityDefinition>(nullptr, *AbilityPath);
		TestNotNull(FString::Printf(TEXT("enemy ability %s loads for audio contract"), EnemyAbility), Ability);
		const UW11SkillVisualDefinition* Visual = Ability ? Ability->Visuals.LoadSynchronous() : nullptr;
		TestNotNull(FString::Printf(TEXT("enemy ability %s has a visual/audio definition"), EnemyAbility), Visual);
		const USoundBase* CastSound = Visual ? Visual->CastSound.LoadSynchronous() : nullptr;
		TestNotNull(FString::Printf(TEXT("enemy ability %s has a cast signature"), EnemyAbility), CastSound);
		if (Visual && CastSound)
		{
			EnemyAttackSounds.Add(Visual->CastSound.ToSoftObjectPath());
		}
	}
	TestEqual(TEXT("the three base enemies use three distinct attack sound assets"), EnemyAttackSounds.Num(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FW11CharacterAnimationFallbackContractsTest,
	"WorldWalker.W11.Presentation.CharacterAnimationFallbackContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FW11CharacterAnimationFallbackContractsTest::RunTest(const FString& Parameters)
{
	const UW11RunRuleSet* Rules = LoadObject<UW11RunRuleSet>(nullptr,
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/DA_W11_RunRules.DA_W11_RunRules"));
	TestNotNull(TEXT("run rules load for character animation fallback contracts"), Rules);
	if (!Rules)
	{
		return false;
	}

	const auto HasCompleteCoreActions = [](const FW11CharacterActionFlipbooks& Set)
	{
		return !Set.Idle.IsNull() && !Set.Move.IsNull()
			&& !Set.Cast.IsNull() && !Set.Hit.IsNull();
	};
	const auto IsEmpty = [](const FW11CharacterActionFlipbooks& Set)
	{
		return Set.Idle.IsNull() && Set.Move.IsNull() && Set.Dodge.IsNull()
			&& Set.Cast.IsNull() && Set.Hit.IsNull();
	};

	TestEqual(TEXT("animation fallback contract covers all ten protagonists"), Rules->Heroes.Num(), 10);
	for (const TSoftObjectPtr<UW11HeroDefinition>& HeroAsset : Rules->Heroes)
	{
		const UW11HeroDefinition* Hero = HeroAsset.LoadSynchronous();
		TestNotNull(TEXT("fallback contract hero loads"), Hero);
		if (!Hero)
		{
			continue;
		}
		const UW11CharacterAnimationSet* AnimationSet = Hero->CombatAnimations.LoadSynchronous();
		const FString HeroLabel = Hero->DefinitionId.ToString();
		TestNotNull(FString::Printf(TEXT("%s animation set loads"), *HeroLabel), AnimationSet);
		if (!AnimationSet)
		{
			continue;
		}

		TestTrue(FString::Printf(TEXT("%s right facing has all four authored core actions"), *HeroLabel),
			HasCompleteCoreActions(AnimationSet->Right));
		TestTrue(FString::Printf(TEXT("%s left facing has all four authored core actions"), *HeroLabel),
			HasCompleteCoreActions(AnimationSet->Left));
		TestTrue(FString::Printf(TEXT("%s right dodge is intentionally left null for runtime Move fallback"), *HeroLabel),
			AnimationSet->Right.Dodge.IsNull());
		TestTrue(FString::Printf(TEXT("%s left dodge is intentionally left null for runtime Move fallback"), *HeroLabel),
			AnimationSet->Left.Dodge.IsNull());

		const bool bUpComplete = HasCompleteCoreActions(AnimationSet->Up);
		const bool bDownComplete = HasCompleteCoreActions(AnimationSet->Down);
		const bool bUpEmpty = IsEmpty(AnimationSet->Up);
		const bool bDownEmpty = IsEmpty(AnimationSet->Down);
		TestTrue(FString::Printf(TEXT("%s up facing is fully authored or fully empty"), *HeroLabel),
			bUpComplete || bUpEmpty);
		TestTrue(FString::Printf(TEXT("%s down facing is fully authored or fully empty"), *HeroLabel),
			bDownComplete || bDownEmpty);
		TestEqual(FString::Printf(TEXT("%s vertical facings use one coherent mode"), *HeroLabel),
			bUpComplete, bDownComplete);
		if (bUpEmpty && bDownEmpty)
		{
			TestTrue(FString::Printf(TEXT("%s keeps horizontal facing while vertical art is absent"), *HeroLabel),
				AnimationSet->bKeepLastHorizontalFacingForVerticalMovement);
		}
		else
		{
			TestFalse(FString::Printf(TEXT("%s enables authored vertical facings"), *HeroLabel),
				AnimationSet->bKeepLastHorizontalFacingForVerticalMovement);
		}

		if (Hero->DefinitionId == FName(TEXT("Hero.Yu")))
		{
			const UPaperFlipbook* RightMove = AnimationSet->Right.Move.LoadSynchronous();
			const UPaperFlipbook* LeftMove = AnimationSet->Left.Move.LoadSynchronous();
			TestNotNull(TEXT("Yu right-facing 24-frame movement sample loads"), RightMove);
			TestNotNull(TEXT("Yu left-facing 24-frame movement sample loads"), LeftMove);
			if (RightMove)
			{
				TestEqual(TEXT("Yu right-facing movement sample has 24 frames"), RightMove->GetNumFrames(), 24);
				TestTrue(TEXT("Yu right-facing movement sample plays at 24 FPS"),
					FMath::IsNearlyEqual(RightMove->GetFramesPerSecond(), 24.0f));
			}
			if (LeftMove)
			{
				TestEqual(TEXT("Yu left-facing movement sample has 24 frames"), LeftMove->GetNumFrames(), 24);
				TestTrue(TEXT("Yu left-facing movement sample plays at 24 FPS"),
					FMath::IsNearlyEqual(LeftMove->GetFramesPerSecond(), 24.0f));
			}
		}
	}

	const UW11EnemyDefinition* StoneFiend = LoadObject<UW11EnemyDefinition>(nullptr,
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Enemies/DA_Enemy_StoneFiend.DA_Enemy_StoneFiend"));
	TestNotNull(TEXT("Stone Fiend movement sample definition loads"), StoneFiend);
	if (StoneFiend)
	{
		const UPaperFlipbook* RightMove = StoneFiend->MoveFlipbook.LoadSynchronous();
		const UPaperFlipbook* LeftMove = StoneFiend->MoveFlipbookLeft.LoadSynchronous();
		TestNotNull(TEXT("Stone Fiend right-facing 24-frame movement sample loads"), RightMove);
		TestNotNull(TEXT("Stone Fiend left-facing 24-frame movement sample loads"), LeftMove);
		if (RightMove)
		{
			TestEqual(TEXT("Stone Fiend right-facing movement sample has 24 frames"), RightMove->GetNumFrames(), 24);
			TestTrue(TEXT("Stone Fiend right-facing movement sample plays at 24 FPS"),
				FMath::IsNearlyEqual(RightMove->GetFramesPerSecond(), 24.0f));
		}
		if (LeftMove)
		{
			TestEqual(TEXT("Stone Fiend left-facing movement sample has 24 frames"), LeftMove->GetNumFrames(), 24);
			TestTrue(TEXT("Stone Fiend left-facing movement sample plays at 24 FPS"),
				FMath::IsNearlyEqual(LeftMove->GetFramesPerSecond(), 24.0f));
		}
	}
	return true;
}

#endif
