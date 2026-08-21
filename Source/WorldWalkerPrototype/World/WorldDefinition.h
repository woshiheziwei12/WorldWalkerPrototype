#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WorldDefinition.generated.h"

class UWorld;
class AActor;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World", meta=(AssetBundles="World"))
	TSoftObjectPtr<UWorld> EntryMap;

	/**
	 * Optional world-owned composition/runtime entry point. The shared game mode
	 * spawns this class without knowing the world's ID or implementation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World", meta=(AssetBundles="World"))
	TSoftClassPtr<AActor> WorldRootActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World")
	bool bIsMainWorld = false;

	/** If true, the main-world hub creates a destination for this registration. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World")
	bool bExposeInMainWorld = true;

	/** Stable ordering for destinations; ties are resolved by WorldId. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="World")
	int32 PortalOrder = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	FLinearColor PortalColor = FLinearColor(0.05f, 0.8f, 1.0f);
};
