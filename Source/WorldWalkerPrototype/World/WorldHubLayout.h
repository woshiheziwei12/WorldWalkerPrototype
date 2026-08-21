#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldHubLayout.generated.h"

class AWorldWalkerCharacter;
class UBoxComponent;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UNiagaraComponent;
class UPointLightComponent;
class UPostProcessComponent;
class UPrimitiveComponent;
class USceneComponent;
class USkyLightComponent;
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

	FVector GetActivePortalLocation() const;
	FVector GetSpiralTowerPortalLocation() const;

protected:
	virtual void BeginPlay() override;

private:
	void ConfigureMainWorldEnvironment();
	void ConfigureMainWorldAvatar();
	void RefreshNightSky();
	void HideLegacyPortal();
	void UpdateMainWorldMovement();
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
	TObjectPtr<UStaticMeshComponent> PortalArch;

	UPROPERTY(VisibleAnywhere, Category="Main World|Portal")
	TObjectPtr<UStaticMeshComponent> PortalCanopy;

	UPROPERTY(VisibleAnywhere, Category="Main World|Portal")
	TObjectPtr<UStaticMeshComponent> PortalDisk;

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

	UPROPERTY(Transient)
	TObjectPtr<UWorldDefinition> PendingDestinationWorld;

	FTimerHandle PortalTravelTimer;
	float PortalPulseTime = 0.0f;
	float DefaultWalkSpeed = 430.0f;
	bool bTravelRequested = false;
	bool bSprintActive = false;

	/** Hub itself spawns 850 cm ahead; -330 places the portal 520 cm ahead of the player. */
	static const FVector ActivePortalLocalLocation;

	/** Keeps the W02 interaction authority 780 cm away from the automatic W01 trigger. */
	static const FVector SpiralTowerPortalLocalLocation;
};
