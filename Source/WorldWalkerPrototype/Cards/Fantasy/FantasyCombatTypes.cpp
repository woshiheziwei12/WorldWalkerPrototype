#include "Cards/Fantasy/FantasyCombatTypes.h"

namespace
{
	const TCHAR* GetStatusLabel(const EFantasyCombatStatus Status)
	{
		switch (Status)
		{
		case EFantasyCombatStatus::Exposed: return TEXT("破绽");
		case EFantasyCombatStatus::Weak: return TEXT("虚弱");
		case EFantasyCombatStatus::Strength: return TEXT("力量");
		case EFantasyCombatStatus::Poison: return TEXT("中毒");
		case EFantasyCombatStatus::Burning: return TEXT("燃烧");
		case EFantasyCombatStatus::Chill: return TEXT("寒冷");
		default: return TEXT("状态");
		}
	}
}

FString FFantasyCombatEffectSpec::BuildRulesFragment() const
{
	const TCHAR* TargetLabel = Target == EFantasyCombatTarget::Self ? TEXT("自身") : TEXT("敌人");
	switch (EffectType)
	{
	case EFantasyCombatEffectType::Damage:
	{
		FString Fragment;
		if (bPiercing)
		{
			Fragment = FString::Printf(TEXT("造成 %d 点穿刺伤害"), Magnitude);
		}
		else
		{
			Fragment = FString::Printf(TEXT("造成 %d 点伤害"), Magnitude);
		}
		if (bScalesWithStrength)
		{
			Fragment += TEXT("（受力量加成）");
		}
		return Fragment;
	}
	case EFantasyCombatEffectType::Block:
		return FString::Printf(TEXT("获得 %d 点格挡"), Magnitude);
	case EFantasyCombatEffectType::Heal:
		return FString::Printf(TEXT("恢复 %d 点生命"), Magnitude);
	case EFantasyCombatEffectType::Draw:
		return FString::Printf(TEXT("抽 %d 张牌"), Magnitude);
	case EFantasyCombatEffectType::ApplyStatus:
		return FString::Printf(TEXT("对%s施加 %d 层%s"), TargetLabel, Magnitude, GetStatusLabel(Status));
	case EFantasyCombatEffectType::RemoveStatus:
		return FString::Printf(TEXT("为%s移除 %d 层%s"), TargetLabel, Magnitude, GetStatusLabel(Status));
	case EFantasyCombatEffectType::GainValor:
		return FString::Printf(TEXT("获得 %d 点英勇"), Magnitude);
	case EFantasyCombatEffectType::GainAction:
		return FString::Printf(TEXT("获得 %d 点行动力"), Magnitude);
	case EFantasyCombatEffectType::GainMana:
		return FString::Printf(TEXT("获得 %d 点法力"), Magnitude);
	case EFantasyCombatEffectType::DiscardRandom:
		return FString::Printf(TEXT("随机丢弃 %d 张牌"), Magnitude);
	case EFantasyCombatEffectType::LoseMana:
		return FString::Printf(TEXT("使%s失去 %d 点法力"), TargetLabel, Magnitude);
	case EFantasyCombatEffectType::AddTemporaryCard:
		return FString::Printf(TEXT("向%s弃牌堆加入 %d 张临时牌[%s]"), TargetLabel, Magnitude, *PayloadId.ToString());
	case EFantasyCombatEffectType::ConsumeStatusForDamage:
		return FString::Printf(TEXT("消耗%s全部%s，每层造成 %d 点伤害"), TargetLabel, GetStatusLabel(Status), Multiplier);
	case EFantasyCombatEffectType::ConsumeStatusForBlock:
		return FString::Printf(TEXT("消耗敌人全部%s，每层获得 %d 点格挡"), GetStatusLabel(Status), Multiplier);
	case EFantasyCombatEffectType::DamagePerMana:
		return FString::Printf(TEXT("造成 %d 点伤害，并按每点法力追加 %d 点"), Magnitude, Multiplier);
	case EFantasyCombatEffectType::CopySourceCard:
		return TEXT("复制触发牌的伤害、防御或状态效果");
	case EFantasyCombatEffectType::GrantFirstCardImmunity:
		return TEXT("免疫玩家下一张牌的敌方目标效果");
	default:
		return TEXT("未知效果");
	}
}

