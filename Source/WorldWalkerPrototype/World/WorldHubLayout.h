#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldHubLayout.generated.h"

class AWorldWalkerCharacter;
class ACharacter;
class APlayerController;
class APostProcessVolume;
class UAnimInstance;
class UBoxComponent;
class UCameraComponent;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UNiagaraComponent;
class UParticleSystemComponent;
class UPointLightComponent;
class UPostProcessComponent;
class UPrimitiveComponent;
class USceneComponent;
class USkeletalMeshComponent;
class USkeletalMesh;
class USkyLightComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UWorldDefinition;

/**
 * W00-only presentation controller.
 *
 * The authored Asian Village level supplies the playable landscape and distant
 * scenery. This actor adds the night treatment and portal shrines, then asks the
 * shared player character to apply the same modular anime form used by W02.
 */
UCLASS()
class WORLDWALKERPROTOTYPE_API AWorldHubLayout : public AActor
{
	GENERATED_BODY()

public:
	AWorldHubLayout();

	virtual void Tick(float DeltaSeconds) override;

	/** Configures the featured vortex from registry data; the hub never names a child world. */
	void ConfigureDestination(UWorldDefinition* InDestinationWorld);

	FVector GetActivePortalLocation() const;
	FVector GetSpiralTowerPortalLocation() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ConfigureMainWorldEnvironment();
	void RestoreMainWorldEnvironment();
	void ConfigureMainWorldAvatar();
	void ConfigureMainWorldMovementAndCamera();
	void RestoreMainWorldAvatar();
	void RestoreOriginalCharacterMesh(USkeletalMeshComponent* HeadComponent);
	void ReleasePortalTransition(bool bRestoreMovementAndFade);
	USkeletalMeshComponent* CreateLinkedAvatarPart(
		ACharacter* PlayerCharacter,
		FName ComponentName,
		USkeletalMesh* Mesh,
		USkeletalMeshComponent* PoseLeader);
	USkeletalMeshComponent* CreateHairPart(
		ACharacter* PlayerCharacter,
		USkeletalMeshComponent* HeadComponent,
		USkeletalMesh* HairMesh);
	void SnapPortalToGround();
	void RefreshNightSky();
	void HideLegacyPortal();
	void UpdateMainWorldMovement(float DeltaSeconds);
	void UpdatePortalPresentation(float DeltaSeconds);
	void UpdatePortalTransition(float DeltaSeconds);
	void CompletePortalTravel();

