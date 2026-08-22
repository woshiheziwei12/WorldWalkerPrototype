#pragma once

#include "Cards/Fantasy/FantasyCombatTypes.h"
#include "Cards/Fantasy/FantasyRunTypes.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FantasyBlessingDefinition.generated.h"

UENUM(BlueprintType)
enum class EFantasyBlessingTrigger : uint8
{
	BattleStarted,
	PlayerTurnStarted,
	EnemyTurnStarted,
	PlayerCardResolved,
	PlayerDamaged,
	RewardSkipped
};

/** Data-driven run blessing dispatched without branching on BlessingId. */
UCLASS(BlueprintType)
class WORLDWALKERPROTOTYPE_API UFantasyBlessingDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	TArray<FString> ValidateDefinition() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blessing")
	FName BlessingId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blessing")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blessing", meta=(MultiLine="true"))
	FText Description;

	/** None is shared; otherwise this blessing is restricted to one profession. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blessing")
	EFantasyPlayerProfession Profession = EFantasyPlayerProfession::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blessing")
	EFantasyBlessingTrigger Trigger = EFantasyBlessingTrigger::BattleStarted;

	/** Optional card tag gate for PlayerCardResolved. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blessing")
	FName RequiredCardTag;

	/** Zero means unlimited; positive values cap activations per battle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blessing", meta=(ClampMin="0"))
	int32 MaxTriggersPerBattle = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blessing", meta=(ClampMin="0"))
	int32 ShopPrice = 90;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blessing")
	TArray<FFantasyCombatEffectSpec> Effects;

	static const FPrimaryAssetType PrimaryAssetType;
};
