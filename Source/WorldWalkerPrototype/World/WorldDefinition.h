#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WorldDefinition.generated.h"

class UWorld;

UCLASS(BlueprintType)
class WORLDWALKERPROTOTYPE_API UWorldDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AssetRegistrySearchable, Category="World")
	FName WorldId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World")
	TSoftObjectPtr<UWorld> EntryMap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World")
	bool bIsMainWorld = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	FLinearColor PortalColor = FLinearColor(0.05f, 0.8f, 1.0f);
};
