#include "Game/W11Enemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/W11AttributeComponent.h"
#include "Core/W11ArenaBounds.h"
#include "Data/W11Definitions.h"
#include "Game/W11GameMode.h"
#include "Game/W11GameState.h"
#include "Game/W11EnemyController.h"
#include "Game/W11Character.h"
#include "Game/W11CombatCueActor.h"
#include "Game/W11RunDirector.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "WorldWalkerW11.h"

AW11Enemy::AW11Enemy()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(20.0f);
	SetMinNetUpdateFrequency(5.0f);
	AIControllerClass = AW11EnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCharacterMovement()->DefaultLandMovementMode = MOVE_Flying;
	GetCharacterMovement()->GravityScale = 0.0f;
	GetCharacterMovement()->MaxFlySpeed = 260.0f;
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector::UpVector);

	AttributeComponent = CreateDefaultSubobject<UW11AttributeComponent>(TEXT("W11EnemyAttributes"));
	FlipbookComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("W11EnemyFlipbook"));
	FlipbookComponent->SetupAttachment(GetCapsuleComponent());
	FlipbookComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FlipbookComponent->SetIsReplicated(false);
	FlipbookComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, -90.0f));
	FlipbookComponent->SetRelativeScale3D(FVector(0.42f));
	FlipbookComponent->SetTranslucentSortPriority(15);
	FlipbookComponent->SetCastShadow(false);
	FlipbookComponent->SetReceivesDecals(false);
	PrototypeSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("W11PrototypeSprite"));
	PrototypeSpriteComponent->SetupAttachment(GetCapsuleComponent());
	PrototypeSpriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PrototypeSpriteComponent->SetIsReplicated(false);
	PrototypeSpriteComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, -90.0f));
	PrototypeSpriteComponent->SetRelativeScale3D(FVector(0.42f));
	PrototypeSpriteComponent->SetTranslucentSortPriority(15);
	PrototypeSpriteComponent->SetCastShadow(false);
	PrototypeSpriteComponent->SetReceivesDecals(false);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TranslucentSpriteMaterial(
		TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	if (TranslucentSpriteMaterial.Succeeded())
	{
		FlipbookComponent->SetMaterial(0, TranslucentSpriteMaterial.Object);
		PrototypeSpriteComponent->SetMaterial(0, TranslucentSpriteMaterial.Object);
	}

	LineTelegraphMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("W11LineTelegraph"));
	LineTelegraphMesh->SetupAttachment(GetRootComponent());
	LineTelegraphMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LineTelegraphMesh->SetHiddenInGame(true);
	LineTelegraphMesh->SetCastShadow(false);
	LineTelegraphMesh->SetReceivesDecals(false);
	LineTelegraphMesh->SetTranslucentSortPriority(20);
	CircleTelegraphMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("W11CircleTelegraph"));
	CircleTelegraphMesh->SetupAttachment(GetRootComponent());
	CircleTelegraphMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CircleTelegraphMesh->SetHiddenInGame(true);
	CircleTelegraphMesh->SetCastShadow(false);
	CircleTelegraphMesh->SetReceivesDecals(false);
	CircleTelegraphMesh->SetTranslucentSortPriority(20);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TelegraphMaterial(
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Materials/M_W11_EnemyTelegraph.M_W11_EnemyTelegraph"));
	if (CubeMesh.Succeeded())
	{
		LineTelegraphMesh->SetStaticMesh(CubeMesh.Object);
	}
	if (CylinderMesh.Succeeded())
	{
		CircleTelegraphMesh->SetStaticMesh(CylinderMesh.Object);
	}
	if (TelegraphMaterial.Succeeded())
	{
		LineTelegraphMesh->SetMaterial(0, TelegraphMaterial.Object);
		CircleTelegraphMesh->SetMaterial(0, TelegraphMaterial.Object);
	}

	AttributeComponent->OnDefeated.AddDynamic(this, &AW11Enemy::HandleDefeated);
	AttributeComponent->OnHealthChanged.AddDynamic(this, &AW11Enemy::HandleBossHealthChanged);
}