	UFUNCTION()
	void HandlePortalOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category="Main World")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category="Main World|Night")
	TObjectPtr<UStaticMeshComponent> NightSkySphere;

	/** Stylised moon anchor layered in front of the HDRI. */
	UPROPERTY(VisibleAnywhere, Category="Main World|Night")
	TObjectPtr<UStaticMeshComponent> NightMoon;

	/** Sparse deterministic star points; the HDRI remains the lighting source. */
	UPROPERTY(VisibleAnywhere, Category="Main World|Night")
	TObjectPtr<UInstancedStaticMeshComponent> NightStars;

	UPROPERTY(VisibleAnywhere, Category="Main World|Night")
	TObjectPtr<UDirectionalLightComponent> MoonLight;

	UPROPERTY(VisibleAnywhere, Category="Main World|Night")
	TObjectPtr<USkyLightComponent> NightSkyLight;

	UPROPERTY(VisibleAnywhere, Category="Main World|Night")
	TObjectPtr<UExponentialHeightFogComponent> NightFog;

	UPROPERTY(VisibleAnywhere, Category="Main World|Night")
	TObjectPtr<UPostProcessComponent> NightPostProcess;

	UPROPERTY(VisibleAnywhere, Category="Main World|Portal")
	TObjectPtr<UStaticMeshComponent> PortalFoundation;

	UPROPERTY(VisibleAnywhere, Category="Main World|Portal")
	TObjectPtr<UStaticMeshComponent> PortalPillarLeft;

	UPROPERTY(VisibleAnywhere, Category="Main World|Portal")
	TObjectPtr<UStaticMeshComponent> PortalPillarRight;

	UPROPERTY(VisibleAnywhere, Category="Main World|Portal")
	TObjectPtr<UStaticMeshComponent> PortalCanopy;

	UPROPERTY(VisibleAnywhere, Category="Main World|Portal")
	TObjectPtr<UStaticMeshComponent> PortalBackdrop;

	UPROPERTY(VisibleAnywhere, Category="Main World|Portal")
	TObjectPtr<UStaticMeshComponent> PortalDisk;

	UPROPERTY(VisibleAnywhere, Category="Main World|Portal")
	TObjectPtr<UStaticMeshComponent> PortalOuterRing;

	UPROPERTY(VisibleAnywhere, Category="Main World|Portal")
	TObjectPtr<UNiagaraComponent> PortalVortex;

	UPROPERTY(VisibleAnywhere, Category="Main World|Portal")
	TObjectPtr<UNiagaraComponent> PortalLightning;

	UPROPERTY(VisibleAnywhere, Category="Main World|Portal")
	TObjectPtr<UBoxComponent> PortalTrigger;

	UPROPERTY(VisibleAnywhere, Category="Main World|Portal")
	TObjectPtr<UPointLightComponent> PortalLight;

	UPROPERTY(VisibleAnywhere, Category="Main World|Portal")
	TObjectPtr<UTextRenderComponent> PortalInstruction;

	UPROPERTY(VisibleAnywhere, Category="Main World|Portal")
	TArray<TObjectPtr<UStaticMeshComponent>> PortalShrineScenery;

	UPROPERTY(VisibleAnywhere, Category="Main World|Portal")
	TArray<TObjectPtr<UPointLightComponent>> ShrineLights;

	/** Explicit simple collision keeps both authored arch openings traversable. */
	UPROPERTY(VisibleAnywhere, Category="Main World|Portal Collision")
	TArray<TObjectPtr<UBoxComponent>> PortalCollisionShapes;

	UPROPERTY(VisibleAnywhere, Category="Main World|Spiral Tower Portal")
	TObjectPtr<UStaticMeshComponent> SpiralTowerPortalFoundation;

	UPROPERTY(VisibleAnywhere, Category="Main World|Spiral Tower Portal")
	TObjectPtr<UStaticMeshComponent> SpiralTowerPortalArch;

	UPROPERTY(VisibleAnywhere, Category="Main World|Spiral Tower Portal")
	TObjectPtr<UStaticMeshComponent> SpiralTowerPortalCanopy;

	UPROPERTY(VisibleAnywhere, Category="Main World|Spiral Tower Portal")
	TObjectPtr<UStaticMeshComponent> SpiralTowerPortalDisk;

	UPROPERTY(VisibleAnywhere, Category="Main World|Spiral Tower Portal")
	TObjectPtr<UNiagaraComponent> SpiralTowerPortalVortex;

	UPROPERTY(VisibleAnywhere, Category="Main World|Spiral Tower Portal")
	TObjectPtr<UPointLightComponent> SpiralTowerPortalLight;

	UPROPERTY(VisibleAnywhere, Category="Main World|Spiral Tower Portal")
	TObjectPtr<UTextRenderComponent> SpiralTowerPortalInstruction;

	UPROPERTY(VisibleAnywhere, Category="Main World|Spiral Tower Portal")
	TArray<TObjectPtr<UStaticMeshComponent>> SpiralTowerPortalScenery;

	UPROPERTY(VisibleAnywhere, Category="Main World|Spiral Tower Portal")
	TArray<TObjectPtr<UPointLightComponent>> SpiralTowerShrineLights;

	UPROPERTY(Transient)
	TObjectPtr<AWorldWalkerCharacter> MainWorldCharacter;

	UPROPERTY(VisibleAnywhere, Category="Main World|Atmosphere")
	TArray<TObjectPtr<UParticleSystemComponent>> AmbientFireflies;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> MainWorldCamera;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> MainWorldCameraBoom;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> MainWorldPrototypeBody;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USkeletalMeshComponent>> MainWorldAvatarParts;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMesh> OriginalCharacterMesh;

	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> OriginalCharacterAnimClass;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PortalBackdropMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UWorldDefinition> DestinationWorld;

	UPROPERTY(Transient)
	TObjectPtr<UWorldDefinition> PendingDestinationWorld;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> PortalTransitionController;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDirectionalLightComponent>> EnvironmentDirectionalLights;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USkyLightComponent>> EnvironmentSkyLights;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UExponentialHeightFogComponent>> EnvironmentFogComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<APostProcessVolume>> EnvironmentPostProcessVolumes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> HiddenEnvironmentActors;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> HiddenEnvironmentMeshes;

	TArray<bool> OriginalDirectionalLightVisibility;
	TArray<bool> OriginalSkyLightVisibility;
	TArray<bool> OriginalFogVisibility;
	TArray<bool> OriginalPostProcessEnabled;
	TArray<float> OriginalPostProcessBlendWeights;
	TArray<bool> OriginalEnvironmentActorHidden;
	TArray<bool> OriginalEnvironmentMeshVisibility;
	TArray<bool> OriginalEnvironmentMeshHiddenInGame;

	FTimerHandle PortalTravelTimer;
	float PortalPulseTime = 0.0f;
	float DefaultWalkSpeed = 430.0f;
	float DefaultMaxAcceleration = 1250.0f;
	float DefaultBrakingDeceleration = 1400.0f;
	float DefaultGroundFriction = 7.0f;
	float DefaultJumpZVelocity = 500.0f;
	float DefaultAirControl = 0.35f;
	float DefaultCameraFOV = 90.0f;
	float DefaultCameraArmLength = 450.0f;
	float DefaultCameraLagSpeed = 10.0f;
	float DefaultCameraLagMaxDistance = 0.0f;
	float DefaultCameraRotationLagSpeed = 10.0f;
	float SprintBlend = 0.0f;
	float LandingResponse = 0.0f;
	float PortalTransitionElapsed = 0.0f;
	FTransform OriginalCharacterMeshTransform;
	FVector PortalEntryStartLocation = FVector::ZeroVector;
	FVector PortalEntryVelocity = FVector::ZeroVector;
	FRotator PortalEntryStartRotation = FRotator::ZeroRotator;
	bool bOriginalCameraLagEnabled = false;
	bool bOriginalCameraRotationLagEnabled = false;
	bool bOriginalCharacterMeshVisible = true;
	bool bOriginalCharacterMeshHiddenInGame = false;
	bool bOriginalPrototypeBodyVisible = true;
	bool bOriginalPrototypeBodyHiddenInGame = false;
	uint8 OriginalCharacterAnimationMode = 0;
	uint8 OriginalVisibilityBasedAnimTickOption = 0;
	uint8 OriginalCharacterCollisionEnabled = 0;
	uint8 PortalEntryMovementMode = 0;
	uint8 PortalEntryCustomMovementMode = 0;
	bool bMovementSnapshotValid = false;
	bool bCameraSnapshotValid = false;
	bool bHeadSnapshotValid = false;
	bool bEnvironmentConfigured = false;
	bool bAvatarConfigured = false;
	bool bWasFalling = false;
	bool bTravelRequested = false;
	bool bPortalProximityLatched = false;
	bool bSprintActive = false;
	bool bPortalLookInputIgnored = false;

	/** Local shrine anchor; BeginPlay searches for a safe 12-18 m placement before using this fallback. */
	static const FVector ActivePortalLocalLocation;

	/** Keeps the W02 interaction authority 780 cm away from the automatic W01 trigger. */
	static const FVector SpiralTowerPortalLocalLocation;
	static constexpr float PortalTransitionDuration = 0.72f;
};
