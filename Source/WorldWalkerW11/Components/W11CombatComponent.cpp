#include "Components/W11CombatComponent.h"

#include "Components/W11AttributeComponent.h"
#include "Components/W11RunEconomyComponent.h"
#include "Core/W11StatLibrary.h"
#include "Data/W11Definitions.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Game/W11Enemy.h"
#include "Game/W11GameMode.h"
#include "Game/W11GameState.h"
#include "Game/W11PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "WorldWalkerW11.h"

namespace
{
int32 AdvanceSequence(const int32 Sequence)
{
	return Sequence == MAX_int32 ? 1 : Sequence + 1;
}

bool IsFiniteVector2D(const FVector2D& Value)
{
	return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y);
}

bool IsFiniteVector(const FVector& Value)
{
	return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y) && FMath::IsFinite(Value.Z);
}
}

UW11CombatComponent::UW11CombatComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
	ActiveAbilityCooldownEndTimes.Init(0.0f, 4);
}

void UW11CombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UW11CombatComponent, LastAttackDirection);
	DOREPLIFETIME(UW11CombatComponent, AttackSequence);
	DOREPLIFETIME(UW11CombatComponent, AbilitySequence);
	DOREPLIFETIME(UW11CombatComponent, LastExecutedAbilityId);
	DOREPLIFETIME(UW11CombatComponent, AuthorityExecutionSequence);
	DOREPLIFETIME_CONDITION(UW11CombatComponent, BasicAttackCooldownEndTime, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UW11CombatComponent, ActiveAbilityCooldownEndTimes, COND_OwnerOnly);
}

float UW11CombatComponent::GetAbilityCooldownEndTime(const EW11AbilitySlot Slot) const
{
	if (Slot == EW11AbilitySlot::BasicAttack)
	{
		return BasicAttackCooldownEndTime;
	}
	const int32 Index = static_cast<int32>(Slot) - static_cast<int32>(EW11AbilitySlot::Active1);
	return ActiveAbilityCooldownEndTimes.IsValidIndex(Index) ? ActiveAbilityCooldownEndTimes[Index] : 0.0f;
}

void UW11CombatComponent::ResetForNewMemoryRun()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	BasicAttackCooldownEndTime = 0.0f;
	ActiveAbilityCooldownEndTimes.Init(0.0f, 4);
	ActiveAbilityCooldownEndTime = 0.0f;
	LastExecutedAbilityId = NAME_None;
	LastAttackDirection = FVector2D(0.0f, 1.0f);
	OnRep_BasicAttackCooldown();
	OnRep_ActiveAbilityCooldowns();
	GetOwner()->ForceNetUpdate();
}

bool UW11CombatComponent::IsNewerRequestSequence(const int32 Candidate, const int32 Previous)
{
	if (Candidate <= 0)
	{
		return false;
	}
	if (Previous <= 0)
	{
		return true;
	}
	if (Candidate > Previous)
	{
		return true;
	}
	// Sequences use the positive int32 domain and explicitly wrap MAX_int32 -> 1.
	return Previous > MAX_int32 - 1024 && Candidate < 1024;
}

bool UW11CombatComponent::IsValidRequestInput(const FW11AbilityRequest& Request)
{
	const int32 Slot = static_cast<int32>(Request.Slot);
	const float DirectionLengthSquared = Request.InputDirection.SizeSquared();
	return Slot >= static_cast<int32>(EW11AbilitySlot::BasicAttack)
		&& Slot <= static_cast<int32>(EW11AbilitySlot::Active4)
		&& IsFiniteVector2D(Request.InputDirection)
		&& IsFiniteVector(Request.TargetPoint)
		&& DirectionLengthSquared >= 0.98f * 0.98f
		&& DirectionLengthSquared <= 1.02f * 1.02f;
}

FW11EffectResult UW11CombatComponent::BuildNonDamageEffectResult(
	const EW11EffectResultType Type,
	AActor* Source,
	AActor* Target,
	const FName AbilityId,
	const float RequestedAmount,
	const float ActualAmount)
{
	FW11EffectResult Effect;
	Effect.Type = Type;
	Effect.Source = Source;
	Effect.Target = Target;
	Effect.AbilityId = AbilityId;
	Effect.RequestedAmount = FMath::Max(0.0f, RequestedAmount);
	Effect.ActualAmount = FMath::Clamp(ActualAmount, 0.0f, Effect.RequestedAmount);
	Effect.ImpactLocation = Target ? Target->GetActorLocation() : FVector::ZeroVector;
	return Effect;
}

