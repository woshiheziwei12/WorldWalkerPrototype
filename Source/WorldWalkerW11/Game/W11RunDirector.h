#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/W11Types.h"
#include "W11RunDirector.generated.h"

class AW11PlayerState;
class UW11ManualDefinition;
class UW11AbilityDefinition;
class UW11RunRuleSet;
class UW11EncounterDefinition;
class UW11ShopServiceDefinition;
class UW11StatChoiceDefinition;
class UW11TreasureDefinition;

UCLASS()
class WORLDWALKERW11_API AW11RunDirector : public AActor
{
	GENERATED_BODY()

public:
	AW11RunDirector();
	static uint32 GetStableNameHash(FName Value);
	static int32 CalculateCurrentWaveBudget(int32 RemainingBudget, int32 RemainingWaves, bool bLastWave);
	static bool ShouldRefillCurrentWave(
		int32 ActiveEnemies,
		int32 CurrentWaveBudgetRemaining,
		int32 EncounterBudgetRemaining,
		int32 ConcurrentCap);

	void Initialize(UW11RunRuleSet* InRuleSet, int32 InRunSeed);

	TArray<FW11CultivationOffer> GenerateCultivationOffers(const AW11PlayerState* PlayerState) const;
	TArray<FW11ShopOffer> GenerateShopOffers(const AW11PlayerState* PlayerState, int32 StageIndex) const;

	const UW11ManualDefinition* FindManual(FName DefinitionId) const;
	const UW11TreasureDefinition* FindTreasure(FName DefinitionId) const;
	const UW11ShopServiceDefinition* FindService(FName DefinitionId) const;
	const UW11AbilityDefinition* FindAbility(FName DefinitionId) const;

	void StartStage(int32 StageIndex, int32 EncounterInstanceId,
		UW11EncounterDefinition* OverrideEncounter = nullptr);
	void NotifyEnemyDefeated(const class AW11Enemy* Enemy);
	void CancelActiveEncounter(int32 EncounterInstanceId);
	void SignalExternalObjectiveCompleted(FName ObjectiveId);

	UFUNCTION(BlueprintPure, Category="W11|Run")
	int32 GetActiveEnemyCount() const { return ActiveEnemyCount; }

	UFUNCTION(BlueprintPure, Category="W11|Run")
	UW11RunRuleSet* GetRuleSet() const { return RuleSet; }
	FName GetActiveEncounterId() const;
	EW11EncounterType GetActiveEncounterType() const;
	uint32 GetCandidateSignature() const { return CandidateSignature; }
	uint32 GetSpawnSignature() const { return SpawnSignature; }

private:
	void SpawnNextWave();
	void SpawnCurrentWaveEnemies(bool bReinforcement);
	void CompleteActiveObjective(FName Reason);
	void ResolveWaveForAutomation();
	FVector ChooseSafeSpawnLocation(FRandomStream& Stream, float ArenaPawnZ) const;
	FRandomStream MakeStream(const AW11PlayerState* PlayerState, int32 StageIndex, FName Purpose) const;
	void BuildFallbackCultivationOffers(TArray<FW11CultivationOffer>& OutOffers, FRandomStream& Stream) const;

	UPROPERTY(Transient)
	TObjectPtr<UW11RunRuleSet> RuleSet;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UW11StatChoiceDefinition>> StatChoices;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UW11ManualDefinition>> Manuals;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UW11TreasureDefinition>> Treasures;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UW11ShopServiceDefinition>> Services;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UW11AbilityDefinition>> Abilities;

	int32 RunSeed = 0;
	int32 ActiveStageIndex = 0;
	int32 ActiveEnemyCount = 0;
	int32 ActiveEncounterInstanceId = 0;
	FW11BattleReward ActiveReward;

	UPROPERTY(Transient)
	TObjectPtr<class UW11EncounterDefinition> ActiveEncounter;

	int32 ActiveWaveIndex = 0;
	int32 ActiveWaveBudgetRemaining = 0;
	int32 ActiveWaveSpawnOrdinal = 0;
	int32 NextStableSpawnId = 1;
	int32 RemainingEncounterBudget = 0;
	float ActiveArenaPawnZ = 96.0f;
	FRandomStream ActiveWaveStream;
	uint32 CandidateSignature = 0;
	uint32 SpawnSignature = 0;
	bool bObjectiveCompleted = false;
	FTimerHandle WaveTimerHandle;
	FTimerHandle SurvivalTimerHandle;
	FTimerHandle AutomationWaveResolveTimerHandle;
};
