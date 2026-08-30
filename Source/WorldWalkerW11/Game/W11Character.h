#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/W11Types.h"
#include "W11Character.generated.h"

class UPaperFlipbookComponent;
class UPaperSpriteComponent;
class USpringArmComponent;
class UCameraComponent;
class UW11CombatComponent;
class UW11CharacterAnimationSet;
class UW11SkillVisualDefinition;
class AW11Enemy;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FW11IncomingCombatCueReceived, const FW11CombatCue&, Cue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FW11DodgeCooldownChanged, float, CooldownEndServerTime);

UCLASS()
class WORLDWALKERW11_API AW11Character : public ACharacter
{
	GENERATED_BODY()

public:
	AW11Character();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="W11|Presentation")
	UPaperFlipbookComponent* GetFlipbookComponent() const { return FlipbookComponent; }

	UFUNCTION(BlueprintPure, Category="W11|Presentation")
	UPaperSpriteComponent* GetPrototypeSpriteComponent() const { return PrototypeSpriteComponent; }

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	UW11CombatComponent* GetCombatComponent() const { return CombatComponent; }

	UFUNCTION(BlueprintPure, Category="W11|Presentation")
	UPaperFlipbookComponent* GetSkillEffectComponent() const { return SkillEffectComponent; }

	UFUNCTION(BlueprintPure, Category="W11|Movement")
	bool IsDodging() const { return bDodgeActive || bLocalDodgeActive; }

	void PlayCombatHitReaction(float HitStopSeconds = 0.025f, float CameraImpactStrength = 0.0f,
		float CameraImpactDuration = 0.12f);

	UFUNCTION(BlueprintPure, Category="W11|Combat")
	float GetDodgeCooldownEndTime() const { return FMath::Max(DodgeCooldownEndTime, LocalNextDodgeAllowedTime); }

	/** Clears dodge, cooldown and local impact state at the authoritative memory-run boundary. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Combat")
	void ResetForNewMemoryRun();

	static bool IsValidDodgeRequest(
		const FVector& Direction,
		bool bCombatRequestAllowed,
		bool bDodgeAlreadyActive,
		bool bDefeated,
		float CurrentServerTime,
		float NextAllowedServerTime);
	static float ResolveArenaCameraWidth(int32 ChapterIndex, int32 StageIndex);

	/** Server forwards transient incoming-hit presentation only to the owning player. */
	void SendIncomingCombatCue(const FW11CombatCue& Cue);

	UPROPERTY(BlueprintAssignable, Category="W11|Combat")
	FW11IncomingCombatCueReceived OnIncomingCombatCue;

	/** Owner-only authoritative dodge cooldown timestamp changed. */
	UPROPERTY(BlueprintAssignable, Category="W11|Combat")
	FW11DodgeCooldownChanged OnDodgeCooldownChanged;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void AttackFacing();
	void TryDodge();
	float PerformDodgeMovement(const FVector& Direction);
	void FinishDodge();
	void CheckReplicatedDodgeLanding(int32 ExpectedSequence, FVector ExpectedLanding);
	FVector ResolveDodgeDirection() const;
	FVector2D ResolveFacingCombatDirection() const;
	void AttackUp();
	void AttackDown();
	void AttackLeft();
	void AttackRight();
	void CastInitialAbility();
	void CastAbilitySlot2();
	void CastAbilitySlot3();
	void CastAbilitySlot4();
	void RunScriptedCombatInput();
	void UpdateScriptedMovement();
	AW11Enemy* FindNearestScriptedEnemy(int32 EncounterInstanceId) const;
	void RefreshPresentationDefinitions();
	void LoadSkillVisual(FName AbilityId);
	void UpdatePresentation(float DeltaSeconds);
	void UpdateFacingFromDirection(const FVector2D& Direction);
	void PlayCharacterAction(EW11CharacterAction Action, float LockDuration);
	void PlayActiveAbilityPresentation();
	void ApplyLocalHitStop(float DurationSeconds);
	void FinishLocalHitStop();
	void TriggerLocalCameraImpact(float Strength, float DurationSeconds);
	void UpdateLocalCameraImpact(float DeltaSeconds);
	class UPaperFlipbook* ResolveActionFlipbook(EW11CharacterAction Action) const;

	UFUNCTION()
	void HandleCombatCue(const FW11CombatCue& Cue);

	UFUNCTION(Client, Unreliable)
	void ClientReceiveIncomingCombatCue(FW11CombatCue Cue);

	UFUNCTION(Server, Reliable)
	void ServerTryDodge(FVector_NetQuantizeNormal Direction);

	UFUNCTION(Client, Reliable)
	void ClientResetForNewMemoryRun();

	UFUNCTION()
	void OnRep_DodgeSequence();

	UFUNCTION()
	void OnRep_DodgeCooldownEndTime();

	void ResetLocalTransientCombatState();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="W11|Combat", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UW11CombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="W11|Presentation", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPaperFlipbookComponent> FlipbookComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="W11|Presentation", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPaperFlipbookComponent> SkillEffectComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="W11|Presentation", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPaperSpriteComponent> PrototypeSpriteComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="W11|Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="W11|Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> TopDownCamera;

	UPROPERTY(Transient)
	TObjectPtr<UW11CharacterAnimationSet> ActiveAnimationSet;

	UPROPERTY(Transient)
	TObjectPtr<UW11SkillVisualDefinition> ActiveSkillVisual;

	FName LoadedHeroId = NAME_None;
	FName LoadedAbilityId = NAME_None;
	EW11FacingDirection FacingDirection;
	EW11FacingDirection LastHorizontalFacing;
	EW11CharacterAction CurrentAction;
	int32 ObservedAttackSequence = 0;
	int32 ObservedAbilitySequence = 0;
	float ActionLockUntil = -1.0f;
	float MoveVerticalInput = 0.0f;
	float MoveHorizontalInput = 0.0f;
	float NextDodgeAllowedTime = -1.0f;
	float LocalNextDodgeAllowedTime = -1.0f;
	bool bLocalDodgeActive = false;
	FTimerHandle DodgeTimerHandle;
	FTimerHandle DodgePositionCheckTimerHandle;
	FTimerHandle ScriptedInputTimerHandle;
	FTimerHandle LocalHitStopTimerHandle;
	int32 ScriptedInputStep = 0;
	FName ScriptedInputMode = NAME_None;
	FW11CombatCueDeduplicator ObservedCueDeduplicator;
	FW11CombatCueDeduplicator IncomingCueDeduplicator;
	FW11CombatAudioGate CombatAudioGate;
	FW11CombatAudioGate CombatCameraGate;
	FVector BaseCameraRelativeLocation = FVector::ZeroVector;
	float LocalCameraImpactRemaining = 0.0f;
	float LocalCameraImpactDuration = 0.0f;
	float LocalCameraImpactStrength = 0.0f;
	int32 AppliedCameraChapterIndex = INDEX_NONE;
	int32 AppliedCameraStageIndex = INDEX_NONE;

	UPROPERTY(Replicated)
	bool bDodgeActive = false;

	UPROPERTY(Replicated)
	FVector_NetQuantizeNormal LastDodgeDirection = FVector::ForwardVector;

	UPROPERTY(ReplicatedUsing=OnRep_DodgeSequence)
	int32 DodgeSequence = 0;

	/** Quantized authoritative sweep result used to audit remote dodge convergence. */
	UPROPERTY(Replicated)
	FVector_NetQuantize100 DodgeLandingLocation = FVector::ZeroVector;

	UPROPERTY(ReplicatedUsing=OnRep_DodgeCooldownEndTime)
	float DodgeCooldownEndTime = 0.0f;

	UPROPERTY(Replicated)
	float DodgeDistance = 260.0f;

	UPROPERTY(Replicated)
	float DodgeDuration = 0.18f;

	UPROPERTY(Replicated)
	float DodgeCooldown = 0.75f;

	UPROPERTY(Replicated)
	float DodgeInvulnerabilityDuration = 0.20f;
};
