#include "Characters/WorldWalkerCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Cards/CardCombatComponent.h"
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
#include "UObject/UObjectGlobals.h"

AWorldWalkerCharacter::AWorldWalkerCharacter()
{
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
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AWorldWalkerCharacter::HandleJumpPressed);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &AWorldWalkerCharacter::HandleJumpReleased);
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

void AWorldWalkerCharacter::HandleJumpPressed()
{
	if (bCombatLocked)
	{
		return;
	}

	if (bExternalJumpHandlingEnabled && ExternalJumpPressed.IsBound())
	{
		ExternalJumpPressed.Broadcast();
		return;
	}
	Jump();
}

void AWorldWalkerCharacter::HandleJumpReleased()
{
	if (bExternalJumpHandlingEnabled && ExternalJumpReleased.IsBound())
	{
		ExternalJumpReleased.Broadcast();
		return;
	}
	StopJumping();
}

bool AWorldWalkerCharacter::ConfigureMainWorldAnimeForm()
{
	static const TCHAR* HeadMeshPath =
		TEXT("/Game/AnimeCharacters/Blueprints/Characters/Female_Average/Head/Starter/skl_AnimeF_Head_1.skl_AnimeF_Head_1");
	static const TCHAR* HumanAnimBlueprintPath =
		TEXT("/Game/AnimeCharacters/Animations/abp_Human.abp_Human_C");

	USkeletalMeshComponent* HeadComponent = GetMesh();
	USkeletalMesh* HeadMesh = LoadObject<USkeletalMesh>(nullptr, HeadMeshPath);
	UClass* HumanAnimClass = LoadClass<UAnimInstance>(nullptr, HumanAnimBlueprintPath);
	if (!HeadComponent || !HeadMesh || !HumanAnimClass)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Anime world form failed to load head or locomotion AnimBP. Head=%s AnimBP=%s"),
			HeadMesh ? TEXT("ok") : TEXT("missing"),
			HumanAnimClass ? TEXT("ok") : TEXT("missing"));
		return false;
	}

	HeadComponent->SetSkeletalMeshAsset(HeadMesh);
	HeadComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	HeadComponent->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	HeadComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	HeadComponent->SetAnimInstanceClass(HumanAnimClass);
	MainWorldLocomotionAnimClass = HumanAnimClass;
	HeadComponent->VisibilityBasedAnimTickOption =
		EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	if (!MainWorldAnimeTop)
	{
		MainWorldAnimeTop = CreateLinkedAnimePart(
			TEXT("MainWorldAvatarTop"),
			TEXT("/Game/AnimeCharacters/Blueprints/Characters/Female_Average/tops/Starter_TurtleNeck/skl_AnimeF_top_TurtleNeck.skl_AnimeF_top_TurtleNeck"),
			HeadComponent);
	}
	if (!MainWorldAnimeBottom)
	{
		MainWorldAnimeBottom = CreateLinkedAnimePart(
			TEXT("MainWorldAvatarBottom"),
			TEXT("/Game/AnimeCharacters/Blueprints/Characters/Female_Average/bottoms/Starter_Skirt/skl_AnimeF_bottom_skirt.skl_AnimeF_bottom_skirt"),
			HeadComponent);
	}
	if (!MainWorldAnimeHair)
	{
		MainWorldAnimeHair = CreateAnimeHairPart(HeadComponent);
	}

	bMainWorldAnimeFormConfigured =
		MainWorldAnimeTop != nullptr
		&& MainWorldAnimeBottom != nullptr
		&& MainWorldAnimeHair != nullptr;
	if (!bMainWorldAnimeFormConfigured)
	{
		bMainWorldAnimeFormActive = false;
		SetMainWorldAnimeFormVisibility(false);
		ApplyWorldFormVisibility();
		UE_LOG(LogTemp, Error, TEXT("Anime world form is incomplete; using the prototype body."));
		return false;
	}

	bMainWorldAnimeFormActive = true;
	bFantasyFormAvailable = false;
	bFantasyFormActive = false;
	bFantasyActionPlaying = false;
	CurrentFantasyAnimationState = EWorldWalkerFantasyAnimationState::None;
	ApplyWorldFormVisibility();
	return true;
}

