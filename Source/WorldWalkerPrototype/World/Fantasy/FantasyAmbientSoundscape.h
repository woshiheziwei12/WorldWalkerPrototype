#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "FantasyAmbientSoundscape.generated.h"

class UAudioComponent;
class USceneComponent;
class USoundBase;

/**
 * W01-only environmental audio bed for the Ashen Kingdom exploration route.
 *
 * The actor owns four quiet, spatial fire loops and one reusable component for
 * occasional dark ambience cues at the edge of the route. All audio assets are
 * optional: a missing import disables only the corresponding layer.
 */
UCLASS()
class WORLDWALKERPROTOTYPE_API AFantasyAmbientSoundscape : public AActor
{
	GENERATED_BODY()

public:
	AFantasyAmbientSoundscape();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ConfigureSpatialAudio(
		UAudioComponent* AudioComponent,
		float InnerRadius,
		float FalloffDistance) const;
	USoundBase* LoadOptionalSound(const FString& ObjectPath, const TCHAR* LayerName) const;
	void ScheduleNextAmbientCue(float DelaySeconds);
	void PlayAmbientCue();

	UPROPERTY(VisibleAnywhere, Category="Ambient Audio")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category="Ambient Audio")
	TArray<TObjectPtr<UAudioComponent>> FireLoopComponents;

	UPROPERTY(VisibleAnywhere, Category="Ambient Audio")
	TObjectPtr<UAudioComponent> AmbientCueComponent;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> FireLoopSound;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundBase>> AmbientCueSounds;

	TArray<FVector> AmbientCueLocations;
	FTimerHandle AmbientCueTimer;
	int32 LastAmbientCueIndex = INDEX_NONE;
};
