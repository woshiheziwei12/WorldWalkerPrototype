#include "Game/W11Character.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/W11AttributeComponent.h"
#include "Components/W11CombatComponent.h"
#include "Core/W11Types.h"
#include "Core/W11ArenaBounds.h"
#include "Data/W11Definitions.h"
#include "Game/W11PlayerState.h"
#include "Game/W11Enemy.h"
#include "Game/W11CombatCueActor.h"
#include "Game/W11GameMode.h"
#include "Game/W11GameState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "PaperFlipbookComponent.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/AssetManager.h"
#include "EngineUtils.h"
#include "WorldWalkerW11.h"

namespace
{
template <typename TDefinition>
TDefinition* LoadW11PrimaryDefinition(const FName TypeName, const FName DefinitionId)
{
	if (DefinitionId.IsNone())
	{
		return nullptr;
	}
	const FSoftObjectPath Path = UAssetManager::Get().GetPrimaryAssetPath(
		FPrimaryAssetId(FPrimaryAssetType(TypeName), DefinitionId));
	if (TDefinition* Definition = Cast<TDefinition>(Path.TryLoad()))
	{
		return Definition;
	}

	// The first game tick can precede the Asset Manager's asynchronous discovery in
	// uncooked editor runs. Stable W11 package names provide a deterministic fallback;
	// the data asset's own soft references still own all presentation dependencies.
	FString Prefix;
	FString Key;
	DefinitionId.ToString().Split(TEXT("."), &Prefix, &Key, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	FString ObjectPath;
	if (TypeName == TEXT("W11Hero"))
	{
		ObjectPath = FString::Printf(
			TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Heroes/DA_Hero_%s.DA_Hero_%s"),
			*Key, *Key);
	}
	else if (TypeName == TEXT("W11Ability"))
	{
		ObjectPath = FString::Printf(
			TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/Abilities/DA_Ability_%s.DA_Ability_%s"),
			*Key, *Key);
	}
	return ObjectPath.IsEmpty() ? nullptr : LoadObject<TDefinition>(nullptr, *ObjectPath);
}

UPaperFlipbook* SelectActionFlipbook(
	const FW11CharacterActionFlipbooks& Set,
	const EW11CharacterAction Action)
{
	const TSoftObjectPtr<UPaperFlipbook>* Flipbook = nullptr;
	switch (Action)
	{
	case EW11CharacterAction::Move:
		Flipbook = &Set.Move;
		break;
	case EW11CharacterAction::Dodge:
		Flipbook = &Set.Dodge;
		if (Flipbook->IsNull())
		{
			Flipbook = &Set.Move;
		}
		break;
	case EW11CharacterAction::Cast:
		Flipbook = &Set.Cast;
		break;
	case EW11CharacterAction::Hit:
		Flipbook = &Set.Hit;
		break;
	case EW11CharacterAction::Idle:
	default:
		Flipbook = &Set.Idle;
		break;
	}
	return Flipbook ? Flipbook->LoadSynchronous() : nullptr;
}
}

AW11Character::AW11Character()
{
	FacingDirection = EW11FacingDirection::Right;
	LastHorizontalFacing = EW11FacingDirection::Right;
	CurrentAction = EW11CharacterAction::Idle;
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(30.0f);
	SetMinNetUpdateFrequency(10.0f);

	GetCharacterMovement()->DefaultLandMovementMode = MOVE_Flying;
	GetCharacterMovement()->GravityScale = 0.0f;
	GetCharacterMovement()->MaxFlySpeed = 480.0f;
	GetCharacterMovement()->MaxAcceleration = 6000.0f;
	GetCharacterMovement()->BrakingDecelerationFlying = 10000.0f;
	GetCharacterMovement()->bUseSeparateBrakingFriction = true;
	GetCharacterMovement()->BrakingFriction = 12.0f;
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector::UpVector);
	// Preserve the PlayerStart height. Snapping to the default Z=0 plane placed
	// the capsule halfway inside the arena floor and blocked every XY move/sweep.
	GetCharacterMovement()->bSnapToPlaneAtStart = false;
	CombatComponent = CreateDefaultSubobject<UW11CombatComponent>(TEXT("W11Combat"));

	FlipbookComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("W11Flipbook"));
	FlipbookComponent->SetupAttachment(GetCapsuleComponent());
	FlipbookComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FlipbookComponent->SetIsReplicated(false);
	FlipbookComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, -90.0f));
	FlipbookComponent->SetTranslucentSortPriority(25);
	FlipbookComponent->SetCastShadow(false);
	FlipbookComponent->SetReceivesDecals(false);

	SkillEffectComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("W11SkillEffect"));
	SkillEffectComponent->SetupAttachment(GetCapsuleComponent());
	SkillEffectComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkillEffectComponent->SetIsReplicated(false);
	SkillEffectComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, -90.0f));
	SkillEffectComponent->SetTranslucentSortPriority(40);
	SkillEffectComponent->SetCastShadow(false);
	SkillEffectComponent->SetReceivesDecals(false);
	SkillEffectComponent->SetLooping(false);
	SkillEffectComponent->SetVisibility(false);

	PrototypeSpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("W11PrototypeSprite"));
	PrototypeSpriteComponent->SetupAttachment(GetCapsuleComponent());
	PrototypeSpriteComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PrototypeSpriteComponent->SetIsReplicated(false);
	PrototypeSpriteComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, -90.0f));
	PrototypeSpriteComponent->SetRelativeScale3D(FVector(0.45f));
	PrototypeSpriteComponent->SetTranslucentSortPriority(20);
	PrototypeSpriteComponent->SetCastShadow(false);
	PrototypeSpriteComponent->SetReceivesDecals(false);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TranslucentSpriteMaterial(
		TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	if (TranslucentSpriteMaterial.Succeeded())
	{
		FlipbookComponent->SetMaterial(0, TranslucentSpriteMaterial.Object);
		SkillEffectComponent->SetMaterial(0, TranslucentSpriteMaterial.Object);
		PrototypeSpriteComponent->SetMaterial(0, TranslucentSpriteMaterial.Object);
	}
	static ConstructorHelpers::FObjectFinder<UPaperSprite> PlayerSprite(
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Prototype/S_W11_Player.S_W11_Player"));
	if (PlayerSprite.Succeeded())
	{
		PrototypeSpriteComponent->SetSprite(PlayerSprite.Object);
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("W11CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 1200.0f;
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("W11TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->ProjectionMode = ECameraProjectionMode::Orthographic;
	TopDownCamera->OrthoWidth = ResolveArenaCameraWidth(0, 0);
	TopDownCamera->SetConstraintAspectRatio(false);
	TopDownCamera->bOverrideAspectRatioAxisConstraint = true;
	TopDownCamera->SetAspectRatioAxisConstraint(EAspectRatioAxisConstraint::AspectRatio_MaintainXFOV);
}

void AW11Character::BeginPlay()
{
	Super::BeginPlay();
	BaseCameraRelativeLocation = TopDownCamera
		? TopDownCamera->GetRelativeLocation() : FVector::ZeroVector;
	if (HasAuthority())
	{
		const AW11GameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AW11GameMode>() : nullptr;
		if (const UW11RunRuleSet* Rules = GameMode ? GameMode->GetActiveRuleSet() : nullptr)
		{
			DodgeDistance = FMath::Max(1.0f, Rules->DodgeDistance);
			DodgeDuration = FMath::Max(0.01f, Rules->DodgeDuration);
			DodgeCooldown = FMath::Max(0.01f, Rules->DodgeCooldown);
			DodgeInvulnerabilityDuration = FMath::Clamp(
				Rules->DodgeInvulnerabilityDuration, 0.0f, DodgeCooldown);
			ForceNetUpdate();
		}
	}
	GetCharacterMovement()->SetPlaneConstraintOrigin(GetActorLocation());
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	PrototypeSpriteComponent->SetVisibility(FlipbookComponent->GetFlipbook() == nullptr);
	CombatComponent->OnCombatCue.AddDynamic(this, &AW11Character::HandleCombatCue);
	if (HasAuthority() && FParse::Param(FCommandLine::Get(), TEXT("W11AutoCastInitialAbility")))
	{
		FTimerHandle AutoCastHandle;
		GetWorldTimerManager().SetTimer(AutoCastHandle, this, &AW11Character::CastInitialAbility, 1.0f, false);
	}
	FString InputScript;
	if (IsLocallyControlled() && FParse::Value(FCommandLine::Get(), TEXT("W11InputScript="), InputScript)
		&& (InputScript.Equals(TEXT("CombatLoop"), ESearchCase::IgnoreCase)
			|| InputScript.Equals(TEXT("DodgeLoop"), ESearchCase::IgnoreCase)
			|| InputScript.Equals(TEXT("SpamLoop"), ESearchCase::IgnoreCase)
			|| InputScript.Equals(TEXT("MoveEvasionLoop"), ESearchCase::IgnoreCase)
			|| InputScript.Equals(TEXT("RecoveryPunish"), ESearchCase::IgnoreCase)
			|| InputScript.Equals(TEXT("PacingCombatLoop"), ESearchCase::IgnoreCase)))
	{
		ScriptedInputMode = FName(InputScript);
		const float Interval = InputScript.Equals(TEXT("DodgeLoop"), ESearchCase::IgnoreCase)
			? 0.78f
			: InputScript.Equals(TEXT("SpamLoop"), ESearchCase::IgnoreCase) ? 0.05f
			: InputScript.Equals(TEXT("MoveEvasionLoop"), ESearchCase::IgnoreCase) ? 1.10f : 0.48f;
		GetWorldTimerManager().SetTimer(
			ScriptedInputTimerHandle, this, &AW11Character::RunScriptedCombatInput, Interval, true, 0.75f);
	}
}

void AW11Character::RunScriptedCombatInput()
{
	static const FVector2D Directions[] = {
		FVector2D(1.0f, 0.0f), FVector2D(0.0f, 1.0f),
		FVector2D(-1.0f, 0.0f), FVector2D(0.0f, -1.0f)};
	if (!IsLocallyControlled() || !CombatComponent || !GetWorld())
	{
		return;
	}
	const AW11GameState* State = GetWorld()->GetGameState<AW11GameState>();
	if (!State || !AW11GameState::IsPendingCombatEncounter(
		State->GetRunPhase(), State->GetCombatOutcome(),
		State->GetEncounterInstanceId(), State->GetEncounterInstanceId()))
	{
		return;
	}
	const FVector2D Direction = Directions[ScriptedInputStep % UE_ARRAY_COUNT(Directions)];
	if (ScriptedInputMode.IsEqual(TEXT("PacingCombatLoop"), ENameCase::IgnoreCase))
	{
		AW11Enemy* Target = FindNearestScriptedEnemy(State->GetEncounterInstanceId());
		if (!Target)
		{
			return;
		}
		const FVector WorldDirection =
			(Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		const FVector2D AimDirection(WorldDirection.Y, WorldDirection.X);
		bool bWindupThreat = false;
		for (TActorIterator<AW11Enemy> It(GetWorld()); It; ++It)
		{
			if (It->GetEncounterInstanceId() == State->GetEncounterInstanceId()
				&& It->GetAttackState().State == EW11EnemyCombatState::Windup)
			{
				bWindupThreat = true;
				break;
			}
		}
		if (bWindupThreat)
		{
			// A deterministic tangential dodge is intentionally conservative: it
			// exercises reaction windows without reading future hit results.
			MoveHorizontalInput = -AimDirection.Y;
			MoveVerticalInput = AimDirection.X;
			TryDodge();
		}
		CombatComponent->TryBasicAttack(AimDirection);
		if ((ScriptedInputStep % 10) == 2)
		{
			CombatComponent->TryAbilitySlot(EW11AbilitySlot::Active1, AimDirection);
		}
		++ScriptedInputStep;
		return;
	}
	if (ScriptedInputMode.IsEqual(TEXT("MoveEvasionLoop"), ENameCase::IgnoreCase))
	{
		++ScriptedInputStep;
		const FVector Location = GetActorLocation();
		const UCharacterMovementComponent* Movement = GetCharacterMovement();
		const FVector Velocity = Movement ? Movement->Velocity : FVector::ZeroVector;
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_SCRIPT_MOVE_EVASION_LEG Step=%d Direction=%.2f,%.2f Location=%.1f,%.1f Speed=%.1f FrameDelta=%.6f"),
			ScriptedInputStep, MoveHorizontalInput, MoveVerticalInput,
			Location.X, Location.Y, Velocity.Size2D(), GetWorld()->GetDeltaSeconds());
		return;
	}
	if (ScriptedInputMode.IsEqual(TEXT("SpamLoop"), ENameCase::IgnoreCase))
	{
		// Deliberately exceed the authored basic-attack interval through the same
		// public client request path used by gameplay. Network smoke logs prove the
		// server admits only cooldown-legal requests.
		CombatComponent->TryBasicAttack(Direction);
		++ScriptedInputStep;
		return;
	}
	if (ScriptedInputMode.IsEqual(TEXT("RecoveryPunish"), ENameCase::IgnoreCase))
	{
		AW11Enemy* RecoveryTarget = nullptr;
		float BestDistanceSquared = TNumericLimits<float>::Max();
		for (TActorIterator<AW11Enemy> It(GetWorld()); It; ++It)
		{
			AW11Enemy* Candidate = *It;
			if (!Candidate || Candidate->GetEncounterInstanceId() != State->GetEncounterInstanceId()
				|| !Candidate->GetAttributeComponent()
				|| Candidate->GetAttributeComponent()->IsDefeated()
				|| Candidate->GetAttackState().State != EW11EnemyCombatState::Recovery)
			{
				continue;
			}
			const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
			if (!RecoveryTarget || DistanceSquared < BestDistanceSquared
				|| (FMath::IsNearlyEqual(DistanceSquared, BestDistanceSquared)
					&& Candidate->GetStableSpawnId() < RecoveryTarget->GetStableSpawnId()))
			{
				RecoveryTarget = Candidate;
				BestDistanceSquared = DistanceSquared;
			}
		}
		if (RecoveryTarget)
		{
			const FVector WorldDirection =
				(RecoveryTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
			const FVector2D PunishDirection(WorldDirection.Y, WorldDirection.X);
			const AW11PlayerState* W11PlayerState = GetPlayerState<AW11PlayerState>();
			UE_LOG(LogWorldWalkerW11, Display,
				TEXT("W11_SCRIPT_RECOVERY_PUNISH_REQUEST Player=%d Step=%d SpawnId=%d AttackInstance=%d Distance=%.1f Direction=%.2f,%.2f"),
				W11PlayerState ? W11PlayerState->GetPlayerId() : 0,
				ScriptedInputStep, RecoveryTarget->GetStableSpawnId(),
				RecoveryTarget->GetAttackState().AttackInstanceId, FMath::Sqrt(BestDistanceSquared),
				PunishDirection.X, PunishDirection.Y);
			CombatComponent->TryBasicAttack(PunishDirection);
		}
		++ScriptedInputStep;
		return;
	}
	if (ScriptedInputMode.IsEqual(TEXT("DodgeLoop"), ENameCase::IgnoreCase))
	{
		UpdateFacingFromDirection(Direction);
		TryDodge();
	}
	CombatComponent->TryBasicAttack(Direction);
	const bool bM5Loadout = FParse::Param(FCommandLine::Get(), TEXT("W11M5Loadout"));
	if (bM5Loadout || (ScriptedInputStep % 4) == 2)
	{
		const int32 SlotOffset = bM5Loadout ? ScriptedInputStep % 4 : (ScriptedInputStep / 4) % 4;
		CombatComponent->TryAbilitySlot(
			static_cast<EW11AbilitySlot>(static_cast<int32>(EW11AbilitySlot::Active1) + SlotOffset),
			Direction);
	}
	++ScriptedInputStep;
}

void AW11Character::PlayCombatHitReaction(
	const float HitStopSeconds,
	const float CameraImpactStrength,
	const float CameraImpactDuration)
{
	PlayCharacterAction(EW11CharacterAction::Hit, 0.24f);
	ApplyLocalHitStop(HitStopSeconds);
	TriggerLocalCameraImpact(CameraImpactStrength, CameraImpactDuration);
}

void AW11Character::ApplyLocalHitStop(const float DurationSeconds)
{
	if (!GetWorld() || GetNetMode() == NM_DedicatedServer || !FlipbookComponent
		|| DurationSeconds <= 0.0f)
	{
		return;
	}
	const float ClampedDuration = FMath::Clamp(DurationSeconds, 0.0f, 0.12f);
	FlipbookComponent->SetPlayRate(0.0f);
	GetWorldTimerManager().SetTimer(
		LocalHitStopTimerHandle, this, &AW11Character::FinishLocalHitStop,
		ClampedDuration, false);
	if (FParse::Param(FCommandLine::Get(), TEXT("W11PresentationSmoke")))
	{
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_LOCAL_HIT_STOP Actor=Player Authority=%d LocallyControlled=%d Duration=%.3f GlobalTimeDilation=%.3f"),
			HasAuthority() ? 1 : 0, IsLocallyControlled() ? 1 : 0, ClampedDuration,
			UGameplayStatics::GetGlobalTimeDilation(this));
	}
}

void AW11Character::FinishLocalHitStop()
{
	if (FlipbookComponent)
	{
		FlipbookComponent->SetPlayRate(1.0f);
	}
}

void AW11Character::TriggerLocalCameraImpact(const float Strength, const float DurationSeconds)
{
	if (!IsLocallyControlled() || !TopDownCamera || Strength <= 0.0f || DurationSeconds <= 0.0f)
	{
		return;
	}
	LocalCameraImpactStrength = FMath::Max(
		LocalCameraImpactStrength, FMath::Clamp(Strength, 0.0f, 8.0f));
	LocalCameraImpactDuration = FMath::Max(
		LocalCameraImpactDuration, FMath::Clamp(DurationSeconds, 0.01f, 0.30f));
	LocalCameraImpactRemaining = LocalCameraImpactDuration;
	if (FParse::Param(FCommandLine::Get(), TEXT("W11PresentationSmoke")))
	{
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_LOCAL_CAMERA_IMPACT Strength=%.2f Duration=%.3f Cap=8.00"),
			LocalCameraImpactStrength, LocalCameraImpactDuration);
	}
}

void AW11Character::UpdateLocalCameraImpact(const float DeltaSeconds)
{
	if (!TopDownCamera)
	{
		return;
	}
	if (LocalCameraImpactRemaining <= 0.0f || LocalCameraImpactDuration <= 0.0f)
	{
		TopDownCamera->SetRelativeLocation(BaseCameraRelativeLocation);
		LocalCameraImpactRemaining = 0.0f;
		LocalCameraImpactDuration = 0.0f;
		LocalCameraImpactStrength = 0.0f;
		return;
	}
	LocalCameraImpactRemaining = FMath::Max(0.0f, LocalCameraImpactRemaining - DeltaSeconds);
	const float RemainingAlpha = LocalCameraImpactRemaining / LocalCameraImpactDuration;
	const float Progress = 1.0f - RemainingAlpha;
	const float Oscillation = Progress * 6.0f * UE_PI;
	const FVector Offset(
		0.0f,
		FMath::Sin(Oscillation) * LocalCameraImpactStrength * RemainingAlpha,
		FMath::Cos(Oscillation * 0.73f) * LocalCameraImpactStrength * 0.6f * RemainingAlpha);
	TopDownCamera->SetRelativeLocation(BaseCameraRelativeLocation + Offset);
}

void AW11Character::SendIncomingCombatCue(const FW11CombatCue& Cue)
{
	if (!HasAuthority())
	{
		return;
	}
	if (GetNetMode() == NM_Standalone)
	{
		if (IncomingCueDeduplicator.Accept(Cue))
		{
			OnIncomingCombatCue.Broadcast(Cue);
		}
		return;
	}
	ClientReceiveIncomingCombatCue(Cue);
}

void AW11Character::ClientReceiveIncomingCombatCue_Implementation(const FW11CombatCue Cue)
{
	if (!IncomingCueDeduplicator.Accept(Cue))
	{
		return;
	}
	OnIncomingCombatCue.Broadcast(Cue);
	if (FParse::Param(FCommandLine::Get(), TEXT("W11NetworkSmoke")))
	{
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_INCOMING_HIT_CUE EncounterInstance=%d SourceStableId=%d AttackInstance=%d Source=%s Damage=%.1f Barrier=%.1f Direction=(%.2f,%.2f)"),
			Cue.EncounterInstanceId, Cue.SourceStableId, Cue.AuthorityExecutionSequence, *GetNameSafe(Cue.Source),
			Cue.PrimaryAmount, Cue.SecondaryAmount, Cue.Direction.X, Cue.Direction.Y);
	}
}

void AW11Character::HandleCombatCue(const FW11CombatCue& Cue)
{
	if (!GetWorld() || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (!ObservedCueDeduplicator.Accept(Cue))
	{
		return;
	}

	UPaperFlipbook* ImpactFlipbook = nullptr;
	UW11SkillVisualDefinition* CueVisual = nullptr;
	float ImpactScale = 0.55f;
	if (UW11AbilityDefinition* Ability = LoadW11PrimaryDefinition<UW11AbilityDefinition>(TEXT("W11Ability"), Cue.AbilityId))
	{
		if (UW11SkillVisualDefinition* Visual = Ability->Visuals.LoadSynchronous())
		{
			CueVisual = Visual;
			ImpactFlipbook = Visual->ImpactFlipbook.LoadSynchronous();
			if (!ImpactFlipbook && Ability->LogicalSlot == EW11AbilitySlot::BasicAttack)
			{
				ImpactFlipbook = Visual->EffectFlipbook.LoadSynchronous();
			}
			ImpactScale = Visual->WorldScale;
		}
	}
	const bool bImpactCue = Cue.Type == EW11CombatCueType::Damage
		|| Cue.Type == EW11CombatCueType::Immune
		|| Cue.Type == EW11CombatCueType::Defeated;
	if (bImpactCue)
	{
		const UW11SkillVisualDefinition* ImpactProfile = CueVisual
			? CueVisual : GetDefault<UW11SkillVisualDefinition>();
		const float HitStopSeconds = ImpactProfile->ResolveHitStopSeconds(Cue.Type, Cue.bCritical);
		const float CameraStrength = ImpactProfile->ResolveCameraImpactStrength(Cue.Type, Cue.bCritical);
		if (AW11Enemy* EnemyTarget = Cast<AW11Enemy>(Cue.Target))
		{
			EnemyTarget->PlayCombatHitReaction(HitStopSeconds);
		}
		else if (AW11Character* CharacterTarget = Cast<AW11Character>(Cue.Target))
		{
			CharacterTarget->PlayCombatHitReaction(
				HitStopSeconds, CameraStrength, ImpactProfile->CameraImpactDuration);
		}
		const EW11CombatAudioLayer CameraLayer = Cue.Type == EW11CombatCueType::Defeated
			? EW11CombatAudioLayer::Defeat
			: Cue.bCritical ? EW11CombatAudioLayer::Critical : EW11CombatAudioLayer::Primary;
		if (IsLocallyControlled() && CombatCameraGate.Accept(Cue, CameraLayer))
		{
			TriggerLocalCameraImpact(CameraStrength, ImpactProfile->CameraImpactDuration);
		}
	}

	FText NumberText;
	FLinearColor Color(0.95f, 0.25f, 0.18f, 1.0f);
	if (Cue.Type == EW11CombatCueType::Immune)
	{
		NumberText = FText::FromString(TEXT("免疫"));
		Color = FLinearColor(0.65f, 0.85f, 1.0f, 1.0f);
	}
	else if (Cue.Type == EW11CombatCueType::Heal)
	{
		NumberText = FText::FromString(FString::Printf(TEXT("+%d"), FMath::RoundToInt(Cue.PrimaryAmount)));
		Color = FLinearColor(0.35f, 1.0f, 0.48f, 1.0f);
	}
	else if (Cue.Type == EW11CombatCueType::Barrier)
	{
		NumberText = FText::FromString(FString::Printf(TEXT("护障 +%d"), FMath::RoundToInt(Cue.PrimaryAmount)));
		Color = FLinearColor(0.35f, 0.75f, 1.0f, 1.0f);
	}
	else
	{
		FString Value = Cue.PrimaryAmount > 0.0f
			? FString::Printf(TEXT("-%d"), FMath::RoundToInt(Cue.PrimaryAmount)) : FString();
		if (Cue.SecondaryAmount > 0.0f)
		{
			Value += FString::Printf(TEXT("  护障-%d"), FMath::RoundToInt(Cue.SecondaryAmount));
		}
		if (Cue.bCritical)
		{
			Value = TEXT("会心 ") + Value;
			Color = FLinearColor(1.0f, 0.78f, 0.18f, 1.0f);
		}
		NumberText = FText::FromString(Value);
	}
	if (CueVisual)
	{
		if (USoundBase* ImpactSound = CueVisual->ImpactSound.LoadSynchronous())
		{
			if (CombatAudioGate.Accept(Cue, EW11CombatAudioLayer::Primary))
			{
				UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Cue.Location);
			}
		}
		if (Cue.bCritical)
		{
			if (USoundBase* CriticalSound = CueVisual->CriticalSound.LoadSynchronous())
			{
				if (CombatAudioGate.Accept(Cue, EW11CombatAudioLayer::Critical))
				{
					UGameplayStatics::PlaySoundAtLocation(this, CriticalSound, Cue.Location);
				}
			}
		}
		if (Cue.Type == EW11CombatCueType::Defeated)
		{
			if (USoundBase* DefeatSound = CueVisual->DefeatSound.LoadSynchronous())
			{
				if (CombatAudioGate.Accept(Cue, EW11CombatAudioLayer::Defeat))
				{
					UGameplayStatics::PlaySoundAtLocation(this, DefeatSound, Cue.Location);
				}
			}
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AW11CombatCueActor* CueActor = GetWorld()->SpawnActor<AW11CombatCueActor>(
		AW11CombatCueActor::StaticClass(), FVector(Cue.Location), FRotator::ZeroRotator, Params))
	{
		CueActor->Configure(ImpactFlipbook, NumberText, Color, ImpactScale);
	}
}

void AW11Character::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AW11Character, bDodgeActive);
	DOREPLIFETIME(AW11Character, LastDodgeDirection);
	DOREPLIFETIME(AW11Character, DodgeSequence);
	DOREPLIFETIME(AW11Character, DodgeLandingLocation);
	DOREPLIFETIME_CONDITION(AW11Character, DodgeCooldownEndTime, COND_OwnerOnly);
	DOREPLIFETIME(AW11Character, DodgeDistance);
	DOREPLIFETIME(AW11Character, DodgeDuration);
	DOREPLIFETIME(AW11Character, DodgeCooldown);
	DOREPLIFETIME(AW11Character, DodgeInvulnerabilityDuration);
}

void AW11Character::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AW11GameState* RuntimeState = GetWorld() ? GetWorld()->GetGameState<AW11GameState>() : nullptr;
	const int32 RuntimeChapterIndex = RuntimeState ? RuntimeState->GetChapterIndex() : 0;
	const int32 RuntimeStageIndex = RuntimeState ? RuntimeState->GetStageIndex() : 0;
	if (HasAuthority() || IsLocallyControlled())
	{
		FW11ArenaBounds::ConstrainCharacter(
			*this, FW11ArenaBounds::ResolveHalfExtent(RuntimeChapterIndex, RuntimeStageIndex));
	}
	if (IsLocallyControlled() && TopDownCamera)
	{
		const int32 ChapterIndex = RuntimeChapterIndex;
		const int32 StageIndex = RuntimeStageIndex;
		if (ChapterIndex != AppliedCameraChapterIndex || StageIndex != AppliedCameraStageIndex)
		{
			AppliedCameraChapterIndex = ChapterIndex;
			AppliedCameraStageIndex = StageIndex;
			TopDownCamera->OrthoWidth = ResolveArenaCameraWidth(ChapterIndex, StageIndex);
		}
	}
	UpdateLocalCameraImpact(DeltaSeconds);
	UpdateScriptedMovement();
	RefreshPresentationDefinitions();
	UpdatePresentation(DeltaSeconds);
	const AW11PlayerState* W11PlayerState = GetPlayerState<AW11PlayerState>();
	const UW11AttributeComponent* Attributes = W11PlayerState ? W11PlayerState->GetAttributeComponent() : nullptr;
	if (Attributes)
	{
		GetCharacterMovement()->MaxFlySpeed = 480.0f * Attributes->GetStats().MoveSpeedScale
			* Attributes->GetMovementStatusScale();
	}
}

