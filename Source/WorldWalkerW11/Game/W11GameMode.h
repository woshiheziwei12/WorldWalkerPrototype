#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/W11Types.h"
#include "Game/W11ChapterRouteSubsystem.h"
#include "W11GameMode.generated.h"

class AW11PlayerController;
class AW11PlayerState;
class AW11RunDirector;
class UW11AbilityDefinition;
class UW11ChapterRouteSubsystem;
class UW11EncounterDefinition;
class UW11HeroDefinition;
class UW11RunRuleSet;
class UW11SectDefinition;

struct FW11EnemyCombatTelemetry
{
	int32 Spawned = 0;
	int32 Attacks = 0;
	int32 Hits = 0;
	int32 Evaded = 0;
	int32 Cancelled = 0;
};

struct FW11CombatTelemetry
{
	int32 EncounterInstanceId = 0;
	float StartServerTime = 0.0f;
	float FirstSpawnServerTime = 0.0f;
	float LastSpawnServerTime = 0.0f;
	float PlayerDamageDealt = 0.0f;
	float PlayerDamageTaken = 0.0f;
	float ArmorMitigated = 0.0f;
	float BarrierAbsorbed = 0.0f;
	float ManaSpent = 0.0f;
	float ManaRestored = 0.0f;
	int32 CriticalHits = 0;
	int32 Dodges = 0;
	int32 SuccessfulDodgeImmunities = 0;
	int32 RecoveryPunishHits = 0;
	int32 AbilityUses = 0;
	int32 AbilityFailures = 0;
	int32 RewardGrants = 0;
	int32 DuplicateRewards = 0;
	int32 GhostDamage = 0;
	int32 NoWarningDamage = 0;
	int32 WarnedEnemyHitsAwaitingDamage = 0;
	int32 CancelledThreats = 0;
	bool bBasicAttackEffectObserved = false;
	bool bMultiTargetActiveObserved = false;
	bool bActualHealObserved = false;
	bool bActualBarrierObserved = false;
	bool bEnemyDefeatObserved = false;
	TMap<EW11AbilityRejectionReason, int32> AbilityFailureReasons;
	TMap<FName, int32> AbilityUseCounts;
	TMap<FName, FW11EnemyCombatTelemetry> Enemies;
	TSet<int32> RewardedPlayerIds;
};

struct FW11ManualEvidenceProgress
{
	int32 CompletedSteps = 0;
	int32 TotalSteps = 0;
	bool bComplete = false;
	bool bViolation = false;
	FString Detail;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FW11CombatNodeCompleted,
	const FW11CombatNodeResult&,
	Result);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FW11BeforeMemoryRunReset,
	const FW11MemoryRunResetContext&,
	Context);

