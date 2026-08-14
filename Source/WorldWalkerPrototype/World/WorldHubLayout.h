#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldHubLayout.generated.h"

class ACharacter;
class UBoxComponent;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UNiagaraComponent;
class UPointLightComponent;
class UPostProcessComponent;
class UPrimitiveComponent;
class USceneComponent;
class USkeletalMeshComponent;
class USkyLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UWorldDefinition;

/**
 * W00-only presentation controller.
 *
 * The authored Asian Village level supplies the playable landscape and distant
 * scenery. This actor adds the night treatment, the modular anime avatar, and
 * the automatic portal shrine without changing shared character/game-mode code.
 */
UCLASS()
class WORLDWALKERPROTOTYPE_API AWorldHubLayout : public AActor
{
	GENERATED_BODY()

public:
	AWorldHubLayout();

	virtual void Tick(float DeltaSeconds) override;

	FVector GetActivePortalLocation() const;

protected:
	virtual void BeginPlay() override;

private:
	void ConfigureMainWorldEnvironment();
	void ConfigureMainWorldAvatar();
	USkeletalMeshComponent* CreateLinkedAvatarPart(
		ACharacter* PlayerCharacter,
		FName ComponentName,
		const TCHAR* MeshPath,
		USkeletalMeshComponent* PoseLeader);
	USkeletalMeshComponent* CreateHairPart(
		ACharacter* PlayerCharacter,
		USkeletalMeshComponent* HeadComponent);
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

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> MainWorldCharacter;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USkeletalMeshComponent>> MainWorldAvatarParts;

	UPROPERTY(Transient)
	TObjectPtr<UWorldDefinition> PendingDestinationWorld;

	FTimerHandle PortalTravelTimer;
	float PortalPulseTime = 0.0f;
	float DefaultWalkSpeed = 430.0f;
	bool bTravelRequested = false;
	bool bSprintActive = false;

	/** Hub itself spawns 850 cm ahead; -330 places the portal 520 cm ahead of the player. */
	static const FVector ActivePortalLocalLocation;
};
