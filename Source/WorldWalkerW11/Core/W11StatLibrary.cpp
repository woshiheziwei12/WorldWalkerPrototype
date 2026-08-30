#include "Core/W11StatLibrary.h"

namespace
{
void ApplyOperation(float& Value, const EW11ModifierOperation Operation, const float Magnitude)
{
	switch (Operation)
	{
	case EW11ModifierOperation::Add:
		Value += Magnitude;
		break;
	case EW11ModifierOperation::Multiply:
		Value *= Magnitude;
		break;
	case EW11ModifierOperation::Override:
		Value = Magnitude;
		break;
	default:
		break;
	}
}
}

float UW11StatLibrary::GetEffectiveLuck(const float Luck)
{
	const float SafeLuck = FMath::Max(0.0f, Luck);
	return SafeLuck / (SafeLuck + 100.0f);
}

float UW11StatLibrary::GetArmorDamageReduction(const float Armor)
{
	const float SafeArmor = FMath::Max(0.0f, Armor);
	return FMath::Min(0.75f, SafeArmor / (SafeArmor + 100.0f));
}

float UW11StatLibrary::GetFinalAreaRadius(const float BaseRadius, const float AreaScale)
{
	return FMath::Max(0.0f, BaseRadius) * FMath::Sqrt(FMath::Max(0.01f, AreaScale));
}

float UW11StatLibrary::GetFinalCooldown(const float BaseCooldown, const float AbilityHaste)
{
	return FMath::Max(0.05f, BaseCooldown / (1.0f + FMath::Max(-0.9f, AbilityHaste)));
}

void UW11StatLibrary::ApplyModifier(FW11StatBlock& Stats, const FW11StatModifier& Modifier)
{
	float* Target = nullptr;
	switch (Modifier.Stat)
	{
	case EW11StatType::MaxHealth: Target = &Stats.MaxHealth; break;
	case EW11StatType::MaxMana: Target = &Stats.MaxMana; break;
	case EW11StatType::ManaRegen: Target = &Stats.ManaRegenPerSecond; break;
	case EW11StatType::Power: Target = &Stats.Power; break;
	case EW11StatType::Armor: Target = &Stats.Armor; break;
	case EW11StatType::MoveSpeed: Target = &Stats.MoveSpeedScale; break;
	case EW11StatType::AttackSpeed: Target = &Stats.AttackSpeedScale; break;
	case EW11StatType::AbilityHaste: Target = &Stats.AbilityHaste; break;
	case EW11StatType::CritChance: Target = &Stats.CritChance; break;
	case EW11StatType::CritMultiplier: Target = &Stats.CritMultiplier; break;
	case EW11StatType::Luck: Target = &Stats.Luck; break;
	case EW11StatType::AttackRange: Target = &Stats.AttackRangeScale; break;
	case EW11StatType::Area: Target = &Stats.AreaScale; break;
	case EW11StatType::PickupRadius: Target = &Stats.PickupRadius; break;
	case EW11StatType::Duration: Target = &Stats.DurationScale; break;
	case EW11StatType::HealingReceived: Target = &Stats.HealingReceivedScale; break;
	default: break;
	}

	if (Target)
	{
		ApplyOperation(*Target, Modifier.Operation, Modifier.Magnitude);
		Sanitize(Stats);
	}
}

void UW11StatLibrary::Sanitize(FW11StatBlock& Stats)
{
	Stats.MaxHealth = FMath::Max(1.0f, Stats.MaxHealth);
	Stats.MaxMana = FMath::Max(0.0f, Stats.MaxMana);
	Stats.ManaRegenPerSecond = FMath::Max(0.0f, Stats.ManaRegenPerSecond);
	Stats.Power = FMath::Max(0.05f, Stats.Power);
	Stats.Armor = FMath::Max(0.0f, Stats.Armor);
	Stats.MoveSpeedScale = FMath::Clamp(Stats.MoveSpeedScale, 0.25f, 3.0f);
	Stats.AttackSpeedScale = FMath::Clamp(Stats.AttackSpeedScale, 0.1f, 5.0f);
	Stats.AbilityHaste = FMath::Clamp(Stats.AbilityHaste, -0.5f, 4.0f);
	Stats.CritChance = FMath::Clamp(Stats.CritChance, 0.0f, 1.0f);
	Stats.CritMultiplier = FMath::Max(1.0f, Stats.CritMultiplier);
	Stats.Luck = FMath::Max(0.0f, Stats.Luck);
	Stats.AttackRangeScale = FMath::Clamp(Stats.AttackRangeScale, 0.25f, 5.0f);
	Stats.AreaScale = FMath::Clamp(Stats.AreaScale, 0.1f, 9.0f);
	Stats.PickupRadius = FMath::Clamp(Stats.PickupRadius, 0.0f, 5000.0f);
	Stats.DurationScale = FMath::Clamp(Stats.DurationScale, 0.1f, 10.0f);
	Stats.HealingReceivedScale = FMath::Clamp(Stats.HealingReceivedScale, 0.0f, 5.0f);
}
