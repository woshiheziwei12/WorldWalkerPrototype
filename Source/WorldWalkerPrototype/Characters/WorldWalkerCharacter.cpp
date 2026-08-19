#include "Characters/WorldWalkerCharacter.h"

#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Cards/CardCombatComponent.h"
#include "Cards/Fantasy/FantasyRunTypes.h"
#include "Combat/CombatantComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interaction/WorldWalkerInteractable.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AWorldWalkerCharacter::AWorldWalkerCharacter()
{
	RequestedFantasyProfession = EFantasyPlayerProfession::Knight;
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 88.0f);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 500.0f;
	GetCharacterMovement()->MaxWalkSpeed = 430.0f;
	GetCharacterMovement()->MaxAcceleration = 1250.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1400.0f;
	GetCharacterMovement()->GroundFriction = 7.0f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 120.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 450.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 90.0f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	PrototypeBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeBody"));
	PrototypeBody->SetupAttachment(RootComponent);
	PrototypeBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PrototypeBody->SetRelativeScale3D(FVector(0.45f, 0.45f, 1.65f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (BodyMesh.Succeeded())
	{
		PrototypeBody->SetStaticMesh(BodyMesh.Object);
	}

	FantasyFormMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FantasyFormMesh"));
	FantasyFormMesh->SetupAttachment(RootComponent);
	FantasyFormMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FantasyFormMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
	FantasyFormMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	// The Quaternius source character is authored at roughly 3.4 metres tall.
	// Scale only the visual mesh so the shared movement capsule remains stable.
	FantasyFormMesh->SetRelativeScale3D(FVector(0.52f));
	FantasyFormMesh->SetVisibility(false, true);
	FantasyFormMesh->SetHiddenInGame(true, true);
	FantasyFormMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	CombatantComponent = CreateDefaultSubobject<UCombatantComponent>(TEXT("CombatantComponent"));
	CardCombatComponent = CreateDefaultSubobject<UCardCombatComponent>(TEXT("CardCombatComponent"));
}

void AWorldWalkerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UMaterialInterface* BaseMaterial = PrototypeBody->GetMaterial(0))
	{
		UMaterialInstanceDynamic* BodyMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		BodyMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.05f, 0.35f, 1.0f));
		PrototypeBody->SetMaterial(0, BodyMaterial);
	}
}

void AWorldWalkerCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bFantasyFormAvailable || !bFantasyFormActive || !FantasyFormMesh->GetSkeletalMeshAsset())
	{
		return;
	}

	if (bFantasyActionPlaying)
	{
		if (GetWorld() && GetWorld()->GetTimeSeconds() >= FantasyActionEndTime)
		{
			bFantasyActionPlaying = false;
			RefreshFantasyLocomotionAnimation(true);
		}
		return;
	}

	RefreshFantasyLocomotionAnimation();
}

void AWorldWalkerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AWorldWalkerCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AWorldWalkerCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AWorldWalkerCharacter::TryInteract);
	PlayerInputComponent->BindAction(TEXT("ToggleWorldForm"), IE_Pressed, this, &AWorldWalkerCharacter::ToggleWorldForm);
}

void AWorldWalkerCharacter::MoveForward(const float Value)
{
	if (!bCombatLocked && Controller && !FMath::IsNearlyZero(Value))
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
	}
}

void AWorldWalkerCharacter::MoveRight(const float Value)
{
	if (!bCombatLocked && Controller && !FMath::IsNearlyZero(Value))
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
	}
}

void AWorldWalkerCharacter::TryInteract()
{
	if (bCombatLocked)
	{
		return;
	}

	AActor* NearestInteractable = nullptr;
	float NearestDistanceSquared = FMath::Square(InteractionDistance);

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Candidate = *It;
		if (Candidate == this || !Candidate->Implements<UWorldWalkerInteractable>())
		{
			continue;
		}
		if (!IWorldWalkerInteractable::Execute_CanInteract(Candidate, this))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
		if (DistanceSquared <= NearestDistanceSquared)
		{
			NearestInteractable = Candidate;
			NearestDistanceSquared = DistanceSquared;
		}
	}

	if (NearestInteractable)
	{
		IWorldWalkerInteractable::Execute_Interact(NearestInteractable, this);
	}
}

void AWorldWalkerCharacter::SetCombatLocked(const bool bLocked)
{
	bCombatLocked = bLocked;
	bFantasyActionPlaying = false;

	if (bLocked)
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	RefreshFantasyLocomotionAnimation(true);
}