void AWorldWalkerCharacter::RestoreMainWorldLocomotionAnimation()
{
	if (bFantasyFormAvailable && bFantasyFormActive
		&& FantasyFormMesh && FantasyFormMesh->GetSkeletalMeshAsset())
	{
		FantasyFormMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
		FantasyFormMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		FantasyFormMesh->SetRelativeScale3D(FantasyFormVisualScale);
		FantasyFormMesh->GlobalAnimRateScale = 1.0f;
		bFantasyActionPlaying = false;
		RefreshFantasyLocomotionAnimation(true);
		return;
	}

	USkeletalMeshComponent* HeadComponent = GetMesh();
	if (!bMainWorldAnimeFormConfigured || !bMainWorldAnimeFormActive || !HeadComponent)
	{
		return;
	}

	HeadComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	HeadComponent->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	HeadComponent->GlobalAnimRateScale = 1.0f;
	HeadComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	if (MainWorldLocomotionAnimClass)
	{
		HeadComponent->SetAnimInstanceClass(MainWorldLocomotionAnimClass);
	}

	for (USkeletalMeshComponent* Part : {MainWorldAnimeTop.Get(), MainWorldAnimeBottom.Get()})
	{
		if (Part)
		{
			Part->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
			Part->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		}
	}
}

void AWorldWalkerCharacter::SetMainWorldLedgeClimbPose(const float NormalizedTime)
{
	const bool bUseMedievalClimber = bFantasyFormAvailable && bFantasyFormActive
		&& FantasyFormMesh && FantasyFormMesh->GetSkeletalMeshAsset();
	const bool bUseAnimeForm = bMainWorldAnimeFormConfigured && bMainWorldAnimeFormActive;
	if (!bUseMedievalClimber && !bUseAnimeForm)
	{
		return;
	}

	const float ClampedTime = FMath::Clamp(NormalizedTime, 0.0f, 1.0f);
	const float PoseWeight = FMath::Sin(ClampedTime * PI);
	const float PullWeight = FMath::SmoothStep(0.30f, 0.82f, ClampedTime);
	const float PitchOffset = -22.0f * PoseWeight + 10.0f * PullWeight * PoseWeight;
	const float RollOffset = FMath::Sin(ClampedTime * 2.0f * PI) * 4.0f;
	const FVector LocationOffset(
		-7.0f * PoseWeight,
		0.0f,
		8.0f * PoseWeight + 5.0f * PullWeight * PoseWeight);

	auto ApplyPose = [&](USkeletalMeshComponent* Component)
	{
		if (!Component)
		{
			return;
		}

		Component->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f) + LocationOffset);
		Component->SetRelativeRotation(FRotator(PitchOffset, -90.0f, RollOffset));
	};

	if (bUseMedievalClimber)
	{
		FantasyFormMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f) + LocationOffset);
		FantasyFormMesh->SetRelativeRotation(FRotator(PitchOffset, -90.0f, RollOffset));
		FantasyFormMesh->GlobalAnimRateScale = PoseWeight > 0.01f ? 0.72f : 1.0f;
	}
	else
	{
		ApplyPose(GetMesh());
		ApplyPose(MainWorldAnimeTop);
		ApplyPose(MainWorldAnimeBottom);
		GetMesh()->GlobalAnimRateScale = PoseWeight > 0.01f ? 0.72f : 1.0f;
	}
}

USkeletalMeshComponent* AWorldWalkerCharacter::CreateLinkedAnimePart(
	const FName ComponentName,
	const TCHAR* MeshPath,
	USkeletalMeshComponent* PoseLeader)
{
	USkeletalMesh* LoadedMesh = LoadObject<USkeletalMesh>(nullptr, MeshPath);
	if (!PoseLeader || !LoadedMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("Anime avatar part failed to load: %s"), MeshPath);
		return nullptr;
	}

	USkeletalMeshComponent* Part = NewObject<USkeletalMeshComponent>(this, ComponentName);
	AddInstanceComponent(Part);
	Part->SetupAttachment(GetRootComponent());
	Part->SetSkeletalMeshAsset(LoadedMesh);
	Part->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	Part->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetCastShadow(true);
	Part->RegisterComponent();
	Part->SetLeaderPoseComponent(PoseLeader, true, false);
	return Part;
}

USkeletalMeshComponent* AWorldWalkerCharacter::CreateAnimeHairPart(
	USkeletalMeshComponent* HeadComponent)
{
	static const TCHAR* HairMeshPath =
		TEXT("/Game/AnimeCharacters/Blueprints/Characters/Female_Average/Hair/Starter2/skl_AnimeHair_F2.skl_AnimeHair_F2");
	USkeletalMesh* HairMesh = LoadObject<USkeletalMesh>(nullptr, HairMeshPath);
	if (!HeadComponent || !HairMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("Anime avatar hair failed to load: %s"), HairMeshPath);
		return nullptr;
	}

	USkeletalMeshComponent* Hair = NewObject<USkeletalMeshComponent>(this, TEXT("MainWorldAvatarHair"));
	AddInstanceComponent(Hair);
	Hair->SetupAttachment(HeadComponent, TEXT("HeadAttachment"));
	Hair->SetSkeletalMeshAsset(HairMesh);
	Hair->SetRelativeTransform(FTransform::Identity);
	Hair->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hair->SetCastShadow(true);
	Hair->RegisterComponent();
	return Hair;
}

