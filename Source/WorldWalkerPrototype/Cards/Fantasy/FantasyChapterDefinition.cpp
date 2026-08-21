#include "Cards/Fantasy/FantasyChapterDefinition.h"

const FPrimaryAssetType UFantasyChapterDefinition::PrimaryAssetType(
	TEXT("FantasyChapterDefinition"));

namespace
{
	uint32 HashStableUtf8(const FString& Value)
	{
		const FTCHARToUTF8 Utf8(*Value);
		uint32 Hash = 2166136261u;
		for (int32 Index = 0; Index < Utf8.Length(); ++Index)
		{
			Hash ^= static_cast<uint8>(Utf8.Get()[Index]);
			Hash *= 16777619u;
		}
		return Hash;
	}

	int32 BuildRouteOfferSeed(
		const int32 RouteSeed,
		const FString& ContentVersion,
		const EFantasyRunDifficulty Difficulty,
		const int32 Chapter,
		const int32 Depth)
	{
		const uint32 SeedHash = HashCombineFast(
			GetTypeHash(RouteSeed),
			HashCombineFast(
				HashStableUtf8(ContentVersion),
				HashCombineFast(
					GetTypeHash(static_cast<uint8>(Difficulty)),
					HashCombineFast(GetTypeHash(Chapter), GetTypeHash(Depth)))));
		return static_cast<int32>(SeedHash);
	}

	template<typename CandidateType, typename WeightAccessor>
	int32 PickWeightedIndex(
		const TArray<CandidateType>& Candidates,
		FRandomStream& Stream,
		WeightAccessor GetWeight)
	{
		float TotalWeight = 0.0f;
		for (const CandidateType& Candidate : Candidates)
		{
			TotalWeight += FMath::Max(0.0f, GetWeight(Candidate));
		}
		if (Candidates.IsEmpty() || TotalWeight <= 0.0f)
		{
			return INDEX_NONE;
		}

		const float Roll = Stream.FRandRange(0.0f, TotalWeight);
		float RunningWeight = 0.0f;
		for (int32 Index = 0; Index < Candidates.Num(); ++Index)
		{
			RunningWeight += FMath::Max(0.0f, GetWeight(Candidates[Index]));
			if (Roll <= RunningWeight || Index == Candidates.Num() - 1)
			{
				return Index;
			}
		}
		return Candidates.Num() - 1;
	}

	FString BuildEncounterDescription(const UFantasyEnemyDefinition& Enemy)
	{
		const FString TierName = Enemy.EncounterTier == EFantasyEncounterTier::Boss
			? TEXT("守关")
			: Enemy.EncounterTier == EFantasyEncounterTier::Elite
				? TEXT("精英")
				: TEXT("普通");
		if (!Enemy.PassiveName.IsEmpty() && !Enemy.PassiveDescription.IsEmpty())
		{
			return FString::Printf(
				TEXT("%s遭遇 · 危险度 %d · %s：%s"),
				*TierName,
				Enemy.DangerRating,
				*Enemy.PassiveName.ToString(),
				*Enemy.PassiveDescription.ToString());
		}
		return FString::Printf(
			TEXT("%s遭遇 · 危险度 %d · 家族 %s"),
			*TierName,
			Enemy.DangerRating,
			*Enemy.Family.ToString());
	}

	EFantasyRouteNodeType GetRouteNodeType(const EFantasyEncounterTier Tier)
	{
		switch (Tier)
		{
		case EFantasyEncounterTier::Elite: return EFantasyRouteNodeType::EliteCombat;
		case EFantasyEncounterTier::Boss: return EFantasyRouteNodeType::Boss;
		case EFantasyEncounterTier::Normal:
		default: return EFantasyRouteNodeType::Combat;
		}
	}
}

FPrimaryAssetId UFantasyChapterDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}

