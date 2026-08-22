#pragma once

#include "CoreMinimal.h"
#include "FantasyCombatTypes.generated.h"

UENUM(BlueprintType)
enum class ECardType : uint8
{
	Attack,
	Skill,
	Spell,
	Oath,
	Action,
	Mana,
	Equipment,
	Counter,
	Prayer,
	Special
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
	GainValor,
	GainAction,
	GainMana,
	DiscardRandom,
	LoseMana,
	AddTemporaryCard,
	ConsumeStatusForDamage,
	ConsumeStatusForBlock,
	DamagePerMana
};

UENUM(BlueprintType)
enum class EFantasyCombatStatus : uint8
{
	None,
	Exposed,
	Weak,
	Strength,
	Poison,
	Burning,
	Chill
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

	/** Stable payload ID, currently used by AddTemporaryCard. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect")
	FName PayloadId;

	/** Per-battle insertion cap for AddTemporaryCard; zero means uncapped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect", meta=(ClampMin="0"))
	int32 Limit = 0;

	/** Scalar used by status-consumption and mana-scaling effects. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect", meta=(ClampMin="0"))
	int32 Multiplier = 1;

	/** Piercing damage bypasses temporary Block but still receives other modifiers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect")
	bool bPiercing = false;

	/** Allows non-Attack damage (for example a weapon-like Spell) to receive Strength. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effect")
	bool bScalesWithStrength = false;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status")
	int32 Poison = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status")
	int32 Burning = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Status")
	int32 Chill = 0;

	int32 GetStatus(EFantasyCombatStatus Status) const;

	void AddStatus(EFantasyCombatStatus Status, int32 Amount);
	void RemoveStatus(EFantasyCombatStatus Status, int32 Amount);
	int32 AbsorbDamage(int32 IncomingDamage);
	FString BuildSummary() const;
};
