#pragma once

#include "CoreMinimal.h"
#include "Cards/Fantasy/FantasyCombatTypes.h"
#include "Cards/Fantasy/FantasyRunTypes.h"
#include "Engine/DataAsset.h"
#include "CardDefinition.generated.h"

class UTexture2D;

/** Progression rarity is content data; combat rules must not infer it from costs. */
UENUM(BlueprintType)
enum class EFantasyCardRarity : uint8
{
	Starter,
	Common,
	Uncommon,
	Rare,
	Enemy
};

/** Data-only description of a playable combat card. */
UCLASS(BlueprintType)
class WORLDWALKERPROTOTYPE_API UCardDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	FString BuildRulesText() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	FName CardId;

	/** Stable content-set identifier. W01 starter cards use W01_EasternHorror. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	FName CardSetId;

	/** Profession-specific player pool. None is reserved for enemies/shared cards. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Progression")
	EFantasyPlayerProfession Profession = EFantasyPlayerProfession::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Progression")
	EFantasyCardRarity Rarity = EFantasyCardRarity::Common;

	/** Stable semantic tags such as Profession.Mage, Archetype.Fire, and Effect.Draw. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Progression")
	TArray<FName> BuildTags;

	/** Optional single upgrade target. A configured target must be exactly one level higher. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Progression")
	TSoftObjectPtr<UCardDefinition> UpgradeCard;

	/** Zero is the base card. M1 permits only levels zero and one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Progression", meta=(ClampMin="0", ClampMax="1"))
	int32 UpgradeLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card", meta=(ClampMin="0"))
	int32 EnergyCost = 1;

	/** Classic-mode action cards consume this resource; it refreshes each turn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Classic", meta=(ClampMin="0"))
	int32 ActionCost = 0;

	/** Spell cards consume persistent combat Mana. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Classic", meta=(ClampMin="0"))
	int32 ManaCost = 0;

	/** Uses Action/Mana instead of the legacy three-Energy prototype resource. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Classic")
	bool bUseClassicResources = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card", meta=(ClampMin="0"))
	int32 ValorCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	ECardType CardType = ECardType::Attack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	ECardSchool School = ECardSchool::None;

	/** Effects are resolved in array order by the authoritative combat flow. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	TArray<FFantasyCombatEffectSpec> Effects;

	/** Retained cards remain in hand when the player ends the turn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	bool bRetain = false;

	/** Exhausted cards leave the battle after being played instead of entering discard. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	bool bExhaust = false;

	/** Optional world-specific card illustration. UI must provide a fallback when unset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	TSoftObjectPtr<UTexture2D> Artwork;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card", meta=(ClampMin="0"))
	int32 StartingDeckCopies = 1;

	/** Reward cards never enter the starter deck until the player claims them after a W01 victory. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Progression")
	bool bRewardEligible = false;

	/** Equipment remains in a persistent zone and contributes these passive values. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Classic", meta=(ClampMin="0"))
	int32 EquipmentAttackBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Classic", meta=(ClampMin="0"))
	int32 EquipmentTurnStartBlock = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Classic", meta=(ClampMin="0"))
	int32 EquipmentTurnStartDraw = 0;

	static const FPrimaryAssetType PrimaryAssetType;
};
