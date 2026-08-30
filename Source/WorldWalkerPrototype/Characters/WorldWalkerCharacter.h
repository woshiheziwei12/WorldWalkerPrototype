#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WorldWalkerCharacter.generated.h"

class UCameraComponent;
class UCardCombatComponent;
class UCombatantComponent;
class UAnimSequence;
class UAnimInstance;
class UPoseableMeshComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UWorldWalkerMotionPickerWidget;
enum class EFantasyPlayerProfession : uint8;

enum class EWorldWalkerFantasyAnimationState : uint8
{
	None,
	Idle,
	Walk,
	Run,
	Attack,
	Utility,
	Spell,
	HitReact
};

enum class EWorldWalkerMainMotionState : uint8
{
	None,
	Idle,
	Walk,
	Run,
	Sprint,
	Dash,
	Jump,
	Fall,
	LightLanding,
	HardLanding,
	Roll,
	Attack,
	LightStop,
	MediumStop,
	HardStop
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
	/** Applies Lumine in W00, with the modular anime woman retained only as a visual fallback. */
	bool ConfigureMainWorldAnimeForm();
	/** Reuses W00's Lumine visual in W02 and disables world-form switching. */
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
	void ConfigureFantasyProfession(EFantasyPlayerProfession Profession);
	bool IsFantasyFormActive() const { return bFantasyFormActive; }

	/** Plays the W01 form's card action, then safely returns to locomotion/idle. */
	UFUNCTION(BlueprintCallable, Category="World Walker|Fantasy Animation")
	void PlayFantasyCardAttackAnimation();

	/** Plays a guarded/utility card gesture distinct from an attack. */
	UFUNCTION(BlueprintCallable, Category="World Walker|Fantasy Animation")
	void PlayFantasyCardUtilityAnimation();

	/** Plays the W01 form's spell gesture. */
	UFUNCTION(BlueprintCallable, Category="World Walker|Fantasy Animation")
	void PlayFantasyCardSpellAnimation();

	/** Plays the W01 form's hit reaction, then safely returns to locomotion/idle. */
	UFUNCTION(BlueprintCallable, Category="World Walker|Fantasy Animation")
	void PlayFantasyHitReactionAnimation();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void HandleJumpPressed();
	void HandleJumpReleased();
	void HandleMainWorldDashPressed();
	void HandleMainWorldRollPressed();
	void HandleMainWorldAttackPressed();
	void StartMainWorldAttackComboStep(int32 AttackIndex);
	void UpdateMainWorldAttackCombo();
	void CancelMainWorldAttackCombo(const TCHAR* Reason);
	void HandleMainWorldNextMotionPressed();
	void TriggerMainWorldMotionPreview(int32 PreviewIndex);
	void HandleMainWorldMotionPicked(int32 PreviewIndex);
	void CloseMainWorldMotionPicker();
	void MoveForward(float Value);
	void MoveRight(float Value);

	/** Bound to E; reflected so unattended smoke tests can exercise the real interaction path. */
	UFUNCTION()
	void TryInteract();
	void ToggleWorldForm();
	void ApplyWorldFormVisibility();
	void SetMainWorldAnimeFormVisibility(bool bVisible);
	bool ConfigureLumineVisual(USkeletalMeshComponent* PoseSource);
	bool ConfigureGenshinMotionSource();
	USkeletalMeshComponent* GetMainWorldPoseSource() const;
	void BuildLumineBoneMap(USkeletalMeshComponent* PoseSource);
	void RefreshMainWorldGenshinMotion(float DeltaSeconds, bool bForce = false);
	void PlayMainWorldGenshinMotion(
		UAnimSequence* Animation,
		EWorldWalkerMainMotionState State,
		bool bLoop,
		float PlayRate = 1.0f);
	void UpdateMainWorldLuminePose(float DeltaSeconds);
	USkeletalMeshComponent* CreateLinkedAnimePart(
		FName ComponentName,
		const TCHAR* MeshPath,
		USkeletalMeshComponent* PoseLeader);
	USkeletalMeshComponent* CreateAnimeHairPart(USkeletalMeshComponent* HeadComponent);
	void ApplyW02AnimeMaterialTuning();
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
	TObjectPtr<USkeletalMeshComponent> MainWorldAnimeTop;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> MainWorldAnimeBottom;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> MainWorldAnimeHair;

	UPROPERTY(Transient)
	TObjectPtr<UPoseableMeshComponent> MainWorldLumineMesh;

	/** Traveler sword follows the right hand through the authored WeaponR grip offset. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> MainWorldLumineSword;

	/** Hidden same-skeleton PlayerGirl source that evaluates the original clips for Lumine. */
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> MainWorldGenshinMotionSource;