void AW11Character::UpdateScriptedMovement()
{
	const bool bMoveEvasion = ScriptedInputMode.IsEqual(TEXT("MoveEvasionLoop"), ENameCase::IgnoreCase);
	const bool bPacingCombat = ScriptedInputMode.IsEqual(TEXT("PacingCombatLoop"), ENameCase::IgnoreCase);
	if ((!bMoveEvasion && !bPacingCombat)
		|| !IsLocallyControlled() || !GetWorld())
	{
		return;
	}

	const AW11GameState* State = GetWorld()->GetGameState<AW11GameState>();
	if (!State || !AW11GameState::IsPendingCombatEncounter(
		State->GetRunPhase(), State->GetCombatOutcome(),
		State->GetEncounterInstanceId(), State->GetEncounterInstanceId()))
	{
		if (!FMath::IsNearlyZero(MoveHorizontalInput) || !FMath::IsNearlyZero(MoveVerticalInput))
		{
			MoveHorizontalInput = 0.0f;
			MoveVerticalInput = 0.0f;
			GetCharacterMovement()->StopMovementImmediately();
		}
		return;
	}

	FVector2D InputDirection = FVector2D::ZeroVector;
	if (bPacingCombat)
	{
		if (const AW11Enemy* Target = FindNearestScriptedEnemy(State->GetEncounterInstanceId()))
		{
			const FVector Offset = Target->GetActorLocation() - GetActorLocation();
			const FVector2D Toward(Offset.Y, Offset.X);
			const FVector2D AimDirection = Toward.GetSafeNormal();
			const float Distance = Offset.Size2D();
			InputDirection = Distance > 235.0f ? AimDirection
				: Distance < 145.0f ? -AimDirection
				: FVector2D(-AimDirection.Y, AimDirection.X);
		}
	}
	else
	{
		const AW11Enemy* MiasmaTarget = nullptr;
		float BestDistanceSquared = TNumericLimits<float>::Max();
		for (TActorIterator<AW11Enemy> It(GetWorld()); It; ++It)
		{
			const AW11Enemy* Candidate = *It;
			if (!Candidate || Candidate->GetEncounterInstanceId() != State->GetEncounterInstanceId()
				|| Candidate->GetEnemyDefinitionId() != TEXT("Enemy.MiasmaWisp")
				|| !Candidate->GetAttributeComponent()
				|| Candidate->GetAttributeComponent()->IsDefeated())
			{
				continue;
			}
			const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
			if (!MiasmaTarget || DistanceSquared < BestDistanceSquared)
			{
				MiasmaTarget = Candidate;
				BestDistanceSquared = DistanceSquared;
			}
		}
		if (MiasmaTarget)
		{
			const FVector Offset = MiasmaTarget->GetActorLocation() - GetActorLocation();
			const FVector2D Toward(Offset.Y, Offset.X);
			const FVector2D RadialDirection = Toward.GetSafeNormal();
			const FVector2D TangentDirection(-RadialDirection.Y, RadialDirection.X);
			const float Distance = Offset.Size2D();
			InputDirection = Distance > 620.0f ? RadialDirection
				: Distance < 360.0f ? -RadialDirection
				: TangentDirection;
		}
		else
		{
			static const FVector2D Directions[] = {
				FVector2D(1.0f, 0.0f), FVector2D(0.0f, 1.0f),
				FVector2D(-1.0f, 0.0f), FVector2D(0.0f, -1.0f)};
			InputDirection = Directions[ScriptedInputStep % UE_ARRAY_COUNT(Directions)];
		}
	}
	MoveHorizontalInput = InputDirection.X;
	MoveVerticalInput = InputDirection.Y;
	UpdateFacingFromDirection(InputDirection);
	AddMovementInput(
		FVector::RightVector * MoveHorizontalInput + FVector::ForwardVector * MoveVerticalInput,
		1.0f);
}