void AWorldWalkerCharacter::ConfigureFantasyWorldForm(
	const bool bEnabled,
	const bool bStartInFantasyForm)
{
	bMainWorldAnimeFormActive = false;
	bFantasyFormAvailable = bEnabled;
	FantasyFormVisualScale = FVector(0.52f);
	FantasyFormMesh->SetRelativeScale3D(FantasyFormVisualScale);

	if (bFantasyFormAvailable && !FantasyFormMesh->GetSkeletalMeshAsset())
	{
		static const TCHAR* FantasyMeshPath =
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Warrior/SK_W01_Warrior.SK_W01_Warrior");
		if (USkeletalMesh* FantasyMesh = LoadObject<USkeletalMesh>(nullptr, FantasyMeshPath))
		{
			FantasyFormMesh->SetSkeletalMeshAsset(FantasyMesh);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("W01 fantasy player mesh is not imported yet: %s"), FantasyMeshPath);
		}
	}

	LoadFantasyAnimationAssets(
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Warrior"));

	bFantasyFormActive = bFantasyFormAvailable && bStartInFantasyForm;
	bWorldFormToggleEnabled = true;
	ApplyWorldFormVisibility();
	RefreshFantasyLocomotionAnimation(true);
}

bool AWorldWalkerCharacter::ConfigureSpiralTowerAnimeForm()
{
	if (!ConfigureMainWorldAnimeForm())
	{
		return false;
	}

	bWorldFormToggleEnabled = false;
	ApplyW02AnimeMaterialTuning();
	UE_LOG(LogTemp, Display, TEXT("W02 anime avatar configured. Model=W00Shared RimLightIntensity=0.00 ToonTint=0.035 FormToggle=Disabled"));
	return true;
}

void AWorldWalkerCharacter::ApplyW02AnimeMaterialTuning()
{
	USkeletalMeshComponent* AnimeComponents[] =
	{
		GetMesh(),
		MainWorldAnimeTop.Get(),
		MainWorldAnimeBottom.Get(),
		MainWorldAnimeHair.Get()
	};
	int32 TunedMaterialSlots = 0;
	for (USkeletalMeshComponent* Component : AnimeComponents)
	{
		if (!Component)
		{
			continue;
		}

		for (int32 MaterialIndex = 0; MaterialIndex < Component->GetNumMaterials(); ++MaterialIndex)
		{
			if (UMaterialInstanceDynamic* Material = Component->CreateAndSetMaterialInstanceDynamic(MaterialIndex))
			{
				Material->SetScalarParameterValue(TEXT("RimLight_Intensity"), 0.0f);
				Material->SetVectorParameterValue(
					TEXT("RimlightColor"),
					FLinearColor(0.01f, 0.015f, 0.025f, 1.0f));
				// The Anime Character Pack's toon masters route most surface color
				// through unlit-style parameters. A neutral sub-one tint reduces that
				// apparent emissive output without mutating W00's shared assets.
				Material->SetVectorParameterValue(
					TEXT("TintColor"),
					FLinearColor(0.035f, 0.035f, 0.04f, 1.0f));
				Material->SetVectorParameterValue(
					TEXT("HighlightColor"),
					FLinearColor(0.04f, 0.04f, 0.045f, 1.0f));
				Material->SetVectorParameterValue(
					TEXT("Color 1"),
					FLinearColor(0.055f, 0.032f, 0.025f, 1.0f));
				Material->SetVectorParameterValue(
					TEXT("Color 2"),
					FLinearColor(0.025f, 0.018f, 0.022f, 1.0f));
				Material->SetVectorParameterValue(
					TEXT("Color 3"),
					FLinearColor(0.012f, 0.014f, 0.02f, 1.0f));
				Material->SetVectorParameterValue(
					TEXT("ShadowColor"),
					FLinearColor(0.008f, 0.01f, 0.016f, 1.0f));
				Material->SetVectorParameterValue(
					TEXT("MidShadeColor"),
					FLinearColor(0.018f, 0.02f, 0.028f, 1.0f));
				Material->SetVectorParameterValue(
					TEXT("Hair Color"),
					FLinearColor(0.022f, 0.011f, 0.016f, 1.0f));
				Material->SetVectorParameterValue(
					TEXT("GradientColor"),
					FLinearColor(0.008f, 0.006f, 0.012f, 1.0f));
				Material->SetVectorParameterValue(
					TEXT("HairShadowColor"),
					FLinearColor(0.004f, 0.003f, 0.006f, 1.0f));
				Material->SetVectorParameterValue(
					TEXT("Color"),
					FLinearColor(0.035f, 0.035f, 0.04f, 1.0f));
				Material->SetVectorParameterValue(
					TEXT("Tint"),
					FLinearColor(0.025f, 0.035f, 0.055f, 1.0f));
				Material->SetScalarParameterValue(TEXT("Gradient Intensity"), 0.0f);
				++TunedMaterialSlots;
			}
		}
	}

	UE_LOG(LogTemp, Display, TEXT("W02 anime material tuning applied. Slots=%d RimLightIntensity=0.00 ToonTint=0.035"), TunedMaterialSlots);
}