void UW11CombatComponent::TryBasicAttack(const FVector2D Direction)
{
	TryAbilitySlot(EW11AbilitySlot::BasicAttack, Direction);
}

void UW11CombatComponent::TryInitialAbility(const FVector2D Direction)
{
	TryAbilitySlot(EW11AbilitySlot::Active1, Direction);
}

void UW11CombatComponent::TryAbilitySlot(
	const EW11AbilitySlot Slot,
	FVector2D Direction,
	const FVector TargetPoint)
{
	if (!IsFiniteVector2D(Direction) || !IsFiniteVector(TargetPoint))
	{
		return;
	}
	if (Direction.IsNearlyZero())
	{
		Direction = LastAttackDirection;
	}
	Direction.Normalize();
	SubmitLocalRequest(Slot, Direction, TargetPoint);
}

void UW11CombatComponent::SubmitLocalRequest(
	const EW11AbilitySlot Slot,
	const FVector2D& Direction,
	const FVector& TargetPoint)
{
	const AW11GameState* State = GetWorld() ? GetWorld()->GetGameState<AW11GameState>() : nullptr;
	FW11AbilityRequest Request;
	Request.EncounterInstanceId = State ? State->GetEncounterInstanceId() : 0;
	Request.Slot = Slot;
	Request.InputDirection = Direction;
	Request.TargetPoint = TargetPoint;
	LocalRequestSequence = AdvanceSequence(LocalRequestSequence);
	Request.ClientRequestSequence = LocalRequestSequence;
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ExecuteServerRequest(Request);
	}
	else
	{
		ServerSubmitAbilityRequest(Request);
	}
}

void UW11CombatComponent::ServerSubmitAbilityRequest_Implementation(const FW11AbilityRequest Request)
{
	ExecuteServerRequest(Request);
}

