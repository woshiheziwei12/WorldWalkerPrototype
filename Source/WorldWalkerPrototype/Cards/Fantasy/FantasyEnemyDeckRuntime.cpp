#include "Cards/Fantasy/FantasyEnemyDeckRuntime.h"

#include "Cards/CardDefinition.h"
#include "Cards/Fantasy/FantasyEnemyDefinition.h"

namespace
{
	void ShuffleCards(TArray<TObjectPtr<UCardDefinition>>& Cards)
	{
		for (int32 Index = Cards.Num() - 1; Index > 0; --Index)
		{
			Cards.Swap(Index, FMath::RandRange(0, Index));
		}
	}

	FString BuildCostPreview(const UCardDefinition* Card)
	{
		if (!Card)
		{
			return TEXT("无费用");
		}

		TArray<FString> Parts;
		if (Card->bUseClassicResources)
		{
			if (Card->ActionCost > 0)
			{
				Parts.Add(FString::Printf(TEXT("%d 行动力"), Card->ActionCost));
			}
			if (Card->ManaCost > 0)
			{
				Parts.Add(FString::Printf(TEXT("%d 法力"), Card->ManaCost));
			}
		}
		else if (Card->EnergyCost > 0)
		{
			Parts.Add(FString::Printf(TEXT("%d 行动力（兼容能量费用）"), Card->EnergyCost));
		}

		return Parts.IsEmpty() ? TEXT("0 费用") : FString::Join(Parts, TEXT(" + "));
	}
}

bool UFantasyEnemyDeckRuntime::Initialize(UFantasyEnemyDefinition* InDefinition)
{
	Definition = InDefinition;
	DrawPile.Reset();
	Hand.Reset();
	DiscardPile.Reset();
	ExhaustPile.Reset();
	EquipmentZone.Reset();
	CounterZone.Reset();
	PendingPlayedCard = nullptr;
	PendingCounterCard = nullptr;
	InitialDeckSize = 0;
	CurrentActionPoints = 0;
	CurrentMana = 0;
	CurrentTurnHandLimit = 0;
	CardsPlayedThisTurn = 0;

	if (!Definition)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot initialize W01 enemy deck: definition is null."));
		return false;
	}

	for (const FFantasyEnemyDeckEntry& Entry : Definition->Deck)
	{
		if (!Entry.Card || Entry.Copies <= 0)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("W01 enemy deck skipped invalid entry. Enemy=%s Card=%s Copies=%d"),
				*Definition->EnemyId.ToString(),
				Entry.Card ? *Entry.Card->CardId.ToString() : TEXT("null"),
				Entry.Copies);
			continue;
		}

		for (int32 CopyIndex = 0; CopyIndex < Entry.Copies; ++CopyIndex)
		{
			DrawPile.Add(Entry.Card);
		}
	}

	InitialDeckSize = DrawPile.Num();
	CurrentMana = FMath::Max(0, Definition->StartingMana);
	CurrentTurnHandLimit = FMath::Max(1, Definition->MaxHandSize);
	ShuffleDrawPile();

	if (InitialDeckSize <= 0)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("W01 enemy definition has no real deck and must use IntentCycle fallback. Enemy=%s Intents=%d"),
			*Definition->EnemyId.ToString(),
			Definition->IntentCycle.Num());
		return false;
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_ENEMY_DECK_READY Enemy=%s Cards=%d Entries=%d Hand=%d Action=%d Mana=%d CardsPerTurn=%d"),
		*Definition->EnemyId.ToString(),
		InitialDeckSize,
		Definition->Deck.Num(),
		Definition->MaxHandSize,
		Definition->MaxActionPoints,
		CurrentMana,
		Definition->CardsPerTurn);
	return true;
}