UCLASS()
class WORLDWALKERW11_API AW11GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AW11GameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Run")
	void StartRun();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Run")
	bool StartSoloRun(AW11PlayerController* Controller, FName HeroId, FName SectId);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Run")
	bool ResumeSoloRun(AW11PlayerController* Controller);

	UFUNCTION(BlueprintPure, Category="W11|Run")
	bool CanResumeActiveRun() const;

	UFUNCTION(BlueprintPure, Category="W11|Run")
	UW11AbilityDefinition* FindAbilityDefinition(FName AbilityId) const;

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	UW11AbilityDefinition* GetBasicAttackDefinition() const;

	UFUNCTION(BlueprintPure, Category="W11|Run")
	const UW11RunRuleSet* GetActiveRuleSet() const { return ActiveRuleSet; }

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	bool IsCombatRequestAllowed(int32 EncounterInstanceId) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Run")
	void CompleteCombatStage(const FW11BattleReward& Reward, FName Reason = NAME_None);

	bool TryFinalizeCombat(EW11CombatOutcome RequestedOutcome, FName Reason, const FW11BattleReward& Reward);
	static EW11CombatOutcome ResolveSoloTerminalOutcome(
		EW11CombatOutcome RequestedOutcome,
		bool bSoloPlayerDefeated);
	static bool IsAutomatedOutcomeExitRequested(const TCHAR* CommandLine);
	static bool IsValidManualEvidenceRole(FName Role);
	static FString GetManualEvidenceRoleDisplayName(FName Role);
	static FString GetManualEvidenceRoleInstruction(FName Role);
	static FName ResolveManualEvidenceSessionRejectionReason(
		FName Role,
		FName InputScriptId,
		bool bUnattended,
		bool bNullRHI,
		bool bRenderOffscreen,
		bool bNoSound,
		bool bAutoStart,
		bool bTestOverride,
		ENetMode NetMode);
	static bool IsManualEvidenceSessionAllowed(
		FName Role,
		FName InputScriptId,
		bool bUnattended,
		bool bNullRHI,
		bool bRenderOffscreen,
		bool bNoSound,
		bool bAutoStart,
		bool bTestOverride,
		ENetMode NetMode);
	FName GetActiveManualEvidenceRole() const { return ActiveManualEvidenceRole; }
	FName GetManualEvidenceSessionRejectionReason() const
	{
		return ManualEvidenceSessionRejectionReason;
	}
	static FW11ManualEvidenceProgress ResolveManualEvidenceProgress(
		FName Role,
		const FW11CombatTelemetry& Telemetry);
	FW11ManualEvidenceProgress GetManualEvidenceProgress() const
	{
		return ResolveManualEvidenceProgress(ActiveManualEvidenceRole, CombatTelemetry);
	}
	static void ClassifyCombatParticipant(
		int32 PlayerId,
		bool bDefeated,
		TArray<int32>& DefeatedPlayerIds,
		TArray<int32>& SurvivingPlayerIds);

	void RecordCombatDamage(AActor* Target, const FW11DamageSpec& Spec, const FW11DamageBreakdown& Damage);
	void RecordAbilityUse(FName AbilityId, float ManaCost);
	void RecordAbilityExecutionResult(const FW11AbilityExecutionResult& Result, int32 TargetCount);
	void RecordAbilityFailure(EW11AbilityRejectionReason Reason);
	void RecordManaRestored(float Amount);
	void RecordDodge();
	void RecordRecoveryPunishHit(FName EnemyId, int32 StableSpawnId, FName AbilityId, float HealthDamage);
	static bool IsRecoveryPunishHit(EW11EnemyCombatState EnemyState, float HealthDamage);
	void RecordEnemySpawn(FName EnemyId);
	void RecordEnemyAttack(FName EnemyId);
	void RecordEnemyHit(FName EnemyId, bool bHadWarning = true);
	void RecordEnemyAttackEvaded(FName EnemyId);
	void RecordEnemyAttackCancelled(FName EnemyId);
	void RecordCancelledThreat();

	void RestartRunToLobby(AW11PlayerController* RequestingController);

	void HandleCultivationSelection(AW11PlayerController* Controller, int32 OfferIndex);
	void HandleShopPurchase(AW11PlayerController* Controller, int32 OfferIndex);
	void HandleEquipAbility(AW11PlayerController* Controller, EW11AbilitySlot Slot, FName AbilityId);
	void HandleReadyForNextStage(AW11PlayerController* Controller, bool bReady);

	UFUNCTION(BlueprintPure, Category="W11|Run")
	AW11RunDirector* GetRunDirector() const { return RunDirector; }

	/** Observation-only terminal event. Binding it never claims transition ownership. */
	UPROPERTY(BlueprintAssignable, Category="W11|Chapter")
	FW11CombatNodeCompleted OnCombatNodeCompleted;

	/** A chapter router must explicitly claim transition ownership before combat finalizes. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Chapter")
	bool ClaimChapterTransitionOwnership(UObject* RequestingOwner);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Chapter")
	void ReleaseChapterTransitionOwnership(UObject* RequestingOwner);

	UFUNCTION(BlueprintPure, Category="W11|Chapter")
	bool HasChapterTransitionOwner() const { return ChapterTransitionOwner.IsValid(); }

	static bool ShouldChapterRouterOwnTransition(bool bHasExplicitOwner, bool bHasObservers);

	/** Future snapshot/archive systems bind here and finish synchronous hand-off before reset. */
	UPROPERTY(BlueprintAssignable, Category="W11|Persistence")
	FW11BeforeMemoryRunReset OnBeforeMemoryRunReset;

private:
	void InitializePlayerState(AW11PlayerState* PlayerState);
	void EnsureFallbackCharacterSelection(AW11PlayerState* PlayerState) const;
	UW11HeroDefinition* FindHeroDefinition(FName HeroId) const;
	UW11SectDefinition* FindSectDefinition(FName SectId) const;
	void EnterCultivationOrMarket();
	void EnterMarket();
	void StartNextCombatStage();
	void EnterCurrentChapterNode();
	void AdvanceFormalRouteAfterCombat();
	void StartFormalEncounter(UW11EncounterDefinition* Encounter);
	void AutoAdvanceFormalChapterNode();
	void AutoResumeActiveRun();
	void RestorePendingSafeNodeSnapshot();
	void AutoRestartAfterDefeat();

	UFUNCTION()
	void HandleNodeObserverSmoke(const FW11CombatNodeResult& Result);
	bool AreAllPlayersDoneWithCultivation() const;
	bool AreAllPlayersReady() const;
	TArray<AW11PlayerState*> GetW11PlayerStates() const;
	int32 ResolveRunSeed() const;
	void ResetCombatTelemetry(int32 EncounterInstanceId);
	void RecordRewardGrant(int32 PlayerId);
	void EmitCombatTelemetry(EW11CombatOutcome Outcome, FName Reason) const;
	void ScheduleAutomatedOutcomeExit(EW11CombatOutcome Outcome, FName Reason);
	void EmitManualEvidenceProgressIfChanged();

	UFUNCTION()
	void HandlePlayerDefeated();

	UPROPERTY(EditDefaultsOnly, Category="W11|Run")
	TSoftObjectPtr<UW11RunRuleSet> RuleSetAsset;

	UPROPERTY(Transient)
	TObjectPtr<UW11RunRuleSet> ActiveRuleSet;

	UPROPERTY(Transient)
	TObjectPtr<AW11RunDirector> RunDirector;

	UPROPERTY(Transient)
	TObjectPtr<UW11ChapterRouteSubsystem> ChapterRouteSubsystem;

	bool bRunStarted = false;
	bool bFormalChapterRouteActive = false;
	bool bResumeRequested = false;
	FW11ActiveRunSnapshot PendingResumeSnapshot;
	FW11CombatTelemetry CombatTelemetry;
	FW11CombatNodeResult LastCombatNodeResult;
	bool bHasLastCombatNodeResult = false;

	UPROPERTY(Transient)
	TWeakObjectPtr<UObject> ChapterTransitionOwner;
	FName ActiveManualEvidenceRole = NAME_None;
	FName ManualEvidenceSessionRejectionReason = NAME_None;
	FString LastManualEvidenceProgressSignature;
};