void AW11Enemy::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateMovementPresentation();
	if (!HasAuthority())
	{
		return;
	}
	const AW11GameState* State = GetWorld() ? GetWorld()->GetGameState<AW11GameState>() : nullptr;
	FW11ArenaBounds::ConstrainCharacter(
		*this,
		FW11ArenaBounds::ResolveHalfExtent(
			State ? State->GetChapterIndex() : 0,
			State ? State->GetStageIndex() : 0));
}

void AW11Enemy::BeginPlay()
{
	Super::BeginPlay();
	LineTelegraphMaterial = LineTelegraphMesh
		? LineTelegraphMesh->CreateDynamicMaterialInstance(0) : nullptr;
	CircleTelegraphMaterial = CircleTelegraphMesh
		? CircleTelegraphMesh->CreateDynamicMaterialInstance(0) : nullptr;
	RefreshTelegraphVisual();
}

void AW11Enemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AW11Enemy, EnemyDefinitionId);
	DOREPLIFETIME(AW11Enemy, EncounterInstanceId);
	DOREPLIFETIME(AW11Enemy, StableSpawnId);
	DOREPLIFETIME(AW11Enemy, bBoss);
	DOREPLIFETIME(AW11Enemy, BossPhaseIndex);
	DOREPLIFETIME(AW11Enemy, BossPhaseId);
	DOREPLIFETIME(AW11Enemy, PresentationSprite);
	DOREPLIFETIME(AW11Enemy, PresentationMoveFlipbookRight);
	DOREPLIFETIME(AW11Enemy, PresentationMoveFlipbookLeft);
	DOREPLIFETIME(AW11Enemy, AttackState);
}

float AW11Enemy::TakeDamage(
	const float DamageAmount,
	const FDamageEvent& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	return HasAuthority() && AttributeComponent
		? AttributeComponent->ApplyRawDamage(DamageAmount)
		: 0.0f;
}

void AW11Enemy::InitializeFromDefinition(
	const UW11EnemyDefinition* Definition,
	const float HealthScale,
	const float PowerScale,
	const int32 InEncounterInstanceId,
	const int32 InStableSpawnId)
{
	if (!HasAuthority() || !Definition || !AttributeComponent)
	{
		return;
	}
	EnemyDefinitionId = Definition->DefinitionId;
	EncounterInstanceId = InEncounterInstanceId;
	StableSpawnId = InStableSpawnId;
	bBoss = Definition->bBoss;
	BossPhases = Definition->BossPhases;
	BossPhaseIndex = 0;
	BossPhaseId = Definition->InitialBossPhaseId;
	PresentationSprite = Definition->IdleSprite;
	PresentationMoveFlipbookRight = Definition->MoveFlipbook;
	PresentationMoveFlipbookLeft = Definition->MoveFlipbookLeft;
	OnRep_MovementFlipbooks();
	AttackDefinitions.Reset();
	for (const TSoftObjectPtr<UW11AbilityDefinition>& AbilityAsset : Definition->Abilities)
	{
		if (UW11AbilityDefinition* Ability = AbilityAsset.LoadSynchronous())
		{
			AttackDefinitions.AddUnique(Ability);
		}
	}
	PrimaryAttackDefinition = AttackDefinitions.IsEmpty() ? nullptr : AttackDefinitions[0];
	FW11StatBlock ScaledStats = Definition->BaseStats;
	ScaledStats.MaxHealth *= FMath::Max(0.1f, HealthScale);
	ScaledStats.Power *= FMath::Max(0.0f, PowerScale);
	AttributeComponent->InitializeStats(ScaledStats, true);
	if (Definition->bFormalBoss)
	{
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_BOSS_PHASE_ENTERED EncounterInstance=%d SpawnId=%d Boss=%s PhaseIndex=0 Phase=%s Mechanic=%s HealthFraction=1.000"),
			EncounterInstanceId, StableSpawnId, *EnemyDefinitionId.ToString(),
			*Definition->InitialBossPhaseId.ToString(), *Definition->InitialBossMechanicId.ToString());
	}
	GetCharacterMovement()->MaxFlySpeed = 260.0f * ScaledStats.MoveSpeedScale;
	if (UPaperFlipbook* Flipbook = Definition->IdleFlipbook.LoadSynchronous())
	{
		FlipbookComponent->SetFlipbook(Flipbook);
	}
	OnRep_PresentationSprite();
	PrototypeSpriteComponent->SetVisibility(FlipbookComponent->GetFlipbook() == nullptr);
	ForceNetUpdate();
}

