#include "Components/W11AttributeComponent.h"

#include "Core/W11StatLibrary.h"
#include "Game/W11GameMode.h"
#include "Game/W11PlayerState.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "WorldWalkerW11.h"

UW11AttributeComponent::UW11AttributeComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UW11AttributeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UW11AttributeComponent, Stats);
	DOREPLIFETIME(UW11AttributeComponent, Health);
	DOREPLIFETIME(UW11AttributeComponent, Mana);
	DOREPLIFETIME(UW11AttributeComponent, Barrier);
	DOREPLIFETIME(UW11AttributeComponent, ActiveStatuses);
}

void UW11AttributeComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		TickStatuses(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f);
		if (Health > 0.0f && Mana < Stats.MaxMana)
		{
			RestoreMana(Stats.ManaRegenPerSecond * DeltaTime);
		}
	}
}

void UW11AttributeComponent::InitializeStats(const FW11StatBlock& InStats, const bool bFillVitals)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	Stats = InStats;
	UW11StatLibrary::Sanitize(Stats);
	if (bFillVitals)
	{
		Health = Stats.MaxHealth;
		Mana = Stats.MaxMana;
		Barrier = 0.0f;
		// A filled initialization is a hard run boundary, not a heal. Do not let
		// the tail of a dodge immunity window leak into the next memory run.
		DamageImmunityEndTime = -1.0f;
		ActiveStatuses.Reset();
		OnRep_ActiveStatuses();
	}
	else
	{
		Health = FMath::Clamp(Health, 0.0f, Stats.MaxHealth);
		Mana = FMath::Clamp(Mana, 0.0f, Stats.MaxMana);
	}
	OnRep_Stats();
	OnRep_Health();
	OnRep_Mana();
}

void UW11AttributeComponent::RestoreSafeNodeSnapshot(
	const FW11StatBlock& InStats,
	const float InHealth,
	const float InMana,
	const float InBarrier)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	Stats = InStats;
	UW11StatLibrary::Sanitize(Stats);
	Health = FMath::Clamp(InHealth, UE_KINDA_SMALL_NUMBER, Stats.MaxHealth);
	Mana = FMath::Clamp(InMana, 0.0f, Stats.MaxMana);
	Barrier = FMath::Max(0.0f, InBarrier);
	DamageImmunityEndTime = -1.0f;
	ActiveStatuses.Reset();
	OnRep_Stats();
	OnRep_Health();
	OnRep_Mana();
	OnRep_Barrier();
	OnRep_ActiveStatuses();
}

bool UW11AttributeComponent::ApplyPermanentModifier(const FW11StatModifier& Modifier)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	const float OldMaxHealth = Stats.MaxHealth;
	const float OldMaxMana = Stats.MaxMana;
	UW11StatLibrary::ApplyModifier(Stats, Modifier);
	if (Stats.MaxHealth > OldMaxHealth)
	{
		Health += Stats.MaxHealth - OldMaxHealth;
	}
	if (Stats.MaxMana > OldMaxMana)
	{
		Mana += Stats.MaxMana - OldMaxMana;
	}
	Health = FMath::Clamp(Health, 0.0f, Stats.MaxHealth);
	Mana = FMath::Clamp(Mana, 0.0f, Stats.MaxMana);
	OnRep_Stats();
	OnRep_Health();
	OnRep_Mana();
	return true;
}

float UW11AttributeComponent::ApplyRawDamage(const float RawDamage)
{
	FW11DamageSpec DamageSpec;
	DamageSpec.RawDamage = RawDamage;
	return ApplyDamageSpec(DamageSpec).HealthDamage;
}

FW11DamageBreakdown UW11AttributeComponent::CalculateDamageBreakdown(
	const float RawDamage,
	const float Armor,
	const float CurrentBarrier,
	const float CurrentHealth,
	const bool bImmune)
{
	FW11DamageBreakdown Result;
	Result.RawDamage = FMath::Max(0.0f, RawDamage);
	Result.bImmune = bImmune;
	if (Result.RawDamage <= 0.0f || CurrentHealth <= 0.0f || bImmune)
	{
		return Result;
	}
	const float AfterArmor = Result.RawDamage * (1.0f - UW11StatLibrary::GetArmorDamageReduction(Armor));
	Result.ArmorMitigated = Result.RawDamage - AfterArmor;
	Result.BarrierAbsorbed = FMath::Min(FMath::Max(0.0f, CurrentBarrier), AfterArmor);
	Result.HealthDamage = FMath::Min(FMath::Max(0.0f, CurrentHealth), AfterArmor - Result.BarrierAbsorbed);
	Result.bDefeated = Result.HealthDamage + UE_KINDA_SMALL_NUMBER >= CurrentHealth;
	return Result;
}

