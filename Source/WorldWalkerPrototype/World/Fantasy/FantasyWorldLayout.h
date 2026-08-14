#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FantasyWorldLayout.generated.h"

class AExponentialHeightFog;
class APostProcessVolume;
class AFantasyNPC;
class UMaterialInterface;
class UPointLightComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * W01-only exploration layout for Ashen Kingdom.
 *
 * It composes imported CC0 meshes when they are available and always supplies
 * collision-safe engine-primitive fallbacks.  It owns presentation and NPC
 * placement only; card combat remains authoritative in the GameMode.
 */
UCLASS()
class WORLDWALKERPROTOTYPE_API AFantasyWorldLayout : public AActor
{
	GENERATED_BODY()

public:
	AFantasyWorldLayout();

	FVector GetBattleAnchorLocation() const;
	FVector GetReturnPortalLocation() const;
	FRotator GetForwardFacingRotation() const;

protected:
	virtual void BeginPlay() override;

private:
	void BuildGroundAndRoad();
	void BuildVillageDistrict();
	void BuildCampAndLandmarks();
	void BuildBattleApproach();
	void BuildReturnPortalLandmark();
	void SpawnResidents();
	void ConfigureWorldAtmosphere();
	void RefreshCapturedSky();
	void HideTemplateFloor();
	void FinalizeReturnPortalPresentation();
	void AddTorch(const FVector& LocalLocation, int32 TorchIndex);
	void AddAccentLight(
		const FString& ComponentName,
		const FVector& LocalLocation,
		const FLinearColor& Color,
		float Intensity,
		float Radius,
		bool bCastShadows);
	UStaticMeshComponent* AddEnvironmentMesh(
		const FString& ComponentName,
		const TCHAR* ImportedMeshPath,
		const FVector& LocalLocation,
		const FRotator& LocalRotation,
		const FVector& LocalScale,
		bool bEnableCollision,
		UStaticMesh* FallbackMesh,
		const FLinearColor& FallbackTint);
	void ApplyTint(UStaticMeshComponent* Component, const FLinearColor& Color) const;

	UPROPERTY(VisibleAnywhere, Category="World")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> EnvironmentMeshes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> TorchLights;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AFantasyNPC>> Residents;

	UPROPERTY(Transient)
	TObjectPtr<AExponentialHeightFog> SpawnedFog;

	UPROPERTY(Transient)
	TObjectPtr<APostProcessVolume> SpawnedPostProcess;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> GroundMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> RoadMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeFallback;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CylinderFallback;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> ConeFallback;

	static const FVector BattleAnchorLocalLocation;
	static const FVector ReturnPortalLocalLocation;
};