bool UFantasyEnemyDeckRuntime::StartTurn()
{
	if (!IsInitialized())
	{
		return false;
	}
	if (PendingPlayedCard)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Cannot start W01 enemy turn before pending card is finalized. Enemy=%s Card=%s"),
			*Definition->EnemyId.ToString(),
			*PendingPlayedCard->CardId.ToString());
		return false;
	}

	for (int32 HandIndex = Hand.Num() - 1; HandIndex >= 0; --HandIndex)
	{
		UCardDefinition* Card = Hand[HandIndex];
		if (Card && Card->bRetain)
		{
			continue;
		}

		if (Card)
		{
			DiscardPile.Add(Card);
		}
		Hand.RemoveAt(HandIndex, 1, EAllowShrinking::No);
	}

	CurrentActionPoints = FMath::Max(0, Definition->MaxActionPoints);
	CardsPlayedThisTurn = 0;
	int32 EquipmentDraw = 0;
	for (const UCardDefinition* Equipment : EquipmentZone)
	{
		if (Equipment)
		{
			EquipmentDraw += FMath::Max(0, Equipment->EquipmentTurnStartDraw);
		}
	}
	CurrentTurnHandLimit = FMath::Max(1, Definition->MaxHandSize) + EquipmentDraw;
	const int32 Drawn = DrawCards(CurrentTurnHandLimit - Hand.Num());

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_ENEMY_DECK_DRAW Enemy=%s Drawn=%d Hand=%d Draw=%d Discard=%d Action=%d Mana=%d"),
		*Definition->EnemyId.ToString(),
		Drawn,
		Hand.Num(),
		DrawPile.Num(),
		DiscardPile.Num(),
		CurrentActionPoints,
		CurrentMana);
	return true;
}

UCardDefinition* UFantasyEnemyDeckRuntime::GetNextPlayableCard() const
{
	if (!IsInitialized() || PendingPlayedCard
		|| CardsPlayedThisTurn >= FMath::Max(1, Definition->CardsPerTurn))
	{
		return nullptr;
	}

	for (UCardDefinition* Card : Hand)
	{
		if (IsCardPlayable(Card))
		{
			return Card;
		}
	}
	return nullptr;
}

UCardDefinition* UFantasyEnemyDeckRuntime::PlayNextCard()
{
	UCardDefinition* Card = GetNextPlayableCard();
	if (!Card)
	{
		return nullptr;
	}

	const int32 HandIndex = Hand.IndexOfByPredicate([Card](const UCardDefinition* Candidate)
	{
		return Candidate == Card;
	});
	if (HandIndex == INDEX_NONE)
	{
		return nullptr;
	}

	if (Card->bUseClassicResources)
	{
		CurrentActionPoints -= Card->ActionCost;
		CurrentMana -= Card->ManaCost;
	}
	else
	{
		// Converted enemies have no separate legacy energy pool. Old cards pay
		// EnergyCost from the per-turn action pool until their assets are migrated.
		CurrentActionPoints -= Card->EnergyCost;
	}
	Hand.RemoveAt(HandIndex, 1, EAllowShrinking::No);
	PendingPlayedCard = Card;
	++CardsPlayedThisTurn;

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_ENEMY_CARD_PLAY Enemy=%s Card=%s Played=%d/%d Action=%d Mana=%d"),
		*Definition->EnemyId.ToString(),
		*Card->CardId.ToString(),
		CardsPlayedThisTurn,
		FMath::Max(1, Definition->CardsPerTurn),
		CurrentActionPoints,
		CurrentMana);
	return Card;
}