void UW11CombatComponent::ExecuteServerRequest(const FW11AbilityRequest& Request)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !GetWorld())
	{
		return;
	}

	if (!IsNewerRequestSequence(Request.ClientRequestSequence, LastClientRequestSequence))
	{
		SendRejectedResult(Request, EW11AbilityRejectionReason::DuplicateRequest);
		return;
	}
	LastClientRequestSequence = Request.ClientRequestSequence;

	if (!IsValidRequestInput(Request))
	{
		SendRejectedResult(Request, EW11AbilityRejectionReason::InvalidInput);
		return;
	}

	AW11GameMode* GameMode = GetWorld()->GetAuthGameMode<AW11GameMode>();
	const AW11GameState* GameState = GetWorld()->GetGameState<AW11GameState>();
	if (!GameMode || !GameState || Request.EncounterInstanceId != GameState->GetEncounterInstanceId())
	{
		SendRejectedResult(Request, EW11AbilityRejectionReason::StaleEncounter);
		return;
	}
	if (!GameMode->IsCombatRequestAllowed(Request.EncounterInstanceId))
	{
		SendRejectedResult(Request, EW11AbilityRejectionReason::InvalidPhase);
		return;
	}

	UW11AttributeComponent* Attributes = ResolveOwnerAttributes();
	if (!Attributes || Attributes->IsDefeated())
	{
		SendRejectedResult(Request, EW11AbilityRejectionReason::Defeated);
		return;
	}
	UW11AbilityDefinition* Ability = ResolveSlottedAbility(Request.Slot);
	if (!Ability || Ability->DefinitionId.IsNone()
		|| (Request.Slot == EW11AbilitySlot::BasicAttack
			&& Ability->LogicalSlot != EW11AbilitySlot::BasicAttack))
	{
		SendRejectedResult(Request, EW11AbilityRejectionReason::UnslottedAbility);
		return;
	}

	const FW11StatBlock& Stats = Attributes->GetStats();
	const float Cooldown = CalculateAbilityCooldown(
		Ability->Cooldown, Ability->CooldownScaling, Stats);
	const float Now = GetWorld()->GetTimeSeconds();
	const float ExistingCooldownEnd = GetAbilityCooldownEndTime(Request.Slot);
	if (Now + UE_KINDA_SMALL_NUMBER < ExistingCooldownEnd)
	{
		SendRejectedResult(Request, EW11AbilityRejectionReason::Cooldown, ExistingCooldownEnd);
		return;
	}
	if (Ability->ManaCost > Attributes->GetMana() + UE_KINDA_SMALL_NUMBER)
	{
		SendRejectedResult(Request, EW11AbilityRejectionReason::InsufficientMana, ExistingCooldownEnd);
		return;
	}

	const FVector OwnerLocation = GetOwner()->GetActorLocation();
	if (Ability->TargetMode == EW11AbilityTargetMode::TargetPoint
		&& FVector::DistSquared2D(OwnerLocation, Request.TargetPoint)
		> FMath::Square(Ability->BaseRange * Stats.AttackRangeScale + 1.0f))
	{
		SendRejectedResult(Request, EW11AbilityRejectionReason::InvalidInput);
		return;
	}

	TArray<AW11Enemy*> Targets = FindStableTargets(
		Ability, Request.InputDirection, Request.TargetPoint, Request.EncounterInstanceId);
	if (Ability->TargetMode == EW11AbilityTargetMode::AutoTarget && Targets.IsEmpty())
	{
		SendRejectedResult(Request, EW11AbilityRejectionReason::NoLegalTarget);
		return;
	}
	if (!Attributes->SpendMana(Ability->ManaCost))
	{
		SendRejectedResult(Request, EW11AbilityRejectionReason::InsufficientMana);
		return;
	}
	GameMode->RecordAbilityUse(Ability->DefinitionId, Ability->ManaCost);

	AuthorityExecutionSequence = AdvanceSequence(AuthorityExecutionSequence);
	LastAttackDirection = Request.InputDirection.GetSafeNormal();
	const float CooldownEnd = Now + FMath::Max(0.0f, Cooldown);
	if (Request.Slot == EW11AbilitySlot::BasicAttack)
	{
		BasicAttackCooldownEndTime = CooldownEnd;
		OnRep_BasicAttackCooldown();
		AttackSequence = AdvanceSequence(AttackSequence);
	}
	else
	{
		const int32 ActiveIndex = static_cast<int32>(Request.Slot) - static_cast<int32>(EW11AbilitySlot::Active1);
		if (ActiveAbilityCooldownEndTimes.Num() != 4)
		{
			ActiveAbilityCooldownEndTimes.Init(0.0f, 4);
		}
		ActiveAbilityCooldownEndTimes[ActiveIndex] = CooldownEnd;
		ActiveAbilityCooldownEndTime = ActiveAbilityCooldownEndTimes[0];
		OnRep_ActiveAbilityCooldowns();
		LastExecutedAbilityId = Ability->DefinitionId;
		AbilitySequence = AdvanceSequence(AbilitySequence);
	}

	FW11AbilityExecutionResult Result;
	Result.EncounterInstanceId = Request.EncounterInstanceId;
	Result.ClientRequestSequence = Request.ClientRequestSequence;
	Result.AuthorityExecutionSequence = AuthorityExecutionSequence;
	Result.bAccepted = true;
	Result.ServerCooldownEndTime = CooldownEnd;
	Result.Source = GetOwner();
	Result.AbilityId = Ability->DefinitionId;
	Result.Slot = Request.Slot;

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const AW11PlayerState* OwnerPlayerState = OwnerPawn ? OwnerPawn->GetPlayerState<AW11PlayerState>() : nullptr;
	const int32 AbilityLevel = Request.Slot == EW11AbilitySlot::BasicAttack || !OwnerPlayerState
		? 1 : FMath::Max(1, OwnerPlayerState->GetActiveAbilityLevel(Request.Slot));
	const float UpgradeScale = 1.0f + 0.15f * FMath::Max(0, AbilityLevel - 1);
	const bool bExecutionCritical = Ability->CriticalRollPolicy == EW11CriticalRollPolicy::PerExecution
		&& RollCritical(Ability, Stats, AuthorityExecutionSequence, 0);
	for (AW11Enemy* Enemy : Targets)
	{
		if (!Enemy || !Enemy->GetAttributeComponent() || Enemy->GetAttributeComponent()->IsDefeated())
		{
			continue;
		}
		const bool bCritical = bExecutionCritical
			|| (Ability->CriticalRollPolicy == EW11CriticalRollPolicy::PerTarget
				&& RollCritical(Ability, Stats, AuthorityExecutionSequence, Enemy->GetStableSpawnId()));
		FW11DamageSpec DamageSpec;
		DamageSpec.Source = GetOwner();
		DamageSpec.AbilityId = Ability->DefinitionId;
		DamageSpec.RawDamage = Ability->BaseDamage * Ability->PowerCoefficient * Stats.Power * UpgradeScale
			* (bCritical ? Stats.CritMultiplier : 1.0f);
		DamageSpec.bCritical = bCritical;
		DamageSpec.ImpactLocation = Enemy->GetActorLocation();
		DamageSpec.ImpactDirection = FVector(LastAttackDirection.Y, LastAttackDirection.X, 0.0f);
		const EW11EnemyCombatState EnemyStateBeforeDamage = Enemy->GetAttackState().State;

		FW11EffectResult& Effect = Result.Effects.AddDefaulted_GetRef();
		Effect.Type = EW11EffectResultType::Damage;
		Effect.Source = GetOwner();
		Effect.Target = Enemy;
		Effect.AbilityId = Ability->DefinitionId;
		Effect.StableTargetId = Enemy->GetStableSpawnId();
		Effect.RequestedAmount = DamageSpec.RawDamage;
		Effect.Damage = Enemy->GetAttributeComponent()->ApplyDamageSpec(DamageSpec);
		Effect.ActualAmount = Effect.Damage.HealthDamage;
		if (AW11GameMode::IsRecoveryPunishHit(EnemyStateBeforeDamage, Effect.Damage.HealthDamage))
		{
			GameMode->RecordRecoveryPunishHit(
				Enemy->GetEnemyDefinitionId(), Enemy->GetStableSpawnId(),
				Ability->DefinitionId, Effect.Damage.HealthDamage);
		}
		Effect.bCritical = bCritical;
		Effect.ImpactLocation = DamageSpec.ImpactLocation;
		Effect.ImpactDirection = DamageSpec.ImpactDirection;

		TArray<FW11StatusEffectSpec> StatusSpecs = Ability->StatusEffects;
		const UW11RunEconomyComponent* Economy = OwnerPlayerState ? OwnerPlayerState->GetEconomyComponent() : nullptr;
		if (Request.Slot != EW11AbilitySlot::BasicAttack && Economy
			&& Economy->HasBehavior(TEXT("ManualBehavior.ActiveBurn")))
		{
			FW11StatusEffectSpec Burn;
			Burn.StatusId = TEXT("Status.Burn.ManualThunder");
			Burn.Type = EW11StatusType::Burn;
			Burn.DurationSeconds = 4.0f;
			Burn.Magnitude = 3.0f * UpgradeScale;
			Burn.TickIntervalSeconds = 1.0f;
			Burn.MaxStacks = 3;
			Burn.StackingRule = EW11StatusStackingRule::AddStackAndRefresh;
			StatusSpecs.Add(Burn);
		}
		for (const FW11StatusEffectSpec& StatusSpec : StatusSpecs)
		{
			FW11ActiveStatus Applied;
			bool bRefreshed = false;
			if (Enemy->GetAttributeComponent()->ApplyStatusEffect(
				StatusSpec, GetOwner(), Ability->DefinitionId, Applied, bRefreshed, Enemy->IsBoss()))
			{
				FW11EffectResult& StatusResult = Result.Effects.AddDefaulted_GetRef();
				StatusResult.Type = EW11EffectResultType::Status;
				StatusResult.Source = GetOwner();
				StatusResult.Target = Enemy;
				StatusResult.AbilityId = Ability->DefinitionId;
				StatusResult.StableTargetId = Enemy->GetStableSpawnId();
				StatusResult.RequestedAmount = StatusSpec.DurationSeconds;
				StatusResult.ActualAmount = Applied.Magnitude;
				StatusResult.StatusId = Applied.StatusId;
				StatusResult.StatusType = Applied.Type;
				StatusResult.StatusStacks = Applied.Stacks;
				StatusResult.StatusEndServerTime = Applied.EndServerTime;
				StatusResult.bStatusRefreshed = bRefreshed;
				StatusResult.ImpactLocation = Enemy->GetActorLocation();
				UE_LOG(LogWorldWalkerW11, Display,
					TEXT("W11_STATUS_APPLIED Target=%s Status=%s Type=%d Stacks=%d End=%.3f Ability=%s"),
					*GetNameSafe(Enemy), *Applied.StatusId.ToString(), static_cast<int32>(Applied.Type),
					Applied.Stacks, Applied.EndServerTime, *Ability->DefinitionId.ToString());
			}
		}
	}

	if (Ability->Archetype == EW11AbilityArchetype::SelfBarrier)
	{
		const float Requested = Ability->BarrierAmount * Stats.Power * UpgradeScale;
		Result.Effects.Add(BuildNonDamageEffectResult(
			EW11EffectResultType::Barrier, GetOwner(), GetOwner(), Ability->DefinitionId,
			Requested, Attributes->AddBarrier(Requested)));
	}
	else if (Ability->Archetype == EW11AbilityArchetype::SelfHeal)
	{
		const float Requested = Ability->HealAmount * Stats.Power * UpgradeScale;
		Result.Effects.Add(BuildNonDamageEffectResult(
			EW11EffectResultType::Heal, GetOwner(), GetOwner(), Ability->DefinitionId,
			Requested, Attributes->Heal(Requested)));
	}
	if (Ability->ManaRestoreAmount > 0.0f)
	{
		const float Requested = Ability->ManaRestoreAmount * UpgradeScale;
		Result.Effects.Add(BuildNonDamageEffectResult(
			EW11EffectResultType::Mana, GetOwner(), GetOwner(), Ability->DefinitionId,
			Requested, Attributes->RestoreMana(Requested)));
	}

	GameMode->RecordAbilityExecutionResult(Result, Targets.Num());
	BroadcastAcceptedResult(Result);
	GetOwner()->ForceNetUpdate();
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_ABILITY_EXECUTED EncounterInstance=%d Execution=%d Request=%d Slot=%d Ability=%s Targets=%d Effects=%d ManaCost=%.1f CooldownEnd=%.3f"),
		Result.EncounterInstanceId, Result.AuthorityExecutionSequence, Result.ClientRequestSequence,
		static_cast<int32>(Result.Slot), *Result.AbilityId.ToString(), Targets.Num(), Result.Effects.Num(),
		Ability->ManaCost, CooldownEnd);
}

