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
		case ECardType::Action: return TEXT("行动");
		case ECardType::Mana: return TEXT("法力");
		case ECardType::Equipment: return TEXT("装备");
		case ECardType::Counter: return TEXT("反制");
		case ECardType::Prayer: return TEXT("祈祷");
		case ECardType::Special: return TEXT("特殊");
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
	if (bUseClassicResources && ActionCost > 0)
	{
		CostParts.Add(FString::Printf(TEXT("%d 行动力"), ActionCost));
	}
	if (bUseClassicResources && ManaCost > 0)
	{
		CostParts.Add(FString::Printf(TEXT("%d 法力"), ManaCost));
	}
	if (!bUseClassicResources && (EnergyCost > 0 || ValorCost == 0))
	{
		CostParts.Add(FString::Printf(TEXT("%d 能量"), EnergyCost));
	}
	if (ValorCost > 0)
	{
		CostParts.Add(FString::Printf(TEXT("%d 英勇"), ValorCost));
	}
	if (CostParts.IsEmpty())
	{
		CostParts.Add(TEXT("无需资源"));
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
	if (CardType == ECardType::Equipment)
	{
		if (EquipmentAttackBonus > 0)
		{
			RuleFragments.Add(FString::Printf(TEXT("装备：攻击伤害 +%d"), EquipmentAttackBonus));
		}
		if (EquipmentTurnStartBlock > 0)
		{
			RuleFragments.Add(FString::Printf(TEXT("每回合格挡 +%d"), EquipmentTurnStartBlock));
		}
		if (EquipmentTurnStartDraw > 0)
		{
			RuleFragments.Add(FString::Printf(TEXT("每回合额外抽 %d 张"), EquipmentTurnStartDraw));
		}
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
