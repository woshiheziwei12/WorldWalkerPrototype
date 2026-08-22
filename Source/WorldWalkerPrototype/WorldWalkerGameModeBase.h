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
class AFantasyAmbientSoundscape;
class AFantasyWorldLayout;
class ASpiralTowerWorldLayout;
class AFantasyWorldChoiceActor;
class UCardCombatComponent;
class UCardDefinition;
class UFantasyEnemyDefinition;
class UFantasyEnemyDeckRuntime;
class UFantasyBlessingDefinition;
class UFantasyCardProgressionSubsystem;
class UTexture2D;
struct FFantasyEnemyIntentStep;
enum class EFantasyMechanicTrigger : uint8;
enum class EFantasyBlessingTrigger : uint8;
class UWorldDefinition;

UCLASS()
class WORLDWALKERPROTOTYPE_API AWorldWalkerGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWorldWalkerGameModeBase();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void StartCombat(AWorldWalkerCharacter* PlayerCharacter, AWorldWalkerEnemy* EnemyCharacter);
	void HandlePlayCard(int32 HandIndex);
	void HandleEndPlayerTurn();
	void HandleRewardSelection(int32 RewardIndex);
	void HandleRewardSkip();
	void HandleReturnToExploration();
	void HandleProfessionSelection(int32 ChoiceIndex);
	void HandleDeckViewToggle();
	void HandleRouteSelection(int32 ChoiceIndex);
	void HandleEventSelection(int32 ChoiceIndex);
	void HandleNodeResolutionContinue();
	void RestartDemo();
	const FString& GetExplorationMessage() const { return ExplorationMessage; }

private:
	void InitializeWorldContent();
	void SpawnRegisteredWorldRoot();
	void SpawnMainWorldHub(const TArray<UWorldDefinition*>& DestinationWorlds);
	void SpawnTestEnemy();
	void SpawnFantasyWorldLayout();
	void SpawnFantasyBattleArena();
	void SpawnSpiralTowerWorldLayout();
	void SpawnPortal(UWorldDefinition* DestinationWorld, const FVector& OffsetFromPlayer);
	bool LoadFantasyEnemyDefinition(FName EnemyId);
	void InitializeFantasyRun();
	void PresentProfessionChoices();
	void BeginSelectedProfessionRun();
	void ResumeOrPresentFantasyRun();
	void PresentRouteChoices();
	void SpawnWorldChoices(
		bool bEventChoices,
		const TArray<FText>& Titles,
		const TArray<FText>& Descriptions,
		const TArray<FLinearColor>& Colors);
	void DestroyWorldChoices();
	void ActivateRouteNode(const FFantasyRouteNodeChoice& Node);
	void PresentEventChoices(FName EventId);
	void CompleteActiveNode(const FString& ResolutionMessage);
	void PrepareCombatNode(FName EnemyId);
	void ClearActiveEnemy();
	void HandleEnemyTurn();
	void HandleEnemyTurnStep();
	void FinishEnemyTurnSequence();
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
	void DispatchEnemyMechanics(
		EFantasyMechanicTrigger Trigger,
		const UCardDefinition* SourceCard = nullptr,
		int32 ActualDamage = 0,
		int32 TargetBlockBefore = 0);
	void ExecuteEnemyMechanicEffects(
		const TArray<FFantasyCombatEffectSpec>& Effects,
		const UCardDefinition* SourceCard = nullptr);
	void LoadBlessingDefinitions();
	UFantasyBlessingDefinition* SelectAvailableBlessing(FName StreamName);
	void DispatchPlayerBlessings(
		EFantasyBlessingTrigger Trigger,
		const UCardDefinition* SourceCard = nullptr,
		int32 ActualDamage = 0);
	void ExecutePlayerBlessingEffects(const TArray<FFantasyCombatEffectSpec>& Effects);
	bool TryTriggerEnemyDefeatPassive();
	void RestoreRunHealthToPlayer();
	void SyncRunHealthFromPlayer();
	FString BuildNextIntentText() const;
	void FinishCombat(bool bPlayerWon);
	void BeginVictoryReward();
	void InitializeM0RunAutomation();
	void HandleM0RunAutomationStep();
	void FailM0RunAutomation(const FString& Reason);
	void RefreshCombatUI() const;
	AFantasyAmbientSoundscape* GetFantasySoundscape() const;
	UTexture2D* GetEnemyPortraitTexture() const;
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
	TObjectPtr<ASpiralTowerWorldLayout> ActiveSpiralTowerWorld;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AFantasyWorldChoiceActor>> ActiveWorldChoices;

	UPROPERTY(Transient)
	TObjectPtr<UFantasyEnemyDefinition> ActiveFantasyEnemyDefinition;

	UPROPERTY(Transient)
	TObjectPtr<UFantasyEnemyDeckRuntime> ActiveEnemyDeck;

	UPROPERTY(Transient)
	TObjectPtr<UWorldDefinition> CurrentWorldDefinition;

	UPROPERTY(Transient)
	TObjectPtr<AActor> ActiveWorldRoot;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCardDefinition>> PendingRewardChoices;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCardDefinition>> PendingShopCards;

	UPROPERTY(Transient)
	TObjectPtr<UCardDefinition> PendingTreasureCard;

	UPROPERTY(Transient)
	TObjectPtr<UFantasyBlessingDefinition> PendingEventBlessing;

	FTimerHandle EnemyTurnTimer;
	FTimerHandle M0RunAutomationTimer;
	TArray<FString> CurrentEnemyTurnCardNames;
	FString ExplorationMessage = TEXT("WASD Move | Mouse Look | Space Jump | E Interact");
	FFantasyCombatRuntimeState PlayerFantasyState;
	FFantasyCombatRuntimeState EnemyFantasyState;
	FFantasyCombatTiming CombatTiming;
	int32 CurrentEnemyIntentIndex = 0;
	int32 CurrentEnemyTurnNumber = 0;
	TMap<FName, int32> MechanicTurnTriggerCounts;
	TMap<FName, int32> MechanicBattleTriggerCounts;
	TMap<FName, int32> MechanicSuppressedTurns;
	bool bEnemyFirstPlayerCardImmune = false;
	TMap<FName, TObjectPtr<UFantasyBlessingDefinition>> BlessingDefinitions;
	TMap<FName, int32> BlessingBattleTriggerCounts;
	FName CurrentEventId;
	FString PendingEventFeedback;
	int32 PendingEventChoiceCount = 0;
	bool bEnemyDefeatPassiveConsumed = false;
	bool bEnemyReactivePassiveTriggered = false;
	bool bM0RunAutomationEnabled = false;
	bool bM0AutomationRestartPending = false;
	int32 M0AutomationDeckViewPhase = 0;
	int32 M0AutomationRewardCount = 0;
	int32 M0AutomationStepCount = 0;
	EFantasyPlayerProfession M0AutomationProfession = EFantasyPlayerProfession::Mage;

	enum class EFantasyRunFlowState : uint8
	{
		Exploration,
		ProfessionChoice,
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
	bool bRewardWasSkipped = false;
	bool bDeckViewerOpen = false;
	bool bRestoreMovementAfterDeckViewer = false;
};
