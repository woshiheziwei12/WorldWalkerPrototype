#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/W11Types.h"
#include "W11AttributeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FW11VitalChanged, float, CurrentValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FW11BarrierChanged, float, CurrentValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FW11Defeated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FW11StatusesChanged);

UCLASS(ClassGroup=(W11), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class WORLDWALKERW11_API UW11AttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UW11AttributeComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Attributes")
	void InitializeStats(const FW11StatBlock& InStats, bool bFillVitals = true);

	/** Restores only a previously sealed safe-node snapshot; transient statuses and immunity never persist. */
	void RestoreSafeNodeSnapshot(
		const FW11StatBlock& InStats,
		float InHealth,
		float InMana,
		float InBarrier);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Attributes")
	bool ApplyPermanentModifier(const FW11StatModifier& Modifier);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Attributes")
	float ApplyRawDamage(float RawDamage);

	FW11DamageBreakdown ApplyDamageSpec(const FW11DamageSpec& DamageSpec);

	bool ApplyStatusEffect(
		const FW11StatusEffectSpec& Spec,
		AActor* Source,
		FName SourceAbilityId,
		FW11ActiveStatus& OutStatus,
		bool& bOutRefreshed,
		bool bTargetIsBoss = false);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Status")
	bool DispelStatus(FName StatusId);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Status")
	void ClearAllStatuses();

	static FW11DamageBreakdown CalculateDamageBreakdown(
		float RawDamage, float Armor, float CurrentBarrier, float CurrentHealth, bool bImmune);
	static bool IsImmunityWindowActive(float CurrentServerTime, float ImmunityEndServerTime);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Attributes")
	void GrantDamageImmunity(float DurationSeconds);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Attributes")
	float Heal(float Amount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Attributes")
	bool SpendMana(float Amount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Attributes")
	float RestoreMana(float Amount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Attributes")
	float AddBarrier(float Amount);

	UFUNCTION(BlueprintPure, Category="W11|Attributes")
	const FW11StatBlock& GetStats() const { return Stats; }

	UFUNCTION(BlueprintPure, Category="W11|Attributes")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category="W11|Attributes")
	float GetMana() const { return Mana; }

	UFUNCTION(BlueprintPure, Category="W11|Attributes")
	float GetBarrier() const { return Barrier; }

	UFUNCTION(BlueprintPure, Category="W11|Attributes")
	bool IsDefeated() const { return Health <= 0.0f; }

	UFUNCTION(BlueprintPure, Category="W11|Attributes")
	bool IsDamageImmune() const;

	UFUNCTION(BlueprintPure, Category="W11|Status")
	const TArray<FW11ActiveStatus>& GetActiveStatuses() const { return ActiveStatuses; }

	UFUNCTION(BlueprintPure, Category="W11|Status")
	float GetMovementStatusScale() const;

	UPROPERTY(BlueprintAssignable)
	FW11VitalChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FW11VitalChanged OnManaChanged;

	UPROPERTY(BlueprintAssignable)
	FW11BarrierChanged OnBarrierChanged;

	UPROPERTY(BlueprintAssignable)
	FW11Defeated OnDefeated;

	UPROPERTY(BlueprintAssignable)
	FW11StatusesChanged OnStatusesChanged;

private:
	UFUNCTION()
	void OnRep_Stats();

	UFUNCTION()
	void OnRep_Health();

	UFUNCTION()
	void OnRep_Mana();

	UFUNCTION()
	void OnRep_Barrier();

	UFUNCTION()
	void OnRep_ActiveStatuses();

	void TickStatuses(float Now);

	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_Stats, Category="W11|Attributes")
	FW11StatBlock Stats;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_Health, Category="W11|Attributes")
	float Health = 100.0f;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_Mana, Category="W11|Attributes")
	float Mana = 100.0f;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_Barrier, Category="W11|Attributes")
	float Barrier = 0.0f;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_ActiveStatuses, Category="W11|Status")
	TArray<FW11ActiveStatus> ActiveStatuses;

	float DamageImmunityEndTime = -1.0f;
};
