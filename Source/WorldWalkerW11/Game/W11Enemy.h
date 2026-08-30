#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/W11Types.h"
#include "Data/W11Definitions.h"
#include "W11Enemy.generated.h"

class UPaperFlipbookComponent;
class UPaperFlipbook;
class UPaperSprite;
class UPaperSpriteComponent;
class UW11AttributeComponent;
class UW11AbilityDefinition;
class UW11EnemyDefinition;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UCLASS()
class WORLDWALKERW11_API AW11Enemy : public ACharacter
{
	GENERATED_BODY()

public:
	AW11Enemy();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser) override;

	void InitializeFromDefinition(
		const UW11EnemyDefinition* Definition,
		float HealthScale = 1.0f,
		float PowerScale = 1.0f,
		int32 InEncounterInstanceId = 0,
		int32 InStableSpawnId = 0);

	void CancelForCombatEnd(int32 EndingEncounterInstanceId);
	static FW11ReplicatedAttackState MakeInactiveAttackState(
		const FW11ReplicatedAttackState& CurrentState,
		EW11EnemyCombatState NextState);

	UFUNCTION(BlueprintPure, Category="W11|Enemy")
	UW11AttributeComponent* GetAttributeComponent() const { return AttributeComponent; }

	UFUNCTION(BlueprintPure, Category="W11|Enemy")
	FName GetEnemyDefinitionId() const { return EnemyDefinitionId; }

	UFUNCTION(BlueprintPure, Category="W11|Enemy")
	int32 GetEncounterInstanceId() const { return EncounterInstanceId; }

	UFUNCTION(BlueprintPure, Category="W11|Enemy")
	int32 GetStableSpawnId() const { return StableSpawnId; }

	UFUNCTION(BlueprintPure, Category="W11|Enemy")
	bool IsBoss() const { return bBoss; }

	UFUNCTION(BlueprintPure, Category="W11|Enemy|Boss")
	int32 GetBossPhaseIndex() const { return BossPhaseIndex; }

	UFUNCTION(BlueprintPure, Category="W11|Enemy|Boss")
	FName GetBossPhaseId() const { return BossPhaseId; }
	int32 GetAuthoredBossTransitionCount() const { return BossPhases.Num(); }

	UFUNCTION(BlueprintPure, Category="W11|Enemy")
	const FW11ReplicatedAttackState& GetAttackState() const { return AttackState; }

	const UW11AbilityDefinition* GetPrimaryAttackDefinition() const { return PrimaryAttackDefinition; }
	const UW11AbilityDefinition* GetAttackDefinitionForSequence(int32 AttackSequence) const;
	const UW11AbilityDefinition* FindAttackDefinition(FName AbilityId) const;
	void SetAttackState(const FW11ReplicatedAttackState& NewState);
	void PlayCombatHitReaction(float HitStopSeconds = 0.025f);
	void BroadcastCombatCue(const FW11CombatCue& Cue);

private:
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastCombatCue(FW11CombatCue Cue);

	UFUNCTION()
	void HandleDefeated();

	UFUNCTION()
	void HandleBossHealthChanged(float CurrentHealth, float MaximumHealth);

	UFUNCTION()
	void OnRep_PresentationSprite();

	UFUNCTION()
	void OnRep_MovementFlipbooks();

	UFUNCTION()
	void OnRep_AttackState();

	void RefreshTelegraphVisual();
	void PlayAttackWarningAudio();
	void ApplyLocalHitStop(float DurationSeconds);
	void FinishLocalHitStop();
	void UpdateMovementPresentation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="W11|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UW11AttributeComponent> AttributeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="W11|Presentation", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPaperFlipbookComponent> FlipbookComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="W11|Presentation", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPaperSpriteComponent> PrototypeSpriteComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="W11|Telegraph", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> LineTelegraphMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="W11|Telegraph", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> CircleTelegraphMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LineTelegraphMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CircleTelegraphMaterial;

	UPROPERTY(Replicated, VisibleAnywhere, Category="W11|Enemy")
	FName EnemyDefinitionId = NAME_None;

	UPROPERTY(Replicated, VisibleAnywhere, Category="W11|Enemy")
	int32 EncounterInstanceId = 0;

	UPROPERTY(Replicated, VisibleAnywhere, Category="W11|Enemy")
	int32 StableSpawnId = 0;

	UPROPERTY(Replicated, VisibleAnywhere, Category="W11|Enemy")
	bool bBoss = false;

	UPROPERTY(Replicated, VisibleAnywhere, Category="W11|Enemy|Boss")
	int32 BossPhaseIndex = 0;

	UPROPERTY(Replicated, VisibleAnywhere, Category="W11|Enemy|Boss")
	FName BossPhaseId = NAME_None;

	UPROPERTY(ReplicatedUsing=OnRep_PresentationSprite)
	TSoftObjectPtr<UPaperSprite> PresentationSprite;

	UPROPERTY(ReplicatedUsing=OnRep_MovementFlipbooks)
	TSoftObjectPtr<UPaperFlipbook> PresentationMoveFlipbookRight;

	UPROPERTY(ReplicatedUsing=OnRep_MovementFlipbooks)
	TSoftObjectPtr<UPaperFlipbook> PresentationMoveFlipbookLeft;

	UPROPERTY(ReplicatedUsing=OnRep_AttackState)
	FW11ReplicatedAttackState AttackState;

	UPROPERTY(Transient)
	TObjectPtr<UW11AbilityDefinition> PrimaryAttackDefinition;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UW11AbilityDefinition>> AttackDefinitions;

	UPROPERTY(Transient)
	TObjectPtr<UPaperFlipbook> MoveFlipbookRight;

	UPROPERTY(Transient)
	TObjectPtr<UPaperFlipbook> MoveFlipbookLeft;

	TArray<FW11BossPhaseDefinition> BossPhases;
	FW11CombatCueDeduplicator CueDeduplicator;
	FW11CombatAudioGate CombatAudioGate;
	int32 LastWarningAudioEncounterInstanceId = 0;
	int32 LastWarningAudioAttackInstanceId = 0;
	int32 LastTelegraphVisualEncounterInstanceId = 0;
	int32 LastTelegraphVisualAttackInstanceId = 0;
	FTimerHandle LocalHitStopTimerHandle;

	bool bDefeatHandled = false;
	bool bLastMovementFacingLeft = false;
};
