#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WorldWalkerCharacter.generated.h"

class UCameraComponent;
class UCardCombatComponent;
class UCombatantComponent;
class UAnimSequence;
class UAnimInstance;
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
	/** Applies the modular anime woman used by W00 without spawning the W00 hub. */
	bool ConfigureMainWorldAnimeForm();
	/** Reuses W00's anime woman in W02 with subdued night-scene rim lighting. */
	bool ConfigureSpiralTowerAnimeForm();

	/** Applies the W02 procedural hang/pull-up pose to the active visual form. */
	void SetMainWorldLedgeClimbPose(float NormalizedTime);

	/** Restores the shared locomotion AnimBP after a W02 one-shot action. */
	void RestoreMainWorldLocomotionAnimation();

	/** Lets a world-specific traversal controller own pressed/released jump semantics. */
	void SetExternalJumpHandlingEnabled(bool bEnabled) { bExternalJumpHandlingEnabled = bEnabled; }
	FSimpleMulticastDelegate& OnExternalJumpPressed() { return ExternalJumpPressed; }
	FSimpleMulticastDelegate& OnExternalJumpReleased() { return ExternalJumpReleased; }
	void ConfigureFantasyWorldForm(bool bEnabled, bool bStartInFantasyForm = true);
	bool IsFantasyFormActive() const { return bFantasyFormActive; }

	/** Plays the rune knight's sword action, then safely returns to locomotion/idle. */
	UFUNCTION(BlueprintCallable, Category="World Walker|Fantasy Animation")
	void PlayFantasyCardAttackAnimation();

	/** Plays the rune knight's hit reaction, then safely returns to locomotion/idle. */
	UFUNCTION(BlueprintCallable, Category="World Walker|Fantasy Animation")
	void PlayFantasyHitReactionAnimation();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void HandleJumpPressed();
	void HandleJumpReleased();
	void MoveForward(float Value);
	void MoveRight(float Value);
	void TryInteract();
	void ToggleWorldForm();
	void ApplyWorldFormVisibility();
	void SetMainWorldAnimeFormVisibility(bool bVisible);
	USkeletalMeshComponent* CreateLinkedAnimePart(
		FName ComponentName,
		const TCHAR* MeshPath,
		USkeletalMeshComponent* PoseLeader);
	USkeletalMeshComponent* CreateAnimeHairPart(USkeletalMeshComponent* HeadComponent);
	void ApplyW02AnimeMaterialTuning();
	void LoadFantasyAnimationAssets(const TCHAR* AssetRoot);
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
	TObjectPtr<USkeletalMeshComponent> MainWorldAnimeTop;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> MainWorldAnimeBottom;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> MainWorldAnimeHair;

	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> MainWorldLocomotionAnimClass;

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
	bool bExternalJumpHandlingEnabled = false;
	bool bMainWorldAnimeFormConfigured = false;
	bool bMainWorldAnimeFormActive = false;
	bool bFantasyFormAvailable = false;
	bool bFantasyFormActive = false;
	bool bWorldFormToggleEnabled = true;
	bool bFantasyActionPlaying = false;
	FVector FantasyFormVisualScale = FVector(0.52f);
	float FantasyActionEndTime = 0.0f;
	EWorldWalkerFantasyAnimationState CurrentFantasyAnimationState =
		EWorldWalkerFantasyAnimationState::None;
	float InteractionDistance = 350.0f;
	FSimpleMulticastDelegate ExternalJumpPressed;
	FSimpleMulticastDelegate ExternalJumpReleased;
};
