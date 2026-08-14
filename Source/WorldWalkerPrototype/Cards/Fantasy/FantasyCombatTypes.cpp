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
		return FString::Printf(TEXT("造成 %d 点伤害"), Magnitude);
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
	default: break;
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
	return Parts.IsEmpty() ? TEXT("无状态") : FString::Join(Parts, TEXT(" | "));
}