void UW11CombatComponent::SendRejectedResult(
	const FW11AbilityRequest& Request,
	const EW11AbilityRejectionReason Reason,
	const float CooldownEndTime)
{
	FName AbilityId = NAME_None;
	if (UW11AbilityDefinition* Ability = ResolveSlottedAbility(Request.Slot))
	{
		AbilityId = Ability->DefinitionId;
	}
	const FW11AbilityExecutionResult Result = BuildRejectedAbilityResult(
		Request, Reason, CooldownEndTime, AuthorityExecutionSequence, GetOwner(), AbilityId);
	ClientReceiveAbilityResult(Result);
	if (AW11GameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AW11GameMode>() : nullptr)
	{
		GameMode->RecordAbilityFailure(Reason);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("W11NetworkSmoke")))
	{
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_ABILITY_REJECTED EncounterInstance=%d Request=%d Slot=%d Reason=%d CooldownEnd=%.3f"),
			Request.EncounterInstanceId, Request.ClientRequestSequence, static_cast<int32>(Request.Slot),
			static_cast<int32>(Reason), CooldownEndTime);
	}
	else
	{
		UE_LOG(LogWorldWalkerW11, Verbose,
			TEXT("W11_ABILITY_REJECTED EncounterInstance=%d Request=%d Slot=%d Reason=%d CooldownEnd=%.3f"),
			Request.EncounterInstanceId, Request.ClientRequestSequence, static_cast<int32>(Request.Slot),
			static_cast<int32>(Reason), CooldownEndTime);
	}
}

