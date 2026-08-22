#include "Cards/CardCombatComponent.h"

#include "Cards/CardDefinition.h"
#include "Cards/Fantasy/FantasyCardProgressionSubsystem.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"

namespace
{
	const FString FantasyCardPath(TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/Data/Cards"));
	constexpr uint8 AllSchoolsMask = (1 << 0) | (1 << 1) | (1 << 2);

	TArray<UCardDefinition*> LoadCardDefinitions(
		const FName CardSetId,
		int32* OutScannedAssetCount = nullptr,
		int32* OutRegisteredDefinitionCount = nullptr)
	{
		UAssetManager& AssetManager = UAssetManager::Get();
		const TArray<FString> CardScanPaths = {FantasyCardPath};
		const int32 ScannedAssetCount = AssetManager.ScanPathsForPrimaryAssets(
			UCardDefinition::PrimaryAssetType,
			CardScanPaths,
			UCardDefinition::StaticClass(),
			false,
			false,
			true);
		TArray<FPrimaryAssetId> CardIds;
		AssetManager.GetPrimaryAssetIdList(UCardDefinition::PrimaryAssetType, CardIds);
		CardIds.Sort([](const FPrimaryAssetId& Left, const FPrimaryAssetId& Right)
		{
			return Left.PrimaryAssetName.LexicalLess(Right.PrimaryAssetName);
		});

		if (OutScannedAssetCount)
		{
			*OutScannedAssetCount = ScannedAssetCount;
		}
		if (OutRegisteredDefinitionCount)
		{
			*OutRegisteredDefinitionCount = CardIds.Num();
		}

		TArray<UCardDefinition*> Definitions;
		for (const FPrimaryAssetId& CardId : CardIds)
		{
			const FSoftObjectPath CardPath = AssetManager.GetPrimaryAssetPath(CardId);
			if (!CardPath.ToString().StartsWith(FantasyCardPath + TEXT("/")))
			{
				continue;
			}

			UCardDefinition* Card = Cast<UCardDefinition>(CardPath.TryLoad());
			if (Card && Card->CardSetId == CardSetId)
			{
				Definitions.Add(Card);
			}
		}
		return Definitions;
	}

	void ShuffleCards(TArray<UCardDefinition*>& Cards, FRandomStream& RandomStream)
	{
		for (int32 Index = Cards.Num() - 1; Index > 0; --Index)
		{
			Cards.Swap(Index, RandomStream.RandRange(0, Index));
		}
	}
}

const FName UCardCombatComponent::FantasyCardSetId(TEXT("W01_EasternHorror"));

UCardCombatComponent::UCardCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UCardCombatComponent::LoadStartingDeck()
{
	return LoadStartingDeck(FantasyCardSetId);
}

bool UCardCombatComponent::LoadStartingDeck(const FName CardSetId)
{
	StartingDeck.Reset();
	LoadedCardSetId = NAME_None;
	if (CardSetId != FantasyCardSetId)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Unsupported starter CardSetId %s. This component currently owns only %s."),
			*CardSetId.ToString(),
			*FantasyCardSetId.ToString());
		return false;
	}

	int32 ScannedAssetCount = 0;
	int32 RegisteredDefinitionCount = 0;
	const TArray<UCardDefinition*> Definitions = LoadCardDefinitions(
		CardSetId,
		&ScannedAssetCount,
		&RegisteredDefinitionCount);
	UFantasyCardProgressionSubsystem* Progression = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	const EFantasyPlayerProfession Profession = Progression && Progression->HasSelectedProfession()
		? Progression->GetSelectedProfession()
		: EFantasyPlayerProfession::Knight;

	for (UCardDefinition* Card : Definitions)
	{
		if (!Card || (Card->Profession != EFantasyPlayerProfession::None
			&& Card->Profession != Profession))
		{
			continue;
		}
		const int32 RewardCopies = Progression
			? Progression->GetGrantedCopies(Card->CardId, Card->UpgradeLevel)
			: 0;
		const int32 RemovedCopies = Progression
			? Progression->GetRemovedCopies(Card->CardId, Card->UpgradeLevel)
			: 0;
		const int32 UpgradedFromCopies = Progression
			? Progression->GetUpgradedFromCopies(Card->CardId, Card->UpgradeLevel)
			: 0;
		const int32 UpgradedToCopies = Progression
			? Progression->GetUpgradedToCopies(Card->CardId, Card->UpgradeLevel)
			: 0;
		const int32 EffectiveCopies = FMath::Max(
			0,
			Card->StartingDeckCopies + RewardCopies - RemovedCopies
				- UpgradedFromCopies + UpgradedToCopies);
		for (int32 CopyIndex = 0; CopyIndex < EffectiveCopies; ++CopyIndex)
		{
			StartingDeck.Add(Card);
		}
	}
	if (Progression)
	{
		TArray<UCardDefinition*> DeckSnapshot;
		DeckSnapshot.Reserve(StartingDeck.Num());
		for (UCardDefinition* Card : StartingDeck)
		{
			DeckSnapshot.Add(Card);
		}
		Progression->ReplaceDeckSnapshot(DeckSnapshot);
	}

	if (StartingDeck.IsEmpty())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("WorldWalker starter deck is empty for set %s. Path=%s Scanned=%d RegisteredDefinitions=%d"),
			*CardSetId.ToString(),
			*FantasyCardPath,
			ScannedAssetCount,
			RegisteredDefinitionCount);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("WorldWalker starter deck loaded. Set=%s Scanned=%d RegisteredDefinitions=%d Cards=%d RunRewards=%d"),
			*CardSetId.ToString(),
			ScannedAssetCount,
			RegisteredDefinitionCount,
			StartingDeck.Num(),
			Progression ? Progression->GetTotalGrantedCopies() : 0);
		LoadedCardSetId = CardSetId;
	}
	return !StartingDeck.IsEmpty();
}

