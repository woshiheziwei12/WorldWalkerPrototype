#include "Characters/WorldWalkerEnemy.h"

#include "Animation/AnimSequence.h"
#include "Combat/CombatantComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/WorldWalkerDialogueWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "WorldWalkerGameModeBase.h"

AWorldWalkerEnemy::AWorldWalkerEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(48.0f, 112.0f);
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = 0.0f;

	PrototypeBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeBody"));
	PrototypeBody->SetupAttachment(RootComponent);
	PrototypeBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PrototypeBody->SetRelativeScale3D(FVector(0.75f, 0.75f, 1.8f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		PrototypeBody->SetStaticMesh(CubeMesh.Object);
	}

	FantasyEnemyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FantasyEnemyMesh"));
	FantasyEnemyMesh->SetupAttachment(RootComponent);
	FantasyEnemyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FantasyEnemyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -112.0f));
	FantasyEnemyMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	// The source skeleton is about 5.1 metres tall. At 0.42 it reads as a
	// slightly oversized boss without swallowing the arena or camera view.
	FantasyEnemyMesh->SetRelativeScale3D(FVector(0.42f));
	FantasyEnemyMesh->SetVisibility(false, true);
	FantasyEnemyMesh->SetHiddenInGame(true, true);
	FantasyEnemyMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	InteractionPrompt = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionPrompt"));
	InteractionPrompt->SetupAttachment(RootComponent);
	InteractionPrompt->SetRelativeLocation(FVector(0.0f, 0.0f, 155.0f));
	InteractionPrompt->SetWidgetSpace(EWidgetSpace::Screen);
	InteractionPrompt->SetDrawSize(FVector2D(320.0f, 90.0f));
	InteractionPrompt->SetDrawAtDesiredSize(true);
	InteractionPrompt->SetCullDistance(900.0f);
	InteractionPrompt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionPrompt->SetWidgetClass(UWorldWalkerNPCPromptWidget::StaticClass());

	CombatantComponent = CreateDefaultSubobject<UCombatantComponent>(TEXT("CombatantComponent"));
}

void AWorldWalkerEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (UMaterialInterface* BaseMaterial = PrototypeBody->GetMaterial(0))
	{
		UMaterialInstanceDynamic* BodyMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		BodyMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.85f, 0.04f, 0.03f));
		PrototypeBody->SetMaterial(0, BodyMaterial);
	}

	InteractionPrompt->InitWidget();
	if (UWorldWalkerNPCPromptWidget* Prompt =
		Cast<UWorldWalkerNPCPromptWidget>(InteractionPrompt->GetUserWidgetObject()))
	{
		Prompt->SetPrompt(
			FText::FromString(TEXT("测试敌人")),
			FText::FromString(TEXT("按 E 挑战")));
	}
}

void AWorldWalkerEnemy::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bFantasyActionPlaying && !bFantasyDeathPlaying && GetWorld()
		&& GetWorld()->GetTimeSeconds() >= FantasyActionEndTime)
	{
		ReturnToFantasyIdle();
	}
}

void AWorldWalkerEnemy::SetInCombat(const bool bInCombat)
{
	InteractionPrompt->SetVisibility(!bInCombat, true);
	InteractionPrompt->SetHiddenInGame(bInCombat, true);
}

void AWorldWalkerEnemy::ConfigureFantasyPresentation(const FText& EnemyName)
{
	InteractionPrompt->InitWidget();
	if (UWorldWalkerNPCPromptWidget* Prompt =
		Cast<UWorldWalkerNPCPromptWidget>(InteractionPrompt->GetUserWidgetObject()))
	{
		Prompt->SetPrompt(EnemyName, FText::FromString(TEXT("按 E 挑战")));
	}

	static const TCHAR* FantasyEnemyMeshPath =
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Skeleton/SK_W01_Blackthorn.SK_W01_Blackthorn");
	if (USkeletalMesh* FantasyMesh = LoadObject<USkeletalMesh>(nullptr, FantasyEnemyMeshPath))
	{
		FantasyEnemyMesh->SetSkeletalMeshAsset(FantasyMesh);
		FantasyEnemyMesh->SetVisibility(true, true);
		FantasyEnemyMesh->SetHiddenInGame(false, true);
		PrototypeBody->SetVisibility(false, true);
		PrototypeBody->SetHiddenInGame(true, true);
		bFantasyPresentationActive = true;
		LoadFantasyAnimationAssets();
		ReturnToFantasyIdle();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("W01 fantasy enemy mesh is not imported yet: %s"), FantasyEnemyMeshPath);
	}
}