UW11AbilityDefinition* UW11CombatComponent::ResolveSlottedAbility(const EW11AbilitySlot Slot) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const AW11PlayerState* PlayerState = OwnerPawn ? OwnerPawn->GetPlayerState<AW11PlayerState>() : nullptr;
	const AW11GameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AW11GameMode>() : nullptr;
	if (!GameMode)
	{
		return nullptr;
	}
	if (Slot == EW11AbilitySlot::BasicAttack)
	{
		return GameMode->GetBasicAttackDefinition();
	}
	return PlayerState ? GameMode->FindAbilityDefinition(PlayerState->GetActiveAbilityId(Slot)) : nullptr;
}

UW11AttributeComponent* UW11CombatComponent::ResolveOwnerAttributes() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const AW11PlayerState* PlayerState = Pawn ? Pawn->GetPlayerState<AW11PlayerState>() : nullptr;
	return PlayerState ? PlayerState->GetAttributeComponent() : nullptr;
}

TArray<AW11Enemy*> UW11CombatComponent::FindStableTargets(
	const UW11AbilityDefinition* Ability,
	const FVector2D& Direction,
	const FVector& TargetPoint,
	const int32 EncounterInstanceId) const
{
	TArray<AW11Enemy*> Result;
	if (!Ability || !GetWorld() || !GetOwner()
		|| Ability->TargetMode == EW11AbilityTargetMode::Self
		|| Ability->BaseDamage <= 0.0f)
	{
		return Result;
	}

	const UW11AttributeComponent* Attributes = ResolveOwnerAttributes();
	const FW11StatBlock Stats = Attributes ? Attributes->GetStats() : FW11StatBlock();
	const FVector Start = GetOwner()->GetActorLocation();
	const FVector WorldDirection(Direction.Y, Direction.X, 0.0f);
	const float Range = Ability->BaseRange * Stats.AttackRangeScale;
	const float Radius = UW11StatLibrary::GetFinalAreaRadius(Ability->BaseRadius, Stats.AreaScale);
	FVector QueryStart = Start;
	FVector QueryEnd = Start;
	if (Ability->TargetMode == EW11AbilityTargetMode::TargetPoint)
	{
		QueryStart = TargetPoint;
		QueryEnd = TargetPoint;
	}
	else if (Ability->HitShape == EW11HitShape::Line
		|| Ability->HitShape == EW11HitShape::Projectile
		|| Ability->Archetype == EW11AbilityArchetype::DirectedSweep
		|| Ability->Archetype == EW11AbilityArchetype::PiercingLine)
	{
		QueryEnd = Start + WorldDirection * Range;
	}

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(W11AbilityQuery), false, GetOwner());
	GetWorld()->SweepMultiByObjectType(
		Hits, QueryStart, QueryEnd, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(FMath::Max(1.0f, Radius)), Params);
	TSet<AW11Enemy*> Unique;
	for (const FHitResult& Hit : Hits)
	{
		AW11Enemy* Enemy = Cast<AW11Enemy>(Hit.GetActor());
		if (!Enemy || Unique.Contains(Enemy) || Enemy->GetEncounterInstanceId() != EncounterInstanceId
			|| !Enemy->GetAttributeComponent() || Enemy->GetAttributeComponent()->IsDefeated())
		{
			continue;
		}
		if (Ability->HitShape == EW11HitShape::Cone)
		{
			const FVector ToEnemy = (Enemy->GetActorLocation() - Start).GetSafeNormal2D();
			const float MinimumDot = FMath::Cos(FMath::DegreesToRadians(Ability->ConeAngleDegrees * 0.5f));
			if (FVector::DotProduct(WorldDirection, ToEnemy) < MinimumDot)
			{
				continue;
			}
		}
		Unique.Add(Enemy);
		Result.Add(Enemy);
	}

	// Basic attacks are short, immediate sweeps. A physics-only multi-sweep can
	// intermittently miss an enemy at a capsule edge while collision state is
	// being updated. Re-evaluate live encounter enemies using the same authored
	// range/direction plus capsule-aware lateral forgiveness. The server remains
	// authoritative and targets behind or outside the local attack reach fail.
	if (Ability->LogicalSlot == EW11AbilitySlot::BasicAttack)
	{
		for (TActorIterator<AW11Enemy> It(GetWorld()); It; ++It)
		{
			AW11Enemy* Enemy = *It;
			if (!Enemy || Unique.Contains(Enemy)
				|| Enemy->GetEncounterInstanceId() != EncounterInstanceId
				|| !Enemy->GetAttributeComponent()
				|| Enemy->GetAttributeComponent()->IsDefeated())
			{
				continue;
			}
			const UCapsuleComponent* Capsule = Enemy->GetCapsuleComponent();
			const float CapsuleRadius = Capsule ? Capsule->GetScaledCapsuleRadius() : 0.0f;
			if (IsBasicAttackTargetInForgivingSweep(
				Start, WorldDirection, Enemy->GetActorLocation(), Range, Radius, CapsuleRadius))
			{
				Unique.Add(Enemy);
				Result.Add(Enemy);
			}
		}
	}
	Result.Sort([Start](const AW11Enemy& Left, const AW11Enemy& Right)
	{
		const float LeftDistance = FVector::DistSquared2D(Start, Left.GetActorLocation());
		const float RightDistance = FVector::DistSquared2D(Start, Right.GetActorLocation());
		return IsStableTargetBefore(
			LeftDistance, Left.GetStableSpawnId(), RightDistance, Right.GetStableSpawnId());
	});
	Result.SetNum(FMath::Min(Result.Num(), FMath::Max(1, Ability->MaxTargets)));
	return Result;
}

