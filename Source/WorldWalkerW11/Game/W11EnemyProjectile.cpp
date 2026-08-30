#include "Game/W11EnemyProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/W11AttributeComponent.h"
#include "Data/W11Definitions.h"
#include "Game/W11Character.h"
#include "Game/W11Enemy.h"
#include "Game/W11GameMode.h"
#include "Game/W11GameState.h"
#include "Game/W11PlayerState.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "WorldWalkerW11.h"

AW11EnemyProjectile::AW11EnemyProjectile()
{
	bReplicates = true;
	SetReplicateMovement(true);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.02f;
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("W11ProjectileCollision"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	VisualComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("W11ProjectileVisual"));
	VisualComponent->SetupAttachment(CollisionComponent);
	VisualComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualComponent->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (SphereMesh.Succeeded())
	{
		VisualComponent->SetStaticMesh(SphereMesh.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		VisualComponent->SetMaterial(0, ShapeMaterial.Object);
	}
}

void AW11EnemyProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AW11EnemyProjectile, SourceEnemy);
	DOREPLIFETIME(AW11EnemyProjectile, EncounterInstanceId);
	DOREPLIFETIME(AW11EnemyProjectile, AttackInstanceId);
	DOREPLIFETIME(AW11EnemyProjectile, AbilityId);
	DOREPLIFETIME(AW11EnemyProjectile, TravelDirection);
}

void AW11EnemyProjectile::InitializeProjectile(
	AW11Enemy* InSource,
	const UW11AbilityDefinition* Ability,
	const FVector& Direction,
	const int32 InEncounterInstanceId,
	const int32 InAttackInstanceId)
{
	if (!HasAuthority() || !InSource || !Ability)
	{
		return;
	}
	SourceEnemy = InSource;
	EncounterInstanceId = InEncounterInstanceId;
	AttackInstanceId = InAttackInstanceId;
	AbilityId = Ability->DefinitionId;
	TravelDirection = FVector(Direction.X, Direction.Y, 0.0f).GetSafeNormal();
	Speed = FMath::Max(1.0f, Ability->ProjectileSpeed);
	Radius = FMath::Max(4.0f, Ability->BaseRadius);
	RawDamage = Ability->BaseDamage * Ability->PowerCoefficient
		* InSource->GetAttributeComponent()->GetStats().Power;
	StatusEffects = Ability->StatusEffects;
	CollisionComponent->SetSphereRadius(Radius);
	VisualComponent->SetRelativeScale3D(FVector(Radius / 50.0f));
	SetLifeSpan(FMath::Max(0.05f, Ability->ProjectileLifetime));
	ForceNetUpdate();
}

void AW11EnemyProjectile::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || bResolved)
	{
		return;
	}
	if (!IsEncounterCurrent() || !SourceEnemy || SourceEnemy->GetAttributeComponent()->IsDefeated())
	{
		if (AW11GameMode* GameMode = GetWorld()->GetAuthGameMode<AW11GameMode>())
		{
			GameMode->RecordCancelledThreat();
		}
		Destroy();
		return;
	}
	const FVector Start = GetActorLocation();
	const FVector End = Start + FVector(TravelDirection) * Speed * DeltaSeconds;
	if (!TryHitAlongSegment(Start, End))
	{
		SetActorLocation(End, false, nullptr, ETeleportType::None);
	}
}

void AW11EnemyProjectile::LifeSpanExpired()
{
	if (HasAuthority() && !bResolved && IsEncounterCurrent() && SourceEnemy
		&& !SourceEnemy->GetAttributeComponent()->IsDefeated()
		&& TryClaimResolution(bResolved))
	{
		if (AW11GameMode* GameMode = GetWorld()->GetAuthGameMode<AW11GameMode>())
		{
			GameMode->RecordEnemyAttackEvaded(SourceEnemy->GetEnemyDefinitionId());
		}
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_ENEMY_PROJECTILE_EVADED EncounterInstance=%d AttackInstance=%d Ability=%s"),
			EncounterInstanceId, AttackInstanceId, *AbilityId.ToString());
	}
	Super::LifeSpanExpired();
}

bool AW11EnemyProjectile::IsEncounterCurrent() const
{
	const AW11GameState* State = GetWorld() ? GetWorld()->GetGameState<AW11GameState>() : nullptr;
	return State && AW11GameState::IsPendingCombatEncounter(
		State->GetRunPhase(), State->GetCombatOutcome(),
		State->GetEncounterInstanceId(), EncounterInstanceId);
}

