#include "Game/W11GameMode.h"

#include "Components/W11AttributeComponent.h"
#include "Components/W11CombatComponent.h"
#include "Components/W11RunEconomyComponent.h"
#include "Core/W11StatLibrary.h"
#include "Data/W11Definitions.h"
#include "Game/W11Character.h"
#include "Game/W11Enemy.h"
#include "Game/W11GameState.h"
#include "Game/W11ChapterRouteSubsystem.h"
#include "Game/W11PlayerController.h"
#include "Game/W11PlayerState.h"
#include "Game/W11RunDirector.h"
#include "Online/W11SessionSubsystem.h"
#include "UI/W11HUD.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformMisc.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "WorldWalkerW11.h"

namespace
{
bool HasManualEvidenceTestOverride()
{
	const TCHAR* CommandLine = FCommandLine::Get();
	int32 RequestedStartStage = 0;
	return FParse::Param(CommandLine, TEXT("W11M5Loadout"))
		|| FParse::Value(CommandLine, TEXT("W11StartStage="), RequestedStartStage)
		|| FParse::Param(CommandLine, TEXT("W11AutoCastInitialAbility"))
		|| FParse::Param(CommandLine, TEXT("W11AutoResolveWaves"))
		|| FParse::Param(CommandLine, TEXT("W11AutoRestartOnDefeat"))
		|| FParse::Param(CommandLine, TEXT("W11AutoStartNewMemoryRun"))
		|| FParse::Param(CommandLine, TEXT("W11NodeObserverSmoke"))
		|| FParse::Param(CommandLine, TEXT("W11ChapterOwnerSmoke"))
		|| FParse::Param(CommandLine, TEXT("W11AutoAdvanceChapterRoute"))
		|| FParse::Param(CommandLine, TEXT("W11AutoQuitOnRouteComplete"))
		|| FParse::Param(CommandLine, TEXT("W11AutoQuitAtFirstSafeNode"))
		|| FParse::Param(CommandLine, TEXT("W11AutoResume"))
		|| FParse::Param(CommandLine, TEXT("W11NetworkSmoke"))
		|| FParse::Param(CommandLine, TEXT("W11AudioSmoke"))
		|| FParse::Param(CommandLine, TEXT("W11TelegraphSmoke"))
		|| FParse::Param(CommandLine, TEXT("W11PresentationSmoke"))
		|| AW11GameMode::IsAutomatedOutcomeExitRequested(CommandLine);
}
}

AW11GameMode::AW11GameMode()
{
	DefaultPawnClass = AW11Character::StaticClass();
	PlayerControllerClass = AW11PlayerController::StaticClass();
	PlayerStateClass = AW11PlayerState::StaticClass();
	GameStateClass = AW11GameState::StaticClass();
	HUDClass = AW11HUD::StaticClass();
	bUseSeamlessTravel = false;
	RuleSetAsset = TSoftObjectPtr<UW11RunRuleSet>(FSoftObjectPath(
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/DA_W11_RunRules.DA_W11_RunRules")));
}

void AW11GameMode::BeginPlay()
{
	Super::BeginPlay();
	if (FParse::Param(FCommandLine::Get(), TEXT("W11NodeObserverSmoke")))
	{
		OnCombatNodeCompleted.AddDynamic(this, &AW11GameMode::HandleNodeObserverSmoke);
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_COMBAT_NODE_OBSERVER_SMOKE_BOUND OwnershipClaimed=0"));
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("W11ChapterOwnerSmoke")))
	{
		ClaimChapterTransitionOwnership(this);
	}
	ActiveRuleSet = RuleSetAsset.LoadSynchronous();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		ChapterRouteSubsystem = GameInstance->GetSubsystem<UW11ChapterRouteSubsystem>();
		UW11ChapterRouteDefinition* Route = ActiveRuleSet
			? ActiveRuleSet->ChapterRoute.LoadSynchronous() : nullptr;
		if (ChapterRouteSubsystem)
		{
			ChapterRouteSubsystem->ConfigureRoute(Route);
			bFormalChapterRouteActive = Route
				&& UW11ChapterRouteSubsystem::IsCursorValid(
					Route, ChapterRouteSubsystem->GetRouteCursor());
			if (bFormalChapterRouteActive)
			{
				ClaimChapterTransitionOwnership(this);
				UE_LOG(LogWorldWalkerW11, Display,
					TEXT("W11_FORMAL_CHAPTER_ROUTE_READY Route=%s Chapters=%d SafeResume=%d"),
					*Route->DefinitionId.ToString(), Route->Chapters.Num(),
					ChapterRouteSubsystem->DoesActiveRunSnapshotExist() ? 1 : 0);
			}
		}
	}
	const int32 RunSeed = ResolveRunSeed();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	RunDirector = GetWorld()->SpawnActor<AW11RunDirector>(AW11RunDirector::StaticClass(), FTransform::Identity, SpawnParameters);
	if (RunDirector)
	{
		RunDirector->Initialize(ActiveRuleSet, RunSeed);
	}

	if (AW11GameState* State = GetGameState<AW11GameState>())
	{
		State->InitializeRun(RunSeed, ActiveRuleSet ? ActiveRuleSet->DefinitionId : TEXT("W11_DefaultRules"));
	}

	UE_LOG(
		LogWorldWalkerW11,
		Display,
		TEXT("W11_ARCHITECTURE_READY Schema=%d Seed=%d ContentVersion=%s RuleVersion=%s RuleSet=%s Director=%d NetMode=%d"),
		ActiveRuleSet ? ActiveRuleSet->CombatLogSchemaVersion : 1,
		RunSeed,
		ActiveRuleSet ? *ActiveRuleSet->ContentVersion : TEXT("Fallback"),
		ActiveRuleSet ? *ActiveRuleSet->RuleVersion : TEXT("Fallback"),
		*GetNameSafe(ActiveRuleSet),
		RunDirector ? 1 : 0,
		static_cast<int32>(GetNetMode()));

	FString InputScriptIdString = TEXT("None");
	FParse::Value(FCommandLine::Get(), TEXT("W11InputScript="), InputScriptIdString);
	const bool bAutoStart = FParse::Param(FCommandLine::Get(), TEXT("W11AutoStart"))
		|| UGameplayStatics::HasOption(OptionsString, TEXT("W11Start"));
	const bool bAutoResume = FParse::Param(FCommandLine::Get(), TEXT("W11AutoResume"));
	FString ManualEvidenceRoleString;
	if (FParse::Value(FCommandLine::Get(), TEXT("W11ManualEvidenceRole="), ManualEvidenceRoleString))
	{
		const FName ManualEvidenceRole(*ManualEvidenceRoleString);
		ActiveManualEvidenceRole = ManualEvidenceRole;
		const FName InputScriptId(*InputScriptIdString);
		const bool bUnattended = FApp::IsUnattended();
		const bool bNullRHI = FParse::Param(FCommandLine::Get(), TEXT("NullRHI"));
		const bool bRenderOffscreen = FParse::Param(FCommandLine::Get(), TEXT("RenderOffscreen"));
		const bool bNoSound = FParse::Param(FCommandLine::Get(), TEXT("NoSound"));
		const bool bTestOverride = HasManualEvidenceTestOverride();
		const ENetMode NetMode = GetNetMode();
		const FName RejectionReason = ResolveManualEvidenceSessionRejectionReason(
			ManualEvidenceRole, InputScriptId, bUnattended, bNullRHI, bRenderOffscreen, bNoSound,
			bAutoStart, bTestOverride, NetMode);
		ManualEvidenceSessionRejectionReason = RejectionReason;
		if (RejectionReason.IsNone())
		{
			UE_LOG(LogWorldWalkerW11, Display,
				TEXT("W11_MANUAL_EVIDENCE_SESSION Role=%s Seed=%d Schema=%d InputScript=None Interactive=1 Rendered=1 Audio=1 AutoStart=0 NetMode=%d TestOverride=0"),
				*ManualEvidenceRole.ToString(), RunSeed,
				ActiveRuleSet ? ActiveRuleSet->CombatLogSchemaVersion : 1,
				static_cast<int32>(NetMode));
		}
		else
		{
			UE_LOG(LogWorldWalkerW11, Error,
				TEXT("W11_MANUAL_EVIDENCE_SESSION_REJECTED Role=%s Seed=%d Schema=%d InputScript=%s Interactive=%d Rendered=%d Audio=%d AutoStart=%d NetMode=%d TestOverride=%d Reason=%s"),
				*ManualEvidenceRoleString, RunSeed,
				ActiveRuleSet ? ActiveRuleSet->CombatLogSchemaVersion : 1,
				*InputScriptIdString, bUnattended ? 0 : 1,
				(bNullRHI || bRenderOffscreen) ? 0 : 1, bNoSound ? 0 : 1,
				bAutoStart ? 1 : 0, static_cast<int32>(NetMode), bTestOverride ? 1 : 0,
				*RejectionReason.ToString());
		}
	}

	if (bAutoResume)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AW11GameMode::AutoResumeActiveRun);
	}
	else if (bAutoStart)
	{
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_INPUT_SCRIPT_SELECTED Script=%s Seed=%d"), *InputScriptIdString, RunSeed);
		float AutoStartDelay = 0.0f;
		FParse::Value(FCommandLine::Get(), TEXT("W11AutoStartDelay="), AutoStartDelay);
		if (AutoStartDelay > UE_KINDA_SMALL_NUMBER)
		{
			FTimerHandle AutoStartTimerHandle;
			GetWorldTimerManager().SetTimer(
				AutoStartTimerHandle, this, &AW11GameMode::StartRun, AutoStartDelay, false);
			UE_LOG(LogWorldWalkerW11, Display,
				TEXT("W11_AUTO_START_DELAYED Seconds=%.2f"), AutoStartDelay);
		}
		else
		{
			GetWorldTimerManager().SetTimerForNextTick(this, &AW11GameMode::StartRun);
		}
	}
	else
	{
		UE_LOG(LogWorldWalkerW11, Display, TEXT("W11_ENTRY_MENU_READY Phase=Lobby"));
	}
}

void AW11GameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	AW11PlayerState* PlayerState = NewPlayer ? NewPlayer->GetPlayerState<AW11PlayerState>() : nullptr;
	InitializePlayerState(PlayerState);
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UW11SessionSubsystem* Sessions = GameInstance->GetSubsystem<UW11SessionSubsystem>())
		{
			Sessions->RecordParticipantJoined(
				PlayerState ? PlayerState->GetPlayerId() : INDEX_NONE,
				GetW11PlayerStates().Num());
		}
	}
}

void AW11GameMode::Logout(AController* Exiting)
{
	const AW11PlayerState* PlayerState = Exiting ? Exiting->GetPlayerState<AW11PlayerState>() : nullptr;
	const int32 PlayerId = PlayerState ? PlayerState->GetPlayerId() : INDEX_NONE;
	const TArray<AW11PlayerState*> PlayersBeforeLogout = GetW11PlayerStates();
	const int32 RemainingPlayerCount = FMath::Max(
		0, PlayersBeforeLogout.Num() - (PlayersBeforeLogout.Contains(PlayerState) ? 1 : 0));
	Super::Logout(Exiting);
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UW11SessionSubsystem* Sessions = GameInstance->GetSubsystem<UW11SessionSubsystem>())
		{
			Sessions->RecordParticipantLeft(PlayerId, RemainingPlayerCount);
		}
	}
}