AW11Enemy* AW11Character::FindNearestScriptedEnemy(const int32 EncounterInstanceId) const
{
	AW11Enemy* BestTarget = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (TActorIterator<AW11Enemy> It(GetWorld()); It; ++It)
	{
		AW11Enemy* Candidate = *It;
		if (!Candidate || Candidate->GetEncounterInstanceId() != EncounterInstanceId
			|| !Candidate->GetAttributeComponent()
			|| Candidate->GetAttributeComponent()->IsDefeated())
		{
			continue;
		}
		const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
		if (!BestTarget || DistanceSquared < BestDistanceSquared
			|| (FMath::IsNearlyEqual(DistanceSquared, BestDistanceSquared)
				&& Candidate->GetStableSpawnId() < BestTarget->GetStableSpawnId()))
		{
			BestTarget = Candidate;
			BestDistanceSquared = DistanceSquared;
		}
	}
	return BestTarget;
}

void AW11Character::RefreshPresentationDefinitions()
{
	const AW11PlayerState* W11PlayerState = GetPlayerState<AW11PlayerState>();
	if (!W11PlayerState)
	{
		return;
	}

	const FName HeroId = W11PlayerState->GetSelectedHeroId();
	if (HeroId != LoadedHeroId)
	{
		LoadedHeroId = HeroId;
		ActiveAnimationSet = nullptr;
		if (UW11HeroDefinition* Hero = LoadW11PrimaryDefinition<UW11HeroDefinition>(TEXT("W11Hero"), HeroId))
		{
			ActiveAnimationSet = Hero->CombatAnimations.LoadSynchronous();
		}
		if (ActiveAnimationSet)
		{
			FlipbookComponent->SetRelativeScale3D(FVector(ActiveAnimationSet->WorldScale));
			PrototypeSpriteComponent->SetVisibility(false);
			PlayCharacterAction(EW11CharacterAction::Idle, 0.0f);
			UE_LOG(LogWorldWalkerW11, Display,
				TEXT("W11_CHARACTER_ANIMATION_LOADED Hero=%s Set=%s"),
				*HeroId.ToString(), *ActiveAnimationSet->DefinitionId.ToString());
		}
	}

	const FName AbilityId = W11PlayerState->GetActiveAbilityId(EW11AbilitySlot::Active1);
	LoadSkillVisual(AbilityId);
}

