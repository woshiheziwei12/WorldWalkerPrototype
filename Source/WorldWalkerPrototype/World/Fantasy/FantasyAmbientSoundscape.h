#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "FantasyAmbientSoundscape.generated.h"

class UAudioComponent;
class USceneComponent;
class USoundBase;

UENUM()
enum class EFantasyAudioCue : uint8
{
	Attack,
	Spell,
	Defense,
	Equipment,
	Counter,
	Draw,
	TurnEnd,
	Reward,
	Route
};

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

	/** Cross-fades the exploration and battle music beds. */
	void SetBattleMusicActive(bool bBattleActive);

	/** Plays the universal card placement sound plus the matching combat cue. */
	void PlayCardCue(EFantasyAudioCue Cue, bool bEnemy);

	/** Plays a non-card route, draw, turn, or reward cue. */
	void PlayInterfaceCue(EFantasyAudioCue Cue);

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

	UPROPERTY(VisibleAnywhere, Category="Music")
	TObjectPtr<UAudioComponent> ExplorationMusicComponent;

	UPROPERTY(VisibleAnywhere, Category="Music")
	TObjectPtr<UAudioComponent> BattleMusicComponent;

	UPROPERTY(VisibleAnywhere, Category="Combat Audio")
	TObjectPtr<UAudioComponent> CardSfxComponent;

	UPROPERTY(VisibleAnywhere, Category="Combat Audio")
	TObjectPtr<UAudioComponent> ActionSfxComponent;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> FireLoopSound;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundBase>> AmbientCueSounds;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> ExplorationMusic;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> BattleMusic;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> CardPlaySound;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> CardDrawSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> CardShuffleSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> TurnPassSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> RouteSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> AttackSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> SpellSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> DefenseSound;

	TArray<FVector> AmbientCueLocations;
	FTimerHandle AmbientCueTimer;
	int32 LastAmbientCueIndex = INDEX_NONE;
};
