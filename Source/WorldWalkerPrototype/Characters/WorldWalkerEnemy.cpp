#include "Characters/WorldWalkerEnemy.h"

#include "Animation/AnimSequence.h"
#include "Cards/Fantasy/FantasyEnemyDefinition.h"
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

void AWorldWalkerEnemy::ConfigureFantasyPresentation(
	const FText& EnemyName,
	const EFantasyEnemyVisualProfile VisualProfile)
{
	InteractionPrompt->InitWidget();
	InteractionPrompt->SetVisibility(true, true);
	InteractionPrompt->SetHiddenInGame(false, true);
	if (UWorldWalkerNPCPromptWidget* Prompt =
		Cast<UWorldWalkerNPCPromptWidget>(InteractionPrompt->GetUserWidgetObject()))
	{
		Prompt->SetPrompt(EnemyName, FText::FromString(TEXT("按 E 挑战")));
	}

	bFantasyDeathPlaying = false;
	bFantasyActionPlaying = false;
	FantasyActionEndTime = 0.0f;
	bFantasyPresentationActive = LoadFantasyPresentationAssets(VisualProfile);
	if (bFantasyPresentationActive)
	{
		FantasyEnemyMesh->SetVisibility(true, true);
		FantasyEnemyMesh->SetHiddenInGame(false, true);
		PrototypeBody->SetVisibility(false, true);
		PrototypeBody->SetHiddenInGame(true, true);
		ReturnToFantasyIdle();
	}
	else
	{
		FantasyEnemyMesh->SetVisibility(false, true);
		FantasyEnemyMesh->SetHiddenInGame(true, true);
		PrototypeBody->SetVisibility(true, true);
		PrototypeBody->SetHiddenInGame(false, true);
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("W01 enemy presentation unavailable. RequestedProfile=%d; prototype body restored."),
			static_cast<int32>(VisualProfile));
	}
}