void AW11Character::LoadSkillVisual(const FName AbilityId)
{
	if (AbilityId != LoadedAbilityId)
	{
		LoadedAbilityId = AbilityId;
		ActiveSkillVisual = nullptr;
		if (UW11AbilityDefinition* Ability = LoadW11PrimaryDefinition<UW11AbilityDefinition>(TEXT("W11Ability"), AbilityId))
		{
			ActiveSkillVisual = Ability->Visuals.LoadSynchronous();
		}
		if (ActiveSkillVisual)
		{
			UE_LOG(LogWorldWalkerW11, Display,
				TEXT("W11_SKILL_VISUAL_LOADED Ability=%s Visual=%s"),
				*AbilityId.ToString(), *ActiveSkillVisual->DefinitionId.ToString());
		}
	}
}

void AW11Character::UpdatePresentation(const float DeltaSeconds)
{
	(void)DeltaSeconds;
	if (!ActiveAnimationSet || !GetWorld())
	{
		return;
	}

	if (CombatComponent->GetAbilitySequence() != ObservedAbilitySequence)
	{
		ObservedAbilitySequence = CombatComponent->GetAbilitySequence();
		UpdateFacingFromDirection(CombatComponent->GetLastAttackDirection());
		PlayCharacterAction(EW11CharacterAction::Cast, 0.52f);
		LoadSkillVisual(CombatComponent->GetLastExecutedAbilityId());
		PlayActiveAbilityPresentation();
	}
	else if (CombatComponent->GetAttackSequence() != ObservedAttackSequence)
	{
		ObservedAttackSequence = CombatComponent->GetAttackSequence();
		UpdateFacingFromDirection(CombatComponent->GetLastAttackDirection());
		PlayCharacterAction(EW11CharacterAction::Cast, 0.32f);
		if (UW11AbilityDefinition* BasicAttack = LoadW11PrimaryDefinition<UW11AbilityDefinition>(
			TEXT("W11Ability"), TEXT("Ability.BasicAttack")))
		{
			if (UW11SkillVisualDefinition* Visual = BasicAttack->Visuals.LoadSynchronous())
			{
				if (USoundBase* AttackSound = Visual->CastSound.LoadSynchronous())
				{
					UGameplayStatics::PlaySoundAtLocation(this, AttackSound, GetActorLocation());
				}
			}
		}
	}

	if (SkillEffectComponent->IsVisible() && !SkillEffectComponent->IsPlaying())
	{
		SkillEffectComponent->SetVisibility(false);
		SkillEffectComponent->SetFlipbook(nullptr);
	}

	if (GetWorld()->GetTimeSeconds() >= ActionLockUntil)
	{
		const FVector Velocity = GetVelocity();
		if (Velocity.SizeSquared2D() > 9.0f)
		{
			UpdateFacingFromDirection(FVector2D(Velocity.Y, Velocity.X));
		}
		const EW11CharacterAction Desired = Velocity.SizeSquared2D() > 36.0f
			? EW11CharacterAction::Move : EW11CharacterAction::Idle;
		if (Desired != CurrentAction)
		{
			PlayCharacterAction(Desired, 0.0f);
		}
	}
}

