#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/W11Types.h"
#include "W11CombatComponent.generated.h"

class AW11Enemy;
class UW11AbilityDefinition;
class UW11AttributeComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FW11AbilityResultReceived, const FW11AbilityExecutionResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FW11CombatCueReceived, const FW11CombatCue&, Cue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FW11CooldownChanged, EW11AbilitySlot, Slot, float, CooldownEndServerTime);

/**
 * Server-authoritative ability gateway. Clients submit only a logical slot and
 * aiming intent; definitions, costs, targets and effects are resolved here.
 */
UCLASS(ClassGroup=(W11), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class WORLDWALKERW11_API UW11CombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UW11CombatComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category="W11|Combat")
	void TryBasicAttack(FVector2D Direction);

	UFUNCTION(BlueprintCallable, Category="W11|Combat")
	void TryInitialAbility(FVector2D Direction);

	UFUNCTION(BlueprintCallable, Category="W11|Combat")
	void TryAbilitySlot(EW11AbilitySlot Slot, FVector2D Direction, FVector TargetPoint = FVector::ZeroVector);

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	FVector2D GetLastAttackDirection() const { return LastAttackDirection; }

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	int32 GetAttackSequence() const { return AttackSequence; }

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	int32 GetAbilitySequence() const { return AbilitySequence; }

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	FName GetLastExecutedAbilityId() const { return LastExecutedAbilityId; }

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	float GetBasicAttackCooldownEndTime() const { return BasicAttackCooldownEndTime; }

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	float GetActiveAbilityCooldownEndTime() const { return ActiveAbilityCooldownEndTime; }

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	float GetAbilityCooldownEndTime(EW11AbilitySlot Slot) const;

	/** Clears per-run cooldown and presentation state while preserving monotonic authority sequences. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Combat")
	void ResetForNewMemoryRun();

	static bool IsNewerRequestSequence(int32 Candidate, int32 Previous);
	static bool IsValidRequestInput(const FW11AbilityRequest& Request);
	static FW11EffectResult BuildNonDamageEffectResult(
		EW11EffectResultType Type,
		AActor* Source,
		AActor* Target,
		FName AbilityId,
		float RequestedAmount,
		float ActualAmount);
	static float CalculateAbilityCooldown(
		float BaseCooldown,
		EW11CooldownScaling Scaling,
		const FW11StatBlock& Stats);
	static bool IsStableTargetBefore(
		float LeftDistanceSquared,
		int32 LeftStableId,
		float RightDistanceSquared,
		int32 RightStableId);
	static bool IsBasicAttackTargetInForgivingSweep(
		const FVector& Start,
		const FVector& WorldDirection,
		const FVector& TargetLocation,
		float Range,
		float Radius,
		float TargetCapsuleRadius);
	static FW11AbilityExecutionResult BuildRejectedAbilityResult(
		const FW11AbilityRequest& Request,
		EW11AbilityRejectionReason Reason,
		float CooldownEndTime,
		int32 AuthorityExecutionSequence,
		AActor* Source,
		FName AbilityId);

	UPROPERTY(BlueprintAssignable, Category="W11|Combat")
	FW11AbilityResultReceived OnAbilityResult;

	UPROPERTY(BlueprintAssignable, Category="W11|Combat")
	FW11CombatCueReceived OnCombatCue;

	UPROPERTY(BlueprintAssignable, Category="W11|Combat")
	FW11CooldownChanged OnCooldownChanged;

protected:
	UFUNCTION(Server, Reliable)
	void ServerSubmitAbilityRequest(FW11AbilityRequest Request);

	UFUNCTION(Client, Reliable)
	void ClientReceiveAbilityResult(FW11AbilityExecutionResult Result);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastCombatCue(FW11CombatCue Cue);

private:
	void SubmitLocalRequest(EW11AbilitySlot Slot, const FVector2D& Direction, const FVector& TargetPoint);
	void ExecuteServerRequest(const FW11AbilityRequest& Request);
	void SendRejectedResult(
		const FW11AbilityRequest& Request,
		EW11AbilityRejectionReason Reason,
		float CooldownEndTime = 0.0f);
	UW11AbilityDefinition* ResolveSlottedAbility(EW11AbilitySlot Slot) const;
	UW11AttributeComponent* ResolveOwnerAttributes() const;
	TArray<AW11Enemy*> FindStableTargets(
		const UW11AbilityDefinition* Ability,
		const FVector2D& Direction,
		const FVector& TargetPoint,
		int32 EncounterInstanceId) const;
	bool RollCritical(
		const UW11AbilityDefinition* Ability,
		const FW11StatBlock& Stats,
		int32 ExecutionSequence,
		int32 TargetStableId) const;
	void BroadcastAcceptedResult(const FW11AbilityExecutionResult& Result);

	UFUNCTION()
	void OnRep_BasicAttackCooldown();

	UFUNCTION()
	void OnRep_ActiveAbilityCooldowns();

	UPROPERTY(Replicated)
	FVector2D LastAttackDirection = FVector2D(0.0f, 1.0f);

	UPROPERTY(Replicated)
	int32 AttackSequence = 0;

	UPROPERTY(Replicated)
	int32 AbilitySequence = 0;

	UPROPERTY(Replicated)
	FName LastExecutedAbilityId = NAME_None;

	UPROPERTY(Replicated)
	int32 AuthorityExecutionSequence = 0;

	UPROPERTY(ReplicatedUsing=OnRep_BasicAttackCooldown)
	float BasicAttackCooldownEndTime = 0.0f;

	/** Active1..Active4 owner-only server timestamps. */
	UPROPERTY(ReplicatedUsing=OnRep_ActiveAbilityCooldowns)
	TArray<float> ActiveAbilityCooldownEndTimes;

	/** Compatibility accessor used by the existing Active1 presentation. */
	float ActiveAbilityCooldownEndTime = 0.0f;

	int32 LocalRequestSequence = 0;
	int32 LastClientRequestSequence = 0;
};