void AW11GameMode::StartRun()
{
	if (bRunStarted || !HasAuthority())
	{
		return;
	}
	bRunStarted = true;
	if (bFormalChapterRouteActive && ChapterRouteSubsystem && !bResumeRequested)
	{
		ChapterRouteSubsystem->ResetRouteCursor();
		ChapterRouteSubsystem->DeleteActiveRunSnapshot();
	}
	for (AW11PlayerState* PlayerState : GetW11PlayerStates())
	{
		EnsureFallbackCharacterSelection(PlayerState);
		InitializePlayerState(PlayerState);
		if (PlayerState && ActiveRuleSet
			&& FParse::Param(FCommandLine::Get(), TEXT("W11M5Loadout")))
		{
			for (int32 Index = 0; Index < FMath::Min(4, ActiveRuleSet->ActiveAbilities.Num()); ++Index)
			{
				if (UW11AbilityDefinition* Ability = ActiveRuleSet->ActiveAbilities[Index].LoadSynchronous())
				{
					if (!PlayerState->IsAbilityUnlocked(Ability->DefinitionId))
					{
						PlayerState->UnlockOrUpgradeAbility(Ability->DefinitionId);
					}
					PlayerState->EquipActiveAbility(
						static_cast<EW11AbilitySlot>(static_cast<int32>(EW11AbilitySlot::Active1) + Index),
						Ability->DefinitionId);
				}
			}
			for (const TSoftObjectPtr<UW11ManualDefinition>& ManualAsset : ActiveRuleSet->Manuals)
			{
				if (const UW11ManualDefinition* Manual = ManualAsset.LoadSynchronous();
					Manual && Manual->DefinitionId == TEXT("Manual.ThunderSpell"))
				{
					PlayerState->GetEconomyComponent()->PurchaseManual(Manual, 0);
					break;
				}
			}
			FW11StatBlock SmokeStats = PlayerState->GetAttributeComponent()->GetStats();
			SmokeStats.MaxHealth = FMath::Max(SmokeStats.MaxHealth, 1000.0f);
			SmokeStats.MaxMana = FMath::Max(SmokeStats.MaxMana, 500.0f);
			SmokeStats.ManaRegenPerSecond = FMath::Max(SmokeStats.ManaRegenPerSecond, 50.0f);
			SmokeStats.Armor = FMath::Max(SmokeStats.Armor, 50.0f);
			SmokeStats.Power = FMath::Max(SmokeStats.Power, 3.0f);
			PlayerState->GetAttributeComponent()->InitializeStats(SmokeStats, true);
			UE_LOG(LogWorldWalkerW11, Display,
				TEXT("W11_M5_LOADOUT_READY Player=%d Slots=%d BehaviorActiveBurn=%d HP=%.0f MP=%.0f"),
				PlayerState->GetPlayerId(), PlayerState->GetActiveAbilitySlots().Num(),
				PlayerState->GetEconomyComponent()->HasBehavior(TEXT("ManualBehavior.ActiveBurn")) ? 1 : 0,
				SmokeStats.MaxHealth, SmokeStats.MaxMana);
		}
	}
	if (bResumeRequested)
	{
		RestorePendingSafeNodeSnapshot();
	}
	bResumeRequested = false;
	int32 RequestedStartStage = 1;
	if (FParse::Value(FCommandLine::Get(), TEXT("W11StartStage="), RequestedStartStage)
		&& RequestedStartStage > 1)
	{
		if (AW11GameState* State = GetGameState<AW11GameState>())
		{
			State->SetStageIndex(RequestedStartStage - 1);
			UE_LOG(LogWorldWalkerW11, Display,
				TEXT("W11_TEST_START_STAGE_SELECTED Stage=%d"), RequestedStartStage);
		}
	}
	if (bFormalChapterRouteActive)
	{
		EnterCurrentChapterNode();
	}
	else
	{
		StartNextCombatStage();
	}
}

bool AW11GameMode::StartSoloRun(
	AW11PlayerController* Controller,
	const FName HeroId,
	const FName SectId)
{
	if (!HasAuthority() || bRunStarted || !Controller)
	{
		return false;
	}

	AW11PlayerState* PlayerState = Controller->GetPlayerState<AW11PlayerState>();
	UW11HeroDefinition* Hero = FindHeroDefinition(HeroId);
	UW11SectDefinition* Sect = FindSectDefinition(SectId);
	UW11AbilityDefinition* Ability = Sect ? Sect->InitialAbility.LoadSynchronous() : nullptr;
	if (!PlayerState || !Hero || !Sect || !Ability || Ability->DefinitionId.IsNone())
	{
		UE_LOG(LogWorldWalkerW11, Warning,
			TEXT("W11_CHARACTER_SELECTION_REJECTED Hero=%s Sect=%s"),
			*HeroId.ToString(), *SectId.ToString());
		return false;
	}

	PlayerState->SetCharacterCreationSelection(Hero->DefinitionId, Sect->DefinitionId, Ability->DefinitionId);
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_CHARACTER_SELECTION_CONFIRMED Player=%d Hero=%s Sect=%s Ability=%s"),
		PlayerState->GetPlayerId(), *Hero->DefinitionId.ToString(), *Sect->DefinitionId.ToString(),
		*Ability->DefinitionId.ToString());
	StartRun();
	return bRunStarted;
}

bool AW11GameMode::CanResumeActiveRun() const
{
	if (!bFormalChapterRouteActive || !ChapterRouteSubsystem || bRunStarted || !ActiveRuleSet)
	{
		return false;
	}
	FW11ActiveRunSnapshot Snapshot;
	return ChapterRouteSubsystem->LoadSafeNodeSnapshot(
		ActiveRuleSet->ContentVersion, ActiveRuleSet->RuleVersion, Snapshot);
}

bool AW11GameMode::ResumeSoloRun(AW11PlayerController* Controller)
{
	if (!HasAuthority() || !Controller || bRunStarted || !bFormalChapterRouteActive
		|| !ChapterRouteSubsystem || !ActiveRuleSet)
	{
		return false;
	}
	FW11ActiveRunSnapshot Snapshot;
	if (!ChapterRouteSubsystem->LoadSafeNodeSnapshot(
		ActiveRuleSet->ContentVersion, ActiveRuleSet->RuleVersion, Snapshot)
		|| Snapshot.Players.Num() != 1
		|| !ChapterRouteSubsystem->SetRouteCursor(Snapshot.Cursor))
	{
		return false;
	}
	AW11PlayerState* PlayerState = Controller->GetPlayerState<AW11PlayerState>();
	const FW11PlayerRunSnapshot& Player = Snapshot.Players[0];
	if (!PlayerState || Player.HeroId.IsNone() || Player.SectId.IsNone()
		|| Player.InitialAbilityId.IsNone())
	{
		return false;
	}
	PlayerState->SetCharacterCreationSelection(Player.HeroId, Player.SectId, Player.InitialAbilityId);
	PendingResumeSnapshot = MoveTemp(Snapshot);
	bResumeRequested = true;
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_SAFE_NODE_RESUME_REQUESTED Route=%s Chapter=%s Node=%s Seed=%d"),
		*PendingResumeSnapshot.RouteId.ToString(), *PendingResumeSnapshot.ChapterId.ToString(),
		*PendingResumeSnapshot.NodeId.ToString(), PendingResumeSnapshot.RunSeed);
	StartRun();
	return bRunStarted;
}

UW11AbilityDefinition* AW11GameMode::FindAbilityDefinition(const FName AbilityId) const
{
	if (AbilityId.IsNone() || !ActiveRuleSet)
	{
		return nullptr;
	}
	if (UW11AbilityDefinition* BasicAttack = GetBasicAttackDefinition();
		BasicAttack && BasicAttack->DefinitionId == AbilityId)
	{
		return BasicAttack;
	}
	for (const TSoftObjectPtr<UW11SectDefinition>& SectAsset : ActiveRuleSet->Sects)
	{
		if (const UW11SectDefinition* Sect = SectAsset.LoadSynchronous())
		{
			UW11AbilityDefinition* Ability = Sect->InitialAbility.LoadSynchronous();
			if (Ability && Ability->DefinitionId == AbilityId)
			{
				return Ability;
			}
		}
	}
	for (const TSoftObjectPtr<UW11AbilityDefinition>& AbilityAsset : ActiveRuleSet->ActiveAbilities)
	{
		UW11AbilityDefinition* Ability = AbilityAsset.LoadSynchronous();
		if (Ability && Ability->DefinitionId == AbilityId)
		{
			return Ability;
		}
	}
	return nullptr;
}

UW11AbilityDefinition* AW11GameMode::GetBasicAttackDefinition() const
{
	return ActiveRuleSet ? ActiveRuleSet->BasicAttackAbility.LoadSynchronous() : nullptr;
}

bool AW11GameMode::IsCombatRequestAllowed(const int32 EncounterInstanceId) const
{
	const AW11GameState* State = GetGameState<AW11GameState>();
	return HasAuthority() && State
		&& AW11GameState::IsPendingCombatEncounter(
			State->GetRunPhase(), State->GetCombatOutcome(),
			State->GetEncounterInstanceId(), EncounterInstanceId);
}

void AW11GameMode::CompleteCombatStage(const FW11BattleReward& Reward, const FName Reason)
{
	TryFinalizeCombat(EW11CombatOutcome::Victory,
		Reason.IsNone() ? FName(TEXT("EncounterObjectiveCompleted")) : Reason, Reward);
}

