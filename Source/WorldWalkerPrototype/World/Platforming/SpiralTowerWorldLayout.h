#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpiralTowerWorldLayout.generated.h"

class AExponentialHeightFog;
class ADirectionalLight;
class ASkyLight;
class AWorldWalkerCharacter;
class UBoxComponent;
class UAnimSequence;
class UMaterialInterface;
class USkyAtmosphereComponent;
class UPointLightComponent;
class USceneComponent;
class USphereComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;
class UWorldDefinition;

/**
 * W02-only runtime layout and traversal controller.
 *
 * The authored entry map only supplies a PlayerStart and the engine atmosphere.
 * This actor builds a collision-authoritative seven-level Babylonian ziggurat and its
 * exterior construction route, tunes the local
 * player for platforming, advances safe checkpoints, resets falls, and owns the
 * dawn progression and summit choice that lets the player remain outside or
 * deliberately return to W00. Optional medieval meshes are
 * loaded only from W02; engine primitives keep the complete route playable when
 * those assets are absent.
 */
UCLASS()
class WORLDWALKERPROTOTYPE_API ASpiralTowerWorldLayout : public AActor
{
	GENERATED_BODY()

public:
	ASpiralTowerWorldLayout();

	/** Ground-space start marker. The player capsule is placed above this point. */
	FVector GetStartLocation() const;

	/** Centre of the walkable summit surface. */
	FVector GetSummitLocation() const;

	int32 GetPlatformCount() const { return BuiltPlatformCount; }
	int32 GetCheckpointCount() const { return CheckpointLocalTransforms.Num(); }
	float GetTowerHeight() const { return SummitSurfaceLocalZ; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void LoadOptionalEnvironmentAssets();
	void ConfigureNightAtmosphere();
	void UpdateDawnAtmosphere(float DeltaSeconds);
	void RefreshCapturedSky();
	void HideTemplateFloor();
	void BuildTower();
	void GeneratePlatformSpecs();
	void BuildBaseCourtyard();
	void BuildSpiralPlatforms();
	void BuildBabylonZiggurat();
	void BuildBabylonScaffolds();
	void BuildInteriorShell();
	void BuildOverheadTimbers();
	void BuildSummit();
	void BuildJourneyMarker(int32 PlatformIndex);
	void BuildClimbRelic();
	void BuildObservationDeckDetails(int32 PlatformIndex);
	void BuildLadderToPlatform(int32 PlatformIndex);
	void UpdateSprintMovement();
	void UpdateJumpMovement(float DeltaSeconds);
	void UpdateTraversalStamina(float DeltaSeconds);
	void UpdatePlatformingHUD();
	void HandleW02JumpPressed();
	void HandleW02JumpReleased();
	void StartBufferedJump();
	void BeginLandingRecovery(float ImpactSpeed);
	void FinishLandingRecovery();
	void UpdateClimbRelicPresentation(float DeltaSeconds);
	void GrantLedgeClimbAbility();
	bool TryStartLedgeClimb(bool bIgnoreInputRequirement = false);
	bool FindLedgeClimbTarget(
		FVector& OutHangWorldLocation,
		FVector& OutTargetWorldLocation,
		FRotator& OutTargetWorldRotation) const;
	void TickLedgeClimb(float DeltaSeconds);
	void FinishLedgeClimb();
	void CancelLedgeClimbForExhaustion();
	void StartAutomatedClimbValidation();
	void AddPlatformPresentation(
		int32 PlatformIndex,
		const FVector& SurfaceLocation,
		const FRotator& PlatformRotation,
		float PlatformLength,
		float PlatformWidth,
		uint8 PlatformStyle,
		bool bCheckpointPlatform);
	void AddWallAnchor(
		int32 PlatformIndex,
		const FVector& SurfaceLocation,
		const FRotator& PlatformRotation,
		float PlatformWidth,
		uint8 PlatformStyle);
	void ConfigurePlayerForPlatforming();
	void PlacePlayerAtInitialStart();
	void StartAutomatedRouteValidation();
	void AdvanceAutomatedRouteValidation();
	void UpdateCheckpointProgress();
	void ResetPlayerToCheckpoint(const TCHAR* Reason);
	void CompleteReturnTravel();
	void EnterSummitChoice();
	void BeginReturnTravel();
	void ShowJourneyBeat(int32 BeatIndex, bool bPersistent = false);
	void HideJourneyMessage();
	void RestoreAfterFailedTravel();
	FTransform GetCheckpointWorldTransform(int32 CheckpointSlot) const;
	float GetPlayerCapsuleHalfHeight() const;

	UFUNCTION()
	void HandleSummitOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleClimbRelicOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UStaticMeshComponent* AddCollisionPrimitive(
		const FString& ComponentName,
		UStaticMesh* Mesh,
		const FVector& LocalLocation,
		const FRotator& LocalRotation,
		const FVector& LocalScale,
		const FLinearColor& Tint,
		bool bEnableCollision = true,
		UMaterialInterface* PreferredMaterial = nullptr);
	UStaticMeshComponent* AddDecorMesh(
		const FString& ComponentName,
		UStaticMesh* PreferredMesh,
		UStaticMesh* FallbackMesh,
		const FVector& LocalLocation,
		const FRotator& LocalRotation,
		const FVector& PreferredScale,
		const FVector& FallbackScale,
		const FLinearColor& FallbackTint,
		UMaterialInterface* PreferredFallbackMaterial = nullptr);
	void AddTorch(const FVector& LocalSurfaceLocation, const FVector& OutwardDirection, int32 TorchIndex);
	void ApplyPrimitiveTint(UStaticMeshComponent* Component, const FLinearColor& Color) const;

	UPROPERTY(VisibleAnywhere, Category="Spiral Tower")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category="Spiral Tower|Travel")
	TObjectPtr<UBoxComponent> SummitTrigger;

