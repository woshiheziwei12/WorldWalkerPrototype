#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Core/W11Types.h"
#include "W11EnemyController.generated.h"

class AW11Character;
class AW11Enemy;
class UW11AbilityDefinition;

/** Explicit server-only decision state machine; the recoverable snapshot lives on AW11Enemy. */
UCLASS()
class WORLDWALKERW11_API AW11EnemyController : public AAIController
{
	GENERATED_BODY()

public:
	AW11EnemyController();
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	AW11Character* FindNearestTarget() const;
	bool IsValidTarget(const AW11Character* Target) const;
	bool IsEncounterCurrent(const AW11Enemy* Enemy) const;
	void SetLocomotionState(AW11Enemy* Enemy, EW11EnemyCombatState NewState);
	void BeginWindup(AW11Enemy* Enemy, AW11Character* Target, const UW11AbilityDefinition* Ability);
	void ResolveActive(AW11Enemy* Enemy, const UW11AbilityDefinition* Ability);
	void ApplyImmediateAttack(
		AW11Enemy* Enemy,
		const UW11AbilityDefinition* Ability,
		const FW11ReplicatedAttackState& AttackState);
	void CancelCurrentAttack(AW11Enemy* Enemy, EW11EnemyCombatState NextState);

	TWeakObjectPtr<AW11Character> CurrentTarget;
	int32 AttackInstanceSequence = 0;
	bool bActiveResolved = false;
	bool bLoggedMissingAbility = false;
	bool bLoggedMissingTarget = false;
};