bool AW11GameMode::TryFinalizeCombat(
	const EW11CombatOutcome RequestedOutcome,
	const FName Reason,
	const FW11BattleReward& Reward)
{
	AW11GameState* State = GetGameState<AW11GameState>();
	if (!HasAuthority() || !State || State->GetRunPhase() != EW11RunPhase::Combat
		|| State->GetCombatOutcome() != EW11CombatOutcome::Pending)
	{
		return false;
	}

	EW11CombatOutcome FinalOutcome = RequestedOutcome;
	FName FinalReason = Reason;
	const TArray<AW11PlayerState*> Players = GetW11PlayerStates();
	// Frozen solo precedence: health reaching zero in the same authority batch wins
	// over clearing the final enemy. Co-op terminal priority remains intentionally open.
	const bool bSoloPlayerDefeated = Players.Num() == 1 && Players[0]
		&& Players[0]->GetAttributeComponent()->IsDefeated();
	FinalOutcome = ResolveSoloTerminalOutcome(RequestedOutcome, bSoloPlayerDefeated);
	if (FinalOutcome == EW11CombatOutcome::Defeat && RequestedOutcome != EW11CombatOutcome::Defeat)
	{
		FinalReason = TEXT("SoloPlayerDefeated");
	}

	const int32 EncounterInstanceId = State->GetEncounterInstanceId();
	if (!State->TrySetCombatOutcome(EncounterInstanceId, FinalOutcome, FinalReason))
	{
		UE_LOG(LogWorldWalkerW11, Warning,
			TEXT("W11_COMBAT_FINALIZE_IGNORED EncounterInstance=%d Requested=%d Reason=%s"),
			EncounterInstanceId, static_cast<int32>(RequestedOutcome), *Reason.ToString());
		return false;
	}

	if (RunDirector)
	{
		RunDirector->CancelActiveEncounter(EncounterInstanceId);
	}
	for (AW11PlayerState* PlayerState : Players)
	{
		if (PlayerState && PlayerState->GetAttributeComponent())
		{
			PlayerState->GetAttributeComponent()->ClearAllStatuses();
		}
	}
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_COMBAT_FINALIZED EncounterInstance=%d Outcome=%d Reason=%s Sequence=%d"),
		EncounterInstanceId, static_cast<int32>(FinalOutcome), *FinalReason.ToString(), State->GetOutcomeSequence());
	FW11CombatNodeResult NodeResult;
	NodeResult.EncounterInstanceId = EncounterInstanceId;
	NodeResult.MemoryRunSequence = State->GetMemoryRunSequence();
	NodeResult.StageIndex = State->GetStageIndex();
	NodeResult.EncounterId = RunDirector ? RunDirector->GetActiveEncounterId() : NAME_None;
	NodeResult.EncounterType = RunDirector ? RunDirector->GetActiveEncounterType() : EW11EncounterType::Clear;
	NodeResult.Outcome = FinalOutcome;
	NodeResult.Reason = FinalReason;
	NodeResult.Reward = Reward;
	for (const AW11PlayerState* PlayerState : Players)
	{
		if (!PlayerState || !PlayerState->GetAttributeComponent())
		{
			continue;
		}
		ClassifyCombatParticipant(
			PlayerState->GetPlayerId(),
			PlayerState->GetAttributeComponent()->IsDefeated(),
			NodeResult.DefeatedPlayerIds,
			NodeResult.SurvivingPlayerIds);
	}
	auto JoinPlayerIds = [](const TArray<int32>& PlayerIds)
	{
		TArray<FString> Values;
		Values.Reserve(PlayerIds.Num());
		for (const int32 PlayerId : PlayerIds)
		{
			Values.Add(FString::FromInt(PlayerId));
		}
		return FString::Join(Values, TEXT(","));
	};
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_COMBAT_NODE_PARTICIPANTS EncounterInstance=%d Outcome=%d Defeated=[%s] Surviving=[%s]"),
		EncounterInstanceId, static_cast<int32>(FinalOutcome),
		*JoinPlayerIds(NodeResult.DefeatedPlayerIds),
		*JoinPlayerIds(NodeResult.SurvivingPlayerIds));
	LastCombatNodeResult = NodeResult;
	bHasLastCombatNodeResult = true;
	const bool bHasNodeObservers = OnCombatNodeCompleted.IsBound();
	const bool bChapterRouterOwnsTransition = ShouldChapterRouterOwnTransition(
		HasChapterTransitionOwner(), bHasNodeObservers);
	OnCombatNodeCompleted.Broadcast(NodeResult);
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_COMBAT_NODE_TRANSITION_DECISION EncounterInstance=%d ExplicitOwner=%d Observers=%d OwnsTransition=%d"),
		EncounterInstanceId, HasChapterTransitionOwner() ? 1 : 0,
		bHasNodeObservers ? 1 : 0, bChapterRouterOwnsTransition ? 1 : 0);

	if (FinalOutcome == EW11CombatOutcome::Defeat)
	{
		if (bFormalChapterRouteActive && ChapterRouteSubsystem)
		{
			ChapterRouteSubsystem->DeleteActiveRunSnapshot();
		}
		State->SetRunPhase(bFormalChapterRouteActive
			? EW11RunPhase::Results : bChapterRouterOwnsTransition
			? EW11RunPhase::ChapterTransition : EW11RunPhase::Results);
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_COMBAT_NODE_DEFEAT_DESTINATION EncounterInstance=%d Destination=%s"),
			EncounterInstanceId,
			bFormalChapterRouteActive ? TEXT("ResultsSnapshotDeleted")
				: bChapterRouterOwnsTransition ? TEXT("ChapterTransition") : TEXT("Results"));
		for (AW11PlayerState* PlayerState : Players)
		{
			if (APawn* Pawn = PlayerState ? PlayerState->GetPawn() : nullptr)
			{
				Pawn->DisableInput(Cast<APlayerController>(Pawn->GetController()));
			}
		}
		if (!bChapterRouterOwnsTransition
			&& FParse::Param(FCommandLine::Get(), TEXT("W11AutoRestartOnDefeat")))
		{
			GetWorldTimerManager().SetTimerForNextTick(this, &AW11GameMode::AutoRestartAfterDefeat);
		}
		EmitCombatTelemetry(FinalOutcome, FinalReason);
		ScheduleAutomatedOutcomeExit(FinalOutcome, FinalReason);
		return true;
	}

	for (AW11PlayerState* PlayerState : Players)
	{
		if (!PlayerState || !PlayerState->GetEconomyComponent())
		{
			continue;
		}
		UW11RunEconomyComponent* Economy = PlayerState->GetEconomyComponent();
		RecordRewardGrant(PlayerState->GetPlayerId());
		Economy->AddSpiritStones(Reward.SpiritStones);
		const int32 ChoicesEarned = Economy->AddCultivationExperience(Reward.CultivationExperience);
		PlayerState->SetPendingCultivationSelections(
			PlayerState->GetPendingCultivationSelections() + ChoicesEarned);
		PlayerState->SetReadyForNextStage(false);
		UE_LOG(
			LogWorldWalkerW11,
			Display,
			TEXT("W11_BATTLE_REWARD_GRANTED Player=%d Stage=%d SpiritStones=%d Cultivation=%d Choices=%d"),
			PlayerState->GetPlayerId(),
			State->GetStageIndex(),
			Reward.SpiritStones,
			Reward.CultivationExperience,
			ChoicesEarned);
	}
	EmitCombatTelemetry(FinalOutcome, FinalReason);
	ScheduleAutomatedOutcomeExit(FinalOutcome, FinalReason);
	if (bChapterRouterOwnsTransition)
	{
		State->SetRunPhase(EW11RunPhase::ChapterTransition);
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_COMBAT_NODE_HANDED_OFF EncounterInstance=%d Encounter=%s Type=%d"),
			EncounterInstanceId, *NodeResult.EncounterId.ToString(), static_cast<int32>(NodeResult.EncounterType));
		if (bFormalChapterRouteActive)
		{
			AdvanceFormalRouteAfterCombat();
		}
	}
	else
	{
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_COMBAT_NODE_LEGACY_FALLBACK EncounterInstance=%d Destination=CultivationOrMarket"),
			EncounterInstanceId);
		EnterCultivationOrMarket();
	}
	return true;
}

EW11CombatOutcome AW11GameMode::ResolveSoloTerminalOutcome(
	const EW11CombatOutcome RequestedOutcome,
	const bool bSoloPlayerDefeated)
{
	return bSoloPlayerDefeated ? EW11CombatOutcome::Defeat : RequestedOutcome;
}

bool AW11GameMode::IsAutomatedOutcomeExitRequested(const TCHAR* CommandLine)
{
	return CommandLine && FParse::Param(CommandLine, TEXT("W11AutoQuitOnOutcome"));
}

void AW11GameMode::ScheduleAutomatedOutcomeExit(
	const EW11CombatOutcome Outcome,
	const FName Reason)
{
	if (!IsAutomatedOutcomeExitRequested(FCommandLine::Get()) || !GetWorld())
	{
		return;
	}

	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_AUTO_QUIT_ON_OUTCOME Outcome=%d Reason=%s Delay=0.500"),
		static_cast<int32>(Outcome), *Reason.ToString());
	FTimerHandle ExitTimerHandle;
	GetWorldTimerManager().SetTimer(ExitTimerHandle, []
	{
		FGenericPlatformMisc::RequestExit(false);
	}, 0.5f, false);
}

bool AW11GameMode::IsValidManualEvidenceRole(const FName Role)
{
	return Role == TEXT("Observe")
		|| Role == TEXT("Evasion")
		|| Role == TEXT("Dodge")
		|| Role == TEXT("Recovery")
		|| Role == TEXT("Feedback");
}

FString AW11GameMode::GetManualEvidenceRoleDisplayName(const FName Role)
{
	if (Role == TEXT("Observe"))
	{
		return TEXT("观察敌袭");
	}
	if (Role == TEXT("Evasion"))
	{
		return TEXT("纯移动躲弹");
	}
	if (Role == TEXT("Dodge"))
	{
		return TEXT("冲斩闪避");
	}
	if (Role == TEXT("Recovery"))
	{
		return TEXT("后摇惩罚");
	}
	if (Role == TEXT("Feedback"))
	{
		return TEXT("战斗反馈");
	}
	return TEXT("未知角色");
}

FString AW11GameMode::GetManualEvidenceRoleInstruction(const FName Role)
{
	if (Role == TEXT("Observe"))
	{
		return TEXT("不要攻击或闪避；完整观察瘴气弹、残剑冲斩、石甲重砸的前摇、生效与后摇，然后主动战败。");
	}
	if (Role == TEXT("Evasion"))
	{
		return TEXT("不要攻击或闪避；只用移动避开至少一次瘴气弹，并让瘴灵至少完成两次攻击，然后主动战败。");
	}
	if (Role == TEXT("Dodge"))
	{
		return TEXT("诱导残剑魂冲斩，在生效窗口使用空格闪避；必须出现一次免疫结果，之后正常完成本战。");
	}
	if (Role == TEXT("Recovery"))
	{
		return TEXT("诱导石甲妖重砸；等敌人进入后摇再用普攻造成生命伤害，至少成功一次后正常完成本战。");
	}
	if (Role == TEXT("Feedback"))
	{
		return TEXT("覆盖普攻、多目标术法、治疗、护障、会心、护障吸收与击败；与其他日志合计至少三门派及胜负双路径。");
	}
	return TEXT("本局人工验收角色无效；退出并使用 RunW11.ps1 的 ManualEvidenceRole 参数重新启动。");
}

FW11ManualEvidenceProgress AW11GameMode::ResolveManualEvidenceProgress(
	const FName Role,
	const FW11CombatTelemetry& Telemetry)
{
	FW11ManualEvidenceProgress Progress;
	const auto EnemyTelemetry = [&Telemetry](const FName EnemyId)
	{
		return Telemetry.Enemies.FindRef(EnemyId);
	};
	if (Role == TEXT("Observe"))
	{
		const int32 Miasma = EnemyTelemetry(TEXT("Enemy.MiasmaWisp")).Attacks;
		const int32 Sword = EnemyTelemetry(TEXT("Enemy.SwordWraith")).Attacks;
		const int32 Stone = EnemyTelemetry(TEXT("Enemy.StoneFiend")).Attacks;
		Progress.TotalSteps = 3;
		Progress.CompletedSteps = (Miasma > 0 ? 1 : 0) + (Sword > 0 ? 1 : 0) + (Stone > 0 ? 1 : 0);
		Progress.bViolation = Telemetry.AbilityUses > 0 || Telemetry.Dodges > 0;
		Progress.bComplete = Progress.CompletedSteps == Progress.TotalSteps && !Progress.bViolation;
		Progress.Detail = FString::Printf(TEXT("瘴灵 %d/1 · 残剑 %d/1 · 石甲 %d/1 · 禁用操作 %s"),
			Miasma > 0 ? 1 : 0, Sword > 0 ? 1 : 0, Stone > 0 ? 1 : 0,
			Progress.bViolation ? TEXT("已违反") : TEXT("正常"));
	}
	else if (Role == TEXT("Evasion"))
	{
		const FW11EnemyCombatTelemetry Miasma = EnemyTelemetry(TEXT("Enemy.MiasmaWisp"));
		Progress.TotalSteps = 2;
		Progress.CompletedSteps = (Miasma.Attacks >= 2 ? 1 : 0) + (Miasma.Evaded >= 1 ? 1 : 0);
		Progress.bViolation = Telemetry.AbilityUses > 0 || Telemetry.Dodges > 0;
		Progress.bComplete = Progress.CompletedSteps == Progress.TotalSteps && !Progress.bViolation;
		Progress.Detail = FString::Printf(TEXT("瘴气攻击 %d/2 · 已躲开 %d/1 · 禁用操作 %s"),
			FMath::Min(Miasma.Attacks, 2), FMath::Min(Miasma.Evaded, 1),
			Progress.bViolation ? TEXT("已违反") : TEXT("正常"));
	}
	else if (Role == TEXT("Dodge"))
	{
		Progress.TotalSteps = 1;
		Progress.CompletedSteps = Telemetry.SuccessfulDodgeImmunities > 0 ? 1 : 0;
		Progress.bComplete = Progress.CompletedSteps == Progress.TotalSteps;
		Progress.Detail = FString::Printf(TEXT("冲斩免疫 %d/1"),
			FMath::Min(Telemetry.SuccessfulDodgeImmunities, 1));
	}
	else if (Role == TEXT("Recovery"))
	{
		Progress.TotalSteps = 1;
		Progress.CompletedSteps = Telemetry.RecoveryPunishHits > 0 ? 1 : 0;
		Progress.bComplete = Progress.CompletedSteps == Progress.TotalSteps;
		Progress.Detail = FString::Printf(TEXT("后摇生命命中 %d/1"),
			FMath::Min(Telemetry.RecoveryPunishHits, 1));
	}
	else if (Role == TEXT("Feedback"))
	{
		Progress.TotalSteps = 7;
		Progress.CompletedSteps = (Telemetry.bBasicAttackEffectObserved ? 1 : 0)
			+ (Telemetry.bMultiTargetActiveObserved ? 1 : 0)
			+ (Telemetry.bActualHealObserved ? 1 : 0)
			+ (Telemetry.bActualBarrierObserved ? 1 : 0)
			+ (Telemetry.CriticalHits > 0 ? 1 : 0)
			+ (Telemetry.BarrierAbsorbed > 0.0f ? 1 : 0)
			+ (Telemetry.bEnemyDefeatObserved ? 1 : 0);
		Progress.bComplete = Progress.CompletedSteps == Progress.TotalSteps;
		Progress.Detail = FString::Printf(TEXT("普攻%d 范围%d 治疗%d 护障%d 会心%d 吸收%d 击败%d"),
			Telemetry.bBasicAttackEffectObserved ? 1 : 0,
			Telemetry.bMultiTargetActiveObserved ? 1 : 0,
			Telemetry.bActualHealObserved ? 1 : 0,
			Telemetry.bActualBarrierObserved ? 1 : 0,
			Telemetry.CriticalHits > 0 ? 1 : 0,
			Telemetry.BarrierAbsorbed > 0.0f ? 1 : 0,
			Telemetry.bEnemyDefeatObserved ? 1 : 0);
	}
	return Progress;
}