void AW11Character::UpdateFacingFromDirection(const FVector2D& Direction)
{
	const EW11FacingDirection PreviousFacing = FacingDirection;
	if (FMath::Abs(Direction.X) > 0.2f)
	{
		LastHorizontalFacing = Direction.X < 0.0f
			? EW11FacingDirection::Left : EW11FacingDirection::Right;
		FacingDirection = LastHorizontalFacing;
	}
	else if (FMath::Abs(Direction.Y) > 0.2f)
	{
		FacingDirection = Direction.Y < 0.0f
			? EW11FacingDirection::Down : EW11FacingDirection::Up;
	}

	if (FacingDirection != PreviousFacing && ActiveAnimationSet)
	{
		if (UPaperFlipbook* FacingFlipbook = ResolveActionFlipbook(CurrentAction);
			FacingFlipbook && FlipbookComponent->GetFlipbook() != FacingFlipbook)
		{
			FlipbookComponent->SetFlipbook(FacingFlipbook);
			FlipbookComponent->PlayFromStart();
		}
	}
}

void AW11Character::PlayCharacterAction(const EW11CharacterAction Action, const float LockDuration)
{
	UPaperFlipbook* Flipbook = ResolveActionFlipbook(Action);
	if (!Flipbook)
	{
		return;
	}
	CurrentAction = Action;
	if (FlipbookComponent->GetFlipbook() != Flipbook)
	{
		FlipbookComponent->SetFlipbook(Flipbook);
	}
	const bool bLoop = Action == EW11CharacterAction::Idle || Action == EW11CharacterAction::Move;
	FlipbookComponent->SetLooping(bLoop);
	FlipbookComponent->PlayFromStart();
	if (GetWorld() && LockDuration > 0.0f)
	{
		ActionLockUntil = GetWorld()->GetTimeSeconds() + LockDuration;
	}
}

UPaperFlipbook* AW11Character::ResolveActionFlipbook(const EW11CharacterAction Action) const
{
	if (!ActiveAnimationSet)
	{
		return nullptr;
	}
	const FW11CharacterActionFlipbooks* Set = nullptr;
	switch (FacingDirection)
	{
	case EW11FacingDirection::Left:
		Set = &ActiveAnimationSet->Left;
		break;
	case EW11FacingDirection::Up:
		Set = &ActiveAnimationSet->Up;
		break;
	case EW11FacingDirection::Down:
		Set = &ActiveAnimationSet->Down;
		break;
	case EW11FacingDirection::Right:
	default:
		Set = &ActiveAnimationSet->Right;
		break;
	}
	UPaperFlipbook* Result = SelectActionFlipbook(*Set, Action);
	if (!Result && ActiveAnimationSet->bKeepLastHorizontalFacingForVerticalMovement)
	{
		Result = SelectActionFlipbook(
			LastHorizontalFacing == EW11FacingDirection::Left
				? ActiveAnimationSet->Left : ActiveAnimationSet->Right,
			Action);
	}
	return Result;
}

void AW11Character::PlayActiveAbilityPresentation()
{
	if (!ActiveSkillVisual)
	{
		return;
	}
	if (USoundBase* CastSound = ActiveSkillVisual->CastSound.LoadSynchronous())
	{
		UGameplayStatics::PlaySoundAtLocation(this, CastSound, GetActorLocation());
	}
	UPaperFlipbook* Effect = ActiveSkillVisual->EffectFlipbook.LoadSynchronous();
	if (!Effect)
	{
		return;
	}
	const FVector2D Direction2D = CombatComponent->GetLastAttackDirection().GetSafeNormal();
	const FVector WorldDirection(Direction2D.Y, Direction2D.X, 0.0f);
	SkillEffectComponent->SetFlipbook(Effect);
	SkillEffectComponent->SetLooping(false);
	SkillEffectComponent->SetRelativeScale3D(FVector(ActiveSkillVisual->WorldScale));
	SkillEffectComponent->SetTranslucentSortPriority(ActiveSkillVisual->TranslucentSortPriority);
	SkillEffectComponent->SetRelativeLocation(
		WorldDirection * ActiveSkillVisual->LocalOffset.X
		+ FVector(0.0f, 0.0f, ActiveSkillVisual->LocalOffset.Y));
	if (ActiveSkillVisual->bDirectional)
	{
		const float Angle = FMath::Atan2(-WorldDirection.X, WorldDirection.Y);
		const FQuat BaseRotation = FRotator(0.0f, 90.0f, -90.0f).Quaternion();
		SkillEffectComponent->SetRelativeRotation(FQuat(FVector::UpVector, Angle) * BaseRotation);
	}
	else
	{
		SkillEffectComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, -90.0f));
	}
	SkillEffectComponent->SetVisibility(true);
	SkillEffectComponent->PlayFromStart();
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_SKILL_VISUAL_PLAYED Hero=%s Ability=%s Visual=%s Direction=%.2f,%.2f"),
		*LoadedHeroId.ToString(), *LoadedAbilityId.ToString(),
		*ActiveSkillVisual->DefinitionId.ToString(), Direction2D.X, Direction2D.Y);
}