void FFantasyCombatRuntimeState::AddStatus(const EFantasyCombatStatus Status, const int32 Amount)
{
	const int32 SafeAmount = FMath::Max(0, Amount);
	switch (Status)
	{
	case EFantasyCombatStatus::Exposed: Exposed += SafeAmount; break;
	case EFantasyCombatStatus::Weak: Weak += SafeAmount; break;
	case EFantasyCombatStatus::Strength: Strength += SafeAmount; break;
	case EFantasyCombatStatus::Poison: Poison += SafeAmount; break;
	case EFantasyCombatStatus::Burning: Burning += SafeAmount; break;
	case EFantasyCombatStatus::Chill: Chill += SafeAmount; break;
	default: break;
	}
}

void FFantasyCombatRuntimeState::RemoveStatus(const EFantasyCombatStatus Status, const int32 Amount)
{
	const int32 SafeAmount = FMath::Max(0, Amount);
	switch (Status)
	{
	case EFantasyCombatStatus::Exposed: Exposed = FMath::Max(0, Exposed - SafeAmount); break;
	case EFantasyCombatStatus::Weak: Weak = FMath::Max(0, Weak - SafeAmount); break;
	case EFantasyCombatStatus::Strength: Strength = FMath::Max(0, Strength - SafeAmount); break;
	case EFantasyCombatStatus::Poison: Poison = FMath::Max(0, Poison - SafeAmount); break;
	case EFantasyCombatStatus::Burning: Burning = FMath::Max(0, Burning - SafeAmount); break;
	case EFantasyCombatStatus::Chill: Chill = FMath::Max(0, Chill - SafeAmount); break;
	default: break;
	}
}

int32 FFantasyCombatRuntimeState::GetStatus(const EFantasyCombatStatus Status) const
{
	switch (Status)
	{
	case EFantasyCombatStatus::Exposed: return Exposed;
	case EFantasyCombatStatus::Weak: return Weak;
	case EFantasyCombatStatus::Strength: return Strength;
	case EFantasyCombatStatus::Poison: return Poison;
	case EFantasyCombatStatus::Burning: return Burning;
	case EFantasyCombatStatus::Chill: return Chill;
	default: return 0;
	}
}

int32 FFantasyCombatRuntimeState::AbsorbDamage(const int32 IncomingDamage)
{
	const int32 SafeDamage = FMath::Max(0, IncomingDamage);
	const int32 Absorbed = FMath::Min(Block, SafeDamage);
	Block -= Absorbed;
	return SafeDamage - Absorbed;
}

FString FFantasyCombatRuntimeState::BuildSummary() const
{
	TArray<FString> Parts;
	if (Block > 0) Parts.Add(FString::Printf(TEXT("格挡 %d"), Block));
	if (Exposed > 0) Parts.Add(FString::Printf(TEXT("破绽 %d"), Exposed));
	if (Weak > 0) Parts.Add(FString::Printf(TEXT("虚弱 %d"), Weak));
	if (Strength > 0) Parts.Add(FString::Printf(TEXT("力量 %d"), Strength));
	if (Poison > 0) Parts.Add(FString::Printf(TEXT("中毒 %d"), Poison));
	if (Burning > 0) Parts.Add(FString::Printf(TEXT("燃烧 %d"), Burning));
	if (Chill > 0) Parts.Add(FString::Printf(TEXT("寒冷 %d"), Chill));
	return Parts.IsEmpty() ? TEXT("无状态") : FString::Join(Parts, TEXT(" | "));
}