FName AW11GameMode::ResolveManualEvidenceSessionRejectionReason(
	const FName Role,
	const FName InputScriptId,
	const bool bUnattended,
	const bool bNullRHI,
	const bool bRenderOffscreen,
	const bool bNoSound,
	const bool bAutoStart,
	const bool bTestOverride,
	const ENetMode NetMode)
{
	if (!IsValidManualEvidenceRole(Role))
	{
		return TEXT("InvalidRole");
	}
	if (!InputScriptId.IsNone() && InputScriptId != TEXT("None"))
	{
		return TEXT("ScriptedInput");
	}
	if (bUnattended)
	{
		return TEXT("Unattended");
	}
	if (bNullRHI)
	{
		return TEXT("NullRHI");
	}
	if (bRenderOffscreen)
	{
		return TEXT("RenderOffscreen");
	}
	if (bNoSound)
	{
		return TEXT("NoSound");
	}
	if (bAutoStart)
	{
		return TEXT("AutoStart");
	}
	if (bTestOverride)
	{
		return TEXT("TestOverride");
	}
	if (NetMode != NM_Standalone)
	{
		return TEXT("NonStandalone");
	}
	return NAME_None;
}

bool AW11GameMode::IsManualEvidenceSessionAllowed(
	const FName Role,
	const FName InputScriptId,
	const bool bUnattended,
	const bool bNullRHI,
	const bool bRenderOffscreen,
	const bool bNoSound,
	const bool bAutoStart,
	const bool bTestOverride,
	const ENetMode NetMode)
{
	return ResolveManualEvidenceSessionRejectionReason(
		Role, InputScriptId, bUnattended, bNullRHI, bRenderOffscreen, bNoSound,
		bAutoStart, bTestOverride, NetMode).IsNone();
}

bool AW11GameMode::ShouldChapterRouterOwnTransition(
	const bool bHasExplicitOwner,
	const bool bHasObservers)
{
	// Observers may record telemetry, persistence or tests. They must never
	// suppress the compatibility transition simply by listening to the result.
	(void)bHasObservers;
	return bHasExplicitOwner;
}

bool AW11GameMode::ClaimChapterTransitionOwnership(UObject* RequestingOwner)
{
	if (!HasAuthority() || !IsValid(RequestingOwner))
	{
		return false;
	}
	if (ChapterTransitionOwner.IsValid() && ChapterTransitionOwner.Get() != RequestingOwner)
	{
		UE_LOG(LogWorldWalkerW11, Warning,
			TEXT("W11_CHAPTER_TRANSITION_OWNERSHIP_REJECTED Existing=%s Requesting=%s"),
			*GetNameSafe(ChapterTransitionOwner.Get()), *GetNameSafe(RequestingOwner));
		return false;
	}
	ChapterTransitionOwner = RequestingOwner;
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_CHAPTER_TRANSITION_OWNERSHIP_CLAIMED Owner=%s"), *GetNameSafe(RequestingOwner));
	return true;
}

void AW11GameMode::ReleaseChapterTransitionOwnership(UObject* RequestingOwner)
{
	if (!HasAuthority() || !IsValid(RequestingOwner)
		|| ChapterTransitionOwner.Get() != RequestingOwner)
	{
		return;
	}
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_CHAPTER_TRANSITION_OWNERSHIP_RELEASED Owner=%s"), *GetNameSafe(RequestingOwner));
	ChapterTransitionOwner.Reset();
}

void AW11GameMode::HandleNodeObserverSmoke(const FW11CombatNodeResult& Result)
{
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_COMBAT_NODE_OBSERVER_SMOKE_RECEIVED Run=%d EncounterInstance=%d Outcome=%d"),
		Result.MemoryRunSequence, Result.EncounterInstanceId, static_cast<int32>(Result.Outcome));
}

void AW11GameMode::ClassifyCombatParticipant(
	const int32 PlayerId,
	const bool bDefeated,
	TArray<int32>& DefeatedPlayerIds,
	TArray<int32>& SurvivingPlayerIds)
{
	if (PlayerId < 0)
	{
		return;
	}
	DefeatedPlayerIds.Remove(PlayerId);
	SurvivingPlayerIds.Remove(PlayerId);
	(bDefeated ? DefeatedPlayerIds : SurvivingPlayerIds).Add(PlayerId);
	DefeatedPlayerIds.Sort();
	SurvivingPlayerIds.Sort();
}

void AW11GameMode::AutoRestartAfterDefeat()
{
	RestartRunToLobby(Cast<AW11PlayerController>(GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr));
}

void AW11GameMode::RestartRunToLobby(AW11PlayerController* RequestingController)
{
	AW11GameState* State = GetGameState<AW11GameState>();
	if (!HasAuthority() || !State || State->GetRunPhase() != EW11RunPhase::Results
		|| State->GetCombatOutcome() != EW11CombatOutcome::Defeat || !RequestingController)
	{
		return;
	}
	FW11MemoryRunResetContext ResetContext;
	ResetContext.CompletedMemoryRunSequence = State->GetMemoryRunSequence();
	ResetContext.NextMemoryRunSequence = AW11GameState::AdvanceAuthoritySequence(
		ResetContext.CompletedMemoryRunSequence);
	ResetContext.CompletedRunSeed = State->GetRunSeed();
	ResetContext.RuleSetId = State->GetRuleSetId();
	if (bHasLastCombatNodeResult)
	{
		ResetContext.TerminalNode = LastCombatNodeResult;
	}
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_MEMORY_RUN_RESET_BOUNDARY CompletedRun=%d NextRun=%d Seed=%d RuleSet=%s EncounterInstance=%d Outcome=%d Listener=%d"),
		ResetContext.CompletedMemoryRunSequence, ResetContext.NextMemoryRunSequence,
		ResetContext.CompletedRunSeed, *ResetContext.RuleSetId.ToString(),
		ResetContext.TerminalNode.EncounterInstanceId,
		static_cast<int32>(ResetContext.TerminalNode.Outcome),
		OnBeforeMemoryRunReset.IsBound() ? 1 : 0);
	OnBeforeMemoryRunReset.Broadcast(ResetContext);
	bRunStarted = false;
	const int32 NewSeed = ResolveRunSeed();
	if (RunDirector)
	{
		RunDirector->Initialize(ActiveRuleSet, NewSeed);
	}
	State->InitializeRun(NewSeed, ActiveRuleSet ? ActiveRuleSet->DefinitionId : TEXT("W11_DefaultRules"));
	for (AW11PlayerState* PlayerState : GetW11PlayerStates())
	{
		if (!PlayerState)
		{
			continue;
		}
		PlayerState->ClearCharacterCreationSelection();
		InitializePlayerState(PlayerState);
		AW11Character* Character = Cast<AW11Character>(PlayerState->GetPawn());
		if (Character)
		{
			Character->ResetForNewMemoryRun();
		}
		int32 EquippedAbilityCount = 0;
		for (const FName AbilityId : PlayerState->GetActiveAbilitySlots())
		{
			EquippedAbilityCount += AbilityId.IsNone() ? 0 : 1;
		}
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_NEW_MEMORY_RUN_PLAYER_RESET Run=%d Player=%d Hero=%s Sect=%s ActiveSlots=%d Equipped=%d Unlocked=%d SpiritStones=%d CultivationLevel=%d Manuals=%d Treasures=%d Pending=%d BasicCooldown=%.3f DodgeCooldown=%.3f DamageImmune=%d"),
			State->GetMemoryRunSequence(), PlayerState->GetPlayerId(),
			*PlayerState->GetSelectedHeroId().ToString(), *PlayerState->GetSelectedSectId().ToString(),
			PlayerState->GetActiveAbilitySlots().Num(), EquippedAbilityCount,
			PlayerState->GetUnlockedAbilities().Num(),
			PlayerState->GetEconomyComponent()->GetSpiritStones(),
			PlayerState->GetEconomyComponent()->GetCultivationLevel(),
			PlayerState->GetEconomyComponent()->GetOwnedManuals().Num(),
			PlayerState->GetEconomyComponent()->GetOwnedTreasures().Num(),
			PlayerState->GetPendingCultivationSelections(),
			Character && Character->GetCombatComponent()
				? Character->GetCombatComponent()->GetBasicAttackCooldownEndTime() : 0.0f,
			Character ? Character->GetDodgeCooldownEndTime() : 0.0f,
			PlayerState->GetAttributeComponent()->IsDamageImmune() ? 1 : 0);
		if (APawn* Pawn = PlayerState->GetPawn())
		{
			Pawn->EnableInput(Cast<APlayerController>(Pawn->GetController()));
		}
		if (AW11PlayerController* Controller = Cast<AW11PlayerController>(PlayerState->GetPlayerController()))
		{
			Controller->ClientReturnToEntryMenu();
		}
	}
	LastCombatNodeResult = FW11CombatNodeResult();
	bHasLastCombatNodeResult = false;
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_NEW_MEMORY_RUN_READY Run=%d Seed=%d Phase=Lobby EncounterSequence=%d OutcomeSequence=%d"),
		State->GetMemoryRunSequence(), NewSeed, State->GetEncounterInstanceId(), State->GetOutcomeSequence());
	if (FParse::Param(FCommandLine::Get(), TEXT("W11AutoStartNewMemoryRun")))
	{
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_NEW_MEMORY_RUN_AUTOSTART_SCHEDULED Run=%d"), State->GetMemoryRunSequence());
		GetWorldTimerManager().SetTimerForNextTick(this, &AW11GameMode::StartRun);
	}
}