FW11DamageBreakdown UW11AttributeComponent::ApplyDamageSpec(const FW11DamageSpec& DamageSpec)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return {};
	}
	FW11DamageBreakdown Result = CalculateDamageBreakdown(
		DamageSpec.RawDamage, Stats.Armor, Barrier, Health, IsDamageImmune());
	bool bNewlyDefeated = false;
	if (Result.BarrierAbsorbed > 0.0f)
	{
		Barrier = FMath::Max(0.0f, Barrier - Result.BarrierAbsorbed);
		OnRep_Barrier();
	}
	if (Result.HealthDamage > 0.0f)
	{
		const float PreviousHealth = Health;
		Health = FMath::Max(0.0f, Health - Result.HealthDamage);
		OnRep_Health();
		bNewlyDefeated = PreviousHealth > 0.0f && Health <= 0.0f;
	}
	if (AW11GameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AW11GameMode>() : nullptr)
	{
		GameMode->RecordCombatDamage(GetOwner(), DamageSpec, Result);
	}
	if (bNewlyDefeated)
	{
		OnDefeated.Broadcast();
	}
	return Result;
}

bool UW11AttributeComponent::ApplyStatusEffect(
	const FW11StatusEffectSpec& Spec,
	AActor* Source,
	const FName SourceAbilityId,
	FW11ActiveStatus& OutStatus,
	bool& bOutRefreshed,
	const bool bTargetIsBoss)
{
	bOutRefreshed = false;
	if (!GetOwner() || !GetOwner()->HasAuthority() || !GetWorld() || Health <= 0.0f
		|| Spec.StatusId.IsNone() || Spec.Type == EW11StatusType::None
		|| Spec.DurationSeconds <= 0.0f || Spec.Magnitude <= 0.0f)
	{
		return false;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	const float Duration = Spec.DurationSeconds * (bTargetIsBoss ? Spec.BossDurationScale : 1.0f);
	if (Duration <= 0.0f)
	{
		return false;
	}
	FW11ActiveStatus* Existing = ActiveStatuses.FindByPredicate([&](const FW11ActiveStatus& Status)
	{
		return Status.StatusId == Spec.StatusId && Status.Source == Source;
	});
	if (Existing)
	{
		bOutRefreshed = true;
		if (Spec.StackingRule == EW11StatusStackingRule::AddStackAndRefresh)
		{
			Existing->Stacks = FMath::Min(FMath::Max(1, Spec.MaxStacks), Existing->Stacks + 1);
		}
		Existing->Magnitude = Spec.Magnitude;
		Existing->EndServerTime = Now + Duration;
		Existing->SourceAbilityId = SourceAbilityId;
		OutStatus = *Existing;
	}
	else
	{
		FW11ActiveStatus& Added = ActiveStatuses.AddDefaulted_GetRef();
		Added.StatusId = Spec.StatusId;
		Added.Type = Spec.Type;
		Added.Source = Source;
		Added.SourceAbilityId = SourceAbilityId;
		Added.Stacks = 1;
		Added.Magnitude = Spec.Magnitude;
		Added.TickIntervalSeconds = FMath::Max(0.05f, Spec.TickIntervalSeconds);
		Added.StartServerTime = Now;
		Added.EndServerTime = Now + Duration;
		Added.NextTickServerTime = Now + Added.TickIntervalSeconds;
		Added.MaxStacks = FMath::Max(1, Spec.MaxStacks);
		Added.StackingRule = Spec.StackingRule;
		Added.bDispellable = Spec.bDispellable;
		OutStatus = Added;
	}
	OnRep_ActiveStatuses();
	GetOwner()->ForceNetUpdate();
	return true;
}

bool UW11AttributeComponent::DispelStatus(const FName StatusId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || StatusId.IsNone())
	{
		return false;
	}
	const int32 Removed = ActiveStatuses.RemoveAll([StatusId](const FW11ActiveStatus& Status)
	{
		return Status.StatusId == StatusId && Status.bDispellable;
	});
	if (Removed > 0)
	{
		OnRep_ActiveStatuses();
		GetOwner()->ForceNetUpdate();
	}
	return Removed > 0;
}

void UW11AttributeComponent::ClearAllStatuses()
{
	if (GetOwner() && GetOwner()->HasAuthority() && !ActiveStatuses.IsEmpty())
	{
		ActiveStatuses.Reset();
		OnRep_ActiveStatuses();
		GetOwner()->ForceNetUpdate();
	}
}

float UW11AttributeComponent::GetMovementStatusScale() const
{
	float Scale = 1.0f;
	for (const FW11ActiveStatus& Status : ActiveStatuses)
	{
		if (Status.Type == EW11StatusType::Slow)
		{
			Scale = FMath::Min(Scale, FMath::Pow(FMath::Clamp(Status.Magnitude, 0.1f, 1.0f), Status.Stacks));
		}
	}
	return Scale;
}