void AWorldWalkerCharacter::ConfigureFantasyWorldForm(
	const bool bEnabled,
	const bool bStartInFantasyForm)
{
	bFantasyFormAvailable = bEnabled;

	if (bFantasyFormAvailable && !LoadFantasyPresentationAssets())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("W01 player presentation unavailable; the prototype body remains visible."));
	}

	bFantasyFormActive = bFantasyFormAvailable && bStartInFantasyForm;
	ApplyWorldFormVisibility();
	RefreshFantasyLocomotionAnimation(true);
}

void AWorldWalkerCharacter::ConfigureFantasyProfession(
	const EFantasyPlayerProfession Profession)
{
	RequestedFantasyProfession = Profession == EFantasyPlayerProfession::Mage
		? EFantasyPlayerProfession::Mage
		: EFantasyPlayerProfession::Knight;
	if (bFantasyFormAvailable)
	{
		LoadFantasyPresentationAssets();
		ApplyWorldFormVisibility();
		RefreshFantasyLocomotionAnimation(true);
	}
}

void AWorldWalkerCharacter::ToggleWorldForm()
{
	if (bCombatLocked || !bFantasyFormAvailable)
	{
		return;
	}

	bFantasyFormActive = !bFantasyFormActive;
	bFantasyActionPlaying = false;
	ApplyWorldFormVisibility();
	RefreshFantasyLocomotionAnimation(true);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01 active form changed to %s."),
		bFantasyFormActive
			? (RequestedFantasyProfession == EFantasyPlayerProfession::Mage
				? TEXT("Little Witch Wizard")
				: TEXT("Red Hood Knight"))
			: TEXT("World Walker"));
}

void AWorldWalkerCharacter::ApplyWorldFormVisibility()
{
	const bool bCanShowFantasyMesh =
		bFantasyFormAvailable && bFantasyFormActive && FantasyFormMesh->GetSkeletalMeshAsset() != nullptr;
	FantasyFormMesh->SetVisibility(bCanShowFantasyMesh, true);
	FantasyFormMesh->SetHiddenInGame(!bCanShowFantasyMesh, true);
	PrototypeBody->SetVisibility(!bCanShowFantasyMesh, true);
	PrototypeBody->SetHiddenInGame(bCanShowFantasyMesh, true);

	if (!bCanShowFantasyMesh)
	{
		CurrentFantasyAnimationState = EWorldWalkerFantasyAnimationState::None;
	}
}