void AW11GameMode::HandleCultivationSelection(
	AW11PlayerController* Controller,
	const int32 OfferIndex)
{
	AW11GameState* State = GetGameState<AW11GameState>();
	AW11PlayerState* PlayerState = Controller ? Controller->GetPlayerState<AW11PlayerState>() : nullptr;
	if (!State || State->GetRunPhase() != EW11RunPhase::Cultivation || !PlayerState
		|| !PlayerState->GetCultivationOffers().IsValidIndex(OfferIndex)
		|| PlayerState->GetPendingCultivationSelections() <= 0)
	{
		return;
	}

	const FW11CultivationOffer Offer = PlayerState->GetCultivationOffers()[OfferIndex];
	FW11StatModifier Modifier;
	Modifier.Stat = Offer.Stat;
	Modifier.Operation = Offer.Operation;
	Modifier.Magnitude = Offer.Magnitude;
	Modifier.SourceId = Offer.OfferId;
	if (!PlayerState->GetAttributeComponent()->ApplyPermanentModifier(Modifier))
	{
		return;
	}

	PlayerState->ConsumeCultivationSelection();
	UE_LOG(
		LogWorldWalkerW11,
		Display,
		TEXT("W11_CULTIVATION_CHOICE_SELECTED Player=%d Offer=%s Stat=%d Magnitude=%.3f Pending=%d"),
		PlayerState->GetPlayerId(),
		*Offer.OfferId.ToString(),
		static_cast<int32>(Offer.Stat),
		Offer.Magnitude,
		PlayerState->GetPendingCultivationSelections());

	if (PlayerState->GetPendingCultivationSelections() > 0 && RunDirector)
	{
		PlayerState->SetCultivationOffers(RunDirector->GenerateCultivationOffers(PlayerState));
	}
	if (AreAllPlayersDoneWithCultivation())
	{
		if (bFormalChapterRouteActive)
		{
			EnterCurrentChapterNode();
		}
		else
		{
			EnterMarket();
		}
	}
}

void AW11GameMode::HandleShopPurchase(AW11PlayerController* Controller, const int32 OfferIndex)
{
	AW11GameState* State = GetGameState<AW11GameState>();
	AW11PlayerState* PlayerState = Controller ? Controller->GetPlayerState<AW11PlayerState>() : nullptr;
	if (!State || State->GetRunPhase() != EW11RunPhase::ImmortalMarket || !RunDirector || !PlayerState
		|| !PlayerState->GetShopOffers().IsValidIndex(OfferIndex))
	{
		return;
	}
	const FW11ShopOffer Offer = PlayerState->GetShopOffers()[OfferIndex];
	if (Offer.bPurchased || !PlayerState->GetEconomyComponent())
	{
		return;
	}

	UW11RunEconomyComponent* Economy = PlayerState->GetEconomyComponent();
	bool bPurchased = false;
	switch (Offer.Kind)
	{
	case EW11ShopOfferKind::Manual:
		bPurchased = Economy->PurchaseManual(RunDirector->FindManual(Offer.DefinitionId), Offer.Price);
		break;
	case EW11ShopOfferKind::Treasure:
		bPurchased = Economy->PurchaseTreasure(RunDirector->FindTreasure(Offer.DefinitionId), Offer.Price);
		break;
	case EW11ShopOfferKind::Service:
		bPurchased = Economy->PurchaseService(RunDirector->FindService(Offer.DefinitionId), Offer.Price);
		break;
	case EW11ShopOfferKind::Ability:
		if (RunDirector->FindAbility(Offer.DefinitionId)
			&& Economy->GetSpiritStones() >= Offer.Price
			&& PlayerState->UnlockOrUpgradeAbility(Offer.DefinitionId))
		{
			bPurchased = Economy->SpendSpiritStones(Offer.Price);
		}
		break;
	default:
		break;
	}
	if (!bPurchased)
	{
		return;
	}

	PlayerState->MarkShopOfferPurchased(OfferIndex);
	UE_LOG(
		LogWorldWalkerW11,
		Display,
		TEXT("W11_SHOP_PURCHASED Player=%d Offer=%s Definition=%s Kind=%d Price=%d Remaining=%d"),
		PlayerState->GetPlayerId(),
		*Offer.OfferId.ToString(),
		*Offer.DefinitionId.ToString(),
		static_cast<int32>(Offer.Kind),
		Offer.Price,
		Economy->GetSpiritStones());
}

void AW11GameMode::HandleEquipAbility(
	AW11PlayerController* Controller,
	const EW11AbilitySlot Slot,
	const FName AbilityId)
{
	AW11GameState* State = GetGameState<AW11GameState>();
	AW11PlayerState* PlayerState = Controller ? Controller->GetPlayerState<AW11PlayerState>() : nullptr;
	if (!HasAuthority() || !State || !PlayerState
		|| (State->GetRunPhase() != EW11RunPhase::Cultivation
			&& State->GetRunPhase() != EW11RunPhase::ImmortalMarket)
		|| !FindAbilityDefinition(AbilityId)
		|| !PlayerState->EquipActiveAbility(Slot, AbilityId))
	{
		return;
	}
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_ABILITY_EQUIPPED Player=%d Slot=%d Ability=%s Level=%d"),
		PlayerState->GetPlayerId(), static_cast<int32>(Slot), *AbilityId.ToString(),
		PlayerState->GetActiveAbilityLevel(Slot));
}

void AW11GameMode::HandleReadyForNextStage(AW11PlayerController* Controller, const bool bReady)
{
	AW11GameState* State = GetGameState<AW11GameState>();
	AW11PlayerState* PlayerState = Controller ? Controller->GetPlayerState<AW11PlayerState>() : nullptr;
	const bool bFormalSafeNode = bFormalChapterRouteActive
		&& State && State->GetRunPhase() == EW11RunPhase::ChapterTransition;
	if (!State || (State->GetRunPhase() != EW11RunPhase::ImmortalMarket && !bFormalSafeNode)
		|| !PlayerState)
	{
		return;
	}
	PlayerState->SetReadyForNextStage(bReady);
	if (AreAllPlayersReady())
	{
		if (bFormalChapterRouteActive && ChapterRouteSubsystem)
		{
			ChapterRouteSubsystem->AdvanceRouteCursor();
			EnterCurrentChapterNode();
		}
		else
		{
			StartNextCombatStage();
		}
	}
}

void AW11GameMode::InitializePlayerState(AW11PlayerState* PlayerState)
{
	if (!PlayerState || !PlayerState->GetAttributeComponent() || !PlayerState->GetEconomyComponent())
	{
		return;
	}
	FW11StatBlock Stats = ActiveRuleSet ? ActiveRuleSet->DefaultPlayerStats : FW11StatBlock();
	if (const UW11HeroDefinition* Hero = FindHeroDefinition(PlayerState->GetSelectedHeroId()))
	{
		for (const FW11StatModifier& Modifier : Hero->StartingModifiers)
		{
			UW11StatLibrary::ApplyModifier(Stats, Modifier);
		}
	}
	UW11StatLibrary::Sanitize(Stats);
	PlayerState->GetAttributeComponent()->InitializeStats(Stats, true);
	PlayerState->GetAttributeComponent()->OnDefeated.AddUniqueDynamic(this, &AW11GameMode::HandlePlayerDefeated);
	PlayerState->GetEconomyComponent()->InitializeRun(
		ActiveRuleSet ? ActiveRuleSet->ManualSlots : 6,
		ActiveRuleSet ? ActiveRuleSet->TreasureSlots : 4);
	PlayerState->ClearTransientOffers();
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_PLAYER_INITIALIZED Player=%d Hero=%s Sect=%s Ability=%s HP=%.1f MP=%.1f Power=%.2f Armor=%.1f Luck=%.1f Area=%.2f Crit=%.2f CritPower=%.2f"),
		PlayerState->GetPlayerId(), *PlayerState->GetSelectedHeroId().ToString(),
		*PlayerState->GetSelectedSectId().ToString(), *PlayerState->GetInitialAbilityId().ToString(),
		Stats.MaxHealth, Stats.MaxMana, Stats.Power, Stats.Armor, Stats.Luck, Stats.AreaScale,
		Stats.CritChance, Stats.CritMultiplier);
}

void AW11GameMode::RestorePendingSafeNodeSnapshot()
{
	if (!ChapterRouteSubsystem || PendingResumeSnapshot.Players.Num() != 1)
	{
		return;
	}
	AW11GameState* State = GetGameState<AW11GameState>();
	const TArray<AW11PlayerState*> Players = GetW11PlayerStates();
	if (!State || Players.Num() != 1 || !Players[0]
		|| !Players[0]->GetAttributeComponent() || !Players[0]->GetEconomyComponent())
	{
		return;
	}
	const FW11PlayerRunSnapshot& SavedPlayer = PendingResumeSnapshot.Players[0];
	AW11PlayerState* PlayerState = Players[0];
	State->InitializeRun(PendingResumeSnapshot.RunSeed,
		ActiveRuleSet ? ActiveRuleSet->DefinitionId : FName(TEXT("W11_DefaultRules")));
	State->SetStageIndex(PendingResumeSnapshot.StageIndex);
	if (RunDirector)
	{
		RunDirector->Initialize(ActiveRuleSet, PendingResumeSnapshot.RunSeed);
	}
	PlayerState->SetCharacterCreationSelection(
		SavedPlayer.HeroId, SavedPlayer.SectId, SavedPlayer.InitialAbilityId);
	PlayerState->GetAttributeComponent()->RestoreSafeNodeSnapshot(
		SavedPlayer.Stats, SavedPlayer.Health, SavedPlayer.Mana, SavedPlayer.Barrier);
	PlayerState->RestoreSafeNodeAbilitySnapshot(
		SavedPlayer.ActiveAbilitySlots, SavedPlayer.UnlockedAbilities);
	PlayerState->GetEconomyComponent()->RestoreSafeNodeSnapshot(
		SavedPlayer.SpiritStones, SavedPlayer.CultivationLevel,
		SavedPlayer.CultivationExperience, SavedPlayer.ExperienceToNextLevel,
		SavedPlayer.OwnedManuals, SavedPlayer.GrantedBehaviors, SavedPlayer.OwnedTreasures,
		SavedPlayer.ManualSlots, SavedPlayer.TreasureSlots);
	PlayerState->SetPendingCultivationSelections(0);
	PlayerState->SetReadyForNextStage(false);
	PlayerState->ClearTransientOffers();
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_SAFE_NODE_RESUMED Route=%s Chapter=%s Node=%s Stage=%d Seed=%d Player=%d HP=%.1f MP=%.1f"),
		*PendingResumeSnapshot.RouteId.ToString(), *PendingResumeSnapshot.ChapterId.ToString(),
		*PendingResumeSnapshot.NodeId.ToString(), PendingResumeSnapshot.StageIndex,
		PendingResumeSnapshot.RunSeed, PlayerState->GetPlayerId(), SavedPlayer.Health, SavedPlayer.Mana);
	PendingResumeSnapshot = FW11ActiveRunSnapshot();
}

