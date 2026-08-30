#include "Game/W11EnemyController.h"

#include "Components/W11AttributeComponent.h"
#include "Data/W11Definitions.h"
#include "EngineUtils.h"
#include "Game/W11Character.h"
#include "Game/W11Enemy.h"
#include "Game/W11EnemyProjectile.h"
#include "Game/W11GameMode.h"
#include "Game/W11GameState.h"
#include "Game/W11PlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WorldWalkerW11.h"

namespace
{
int32 AdvanceAttackSequence(const int32 Sequence)
{
	return Sequence == MAX_int32 ? 1 : Sequence + 1;
}
}

AW11EnemyController::AW11EnemyController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
}

void AW11EnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	UE_LOG(LogWorldWalkerW11, Display, TEXT("W11_ENEMY_AI_POSSESSED Pawn=%s Authority=%d"),
		*GetNameSafe(InPawn), InPawn && InPawn->HasAuthority() ? 1 : 0);
}

void AW11EnemyController::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AW11Enemy* Enemy = Cast<AW11Enemy>(GetPawn());
	if (!Enemy || !Enemy->GetAttributeComponent() || Enemy->GetAttributeComponent()->IsDefeated())
	{
		return;
	}
	if (!IsEncounterCurrent(Enemy))
	{
		CancelCurrentAttack(Enemy, EW11EnemyCombatState::AcquireTarget);
		return;
	}
	const FW11ReplicatedAttackState& AttackState = Enemy->GetAttackState();
	const bool bAttackLocked = AttackState.State == EW11EnemyCombatState::Windup
		|| AttackState.State == EW11EnemyCombatState::Active
		|| AttackState.State == EW11EnemyCombatState::Recovery;
	const UW11AbilityDefinition* Ability = bAttackLocked
		? Enemy->FindAttackDefinition(AttackState.AttackDefinitionId)
		: Enemy->GetAttackDefinitionForSequence(AttackInstanceSequence + 1);
	if (!Ability)
	{
		if (!bLoggedMissingAbility)
		{
			bLoggedMissingAbility = true;
			UE_LOG(LogWorldWalkerW11, Warning,
				TEXT("W11_ENEMY_ATTACK_DEFINITION_MISSING SpawnId=%d Enemy=%s"),
				Enemy->GetStableSpawnId(), *Enemy->GetEnemyDefinitionId().ToString());
		}
		SetLocomotionState(Enemy, EW11EnemyCombatState::AcquireTarget);
		return;
	}
	bLoggedMissingAbility = false;

	const float Now = GetWorld()->GetTimeSeconds();
	switch (AttackState.State)
	{
	case EW11EnemyCombatState::Windup:
		if (!IsValidTarget(CurrentTarget.Get()))
		{
			CancelCurrentAttack(Enemy, EW11EnemyCombatState::AcquireTarget);
		}
		else if (Now + UE_KINDA_SMALL_NUMBER >= AttackState.ActiveStartServerTime)
		{
			FW11ReplicatedAttackState ActiveState = AttackState;
			ActiveState.State = EW11EnemyCombatState::Active;
			Enemy->SetAttackState(ActiveState);
			bActiveResolved = false;
		}
		return;
	case EW11EnemyCombatState::Active:
		if (!bActiveResolved)
		{
			ResolveActive(Enemy, Ability);
			bActiveResolved = true;
		}
		if (Now + UE_KINDA_SMALL_NUMBER >= AttackState.ActiveStartServerTime + Ability->ActiveSeconds)
		{
			FW11ReplicatedAttackState RecoveryState = Enemy->GetAttackState();
			RecoveryState.State = EW11EnemyCombatState::Recovery;
			RecoveryState.TelegraphShape = EW11TelegraphShape::None;
			Enemy->SetAttackState(RecoveryState);
		}
		return;
	case EW11EnemyCombatState::Recovery:
		if (Now + UE_KINDA_SMALL_NUMBER >= AttackState.RecoveryEndServerTime)
		{
			CancelCurrentAttack(Enemy, EW11EnemyCombatState::AcquireTarget);
		}
		return;
	case EW11EnemyCombatState::Defeated:
		return;
	default:
		break;
	}

	if (!IsValidTarget(CurrentTarget.Get()))
	{
		CurrentTarget = FindNearestTarget();
	}
	AW11Character* Target = CurrentTarget.Get();
	if (!Target)
	{
		if (!bLoggedMissingTarget)
		{
			bLoggedMissingTarget = true;
			UE_LOG(LogWorldWalkerW11, Warning,
				TEXT("W11_ENEMY_TARGET_MISSING SpawnId=%d EncounterInstance=%d"),
				Enemy->GetStableSpawnId(), Enemy->GetEncounterInstanceId());
		}
		SetLocomotionState(Enemy, EW11EnemyCombatState::AcquireTarget);
		return;
	}
	bLoggedMissingTarget = false;

	const FVector ToTarget = Target->GetActorLocation() - Enemy->GetActorLocation();
	const float Distance = ToTarget.Size2D();
	const float PreferredRange = FMath::Max(80.0f, Ability->PreferredRange);
	if (Ability->Delivery == EW11HitDelivery::Projectile && Distance < PreferredRange * 0.65f)
	{
		SetLocomotionState(Enemy, EW11EnemyCombatState::Reposition);
		Enemy->AddActorWorldOffset(
			-ToTarget.GetSafeNormal2D() * Enemy->GetCharacterMovement()->MaxFlySpeed
				* Enemy->GetAttributeComponent()->GetMovementStatusScale() * DeltaSeconds,
			true);
		return;
	}
	if (Distance > PreferredRange)
	{
		SetLocomotionState(Enemy, EW11EnemyCombatState::Approach);
		Enemy->AddActorWorldOffset(
			ToTarget.GetSafeNormal2D() * Enemy->GetCharacterMovement()->MaxFlySpeed
				* Enemy->GetAttributeComponent()->GetMovementStatusScale() * DeltaSeconds,
			true);
		return;
	}

	BeginWindup(Enemy, Target, Ability);
}