void AWorldWalkerEnemy::LoadFantasyAnimationAssets()
{
	static const TCHAR* IdlePath =
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Skeleton/SK_W01_BlackthornSkeletonArmature_Skeleton_Idle.SK_W01_BlackthornSkeletonArmature_Skeleton_Idle");
	static const TCHAR* AttackPath =
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Skeleton/SK_W01_BlackthornSkeletonArmature_Skeleton_Attack.SK_W01_BlackthornSkeletonArmature_Skeleton_Attack");
	static const TCHAR* EmpowerPath =
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Skeleton/SK_W01_BlackthornSkeletonArmature_Skeleton_Spawn.SK_W01_BlackthornSkeletonArmature_Skeleton_Spawn");
	static const TCHAR* DeathPath =
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Skeleton/SK_W01_BlackthornSkeletonArmature_Skeleton_Death.SK_W01_BlackthornSkeletonArmature_Skeleton_Death");

	FantasyIdleAnimation = LoadObject<UAnimSequence>(nullptr, IdlePath);
	FantasyAttackAnimation = LoadObject<UAnimSequence>(nullptr, AttackPath);
	FantasyEmpowerAnimation = LoadObject<UAnimSequence>(nullptr, EmpowerPath);
	FantasyDeathAnimation = LoadObject<UAnimSequence>(nullptr, DeathPath);

	if (!FantasyIdleAnimation || !FantasyAttackAnimation || !FantasyDeathAnimation)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("W01 Blackthorn animation set is incomplete; missing intent cues will fall back safely."));
	}
}

void AWorldWalkerEnemy::PlayFantasyAnimation(
	UAnimSequence* Animation,
	const bool bLooping,
	const float PlayRate)
{
	if (!bFantasyPresentationActive || !FantasyEnemyMesh->GetSkeletalMeshAsset() || !Animation)
	{
		return;
	}

	const float SafePlayRate = FMath::Max(0.1f, PlayRate);
	FantasyEnemyMesh->SetPlayRate(SafePlayRate);
	FantasyEnemyMesh->PlayAnimation(Animation, bLooping);

	if (!bLooping)
	{
		bFantasyActionPlaying = true;
		const float ActionDuration = FMath::Max(0.12f, Animation->GetPlayLength() / SafePlayRate);
		FantasyActionEndTime = GetWorld() ? GetWorld()->GetTimeSeconds() + ActionDuration : ActionDuration;
	}
}

void AWorldWalkerEnemy::ReturnToFantasyIdle()
{
	if (bFantasyDeathPlaying)
	{
		return;
	}

	bFantasyActionPlaying = false;
	PlayFantasyAnimation(FantasyIdleAnimation, true);
}

void AWorldWalkerEnemy::PlayIntentAnimation(const EWorldWalkerEnemyAnimationCue Cue)
{
	if (!bFantasyPresentationActive || (bFantasyDeathPlaying && Cue != EWorldWalkerEnemyAnimationCue::Death))
	{
		return;
	}

	switch (Cue)
	{
	case EWorldWalkerEnemyAnimationCue::Idle:
		ReturnToFantasyIdle();
		break;

	case EWorldWalkerEnemyAnimationCue::Attack:
		PlayFantasyAnimation(FantasyAttackAnimation, false, 1.05f);
		break;

	case EWorldWalkerEnemyAnimationCue::Empower:
		PlayFantasyAnimation(FantasyEmpowerAnimation, false, 1.0f);
		break;

	case EWorldWalkerEnemyAnimationCue::HitReact:
		// This CC0 skeleton has no dedicated hit sequence; a brisk spawn recoil is the safest visual fallback.
		PlayFantasyAnimation(FantasyEmpowerAnimation, false, 1.65f);
		break;

	case EWorldWalkerEnemyAnimationCue::Death:
		bFantasyDeathPlaying = true;
		bFantasyActionPlaying = false;
		InteractionPrompt->SetVisibility(false, true);
		InteractionPrompt->SetHiddenInGame(true, true);
		PlayFantasyAnimation(FantasyDeathAnimation, false, 1.0f);
		break;

	default:
		break;
	}
}

bool AWorldWalkerEnemy::CanInteract_Implementation(const AWorldWalkerCharacter* InteractingCharacter) const
{
	return InteractingCharacter != nullptr && CombatantComponent->IsAlive();
}

void AWorldWalkerEnemy::Interact_Implementation(AWorldWalkerCharacter* InteractingCharacter)
{
	if (!CanInteract_Implementation(InteractingCharacter))
	{
		return;
	}

	if (AWorldWalkerGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AWorldWalkerGameModeBase>())
	{
		GameMode->StartCombat(InteractingCharacter, this);
	}
}
