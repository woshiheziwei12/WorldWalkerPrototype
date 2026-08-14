#include "Cards/CardDefinition.h"

namespace
{
	const TCHAR* GetCardTypeLabel(const ECardType CardType)
	{
		switch (CardType)
		{
		case ECardType::Attack: return TEXT("攻击");
		case ECardType::Skill: return TEXT("技能");
		case ECardType::Spell: return TEXT("法术");
		case ECardType::Oath: return TEXT("誓约");
		default: return TEXT("卡牌");
		}
	}

	const TCHAR* GetCardSchoolLabel(const ECardSchool School)
	{
		switch (School)
		{
		case ECardSchool::Steel: return TEXT("钢铁");
		case ECardSchool::Faith: return TEXT("圣徽");
		case ECardSchool::Arcane: return TEXT("奥术");
		default: return TEXT("");
		}
	}
}

const FPrimaryAssetType UCardDefinition::PrimaryAssetType(TEXT("CardDefinition"));

FPrimaryAssetId UCardDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, CardId.IsNone() ? GetFName() : CardId);
}

FString UCardDefinition::BuildRulesText() const
{
	TArray<FString> CostParts;
	if (EnergyCost > 0 || ValorCost == 0)
	{
		CostParts.Add(FString::Printf(TEXT("%d 能量"), EnergyCost));
	}
	if (ValorCost > 0)
	{
		CostParts.Add(FString::Printf(TEXT("%d 英勇"), ValorCost));
	}

	TArray<FString> RuleFragments;
	for (const FFantasyCombatEffectSpec& Effect : Effects)
	{
		RuleFragments.Add(Effect.BuildRulesFragment());
	}
	if (bRetain)
	{
		RuleFragments.Add(TEXT("保留"));
	}
	if (bExhaust)
	{
		RuleFragments.Add(TEXT("消耗"));
	}
	if (RuleFragments.IsEmpty() && !Description.IsEmpty())
	{
		RuleFragments.Add(Description.ToString());
	}

	const FString Classification = School == ECardSchool::None
		? FString(GetCardTypeLabel(CardType))
		: FString::Printf(TEXT("%s %s"), GetCardSchoolLabel(School), GetCardTypeLabel(CardType));

	return FString::Printf(
		TEXT("%s\n[%s] %s\n%s"),
		*DisplayName.ToString(),
		*FString::Join(CostParts, TEXT(" + ")),
		*Classification,
		*FString::Join(RuleFragments, TEXT("; ")));
}
