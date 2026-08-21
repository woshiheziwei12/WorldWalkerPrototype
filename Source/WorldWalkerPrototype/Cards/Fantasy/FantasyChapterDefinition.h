#pragma once

#include "Cards/Fantasy/FantasyEnemyDefinition.h"
#include "Cards/Fantasy/FantasyRunTypes.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FantasyChapterDefinition.generated.h"

/** One authored non-combat candidate in a chapter depth rule. */
USTRUCT(BlueprintType)
struct WORLDWALKERPROTOTYPE_API FFantasyNonCombatRouteDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Route")
	FName NodeId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Route")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Route", meta=(MultiLine="true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Route")
	EFantasyRouteNodeType NodeType = EFantasyRouteNodeType::Event;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Route")
	FName PayloadId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Route", meta=(ClampMin="0.0"))
	float Weight = 1.0f;
};

/** Constraints and authored non-combat pool for one chapter depth. */
USTRUCT(BlueprintType)
struct WORLDWALKERPROTOTYPE_API FFantasyRouteDepthDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Route", meta=(ClampMin="0"))
	int32 Depth = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Route", meta=(ClampMin="1"))
	int32 ChoiceCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Route", meta=(ClampMin="0"))
	int32 CombatChoiceCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Route")
	EFantasyEncounterTier EncounterTier = EFantasyEncounterTier::Normal;

	/** Prefer different enemy families within this offer; relax only if the pool cannot fill. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Route")
	bool bRequireDistinctEnemyFamilies = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Route")
	TArray<FFantasyNonCombatRouteDefinition> NonCombatCandidates;
};

/**
 * Data-only chapter route contract. Enemy candidates come from authored enemy assets;
 * their depth, tier, family, unlock and reward-weight metadata drive generation.
 */
UCLASS(BlueprintType)
class WORLDWALKERPROTOTYPE_API UFantasyChapterDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** Returns all structural/content errors. Empty means the definition is safe to generate. */
	TArray<FString> ValidateDefinition() const;

	/** Deterministically generates one offer. LastEncounterId is avoided when the pool permits. */
	bool GenerateRouteChoices(
		int32 Depth,
		int32 RouteSeed,
		const FString& ContentVersion,
		EFantasyRunDifficulty Difficulty,
		FName LastEncounterId,
		TArray<FFantasyRouteNodeChoice>& OutChoices,
		FString& OutFailureReason) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chapter")
	FName ChapterId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chapter", meta=(ClampMin="1"))
	int32 ChapterNumber = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chapter", meta=(ClampMin="1"))
	int32 TotalDepths = 6;

	/** Encounter unlock IDs must be empty or match this chapter gate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chapter")
	FName UnlockCondition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chapter")
	TArray<TSoftObjectPtr<UFantasyEnemyDefinition>> EncounterPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chapter")
	TArray<FFantasyRouteDepthDefinition> DepthDefinitions;

	static const FPrimaryAssetType PrimaryAssetType;
};
