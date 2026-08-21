#pragma once

#include "Cards/Fantasy/FantasyRunTypes.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FantasyCardProgressionSubsystem.generated.h"

class UCardDefinition;
class UFantasyChapterDefinition;

/**
 * Keeps the W01 classic-mode run alive while the player travels between maps.
 * Card/enemy definitions remain the content source of truth; this subsystem
 * stores only stable IDs, deck deltas, and the current chapter route state.
 */
UCLASS()
class WORLDWALKERPROTOTYPE_API UFantasyCardProgressionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 DefaultRouteSeed = 104729;
	static constexpr int32 StructuredEventSchemaVersion = 2;
	static const FString DefaultContentVersion;

	bool SelectProfession(EFantasyPlayerProfession Profession);
	bool HasSelectedProfession() const { return SelectedProfession != EFantasyPlayerProfession::None; }
	EFantasyPlayerProfession GetSelectedProfession() const { return SelectedProfession; }
	FString GetProfessionDisplayName() const;
	void EnsureRunStarted();
	void ResetRun();
	/** May only change the reproducibility contract before a run starts. */
	bool ConfigureRun(
		int32 InRouteSeed,
		const FString& InContentVersion,
		EFantasyRunDifficulty InDifficulty = EFantasyRunDifficulty::Normal);
	int32 GetRouteSeed() const { return RouteSeed; }
	const FString& GetContentVersion() const { return ContentVersion; }
	const FString& GetBuildVersion() const { return BuildVersion; }
	const FString& GetRunId() const { return RunId; }
	EFantasyRunDifficulty GetDifficulty() const { return Difficulty; }
	FString GetDifficultyName() const;
	EFantasyRunSource GetRunSource() const { return RunSource; }
	EFantasyRunEndReason GetEndReason() const { return EndReason; }
	void EndRun(EFantasyRunEndReason Reason, FName EnemyId = NAME_None);
	FString BuildRouteChoiceSignature() const;
	/** Returns the next deterministic seed for an isolated named random stream. */
	int32 ConsumeDeterministicSeed(FName StreamName);
	int32 BuildDeterministicSeed(FName StreamName, int32 Sequence) const;
	void LogStructuredEvent(
		const FString& EventType,
		const TMap<FString, FString>& StringFields = {},
		const TMap<FString, int64>& NumberFields = {}) const;

	bool GrantCard(const UCardDefinition* Card);
	bool RemoveCardCopy(FName CardId, int32 UpgradeLevel = 0);
	bool UpgradeCardCopy(const UCardDefinition* BaseCard);
	int32 GetGrantedCopies(FName CardId, int32 UpgradeLevel = 0) const;
	int32 GetRemovedCopies(FName CardId, int32 UpgradeLevel = 0) const;
	int32 GetUpgradedFromCopies(FName CardId, int32 UpgradeLevel = 0) const;
	int32 GetUpgradedToCopies(FName CardId, int32 UpgradeLevel = 1) const;
	int32 GetTotalGrantedCopies() const;
	int32 GetTotalRemovedCopies() const;
	int32 GetDeletionCount() const { return DeletionCount; }
	void ReplaceDeckSnapshot(const TArray<UCardDefinition*>& Cards);
	int32 GetRunDeckCopies(FName CardId, int32 UpgradeLevel) const;
	const TArray<FFantasyRunCardCount>& GetRunDeck() const { return RunDeck; }
	int32 AddGold(int32 Amount);
	bool SpendGold(int32 Amount);
	int32 GetGold() const { return Gold; }
	bool GrantBlessing(FName BlessingId);
	const TArray<FName>& GetBlessings() const { return BlessingIds; }
	FString BuildRunSummary() const;
	void RecordRewardOffer(const FString& CandidateSignature);
	void RecordRewardSelection(
		FName CardId,
		bool bSkipped,
		int32 ChoiceIndex = INDEX_NONE);
	void RecordEventOffer(FName EventId, const FString& CandidateSignature);
	void RecordEventSelection(FName EventId, int32 ChoiceIndex);
	void RecordBattleStarted(FName EnemyId, int32 PlayerDeckCount);
	void RecordEnemyDefeated(FName EnemyId);
	FName GetLastEnemyId() const { return LastEnemyId; }
	const TArray<FName>& GetDefeatedEnemyIds() const { return DefeatedEnemyIds; }
	const TArray<FFantasyRunDecisionRecord>& GetDecisionHistory() const { return DecisionHistory; }

	const TArray<FFantasyRouteNodeChoice>& GetRouteChoices();
	bool SelectRouteChoice(int32 ChoiceIndex, FFantasyRouteNodeChoice& OutChoice);
	bool CompleteActiveNode();
	bool HasActiveNode() const { return bHasActiveNode; }
	const FFantasyRouteNodeChoice& GetActiveNode() const { return ActiveNode; }
	int32 GetChapterDepth() const { return ChapterDepth; }
	int32 GetTotalRouteDepths() const;
	bool IsChapterComplete() const { return bChapterComplete; }
	void AddPendingBattleBoon(int32 Block, int32 Valor);
	void ConsumePendingBattleBoon(int32& OutBlock, int32& OutValor);
	void RecordRunHealth(int32 CurrentHealth, int32 MaxHealth);
	int32 GetCurrentRunHealth() const { return CurrentRunHealth; }
	int32 GetRunMaxHealth() const { return RunMaxHealth; }