AW11Character* AW11EnemyController::FindNearestTarget() const
{
	AW11Character* Nearest = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (TActorIterator<AW11Character> It(GetWorld()); It; ++It)
	{
		AW11Character* Candidate = *It;
		if (!IsValidTarget(Candidate))
		{
			continue;
		}
		const float DistanceSquared = FVector::DistSquared2D(GetPawn()->GetActorLocation(), Candidate->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			Nearest = Candidate;
		}
	}
	return Nearest;
}

bool AW11EnemyController::IsValidTarget(const AW11Character* Target) const
{
	const AW11PlayerState* W11PlayerState = Target ? Target->GetPlayerState<AW11PlayerState>() : nullptr;
	return Target && W11PlayerState && W11PlayerState->GetAttributeComponent()
		&& !W11PlayerState->GetAttributeComponent()->IsDefeated();
}

bool AW11EnemyController::IsEncounterCurrent(const AW11Enemy* Enemy) const
{
	const AW11GameState* State = GetWorld() ? GetWorld()->GetGameState<AW11GameState>() : nullptr;
	return State && Enemy && AW11GameState::IsPendingCombatEncounter(
		State->GetRunPhase(), State->GetCombatOutcome(),
		State->GetEncounterInstanceId(), Enemy->GetEncounterInstanceId());
}

void AW11EnemyController::SetLocomotionState(
	AW11Enemy* Enemy,
	const EW11EnemyCombatState NewState)
{
	if (!Enemy || Enemy->GetAttackState().State == NewState)
	{
		return;
	}
	FW11ReplicatedAttackState State = Enemy->GetAttackState();
	State.EncounterInstanceId = Enemy->GetEncounterInstanceId();
	State.State = NewState;
	State.TelegraphShape = EW11TelegraphShape::None;
	Enemy->SetAttackState(State);
	UE_LOG(LogWorldWalkerW11, Verbose,
		TEXT("W11_ENEMY_STATE_CHANGED EncounterInstance=%d SpawnId=%d State=%d"),
		Enemy->GetEncounterInstanceId(), Enemy->GetStableSpawnId(), static_cast<int32>(NewState));
}