void AW11Enemy::UpdateMovementPresentation()
{
	if (!FlipbookComponent || !PrototypeSpriteComponent || bDefeatHandled)
	{
		return;
	}
	const FVector Velocity = GetVelocity();
	const bool bMoving = Velocity.SizeSquared2D() > 36.0f;
	if (!bMoving || !MoveFlipbookRight)
	{
		if (FlipbookComponent->GetFlipbook() == MoveFlipbookRight
			|| FlipbookComponent->GetFlipbook() == MoveFlipbookLeft)
		{
			FlipbookComponent->SetFlipbook(nullptr);
		}
		PrototypeSpriteComponent->SetVisibility(true);
		return;
	}
	if (FMath::Abs(Velocity.Y) > 6.0f)
	{
		bLastMovementFacingLeft = Velocity.Y < 0.0f;
	}
	UPaperFlipbook* Desired = bLastMovementFacingLeft && MoveFlipbookLeft
		? MoveFlipbookLeft : MoveFlipbookRight;
	if (FlipbookComponent->GetFlipbook() != Desired)
	{
		FlipbookComponent->SetFlipbook(Desired);
		FlipbookComponent->SetLooping(true);
		FlipbookComponent->PlayFromStart();
	}
	PrototypeSpriteComponent->SetVisibility(false);
}

void AW11Enemy::HandleBossHealthChanged(const float CurrentHealth, const float MaximumHealth)
{
	if (!HasAuthority() || !bBoss || CurrentHealth <= 0.0f || MaximumHealth <= 0.0f
		|| BossPhaseIndex >= BossPhases.Num())
	{
		return;
	}
	const float HealthFraction = FMath::Clamp(CurrentHealth / MaximumHealth, 0.0f, 1.0f);
	while (BossPhaseIndex < BossPhases.Num()
		&& HealthFraction <= BossPhases[BossPhaseIndex].EnterAtHealthFraction + UE_KINDA_SMALL_NUMBER)
	{
		const FW11BossPhaseDefinition& Phase = BossPhases[BossPhaseIndex];
		AttackDefinitions.Reset();
		for (const TSoftObjectPtr<UW11AbilityDefinition>& AbilityAsset : Phase.Abilities)
		{
			if (UW11AbilityDefinition* Ability = AbilityAsset.LoadSynchronous())
			{
				AttackDefinitions.AddUnique(Ability);
			}
		}
		PrimaryAttackDefinition = AttackDefinitions.IsEmpty() ? nullptr : AttackDefinitions[0];
		++BossPhaseIndex;
		BossPhaseId = Phase.PhaseId;
		SetAttackState(MakeInactiveAttackState(AttackState, EW11EnemyCombatState::AcquireTarget));
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_BOSS_PHASE_ENTERED EncounterInstance=%d SpawnId=%d Boss=%s PhaseIndex=%d Phase=%s Mechanic=%s HealthFraction=%.3f Abilities=%d"),
			EncounterInstanceId, StableSpawnId, *EnemyDefinitionId.ToString(), BossPhaseIndex,
			*Phase.PhaseId.ToString(), *Phase.MechanicId.ToString(), HealthFraction,
			AttackDefinitions.Num());
	}
	ForceNetUpdate();
}

const UW11AbilityDefinition* AW11Enemy::GetAttackDefinitionForSequence(const int32 AttackSequence) const
{
	return AttackDefinitions.IsEmpty() ? nullptr
		: AttackDefinitions[FMath::Abs(AttackSequence - 1) % AttackDefinitions.Num()];
}

const UW11AbilityDefinition* AW11Enemy::FindAttackDefinition(const FName AbilityId) const
{
	const TObjectPtr<UW11AbilityDefinition>* Found = AttackDefinitions.FindByPredicate(
		[AbilityId](const UW11AbilityDefinition* Ability)
		{
			return Ability && Ability->DefinitionId == AbilityId;
		});
	return Found ? Found->Get() : PrimaryAttackDefinition.Get();
}

void AW11Enemy::SetAttackState(const FW11ReplicatedAttackState& NewState)
{
	if (!HasAuthority())
	{
		return;
	}
	AttackState = NewState;
	OnRep_AttackState();
	ForceNetUpdate();
}