TArray<FString> UFantasyChapterDefinition::ValidateDefinition() const
{
	TArray<FString> Errors;
	if (ChapterId.IsNone())
	{
		Errors.Add(TEXT("ChapterId is empty."));
	}
	if (ChapterNumber < 1)
	{
		Errors.Add(TEXT("ChapterNumber must be positive."));
	}
	if (TotalDepths < 1)
	{
		Errors.Add(TEXT("TotalDepths must be positive."));
	}

	TArray<const UFantasyEnemyDefinition*> Enemies;
	TSet<FName> EnemyIds;
	for (const TSoftObjectPtr<UFantasyEnemyDefinition>& EnemyReference : EncounterPool)
	{
		const UFantasyEnemyDefinition* Enemy = EnemyReference.LoadSynchronous();
		if (!Enemy)
		{
			Errors.Add(FString::Printf(
				TEXT("Encounter reference does not load: %s"),
				*EnemyReference.ToString()));
			continue;
		}
		if (Enemy->EnemyId.IsNone() || EnemyIds.Contains(Enemy->EnemyId))
		{
			Errors.Add(FString::Printf(
				TEXT("Encounter EnemyId is empty or duplicated: %s"),
				*Enemy->EnemyId.ToString()));
		}
		EnemyIds.Add(Enemy->EnemyId);
		if (Enemy->Chapter != ChapterNumber)
		{
			Errors.Add(FString::Printf(
				TEXT("Encounter %s belongs to chapter %d, expected %d."),
				*Enemy->EnemyId.ToString(),
				Enemy->Chapter,
				ChapterNumber));
		}
		if (!Enemy->UnlockCondition.IsNone()
			&& Enemy->UnlockCondition != UnlockCondition)
		{
			Errors.Add(FString::Printf(
				TEXT("Encounter %s has incompatible unlock condition %s."),
				*Enemy->EnemyId.ToString(),
				*Enemy->UnlockCondition.ToString()));
		}
		Enemies.Add(Enemy);
	}

	TSet<int32> AuthoredDepths;
	TSet<FName> NonCombatNodeIds;
	for (const FFantasyRouteDepthDefinition& DepthDefinition : DepthDefinitions)
	{
		if (DepthDefinition.Depth < 0 || DepthDefinition.Depth >= TotalDepths
			|| AuthoredDepths.Contains(DepthDefinition.Depth))
		{
			Errors.Add(FString::Printf(
				TEXT("Depth %d is out of range or duplicated."),
				DepthDefinition.Depth));
			continue;
		}
		AuthoredDepths.Add(DepthDefinition.Depth);
		const int32 NonCombatChoiceCount =
			DepthDefinition.ChoiceCount - DepthDefinition.CombatChoiceCount;
		if (DepthDefinition.ChoiceCount < 1
			|| DepthDefinition.CombatChoiceCount < 0
			|| NonCombatChoiceCount < 0)
		{
			Errors.Add(FString::Printf(
				TEXT("Depth %d has invalid choice counts."),
				DepthDefinition.Depth));
			continue;
		}

		TSet<FName> EligibleFamilies;
		int32 EligibleEncounterCount = 0;
		for (const UFantasyEnemyDefinition* Enemy : Enemies)
		{
			if (Enemy && Enemy->EncounterTier == DepthDefinition.EncounterTier
				&& Enemy->MinDepth <= DepthDefinition.Depth
				&& Enemy->MaxDepth >= DepthDefinition.Depth
				&& Enemy->RewardWeight > 0.0f)
			{
				++EligibleEncounterCount;
				EligibleFamilies.Add(Enemy->Family);
			}
		}
		if (EligibleEncounterCount < DepthDefinition.CombatChoiceCount)
		{
			Errors.Add(FString::Printf(
				TEXT("Depth %d needs %d encounters but only %d are eligible."),
				DepthDefinition.Depth,
				DepthDefinition.CombatChoiceCount,
				EligibleEncounterCount));
		}
		if (DepthDefinition.bRequireDistinctEnemyFamilies
			&& EligibleFamilies.Num() < DepthDefinition.CombatChoiceCount)
		{
			Errors.Add(FString::Printf(
				TEXT("Depth %d cannot satisfy distinct enemy families."),
				DepthDefinition.Depth));
		}
		if (DepthDefinition.NonCombatCandidates.Num() < NonCombatChoiceCount)
		{
			Errors.Add(FString::Printf(
				TEXT("Depth %d needs %d non-combat choices but only %d are authored."),
				DepthDefinition.Depth,
				NonCombatChoiceCount,
				DepthDefinition.NonCombatCandidates.Num()));
		}
		for (const FFantasyNonCombatRouteDefinition& Candidate
			: DepthDefinition.NonCombatCandidates)
		{
			if (Candidate.NodeId.IsNone() || Candidate.PayloadId.IsNone()
				|| Candidate.DisplayName.IsEmpty() || Candidate.Description.IsEmpty()
				|| Candidate.Weight <= 0.0f
				|| (Candidate.NodeType != EFantasyRouteNodeType::Event
					&& Candidate.NodeType != EFantasyRouteNodeType::Rest))
			{
				Errors.Add(FString::Printf(
					TEXT("Depth %d has an invalid non-combat candidate %s."),
					DepthDefinition.Depth,
					*Candidate.NodeId.ToString()));
			}
			if (NonCombatNodeIds.Contains(Candidate.NodeId))
			{
				Errors.Add(FString::Printf(
					TEXT("Non-combat NodeId is duplicated: %s"),
					*Candidate.NodeId.ToString()));
			}
			NonCombatNodeIds.Add(Candidate.NodeId);
		}
	}
	for (int32 Depth = 0; Depth < TotalDepths; ++Depth)
	{
		if (!AuthoredDepths.Contains(Depth))
		{
			Errors.Add(FString::Printf(TEXT("Depth %d has no route definition."), Depth));
		}
	}
	return Errors;
}