bool UCardCombatComponent::StartBattle()
{
	return StartBattle(FantasyCardSetId);
}

bool UCardCombatComponent::StartBattle(const FName CardSetId)
{
	if ((StartingDeck.IsEmpty() || LoadedCardSetId != CardSetId) && !LoadStartingDeck(CardSetId))
	{
		return false;
	}

	DrawPile = StartingDeck;
	Hand.Reset();
	DiscardPile.Reset();
	ExhaustPile.Reset();
	EquipmentZone.Reset();
	TemporaryCardInsertions.Reset();
	CurrentEnergy = 0;
	CurrentActionPoints = 0;
	CurrentMana = 0;
	CurrentTurnHandLimit = MaxHandSize;
	CurrentValor = 0;
	CurrentBlock = 0;
	LitSchoolMask = 0;
	bResonanceTriggeredThisTurn = false;
	bLastCardTriggeredResonance = false;
	UFantasyCardProgressionSubsystem* Progression = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	BattleRandomStream.Initialize(Progression
		? Progression->ConsumeDeterministicSeed(TEXT("PlayerBattle"))
		: FMath::Rand());
	bBattleRandomStreamReady = true;
	ShuffleDrawPile();
	StartPlayerTurn();
	return true;
}

void UCardCombatComponent::StartPlayerTurn()
{
	CurrentBlock = 0;
	CurrentEnergy = MaxEnergy;
	CurrentActionPoints = MaxActionPoints;
	LitSchoolMask = 0;
	bResonanceTriggeredThisTurn = false;
	bLastCardTriggeredResonance = false;
	int32 EquipmentDraw = 0;
	for (const UCardDefinition* Equipment : EquipmentZone)
	{
		if (!Equipment)
		{
			continue;
		}
		CurrentBlock += FMath::Max(0, Equipment->EquipmentTurnStartBlock);
		EquipmentDraw += FMath::Max(0, Equipment->EquipmentTurnStartDraw);
	}
	CurrentTurnHandLimit = MaxHandSize + EquipmentDraw;
	DrawCards(CurrentTurnHandLimit - Hand.Num());
}

