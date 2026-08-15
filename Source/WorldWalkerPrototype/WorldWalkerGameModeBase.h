#pragma once

#include "Cards/Fantasy/FantasyCombatTypes.h"
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
	void RestartDemo();
	const FString& GetExplorationMessage() const { return ExplorationMessage; }

private:
	void InitializeWorldContent();
	void SpawnMainWorldHub(UWorldDefinition* DestinationWorld);
	void SpawnTestEnemy();
	void SpawnFantasyWorldLayout();
	void SpawnFantasyBattleArena();
	void SpawnPortal(UWorldDefinition* DestinationWorld, const FVector& OffsetFromPlayer);
	void LoadFantasyEnemyDefinition();
	void HandleEnemyTurn();
	void ResolvePlayerCardEffects(UCardDefinition* Card);
	void ResolveEnemyIntent(const FFantasyEnemyIntentStep& Intent);
	int32 ResolveDamageAgainstEnemy(int32 BaseDamage, bool bIsAttack, bool bConsumeWeak);
	int32 ResolveDamageAgainstPlayer(int32 BaseDamage, bool bConsumeWeak, bool& bPerfectBlock);
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
	TObjectPtr<UWorldDefinition> CurrentWorldDefinition;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCardDefinition>> PendingRewardChoices;

	FTimerHandle EnemyTurnTimer;
	FString ExplorationMessage = TEXT("WASD Move | Mouse Look | Space Jump | E Interact");
	FFantasyCombatRuntimeState PlayerFantasyState;
	FFantasyCombatRuntimeState EnemyFantasyState;
	int32 CurrentEnemyIntentIndex = 0;
	bool bCombatActive = false;
	bool bWaitingForEnemy = false;
	bool bAwaitingRewardSelection = false;
	bool bRewardReadyToLeave = false;
};