void AW11GameMode::EnsureFallbackCharacterSelection(AW11PlayerState* PlayerState) const
{
	if (!PlayerState || PlayerState->HasCompletedCharacterCreation() || !ActiveRuleSet)
	{
		return;
	}

	FString HeroOverride;
	FString SectOverride;
	FParse::Value(FCommandLine::Get(), TEXT("W11Hero="), HeroOverride);
	FParse::Value(FCommandLine::Get(), TEXT("W11Sect="), SectOverride);
	UW11HeroDefinition* Hero = HeroOverride.IsEmpty() ? nullptr : FindHeroDefinition(FName(*HeroOverride));
	UW11SectDefinition* Sect = SectOverride.IsEmpty() ? nullptr : FindSectDefinition(FName(*SectOverride));
	if (!Hero && !ActiveRuleSet->Heroes.IsEmpty())
	{
		Hero = ActiveRuleSet->Heroes[0].LoadSynchronous();
	}
	if (!Sect && !ActiveRuleSet->Sects.IsEmpty())
	{
		Sect = ActiveRuleSet->Sects[0].LoadSynchronous();
	}
	UW11AbilityDefinition* Ability = Sect ? Sect->InitialAbility.LoadSynchronous() : nullptr;
	if (Hero && Sect && Ability)
	{
		PlayerState->SetCharacterCreationSelection(Hero->DefinitionId, Sect->DefinitionId, Ability->DefinitionId);
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_CHARACTER_SELECTION_FALLBACK Hero=%s Sect=%s Ability=%s"),
			*Hero->DefinitionId.ToString(), *Sect->DefinitionId.ToString(), *Ability->DefinitionId.ToString());
	}
}

UW11HeroDefinition* AW11GameMode::FindHeroDefinition(const FName HeroId) const
{
	if (!ActiveRuleSet || HeroId.IsNone())
	{
		return nullptr;
	}
	for (const TSoftObjectPtr<UW11HeroDefinition>& Asset : ActiveRuleSet->Heroes)
	{
		if (UW11HeroDefinition* Definition = Asset.LoadSynchronous(); Definition
			&& (Definition->DefinitionId == HeroId
				|| Definition->DefinitionId.ToString().EndsWith(TEXT(".") + HeroId.ToString())))
		{
			return Definition;
		}
	}
	return nullptr;
}

UW11SectDefinition* AW11GameMode::FindSectDefinition(const FName SectId) const
{
	if (!ActiveRuleSet || SectId.IsNone())
	{
		return nullptr;
	}
	for (const TSoftObjectPtr<UW11SectDefinition>& Asset : ActiveRuleSet->Sects)
	{
		if (UW11SectDefinition* Definition = Asset.LoadSynchronous(); Definition
			&& (Definition->DefinitionId == SectId
				|| Definition->DefinitionId.ToString().EndsWith(TEXT(".") + SectId.ToString())))
		{
			return Definition;
		}
	}
	return nullptr;
}

void AW11GameMode::EnterCultivationOrMarket()
{
	bool bAnyPending = false;
	for (AW11PlayerState* PlayerState : GetW11PlayerStates())
	{
		if (PlayerState && PlayerState->GetPendingCultivationSelections() > 0)
		{
			bAnyPending = true;
			PlayerState->SetCultivationOffers(
				RunDirector ? RunDirector->GenerateCultivationOffers(PlayerState) : TArray<FW11CultivationOffer>());
		}
	}
	if (bAnyPending)
	{
		if (AW11GameState* State = GetGameState<AW11GameState>())
		{
			State->SetRunPhase(EW11RunPhase::Cultivation);
		}
	}
	else
	{
		EnterMarket();
	}
}

void AW11GameMode::EnterMarket()
{
	AW11GameState* State = GetGameState<AW11GameState>();
	if (!State)
	{
		return;
	}
	State->SetRunPhase(EW11RunPhase::ImmortalMarket, GetNetMode() == NM_Standalone ? 0.0f : 60.0f);
	for (AW11PlayerState* PlayerState : GetW11PlayerStates())
	{
		if (!PlayerState)
		{
			continue;
		}
		PlayerState->SetReadyForNextStage(false);
		PlayerState->SetShopOffers(
			RunDirector ? RunDirector->GenerateShopOffers(PlayerState, State->GetStageIndex()) : TArray<FW11ShopOffer>());
	}
	UE_LOG(LogWorldWalkerW11, Display, TEXT("W11_IMMORTAL_MARKET_OPENED Stage=%d"), State->GetStageIndex());
}

void AW11GameMode::AdvanceFormalRouteAfterCombat()
{
	if (!ChapterRouteSubsystem || !ChapterRouteSubsystem->AdvanceRouteCursor())
	{
		UE_LOG(LogWorldWalkerW11, Error, TEXT("W11_CHAPTER_ROUTE_ADVANCE_FAILED"));
		if (AW11GameState* State = GetGameState<AW11GameState>())
		{
			State->SetRunPhase(EW11RunPhase::Results);
		}
		return;
	}
	if (ChapterRouteSubsystem->IsRouteComplete())
	{
		ChapterRouteSubsystem->DeleteActiveRunSnapshot();
		if (AW11GameState* State = GetGameState<AW11GameState>())
		{
			State->SetRunPhase(EW11RunPhase::Results);
		}
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_FORMAL_RUN_COMPLETED Chapters=5 SmallBosses=5 FinalBoss=GuChangyuan"));
		if (FParse::Param(FCommandLine::Get(), TEXT("W11AutoQuitOnRouteComplete")))
		{
			FTimerHandle ExitTimer;
			GetWorldTimerManager().SetTimer(ExitTimer, FTimerDelegate::CreateLambda([]
			{
				FPlatformMisc::RequestExit(false);
			}), 0.25f, false);
		}
		return;
	}

	bool bAnyPendingCultivation = false;
	for (AW11PlayerState* PlayerState : GetW11PlayerStates())
	{
		if (PlayerState && PlayerState->GetPendingCultivationSelections() > 0)
		{
			bAnyPendingCultivation = true;
			PlayerState->SetCultivationOffers(
				RunDirector ? RunDirector->GenerateCultivationOffers(PlayerState)
					: TArray<FW11CultivationOffer>());
		}
	}
	if (bAnyPendingCultivation)
	{
		if (AW11GameState* State = GetGameState<AW11GameState>())
		{
			State->SetRunPhase(EW11RunPhase::Cultivation);
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("W11AutoAdvanceChapterRoute")))
		{
			GetWorldTimerManager().SetTimerForNextTick(
				this, &AW11GameMode::AutoAdvanceFormalChapterNode);
		}
		return;
	}
	EnterCurrentChapterNode();
}

void AW11GameMode::EnterCurrentChapterNode()
{
	if (!bFormalChapterRouteActive || !ChapterRouteSubsystem)
	{
		return;
	}
	AW11GameState* State = GetGameState<AW11GameState>();
	UW11ChapterRouteDefinition* Route = ChapterRouteSubsystem->GetActiveRoute();
	if (!State || !Route)
	{
		return;
	}
	const FW11ChapterRouteCursor& Cursor = ChapterRouteSubsystem->GetRouteCursor();
	if (Cursor.bComplete)
	{
		State->SetRunPhase(EW11RunPhase::Results);
		ChapterRouteSubsystem->DeleteActiveRunSnapshot();
		return;
	}
	for (AW11PlayerState* PlayerState : GetW11PlayerStates())
	{
		if (PlayerState)
		{
			PlayerState->SetReadyForNextStage(false);
			PlayerState->ClearTransientOffers();
		}
	}
	if (Cursor.bFinalBoss)
	{
		State->SetChapterRouteState(5, 0, EW11ChapterNodeType::FinalBoss,
			TEXT("Chapter.Final"), TEXT("Node.Final.GuChangyuan"));
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_CHAPTER_NODE_ENTERED Chapter=6 Node=1 Type=FinalBoss Encounter=Boss.GuChangyuan"));
		StartFormalEncounter(Route->FinalBossEncounter.LoadSynchronous());
		return;
	}

	const UW11ChapterDefinition* Chapter = ChapterRouteSubsystem->GetCurrentChapter();
	const FW11ChapterRouteNode* Node = ChapterRouteSubsystem->GetCurrentNode();
	if (!Chapter || !Node)
	{
		UE_LOG(LogWorldWalkerW11, Error,
			TEXT("W11_CHAPTER_NODE_MISSING ChapterIndex=%d NodeIndex=%d"),
			Cursor.ChapterIndex, Cursor.NodeIndex);
		State->SetRunPhase(EW11RunPhase::Results);
		return;
	}
	State->SetChapterRouteState(Cursor.ChapterIndex, Cursor.NodeIndex,
		Node->NodeType, Chapter->DefinitionId, Node->NodeId);
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_CHAPTER_NODE_ENTERED Chapter=%d Node=%d ChapterId=%s NodeId=%s Type=%d Safe=%d Map=%s"),
		Cursor.ChapterIndex + 1, Cursor.NodeIndex + 1, *Chapter->DefinitionId.ToString(),
		*Node->NodeId.ToString(), static_cast<int32>(Node->NodeType), Node->bSafeNode ? 1 : 0,
		*Chapter->ChapterMap.ToSoftObjectPath().ToString());

	switch (Node->NodeType)
	{
	case EW11ChapterNodeType::Combat:
	case EW11ChapterNodeType::SmallBoss:
		StartFormalEncounter(Node->Encounter.LoadSynchronous());
		break;
	case EW11ChapterNodeType::ImmortalMarket:
		EnterMarket();
		break;
	case EW11ChapterNodeType::Rest:
		for (AW11PlayerState* PlayerState : GetW11PlayerStates())
		{
			if (UW11AttributeComponent* Attributes = PlayerState
				? PlayerState->GetAttributeComponent() : nullptr)
			{
				Attributes->Heal(Attributes->GetStats().MaxHealth * 0.35f);
				Attributes->RestoreMana(Attributes->GetStats().MaxMana * 0.50f);
			}
		}
		State->SetRunPhase(EW11RunPhase::ChapterTransition);
		break;
	case EW11ChapterNodeType::Event:
		State->SetRunPhase(EW11RunPhase::ChapterTransition);
		break;
	default:
		State->SetRunPhase(EW11RunPhase::Results);
		break;
	}
	if (Node->bSafeNode)
	{
		const bool bSaved = ChapterRouteSubsystem->SaveAtCurrentSafeNode(
			State, GetW11PlayerStates(), ActiveRuleSet ? ActiveRuleSet->ContentVersion : FString(),
			ActiveRuleSet ? ActiveRuleSet->RuleVersion : FString());
		if (bSaved && FParse::Param(FCommandLine::Get(), TEXT("W11AutoQuitAtFirstSafeNode")))
		{
			UE_LOG(LogWorldWalkerW11, Display,
				TEXT("W11_SAFE_NODE_AUTOMATION_EXIT Chapter=%d Node=%d"),
				Cursor.ChapterIndex + 1, Cursor.NodeIndex + 1);
			FTimerHandle ExitTimer;
			GetWorldTimerManager().SetTimer(ExitTimer, FTimerDelegate::CreateLambda([]
			{
				FPlatformMisc::RequestExit(false);
			}), 0.25f, false);
		}
	}
	if ((Node->NodeType == EW11ChapterNodeType::Event
		|| Node->NodeType == EW11ChapterNodeType::Rest
		|| Node->NodeType == EW11ChapterNodeType::ImmortalMarket)
		&& FParse::Param(FCommandLine::Get(), TEXT("W11AutoAdvanceChapterRoute"))
		&& !FParse::Param(FCommandLine::Get(), TEXT("W11AutoQuitAtFirstSafeNode")))
	{
		GetWorldTimerManager().SetTimerForNextTick(
			this, &AW11GameMode::AutoAdvanceFormalChapterNode);
	}
}

void AW11GameMode::AutoResumeActiveRun()
{
	AW11PlayerController* Controller = Cast<AW11PlayerController>(
		GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr);
	if (!ResumeSoloRun(Controller))
	{
		UE_LOG(LogWorldWalkerW11, Error, TEXT("W11_SAFE_NODE_AUTO_RESUME_FAILED"));
		FPlatformMisc::RequestExit(true);
	}
}