void UCardCombatComponent::EndPlayerTurn()
{
	CurrentEnergy = 0;
	CurrentActionPoints = 0;
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
}

bool UCardCombatComponent::IsCardPlayable(const int32 HandIndex) const
{
	return Hand.IsValidIndex(HandIndex)
		&& IsCardPlayable(Hand[HandIndex]);
}

bool UCardCombatComponent::IsCardPlayable(const UCardDefinition* Card) const
{
	if (!Card || Card->ValorCost > CurrentValor)
	{
		return false;
	}

	if (Card->bUseClassicResources)
	{
		return Card->ActionCost <= CurrentActionPoints
			&& Card->ManaCost <= CurrentMana;
	}
	return Card->EnergyCost <= CurrentEnergy;
}

bool UCardCombatComponent::CanPlayCard(const int32 HandIndex) const
{
	return IsCardPlayable(HandIndex);
}

UCardDefinition* UCardCombatComponent::PlayCard(const int32 HandIndex)
{
	bLastCardTriggeredResonance = false;
	if (!IsCardPlayable(HandIndex))
	{
		return nullptr;
	}

	UCardDefinition* Card = Hand[HandIndex];
	if (Card->bUseClassicResources)
	{
		CurrentActionPoints -= Card->ActionCost;
		CurrentMana -= Card->ManaCost;
	}
	else
	{
		CurrentEnergy -= Card->EnergyCost;
	}
	CurrentValor -= Card->ValorCost;
	Hand.RemoveAt(HandIndex);
	RegisterPlayedSchool(Card->School);
	return Card;
}

void UCardCombatComponent::FinalizePlayedCard(UCardDefinition* Card)
{
	// Deck copies intentionally share the same definition pointer, so pointer
	// containment cannot be used as a duplicate-call guard here.
	if (!Card)
	{
		return;
	}

	if (Card->bExhaust)
	{
		ExhaustPile.Add(Card);
	}
	else if (Card->CardType == ECardType::Equipment)
	{
		if (EquipmentZone.Num() >= MaxEquipmentSlots)
		{
			DiscardPile.Add(EquipmentZone[0]);
			EquipmentZone.RemoveAt(0, 1, EAllowShrinking::No);
		}
		EquipmentZone.Add(Card);
	}
	else
	{
		DiscardPile.Add(Card);
	}
}

bool UCardCombatComponent::HasAnyPlayableCard() const
{
	for (int32 HandIndex = 0; HandIndex < Hand.Num(); ++HandIndex)
	{
		if (IsCardPlayable(HandIndex))
		{
			return true;
		}
	}
	return false;
}

TArray<UCardDefinition*> UCardCombatComponent::BuildRewardChoices(const int32 ChoiceCount) const
{
	const int32 SafeChoiceCount = FMath::Max(0, ChoiceCount);
	TArray<UCardDefinition*> RewardPool;

	for (UCardDefinition* Card : LoadCardDefinitions(FantasyCardSetId))
	{
		const UFantasyCardProgressionSubsystem* Progression = GetWorld() && GetWorld()->GetGameInstance()
			? GetWorld()->GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
			: nullptr;
		const EFantasyPlayerProfession Profession = Progression && Progression->HasSelectedProfession()
			? Progression->GetSelectedProfession()
			: EFantasyPlayerProfession::Knight;
		if (!Card || !Card->bRewardEligible || Card->Profession != Profession)
		{
			continue;
		}

		RewardPool.Add(Card);
	}

	UFantasyCardProgressionSubsystem* Progression = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	FRandomStream RewardRandomStream(Progression
		? Progression->ConsumeDeterministicSeed(TEXT("RewardOffer"))
		: FMath::Rand());
	ShuffleCards(RewardPool, RewardRandomStream);
	TArray<UCardDefinition*> Choices;
	while (Choices.Num() < SafeChoiceCount && !RewardPool.IsEmpty())
	{
		Choices.Add(RewardPool.Pop(EAllowShrinking::No));
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01 victory reward choices prepared. Requested=%d Offered=%d RewardPool=%d"),
		SafeChoiceCount,
		Choices.Num(),
		Choices.Num() + RewardPool.Num());
	return Choices;
}