bool AW11EnemyProjectile::TryHitAlongSegment(const FVector& Start, const FVector& End)
{
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(W11EnemyProjectile), false, this);
	Params.AddIgnoredActor(SourceEnemy);
	GetWorld()->SweepMultiByObjectType(
		Hits, Start, End, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Radius), Params);
	Hits.Sort([](const FHitResult& Left, const FHitResult& Right)
	{
		return Left.Distance < Right.Distance;
	});
	for (const FHitResult& Hit : Hits)
	{
		AW11Character* Target = Cast<AW11Character>(Hit.GetActor());
		AW11PlayerState* PlayerState = Target ? Target->GetPlayerState<AW11PlayerState>() : nullptr;
		UW11AttributeComponent* Attributes = PlayerState ? PlayerState->GetAttributeComponent() : nullptr;
		if (!Target || !Attributes || Attributes->IsDefeated())
		{
			continue;
		}
		if (!TryClaimResolution(bResolved))
		{
			return true;
		}
		FW11DamageSpec Spec;
		Spec.Source = SourceEnemy;
		Spec.AbilityId = AbilityId;
		Spec.RawDamage = RawDamage;
		const FVector ImpactLocation = Hit.ImpactPoint.IsNearlyZero()
			? Target->GetActorLocation() : FVector(Hit.ImpactPoint);
		Spec.ImpactLocation = ImpactLocation;
		Spec.ImpactDirection = TravelDirection;
		if (AW11GameMode* GameMode = GetWorld()->GetAuthGameMode<AW11GameMode>())
		{
			GameMode->RecordEnemyHit(SourceEnemy->GetEnemyDefinitionId());
		}
		const FW11DamageBreakdown Damage = Attributes->ApplyDamageSpec(Spec);
		for (const FW11StatusEffectSpec& StatusSpec : StatusEffects)
		{
			FW11ActiveStatus Applied;
			bool bRefreshed = false;
			if (Attributes->ApplyStatusEffect(StatusSpec, SourceEnemy, AbilityId, Applied, bRefreshed))
			{
				UE_LOG(LogWorldWalkerW11, Display,
					TEXT("W11_STATUS_APPLIED Target=%s Status=%s Type=%d Stacks=%d End=%.3f Ability=%s"),
					*GetNameSafe(Target), *Applied.StatusId.ToString(), static_cast<int32>(Applied.Type),
					Applied.Stacks, Applied.EndServerTime, *AbilityId.ToString());
			}
		}
		FW11CombatCue Cue;
		Cue.EncounterInstanceId = EncounterInstanceId;
		Cue.SourceStableId = SourceEnemy->GetStableSpawnId();
		Cue.AuthorityExecutionSequence = AttackInstanceId;
		Cue.EffectSequence = PlayerState->GetPlayerId();
		Cue.Source = SourceEnemy;
		Cue.Target = Target;
		Cue.AbilityId = AbilityId;
		Cue.Location = ImpactLocation;
		Cue.Direction = TravelDirection;
		Cue.PrimaryAmount = Damage.HealthDamage;
		Cue.SecondaryAmount = Damage.BarrierAbsorbed;
		Cue.Type = Damage.bImmune ? EW11CombatCueType::Immune
			: Damage.bDefeated ? EW11CombatCueType::Defeated : EW11CombatCueType::Damage;
		SourceEnemy->BroadcastCombatCue(Cue);
		Target->SendIncomingCombatCue(Cue);
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_ENEMY_PROJECTILE_HIT EncounterInstance=%d AttackInstance=%d Ability=%s Target=%d Raw=%.1f Barrier=%.1f Health=%.1f Immune=%d"),
			EncounterInstanceId, AttackInstanceId, *AbilityId.ToString(), PlayerState->GetPlayerId(),
			Damage.RawDamage, Damage.BarrierAbsorbed, Damage.HealthDamage, Damage.bImmune ? 1 : 0);
		Destroy();
		return true;
	}
	return false;
}

bool AW11EnemyProjectile::TryClaimResolution(bool& bInOutResolved)
{
	if (bInOutResolved)
	{
		return false;
	}
	bInOutResolved = true;
	return true;
}
