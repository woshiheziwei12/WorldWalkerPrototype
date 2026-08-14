#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatantComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnCombatantHealthChanged,
	int32, CurrentHealth,
	int32, MaxHealth);

UCLASS(ClassGroup=(WorldWalker), meta=(BlueprintSpawnableComponent))
class WORLDWALKERPROTOTYPE_API UCombatantComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatantComponent();

	UFUNCTION(BlueprintCallable, Category="Combat")
	void ReceiveDamage(int32 DamageAmount);

	UFUNCTION(BlueprintCallable, Category="Combat")
	void RestoreHealth(int32 HealAmount);

	UFUNCTION(BlueprintCallable, Category="Combat")
	void ResetHealth();

	/** Reconfigures a combatant before an encounter and immediately refills its health. */
	UFUNCTION(BlueprintCallable, Category="Combat")
	void ConfigureMaxHealth(int32 NewMaxHealth);

	UFUNCTION(BlueprintPure, Category="Combat")
	int32 GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category="Combat")
	int32 GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category="Combat")
	bool IsAlive() const { return CurrentHealth > 0; }

	UPROPERTY(BlueprintAssignable, Category="Combat")
	FOnCombatantHealthChanged OnHealthChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat", meta=(ClampMin="1"))
	int32 MaxHealth = 100;

private:
	UPROPERTY(VisibleInstanceOnly, Category="Combat")
	int32 CurrentHealth = 100;
};
