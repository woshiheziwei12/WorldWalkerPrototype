#pragma once

#include "CoreMinimal.h"
#include "FantasyCombatTypes.generated.h"

UENUM(BlueprintType)
enum class ECardType : uint8
{
	Attack,
	Skill,
	Spell,
	Oath
};

UENUM(BlueprintType)
enum class ECardSchool : uint8
{
	None,
	Steel,
	Faith,
	Arcane
};

UENUM(BlueprintType)
enum class EFantasyCombatTarget : uint8
{
	Self,
	Opponent
};

UENUM(BlueprintType)
enum class EFantasyCombatEffectType : uint8
{
	Damage,
	Block,
	Heal,
	Draw,
	ApplyStatus,
	RemoveStatus,
	GainValor
};

UENUM(BlueprintType)
enum class EFantasyCombatStatus : uint8
{
	None,
	Exposed,
	Weak,
	Strength
};

USTRUCT(BlueprintType)
struct WORLDWALKERPROTOTYPE_API FFantasyCombatEffectSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect")
	EFantasyCombatEffectType EffectType = EFantasyCombatEffectType::Damage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect")
	EFantasyCombatTarget Target = EFantasyCombatTarget::Opponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect", meta=(ClampMin="0"))
	int32 Magnitude = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect")
	EFantasyCombatStatus Status = EFantasyCombatStatus::None;

	FString BuildRulesFragment() const;
};

USTRUCT(BlueprintType)
struct WORLDWALKERPROTOTYPE_API FFantasyCombatRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status")
	int32 Block = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status")
	int32 Exposed = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status")
	int32 Weak = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status")
	int32 Strength = 0;

	void AddStatus(EFantasyCombatStatus Status, int32 Amount);
	void RemoveStatus(EFantasyCombatStatus Status, int32 Amount);
	int32 AbsorbDamage(int32 IncomingDamage);
	FString BuildSummary() const;
};