bool UCardCombatComponent::GrantRewardCard(UCardDefinition* Card)
{
	return Card && Card->bRewardEligible && GrantRunCard(Card);
}

bool UCardCombatComponent::GrantRunCard(UCardDefinition* Card)
{
	if (!Card || Card->CardSetId != FantasyCardSetId
		|| !GetWorld() || !GetWorld()->GetGameInstance())
	{
		return false;
	}

	UFantasyCardProgressionSubsystem* Progression =
		GetWorld()->GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>();
	if (!Progression || (Card->Profession != EFantasyPlayerProfession::None
		&& Card->Profession != Progression->GetSelectedProfession())
		|| !Progression->GrantCard(Card))
	{
		return false;
	}

	StartingDeck.Add(Card);
	return true;
}

UCardDefinition* UCardCombatComponent::FindCardDefinition(const FName CardId) const
{
	if (CardId.IsNone())
	{
		return nullptr;
	}

	for (UCardDefinition* Card : LoadCardDefinitions(FantasyCardSetId))
	{
		if (Card && Card->CardId == CardId)
		{
			return Card;
		}
	}
	return nullptr;
}

bool UCardCombatComponent::RemoveCardFromRun(const FName CardId)
{
	if (CardId.IsNone() || StartingDeck.Num() <= 1 || !GetWorld() || !GetWorld()->GetGameInstance())
	{
		return false;
	}

	const int32 DeckIndex = StartingDeck.IndexOfByPredicate([CardId](const UCardDefinition* Card)
	{
		return Card && Card->CardId == CardId;
	});
	if (DeckIndex == INDEX_NONE)
	{
		return false;
	}

	UFantasyCardProgressionSubsystem* Progression =
		GetWorld()->GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>();
	const UCardDefinition* CardToRemove = StartingDeck[DeckIndex];
	if (!Progression || !CardToRemove
		|| !Progression->RemoveCardCopy(CardId, CardToRemove->UpgradeLevel))
	{
		return false;
	}

	StartingDeck.RemoveAt(DeckIndex);
	return true;
}

int32 UCardCombatComponent::GetRunRewardCount() const
{
	const UFantasyCardProgressionSubsystem* Progression = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	return Progression ? Progression->GetTotalGrantedCopies() : 0;
}

int32 UCardCombatComponent::GetRunRemovedCount() const
{
	const UFantasyCardProgressionSubsystem* Progression = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	return Progression ? Progression->GetTotalRemovedCopies() : 0;
}

FString UCardCombatComponent::BuildCurrentDeckSummary() const
{
	TMap<FName, int32> CopiesById;
	TMap<FName, const UCardDefinition*> DefinitionById;
	for (const UCardDefinition* Card : StartingDeck)
	{
		if (!Card)
		{
			continue;
		}
		++CopiesById.FindOrAdd(Card->CardId);
		DefinitionById.FindOrAdd(Card->CardId) = Card;
	}

	TArray<FName> CardIds;
	CopiesById.GetKeys(CardIds);
	CardIds.Sort([&DefinitionById](const FName Left, const FName Right)
	{
		const UCardDefinition* const* LeftCard = DefinitionById.Find(Left);
		const UCardDefinition* const* RightCard = DefinitionById.Find(Right);
		const FString LeftName = LeftCard && *LeftCard
			? (*LeftCard)->DisplayName.ToString()
			: Left.ToString();
		const FString RightName = RightCard && *RightCard
			? (*RightCard)->DisplayName.ToString()
			: Right.ToString();
		return LeftName < RightName;
	});

	TArray<FString> Entries;
	for (const FName CardId : CardIds)
	{
		const UCardDefinition* const* Card = DefinitionById.Find(CardId);
		if (!Card || !*Card)
		{
			continue;
		}
		Entries.Add(FString::Printf(
			TEXT("%d × %s"),
			CopiesById.FindRef(CardId),
			*(*Card)->BuildRulesText()));
	}

	return Entries.IsEmpty()
		? TEXT("当前牌组为空。")
		: FString::Join(Entries, TEXT("\n\n"));
}

