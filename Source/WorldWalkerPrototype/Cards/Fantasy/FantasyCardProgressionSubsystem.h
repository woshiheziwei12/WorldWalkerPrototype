#pragma once

#include "Cards/Fantasy/FantasyRunTypes.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FantasyCardProgressionSubsystem.generated.h"

class UCardDefinition;

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
	static const FString DefaultContentVersion;

	bool SelectProfession(EFantasyPlayerProfession Profession);
	bool HasSelectedProfession() const { return SelectedProfession != EFantasyPlayerProfession::None; }
	EFantasyPlayerProfession GetSelectedProfession() const { return SelectedProfession; }
	FString GetProfessionDisplayName() const;
	void EnsureRunStarted();
	void ResetRun();
	/** May only change the reproducibility contract before a run starts. */
	bool ConfigureRun(int32 InRouteSeed, const FString& InContentVersion);
	int32 GetRouteSeed() const { return RouteSeed; }
	const FString& GetContentVersion() const { return ContentVersion; }
	FString BuildRouteChoiceSignature() const;

	bool GrantCard(const UCardDefinition* Card);
	bool RemoveCardCopy(FName CardId);
	int32 GetGrantedCopies(FName CardId) const;
	int32 GetRemovedCopies(FName CardId) const;
	int32 GetTotalGrantedCopies() const;
	int32 GetTotalRemovedCopies() const;
	FString BuildRunSummary() const;

	const TArray<FFantasyRouteNodeChoice>& GetRouteChoices();
	bool SelectRouteChoice(int32 ChoiceIndex, FFantasyRouteNodeChoice& OutChoice);
	bool CompleteActiveNode();
	bool HasActiveNode() const { return bHasActiveNode; }
	const FFantasyRouteNodeChoice& GetActiveNode() const { return ActiveNode; }
	int32 GetChapterDepth() const { return ChapterDepth; }
	int32 GetTotalRouteDepths() const { return 6; }
	bool IsChapterComplete() const { return bChapterComplete; }
	void AddPendingBattleBoon(int32 Block, int32 Valor);
	void ConsumePendingBattleBoon(int32& OutBlock, int32& OutValor);
	void RecordRunHealth(int32 CurrentHealth, int32 MaxHealth);
	int32 GetCurrentRunHealth() const { return CurrentRunHealth; }
	int32 GetRunMaxHealth() const { return RunMaxHealth; }

private:
	void ResolveRunConfiguration();
	void ApplyDeterministicRouteOrder();
	void RebuildRouteChoices();
	void AddRouteChoice(
		FName NodeId,
		const TCHAR* DisplayName,
		const TCHAR* Description,
		EFantasyRouteNodeType NodeType,
		FName PayloadId);

	UPROPERTY(Transient)
	TMap<FName, int32> GrantedCardCopies;

	UPROPERTY(Transient)
	TMap<FName, int32> RemovedCardCopies;

	UPROPERTY(Transient)
	TArray<FFantasyRouteNodeChoice> RouteChoices;

	UPROPERTY(Transient)
	FFantasyRouteNodeChoice ActiveNode;

	EFantasyPlayerProfession SelectedProfession = EFantasyPlayerProfession::None;

	int32 ChapterDepth = 0;
	bool bRunStarted = false;
	bool bHasActiveNode = false;
	bool bChapterComplete = false;
	int32 PendingBattleBlock = 0;
	int32 PendingBattleValor = 0;
	int32 CurrentRunHealth = 100;
	int32 RunMaxHealth = 100;
	int32 RouteSeed = DefaultRouteSeed;
	FString ContentVersion = DefaultContentVersion;
	bool bRunConfigurationResolved = false;
};
