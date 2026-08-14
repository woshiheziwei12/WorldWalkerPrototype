#include "Combat/CombatantComponent.h"

UCombatantComponent::UCombatantComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatantComponent::BeginPlay()
{
	Super::BeginPlay();
	ResetHealth();
}

void UCombatantComponent::ReceiveDamage(const int32 DamageAmount)
{
	if (DamageAmount <= 0 || !IsAlive())
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UCombatantComponent::RestoreHealth(const int32 HealAmount)
{
	if (HealAmount <= 0 || !IsAlive())
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UCombatantComponent::ResetHealth()
{
	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UCombatantComponent::ConfigureMaxHealth(const int32 NewMaxHealth)
{
	MaxHealth = FMath::Max(1, NewMaxHealth);
	ResetHealth();
}
