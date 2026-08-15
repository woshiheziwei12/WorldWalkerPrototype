#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FantasyAmbientWispField.generated.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * Lightweight W01-only ambient motion.
 *
 * The field places a handful of cold magical wisps beside the exploration
 * route.  It is presentation-only: it owns no collision, interaction or
 * combat state and can safely be omitted if the world layout cannot spawn it.
 */
UCLASS()
class WORLDWALKERPROTOTYPE_API AFantasyAmbientWispField : public AActor
{
	GENERATED_BODY()

public:
	AFantasyAmbientWispField();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void BuildWisp(int32 WispIndex, const FVector& LocalLocation, const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere, Category="Ambient")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> WispMeshes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> WispLights;

	TArray<FVector> BaseLocations;
	TArray<FLinearColor> WispColors;
	float AmbientTime = 0.0f;
};
