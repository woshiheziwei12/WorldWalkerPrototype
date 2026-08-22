#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FantasyCampaignDefinition.generated.h"

class UFantasyChapterDefinition;

USTRUCT(BlueprintType)
struct WORLDWALKERPROTOTYPE_API FFantasyCampaignChapterSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Campaign")
	FName ChapterId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Campaign", meta=(ClampMin="1"))
	int32 ChapterNumber = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Campaign")
	bool bOpen = false;

	/** Closed future chapters intentionally allow an empty definition reference. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Campaign")
	TSoftObjectPtr<UFantasyChapterDefinition> Definition;
};

/** Three-chapter campaign shell. Closed slots can be authored before their content exists. */
UCLASS(BlueprintType)
class WORLDWALKERPROTOTYPE_API UFantasyCampaignDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	TArray<FString> ValidateDefinition() const;
	const FFantasyCampaignChapterSlot* FindChapter(int32 ChapterNumber) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Campaign")
	FName CampaignId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Campaign", meta=(ClampMin="1"))
	int32 PlannedChapterCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Campaign", meta=(ClampMin="1"))
	int32 OpenChapterCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Campaign")
	TArray<FFantasyCampaignChapterSlot> Chapters;

	static const FPrimaryAssetType PrimaryAssetType;
};