void AW11Enemy::OnRep_AttackState()
{
	RefreshTelegraphVisual();
	PlayAttackWarningAudio();
	if (FParse::Param(FCommandLine::Get(), TEXT("W11NetworkSmoke")))
	{
		const float ServerNow = GetWorld() && GetWorld()->GetGameState()
			? GetWorld()->GetGameState()->GetServerWorldTimeSeconds()
			: 0.0f;
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_NETWORK_ATTACK_SNAPSHOT Authority=%d SpawnId=%d AttackInstance=%d State=%d Controller=%s ServerNow=%.3f ActiveStart=%.3f EstimatedServerLead=%.3f"),
			HasAuthority() ? 1 : 0, StableSpawnId, AttackState.AttackInstanceId,
			static_cast<int32>(AttackState.State), *GetNameSafe(GetController()), ServerNow,
			AttackState.ActiveStartServerTime, AttackState.ActiveStartServerTime - ServerNow);
	}
}

void AW11Enemy::PlayAttackWarningAudio()
{
	if (!GetWorld() || GetNetMode() == NM_DedicatedServer
		|| AttackState.State != EW11EnemyCombatState::Windup
		|| AttackState.EncounterInstanceId <= 0 || AttackState.AttackInstanceId <= 0
		|| (LastWarningAudioEncounterInstanceId == AttackState.EncounterInstanceId
			&& LastWarningAudioAttackInstanceId == AttackState.AttackInstanceId))
	{
		return;
	}

	LastWarningAudioEncounterInstanceId = AttackState.EncounterInstanceId;
	LastWarningAudioAttackInstanceId = AttackState.AttackInstanceId;
	FString AbilityPrefix;
	FString AbilityKey;
	AttackState.AttackDefinitionId.ToString().Split(
		TEXT("."), &AbilityPrefix, &AbilityKey, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	const FString AbilityPath = FString::Printf(
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Abilities/Enemies/DA_Ability_%s.DA_Ability_%s"),
		*AbilityKey, *AbilityKey);
	UW11AbilityDefinition* Ability = LoadObject<UW11AbilityDefinition>(nullptr, *AbilityPath);
	UW11SkillVisualDefinition* Visual = Ability ? Ability->Visuals.LoadSynchronous() : nullptr;
	USoundBase* WarningSound = Visual ? Visual->CastSound.LoadSynchronous() : nullptr;
	if (!WarningSound)
	{
		UE_LOG(LogWorldWalkerW11, Warning,
			TEXT("W11_ENEMY_WARNING_AUDIO_MISSING SpawnId=%d AttackInstance=%d Ability=%s"),
			StableSpawnId, AttackState.AttackInstanceId, *AttackState.AttackDefinitionId.ToString());
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(this, WarningSound, GetActorLocation());
	if (FParse::Param(FCommandLine::Get(), TEXT("W11AudioSmoke")))
	{
		const float ServerNow = GetWorld()->GetGameState()
			? GetWorld()->GetGameState()->GetServerWorldTimeSeconds() : 0.0f;
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_ENEMY_WARNING_AUDIO_PLAYED Authority=%d SpawnId=%d AttackInstance=%d Ability=%s Sound=%s Lead=%.3f"),
			HasAuthority() ? 1 : 0, StableSpawnId, AttackState.AttackInstanceId,
			*AttackState.AttackDefinitionId.ToString(), *GetNameSafe(WarningSound),
			AttackState.ActiveStartServerTime - ServerNow);
	}
}

void AW11Enemy::RefreshTelegraphVisual()
{
	if (!LineTelegraphMesh || !CircleTelegraphMesh)
	{
		return;
	}
	const bool bShow = AttackState.State == EW11EnemyCombatState::Windup
		|| AttackState.State == EW11EnemyCombatState::Active;
	LineTelegraphMesh->SetHiddenInGame(!bShow
		|| (AttackState.TelegraphShape != EW11TelegraphShape::Line
			&& AttackState.TelegraphShape != EW11TelegraphShape::ProjectilePath));
	CircleTelegraphMesh->SetHiddenInGame(!bShow || AttackState.TelegraphShape != EW11TelegraphShape::Circle);
	if (!bShow)
	{
		return;
	}
	const FLinearColor DangerColor = AttackState.TelegraphShape == EW11TelegraphShape::ProjectilePath
		? FLinearColor(1.4f, 0.02f, 0.48f, 1.0f)
		: AttackState.TelegraphShape == EW11TelegraphShape::Circle
			? FLinearColor(1.55f, 0.20f, 0.01f, 1.0f)
			: FLinearColor(1.60f, 0.035f, 0.01f, 1.0f);
	if (LineTelegraphMaterial)
	{
		LineTelegraphMaterial->SetVectorParameterValue(TEXT("DangerColor"), DangerColor);
		LineTelegraphMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.38f);
	}
	if (CircleTelegraphMaterial)
	{
		CircleTelegraphMaterial->SetVectorParameterValue(TEXT("DangerColor"), DangerColor);
		CircleTelegraphMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.34f);
	}

	// The arena is a horizontal Paper2D backdrop at Z=10. Keep warnings above
	// that depth plane while remaining safely below feet and combat sprites.
	const float FloorZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 14.0f;
	if (AttackState.TelegraphShape == EW11TelegraphShape::Line
		|| AttackState.TelegraphShape == EW11TelegraphShape::ProjectilePath)
	{
		const FVector Direction = FVector(
			AttackState.LockedDirection.X, AttackState.LockedDirection.Y, 0.0f).GetSafeNormal();
		const FVector Start(GetActorLocation().X, GetActorLocation().Y, FloorZ);
		const float Range = FMath::Max(1.0f, AttackState.TelegraphRange);
		LineTelegraphMesh->SetWorldLocation(Start + Direction * Range * 0.5f);
		LineTelegraphMesh->SetWorldRotation(FRotator(0.0f, Direction.Rotation().Yaw, 0.0f));
		LineTelegraphMesh->SetWorldScale3D(FVector(
			Range / 100.0f,
			FMath::Max(8.0f, AttackState.TelegraphRadius * 2.0f) / 100.0f,
			0.025f));
	}
	else if (AttackState.TelegraphShape == EW11TelegraphShape::Circle)
	{
		const FVector Point = AttackState.LockedTargetPoint;
		CircleTelegraphMesh->SetWorldLocation(FVector(Point.X, Point.Y, FloorZ));
		const float Radius = FMath::Max(8.0f, AttackState.TelegraphRadius);
		CircleTelegraphMesh->SetWorldScale3D(FVector(Radius / 50.0f, Radius / 50.0f, 0.025f));
	}

	if (GetNetMode() != NM_DedicatedServer
		&& AttackState.State == EW11EnemyCombatState::Windup
		&& AttackState.EncounterInstanceId > 0 && AttackState.AttackInstanceId > 0
		&& (LastTelegraphVisualEncounterInstanceId != AttackState.EncounterInstanceId
			|| LastTelegraphVisualAttackInstanceId != AttackState.AttackInstanceId))
	{
		LastTelegraphVisualEncounterInstanceId = AttackState.EncounterInstanceId;
		LastTelegraphVisualAttackInstanceId = AttackState.AttackInstanceId;
		if (FParse::Param(FCommandLine::Get(), TEXT("W11TelegraphSmoke")))
		{
			const float ServerNow = GetWorld() && GetWorld()->GetGameState()
				? GetWorld()->GetGameState()->GetServerWorldTimeSeconds() : 0.0f;
			const UStaticMeshComponent* VisibleMesh = AttackState.TelegraphShape == EW11TelegraphShape::Circle
				? CircleTelegraphMesh.Get() : LineTelegraphMesh.Get();
			UE_LOG(LogWorldWalkerW11, Display,
				TEXT("W11_TELEGRAPH_VISUAL_APPLIED Authority=%d SpawnId=%d AttackInstance=%d Ability=%s Shape=%d Material=%s Color=%s ActorLocation=%s MeshLocation=%s FloorZ=%.3f Scale=%s Lead=%.3f Visible=%d Registered=%d ComponentVisible=%d OwnerHidden=%d"),
				HasAuthority() ? 1 : 0, StableSpawnId, AttackState.AttackInstanceId,
				*AttackState.AttackDefinitionId.ToString(), static_cast<int32>(AttackState.TelegraphShape),
				*GetNameSafe(VisibleMesh ? VisibleMesh->GetMaterial(0) : nullptr), *DangerColor.ToString(),
				*GetActorLocation().ToString(),
				VisibleMesh ? *VisibleMesh->GetComponentLocation().ToString() : TEXT("None"), FloorZ,
				VisibleMesh ? *VisibleMesh->GetComponentScale().ToString() : TEXT("None"),
				AttackState.ActiveStartServerTime - ServerNow,
				VisibleMesh && !VisibleMesh->bHiddenInGame ? 1 : 0,
				VisibleMesh && VisibleMesh->IsRegistered() ? 1 : 0,
				VisibleMesh && VisibleMesh->IsVisible() ? 1 : 0,
				IsHidden() ? 1 : 0);
		}
	}
}