bool AWorldWalkerCharacter::LoadFantasyPresentationAssets()
{
	struct FPlayerPresentationProfile
	{
		const TCHAR* Name;
		const TCHAR* MeshPath;
		const TCHAR* IdlePath;
		const TCHAR* WalkPath;
		const TCHAR* RunPath;
		const TCHAR* AttackPath;
		const TCHAR* UtilityPath;
		const TCHAR* SpellPath;
		const TCHAR* HitPath;
	};

	static const FPlayerPresentationProfile Profiles[] =
	{
		{
			TEXT("Wizard"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Wizard/SK_W01_Wizard.SK_W01_Wizard"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Wizard/SK_W01_WizardCharacterArmature_Idle.SK_W01_WizardCharacterArmature_Idle"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Wizard/SK_W01_WizardCharacterArmature_Walk.SK_W01_WizardCharacterArmature_Walk"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Wizard/SK_W01_WizardCharacterArmature_Run.SK_W01_WizardCharacterArmature_Run"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Wizard/SK_W01_WizardCharacterArmature_Staff_Attack.SK_W01_WizardCharacterArmature_Staff_Attack"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Wizard/SK_W01_WizardCharacterArmature_PickUp.SK_W01_WizardCharacterArmature_PickUp"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Wizard/SK_W01_WizardCharacterArmature_Spell1.SK_W01_WizardCharacterArmature_Spell1"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Wizard/SK_W01_WizardCharacterArmature_RecieveHit_Attacking.SK_W01_WizardCharacterArmature_RecieveHit_Attacking"),
		},
		{
			TEXT("Rogue"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Rogue/SK_W01_Rogue.SK_W01_Rogue"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Rogue/SK_W01_RogueCharacterArmature_Idle.SK_W01_RogueCharacterArmature_Idle"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Rogue/SK_W01_RogueCharacterArmature_Walk.SK_W01_RogueCharacterArmature_Walk"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Rogue/SK_W01_RogueCharacterArmature_Run.SK_W01_RogueCharacterArmature_Run"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Rogue/SK_W01_RogueCharacterArmature_Dagger_Attack.SK_W01_RogueCharacterArmature_Dagger_Attack"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Rogue/SK_W01_RogueCharacterArmature_Roll.SK_W01_RogueCharacterArmature_Roll"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Rogue/SK_W01_RogueCharacterArmature_PickUp.SK_W01_RogueCharacterArmature_PickUp"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Rogue/SK_W01_RogueCharacterArmature_RecieveHit.SK_W01_RogueCharacterArmature_RecieveHit"),
		},
		{
			TEXT("WarriorFallback"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Warrior/SK_W01_Warrior.SK_W01_Warrior"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Warrior/SK_W01_WarriorCharacterArmature_Idle.SK_W01_WarriorCharacterArmature_Idle"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Warrior/SK_W01_WarriorCharacterArmature_Walk.SK_W01_WarriorCharacterArmature_Walk"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Warrior/SK_W01_WarriorCharacterArmature_Run.SK_W01_WarriorCharacterArmature_Run"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Warrior/SK_W01_WarriorCharacterArmature_Sword_Attack.SK_W01_WarriorCharacterArmature_Sword_Attack"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Warrior/SK_W01_WarriorCharacterArmature_Roll.SK_W01_WarriorCharacterArmature_Roll"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Warrior/SK_W01_WarriorCharacterArmature_PickUp.SK_W01_WarriorCharacterArmature_PickUp"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Warrior/SK_W01_WarriorCharacterArmature_RecieveHit.SK_W01_WarriorCharacterArmature_RecieveHit"),
		},
	};

	FantasyFormMesh->SetSkeletalMeshAsset(nullptr);
	FantasyIdleAnimation = nullptr;
	FantasyWalkAnimation = nullptr;
	FantasyRunAnimation = nullptr;
	FantasyAttackAnimation = nullptr;
	FantasyUtilityAnimation = nullptr;
	FantasySpellAnimation = nullptr;
	FantasyHitReactionAnimation = nullptr;

	const int32 FirstProfileIndex = RequestedFantasyProfession == EFantasyPlayerProfession::Mage ? 0 : 1;
	for (int32 ProfileIndex = FirstProfileIndex; ProfileIndex < UE_ARRAY_COUNT(Profiles); ++ProfileIndex)
	{
		const FPlayerPresentationProfile& Profile = Profiles[ProfileIndex];
		USkeletalMesh* PresentationMesh = LoadObject<USkeletalMesh>(nullptr, Profile.MeshPath);
		UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr, Profile.IdlePath);
		UAnimSequence* Walk = LoadObject<UAnimSequence>(nullptr, Profile.WalkPath);
		UAnimSequence* Run = LoadObject<UAnimSequence>(nullptr, Profile.RunPath);
		UAnimSequence* Attack = LoadObject<UAnimSequence>(nullptr, Profile.AttackPath);
		UAnimSequence* Utility = LoadObject<UAnimSequence>(nullptr, Profile.UtilityPath);
		UAnimSequence* Spell = LoadObject<UAnimSequence>(nullptr, Profile.SpellPath);
		UAnimSequence* Hit = LoadObject<UAnimSequence>(nullptr, Profile.HitPath);

		const bool bAllAssetsLoaded = PresentationMesh && Idle && Walk && Run && Attack
			&& Utility && Spell && Hit;
		const bool bSkeletonsMatch = bAllAssetsLoaded && PresentationMesh->GetSkeleton()
			&& Idle->GetSkeleton() == PresentationMesh->GetSkeleton()
			&& Walk->GetSkeleton() == PresentationMesh->GetSkeleton()
			&& Run->GetSkeleton() == PresentationMesh->GetSkeleton()
			&& Attack->GetSkeleton() == PresentationMesh->GetSkeleton()
			&& Utility->GetSkeleton() == PresentationMesh->GetSkeleton()
			&& Spell->GetSkeleton() == PresentationMesh->GetSkeleton()
			&& Hit->GetSkeleton() == PresentationMesh->GetSkeleton();
		if (!bSkeletonsMatch)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("W01_PRESENTATION_PROFILE_REJECTED Owner=Player Profile=%s Reason=%s"),
				Profile.Name,
				bAllAssetsLoaded ? TEXT("skeleton-mismatch") : TEXT("missing"));
			continue;
		}

		FantasyFormMesh->SetSkeletalMeshAsset(PresentationMesh);
		FantasyIdleAnimation = Idle;
		FantasyWalkAnimation = Walk;
		FantasyRunAnimation = Run;
		FantasyAttackAnimation = Attack;
		FantasyUtilityAnimation = Utility;
		FantasySpellAnimation = Spell;
		FantasyHitReactionAnimation = Hit;
		FantasyFormMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
		FantasyFormMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		FantasyFormMesh->SetRelativeScale3D(FVector(0.52f));

		UE_LOG(
			LogTemp,
			Display,
			TEXT("W01_PLAYER_PRESENTATION_READY Profile=%s Mesh=1 Animations=7/7 Fallback=%d"),
			Profile.Name,
			ProfileIndex > FirstProfileIndex ? 1 : 0);
		return true;
	}

	return false;
}