void AW11GameMode::AutoAdvanceFormalChapterNode()
{
	AW11GameState* State = GetGameState<AW11GameState>();
	if (!State || !bFormalChapterRouteActive || !ChapterRouteSubsystem)
	{
		return;
	}
	if (State->GetRunPhase() == EW11RunPhase::Cultivation)
	{
		const TArray<AW11PlayerState*> Players = GetW11PlayerStates();
		for (AW11PlayerState* PlayerState : Players)
		{
			AW11PlayerController* Controller = PlayerState
				? Cast<AW11PlayerController>(PlayerState->GetPlayerController()) : nullptr;
			while (Controller && PlayerState->GetPendingCultivationSelections() > 0
				&& State->GetRunPhase() == EW11RunPhase::Cultivation)
			{
				HandleCultivationSelection(Controller, 0);
			}
		}
		return;
	}
	if (State->GetRunPhase() != EW11RunPhase::ImmortalMarket
		&& State->GetRunPhase() != EW11RunPhase::ChapterTransition)
	{
		return;
	}
	for (AW11PlayerState* PlayerState : GetW11PlayerStates())
	{
		if (PlayerState)
		{
			PlayerState->SetReadyForNextStage(true);
		}
	}
	if (AreAllPlayersReady() && ChapterRouteSubsystem->AdvanceRouteCursor())
	{
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_FORMAL_NODE_AUTO_ADVANCED Phase=%d"), static_cast<int32>(State->GetRunPhase()));
		EnterCurrentChapterNode();
	}
}

void AW11GameMode::StartFormalEncounter(UW11EncounterDefinition* Encounter)
{
	AW11GameState* State = GetGameState<AW11GameState>();
	if (!State || !Encounter)
	{
		UE_LOG(LogWorldWalkerW11, Error, TEXT("W11_FORMAL_ENCOUNTER_MISSING"));
		if (State)
		{
			State->SetRunPhase(EW11RunPhase::Results);
		}
		return;
	}
	State->SetStageIndex(State->GetStageIndex() + 1);
	State->SetRunPhase(EW11RunPhase::Combat);
	const int32 EncounterInstanceId = State->BeginEncounter();
	ResetCombatTelemetry(EncounterInstanceId);
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_FORMAL_COMBAT_STARTED Stage=%d EncounterInstance=%d Encounter=%s"),
		State->GetStageIndex(), EncounterInstanceId, *Encounter->DefinitionId.ToString());
	if (RunDirector)
	{
		RunDirector->StartStage(State->GetStageIndex(), EncounterInstanceId, Encounter);
	}
}

void AW11GameMode::StartNextCombatStage()
{
	AW11GameState* State = GetGameState<AW11GameState>();
	if (!State)
	{
		return;
	}
	State->SetStageIndex(State->GetStageIndex() + 1);
	State->SetRunPhase(EW11RunPhase::Combat);
	const int32 EncounterInstanceId = State->BeginEncounter();
	ResetCombatTelemetry(EncounterInstanceId);
	for (AW11PlayerState* PlayerState : GetW11PlayerStates())
	{
		if (PlayerState)
		{
			PlayerState->ClearTransientOffers();
		}
	}
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_COMBAT_STAGE_STARTED Stage=%d EncounterInstance=%d"),
		State->GetStageIndex(), EncounterInstanceId);
	if (RunDirector)
	{
		RunDirector->StartStage(State->GetStageIndex(), EncounterInstanceId);
	}
}

void AW11GameMode::ResetCombatTelemetry(const int32 EncounterInstanceId)
{
	CombatTelemetry = FW11CombatTelemetry();
	CombatTelemetry.EncounterInstanceId = EncounterInstanceId;
	CombatTelemetry.StartServerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	LastManualEvidenceProgressSignature.Reset();
	EmitManualEvidenceProgressIfChanged();
}

void AW11GameMode::RecordCombatDamage(
	AActor* Target,
	const FW11DamageSpec& Spec,
	const FW11DamageBreakdown& Damage)
{
	if (!HasAuthority() || CombatTelemetry.EncounterInstanceId <= 0)
	{
		return;
	}
	const AW11GameState* State = GetGameState<AW11GameState>();
	const bool bCurrentPendingEncounter = State
		&& State->GetRunPhase() == EW11RunPhase::Combat
		&& State->GetCombatOutcome() == EW11CombatOutcome::Pending
		&& State->GetEncounterInstanceId() == CombatTelemetry.EncounterInstanceId;
	if (!bCurrentPendingEncounter && (Damage.HealthDamage > 0.0f || Damage.BarrierAbsorbed > 0.0f))
	{
		++CombatTelemetry.GhostDamage;
	}

	const bool bSourceIsPlayer = Cast<AW11Character>(Spec.Source) != nullptr
		|| Cast<AW11PlayerState>(Spec.Source) != nullptr;
	const bool bSourceIsEnemy = Cast<AW11Enemy>(Spec.Source) != nullptr;
	const bool bTargetIsEnemy = Cast<AW11Enemy>(Target) != nullptr;
	const bool bTargetIsPlayer = Cast<AW11PlayerState>(Target) != nullptr;
	const bool bPlayerDealtDamage = bSourceIsPlayer && bTargetIsEnemy;
	const bool bPlayerTookDamage = bSourceIsEnemy && bTargetIsPlayer;
	if (bPlayerDealtDamage)
	{
		CombatTelemetry.PlayerDamageDealt += Damage.HealthDamage;
		if (Spec.bCritical && (Damage.HealthDamage > 0.0f || Damage.BarrierAbsorbed > 0.0f))
		{
			++CombatTelemetry.CriticalHits;
		}
	}
	else if (bPlayerTookDamage)
	{
		CombatTelemetry.PlayerDamageTaken += Damage.HealthDamage;
		if (CombatTelemetry.WarnedEnemyHitsAwaitingDamage > 0)
		{
			--CombatTelemetry.WarnedEnemyHitsAwaitingDamage;
		}
		else
		{
			++CombatTelemetry.NoWarningDamage;
		}
		if (Damage.bImmune && Damage.RawDamage > 0.0f)
		{
			++CombatTelemetry.SuccessfulDodgeImmunities;
		}
	}
	if (bPlayerDealtDamage || bPlayerTookDamage)
	{
		CombatTelemetry.ArmorMitigated += Damage.ArmorMitigated;
		CombatTelemetry.BarrierAbsorbed += Damage.BarrierAbsorbed;
	}
	EmitManualEvidenceProgressIfChanged();
}

void AW11GameMode::RecordAbilityUse(const FName AbilityId, const float ManaCost)
{
	if (!HasAuthority() || CombatTelemetry.EncounterInstanceId <= 0)
	{
		return;
	}
	++CombatTelemetry.AbilityUses;
	++CombatTelemetry.AbilityUseCounts.FindOrAdd(AbilityId);
	CombatTelemetry.ManaSpent += FMath::Max(0.0f, ManaCost);
	EmitManualEvidenceProgressIfChanged();
}

void AW11GameMode::RecordAbilityExecutionResult(
	const FW11AbilityExecutionResult& Result,
	const int32 TargetCount)
{
	if (!HasAuthority() || CombatTelemetry.EncounterInstanceId <= 0 || !Result.bAccepted
		|| Result.EncounterInstanceId != CombatTelemetry.EncounterInstanceId)
	{
		return;
	}
	if (Result.Slot != EW11AbilitySlot::BasicAttack && TargetCount >= 2)
	{
		CombatTelemetry.bMultiTargetActiveObserved = true;
	}
	for (const FW11EffectResult& Effect : Result.Effects)
	{
		if (Result.Slot == EW11AbilitySlot::BasicAttack
			&& Effect.Type == EW11EffectResultType::Damage && Effect.ActualAmount > 0.0f)
		{
			CombatTelemetry.bBasicAttackEffectObserved = true;
		}
		if (Effect.Type == EW11EffectResultType::Heal && Effect.ActualAmount > 0.0f)
		{
			CombatTelemetry.bActualHealObserved = true;
		}
		if (Effect.Type == EW11EffectResultType::Barrier && Effect.ActualAmount > 0.0f)
		{
			CombatTelemetry.bActualBarrierObserved = true;
		}
		if (Effect.Type == EW11EffectResultType::Damage && Effect.Damage.bDefeated)
		{
			CombatTelemetry.bEnemyDefeatObserved = true;
		}
	}
	EmitManualEvidenceProgressIfChanged();
}

void AW11GameMode::RecordAbilityFailure(const EW11AbilityRejectionReason Reason)
{
	if (!HasAuthority() || CombatTelemetry.EncounterInstanceId <= 0)
	{
		return;
	}
	++CombatTelemetry.AbilityFailures;
	++CombatTelemetry.AbilityFailureReasons.FindOrAdd(Reason);
}

void AW11GameMode::RecordManaRestored(const float Amount)
{
	if (HasAuthority() && CombatTelemetry.EncounterInstanceId > 0)
	{
		CombatTelemetry.ManaRestored += FMath::Max(0.0f, Amount);
	}
}

void AW11GameMode::RecordDodge()
{
	if (HasAuthority() && CombatTelemetry.EncounterInstanceId > 0)
	{
		++CombatTelemetry.Dodges;
		EmitManualEvidenceProgressIfChanged();
	}
}

bool AW11GameMode::IsRecoveryPunishHit(
	const EW11EnemyCombatState EnemyState,
	const float HealthDamage)
{
	return EnemyState == EW11EnemyCombatState::Recovery && HealthDamage > 0.0f;
}

void AW11GameMode::RecordRecoveryPunishHit(
	const FName EnemyId,
	const int32 StableSpawnId,
	const FName AbilityId,
	const float HealthDamage)
{
	if (!HasAuthority() || CombatTelemetry.EncounterInstanceId <= 0
		|| !IsRecoveryPunishHit(EW11EnemyCombatState::Recovery, HealthDamage))
	{
		return;
	}
	++CombatTelemetry.RecoveryPunishHits;
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_RECOVERY_PUNISH_HIT EncounterInstance=%d Enemy=%s SpawnId=%d Ability=%s HealthDamage=%.1f Count=%d"),
		CombatTelemetry.EncounterInstanceId, *EnemyId.ToString(), StableSpawnId,
		*AbilityId.ToString(), HealthDamage, CombatTelemetry.RecoveryPunishHits);
	EmitManualEvidenceProgressIfChanged();
}

void AW11GameMode::RecordEnemySpawn(const FName EnemyId)
{
	if (!HasAuthority() || CombatTelemetry.EncounterInstanceId <= 0)
	{
		return;
	}
	++CombatTelemetry.Enemies.FindOrAdd(EnemyId).Spawned;
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (CombatTelemetry.FirstSpawnServerTime <= 0.0f)
	{
		CombatTelemetry.FirstSpawnServerTime = Now;
	}
	CombatTelemetry.LastSpawnServerTime = Now;
}

void AW11GameMode::RecordEnemyAttack(const FName EnemyId)
{
	if (HasAuthority() && CombatTelemetry.EncounterInstanceId > 0)
	{
		++CombatTelemetry.Enemies.FindOrAdd(EnemyId).Attacks;
		EmitManualEvidenceProgressIfChanged();
	}
}

void AW11GameMode::RecordEnemyHit(const FName EnemyId, const bool bHadWarning)
{
	if (!HasAuthority() || CombatTelemetry.EncounterInstanceId <= 0)
	{
		return;
	}
	++CombatTelemetry.Enemies.FindOrAdd(EnemyId).Hits;
	if (bHadWarning)
	{
		++CombatTelemetry.WarnedEnemyHitsAwaitingDamage;
	}
	EmitManualEvidenceProgressIfChanged();
}

void AW11GameMode::RecordEnemyAttackEvaded(const FName EnemyId)
{
	if (!HasAuthority() || CombatTelemetry.EncounterInstanceId <= 0)
	{
		return;
	}
	++CombatTelemetry.Enemies.FindOrAdd(EnemyId).Evaded;
	EmitManualEvidenceProgressIfChanged();
}

