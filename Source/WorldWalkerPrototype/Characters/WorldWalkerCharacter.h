#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WorldWalkerCharacter.generated.h"

class UCameraComponent;
class UCardCombatComponent;
class UCombatantComponent;
class UAnimSequence;
class USkeletalMeshComponent;
class USpringArmComponent;
class UStaticMeshComponent;

enum class EWorldWalkerFantasyAnimationState : uint8
{
	None,
	Idle,
	Walk,
	Run,
	Attack,
	HitReact
};

UCLASS()
class WORLDWALKERPROTOTYPE_API AWorldWalkerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AWorldWalkerCharacter();

	UCombatantComponent* GetCombatantComponent() const { return CombatantComponent; }
	UCardCombatComponent* GetCardCombatComponent() const { return CardCombatComponent; }
	void SetCombatLocked(bool bLocked);
	void ConfigureFantasyWorldForm(bool bEnabled, bool bStartInFantasyForm = true);
	bool IsFantasyFormActive() const { return bFantasyFormActive; }

	/** Plays the W01 form's card action, then safely returns to locomotion/idle. */
	UFUNCTION(BlueprintCallable, Category="World Walker|Fantasy Animation")
	void PlayFantasyCardAttackAnimation();

	/** Plays the W01 form's hit reaction, then safely returns to locomotion/idle. */
	UFUNCTION(BlueprintCallable, Category="World Walker|Fantasy Animation")
	void PlayFantasyHitReactionAnimation();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void TryInteract();
	void ToggleWorldForm();
	void ApplyWorldFormVisibility();
	bool LoadFantasyPresentationAssets();
	void RefreshFantasyLocomotionAnimation(bool bForce = false);
	void PlayFantasyActionAnimation(
		UAnimSequence* Animation,
		EWorldWalkerFantasyAnimationState ActionState,
		float PlayRate = 1.0f);

	UPROPERTY(VisibleAnywhere, Category="Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category="Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, Category="Visual")
	TObjectPtr<UStaticMeshComponent> PrototypeBody;

	UPROPERTY(VisibleAnywhere, Category="Visual")
	TObjectPtr<USkeletalMeshComponent> FantasyFormMesh;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> FantasyIdleAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> FantasyWalkAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> FantasyRunAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> FantasyAttackAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> FantasyHitReactionAnimation;

	UPROPERTY(VisibleAnywhere, Category="Combat")
	TObjectPtr<UCombatantComponent> CombatantComponent;

	UPROPERTY(VisibleAnywhere, Category="Cards")
	TObjectPtr<UCardCombatComponent> CardCombatComponent;

	bool bCombatLocked = false;
	bool bFantasyFormAvailable = false;
	bool bFantasyFormActive = false;
	bool bFantasyActionPlaying = false;
	float FantasyActionEndTime = 0.0f;
	EWorldWalkerFantasyAnimationState CurrentFantasyAnimationState =
		EWorldWalkerFantasyAnimationState::None;
	float InteractionDistance = 350.0f;
};
