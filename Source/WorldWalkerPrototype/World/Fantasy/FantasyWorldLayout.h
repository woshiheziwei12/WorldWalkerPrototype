#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FantasyWorldLayout.generated.h"

class AExponentialHeightFog;
class AFantasyAmbientSoundscape;
class AFantasyAmbientWispField;
class AFantasyFateAltar;
class APostProcessVolume;
class AFantasyNPC;
class AWorldWalkerCharacter;
class UBoxComponent;
class UMaterialInterface;
class UInstancedStaticMeshComponent;
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
	FVector ConstrainChoiceCenterToPlayableArea(const FVector& DesiredWorldCenter) const;
	AFantasyAmbientSoundscape* GetAmbientSoundscape() const { return AmbientSoundscape; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void BuildGroundAndRoad();
	void BuildWorldBoundaries();
	void BuildVillageDistrict();
	void BuildCampAndLandmarks();
	void BuildBattleApproach();
	void BuildReturnPortalLandmark();
	void SpawnResidents();
	void SpawnWorldAmbienceAndEvent();
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
	void AddBoundaryWall(
		const FString& ComponentName,
		const FVector& LocalLocation,
		const FVector& BoxExtent);
	void CapturePlayerRecoveryPoint();
	void RecoverFallenPlayer();

	UPROPERTY(VisibleAnywhere, Category="World")
	TObjectPtr<USceneComponent> SceneRoot;

	/** W01-only star field; deliberately independent from the W00 sky setup. */
	UPROPERTY(VisibleAnywhere, Category="World|Atmosphere")
	TObjectPtr<UStaticMeshComponent> NightSkySphere;

	/** Stylised moon anchor so the route keeps a readable night focal point. */
	UPROPERTY(VisibleAnywhere, Category="World|Atmosphere")
	TObjectPtr<UStaticMeshComponent> NightMoon;

	/** Sparse deterministic stars layered in front of the HDRI. */
	UPROPERTY(VisibleAnywhere, Category="World|Atmosphere")
	TObjectPtr<UInstancedStaticMeshComponent> NightStars;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> EnvironmentMeshes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBoxComponent>> BoundaryWalls;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> TorchLights;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AFantasyNPC>> Residents;

	UPROPERTY(Transient)
	TObjectPtr<AFantasyAmbientWispField> AmbientWispField;

	UPROPERTY(Transient)
	TObjectPtr<AFantasyAmbientSoundscape> AmbientSoundscape;

	UPROPERTY(Transient)
	TObjectPtr<AFantasyFateAltar> FateAltar;

	UPROPERTY(Transient)
	TObjectPtr<AWorldWalkerCharacter> ProtectedPlayer;

	UPROPERTY(Transient)
	FTransform PlayerRecoveryTransform;

	UPROPERTY(Transient)
	FRotator PlayerRecoveryControlRotation = FRotator::ZeroRotator;

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

	bool bHasPlayerRecoveryPoint = false;

	static const FVector BattleAnchorLocalLocation;
	static const FVector ReturnPortalLocalLocation;
};