void AWorldWalkerCharacter::RefreshFantasyLocomotionAnimation(const bool bForce)
{
	if (!bFantasyFormAvailable || !bFantasyFormActive || !FantasyFormMesh->GetSkeletalMeshAsset()
		|| bFantasyActionPlaying)
	{
		return;
	}

	EWorldWalkerFantasyAnimationState DesiredState = EWorldWalkerFantasyAnimationState::Idle;
	UAnimSequence* DesiredAnimation = FantasyIdleAnimation;

	if (!bCombatLocked)
	{
		const float GroundSpeed = GetVelocity().Size2D();
		if (GroundSpeed >= 320.0f)
		{
			DesiredState = EWorldWalkerFantasyAnimationState::Run;
			DesiredAnimation = FantasyRunAnimation ? FantasyRunAnimation.Get() : FantasyWalkAnimation.Get();
		}
		else if (GroundSpeed >= 10.0f)
		{
			DesiredState = EWorldWalkerFantasyAnimationState::Walk;
			DesiredAnimation = FantasyWalkAnimation ? FantasyWalkAnimation.Get() : FantasyIdleAnimation.Get();
		}
	}

	if (!DesiredAnimation)
	{
		CurrentFantasyAnimationState = DesiredState;
		return;
	}

	if (bForce || CurrentFantasyAnimationState != DesiredState)
	{
		FantasyFormMesh->SetPlayRate(1.0f);
		FantasyFormMesh->PlayAnimation(DesiredAnimation, true);
		CurrentFantasyAnimationState = DesiredState;
	}
}

void AWorldWalkerCharacter::PlayFantasyActionAnimation(
	UAnimSequence* Animation,
	const EWorldWalkerFantasyAnimationState ActionState,
	const float PlayRate)
{
	if (!bFantasyFormAvailable || !bFantasyFormActive || !FantasyFormMesh->GetSkeletalMeshAsset()
		|| !Animation)
	{
		return;
	}

	const float SafePlayRate = FMath::Max(0.1f, PlayRate);
	FantasyFormMesh->SetPlayRate(SafePlayRate);
	FantasyFormMesh->PlayAnimation(Animation, false);
	CurrentFantasyAnimationState = ActionState;
	bFantasyActionPlaying = true;

	const float ActionDuration = FMath::Max(0.12f, Animation->GetPlayLength() / SafePlayRate);
	FantasyActionEndTime = GetWorld() ? GetWorld()->GetTimeSeconds() + ActionDuration : ActionDuration;
}

void AWorldWalkerCharacter::PlayFantasyCardAttackAnimation()
{
	PlayFantasyActionAnimation(
		FantasyAttackAnimation,
		EWorldWalkerFantasyAnimationState::Attack,
		1.15f);
}

void AWorldWalkerCharacter::PlayFantasyCardUtilityAnimation()
{
	PlayFantasyActionAnimation(
		FantasyUtilityAnimation,
		EWorldWalkerFantasyAnimationState::Utility,
		1.18f);
}

void AWorldWalkerCharacter::PlayFantasyCardSpellAnimation()
{
	PlayFantasyActionAnimation(
		FantasySpellAnimation,
		EWorldWalkerFantasyAnimationState::Spell,
		1.08f);
}

void AWorldWalkerCharacter::PlayFantasyHitReactionAnimation()
{
	PlayFantasyActionAnimation(
		FantasyHitReactionAnimation,
		EWorldWalkerFantasyAnimationState::HitReact,
		1.2f);
}
