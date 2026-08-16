#pragma once

#include "CoreMinimal.h"
#include "Cards/Fantasy/FantasyCombatTypes.h"
#include "Components/ActorComponent.h"
#include "CardCombatComponent.generated.h"

class UCardDefinition;

/** Owns player card zones, classic resources, equipment, Block, and legacy prototype resources. */
UCLASS(ClassGroup=(WorldWalker), meta=(BlueprintSpawnableComponent))
class WORLDWALKERPROTOTYPE_API UCardCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCardCombatComponent();

	/** Stable set ID used by the W01 fantasy world without renaming its existing world asset ID. */
	static const FName FantasyCardSetId;

	bool LoadStartingDeck();
	bool LoadStartingDeck(FName CardSetId);
	bool StartBattle();
	bool StartBattle(FName CardSetId);
	void StartPlayerTurn();
	void EndPlayerTurn();

	bool IsCardPlayable(int32 HandIndex) const;
	bool IsCardPlayable(const UCardDefinition* Card) const;
	/** Compatibility alias for existing callers. */
	bool CanPlayCard(int32 HandIndex) const;
	UCardDefinition* PlayCard(int32 HandIndex);
	/** Moves a card to discard/exhaust only after all of its effects and resonance draws resolve. */
	void FinalizePlayedCard(UCardDefinition* Card);
	bool HasAnyPlayableCard() const;
	/** Returns a shuffled, data-driven W01 reward offer without duplicate definitions. */
	TArray<UCardDefinition*> BuildRewardChoices(int32 ChoiceCount = 3) const;
	/** Persists one reward copy for this run and immediately adds it to the next battle deck. */
	bool GrantRewardCard(UCardDefinition* Card);
	/** Persists a player card granted by an exploration event, even when it is not reward-pool eligible. */
	bool GrantRunCard(UCardDefinition* Card);
	UCardDefinition* FindCardDefinition(FName CardId) const;
	/** Removes one concrete copy from the run deck, used by exploration events. */
	bool RemoveCardFromRun(FName CardId);

	/** Draws up to the current five-card turn limit and returns the number actually drawn. */
	int32 DrawCards(int32 Count);
	/** Adds non-negative Valor, clamps to MaxValor, and returns the amount actually gained. */
	int32 AddValor(int32 Amount);
	int32 AddActionPoints(int32 Amount);
	int32 AddMana(int32 Amount);
	int32 DiscardRandomCards(int32 Count);

	void AddBlock(int32 Amount);
	int32 AbsorbIncomingDamage(int32 DamageAmount);

	const TArray<TObjectPtr<UCardDefinition>>& GetHand() const { return Hand; }
	int32 GetCurrentEnergy() const { return CurrentEnergy; }
	int32 GetMaxEnergy() const { return MaxEnergy; }
	int32 GetCurrentActionPoints() const { return CurrentActionPoints; }
	int32 GetMaxActionPoints() const { return MaxActionPoints; }
	int32 GetCurrentMana() const { return CurrentMana; }
	int32 GetCurrentValor() const { return CurrentValor; }
	int32 GetMaxValor() const { return MaxValor; }
	int32 GetCurrentBlock() const { return CurrentBlock; }
	int32 GetDrawPileCount() const { return DrawPile.Num(); }
	int32 GetDiscardPileCount() const { return DiscardPile.Num(); }
	int32 GetExhaustPileCount() const { return ExhaustPile.Num(); }
	int32 GetEquipmentCount() const { return EquipmentZone.Num(); }
	int32 GetEquipmentAttackBonus() const;
	int32 GetStartingDeckCount() const { return StartingDeck.Num(); }
	int32 GetRunRewardCount() const;
	int32 GetRunRemovedCount() const;
	FName GetLoadedCardSetId() const { return LoadedCardSetId; }

	bool IsSchoolLit(ECardSchool School) const;
	FString GetSchoolSummary() const;
	bool LastCardTriggeredResonance() const { return bLastCardTriggeredResonance; }
	bool HasTriggeredResonanceThisTurn() const { return bResonanceTriggeredThisTurn; }

private:
	void RefillDrawPile();
	void ShuffleDrawPile();
	void RegisterPlayedSchool(ECardSchool School);
	static uint8 GetSchoolBit(ECardSchool School);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCardDefinition>> StartingDeck;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCardDefinition>> DrawPile;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCardDefinition>> Hand;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCardDefinition>> DiscardPile;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCardDefinition>> ExhaustPile;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCardDefinition>> EquipmentZone;

	UPROPERTY(EditDefaultsOnly, Category="Cards", meta=(ClampMin="1"))
	int32 MaxEnergy = 3;

	UPROPERTY(EditDefaultsOnly, Category="Cards|Classic", meta=(ClampMin="0"))
	int32 MaxActionPoints = 1;

	UPROPERTY(EditDefaultsOnly, Category="Cards|Classic", meta=(ClampMin="1"))
	int32 MaxEquipmentSlots = 3;

	UPROPERTY(EditDefaultsOnly, Category="Cards", meta=(ClampMin="1"))
	int32 MaxHandSize = 5;

	UPROPERTY(EditDefaultsOnly, Category="Cards", meta=(ClampMin="1"))
	int32 MaxValor = 3;

	FName LoadedCardSetId;
	int32 CurrentEnergy = 0;
	int32 CurrentActionPoints = 0;
	int32 CurrentMana = 0;
	int32 CurrentTurnHandLimit = 5;
	int32 CurrentValor = 0;
	int32 CurrentBlock = 0;
	uint8 LitSchoolMask = 0;
	bool bResonanceTriggeredThisTurn = false;
	bool bLastCardTriggeredResonance = false;
};
