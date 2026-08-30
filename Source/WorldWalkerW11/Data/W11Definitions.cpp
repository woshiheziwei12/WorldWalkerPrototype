#include "Data/W11Definitions.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
FPrimaryAssetId MakeW11AssetId(const FName TypeName, const UW11DefinitionBase* Definition)
{
	const FName AssetName = Definition && !Definition->DefinitionId.IsNone()
		? Definition->DefinitionId
		: FName(*GetNameSafe(Definition));
	return FPrimaryAssetId(TypeName, AssetName);
}
}

#if WITH_EDITOR
EDataValidationResult UW11DefinitionBase::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (DefinitionId.IsNone())
	{
		Context.AddError(FText::FromString(TEXT("W11 definition requires a stable DefinitionId.")));
		Result = EDataValidationResult::Invalid;
	}
	if (DisplayName.IsEmpty())
	{
		Context.AddError(FText::FromString(TEXT("W11 definition requires a display name.")));
		Result = EDataValidationResult::Invalid;
	}
	if (ContentVersion.IsEmpty())
	{
		Context.AddError(FText::FromString(TEXT("W11 definition requires a content version.")));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

FPrimaryAssetId UW11StatChoiceDefinition::GetPrimaryAssetId() const
{
	return MakeW11AssetId(TEXT("W11StatChoice"), this);
}

FPrimaryAssetId UW11ManualDefinition::GetPrimaryAssetId() const
{
	return MakeW11AssetId(TEXT("W11Manual"), this);
}

#if WITH_EDITOR
EDataValidationResult UW11ManualDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	TSet<EW11ComprehensionRank> SeenRanks;
	for (const FW11ManualRankEffects& Effects : RankEffects)
	{
		if (Effects.Rank == EW11ComprehensionRank::None || SeenRanks.Contains(Effects.Rank))
		{
			Context.AddError(FText::FromString(TEXT("Manual rank effects must use unique non-None ranks.")));
			Result = EDataValidationResult::Invalid;
		}
		SeenRanks.Add(Effects.Rank);
	}
	return Result;
}
#endif

FPrimaryAssetId UW11TreasureDefinition::GetPrimaryAssetId() const
{
	return MakeW11AssetId(TEXT("W11Treasure"), this);
}

FPrimaryAssetId UW11ShopServiceDefinition::GetPrimaryAssetId() const
{
	return MakeW11AssetId(TEXT("W11ShopService"), this);
}

FPrimaryAssetId UW11AbilityDefinition::GetPrimaryAssetId() const
{
	return MakeW11AssetId(TEXT("W11Ability"), this);
}

FPrimaryAssetId UW11CharacterAnimationSet::GetPrimaryAssetId() const
{
	return MakeW11AssetId(TEXT("W11CharacterAnimation"), this);
}

FPrimaryAssetId UW11SkillVisualDefinition::GetPrimaryAssetId() const
{
	return MakeW11AssetId(TEXT("W11SkillVisual"), this);
}

float UW11SkillVisualDefinition::ResolveHitStopSeconds(
	const EW11CombatCueType CueType, const bool bCritical) const
{
	const float Requested = CueType == EW11CombatCueType::Defeated
		? DefeatHitStopSeconds : bCritical ? CriticalHitStopSeconds : HitStopSeconds;
	return FMath::Clamp(Requested, 0.0f, 0.12f);
}

float UW11SkillVisualDefinition::ResolveCameraImpactStrength(
	const EW11CombatCueType CueType, const bool bCritical) const
{
	const float Requested = CueType == EW11CombatCueType::Defeated
		? DefeatCameraImpactStrength : bCritical ? CriticalCameraImpactStrength : CameraImpactStrength;
	return FMath::Clamp(Requested, 0.0f, 8.0f);
}

FPrimaryAssetId UW11HeroDefinition::GetPrimaryAssetId() const
{
	return MakeW11AssetId(TEXT("W11Hero"), this);
}

FPrimaryAssetId UW11SectDefinition::GetPrimaryAssetId() const
{
	return MakeW11AssetId(TEXT("W11Sect"), this);
}

FPrimaryAssetId UW11EnemyDefinition::GetPrimaryAssetId() const
{
	return MakeW11AssetId(TEXT("W11Enemy"), this);
}