void AW11EnemyController::BeginWindup(
	AW11Enemy* Enemy,
	AW11Character* Target,
	const UW11AbilityDefinition* Ability)
{
	if (!Enemy || !Target || !Ability || !IsEncounterCurrent(Enemy))
	{
		return;
	}
	Enemy->GetCharacterMovement()->StopMovementImmediately();
	AttackInstanceSequence = AdvanceAttackSequence(AttackInstanceSequence);
	const float Now = GetWorld()->GetTimeSeconds();
	const FVector Direction = (Target->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
	const AW11PlayerState* W11PlayerState = Target->GetPlayerState<AW11PlayerState>();
	FW11ReplicatedAttackState State;
	State.EncounterInstanceId = Enemy->GetEncounterInstanceId();
	State.AttackInstanceId = AttackInstanceSequence;
	State.AttackDefinitionId = Ability->DefinitionId;
	State.State = EW11EnemyCombatState::Windup;
	State.LockedTargetStableId = W11PlayerState ? W11PlayerState->GetPlayerId() : 0;
	State.LockedDirection = Direction;
	State.LockedTargetPoint = Target->GetActorLocation();
	State.TelegraphShape = Ability->TelegraphShape;
	State.TelegraphRange = Ability->BaseRange;
	State.TelegraphRadius = Ability->BaseRadius;
	State.WindupStartServerTime = Now;
	State.ActiveStartServerTime = Now + FMath::Max(0.05f, Ability->WindupSeconds);
	State.RecoveryEndServerTime = State.ActiveStartServerTime
		+ FMath::Max(0.0f, Ability->ActiveSeconds)
		+ FMath::Max(0.0f, Ability->RecoverySeconds);
	bActiveResolved = false;
	Enemy->SetAttackState(State);
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_ENEMY_WINDUP_STARTED EncounterInstance=%d SpawnId=%d AttackInstance=%d Ability=%s Target=%d Windup=%.2f ActiveAt=%.3f RecoveryEnd=%.3f"),
		State.EncounterInstanceId, Enemy->GetStableSpawnId(), State.AttackInstanceId,
		*State.AttackDefinitionId.ToString(), State.LockedTargetStableId, Ability->WindupSeconds,
		State.ActiveStartServerTime, State.RecoveryEndServerTime);
}

void AW11EnemyController::ResolveActive(AW11Enemy* Enemy, const UW11AbilityDefinition* Ability)
{
	if (!Enemy || !Ability || !IsEncounterCurrent(Enemy))
	{
		return;
	}
	const FW11ReplicatedAttackState State = Enemy->GetAttackState();
	if (AW11GameMode* GameMode = GetWorld()->GetAuthGameMode<AW11GameMode>())
	{
		GameMode->RecordEnemyAttack(Enemy->GetEnemyDefinitionId());
	}
	if (Ability->Delivery == EW11HitDelivery::Projectile)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector SpawnLocation = Enemy->GetActorLocation() + FVector(State.LockedDirection) * 72.0f;
		if (AW11EnemyProjectile* Projectile = GetWorld()->SpawnActor<AW11EnemyProjectile>(
			AW11EnemyProjectile::StaticClass(), SpawnLocation, FRotator::ZeroRotator, Params))
		{
			Projectile->InitializeProjectile(
				Enemy, Ability, State.LockedDirection, State.EncounterInstanceId, State.AttackInstanceId);
		}
	}
	else
	{
		ApplyImmediateAttack(Enemy, Ability, State);
	}
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_ENEMY_ATTACK_ACTIVE EncounterInstance=%d SpawnId=%d AttackInstance=%d Ability=%s Delivery=%d"),
		State.EncounterInstanceId, Enemy->GetStableSpawnId(), State.AttackInstanceId,
		*State.AttackDefinitionId.ToString(), static_cast<int32>(Ability->Delivery));
}

