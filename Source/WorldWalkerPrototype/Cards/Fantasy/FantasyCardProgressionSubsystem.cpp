#include "Cards/Fantasy/FantasyCardProgressionSubsystem.h"

#include "Cards/CardDefinition.h"

bool UFantasyCardProgressionSubsystem::GrantCard(const UCardDefinition* Card)
{
	if (!Card || Card->CardId.IsNone() || !Card->bRewardEligible)
	{
		return false;
	}

	int32& Copies = GrantedCardCopies.FindOrAdd(Card->CardId);
	++Copies;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01 card reward persisted for this run. Card=%s Copies=%d TotalRewards=%d"),
		*Card->CardId.ToString(),
		Copies,
		GetTotalGrantedCopies());
	return true;
}

int32 UFantasyCardProgressionSubsystem::GetGrantedCopies(const FName CardId) const
{
	return GrantedCardCopies.FindRef(CardId);
}

int32 UFantasyCardProgressionSubsystem::GetTotalGrantedCopies() const
{
	int32 Total = 0;
	for (const TPair<FName, int32>& Pair : GrantedCardCopies)
	{
		Total += FMath::Max(0, Pair.Value);
	}
	return Total;
}

FString UFantasyCardProgressionSubsystem::BuildRunSummary() const
{
	return FString::Printf(TEXT("本次旅途已获得 %d 张战利品卡"), GetTotalGrantedCopies());
}