float UW11CombatComponent::CalculateAbilityCooldown(
	const float BaseCooldown,
	const EW11CooldownScaling Scaling,
	const FW11StatBlock& Stats)
{
	switch (Scaling)
	{
	case EW11CooldownScaling::AttackSpeed:
		return BaseCooldown / FMath::Max(0.1f, Stats.AttackSpeedScale);
	case EW11CooldownScaling::AbilityHaste:
		return UW11StatLibrary::GetFinalCooldown(BaseCooldown, Stats.AbilityHaste);
	case EW11CooldownScaling::None:
	default:
		return BaseCooldown;
	}
}

bool UW11CombatComponent::IsStableTargetBefore(
	const float LeftDistanceSquared,
	const int32 LeftStableId,
	const float RightDistanceSquared,
	const int32 RightStableId)
{
	return FMath::IsNearlyEqual(LeftDistanceSquared, RightDistanceSquared)
		? LeftStableId < RightStableId
		: LeftDistanceSquared < RightDistanceSquared;
}

bool UW11CombatComponent::IsBasicAttackTargetInForgivingSweep(
	const FVector& Start,
	const FVector& WorldDirection,
	const FVector& TargetLocation,
	const float Range,
	const float Radius,
	const float TargetCapsuleRadius)
{
	const FVector Forward = FVector(WorldDirection.X, WorldDirection.Y, 0.0f).GetSafeNormal();
	if (Forward.IsNearlyZero() || Range <= 0.0f || Radius <= 0.0f)
	{
		return false;
	}
	const FVector Offset(TargetLocation.X - Start.X, TargetLocation.Y - Start.Y, 0.0f);
	const float Along = FVector::DotProduct(Offset, Forward);
	const float SafeCapsuleRadius = FMath::Max(0.0f, TargetCapsuleRadius);
	if (Along < 0.0f || Along > Range + SafeCapsuleRadius)
	{
		return false;
	}
	const float LateralSquared = FMath::Max(0.0f, Offset.SizeSquared2D() - FMath::Square(Along));
	const float AllowedLateral = Radius + SafeCapsuleRadius + Along * 0.15f;
	return LateralSquared <= FMath::Square(AllowedLateral);
}