private:
	static FName MakeCardStateKey(FName CardId, int32 UpgradeLevel);
	static FString GetEndReasonName(EFantasyRunEndReason Reason);
	void ResolveRunConfiguration();
	void ResolveRunIdentityFields();
	bool LoadChapterDefinition();
	void RebuildRouteChoices();
	void BuildEmergencyRouteFallback();
	void AddDecisionRecord(
		FName EventType,
		const FString& CandidateSignature,
		FName SelectionId,
		int32 SelectionIndex = INDEX_NONE);
	void AddDeckCount(FName CardId, int32 UpgradeLevel, int32 Delta);

	UPROPERTY(Transient)
	TObjectPtr<UFantasyChapterDefinition> ChapterDefinition;

	UPROPERTY(Transient)
	TMap<FName, int32> GrantedCardCopies;

	UPROPERTY(Transient)
	TMap<FName, int32> RemovedCardCopies;

	UPROPERTY(Transient)
	TMap<FName, int32> UpgradedFromCardCopies;

	UPROPERTY(Transient)
	TMap<FName, int32> UpgradedToCardCopies;

	UPROPERTY(Transient)
	TArray<FFantasyRunCardCount> RunDeck;

	UPROPERTY(Transient)
	TArray<FName> BlessingIds;

	UPROPERTY(Transient)
	TArray<FName> DefeatedEnemyIds;

	UPROPERTY(Transient)
	TArray<FFantasyRunDecisionRecord> DecisionHistory;

	UPROPERTY(Transient)
	TArray<FFantasyRouteNodeChoice> RouteChoices;

	UPROPERTY(Transient)
	FFantasyRouteNodeChoice ActiveNode;

	EFantasyPlayerProfession SelectedProfession = EFantasyPlayerProfession::None;

	int32 ChapterDepth = 0;
	int32 CurrentChapter = 1;
	bool bRunStarted = false;
	bool bHasActiveNode = false;
	bool bChapterComplete = false;
	int32 PendingBattleBlock = 0;
	int32 PendingBattleValor = 0;
	int32 CurrentRunHealth = 100;
	int32 RunMaxHealth = 100;
	int32 Gold = 0;
	int32 DeletionCount = 0;
	int32 RouteSeed = DefaultRouteSeed;
	FString ContentVersion = DefaultContentVersion;
	FString BuildVersion;
	EFantasyRunDifficulty Difficulty = EFantasyRunDifficulty::Normal;
	EFantasyRunSource RunSource = EFantasyRunSource::Manual;
	EFantasyRunEndReason EndReason = EFantasyRunEndReason::None;
	FName LastEnemyId;
	bool bRunConfigurationResolved = false;
	bool bDeckSnapshotInitialized = false;
	TMap<FName, int32> RandomStreamSequences;
	FString RunId;
};