#if WITH_EDITOR
EDataValidationResult UW11EnemyDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (bFormalBoss)
	{
		if (!bBoss || InitialBossPhaseId.IsNone() || InitialBossMechanicId.IsNone()
			|| BossPhases.Num() < 2)
		{
			Context.AddError(FText::FromString(
				TEXT("A formal boss must be marked Boss and define at least two mechanic phases.")));
			Result = EDataValidationResult::Invalid;
		}
		float PreviousThreshold = 1.0f;
		TSet<FName> PhaseIds;
		for (const FW11BossPhaseDefinition& Phase : BossPhases)
		{
			if (Phase.PhaseId.IsNone() || Phase.MechanicId.IsNone() || Phase.Abilities.IsEmpty()
				|| Phase.EnterAtHealthFraction <= 0.0f
				|| Phase.EnterAtHealthFraction >= PreviousThreshold
				|| PhaseIds.Contains(Phase.PhaseId))
			{
				Context.AddError(FText::FromString(
					TEXT("Formal boss phases require unique ids, mechanics, abilities and descending thresholds.")));
				Result = EDataValidationResult::Invalid;
			}
			PreviousThreshold = Phase.EnterAtHealthFraction;
			PhaseIds.Add(Phase.PhaseId);
		}
	}
	return Result;
}
#endif

FPrimaryAssetId UW11EncounterDefinition::GetPrimaryAssetId() const
{
	return MakeW11AssetId(TEXT("W11Encounter"), this);
}

FPrimaryAssetId UW11ChapterDefinition::GetPrimaryAssetId() const
{
	return MakeW11AssetId(TEXT("W11Chapter"), this);
}

#if WITH_EDITOR
EDataValidationResult UW11ChapterDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (ChapterIndex < 1 || ChapterIndex > 5)
	{
		Context.AddError(FText::FromString(TEXT("ChapterIndex must be within the five confirmed chapter gates.")));
		Result = EDataValidationResult::Invalid;
	}
	if (ChapterMap.IsNull())
	{
		Context.AddError(FText::FromString(TEXT("Chapter requires an explicit map.")));
		Result = EDataValidationResult::Invalid;
	}
	if (Nodes.IsEmpty())
	{
		Context.AddError(FText::FromString(TEXT("Chapter route requires at least one node.")));
		Result = EDataValidationResult::Invalid;
	}
	TSet<FName> NodeIds;
	int32 SmallBossCount = 0;
	for (const FW11ChapterRouteNode& Node : Nodes)
	{
		if (Node.NodeId.IsNone() || NodeIds.Contains(Node.NodeId))
		{
			Context.AddError(FText::FromString(TEXT("Chapter nodes require unique stable NodeIds.")));
			Result = EDataValidationResult::Invalid;
		}
		NodeIds.Add(Node.NodeId);
		const bool bCombatNode = Node.NodeType == EW11ChapterNodeType::Combat
			|| Node.NodeType == EW11ChapterNodeType::SmallBoss
			|| Node.NodeType == EW11ChapterNodeType::FinalBoss;
		if (bCombatNode && Node.Encounter.IsNull())
		{
			Context.AddError(FText::FromString(TEXT("Combat route nodes require an encounter definition.")));
			Result = EDataValidationResult::Invalid;
		}
		SmallBossCount += Node.NodeType == EW11ChapterNodeType::SmallBoss ? 1 : 0;
	}
	if (SmallBossCount != 1 || (!Nodes.IsEmpty() && Nodes.Last().NodeType != EW11ChapterNodeType::SmallBoss))
	{
		Context.AddError(FText::FromString(TEXT("Each chapter must end in exactly one SmallBoss node.")));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

FPrimaryAssetId UW11ChapterRouteDefinition::GetPrimaryAssetId() const
{
	return MakeW11AssetId(TEXT("W11ChapterRoute"), this);
}

#if WITH_EDITOR
EDataValidationResult UW11ChapterRouteDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (Chapters.Num() != 5)
	{
		Context.AddError(FText::FromString(TEXT("The frozen run structure requires exactly five chapter gates.")));
		Result = EDataValidationResult::Invalid;
	}
	TSet<int32> ChapterIndices;
	for (const TSoftObjectPtr<UW11ChapterDefinition>& ChapterAsset : Chapters)
	{
		const UW11ChapterDefinition* Chapter = ChapterAsset.LoadSynchronous();
		if (!Chapter || ChapterIndices.Contains(Chapter->ChapterIndex))
		{
			Context.AddError(FText::FromString(TEXT("Chapter route requires five unique loadable chapter indices.")));
			Result = EDataValidationResult::Invalid;
			continue;
		}
		ChapterIndices.Add(Chapter->ChapterIndex);
	}
	if (FinalBossEncounter.IsNull())
	{
		Context.AddError(FText::FromString(TEXT("Chapter route requires the confirmed Gu Changyuan final encounter.")));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

FPrimaryAssetId UW11RunRuleSet::GetPrimaryAssetId() const
{
	return MakeW11AssetId(TEXT("W11RunRuleSet"), this);
}
