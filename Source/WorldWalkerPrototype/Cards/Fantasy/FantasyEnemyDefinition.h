#pragma once

#include "CoreMinimal.h"
#include "Cards/Fantasy/FantasyCombatTypes.h"
#include "Engine/DataAsset.h"
#include "FantasyEnemyDefinition.generated.h"

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy")
	TArray<FFantasyEnemyIntentStep> IntentCycle;

	static const FPrimaryAssetType PrimaryAssetType;
};