void AW11Enemy::PlayCombatHitReaction(const float HitStopSeconds)
{
	const FLinearColor HitTint(1.0f, 0.35f, 0.28f, 1.0f);
	FlipbookComponent->SetSpriteColor(HitTint);
	PrototypeSpriteComponent->SetSpriteColor(HitTint);
	ApplyLocalHitStop(HitStopSeconds);
	if (GetWorld())
	{
		FTimerHandle RestoreTintHandle;
		GetWorldTimerManager().SetTimer(
			RestoreTintHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				FlipbookComponent->SetSpriteColor(FLinearColor::White);
				PrototypeSpriteComponent->SetSpriteColor(FLinearColor::White);
			}),
			0.12f,
			false);
	}
}

void AW11Enemy::ApplyLocalHitStop(const float DurationSeconds)
{
	if (!GetWorld() || GetNetMode() == NM_DedicatedServer || !FlipbookComponent
		|| DurationSeconds <= 0.0f)
	{
		return;
	}
	const float ClampedDuration = FMath::Clamp(DurationSeconds, 0.0f, 0.12f);
	FlipbookComponent->SetPlayRate(0.0f);
	GetWorldTimerManager().SetTimer(
		LocalHitStopTimerHandle, this, &AW11Enemy::FinishLocalHitStop,
		ClampedDuration, false);
	if (FParse::Param(FCommandLine::Get(), TEXT("W11PresentationSmoke")))
	{
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_LOCAL_HIT_STOP Actor=Enemy SpawnId=%d Authority=%d Duration=%.3f GlobalTimeDilation=%.3f"),
			StableSpawnId, HasAuthority() ? 1 : 0, ClampedDuration,
			UGameplayStatics::GetGlobalTimeDilation(this));
	}
}