int32 UCardCombatComponent::DrawCards(const int32 Count)
{
	const int32 RequestedCount = FMath::Max(0, Count);
	const int32 InitialHandSize = Hand.Num();
	for (int32 DrawIndex = 0;
		DrawIndex < RequestedCount && Hand.Num() < CurrentTurnHandLimit;
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

		UCardDefinition* DrawnCard = DrawPile.Pop(EAllowShrinking::No);
		Hand.Add(DrawnCard);
		if (UFantasyCardProgressionSubsystem* Progression =
			GetWorld() && GetWorld()->GetGameInstance()
				? GetWorld()->GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
				: nullptr)
		{
			Progression->LogStructuredEvent(
				TEXT("CardDrawn"),
				{{TEXT("actor"), TEXT("Player")},
				 {TEXT("cardId"), DrawnCard ? DrawnCard->CardId.ToString() : TEXT("None")}});
		}
	}
	return Hand.Num() - InitialHandSize;
}

int32 UCardCombatComponent::AddValor(const int32 Amount)
{
	const int32 PreviousValor = CurrentValor;
	CurrentValor = FMath::Clamp(CurrentValor + FMath::Max(0, Amount), 0, MaxValor);
	return CurrentValor - PreviousValor;
}

int32 UCardCombatComponent::AddActionPoints(const int32 Amount)
{
	const int32 Previous = CurrentActionPoints;
	CurrentActionPoints = FMath::Max(0, CurrentActionPoints + FMath::Max(0, Amount));
	return CurrentActionPoints - Previous;
}

int32 UCardCombatComponent::AddMana(const int32 Amount)
{
	const int32 Previous = CurrentMana;
	CurrentMana = FMath::Max(0, CurrentMana + FMath::Max(0, Amount));
	return CurrentMana - Previous;
}

int32 UCardCombatComponent::RemoveMana(const int32 Amount)
{
	const int32 Previous = CurrentMana;
	CurrentMana = FMath::Max(0, CurrentMana - FMath::Max(0, Amount));
	return Previous - CurrentMana;
}

int32 UCardCombatComponent::AddTemporaryCardToDiscard(
	UCardDefinition* Card,
	const int32 Count,
	const int32 PerBattleCap)
{
	if (!Card || Count <= 0)
	{
		return 0;
	}

	int32& Inserted = TemporaryCardInsertions.FindOrAdd(Card->CardId);
	const int32 Allowed = PerBattleCap > 0
		? FMath::Min(Count, FMath::Max(0, PerBattleCap - Inserted))
		: Count;
	for (int32 Index = 0; Index < Allowed; ++Index)
	{
		DiscardPile.Add(Card);
	}
	Inserted += Allowed;
	return Allowed;
}

