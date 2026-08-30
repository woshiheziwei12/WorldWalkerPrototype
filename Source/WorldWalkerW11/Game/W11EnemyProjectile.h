#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/W11Types.h"
#include "W11EnemyProjectile.generated.h"

class AW11Enemy;
class UW11AbilityDefinition;
class USphereComponent;
class UStaticMeshComponent;

/** Server-driven, encounter-scoped enemy projectile with single-hit semantics. */
UCLASS()
class WORLDWALKERW11_API AW11EnemyProjectile : public AActor
{
	GENERATED_BODY()

public:
	AW11EnemyProjectile();
	virtual void Tick(float DeltaSeconds) override;
	virtual void LifeSpanExpired() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeProjectile(
		AW11Enemy* InSource,
		const UW11AbilityDefinition* Ability,
		const FVector& Direction,
		int32 InEncounterInstanceId,
		int32 InAttackInstanceId);

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	int32 GetEncounterInstanceId() const { return EncounterInstanceId; }

	/** Atomically claims the one allowed resolution in a projectile lifecycle. */
	static bool TryClaimResolution(bool& bInOutResolved);

private:
	bool IsEncounterCurrent() const;
	bool TryHitAlongSegment(const FVector& Start, const FVector& End);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> VisualComponent;

	UPROPERTY(Replicated)
	TObjectPtr<AW11Enemy> SourceEnemy;

	UPROPERTY(Replicated)
	int32 EncounterInstanceId = 0;

	UPROPERTY(Replicated)
	int32 AttackInstanceId = 0;

	UPROPERTY(Replicated)
	FName AbilityId = NAME_None;

	UPROPERTY(Replicated)
	FVector_NetQuantizeNormal TravelDirection = FVector::ForwardVector;

	float Speed = 0.0f;
	float Radius = 24.0f;
	float RawDamage = 0.0f;
	TArray<FW11StatusEffectSpec> StatusEffects;
	bool bResolved = false;
};