	UPROPERTY(VisibleAnywhere, Category="Spiral Tower|Ledge Climb")
	TObjectPtr<USphereComponent> ClimbRelicTrigger;

	UPROPERTY(VisibleAnywhere, Category="Spiral Tower|Ledge Climb")
	TObjectPtr<UStaticMeshComponent> ClimbRelicCore;

	UPROPERTY(VisibleAnywhere, Category="Spiral Tower|Ledge Climb")
	TObjectPtr<UStaticMeshComponent> ClimbRelicBrace;

	UPROPERTY(VisibleAnywhere, Category="Spiral Tower|Ledge Climb")
	TObjectPtr<UTextRenderComponent> ClimbRelicLabel;

	UPROPERTY(VisibleAnywhere, Category="Spiral Tower|Ledge Climb")
	TObjectPtr<UPointLightComponent> ClimbRelicLight;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> TowerMeshes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> TorchLights;

	UPROPERTY(Transient)
	TObjectPtr<AWorldWalkerCharacter> ActivePlayer;

	UPROPERTY(Transient)
	TObjectPtr<AExponentialHeightFog> SpawnedFog;

	UPROPERTY(Transient)
	TObjectPtr<ADirectionalLight> AtmosphereDirectionalLight;

	UPROPERTY(Transient)
	TObjectPtr<ASkyLight> AtmosphereSkyLight;

	UPROPERTY(Transient)
	TObjectPtr<USkyAtmosphereComponent> AtmosphereComponent;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> ExteriorSkyDome;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> SunriseSunDisc;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> SunriseDeckLight;

	UPROPERTY(Transient)
	TObjectPtr<UWorldDefinition> PendingDestinationWorld;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeFallback;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CylinderFallback;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> ConeFallback;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> SphereFallback;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> TowerMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> WallMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> ArchMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CampfireMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> PathMesh;