void AW11Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("W11MoveVertical"), this, &AW11Character::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("W11MoveHorizontal"), this, &AW11Character::MoveRight);
	PlayerInputComponent->BindAction(TEXT("W11Dodge"), IE_Pressed, this, &AW11Character::TryDodge);
	PlayerInputComponent->BindAction(TEXT("W11BasicAttack"), IE_Pressed, this, &AW11Character::AttackFacing);
	PlayerInputComponent->BindAction(TEXT("W11Active1"), IE_Pressed, this, &AW11Character::CastInitialAbility);
	PlayerInputComponent->BindAction(TEXT("W11InitialAbility"), IE_Pressed, this, &AW11Character::CastInitialAbility);
	PlayerInputComponent->BindAction(TEXT("W11Active2"), IE_Pressed, this, &AW11Character::CastAbilitySlot2);
	PlayerInputComponent->BindAction(TEXT("W11Active3"), IE_Pressed, this, &AW11Character::CastAbilitySlot3);
	PlayerInputComponent->BindAction(TEXT("W11Active4"), IE_Pressed, this, &AW11Character::CastAbilitySlot4);
}

float AW11Character::ResolveArenaCameraWidth(const int32 ChapterIndex, const int32 StageIndex)
{
	// The first authored battle uses a larger arena/readability budget. Stage 1
	// remains a compatibility fallback for older entry flows without chapter data.
	return ChapterIndex == 1 || (ChapterIndex <= 0 && StageIndex == 1) ? 3000.0f : 2200.0f;
}

void AW11Character::MoveForward(const float Value)
{
	MoveVerticalInput = FMath::Clamp(Value, -1.0f, 1.0f);
	if (!FMath::IsNearlyZero(MoveVerticalInput))
	{
		UpdateFacingFromDirection(FVector2D(0.0f, MoveVerticalInput));
		if (!IsDodging())
		{
			AddMovementInput(FVector::ForwardVector, MoveVerticalInput);
		}
	}
	else if (!IsDodging() && FMath::IsNearlyZero(MoveHorizontalInput))
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
}

void AW11Character::MoveRight(const float Value)
{
	MoveHorizontalInput = FMath::Clamp(Value, -1.0f, 1.0f);
	if (!FMath::IsNearlyZero(MoveHorizontalInput))
	{
		UpdateFacingFromDirection(FVector2D(MoveHorizontalInput, 0.0f));
		if (!IsDodging())
		{
			AddMovementInput(FVector::RightVector, MoveHorizontalInput);
		}
	}
	else if (!IsDodging() && FMath::IsNearlyZero(MoveVerticalInput))
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
}

void AW11Character::TryDodge()
{
	if (!IsLocallyControlled() || !GetWorld())
	{
		return;
	}
	const AW11GameState* State = GetWorld()->GetGameState<AW11GameState>();
	if (!State || !AW11GameState::IsPendingCombatEncounter(
		State->GetRunPhase(), State->GetCombatOutcome(),
		State->GetEncounterInstanceId(), State->GetEncounterInstanceId()))
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < LocalNextDodgeAllowedTime || IsDodging())
	{
		return;
	}

	const FVector Direction = ResolveDodgeDirection();
	LocalNextDodgeAllowedTime = Now + DodgeCooldown;
	if (HasAuthority())
	{
		ServerTryDodge_Implementation(Direction);
	}
	else
	{
		bLocalDodgeActive = true;
		PerformDodgeMovement(Direction);
		PlayCharacterAction(EW11CharacterAction::Dodge, DodgeDuration);
		GetWorldTimerManager().SetTimer(
			DodgeTimerHandle, this, &AW11Character::FinishDodge, DodgeDuration, false);
		ServerTryDodge(Direction);
	}
}

void AW11Character::ResetForNewMemoryRun()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(DodgeTimerHandle);
	GetWorldTimerManager().ClearTimer(DodgePositionCheckTimerHandle);
	bDodgeActive = false;
	NextDodgeAllowedTime = -1.0f;
	DodgeCooldownEndTime = 0.0f;
	DodgeLandingLocation = GetActorLocation();
	if (CombatComponent)
	{
		CombatComponent->ResetForNewMemoryRun();
	}
	ResetLocalTransientCombatState();
	ClientResetForNewMemoryRun();
	ForceNetUpdate();
}

void AW11Character::ClientResetForNewMemoryRun_Implementation()
{
	ResetLocalTransientCombatState();
}

void AW11Character::ResetLocalTransientCombatState()
{
	GetWorldTimerManager().ClearTimer(DodgeTimerHandle);
	GetWorldTimerManager().ClearTimer(DodgePositionCheckTimerHandle);
	GetWorldTimerManager().ClearTimer(LocalHitStopTimerHandle);
	bLocalDodgeActive = false;
	LocalNextDodgeAllowedTime = -1.0f;
	ActionLockUntil = -1.0f;
	MoveVerticalInput = 0.0f;
	MoveHorizontalInput = 0.0f;
	LocalCameraImpactRemaining = 0.0f;
	LocalCameraImpactDuration = 0.0f;
	LocalCameraImpactStrength = 0.0f;
	FinishLocalHitStop();
	if (TopDownCamera)
	{
		TopDownCamera->SetRelativeLocation(BaseCameraRelativeLocation);
	}
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
	OnDodgeCooldownChanged.Broadcast(0.0f);
}

void AW11Character::ServerTryDodge_Implementation(const FVector_NetQuantizeNormal Direction)
{
	if (!GetWorld())
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	AW11PlayerState* W11PlayerState = GetPlayerState<AW11PlayerState>();
	UW11AttributeComponent* Attributes = W11PlayerState ? W11PlayerState->GetAttributeComponent() : nullptr;
	const AW11GameState* State = GetWorld()->GetGameState<AW11GameState>();
	const AW11GameMode* GameMode = GetWorld()->GetAuthGameMode<AW11GameMode>();
	const bool bCombatRequestAllowed = State && GameMode
		&& GameMode->IsCombatRequestAllowed(State->GetEncounterInstanceId());
	if (!Attributes || !IsValidDodgeRequest(
		FVector(Direction), bCombatRequestAllowed, bDodgeActive,
		Attributes->IsDefeated(), Now, NextDodgeAllowedTime))
	{
		return;
	}

	const FVector SafeDirection = FVector(Direction.X, Direction.Y, 0.0f).GetSafeNormal();
	if (SafeDirection.IsNearlyZero())
	{
		return;
	}

	NextDodgeAllowedTime = Now + DodgeCooldown;
	DodgeCooldownEndTime = NextDodgeAllowedTime;
	OnDodgeCooldownChanged.Broadcast(DodgeCooldownEndTime);
	bDodgeActive = true;
	LastDodgeDirection = SafeDirection;
	UpdateFacingFromDirection(FVector2D(SafeDirection.Y, SafeDirection.X));
	++DodgeSequence;
	Attributes->GrantDamageImmunity(DodgeInvulnerabilityDuration);
	if (AW11GameMode* MutableGameMode = GetWorld()->GetAuthGameMode<AW11GameMode>())
	{
		MutableGameMode->RecordDodge();
	}
	const float ActualDistance = PerformDodgeMovement(SafeDirection);
	DodgeLandingLocation = GetActorLocation();
	PlayCharacterAction(EW11CharacterAction::Dodge, DodgeDuration);
	GetWorldTimerManager().SetTimer(
		DodgeTimerHandle, this, &AW11Character::FinishDodge, DodgeDuration, false);
	ForceNetUpdate();
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_DODGE_STARTED Player=%d Hero=%s Sequence=%d Direction=%.2f,%.2f Distance=%.0f/%.0f Landing=%.2f,%.2f,%.2f Cooldown=%.2f Invulnerability=%.2f"),
		W11PlayerState->GetPlayerId(), *LoadedHeroId.ToString(), DodgeSequence, SafeDirection.X, SafeDirection.Y,
		ActualDistance, DodgeDistance, DodgeLandingLocation.X, DodgeLandingLocation.Y, DodgeLandingLocation.Z,
		DodgeCooldown, DodgeInvulnerabilityDuration);
}

