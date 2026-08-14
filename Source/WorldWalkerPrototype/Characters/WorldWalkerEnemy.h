#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interaction/WorldWalkerInteractable.h"
#include "WorldWalkerEnemy.generated.h"

class UCombatantComponent;
class UAnimSequence;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UWidgetComponent;

UENUM(BlueprintType)
enum class EWorldWalkerEnemyAnimationCue : uint8
{
	Idle UMETA(DisplayName="Idle"),
	Attack UMETA(DisplayName="Attack"),
	Empower UMETA(DisplayName="Empower"),
	HitReact UMETA(DisplayName="Hit React"),
	Death UMETA(DisplayName="Death")
};

UCLASS()
class WORLDWALKERPROTOTYPE_API AWorldWalkerEnemy
	: public ACharacter
	, public IWorldWalkerInteractable
{
	GENERATED_BODY()

public:
	AWorldWalkerEnemy();

	UCombatantComponent* GetCombatantComponent() const { return CombatantComponent; }
	void SetInCombat(bool bInCombat);
	void ConfigureFantasyPresentation(const FText& EnemyName);

	/** Plays a W01 intent/reaction cue. Non-death cues automatically return to idle. */
	UFUNCTION(BlueprintCallable, Category="World Walker|Fantasy Animation")
	void PlayIntentAnimation(EWorldWalkerEnemyAnimationCue Cue);

	virtual bool CanInteract_Implementation(const AWorldWalkerCharacter* InteractingCharacter) const override;
	virtual void Interact_Implementation(AWorldWalkerCharacter* InteractingCharacter) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void LoadFantasyAnimationAssets();
	void PlayFantasyAnimation(UAnimSequence* Animation, bool bLooping, float PlayRate = 1.0f);
	void ReturnToFantasyIdle();

	UPROPERTY(VisibleAnywhere, Category="Visual")
	TObjectPtr<UStaticMeshComponent> PrototypeBody;

	UPROPERTY(VisibleAnywhere, Category="Visual")
	TObjectPtr<USkeletalMeshComponent> FantasyEnemyMesh;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> FantasyIdleAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> FantasyAttackAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> FantasyEmpowerAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> FantasyDeathAnimation;

	UPROPERTY(VisibleAnywhere, Category="Visual")
	TObjectPtr<UWidgetComponent> InteractionPrompt;

	UPROPERTY(VisibleAnywhere, Category="Combat")
	TObjectPtr<UCombatantComponent> CombatantComponent;

	bool bFantasyPresentationActive = false;
	bool bFantasyActionPlaying = false;
	bool bFantasyDeathPlaying = false;
	float FantasyActionEndTime = 0.0f;
};