FW11AbilityExecutionResult UW11CombatComponent::BuildRejectedAbilityResult(
	const FW11AbilityRequest& Request,
	const EW11AbilityRejectionReason Reason,
	const float CooldownEndTime,
	const int32 InAuthorityExecutionSequence,
	AActor* Source,
	const FName AbilityId)
{
	FW11AbilityExecutionResult Result;
	Result.EncounterInstanceId = Request.EncounterInstanceId;
	Result.ClientRequestSequence = Request.ClientRequestSequence;
	Result.AuthorityExecutionSequence = InAuthorityExecutionSequence;
	Result.bAccepted = false;
	Result.RejectionReason = Reason;
	Result.ServerCooldownEndTime = CooldownEndTime;
	Result.Source = Source;
	Result.AbilityId = AbilityId;
	Result.Slot = Request.Slot;
	return Result;
}

bool UW11CombatComponent::RollCritical(
	const UW11AbilityDefinition* Ability,
	const FW11StatBlock& Stats,
	const int32 ExecutionSequence,
	const int32 TargetStableId) const
{
	if (!Ability || Ability->CriticalRollPolicy == EW11CriticalRollPolicy::Never)
	{
		return false;
	}
	const AW11GameState* GameState = GetWorld() ? GetWorld()->GetGameState<AW11GameState>() : nullptr;
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const APlayerState* OwnerPlayerState = OwnerPawn ? OwnerPawn->GetPlayerState() : nullptr;
	uint32 Seed = HashCombineFast(
		GetTypeHash(GameState ? GameState->GetRunSeed() : 0),
		GetTypeHash(GameState ? GameState->GetEncounterInstanceId() : 0));
	Seed = HashCombineFast(Seed, GetTypeHash(OwnerPlayerState ? OwnerPlayerState->GetPlayerId() : 0));
	Seed = HashCombineFast(Seed, GetTypeHash(ExecutionSequence));
	Seed = HashCombineFast(Seed, GetTypeHash(TargetStableId));
	FRandomStream Stream(static_cast<int32>(Seed));
	return Stream.FRand() < Stats.CritChance;
}