bool UCardCombatComponent::UpgradeFirstEligibleRunCard()
{
	if (!GetWorld() || !GetWorld()->GetGameInstance())
	{
		return false;
	}
	UFantasyCardProgressionSubsystem* Progression =
		GetWorld()->GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>();
	if (!Progression)
	{
		return false;
	}

	for (int32 Index = 0; Index < StartingDeck.Num(); ++Index)
	{
		UCardDefinition* BaseCard = StartingDeck[Index];
		if (!BaseCard || BaseCard->UpgradeLevel != 0 || BaseCard->UpgradeCard.IsNull())
		{
			continue;
		}
		UCardDefinition* Upgrade = BaseCard->UpgradeCard.LoadSynchronous();
		if (Upgrade && Progression->UpgradeCardCopy(BaseCard))
		{
			StartingDeck[Index] = Upgrade;
			return true;
		}
	}
	return false;
}

int32 UCardCombatComponent::DiscardRandomCards(const int32 Count)
{
	const int32 Requested = FMath::Max(0, Count);
	int32 Discarded = 0;
	while (Discarded < Requested && !Hand.IsEmpty())
	{
		const int32 HandIndex = bBattleRandomStreamReady
			? BattleRandomStream.RandRange(0, Hand.Num() - 1)
			: FMath::RandRange(0, Hand.Num() - 1);
		if (Hand[HandIndex])
		{
			DiscardPile.Add(Hand[HandIndex]);
		}
		Hand.RemoveAt(HandIndex, 1, EAllowShrinking::No);
		++Discarded;
	}
	return Discarded;
}

int32 UCardCombatComponent::GetEquipmentAttackBonus() const
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

void UCardCombatComponent::AddBlock(const int32 Amount)
{
	CurrentBlock += FMath::Max(0, Amount);
}

int32 UCardCombatComponent::AbsorbIncomingDamage(const int32 DamageAmount)
{
	const int32 SafeDamage = FMath::Max(0, DamageAmount);
	const int32 AbsorbedDamage = FMath::Min(CurrentBlock, SafeDamage);
	CurrentBlock -= AbsorbedDamage;
	return SafeDamage - AbsorbedDamage;
}

bool UCardCombatComponent::IsSchoolLit(const ECardSchool School) const
{
	const uint8 SchoolBit = GetSchoolBit(School);
	return SchoolBit != 0 && (LitSchoolMask & SchoolBit) != 0;
}

FString UCardCombatComponent::GetSchoolSummary() const
{
	return FString::Printf(
		TEXT("钢铁 %s | 圣徽 %s | 奥术 %s"),
		IsSchoolLit(ECardSchool::Steel) ? TEXT("[X]") : TEXT("[ ]"),
		IsSchoolLit(ECardSchool::Faith) ? TEXT("[X]") : TEXT("[ ]"),
		IsSchoolLit(ECardSchool::Arcane) ? TEXT("[X]") : TEXT("[ ]"));
}

void UCardCombatComponent::RegisterPlayedSchool(const ECardSchool School)
{
	const uint8 SchoolBit = GetSchoolBit(School);
	if (SchoolBit == 0)
	{
		return;
	}

	LitSchoolMask |= SchoolBit;
	if (!bResonanceTriggeredThisTurn && (LitSchoolMask & AllSchoolsMask) == AllSchoolsMask)
	{
		bResonanceTriggeredThisTurn = true;
		bLastCardTriggeredResonance = true;
		AddValor(1);
		DrawCards(1);
	}
}

uint8 UCardCombatComponent::GetSchoolBit(const ECardSchool School)
{
	switch (School)
	{
	case ECardSchool::Steel: return 1 << 0;
	case ECardSchool::Faith: return 1 << 1;
	case ECardSchool::Arcane: return 1 << 2;
	default: return 0;
	}
}

void UCardCombatComponent::RefillDrawPile()
{
	DrawPile = MoveTemp(DiscardPile);
	DiscardPile.Reset();
	ShuffleDrawPile();
}

void UCardCombatComponent::ShuffleDrawPile()
{
	for (int32 Index = DrawPile.Num() - 1; Index > 0; --Index)
	{
		DrawPile.Swap(
			Index,
			bBattleRandomStreamReady
				? BattleRandomStream.RandRange(0, Index)
				: FMath::RandRange(0, Index));
	}
}