bool AWorldWalkerEnemy::LoadFantasyPresentationAssets(
	const EFantasyEnemyVisualProfile RequestedProfile)
{
	struct FEnemyPresentationProfile
	{
		EFantasyEnemyVisualProfile Profile;
		const TCHAR* Name;
		const TCHAR* MeshPath;
		const TCHAR* IdlePath;
		const TCHAR* AttackPath;
		const TCHAR* EmpowerPath;
		const TCHAR* HitPath;
		const TCHAR* DeathPath;
		float TargetHeight;
		float TargetBottomZ;
		float PromptHeight;
	};

	static const FEnemyPresentationProfile Profiles[] =
	{
		{
			EFantasyEnemyVisualProfile::Warrior,
			TEXT("BlackthornWarrior"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Warrior/SK_W01_Warrior.SK_W01_Warrior"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Warrior/SK_W01_WarriorCharacterArmature_Idle_Attacking.SK_W01_WarriorCharacterArmature_Idle_Attacking"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Warrior/SK_W01_WarriorCharacterArmature_Sword_Attack.SK_W01_WarriorCharacterArmature_Sword_Attack"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Warrior/SK_W01_WarriorCharacterArmature_Idle_Weapon.SK_W01_WarriorCharacterArmature_Idle_Weapon"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Warrior/SK_W01_WarriorCharacterArmature_RecieveHit_Attacking.SK_W01_WarriorCharacterArmature_RecieveHit_Attacking"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Warrior/SK_W01_WarriorCharacterArmature_Death.SK_W01_WarriorCharacterArmature_Death"),
			178.0f,
			-112.0f,
			155.0f,
		},
		{
			EFantasyEnemyVisualProfile::Skeleton,
			TEXT("Skeleton"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Skeleton/SK_W01_Blackthorn.SK_W01_Blackthorn"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Skeleton/SK_W01_BlackthornSkeletonArmature_Skeleton_Idle.SK_W01_BlackthornSkeletonArmature_Skeleton_Idle"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Skeleton/SK_W01_BlackthornSkeletonArmature_Skeleton_Attack.SK_W01_BlackthornSkeletonArmature_Skeleton_Attack"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Skeleton/SK_W01_BlackthornSkeletonArmature_Skeleton_Spawn.SK_W01_BlackthornSkeletonArmature_Skeleton_Spawn"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Skeleton/SK_W01_BlackthornSkeletonArmature_Skeleton_Spawn.SK_W01_BlackthornSkeletonArmature_Skeleton_Spawn"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Skeleton/SK_W01_BlackthornSkeletonArmature_Skeleton_Death.SK_W01_BlackthornSkeletonArmature_Skeleton_Death"),
			214.0f,
			-112.0f,
			170.0f,
		},
		{
			EFantasyEnemyVisualProfile::Bat,
			TEXT("DrowsyBat"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Bat/SK_W01_Bat.SK_W01_Bat"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Bat/SK_W01_BatBatArmature_Bat_Flying.SK_W01_BatBatArmature_Bat_Flying"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Bat/SK_W01_BatBatArmature_Bat_Attack.SK_W01_BatBatArmature_Bat_Attack"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Bat/SK_W01_BatBatArmature_Bat_Attack2.SK_W01_BatBatArmature_Bat_Attack2"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Bat/SK_W01_BatBatArmature_Bat_Hit.SK_W01_BatBatArmature_Bat_Hit"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Bat/SK_W01_BatBatArmature_Bat_Death.SK_W01_BatBatArmature_Bat_Death"),
			120.0f,
			-12.0f,
			205.0f,
		},
		{
			EFantasyEnemyVisualProfile::Dragon,
			TEXT("DragonWhelp"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Dragon/SK_W01_Dragon.SK_W01_Dragon"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Dragon/SK_W01_DragonDragonArmature_Dragon_Flying.SK_W01_DragonDragonArmature_Dragon_Flying"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Dragon/SK_W01_DragonDragonArmature_Dragon_Attack.SK_W01_DragonDragonArmature_Dragon_Attack"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Dragon/SK_W01_DragonDragonArmature_Dragon_Attack2.SK_W01_DragonDragonArmature_Dragon_Attack2"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Dragon/SK_W01_DragonDragonArmature_Dragon_Hit.SK_W01_DragonDragonArmature_Dragon_Hit"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Dragon/SK_W01_DragonDragonArmature_Dragon_Death.SK_W01_DragonDragonArmature_Dragon_Death"),
			245.0f,
			-52.0f,
			255.0f,
		},
		{
			EFantasyEnemyVisualProfile::Slime,
			TEXT("ForestSlime"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Slime/SK_W01_Slime.SK_W01_Slime"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Slime/SK_W01_SlimeArmature_Slime_Idle.SK_W01_SlimeArmature_Slime_Idle"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Slime/SK_W01_SlimeArmature_Slime_Attack.SK_W01_SlimeArmature_Slime_Attack"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Slime/SK_W01_SlimeArmature_Slime_Walk.SK_W01_SlimeArmature_Slime_Walk"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Slime/SK_W01_SlimeArmature_Slime_Attack.SK_W01_SlimeArmature_Slime_Attack"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Monsters/Slime/SK_W01_SlimeArmature_Slime_Death.SK_W01_SlimeArmature_Slime_Death"),
			105.0f,
			-112.0f,
			105.0f,
		},
		{
			EFantasyEnemyVisualProfile::Wizard,
			TEXT("MagicApprentice"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Wizard/SK_W01_Wizard.SK_W01_Wizard"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Wizard/SK_W01_WizardCharacterArmature_Idle_Attacking.SK_W01_WizardCharacterArmature_Idle_Attacking"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Wizard/SK_W01_WizardCharacterArmature_Staff_Attack.SK_W01_WizardCharacterArmature_Staff_Attack"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Wizard/SK_W01_WizardCharacterArmature_Spell1.SK_W01_WizardCharacterArmature_Spell1"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Wizard/SK_W01_WizardCharacterArmature_RecieveHit_Attacking.SK_W01_WizardCharacterArmature_RecieveHit_Attacking"),
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/Wizard/SK_W01_WizardCharacterArmature_Death.SK_W01_WizardCharacterArmature_Death"),
			172.0f,
			-112.0f,
			155.0f,
		},
	};

	FantasyEnemyMesh->SetSkeletalMeshAsset(nullptr);
	FantasyIdleAnimation = nullptr;
	FantasyAttackAnimation = nullptr;
	FantasyEmpowerAnimation = nullptr;
	FantasyHitReactionAnimation = nullptr;
	FantasyDeathAnimation = nullptr;

	TArray<EFantasyEnemyVisualProfile, TInlineAllocator<3>> CandidateProfiles;
	CandidateProfiles.Add(RequestedProfile);
	CandidateProfiles.AddUnique(EFantasyEnemyVisualProfile::Warrior);
	CandidateProfiles.AddUnique(EFantasyEnemyVisualProfile::Skeleton);

	for (const EFantasyEnemyVisualProfile CandidateProfile : CandidateProfiles)
	{
		const FEnemyPresentationProfile* Profile = nullptr;
		for (const FEnemyPresentationProfile& Candidate : Profiles)
		{
			if (Candidate.Profile == CandidateProfile)
			{
				Profile = &Candidate;
				break;
			}
		}
		if (!Profile)
		{
			continue;
		}

		USkeletalMesh* PresentationMesh = LoadObject<USkeletalMesh>(nullptr, Profile->MeshPath);
		UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr, Profile->IdlePath);
		UAnimSequence* Attack = LoadObject<UAnimSequence>(nullptr, Profile->AttackPath);
		UAnimSequence* Empower = LoadObject<UAnimSequence>(nullptr, Profile->EmpowerPath);
		UAnimSequence* Hit = LoadObject<UAnimSequence>(nullptr, Profile->HitPath);
		UAnimSequence* Death = LoadObject<UAnimSequence>(nullptr, Profile->DeathPath);

		const bool bAllAssetsLoaded = PresentationMesh && Idle && Attack && Empower && Hit && Death;
		const bool bSkeletonsMatch = bAllAssetsLoaded && PresentationMesh->GetSkeleton()
			&& Idle->GetSkeleton() == PresentationMesh->GetSkeleton()
			&& Attack->GetSkeleton() == PresentationMesh->GetSkeleton()
			&& Empower->GetSkeleton() == PresentationMesh->GetSkeleton()
			&& Hit->GetSkeleton() == PresentationMesh->GetSkeleton()
			&& Death->GetSkeleton() == PresentationMesh->GetSkeleton();
		if (!bSkeletonsMatch)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("W01_PRESENTATION_PROFILE_REJECTED Owner=Enemy Profile=%s Reason=%s"),
				Profile->Name,
				bAllAssetsLoaded ? TEXT("skeleton-mismatch") : TEXT("missing"));
			continue;
		}

		FantasyEnemyMesh->SetSkeletalMeshAsset(PresentationMesh);
		FantasyIdleAnimation = Idle;
		FantasyAttackAnimation = Attack;
		FantasyEmpowerAnimation = Empower;
		FantasyHitReactionAnimation = Hit;
		FantasyDeathAnimation = Death;

		const FBoxSphereBounds MeshBounds = PresentationMesh->GetBounds();
		const float SourceHeight = FMath::Max(1.0f, MeshBounds.BoxExtent.Z * 2.0f);
		const float VisualScale = Profile->TargetHeight / SourceHeight;
		const float SourceBottomZ = MeshBounds.Origin.Z - MeshBounds.BoxExtent.Z;
		const float RelativeZ = Profile->TargetBottomZ - SourceBottomZ * VisualScale;
		FantasyEnemyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, RelativeZ));
		FantasyEnemyMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		FantasyEnemyMesh->SetRelativeScale3D(FVector(VisualScale));
		InteractionPrompt->SetRelativeLocation(FVector(0.0f, 0.0f, Profile->PromptHeight));

		UE_LOG(
			LogTemp,
			Display,
			TEXT("W01_ENEMY_PRESENTATION_READY Profile=%s Mesh=1 Animations=5/5 Fallback=%d Scale=%.3f BottomZ=%.1f"),
			Profile->Name,
			CandidateProfile != RequestedProfile ? 1 : 0,
			VisualScale,
			Profile->TargetBottomZ);
		return true;
	}

	return false;
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
		PlayFantasyAnimation(FantasyHitReactionAnimation, false, 1.2f);
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