void AW11EnemyController::ApplyImmediateAttack(
	AW11Enemy* Enemy,
	const UW11AbilityDefinition* Ability,
	const FW11ReplicatedAttackState& AttackState)
{
	FVector QueryStart = Enemy->GetActorLocation();
	FVector QueryEnd = QueryStart;
	if (Ability->HitShape == EW11HitShape::Line)
	{
		QueryEnd += FVector(AttackState.LockedDirection) * Ability->BaseRange;
	}
	else
	{
		QueryStart = AttackState.LockedTargetPoint;
		QueryEnd = QueryStart;
	}
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(W11EnemyImmediateAttack), false, Enemy);
	GetWorld()->SweepMultiByObjectType(
		Hits, QueryStart, QueryEnd, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(FMath::Max(1.0f, Ability->BaseRadius)), Params);
	TMap<int32, AW11Character*> StableTargets;
	for (const FHitResult& Hit : Hits)
	{
		AW11Character* Target = Cast<AW11Character>(Hit.GetActor());
		AW11PlayerState* W11PlayerState = Target ? Target->GetPlayerState<AW11PlayerState>() : nullptr;
		if (IsValidTarget(Target) && W11PlayerState)
		{
			StableTargets.FindOrAdd(W11PlayerState->GetPlayerId(), Target);
		}
	}
	TArray<int32> StableIds;
	StableTargets.GetKeys(StableIds);
	StableIds.Sort();
	for (const int32 StableId : StableIds)
	{
		AW11Character* Target = StableTargets[StableId];
		AW11PlayerState* W11PlayerState = Target ? Target->GetPlayerState<AW11PlayerState>() : nullptr;
		UW11AttributeComponent* Attributes = W11PlayerState ? W11PlayerState->GetAttributeComponent() : nullptr;
		if (!Attributes)
		{
			continue;
		}
		FW11DamageSpec Spec;
		Spec.Source = Enemy;
		Spec.AbilityId = Ability->DefinitionId;
		Spec.RawDamage = Ability->BaseDamage * Ability->PowerCoefficient
			* Enemy->GetAttributeComponent()->GetStats().Power;
		Spec.ImpactLocation = Target->GetActorLocation();
		Spec.ImpactDirection = AttackState.LockedDirection;
		if (AW11GameMode* GameMode = GetWorld()->GetAuthGameMode<AW11GameMode>())
		{
			GameMode->RecordEnemyHit(Enemy->GetEnemyDefinitionId());
		}
		const FW11DamageBreakdown Damage = Attributes->ApplyDamageSpec(Spec);
		for (const FW11StatusEffectSpec& StatusSpec : Ability->StatusEffects)
		{
			FW11ActiveStatus Applied;
			bool bRefreshed = false;
			if (Attributes->ApplyStatusEffect(StatusSpec, Enemy, Ability->DefinitionId, Applied, bRefreshed))
			{
				UE_LOG(LogWorldWalkerW11, Display,
					TEXT("W11_STATUS_APPLIED Target=%s Status=%s Type=%d Stacks=%d End=%.3f Ability=%s"),
					*GetNameSafe(Target), *Applied.StatusId.ToString(), static_cast<int32>(Applied.Type),
					Applied.Stacks, Applied.EndServerTime, *Ability->DefinitionId.ToString());
			}
		}
		FW11CombatCue Cue;
		Cue.EncounterInstanceId = AttackState.EncounterInstanceId;
		Cue.SourceStableId = Enemy->GetStableSpawnId();
		Cue.AuthorityExecutionSequence = AttackState.AttackInstanceId;
		// One enemy attack can hit several players; the stable player id keeps each
		// target presentation distinct while still deduplicating repeats.
		Cue.EffectSequence = StableId;
		Cue.Source = Enemy;
		Cue.Target = Target;
		Cue.AbilityId = Ability->DefinitionId;
		Cue.Location = Target->GetActorLocation();
		Cue.Direction = AttackState.LockedDirection;
		Cue.PrimaryAmount = Damage.HealthDamage;
		Cue.SecondaryAmount = Damage.BarrierAbsorbed;
		Cue.Type = Damage.bImmune ? EW11CombatCueType::Immune
			: Damage.bDefeated ? EW11CombatCueType::Defeated : EW11CombatCueType::Damage;
		Enemy->BroadcastCombatCue(Cue);
		Target->SendIncomingCombatCue(Cue);
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_ENEMY_ATTACK_HIT EncounterInstance=%d SpawnId=%d AttackInstance=%d Ability=%s Target=%d Raw=%.1f Armor=%.1f Barrier=%.1f Health=%.1f Immune=%d Defeated=%d"),
			AttackState.EncounterInstanceId, Enemy->GetStableSpawnId(), AttackState.AttackInstanceId,
			*Ability->DefinitionId.ToString(), StableId, Damage.RawDamage, Damage.ArmorMitigated,
			Damage.BarrierAbsorbed, Damage.HealthDamage, Damage.bImmune ? 1 : 0, Damage.bDefeated ? 1 : 0);
	}
	if (Ability->HitShape == EW11HitShape::Line)
	{
		Enemy->SetActorLocation(QueryEnd, true, nullptr, ETeleportType::None);
	}
}

void AW11EnemyController::CancelCurrentAttack(
	AW11Enemy* Enemy,
	const EW11EnemyCombatState NextState)
{
	if (!Enemy)
	{
		return;
	}
	const FW11ReplicatedAttackState CurrentState = Enemy->GetAttackState();
	if (CurrentState.State == EW11EnemyCombatState::Windup || CurrentState.State == EW11EnemyCombatState::Active)
	{
		if (AW11GameMode* GameMode = GetWorld()->GetAuthGameMode<AW11GameMode>())
		{
			GameMode->RecordEnemyAttackCancelled(Enemy->GetEnemyDefinitionId());
		}
	}
	Enemy->SetAttackState(AW11Enemy::MakeInactiveAttackState(CurrentState, NextState));
	CurrentTarget.Reset();
	bActiveResolved = false;
}
