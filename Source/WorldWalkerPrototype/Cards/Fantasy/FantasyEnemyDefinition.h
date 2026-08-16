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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Presentation")
	EFantasyEnemyVisualProfile VisualProfile = EFantasyEnemyVisualProfile::Warrior;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy")
	bool bBoss = false;

	/** Compatibility path for old content that has not yet been converted to Deck entries. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy")
	TArray<FFantasyEnemyIntentStep> IntentCycle;

	static const FPrimaryAssetType PrimaryAssetType;
};