void AW11GameMode::RecordEnemyAttackCancelled(const FName EnemyId)
{
	if (HasAuthority() && CombatTelemetry.EncounterInstanceId > 0)
	{
		++CombatTelemetry.Enemies.FindOrAdd(EnemyId).Cancelled;
	}
}

void AW11GameMode::RecordCancelledThreat()
{
	if (HasAuthority() && CombatTelemetry.EncounterInstanceId > 0)
	{
		++CombatTelemetry.CancelledThreats;
	}
}

void AW11GameMode::RecordRewardGrant(const int32 PlayerId)
{
	if (CombatTelemetry.RewardedPlayerIds.Contains(PlayerId))
	{
		++CombatTelemetry.DuplicateRewards;
		return;
	}
	CombatTelemetry.RewardedPlayerIds.Add(PlayerId);
	++CombatTelemetry.RewardGrants;
}

void AW11GameMode::EmitManualEvidenceProgressIfChanged()
{
	if (!HasAuthority() || CombatTelemetry.EncounterInstanceId <= 0
		|| ActiveManualEvidenceRole.IsNone()
		|| !ManualEvidenceSessionRejectionReason.IsNone())
	{
		return;
	}
	const FW11ManualEvidenceProgress Progress = GetManualEvidenceProgress();
	const FString Signature = FString::Printf(TEXT("%d|%d|%d|%d|%s"),
		Progress.CompletedSteps, Progress.TotalSteps,
		Progress.bComplete ? 1 : 0, Progress.bViolation ? 1 : 0, *Progress.Detail);
	if (Signature == LastManualEvidenceProgressSignature)
	{
		return;
	}
	LastManualEvidenceProgressSignature = Signature;
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_MANUAL_EVIDENCE_PROGRESS EncounterInstance=%d Role=%s Steps=%d/%d Complete=%d Violation=%d Detail=%s"),
		CombatTelemetry.EncounterInstanceId, *ActiveManualEvidenceRole.ToString(),
		Progress.CompletedSteps, Progress.TotalSteps, Progress.bComplete ? 1 : 0,
		Progress.bViolation ? 1 : 0, *Progress.Detail);
}

void AW11GameMode::EmitCombatTelemetry(
	const EW11CombatOutcome Outcome,
	const FName Reason) const
{
	if (!HasAuthority() || CombatTelemetry.EncounterInstanceId <= 0)
	{
		return;
	}
	const AW11GameState* State = GetGameState<AW11GameState>();
	const float EndServerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : CombatTelemetry.StartServerTime;
	const float Duration = FMath::Max(0.0f, EndServerTime - CombatTelemetry.StartServerTime);
	FString InputScriptId = TEXT("None");
	FParse::Value(FCommandLine::Get(), TEXT("W11InputScript="), InputScriptId);

	int32 Spawned = 0;
	int32 EnemyAttacks = 0;
	int32 EnemyHits = 0;
	int32 EnemyCancelled = 0;
	TArray<FName> EnemyIds;
	CombatTelemetry.Enemies.GetKeys(EnemyIds);
	EnemyIds.Sort([](const FName Left, const FName Right)
	{
		return Left.LexicalLess(Right);
	});
	for (const FName EnemyId : EnemyIds)
	{
		const FW11EnemyCombatTelemetry& Enemy = CombatTelemetry.Enemies.FindChecked(EnemyId);
		Spawned += Enemy.Spawned;
		EnemyAttacks += Enemy.Attacks;
		EnemyHits += Enemy.Hits;
		EnemyCancelled += Enemy.Cancelled;
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_COMBAT_SUMMARY_ENEMY EncounterInstance=%d Enemy=%s Spawned=%d Attacks=%d Hits=%d Cancelled=%d"),
			CombatTelemetry.EncounterInstanceId, *EnemyId.ToString(), Enemy.Spawned,
			Enemy.Attacks, Enemy.Hits, Enemy.Cancelled);
	}

	TArray<FName> AbilityIds;
	CombatTelemetry.AbilityUseCounts.GetKeys(AbilityIds);
	AbilityIds.Sort([](const FName Left, const FName Right)
	{
		return Left.LexicalLess(Right);
	});
	for (const FName AbilityId : AbilityIds)
	{
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_COMBAT_SUMMARY_ABILITY EncounterInstance=%d Ability=%s Uses=%d"),
			CombatTelemetry.EncounterInstanceId, *AbilityId.ToString(),
			CombatTelemetry.AbilityUseCounts.FindChecked(AbilityId));
	}
	for (int32 ReasonIndex = 0;
		ReasonIndex <= static_cast<int32>(EW11AbilityRejectionReason::NoLegalTarget); ++ReasonIndex)
	{
		const EW11AbilityRejectionReason FailureReason = static_cast<EW11AbilityRejectionReason>(ReasonIndex);
		if (const int32* Count = CombatTelemetry.AbilityFailureReasons.Find(FailureReason))
		{
			UE_LOG(LogWorldWalkerW11, Display,
				TEXT("W11_COMBAT_SUMMARY_REJECTION EncounterInstance=%d Reason=%d Count=%d"),
				CombatTelemetry.EncounterInstanceId, ReasonIndex, *Count);
		}
	}

	float RemainingHealth = 0.0f;
	float MaximumHealth = 0.0f;
	for (const AW11PlayerState* PlayerState : GetW11PlayerStates())
	{
		if (!PlayerState || !PlayerState->GetAttributeComponent())
		{
			continue;
		}
		TArray<FString> ActiveAbilityIds;
		for (const FName ActiveAbilityId : PlayerState->GetActiveAbilitySlots())
		{
			ActiveAbilityIds.Add(ActiveAbilityId.IsNone() ? TEXT("None") : ActiveAbilityId.ToString());
		}
		const UW11AttributeComponent* Attributes = PlayerState->GetAttributeComponent();
		const UW11AbilityDefinition* BasicAttack = GetBasicAttackDefinition();
		RemainingHealth += Attributes->GetHealth();
		MaximumHealth += Attributes->GetStats().MaxHealth;
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_COMBAT_SUMMARY_PLAYER EncounterInstance=%d Player=%d Hero=%s Sect=%s Basic=%s Active=%s Health=%.1f MaxHealth=%.1f"),
			CombatTelemetry.EncounterInstanceId, PlayerState->GetPlayerId(),
			*PlayerState->GetSelectedHeroId().ToString(), *PlayerState->GetSelectedSectId().ToString(),
			BasicAttack ? *BasicAttack->DefinitionId.ToString() : TEXT("None"),
			*FString::Join(ActiveAbilityIds, TEXT(",")),
			Attributes->GetHealth(), Attributes->GetStats().MaxHealth);
	}

	const float FirstSpawn = CombatTelemetry.FirstSpawnServerTime > 0.0f
		? CombatTelemetry.FirstSpawnServerTime - CombatTelemetry.StartServerTime : -1.0f;
	const float LastSpawn = CombatTelemetry.LastSpawnServerTime > 0.0f
		? CombatTelemetry.LastSpawnServerTime - CombatTelemetry.StartServerTime : -1.0f;
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_COMBAT_SUMMARY Schema=%d Seed=%d ContentVersion=%s RuleVersion=%s InputScript=%s Stage=%d EncounterInstance=%d Encounter=%s CandidateSignature=%08X SpawnSignature=%08X Spawned=%d FirstSpawn=%.3f LastSpawn=%.3f Duration=%.3f Outcome=%d Reason=%s Health=%.1f/%.1f DamageDealt=%.1f DamageTaken=%.1f ArmorMitigated=%.1f BarrierAbsorbed=%.1f Criticals=%d Dodges=%d DodgeImmunities=%d RecoveryPunishHits=%d EnemyAttacks=%d EnemyHits=%d EnemyCancelled=%d ManaSpent=%.1f ManaRestored=%.1f AbilityUses=%d AbilityFailures=%d RewardGrants=%d DuplicateRewards=%d GhostDamage=%d NoWarningDamage=%d CancelledThreats=%d UnmatchedWarnings=%d Stuck=0"),
		ActiveRuleSet ? ActiveRuleSet->CombatLogSchemaVersion : 1,
		State ? State->GetRunSeed() : 0,
		ActiveRuleSet ? *ActiveRuleSet->ContentVersion : TEXT("Fallback"),
		ActiveRuleSet ? *ActiveRuleSet->RuleVersion : TEXT("Fallback"),
		*InputScriptId,
		State ? State->GetStageIndex() : 0,
		CombatTelemetry.EncounterInstanceId,
		RunDirector ? *RunDirector->GetActiveEncounterId().ToString() : TEXT("None"),
		RunDirector ? RunDirector->GetCandidateSignature() : 0,
		RunDirector ? RunDirector->GetSpawnSignature() : 0,
		Spawned, FirstSpawn, LastSpawn, Duration, static_cast<int32>(Outcome), *Reason.ToString(),
		RemainingHealth, MaximumHealth, CombatTelemetry.PlayerDamageDealt,
		CombatTelemetry.PlayerDamageTaken, CombatTelemetry.ArmorMitigated,
		CombatTelemetry.BarrierAbsorbed, CombatTelemetry.CriticalHits,
		CombatTelemetry.Dodges, CombatTelemetry.SuccessfulDodgeImmunities,
		CombatTelemetry.RecoveryPunishHits,
		EnemyAttacks, EnemyHits, EnemyCancelled, CombatTelemetry.ManaSpent,
		CombatTelemetry.ManaRestored, CombatTelemetry.AbilityUses,
		CombatTelemetry.AbilityFailures, CombatTelemetry.RewardGrants,
		CombatTelemetry.DuplicateRewards, CombatTelemetry.GhostDamage,
		CombatTelemetry.NoWarningDamage, CombatTelemetry.CancelledThreats,
		CombatTelemetry.WarnedEnemyHitsAwaitingDamage);
}

void AW11GameMode::HandlePlayerDefeated()
{
	const TArray<AW11PlayerState*> Players = GetW11PlayerStates();
	if (Players.Num() == 1)
	{
		TryFinalizeCombat(EW11CombatOutcome::Defeat, TEXT("SoloPlayerDefeated"), FW11BattleReward());
	}
}

bool AW11GameMode::AreAllPlayersDoneWithCultivation() const
{
	const TArray<AW11PlayerState*> Players = GetW11PlayerStates();
	return !Players.IsEmpty() && Players.FindByPredicate([](const AW11PlayerState* PlayerState)
	{
		return PlayerState && PlayerState->GetPendingCultivationSelections() > 0;
	}) == nullptr;
}

bool AW11GameMode::AreAllPlayersReady() const
{
	const TArray<AW11PlayerState*> Players = GetW11PlayerStates();
	return !Players.IsEmpty() && Players.FindByPredicate([](const AW11PlayerState* PlayerState)
	{
		return PlayerState && !PlayerState->IsReadyForNextStage();
	}) == nullptr;
}

TArray<AW11PlayerState*> AW11GameMode::GetW11PlayerStates() const
{
	TArray<AW11PlayerState*> Result;
	if (const AW11GameState* State = GetGameState<AW11GameState>())
	{
		for (APlayerState* PlayerState : State->PlayerArray)
		{
			if (AW11PlayerState* W11PlayerState = Cast<AW11PlayerState>(PlayerState))
			{
				Result.Add(W11PlayerState);
			}
		}
	}
	return Result;
}

int32 AW11GameMode::ResolveRunSeed() const
{
	int32 CommandLineSeed = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("W11RunSeed="), CommandLineSeed))
	{
		return CommandLineSeed;
	}
	return static_cast<int32>(FDateTime::UtcNow().GetTicks() & MAX_int32);
}