bool UFantasyChapterDefinition::GenerateRouteChoices(
	const int32 Depth,
	const int32 RouteSeed,
	const FString& ContentVersion,
	const EFantasyRunDifficulty Difficulty,
	const FName LastEncounterId,
	TArray<FFantasyRouteNodeChoice>& OutChoices,
	FString& OutFailureReason) const
{
	OutChoices.Reset();
	OutFailureReason.Reset();
	const FFantasyRouteDepthDefinition* DepthDefinition =
		DepthDefinitions.FindByPredicate(
			[Depth](const FFantasyRouteDepthDefinition& Candidate)
			{
				return Candidate.Depth == Depth;
			});
	if (!DepthDefinition)
	{
		OutFailureReason = FString::Printf(TEXT("Depth %d is not authored."), Depth);
		return false;
	}

	TArray<const UFantasyEnemyDefinition*> EligibleEnemies;
	for (const TSoftObjectPtr<UFantasyEnemyDefinition>& EnemyReference : EncounterPool)
	{
		const UFantasyEnemyDefinition* Enemy = EnemyReference.LoadSynchronous();
		if (Enemy && Enemy->Chapter == ChapterNumber
			&& (Enemy->UnlockCondition.IsNone() || Enemy->UnlockCondition == UnlockCondition)
			&& Enemy->EncounterTier == DepthDefinition->EncounterTier
			&& Enemy->MinDepth <= Depth && Enemy->MaxDepth >= Depth
			&& Enemy->RewardWeight > 0.0f)
		{
			EligibleEnemies.Add(Enemy);
		}
	}
	EligibleEnemies.Sort([](const UFantasyEnemyDefinition& Left, const UFantasyEnemyDefinition& Right)
	{
		return Left.EnemyId.LexicalLess(Right.EnemyId);
	});
	if (!LastEncounterId.IsNone()
		&& EligibleEnemies.Num() > DepthDefinition->CombatChoiceCount)
	{
		EligibleEnemies.RemoveAll(
			[LastEncounterId](const UFantasyEnemyDefinition* Enemy)
			{
				return Enemy && Enemy->EnemyId == LastEncounterId;
			});
	}
	if (EligibleEnemies.Num() < DepthDefinition->CombatChoiceCount)
	{
		OutFailureReason = FString::Printf(
			TEXT("Depth %d needs %d encounters but only %d are eligible."),
			Depth,
			DepthDefinition->CombatChoiceCount,
			EligibleEnemies.Num());
		return false;
	}

	FRandomStream Stream(BuildRouteOfferSeed(
		RouteSeed,
		ContentVersion,
		Difficulty,
		ChapterNumber,
		Depth));
	TSet<FName> ChosenFamilies;
	for (int32 ChoiceIndex = 0;
		ChoiceIndex < DepthDefinition->CombatChoiceCount;
		++ChoiceIndex)
	{
		TArray<const UFantasyEnemyDefinition*> FamilyFiltered = EligibleEnemies.FilterByPredicate(
			[&ChosenFamilies, DepthDefinition](const UFantasyEnemyDefinition* Enemy)
			{
				return Enemy && (!DepthDefinition->bRequireDistinctEnemyFamilies
					|| !ChosenFamilies.Contains(Enemy->Family));
			});
		TArray<const UFantasyEnemyDefinition*>& PickPool =
			FamilyFiltered.IsEmpty() ? EligibleEnemies : FamilyFiltered;
		const int32 PickIndex = PickWeightedIndex(
			PickPool,
			Stream,
			[](const UFantasyEnemyDefinition* Enemy)
			{
				return Enemy ? Enemy->RewardWeight : 0.0f;
			});
		if (!PickPool.IsValidIndex(PickIndex) || !PickPool[PickIndex])
		{
			OutFailureReason = TEXT("Weighted encounter selection failed.");
			OutChoices.Reset();
			return false;
		}

		const UFantasyEnemyDefinition* Enemy = PickPool[PickIndex];
		FFantasyRouteNodeChoice& Choice = OutChoices.AddDefaulted_GetRef();
		Choice.NodeId = FName(*FString::Printf(TEXT("D%d_%s"), Depth, *Enemy->EnemyId.ToString()));
		Choice.DisplayName = Enemy->DisplayName;
		Choice.Description = FText::FromString(BuildEncounterDescription(*Enemy));
		Choice.NodeType = GetRouteNodeType(Enemy->EncounterTier);
		Choice.PayloadId = Enemy->EnemyId;
		ChosenFamilies.Add(Enemy->Family);
		EligibleEnemies.Remove(Enemy);
	}

	const int32 NonCombatChoiceCount =
		DepthDefinition->ChoiceCount - DepthDefinition->CombatChoiceCount;
	TArray<FFantasyNonCombatRouteDefinition> NonCombatPool =
		DepthDefinition->NonCombatCandidates;
	for (int32 ChoiceIndex = 0; ChoiceIndex < NonCombatChoiceCount; ++ChoiceIndex)
	{
		const int32 PickIndex = PickWeightedIndex(
			NonCombatPool,
			Stream,
			[](const FFantasyNonCombatRouteDefinition& Candidate)
			{
				return Candidate.Weight;
			});
		if (!NonCombatPool.IsValidIndex(PickIndex))
		{
			OutFailureReason = TEXT("Weighted non-combat selection failed.");
			OutChoices.Reset();
			return false;
		}
		const FFantasyNonCombatRouteDefinition Selected = NonCombatPool[PickIndex];
		NonCombatPool.RemoveAt(PickIndex);
		FFantasyRouteNodeChoice& Choice = OutChoices.AddDefaulted_GetRef();
		Choice.NodeId = Selected.NodeId;
		Choice.DisplayName = Selected.DisplayName;
		Choice.Description = Selected.Description;
		Choice.NodeType = Selected.NodeType;
		Choice.PayloadId = Selected.PayloadId;
	}

	for (int32 Index = OutChoices.Num() - 1; Index > 0; --Index)
	{
		OutChoices.Swap(Index, Stream.RandRange(0, Index));
	}
	if (OutChoices.Num() != DepthDefinition->ChoiceCount)
	{
		OutFailureReason = FString::Printf(
			TEXT("Generated %d choices, expected %d."),
			OutChoices.Num(),
			DepthDefinition->ChoiceCount);
		OutChoices.Reset();
		return false;
	}
	return true;
}