bool AW11Character::IsValidDodgeRequest(
	const FVector& Direction,
	const bool bCombatRequestAllowed,
	const bool bDodgeAlreadyActive,
	const bool bDefeated,
	const float CurrentServerTime,
	const float NextAllowedServerTime)
{
	const float DirectionLengthSquared = Direction.SizeSquared2D();
	return bCombatRequestAllowed
		&& !bDodgeAlreadyActive
		&& !bDefeated
		&& CurrentServerTime + UE_KINDA_SMALL_NUMBER >= NextAllowedServerTime
		&& FMath::IsFinite(Direction.X)
		&& FMath::IsFinite(Direction.Y)
		&& FMath::IsFinite(Direction.Z)
		&& FMath::Abs(Direction.Z) <= 0.02f
		&& DirectionLengthSquared >= 0.98f * 0.98f
		&& DirectionLengthSquared <= 1.02f * 1.02f;
}

float AW11Character::PerformDodgeMovement(const FVector& Direction)
{
	const FVector SafeDirection = FVector(Direction.X, Direction.Y, 0.0f).GetSafeNormal();
	if (SafeDirection.IsNearlyZero())
	{
		return 0.0f;
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	const FVector StartLocation = GetActorLocation();
	FHitResult SweepHit;
	SetActorLocation(
		StartLocation + SafeDirection * DodgeDistance,
		true,
		&SweepHit,
		ETeleportType::None);
	return FVector::Dist2D(StartLocation, GetActorLocation());
}

void AW11Character::FinishDodge()
{
	bLocalDodgeActive = false;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->Velocity = FVector::ZeroVector;
	}
	if (HasAuthority())
	{
		bDodgeActive = false;
		ForceNetUpdate();
	}
}

FVector AW11Character::ResolveDodgeDirection() const
{
	FVector Direction = GetLastMovementInputVector();
	if (Direction.SizeSquared2D() <= UE_KINDA_SMALL_NUMBER)
	{
		Direction = GetVelocity();
	}
	if (Direction.SizeSquared2D() > UE_KINDA_SMALL_NUMBER)
	{
		return FVector(Direction.X, Direction.Y, 0.0f).GetSafeNormal();
	}

	switch (FacingDirection)
	{
	case EW11FacingDirection::Left: return -FVector::RightVector;
	case EW11FacingDirection::Up: return FVector::ForwardVector;
	case EW11FacingDirection::Down: return -FVector::ForwardVector;
	case EW11FacingDirection::Right:
	default: return FVector::RightVector;
	}
}

FVector2D AW11Character::ResolveFacingCombatDirection() const
{
	const FVector MovementDirection = ResolveDodgeDirection();
	return FVector2D(MovementDirection.Y, MovementDirection.X).GetSafeNormal();
}

void AW11Character::OnRep_DodgeSequence()
{
	const bool bLocallyControlled = IsLocallyControlled();
	if (!bLocallyControlled)
	{
		const FVector Direction = LastDodgeDirection;
		UpdateFacingFromDirection(FVector2D(Direction.Y, Direction.X));
		PlayCharacterAction(EW11CharacterAction::Dodge, DodgeDuration);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("W11NetworkSmoke")))
	{
		const AW11PlayerState* W11PlayerState = GetPlayerState<AW11PlayerState>();
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_DODGE_SNAPSHOT Player=%d Hero=%s Sequence=%d Owner=%d Direction=%.2f,%.2f Landing=%.2f,%.2f,%.2f Duration=%.2f Cooldown=%.2f Invulnerability=%.2f"),
			W11PlayerState ? W11PlayerState->GetPlayerId() : 0,
			*LoadedHeroId.ToString(), DodgeSequence, bLocallyControlled ? 1 : 0,
			LastDodgeDirection.X, LastDodgeDirection.Y,
			DodgeLandingLocation.X, DodgeLandingLocation.Y, DodgeLandingLocation.Z,
			DodgeDuration, DodgeCooldown, DodgeInvulnerabilityDuration);

		const FTimerDelegate PositionCheckDelegate = FTimerDelegate::CreateUObject(
			this, &AW11Character::CheckReplicatedDodgeLanding,
			DodgeSequence, FVector(DodgeLandingLocation));
		GetWorldTimerManager().SetTimer(
			DodgePositionCheckTimerHandle, PositionCheckDelegate,
			FMath::Max(0.25f, DodgeDuration + 0.15f), false);
	}
}

void AW11Character::OnRep_DodgeCooldownEndTime()
{
	OnDodgeCooldownChanged.Broadcast(DodgeCooldownEndTime);
}

void AW11Character::CheckReplicatedDodgeLanding(
	const int32 ExpectedSequence,
	const FVector ExpectedLanding)
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("W11NetworkSmoke")))
	{
		return;
	}

	const AW11PlayerState* W11PlayerState = GetPlayerState<AW11PlayerState>();
	const FVector ActualLocation = GetActorLocation();
	const float PositionError = FVector::Dist(ActualLocation, ExpectedLanding);
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_DODGE_POSITION_CHECK Player=%d Hero=%s Sequence=%d Owner=%d CurrentSequence=%d Expected=%.2f,%.2f,%.2f Actual=%.2f,%.2f,%.2f Error=%.2f"),
		W11PlayerState ? W11PlayerState->GetPlayerId() : 0,
		*LoadedHeroId.ToString(), ExpectedSequence, IsLocallyControlled() ? 1 : 0, DodgeSequence,
		ExpectedLanding.X, ExpectedLanding.Y, ExpectedLanding.Z,
		ActualLocation.X, ActualLocation.Y, ActualLocation.Z, PositionError);
}

void AW11Character::AttackUp()
{
	CombatComponent->TryBasicAttack(FVector2D(0.0f, 1.0f));
}

void AW11Character::AttackFacing()
{
	CombatComponent->TryBasicAttack(ResolveFacingCombatDirection());
}

void AW11Character::AttackDown()
{
	CombatComponent->TryBasicAttack(FVector2D(0.0f, -1.0f));
}

void AW11Character::AttackLeft()
{
	CombatComponent->TryBasicAttack(FVector2D(-1.0f, 0.0f));
}

void AW11Character::AttackRight()
{
	CombatComponent->TryBasicAttack(FVector2D(1.0f, 0.0f));
}

void AW11Character::CastInitialAbility()
{
	CombatComponent->TryInitialAbility(ResolveFacingCombatDirection());
}

void AW11Character::CastAbilitySlot2()
{
	CombatComponent->TryAbilitySlot(EW11AbilitySlot::Active2, ResolveFacingCombatDirection());
}

void AW11Character::CastAbilitySlot3()
{
	CombatComponent->TryAbilitySlot(EW11AbilitySlot::Active3, ResolveFacingCombatDirection());
}

void AW11Character::CastAbilitySlot4()
{
	CombatComponent->TryAbilitySlot(EW11AbilitySlot::Active4, ResolveFacingCombatDirection());
}