void AW11Enemy::FinishLocalHitStop()
{
	if (FlipbookComponent)
	{
		FlipbookComponent->SetPlayRate(1.0f);
	}
}

void AW11Enemy::BroadcastCombatCue(const FW11CombatCue& Cue)
{
	if (HasAuthority())
	{
		MulticastCombatCue(Cue);
	}
}

void AW11Enemy::MulticastCombatCue_Implementation(const FW11CombatCue Cue)
{
	if (!GetWorld() || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (!CueDeduplicator.Accept(Cue))
	{
		return;
	}
	FString Value;
	FLinearColor Color(0.95f, 0.25f, 0.18f, 1.0f);
	if (Cue.Type == EW11CombatCueType::Immune)
	{
		Value = TEXT("免疫");
		Color = FLinearColor(0.65f, 0.85f, 1.0f, 1.0f);
	}
	else
	{
		Value = Cue.PrimaryAmount > 0.0f
			? FString::Printf(TEXT("-%d"), FMath::RoundToInt(Cue.PrimaryAmount)) : FString();
		if (Cue.SecondaryAmount > 0.0f)
		{
			Value += FString::Printf(TEXT("  护障-%d"), FMath::RoundToInt(Cue.SecondaryAmount));
		}
	}
	FString AbilityPrefix;
	FString AbilityKey;
	Cue.AbilityId.ToString().Split(TEXT("."), &AbilityPrefix, &AbilityKey, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	const FString AbilityPath = FString::Printf(
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Abilities/Enemies/DA_Ability_%s.DA_Ability_%s"),
		*AbilityKey, *AbilityKey);
	UW11SkillVisualDefinition* CueVisual = nullptr;
	if (UW11AbilityDefinition* Ability = LoadObject<UW11AbilityDefinition>(nullptr, *AbilityPath))
	{
		CueVisual = Ability->Visuals.LoadSynchronous();
		if (CueVisual)
		{
			if (USoundBase* ImpactSound = CueVisual->ImpactSound.LoadSynchronous())
			{
				if (CombatAudioGate.Accept(Cue, EW11CombatAudioLayer::Primary))
				{
					UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Cue.Location);
				}
			}
		}
	}
	const bool bImpactCue = Cue.Type == EW11CombatCueType::Damage
		|| Cue.Type == EW11CombatCueType::Immune
		|| Cue.Type == EW11CombatCueType::Defeated;
	if (bImpactCue)
	{
		const UW11SkillVisualDefinition* ImpactProfile = CueVisual
			? CueVisual : GetDefault<UW11SkillVisualDefinition>();
		if (AW11Character* TargetCharacter = Cast<AW11Character>(Cue.Target))
		{
			TargetCharacter->PlayCombatHitReaction(
				ImpactProfile->ResolveHitStopSeconds(Cue.Type, Cue.bCritical),
				ImpactProfile->ResolveCameraImpactStrength(Cue.Type, Cue.bCritical),
				ImpactProfile->CameraImpactDuration);
		}
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AW11CombatCueActor* CueActor = GetWorld()->SpawnActor<AW11CombatCueActor>(
		AW11CombatCueActor::StaticClass(), FVector(Cue.Location), FRotator::ZeroRotator, Params))
	{
		CueActor->Configure(nullptr, FText::FromString(Value), Color, 0.5f);
	}
}

void AW11Enemy::CancelForCombatEnd(const int32 EndingEncounterInstanceId)
{
	if (!HasAuthority() || EndingEncounterInstanceId != EncounterInstanceId)
	{
		return;
	}
	if (AttackState.State == EW11EnemyCombatState::Windup
		|| AttackState.State == EW11EnemyCombatState::Active)
	{
		if (AW11GameMode* GameMode = GetWorld()->GetAuthGameMode<AW11GameMode>())
		{
			GameMode->RecordEnemyAttackCancelled(EnemyDefinitionId);
			GameMode->RecordCancelledThreat();
		}
	}
	DetachFromControllerPendingDestroy();
	const FW11ReplicatedAttackState CancelledState = MakeInactiveAttackState(
		AttackState, bDefeatHandled
			? EW11EnemyCombatState::Defeated : EW11EnemyCombatState::AcquireTarget);
	SetAttackState(CancelledState);
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetLifeSpan(bDefeatHandled ? 0.8f : 0.35f);
}

FW11ReplicatedAttackState AW11Enemy::MakeInactiveAttackState(
	const FW11ReplicatedAttackState& CurrentState,
	const EW11EnemyCombatState NextState)
{
	FW11ReplicatedAttackState Result = CurrentState;
	Result.State = NextState;
	Result.AttackDefinitionId = NAME_None;
	Result.LockedTargetStableId = 0;
	Result.LockedDirection = FVector::ForwardVector;
	Result.LockedTargetPoint = FVector::ZeroVector;
	Result.TelegraphShape = EW11TelegraphShape::None;
	Result.TelegraphRange = 0.0f;
	Result.TelegraphRadius = 0.0f;
	Result.WindupStartServerTime = 0.0f;
	Result.ActiveStartServerTime = 0.0f;
	Result.RecoveryEndServerTime = 0.0f;
	return Result;
}

void AW11Enemy::OnRep_PresentationSprite()
{
	if (PrototypeSpriteComponent)
	{
		PrototypeSpriteComponent->SetSprite(PresentationSprite.LoadSynchronous());
	}
}

void AW11Enemy::OnRep_MovementFlipbooks()
{
	MoveFlipbookRight = PresentationMoveFlipbookRight.LoadSynchronous();
	MoveFlipbookLeft = PresentationMoveFlipbookLeft.LoadSynchronous();
}

void AW11Enemy::HandleDefeated()
{
	if (!HasAuthority() || bDefeatHandled)
	{
		return;
	}
	bDefeatHandled = true;
	SetAttackState(MakeInactiveAttackState(AttackState, EW11EnemyCombatState::Defeated));
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (AW11GameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AW11GameMode>() : nullptr)
	{
		if (AW11RunDirector* Director = GameMode->GetRunDirector())
		{
			Director->NotifyEnemyDefeated(this);
		}
	}
	SetLifeSpan(0.8f);
}
