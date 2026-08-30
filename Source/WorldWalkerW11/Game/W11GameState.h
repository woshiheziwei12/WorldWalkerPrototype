#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Core/W11Types.h"
#include "W11GameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FW11GameStateChanged);

UCLASS()
class WORLDWALKERW11_API AW11GameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AW11GameState();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeRun(int32 InRunSeed, FName InRuleSetId);
	void SetRunPhase(EW11RunPhase NewPhase, float DurationSeconds = 0.0f);
	void SetStageIndex(int32 NewStageIndex);
	void SetChapterRouteState(int32 InChapterIndex, int32 InNodeIndex,
		EW11ChapterNodeType InNodeType, FName InChapterId, FName InNodeId);
	void SetRemainingEnemyCount(int32 NewCount);
	void SetEncounterObjective(FName EncounterId, EW11EncounterType Type, int32 CurrentWave, int32 WaveCount, float ObjectiveEndServerTime);
	int32 BeginEncounter();
	bool TrySetCombatOutcome(int32 ExpectedEncounterInstanceId, EW11CombatOutcome NewOutcome, FName Reason);
	static bool CanTransitionCombatOutcome(
		int32 CurrentEncounterInstanceId,
		int32 ExpectedEncounterInstanceId,
		EW11CombatOutcome CurrentOutcome,
		EW11CombatOutcome NewOutcome);
	static bool IsPendingCombatEncounter(
		EW11RunPhase RunPhase,
		EW11CombatOutcome CurrentOutcome,
		int32 CurrentEncounterInstanceId,
		int32 ExpectedEncounterInstanceId);
	static int32 AdvanceAuthoritySequence(int32 CurrentSequence);
	static int32 AdvanceMemoryRunSequence(
		int32 CurrentMemoryRunSequence,
		int32& InOutEncounterSequence,
		int32& InOutOutcomeSequence);

	UFUNCTION(BlueprintPure, Category="W11|Run")
	EW11RunPhase GetRunPhase() const { return RunPhase; }

	UFUNCTION(BlueprintPure, Category="W11|Run")
	int32 GetRunSeed() const { return RunSeed; }

	UFUNCTION(BlueprintPure, Category="W11|Run")
	int32 GetMemoryRunSequence() const { return MemoryRunSequence; }

	UFUNCTION(BlueprintPure, Category="W11|Run")
	int32 GetStageIndex() const { return StageIndex; }

	UFUNCTION(BlueprintPure, Category="W11|Chapter")
	int32 GetChapterIndex() const { return ChapterIndex; }

	UFUNCTION(BlueprintPure, Category="W11|Chapter")
	int32 GetChapterNodeIndex() const { return ChapterNodeIndex; }

	UFUNCTION(BlueprintPure, Category="W11|Chapter")
	EW11ChapterNodeType GetChapterNodeType() const { return ChapterNodeType; }

	UFUNCTION(BlueprintPure, Category="W11|Chapter")
	FName GetChapterId() const { return ChapterId; }

	UFUNCTION(BlueprintPure, Category="W11|Chapter")
	FName GetChapterNodeId() const { return ChapterNodeId; }

	UFUNCTION(BlueprintPure, Category="W11|Run")
	float GetPhaseTimeRemaining() const;

	UFUNCTION(BlueprintPure, Category="W11|Run")
	FName GetRuleSetId() const { return RuleSetId; }

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	int32 GetEncounterInstanceId() const { return EncounterInstanceId; }

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	EW11CombatOutcome GetCombatOutcome() const { return CombatOutcome; }

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	FName GetOutcomeReason() const { return OutcomeReason; }

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	int32 GetOutcomeSequence() const { return OutcomeSequence; }

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	int32 GetRemainingEnemyCount() const { return RemainingEnemyCount; }

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	FName GetActiveEncounterId() const { return ActiveEncounterId; }

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	EW11EncounterType GetEncounterType() const { return EncounterType; }

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	int32 GetCurrentWave() const { return CurrentWave; }

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	int32 GetWaveCount() const { return WaveCount; }

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	float GetObjectiveTimeRemaining() const;

	/** Replicated run/encounter facts changed; UI can refresh without guessing them in Tick. */
	UPROPERTY(BlueprintAssignable, Category="W11|Run")
	FW11GameStateChanged OnRuntimeStateChanged;

private:
	void BroadcastRuntimeStateChanged();

	UFUNCTION()
	void OnRep_RuntimeState();

	UFUNCTION()
	void OnRep_CombatOutcomeSnapshot();

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Run")
	EW11RunPhase RunPhase = EW11RunPhase::Lobby;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Run")
	int32 RunSeed = 0;

	/** Distinguishes repeated in-memory runs even when they intentionally reuse the same Seed. */
	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Run")
	int32 MemoryRunSequence = 0;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Run")
	int32 StageIndex = 0;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Chapter")
	int32 ChapterIndex = 0;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Chapter")
	int32 ChapterNodeIndex = 0;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Chapter")
	EW11ChapterNodeType ChapterNodeType = EW11ChapterNodeType::Combat;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Chapter")
	FName ChapterId = NAME_None;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Chapter")
	FName ChapterNodeId = NAME_None;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Run")
	float PhaseEndServerTime = 0.0f;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Run")
	FName RuleSetId = NAME_None;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Combat")
	int32 EncounterInstanceId = 0;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Combat")
	EW11CombatOutcome CombatOutcome = EW11CombatOutcome::Pending;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Combat")
	FName OutcomeReason = NAME_None;

	UPROPERTY(ReplicatedUsing=OnRep_CombatOutcomeSnapshot, VisibleAnywhere, Category="W11|Combat")
	int32 OutcomeSequence = 0;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Combat")
	int32 RemainingEnemyCount = 0;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Combat")
	FName ActiveEncounterId = NAME_None;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Combat")
	EW11EncounterType EncounterType = EW11EncounterType::Clear;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Combat")
	int32 CurrentWave = 0;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Combat")
	int32 WaveCount = 0;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleAnywhere, Category="W11|Combat")
	float ObjectiveEndServerTime = 0.0f;
};
