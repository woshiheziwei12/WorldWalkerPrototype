#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FantasyBattleArena.generated.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * W01-only scenery surrounding the Blackthorn encounter.
 *
 * The combat lane and camera corridor stay completely open. Imported CC0
 * scenery is resolved at runtime; subdued, non-colliding primitives are used
 * only when an optional mesh is unavailable. This actor owns presentation only.
 */
UCLASS()
class WORLDWALKERPROTOTYPE_API AFantasyBattleArena : public AActor
{
	GENERATED_BODY()

public:
	AFantasyBattleArena();

protected:
	virtual void BeginPlay() override;

private:
	void BuildCourtyard();
	void BuildKeepBackdrop();
	void BuildSideRuins();
	void BuildNaturalFrame();
	void AddBrazier(const FVector& LocalLocation, int32 Index);
	UStaticMesh* LoadEnvironmentMesh(const TCHAR* AssetName) const;
	UStaticMeshComponent* AddScenery(
		const FString& ComponentName,
		const TCHAR* ImportedAssetName,
		const FVector& LocalLocation,
		const FRotator& LocalRotation,
		const FVector& ImportedScale,
		UStaticMesh* FallbackMesh,
		const FVector& FallbackScale,
		const FLinearColor& FallbackTint);
	UStaticMeshComponent* AddPrimitive(
		const FString& ComponentName,
		UStaticMesh* Mesh,
		const FVector& LocalLocation,
		const FRotator& LocalRotation,
		const FVector& LocalScale,
		const FLinearColor& Tint);
	void ApplyTint(UStaticMeshComponent* Component, const FLinearColor& Color) const;

	UPROPERTY(VisibleAnywhere, Category="Arena")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> SceneryPieces;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> BrazierLights;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeFallback;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CylinderFallback;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> ConeFallback;

	int32 ImportedSceneryCount = 0;
	int32 FallbackSceneryCount = 0;
};