	/** Mesh whose skeleton owns the ACL-fixed generic locomotion clips. */
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMesh> MainWorldGenshinOriginalMotionMesh;

	/** Hidden carrier for the preserved per-character PlayerGirl action clips. */
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMesh> MainWorldGenshinLegacyMotionMesh;

	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> MainWorldLocomotionAnimClass;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> MainWorldGenshinIdle;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> MainWorldGenshinWalk;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> MainWorldGenshinRun;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> MainWorldGenshinSprint;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> MainWorldGenshinDash;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> MainWorldGenshinJump;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> MainWorldGenshinFall;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> MainWorldGenshinLightLanding;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> MainWorldGenshinHardLanding;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> MainWorldGenshinRoll;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAnimSequence>> MainWorldOriginalActionAnimations;

	/** Ordered six-step sword combo driven by repeated left mouse presses. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAnimSequence>> MainWorldAttackAnimations;

	UPROPERTY(Transient)
	TObjectPtr<UWorldWalkerMotionPickerWidget> MainWorldMotionPicker;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> FantasyIdleAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> FantasyWalkAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> FantasyRunAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> FantasyAttackAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> FantasyUtilityAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> FantasySpellAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> FantasyHitReactionAnimation;

	struct FLumineBoneLink
	{
		int32 SourceBoneIndex = INDEX_NONE;
		int32 TargetBoneIndex = INDEX_NONE;
		FTransform SourceReferenceComponentTransform = FTransform::Identity;
		FTransform TargetReferenceComponentTransform = FTransform::Identity;
	};

	TArray<FLumineBoneLink> LumineBoneLinks;
	TArray<int32> LumineLinkIndexByTargetBone;
	TArray<FString> MainWorldOriginalActionNames;
	TArray<EWorldWalkerMainMotionState> MainWorldOriginalActionStates;
	TArray<bool> MainWorldOriginalActionLoops;
	FVector MainWorldLumineBaseLocation = FVector::ZeroVector;
	FRotator MainWorldLumineBaseRotation = FRotator::ZeroRotator;
	float MainWorldLumineVisualScale = 1.0f;
	float CurrentLuminePrimaryArmRotationWeight = 1.0f;

	UPROPERTY(VisibleAnywhere, Category="Combat")
	TObjectPtr<UCombatantComponent> CombatantComponent;

	UPROPERTY(VisibleAnywhere, Category="Cards")
	TObjectPtr<UCardCombatComponent> CardCombatComponent;

	bool bCombatLocked = false;
	bool bExternalJumpHandlingEnabled = false;
	bool bMainWorldAnimeFormConfigured = false;
	bool bMainWorldAnimeFormActive = false;
	bool bMainWorldLumineConfigured = false;
	bool bMainWorldGenshinMotionConfigured = false;
	bool bLuminePoseUsesPlayerGirlSkeleton = false;
	bool bMainWorldGenshinOneShotPlaying = false;
	bool bMainWorldAttackComboActive = false;
	bool bMainWorldMotionPreviewActive = false;
	bool bFantasyFormAvailable = false;
	bool bFantasyFormActive = false;
	bool bWorldFormToggleEnabled = true;
	bool bFantasyActionPlaying = false;
	FVector FantasyFormVisualScale = FVector(0.52f);
	EFantasyPlayerProfession RequestedFantasyProfession;
	float FantasyActionEndTime = 0.0f;
	EWorldWalkerFantasyAnimationState CurrentFantasyAnimationState =
		EWorldWalkerFantasyAnimationState::None;
	EWorldWalkerMainMotionState CurrentMainWorldMotionState =
		EWorldWalkerMainMotionState::None;
	float MainWorldGenshinOneShotEndTime = 0.0f;
	float MainWorldAttackChainTime = 0.0f;
	float MainWorldAttackEndTime = 0.0f;
	int32 CurrentMainWorldAttackIndex = INDEX_NONE;
	int32 PendingMainWorldAttackInputs = 0;
	int32 AttackComboTestInputsSent = 0;
	float MainWorldMotionPreviewEndTime = 0.0f;
	int32 NextMainWorldMotionPreviewIndex = 0;
	float PreviousMainWorldVerticalVelocity = 0.0f;
	bool bMainWorldWasFalling = false;
	bool bAvatarCaptureMode = false;
	bool bAvatarCaptureScreenshotRequested = false;
	bool bMotionPickerVisualTestOpened = false;
	bool bMotionPickerActionTestTriggered = false;
	float AvatarCaptureScreenshotTime = 0.0f;
	float NextMainWorldMotionDiagnosticTime = 0.0f;
	float InteractionDistance = 350.0f;
	FSimpleMulticastDelegate ExternalJumpPressed;
	FSimpleMulticastDelegate ExternalJumpReleased;
};
