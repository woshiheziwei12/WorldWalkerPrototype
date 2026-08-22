#pragma once

#include "CoreMinimal.h"
#include "Cards/Fantasy/FantasyCombatTypes.h"
#include "Engine/DataAsset.h"
#include "FantasyEnemyDefinition.generated.h"

class UCardDefinition;

/** Stable presentation profile selected independently from combat rules. */
UENUM(BlueprintType)
enum class EFantasyEnemyVisualProfile : uint8
{
	Warrior,
	Skeleton,
	Bat,
	Dragon,
	Slime,
	Wizard
};

/** Route classification is authored on the encounter and never inferred by the route picker. */
UENUM(BlueprintType)
enum class EFantasyEncounterTier : uint8
{
	Normal,
	Elite,
	Boss
};

/** One concrete card definition and the number of copies in an enemy deck. */
USTRUCT(BlueprintType)
struct WORLDWALKERPROTOTYPE_API FFantasyEnemyDeckEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Deck")
	TObjectPtr<UCardDefinition> Card;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Deck", meta=(ClampMin="1"))
	int32 Copies = 1;
};

USTRUCT(BlueprintType)
struct WORLDWALKERPROTOTYPE_API FFantasyEnemyIntentStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Intent")
	FName IntentId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Intent")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Intent")
	TArray<FFantasyCombatEffectSpec> Effects;

	FString BuildPreviewText(int32 CurrentStrength) const;
};

UENUM(BlueprintType)
enum class EFantasyMechanicTrigger : uint8
{
	BattleStarted,
	EnemyTurnStarted,
	EnemyAttackResolved,
	PlayerCardResolved,
	DamageResolved
};

UENUM(BlueprintType)
enum class EFantasyMechanicLimit : uint8
{
	Unlimited,
	OncePerTurn,
	OncePerBattle
};

/** Data-authored combat reaction shared by all enemy definitions; no enemy-name branching is required. */
USTRUCT(BlueprintType)
struct WORLDWALKERPROTOTYPE_API FFantasyCombatMechanicRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mechanic")
	FName MechanicId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mechanic")
	EFantasyMechanicTrigger Trigger = EFantasyMechanicTrigger::EnemyTurnStarted;

	/** Optional source-card build tag requirement, for example Element.Fire. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mechanic")
	FName RequiredSourceCardTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mechanic", meta=(ClampMin="0"))
	int32 MinActualDamage = 0;

	/** Negative disables the pre-hit target-block check. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mechanic", meta=(ClampMin="-1"))
	int32 MaxTargetBlock = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mechanic")
	EFantasyMechanicLimit Limit = EFantasyMechanicLimit::Unlimited;

	/** When set, triggering this rule suppresses the target rule for SuppressTurns enemy turns. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mechanic")
	FName SuppressTargetMechanicId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mechanic", meta=(ClampMin="0"))
	int32 SuppressTurns = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mechanic")
	TArray<FFantasyCombatEffectSpec> Effects;
};

UCLASS(BlueprintType)
class WORLDWALKERPROTOTYPE_API UFantasyEnemyDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy")
	FName EnemyId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Progression")
	EFantasyEncounterTier EncounterTier = EFantasyEncounterTier::Normal;

	/** Stable taxonomy used by route constraints and future family-specific events. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Progression")
	FName Family;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Progression", meta=(ClampMin="1"))
	int32 Chapter = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Progression", meta=(ClampMin="1"))
	int32 DangerRating = 1;

	/** Empty means available by default; otherwise this is a stable progression condition ID. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Progression")
	FName UnlockCondition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Progression", meta=(ClampMin="0"))
	int32 MinDepth = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Progression", meta=(ClampMin="0"))
	int32 MaxDepth = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Progression", meta=(ClampMin="0.0"))
	float RewardWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy", meta=(ClampMin="1"))
	int32 MaxHealth = 100;

	/** Real draw-pile deck. IntentCycle remains available only as a compatibility fallback. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Deck")
	TArray<FFantasyEnemyDeckEntry> Deck;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Deck", meta=(ClampMin="1"))
	int32 MaxHandSize = 5;

	/** Refreshed at the beginning of every enemy turn. Legacy EnergyCost also consumes this pool. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Deck", meta=(ClampMin="0"))
	int32 MaxActionPoints = 1;

	/** Mana persists for the whole battle and can be increased by card effects. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Deck", meta=(ClampMin="0"))
	int32 StartingMana = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Deck", meta=(ClampMin="1"))
	int32 CardsPerTurn = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Passive")
	FName PassiveId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Passive")
	FText PassiveName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Passive", meta=(MultiLine="true"))
	FText PassiveDescription;

	/** Generic data-driven reactions and turn-start mechanics for this encounter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Mechanics")
	TArray<FFantasyCombatMechanicRule> Mechanics;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Presentation")
	EFantasyEnemyVisualProfile VisualProfile = EFantasyEnemyVisualProfile::Warrior;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy")
	bool bBoss = false;

	/** Compatibility path for old content that has not yet been converted to Deck entries. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy")
	TArray<FFantasyEnemyIntentStep> IntentCycle;

	static const FPrimaryAssetType PrimaryAssetType;
};