void AWorldWalkerCharacter::ToggleWorldForm()
{
	if (bCombatLocked || !bFantasyFormAvailable || !bWorldFormToggleEnabled)
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
		bFantasyFormActive ? TEXT("Rune Knight") : TEXT("World Walker"));
}

void AWorldWalkerCharacter::ApplyWorldFormVisibility()
{
	const bool bCanShowFantasyMesh =
		bFantasyFormAvailable && bFantasyFormActive && FantasyFormMesh->GetSkeletalMeshAsset() != nullptr;
	const bool bCanShowMainWorldAnimeMesh =
		bMainWorldAnimeFormActive && bMainWorldAnimeFormConfigured;
	FantasyFormMesh->SetVisibility(bCanShowFantasyMesh, false);
	FantasyFormMesh->SetHiddenInGame(!bCanShowFantasyMesh, false);
	FantasyFormMesh->SetRenderInMainPass(bCanShowFantasyMesh);
	FantasyFormMesh->SetCastShadow(bCanShowFantasyMesh);
	SetMainWorldAnimeFormVisibility(bCanShowMainWorldAnimeMesh);
	const bool bShowPrototypeBody = !bCanShowFantasyMesh && !bCanShowMainWorldAnimeMesh;
	PrototypeBody->SetVisibility(bShowPrototypeBody, true);
	PrototypeBody->SetHiddenInGame(!bShowPrototypeBody, true);

	if (!bCanShowFantasyMesh)
	{
		CurrentFantasyAnimationState = EWorldWalkerFantasyAnimationState::None;
	}
}

void AWorldWalkerCharacter::SetMainWorldAnimeFormVisibility(const bool bVisible)
{
	if (USkeletalMeshComponent* HeadComponent = GetMesh())
	{
		HeadComponent->SetVisibility(bVisible, true);
		HeadComponent->SetHiddenInGame(!bVisible, true);
	}

	USkeletalMeshComponent* AnimeParts[] =
	{
		MainWorldAnimeTop.Get(),
		MainWorldAnimeBottom.Get(),
		MainWorldAnimeHair.Get()
	};
	for (USkeletalMeshComponent* Part : AnimeParts)
	{
		if (Part)
		{
			Part->SetVisibility(bVisible, true);
			Part->SetHiddenInGame(!bVisible, true);
		}
	}
}

void AWorldWalkerCharacter::LoadFantasyAnimationAssets(const TCHAR* AssetRoot)
{
	const auto AnimationPath = [AssetRoot](const TCHAR* AssetName)
	{
		return FString::Printf(TEXT("%s/%s.%s"), AssetRoot, AssetName, AssetName);
	};
	FantasyIdleAnimation = LoadObject<UAnimSequence>(
		nullptr,
		*AnimationPath(TEXT("SK_W01_WarriorCharacterArmature_Idle")));
	FantasyWalkAnimation = LoadObject<UAnimSequence>(
		nullptr,
		*AnimationPath(TEXT("SK_W01_WarriorCharacterArmature_Walk")));
	FantasyRunAnimation = LoadObject<UAnimSequence>(
		nullptr,
		*AnimationPath(TEXT("SK_W01_WarriorCharacterArmature_Run")));
	FantasyAttackAnimation = LoadObject<UAnimSequence>(
		nullptr,
		*AnimationPath(TEXT("SK_W01_WarriorCharacterArmature_Sword_Attack")));
	FantasyHitReactionAnimation = LoadObject<UAnimSequence>(
		nullptr,
		*AnimationPath(TEXT("SK_W01_WarriorCharacterArmature_RecieveHit")));

	if (!FantasyIdleAnimation || !FantasyWalkAnimation || !FantasyRunAnimation)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("W01 rune knight locomotion animation set is incomplete; missing sequences will fall back safely."));
	}
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

void AWorldWalkerCharacter::PlayFantasyHitReactionAnimation()
{
	PlayFantasyActionAnimation(
		FantasyHitReactionAnimation,
		EWorldWalkerFantasyAnimationState::HitReact,
		1.2f);
}
