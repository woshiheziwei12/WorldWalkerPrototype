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

	void ShuffleCards(TArray<UCardDefinition*>& Cards)
	{
		for (int32 Index = Cards.Num() - 1; Index > 0; --Index)
		{
			Cards.Swap(Index, FMath::RandRange(0, Index));
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
	const UFantasyCardProgressionSubsystem* Progression = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;

	for (UCardDefinition* Card : Definitions)
	{
		for (int32 CopyIndex = 0; CopyIndex < Card->StartingDeckCopies; ++CopyIndex)
		{
			StartingDeck.Add(Card);
		}
		const int32 RewardCopies = Progression ? Progression->GetGrantedCopies(Card->CardId) : 0;
		for (int32 CopyIndex = 0; CopyIndex < RewardCopies; ++CopyIndex)
		{
			StartingDeck.Add(Card);
		}
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
	CurrentEnergy = 0;
	CurrentValor = 0;
	CurrentBlock = 0;
	LitSchoolMask = 0;
	bResonanceTriggeredThisTurn = false;
	bLastCardTriggeredResonance = false;
	ShuffleDrawPile();
	StartPlayerTurn();
	return true;
}

void UCardCombatComponent::StartPlayerTurn()
{
	CurrentBlock = 0;
	CurrentEnergy = MaxEnergy;
	LitSchoolMask = 0;
	bResonanceTriggeredThisTurn = false;
	bLastCardTriggeredResonance = false;
	DrawCards(MaxHandSize - Hand.Num());
}

void UCardCombatComponent::EndPlayerTurn()
{
	CurrentEnergy = 0;
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
	return Card
		&& Card->EnergyCost <= CurrentEnergy
		&& Card->ValorCost <= CurrentValor;
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
	CurrentEnergy -= Card->EnergyCost;
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
	TArray<UCardDefinition*> SteelCards;
	TArray<UCardDefinition*> FaithCards;
	TArray<UCardDefinition*> ArcaneCards;
	TArray<UCardDefinition*> RemainingCards;

	for (UCardDefinition* Card : LoadCardDefinitions(FantasyCardSetId))
	{
		if (!Card || !Card->bRewardEligible)
		{
			continue;
		}

		switch (Card->School)
		{
		case ECardSchool::Steel: SteelCards.Add(Card); break;
		case ECardSchool::Faith: FaithCards.Add(Card); break;
		case ECardSchool::Arcane: ArcaneCards.Add(Card); break;
		case ECardSchool::None:
		default: RemainingCards.Add(Card); break;
		}
	}

	ShuffleCards(SteelCards);
	ShuffleCards(FaithCards);
	ShuffleCards(ArcaneCards);
	TArray<UCardDefinition*> Choices;
	if (!SteelCards.IsEmpty() && Choices.Num() < SafeChoiceCount)
	{
		Choices.Add(SteelCards.Pop(EAllowShrinking::No));
	}
	if (!FaithCards.IsEmpty() && Choices.Num() < SafeChoiceCount)
	{
		Choices.Add(FaithCards.Pop(EAllowShrinking::No));
	}
	if (!ArcaneCards.IsEmpty() && Choices.Num() < SafeChoiceCount)
	{
		Choices.Add(ArcaneCards.Pop(EAllowShrinking::No));
	}

	RemainingCards.Append(SteelCards);
	RemainingCards.Append(FaithCards);
	RemainingCards.Append(ArcaneCards);
	ShuffleCards(RemainingCards);
	while (Choices.Num() < SafeChoiceCount && !RemainingCards.IsEmpty())
	{
		Choices.Add(RemainingCards.Pop(EAllowShrinking::No));
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01 victory reward choices prepared. Requested=%d Offered=%d RewardPool=%d"),
		SafeChoiceCount,
		Choices.Num(),
		Choices.Num() + RemainingCards.Num());
	return Choices;
}

bool UCardCombatComponent::GrantRewardCard(UCardDefinition* Card)
{
	if (!Card || !Card->bRewardEligible || Card->CardSetId != FantasyCardSetId
		|| !GetWorld() || !GetWorld()->GetGameInstance())
	{
		return false;
	}

	UFantasyCardProgressionSubsystem* Progression =
		GetWorld()->GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>();
	if (!Progression || !Progression->GrantCard(Card))
	{
		return false;
	}

	StartingDeck.Add(Card);
	return true;
}

int32 UCardCombatComponent::GetRunRewardCount() const
{
	const UFantasyCardProgressionSubsystem* Progression = GetWorld() && GetWorld()->GetGameInstance()
		? GetWorld()->GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	return Progression ? Progression->GetTotalGrantedCopies() : 0;
}

int32 UCardCombatComponent::DrawCards(const int32 Count)
{
	const int32 RequestedCount = FMath::Max(0, Count);
	const int32 InitialHandSize = Hand.Num();
	for (int32 DrawIndex = 0; DrawIndex < RequestedCount && Hand.Num() < MaxHandSize; ++DrawIndex)
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

int32 UCardCombatComponent::AddValor(const int32 Amount)
{
	const int32 PreviousValor = CurrentValor;
	CurrentValor = FMath::Clamp(CurrentValor + FMath::Max(0, Amount), 0, MaxValor);
	return CurrentValor - PreviousValor;
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
		DrawPile.Swap(Index, FMath::RandRange(0, Index));
	}
}
