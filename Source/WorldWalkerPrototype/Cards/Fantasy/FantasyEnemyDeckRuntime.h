#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FantasyEnemyDeckRuntime.generated.h"

class UCardDefinition;
class UFantasyEnemyDefinition;

/**
 * Transient card-zone state for one enemy battle.
 *
 * Definitions own authored deck composition. This object owns only the live
 * draw/hand/discard/exhaust/equipment/counter zones and combat resources; the
 * GameMode remains authoritative for resolving card effects against combatants.
 */
UCLASS(BlueprintType)
class WORLDWALKERPROTOTYPE_API UFantasyEnemyDeckRuntime : public UObject
{
	GENERATED_BODY()

public:
	bool Initialize(UFantasyEnemyDefinition* InDefinition);

	/** Refreshes action points, keeps mana, discards non-retained cards, then fills the hand. */
	bool StartTurn();

	/** Returns the first affordable card without changing any zone or resource. */
	UCardDefinition* GetNextPlayableCard() const;

	/** Removes the next card from hand and pays its costs. Call FinalizeCard after resolving effects. */
	UCardDefinition* PlayNextCard();

	/** Moves the pending played card into discard, exhaust, or the persistent equipment zone. */
	bool FinalizeCard(UCardDefinition* Card);

	int32 AddAction(int32 Amount);
	int32 AddMana(int32 Amount);
	/** Draws up to the current enemy hand limit. Used by resolved Draw card effects. */
	int32 DrawCards(int32 Count);
	/** Draws without the current-turn hand cap. Reserved for delayed Counter resolution. */
	int32 DrawCardsIgnoringHandLimit(int32 Count);
	int32 DiscardRandom(int32 Count);
	/** Removes and returns the oldest armed Counter for authoritative GameMode resolution. */
	UCardDefinition* ConsumeNextCounter();
	/** Moves the just-resolved Counter into discard after all of its effects finish. */
	bool FinalizeTriggeredCounter(UCardDefinition* Card);

	int32 GetDrawPileCount() const { return DrawPile.Num(); }
	int32 GetHandCount() const { return Hand.Num(); }
	int32 GetDiscardPileCount() const { return DiscardPile.Num(); }
	int32 GetExhaustPileCount() const { return ExhaustPile.Num(); }
	int32 GetEquipmentCount() const { return EquipmentZone.Num(); }
	int32 GetCounterCount() const { return CounterZone.Num(); }
	int32 GetCurrentActionPoints() const { return CurrentActionPoints; }
	int32 GetCurrentMana() const { return CurrentMana; }
	int32 GetCardsPlayedThisTurn() const { return CardsPlayedThisTurn; }
	int32 GetAttackBonus() const;
	bool IsInitialized() const { return Definition != nullptr && InitialDeckSize > 0; }
	bool HasPendingCard() const { return PendingPlayedCard != nullptr; }

	const TArray<TObjectPtr<UCardDefinition>>& GetHand() const { return Hand; }
	const TArray<TObjectPtr<UCardDefinition>>& GetEquipmentZone() const { return EquipmentZone; }
	UFantasyEnemyDefinition* GetDefinition() const { return Definition; }

	/** Human-readable exact preview for the next currently affordable card. */
	FString BuildPreview(int32 CurrentStrength = 0) const;

private:
	bool IsCardPlayable(const UCardDefinition* Card) const;
	void RefillDrawPile();
	void ShuffleDrawPile();

	UPROPERTY(Transient)
	TObjectPtr<UFantasyEnemyDefinition> Definition;

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

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCardDefinition>> CounterZone;

	UPROPERTY(Transient)
	TObjectPtr<UCardDefinition> PendingPlayedCard;

	UPROPERTY(Transient)
	TObjectPtr<UCardDefinition> PendingCounterCard;

	int32 InitialDeckSize = 0;
	int32 CurrentActionPoints = 0;
	int32 CurrentMana = 0;
	int32 CurrentTurnHandLimit = 0;
	int32 CardsPlayedThisTurn = 0;

	static constexpr int32 MaxEquipmentSlots = 3;
	static constexpr int32 MaxCounterSlots = 3;
};
