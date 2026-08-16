#pragma once

#include "CoreMinimal.h"
#include "Cards/Fantasy/FantasyCombatTypes.h"
#include "Engine/DataAsset.h"
#include "CardDefinition.generated.h"

class UTexture2D;

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