bool UFantasyEnemyDeckRuntime::FinalizeCard(UCardDefinition* Card)
{
	if (!Card || Card != PendingPlayedCard.Get())
	{
		return false;
	}

	const TCHAR* Destination = TEXT("Discard");
	if (Card->bExhaust)
	{
		ExhaustPile.Add(Card);
		Destination = TEXT("Exhaust");
	}
	else if (Card->CardType == ECardType::Equipment)
	{
		if (EquipmentZone.Num() >= MaxEquipmentSlots)
		{
			DiscardPile.Add(EquipmentZone[0]);
			EquipmentZone.RemoveAt(0, 1, EAllowShrinking::No);
		}
		EquipmentZone.Add(Card);
		Destination = TEXT("Equipment");
	}
	else if (Card->CardType == ECardType::Counter)
	{
		if (CounterZone.Num() >= MaxCounterSlots)
		{
			DiscardPile.Add(CounterZone[0]);
			CounterZone.RemoveAt(0, 1, EAllowShrinking::No);
		}
		CounterZone.Add(Card);
		Destination = TEXT("Counter");
	}
	else
	{
		DiscardPile.Add(Card);
	}

	UE_LOG(
		LogTemp,
		Verbose,
		TEXT("W01 enemy card finalized. Enemy=%s Card=%s Zone=%s"),
		*Definition->EnemyId.ToString(),
		*Card->CardId.ToString(),
		Destination);
	PendingPlayedCard = nullptr;
	return true;
}

int32 UFantasyEnemyDeckRuntime::AddAction(const int32 Amount)
{
	const int32 Previous = CurrentActionPoints;
	CurrentActionPoints = FMath::Max(0, CurrentActionPoints + FMath::Max(0, Amount));
	return CurrentActionPoints - Previous;
}

int32 UFantasyEnemyDeckRuntime::AddMana(const int32 Amount)
{
	const int32 Previous = CurrentMana;
	CurrentMana = FMath::Max(0, CurrentMana + FMath::Max(0, Amount));
	return CurrentMana - Previous;
}

int32 UFantasyEnemyDeckRuntime::DiscardRandom(const int32 Count)
{
	const int32 Requested = FMath::Max(0, Count);
	int32 Discarded = 0;
	while (Discarded < Requested && !Hand.IsEmpty())
	{
		const int32 HandIndex = FMath::RandRange(0, Hand.Num() - 1);
		if (Hand[HandIndex])
		{
			DiscardPile.Add(Hand[HandIndex]);
		}
		Hand.RemoveAt(HandIndex, 1, EAllowShrinking::No);
		++Discarded;
	}
	return Discarded;
}

UCardDefinition* UFantasyEnemyDeckRuntime::ConsumeNextCounter()
{
	if (CounterZone.IsEmpty() || PendingCounterCard)
	{
		return nullptr;
	}

	PendingCounterCard = CounterZone[0];
	CounterZone.RemoveAt(0, 1, EAllowShrinking::No);
	if (PendingCounterCard)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("W01_ENEMY_COUNTER_TRIGGERED Enemy=%s Card=%s Remaining=%d"),
			Definition ? *Definition->EnemyId.ToString() : TEXT("Unknown"),
			*PendingCounterCard->CardId.ToString(),
			CounterZone.Num());
	}
	return PendingCounterCard;
}

bool UFantasyEnemyDeckRuntime::FinalizeTriggeredCounter(UCardDefinition* Card)
{
	if (!Card || Card != PendingCounterCard.Get())
	{
		return false;
	}

	DiscardPile.Add(Card);
	PendingCounterCard = nullptr;
	return true;
}

int32 UFantasyEnemyDeckRuntime::GetAttackBonus() const
{
	int32 Bonus = 0;
	for (const UCardDefinition* Equipment : EquipmentZone)
	{
		if (Equipment)
		{
			Bonus += FMath::Max(0, Equipment->EquipmentAttackBonus);
		}
	}
	return Bonus;
}