void UW11CombatComponent::BroadcastAcceptedResult(const FW11AbilityExecutionResult& Result)
{
	ClientReceiveAbilityResult(Result);
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const APlayerState* OwnerPlayerState = OwnerPawn ? OwnerPawn->GetPlayerState() : nullptr;
	const int32 SourceStableId = OwnerPlayerState ? OwnerPlayerState->GetPlayerId() : 0;
	for (int32 Index = 0; Index < Result.Effects.Num(); ++Index)
	{
		const FW11EffectResult& Effect = Result.Effects[Index];
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_ABILITY_EFFECT EncounterInstance=%d Execution=%d Effect=%d Type=%d Ability=%s Source=%s Target=%s StableTarget=%d Requested=%.1f Actual=%.1f Raw=%.1f Armor=%.1f Barrier=%.1f Health=%.1f Critical=%d Immune=%d Defeated=%d Status=%s StatusType=%d Stacks=%d Refreshed=%d"),
			Result.EncounterInstanceId, Result.AuthorityExecutionSequence, Index + 1,
			static_cast<int32>(Effect.Type), *Effect.AbilityId.ToString(), *GetNameSafe(Effect.Source),
			*GetNameSafe(Effect.Target), Effect.StableTargetId, Effect.RequestedAmount,
			Effect.ActualAmount, Effect.Damage.RawDamage, Effect.Damage.ArmorMitigated,
			Effect.Damage.BarrierAbsorbed, Effect.Damage.HealthDamage, Effect.bCritical ? 1 : 0,
			Effect.Damage.bImmune ? 1 : 0, Effect.Damage.bDefeated ? 1 : 0,
			*Effect.StatusId.ToString(), static_cast<int32>(Effect.StatusType), Effect.StatusStacks,
			Effect.bStatusRefreshed ? 1 : 0);
		FW11CombatCue Cue;
		Cue.EncounterInstanceId = Result.EncounterInstanceId;
		Cue.SourceStableId = SourceStableId;
		Cue.AuthorityExecutionSequence = Result.AuthorityExecutionSequence;
		Cue.EffectSequence = Index + 1;
		Cue.Source = Effect.Source;
		Cue.Target = Effect.Target;
		Cue.AbilityId = Effect.AbilityId;
		Cue.Location = Effect.ImpactLocation;
		Cue.Direction = Effect.ImpactDirection;
		Cue.PrimaryAmount = Effect.ActualAmount;
		Cue.SecondaryAmount = Effect.Type == EW11EffectResultType::Damage
			? Effect.Damage.BarrierAbsorbed : 0.0f;
		Cue.bCritical = Effect.bCritical;
		if (Effect.Type == EW11EffectResultType::Damage)
		{
			Cue.Type = Effect.Damage.bImmune ? EW11CombatCueType::Immune
				: Effect.Damage.bDefeated ? EW11CombatCueType::Defeated : EW11CombatCueType::Damage;
		}
		else if (Effect.Type == EW11EffectResultType::Heal)
		{
			Cue.Type = EW11CombatCueType::Heal;
		}
		else if (Effect.Type == EW11EffectResultType::Barrier)
		{
			Cue.Type = EW11CombatCueType::Barrier;
		}
		else
		{
			// Mana/status facts are carried by the execution result and replicated state;
			// they must not fall through as the enum's default AbilityStarted cue.
			continue;
		}
		MulticastCombatCue(Cue);
	}
}

void UW11CombatComponent::ClientReceiveAbilityResult_Implementation(const FW11AbilityExecutionResult Result)
{
	OnAbilityResult.Broadcast(Result);
}

void UW11CombatComponent::MulticastCombatCue_Implementation(const FW11CombatCue Cue)
{
	OnCombatCue.Broadcast(Cue);
}

void UW11CombatComponent::OnRep_BasicAttackCooldown()
{
	OnCooldownChanged.Broadcast(EW11AbilitySlot::BasicAttack, BasicAttackCooldownEndTime);
}

void UW11CombatComponent::OnRep_ActiveAbilityCooldowns()
{
	ActiveAbilityCooldownEndTime = ActiveAbilityCooldownEndTimes.IsValidIndex(0)
		? ActiveAbilityCooldownEndTimes[0] : 0.0f;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		OnCooldownChanged.Broadcast(
			static_cast<EW11AbilitySlot>(static_cast<int32>(EW11AbilitySlot::Active1) + Index),
			ActiveAbilityCooldownEndTimes.IsValidIndex(Index) ? ActiveAbilityCooldownEndTimes[Index] : 0.0f);
	}
}
