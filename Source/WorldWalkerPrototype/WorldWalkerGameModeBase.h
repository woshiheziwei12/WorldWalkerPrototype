#pragma once

#include "Cards/Fantasy/FantasyCombatTypes.h"
#include "Cards/Fantasy/FantasyRunTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WorldWalkerGameModeBase.generated.h"

class AWorldWalkerCharacter;
class AWorldWalkerEnemy;
class AWorldWalkerPlayerController;
class AWorldHubLayout;
class AWorldPortal;
class AFantasyBattleArena;
class AFantasyWorldLayout;
class UCardCombatComponent;
class UCardDefinition;
class UFantasyEnemyDefinition;
class UFantasyEnemyDeckRuntime;
class UFantasyCardProgressionSubsystem;
struct FFantasyEnemyIntentStep;
class UWorldDefinition;

UCLASS()
class WORLDWALKERPROTOTYPE_API AWorldWalkerGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWorldWalkerGameModeBase();

	virtual void BeginPlay() override;

	void StartCombat(AWorldWalkerCharacter* PlayerCharacter, AWorldWalkerEnemy* EnemyCharacter);
	void HandlePlayCard(int32 HandIndex);
	void HandleEndPlayerTurn();
	void HandleRewardSelection(int32 RewardIndex);
	void HandleReturnToExploration();
	void HandleRouteSelection(int32 ChoiceIndex);
	void HandleEventSelection(int32 ChoiceIndex);
	void HandleNodeResolutionContinue();
	void RestartDemo();
	const FString& GetExplorationMessage() const { return ExplorationMessage; }

private:
	void InitializeWorldContent();
	void SpawnMainWorldHub(UWorldDefinition* DestinationWorld);
	void SpawnTestEnemy();
	void SpawnFantasyWorldLayout();
	void SpawnFantasyBattleArena();
	void SpawnPortal(UWorldDefinition* DestinationWorld, const FVector& OffsetFromPlayer);
	bool LoadFantasyEnemyDefinition(FName EnemyId);
	void InitializeFantasyRun();
	void ResumeOrPresentFantasyRun();
	void PresentRouteChoices();
	void ActivateRouteNode(const FFantasyRouteNodeChoice& Node);
	void PresentEventChoices(FName EventId);
	void CompleteActiveNode(const FString& ResolutionMessage);
	void PrepareCombatNode(FName EnemyId);
	void ClearActiveEnemy();
	void HandleEnemyTurn();
	void ResolvePlayerCardEffects(UCardDefinition* Card);
	void ResolveEnemyIntent(const FFantasyEnemyIntentStep& Intent);
	void ResolveEnemyCardEffects(UCardDefinition* Card);
	int32 ResolveDamageAgainstEnemy(
		int32 BaseDamage,
		bool bIsAttack,
		bool bConsumeWeak,
		bool bPiercing = false);
	int32 ResolveDamageAgainstPlayer(
		int32 BaseDamage,
		bool bIsAttack,
		bool bConsumeWeak,
		bool bPiercing);
	void ResolveEndOfTurnPoison(bool bPlayerTurnEnded);
	void ApplyEnemyTurnStartEquipment();
	bool TryTriggerEnemyDefeatPassive();
	void RestoreRunHealthToPlayer();
	void SyncRunHealthFromPlayer();
	FString BuildNextIntentText() const;
	void FinishCombat(bool bPlayerWon);
	void BeginVictoryReward();
	void RefreshCombatUI() const;
	AWorldWalkerPlayerController* GetWorldWalkerController() const;

	UPROPERTY(Transient)
	TObjectPtr<AWorldWalkerCharacter> ActivePlayer;

	UPROPERTY(Transient)
	TObjectPtr<AWorldWalkerEnemy> ActiveEnemy;

	UPROPERTY(Transient)
	TObjectPtr<UCardCombatComponent> ActiveCardCombat;

	UPROPERTY(Transient)
	TObjectPtr<AWorldPortal> ActivePortal;

	UPROPERTY(Transient)
	TObjectPtr<AWorldHubLayout> ActiveHub;

	UPROPERTY(Transient)
	TObjectPtr<AFantasyBattleArena> ActiveFantasyArena;

	UPROPERTY(Transient)
	TObjectPtr<AFantasyWorldLayout> ActiveFantasyWorld;

	UPROPERTY(Transient)
	TObjectPtr<UFantasyEnemyDefinition> ActiveFantasyEnemyDefinition;

	UPROPERTY(Transient)
	TObjectPtr<UFantasyEnemyDeckRuntime> ActiveEnemyDeck;

	UPROPERTY(Transient)
	TObjectPtr<UWorldDefinition> CurrentWorldDefinition;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCardDefinition>> PendingRewardChoices;

	FTimerHandle EnemyTurnTimer;
	FString ExplorationMessage = TEXT("WASD Move | Mouse Look | Space Jump | E Interact");
	FFantasyCombatRuntimeState PlayerFantasyState;
	FFantasyCombatRuntimeState EnemyFantasyState;
	int32 CurrentEnemyIntentIndex = 0;
	int32 CurrentEnemyTurnNumber = 0;
	FName CurrentEventId;
	bool bEnemyDefeatPassiveConsumed = false;
	bool bEnemyReactivePassiveTriggered = false;

	enum class EFantasyRunFlowState : uint8
	{
		Exploration,
		RouteChoice,
		EventChoice,
		PlayerTurn,
		EnemyTurn,
		RewardChoice,
		RewardConfirmed,
		Resolution,
		ChapterComplete,
		Defeat
	};

	EFantasyRunFlowState FantasyRunFlowState = EFantasyRunFlowState::Exploration;
	bool bCombatActive = false;
	bool bWaitingForEnemy = false;
	bool bAwaitingRewardSelection = false;
	bool bRewardReadyToLeave = false;
};