FString UFantasyEnemyDeckRuntime::BuildPreview(const int32 CurrentStrength) const
{
	const UCardDefinition* Card = PendingPlayedCard
		? PendingPlayedCard.Get()
		: GetNextPlayableCard();
	if (!Card)
	{
		if (Definition
			&& CardsPlayedThisTurn >= FMath::Max(1, Definition->CardsPerTurn))
		{
			return TEXT("敌人本回合出牌已达上限");
		}
		return Hand.IsEmpty()
			? TEXT("敌人当前没有手牌")
			: FString::Printf(
				TEXT("敌人暂无可支付的牌（行动力 %d，法力 %d）"),
				CurrentActionPoints,
				CurrentMana);
	}

	TArray<FString> Fragments;
	const bool bAttackCard = Card->CardType == ECardType::Attack;
	for (const FFantasyCombatEffectSpec& Effect : Card->Effects)
	{
		if (Effect.EffectType == EFantasyCombatEffectType::Damage
			&& Effect.Target == EFantasyCombatTarget::Opponent)
		{
			const int32 AttackModifier = bAttackCard
				? FMath::Max(0, CurrentStrength) + GetAttackBonus()
				: 0;
			const int32 Damage = FMath::Max(0, Effect.Magnitude) + AttackModifier;
			if (Effect.bPiercing)
			{
				Fragments.Add(FString::Printf(TEXT("造成 %d 点穿刺伤害"), Damage));
			}
			else
			{
				Fragments.Add(FString::Printf(TEXT("造成 %d 点伤害"), Damage));
			}
		}
		else
		{
			Fragments.Add(Effect.BuildRulesFragment());
		}
	}

	if (Card->CardType == ECardType::Equipment)
	{
		if (Card->EquipmentAttackBonus > 0)
		{
			Fragments.Add(FString::Printf(TEXT("装备后攻击 +%d"), Card->EquipmentAttackBonus));
		}
		if (Card->EquipmentTurnStartBlock > 0)
		{
			Fragments.Add(FString::Printf(
				TEXT("每回合获得 %d 格挡"),
				Card->EquipmentTurnStartBlock));
		}
		if (Card->EquipmentTurnStartDraw > 0)
		{
			Fragments.Add(FString::Printf(
				TEXT("每回合额外抽 %d 张"),
				Card->EquipmentTurnStartDraw));
		}
	}
	if (Fragments.IsEmpty() && !Card->Description.IsEmpty())
	{
		Fragments.Add(Card->Description.ToString());
	}

	const FString DisplayName = Card->DisplayName.ToString();
	const FString CostPreview = BuildCostPreview(Card);
	const FString RulesPreview = FString::Join(Fragments, TEXT("；"));
	if (PendingPlayedCard)
	{
		return FString::Printf(
			TEXT("正在打出：%s [%s] %s"),
			*DisplayName,
			*CostPreview,
			*RulesPreview);
	}
	return FString::Printf(
		TEXT("下一张：%s [%s] %s"),
		*DisplayName,
		*CostPreview,
		*RulesPreview);
}

bool UFantasyEnemyDeckRuntime::IsCardPlayable(const UCardDefinition* Card) const
{
	if (!Card)
	{
		return false;
	}

	if (Card->bUseClassicResources)
	{
		return Card->ActionCost <= CurrentActionPoints
			&& Card->ManaCost <= CurrentMana;
	}
	return Card->EnergyCost <= CurrentActionPoints;
}

int32 UFantasyEnemyDeckRuntime::DrawCards(const int32 Count)
{
	const int32 Requested = FMath::Max(0, Count);
	const int32 InitialHandSize = Hand.Num();
	for (int32 DrawIndex = 0;
		DrawIndex < Requested && Hand.Num() < CurrentTurnHandLimit;
		++DrawIndex)
	{
		if (DrawPile.IsEmpty())
		{
			RefillDrawPile();
		}
		if (DrawPile.IsEmpty())
		{
			break;
		}

		Hand.Add(DrawPile.Pop(EAllowShrinking::No));
	}
	return Hand.Num() - InitialHandSize;
}

void UFantasyEnemyDeckRuntime::RefillDrawPile()
{
	if (DiscardPile.IsEmpty())
	{
		return;
	}

	DrawPile = MoveTemp(DiscardPile);
	DiscardPile.Reset();
	ShuffleDrawPile();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_ENEMY_DECK_RESHUFFLED Enemy=%s Cards=%d"),
		Definition ? *Definition->EnemyId.ToString() : TEXT("Unknown"),
		DrawPile.Num());
}

void UFantasyEnemyDeckRuntime::ShuffleDrawPile()
{
	ShuffleCards(DrawPile);
}