void UW11AttributeComponent::TickStatuses(const float Now)
{
	bool bChanged = false;
	for (int32 Index = ActiveStatuses.Num() - 1; Index >= 0; --Index)
	{
		FW11ActiveStatus& Status = ActiveStatuses[Index];
		if (Now + UE_KINDA_SMALL_NUMBER >= Status.EndServerTime || Health <= 0.0f)
		{
			ActiveStatuses.RemoveAt(Index);
			bChanged = true;
			continue;
		}
		if (Status.Type == EW11StatusType::Burn && Now + UE_KINDA_SMALL_NUMBER >= Status.NextTickServerTime)
		{
			const FName StatusId = Status.StatusId;
			AActor* const Source = Status.Source.Get();
			const FName SourceAbilityId = Status.SourceAbilityId;
			const int32 Stacks = Status.Stacks;
			const float EndServerTime = Status.EndServerTime;
			const float TickInterval = FMath::Max(0.1f, Status.TickIntervalSeconds);
			Status.NextTickServerTime += TickInterval;
			if (Status.NextTickServerTime <= Now)
			{
				Status.NextTickServerTime = Now + TickInterval;
			}

			FW11DamageSpec Spec;
			Spec.Source = Source;
			Spec.AbilityId = SourceAbilityId;
			Spec.RawDamage = Status.Magnitude * Stacks;
			Spec.ImpactLocation = GetOwner()->GetActorLocation();
			const FW11DamageBreakdown Damage = ApplyDamageSpec(Spec);
			bChanged = true;
			UE_LOG(LogWorldWalkerW11, Display,
				TEXT("W11_STATUS_TICK Target=%s Status=%s Stacks=%d Damage=%.1f End=%.3f"),
				*GetNameSafe(GetOwner()), *StatusId.ToString(), Stacks,
				Damage.HealthDamage, EndServerTime);

			// Damage can synchronously finalize the encounter and clear this array.
			if (!ActiveStatuses.IsValidIndex(Index))
			{
				break;
			}
		}
	}
	if (bChanged)
	{
		OnRep_ActiveStatuses();
		if (GetOwner())
		{
			GetOwner()->ForceNetUpdate();
		}
	}
}

void UW11AttributeComponent::GrantDamageImmunity(const float DurationSeconds)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !GetWorld() || DurationSeconds <= 0.0f)
	{
		return;
	}
	DamageImmunityEndTime = FMath::Max(
		DamageImmunityEndTime,
		GetWorld()->GetTimeSeconds() + DurationSeconds);
}

bool UW11AttributeComponent::IsDamageImmune() const
{
	return GetWorld() && IsImmunityWindowActive(GetWorld()->GetTimeSeconds(), DamageImmunityEndTime);
}

bool UW11AttributeComponent::IsImmunityWindowActive(
	const float CurrentServerTime,
	const float ImmunityEndServerTime)
{
	return CurrentServerTime < ImmunityEndServerTime;
}

float UW11AttributeComponent::Heal(const float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0.0f || Health <= 0.0f)
	{
		return 0.0f;
	}
	const float PreviousHealth = Health;
	Health = FMath::Min(Stats.MaxHealth, Health + Amount * Stats.HealingReceivedScale);
	OnRep_Health();
	return Health - PreviousHealth;
}

bool UW11AttributeComponent::SpendMana(const float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount < 0.0f || Mana + UE_KINDA_SMALL_NUMBER < Amount)
	{
		return false;
	}
	Mana = FMath::Max(0.0f, Mana - Amount);
	OnRep_Mana();
	return true;
}

float UW11AttributeComponent::RestoreMana(const float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0.0f)
	{
		return 0.0f;
	}
	const float PreviousMana = Mana;
	Mana = FMath::Min(Stats.MaxMana, Mana + Amount);
	if (!FMath::IsNearlyEqual(PreviousMana, Mana))
	{
		OnRep_Mana();
	}
	const float Restored = Mana - PreviousMana;
	if (Restored > 0.0f && Cast<AW11PlayerState>(GetOwner()))
	{
		if (AW11GameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AW11GameMode>() : nullptr)
		{
			GameMode->RecordManaRestored(Restored);
		}
	}
	return Restored;
}

float UW11AttributeComponent::AddBarrier(const float Amount)
{
	if (GetOwner() && GetOwner()->HasAuthority() && Amount > 0.0f)
	{
		Barrier += Amount;
		OnRep_Barrier();
		return Amount;
	}
	return 0.0f;
}

void UW11AttributeComponent::OnRep_Stats()
{
	OnHealthChanged.Broadcast(Health, Stats.MaxHealth);
	OnManaChanged.Broadcast(Mana, Stats.MaxMana);
}

void UW11AttributeComponent::OnRep_Health()
{
	OnHealthChanged.Broadcast(Health, Stats.MaxHealth);
}

void UW11AttributeComponent::OnRep_Mana()
{
	OnManaChanged.Broadcast(Mana, Stats.MaxMana);
}

void UW11AttributeComponent::OnRep_Barrier()
{
	OnBarrierChanged.Broadcast(Barrier);
}

void UW11AttributeComponent::OnRep_ActiveStatuses()
{
	OnStatusesChanged.Broadcast();
}