	/** W02-owned Kenney Castle Kit stone stair used as the stone-platform skin. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> StoneBrickMesh;

	/** W02-owned Kenney Castle Kit wall used as the interior masonry skin. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> InteriorWallMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CastleDoorwayMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CastleBridgeMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CastleTowerMidMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CastleTowerTopMesh;

	/** Reserved W02-owned Kenney Medieval Kit path: SM_W02_WoodBeam. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> WoodBeamMesh;

	/** Reserved W02-owned Kenney Medieval Kit path: SM_W02_BrokenPlank. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> BrokenPlankMesh;

	/** Poly Haven Modular Fort visual-only modules; primitives remain collision authority. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> PolyHavenWallStraightMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> PolyHavenGateMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> PolyHavenWalkwayMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> PolyHavenTowerRoundMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BasicShapeMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PolyHavenWallMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PolyHavenTrimMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PolyHavenWoodMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> JumpStartAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> JumpLoopAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> JumpLandAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> ClimbAnimation;

	TArray<FTransform> CheckpointLocalTransforms;
	TArray<FVector> PlatformSurfaceLocalLocations;
	TArray<FRotator> PlatformLocalRotations;
	TArray<FVector2D> PlatformLocalDimensions;
	TArray<uint8> PlatformStyles;
	TArray<uint8> PlatformTraversalTypes;
	TArray<float> PlatformStepFlightTimes;
	TArray<float> PlatformStepTheoreticalReaches;
	TArray<float> PlatformStepRequiredTravels;
	TArray<float> PlatformStepSafetyMargins;
	FTimerHandle ReturnTravelTimer;
	FTimerHandle AutomatedValidationTimer;
	FTimerHandle LandingRecoveryTimer;
	FTimerHandle JourneyMessageTimer;
	FVector StartSurfaceLocalLocation = FVector::ZeroVector;
	FVector SummitSurfaceLocalLocation = FVector::ZeroVector;
	FVector SunriseDeckLocalLocation = FVector::ZeroVector;
	FVector ClimbRelicBaseLocalLocation = FVector::ZeroVector;
	FVector LedgeClimbStartWorldLocation = FVector::ZeroVector;
	FVector LedgeClimbHangWorldLocation = FVector::ZeroVector;
	FVector LedgeClimbTargetWorldLocation = FVector::ZeroVector;
	FVector AutomatedClimbExpectedWorldLocation = FVector::ZeroVector;
	FRotator LedgeClimbStartWorldRotation = FRotator::ZeroRotator;
	FRotator LedgeClimbTargetWorldRotation = FRotator::ZeroRotator;
	float SummitSurfaceLocalZ = 0.0f;
	float LedgeClimbElapsed = 0.0f;
	float SavedClimbGravityScale = 1.0f;
	float NextLedgeClimbAllowedTime = 0.0f;
	float CurrentStamina = 100.0f;
	float LastStaminaUseTime = -100.0f;
	float JumpPressedTime = -100.0f;
	float BufferedJumpUntil = -100.0f;
	float LastGroundedTime = -100.0f;
	float LastFallingVerticalSpeed = 0.0f;
	float HighestDawnProgress = 0.0f;
	float DisplayedDawnProgress = 0.0f;
	float NextHUDUpdateTime = 0.0f;
	float GeneratedTurns = 0.0f;
	float GeneratedMaxClearGap = 0.0f;
	float GeneratedMinClearGap = 0.0f;
	float GeneratedMaxCenterSpan = 0.0f;
	float GeneratedMinCenterSpan = 0.0f;
	float GeneratedMaxRequiredTravel = 0.0f;
	float GeneratedMinSafetyMargin = 0.0f;
	float GeneratedMaxReachUsage = 0.0f;
	float GeneratedMaxRequiredSpeed = 0.0f;
	float GeneratedMinAngleStep = 0.0f;
	float GeneratedMaxAngleStep = 0.0f;
	float GeneratedMinRadius = 0.0f;
	float GeneratedMaxRadius = 0.0f;
	float GeneratedMinRise = 0.0f;
	float GeneratedMaxRise = 0.0f;
	float GeneratedMinLength = 0.0f;
	float GeneratedMaxLength = 0.0f;
	float GeneratedMinWidth = 0.0f;
	float GeneratedMaxWidth = 0.0f;
	int32 GeneratedWorstStep = INDEX_NONE;
	int32 AutomatedValidationPlatformIndex = INDEX_NONE;
	int32 AutomatedValidationFallCheckpointSlot = INDEX_NONE;
	int32 AutomatedValidationFallPassCount = 0;
	float KillPlaneLocalZ = -600.0f;
	int32 ActiveCheckpointSlot = 0;
	int32 LastDawnLogBand = INDEX_NONE;
	int32 BuiltPlatformCount = 0;
	int32 ImportedDecorCount = 0;
	int32 KenneyDecorCount = 0;
	int32 PolyHavenDecorCount = 0;
	int32 LoadedKenneyAssetKindCount = 0;
	int32 LoadedPolyHavenAssetKindCount = 0;
	int32 LoadedPolyHavenMaterialCount = 0;
	int32 FallbackDecorCount = 0;
	bool bResettingPlayer = false;
	bool bTravelRequested = false;
	bool bSummitChoiceAvailable = false;
	bool bReturnKeyWasDown = false;
	bool bSprintActive = false;
	bool bJumpHeld = false;
	bool bJumpInProgress = false;
	bool bJumpStartedAsSprint = false;
	bool bJumpLoopAnimationPlaying = false;
	bool bWasFalling = false;
	bool bLandingRecovery = false;
	bool bLedgeClimbUnlocked = false;
	bool bLedgeClimbInProgress = false;
	bool bAutomatedClimbValidation = false;
	ECollisionEnabled::Type SavedPlayerCollisionMode = ECollisionEnabled::QueryAndPhysics;
	bool bAutomatedRouteValidation = false;
	bool bAutomatedValidationFallProbeActive = false;
};
