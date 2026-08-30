#include "Characters/WorldWalkerCharacter.h"

#include "Algo/AllOf.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Cards/CardCombatComponent.h"
#include "Cards/Fantasy/FantasyRunTypes.h"
#include "Combat/CombatantComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interaction/WorldWalkerInteractable.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectGlobals.h"
#include "UI/WorldWalkerMotionPickerWidget.h"

namespace
{
	struct FLumineBoneNamePair
	{
		const TCHAR* Source;
		const TCHAR* Target;
	};

	struct FPlayerGirlMotionDefinition
	{
		const TCHAR* AssetName;
		const TCHAR* DisplayName;
		EWorldWalkerMainMotionState State;
		bool bLoop;
	};

	constexpr FPlayerGirlMotionDefinition PlayerGirlMotionDefinitions[] =
	{
		{TEXT("Attack_01"), TEXT("普通攻击一段"), EWorldWalkerMainMotionState::Attack, false},
		{TEXT("Attack_02"), TEXT("普通攻击二段"), EWorldWalkerMainMotionState::Attack, false},
		{TEXT("Attack_03"), TEXT("普通攻击三段"), EWorldWalkerMainMotionState::Attack, false},
		{TEXT("Attack_04"), TEXT("普通攻击四段"), EWorldWalkerMainMotionState::Attack, false},
		{TEXT("Attack_05"), TEXT("普通攻击五段"), EWorldWalkerMainMotionState::Attack, false},
		{TEXT("ExtraAttack"), TEXT("额外攻击"), EWorldWalkerMainMotionState::Attack, false},
		{TEXT("FallingAttack_Loop"), TEXT("下落攻击 · 循环"), EWorldWalkerMainMotionState::Fall, true},
		{TEXT("FallingAttack_Strike"), TEXT("下落攻击 · 落地"), EWorldWalkerMainMotionState::Fall, false},
		{TEXT("Standby"), TEXT("待机"), EWorldWalkerMainMotionState::Idle, true},
		{TEXT("WalkCycle"), TEXT("行走"), EWorldWalkerMainMotionState::Walk, true},
		{TEXT("RunCycle"), TEXT("奔跑"), EWorldWalkerMainMotionState::Run, true},
		{TEXT("SprintCycle"), TEXT("冲刺跑"), EWorldWalkerMainMotionState::Sprint, true},
		{TEXT("RunBS"), TEXT("奔跑方向混合动作"), EWorldWalkerMainMotionState::Run, true},
		{TEXT("SprintBS"), TEXT("冲刺方向混合动作"), EWorldWalkerMainMotionState::Sprint, true},
		{TEXT("Jump"), TEXT("跳跃"), EWorldWalkerMainMotionState::Jump, false},
		{TEXT("FlyNormal"), TEXT("滑翔 / 空中"), EWorldWalkerMainMotionState::Fall, true},
		{TEXT("FallToGroundL"), TEXT("轻落地"), EWorldWalkerMainMotionState::LightLanding, false},
		{TEXT("FallToGroundH"), TEXT("重落地"), EWorldWalkerMainMotionState::HardLanding, false},
		{TEXT("CrouchRoll"), TEXT("翻滚"), EWorldWalkerMainMotionState::Roll, false},
		{TEXT("Standby"), TEXT("行走停止（安全待机）"), EWorldWalkerMainMotionState::LightStop, false},
		{TEXT("Standby"), TEXT("奔跑停止（安全待机）"), EWorldWalkerMainMotionState::MediumStop, false},
		{TEXT("Standby"), TEXT("冲刺停止（安全待机）"), EWorldWalkerMainMotionState::HardStop, false},
		{TEXT("Hit_L"), TEXT("轻受击"), EWorldWalkerMainMotionState::None, false},
		{TEXT("Hit_H"), TEXT("重受击"), EWorldWalkerMainMotionState::None, false},
		{TEXT("Death"), TEXT("倒下"), EWorldWalkerMainMotionState::None, false},
		{TEXT("SwimStandby"), TEXT("游泳待机"), EWorldWalkerMainMotionState::Idle, true},
		{TEXT("SwimF"), TEXT("向前游泳"), EWorldWalkerMainMotionState::Walk, true},
		{TEXT("ClimbU"), TEXT("向上攀爬"), EWorldWalkerMainMotionState::None, true},
	};

	bool IsLumineArmChainBone(const FName BoneName)
	{
		const FString Name = BoneName.ToString();
		return Name.Contains(TEXT("Clavicle"))
			|| Name.Contains(TEXT("UpperArm"))
			|| Name.Contains(TEXT("Forearm"))
			|| Name.EndsWith(TEXT("-Hand"))
			|| Name.Contains(TEXT("-Finger"));
	}

	float GetLuminePrimaryArmRotationWeight(const EWorldWalkerMainMotionState State)
	{
		switch (State)
		{
		case EWorldWalkerMainMotionState::Idle:
			return 0.25f;
		case EWorldWalkerMainMotionState::Walk:
			return 0.20f;
		case EWorldWalkerMainMotionState::Run:
			return 0.35f;
		case EWorldWalkerMainMotionState::Sprint:
			return 0.45f;
		case EWorldWalkerMainMotionState::Dash:
			return 0.50f;
		case EWorldWalkerMainMotionState::Jump:
			return 0.40f;
		case EWorldWalkerMainMotionState::Fall:
			return 0.35f;
		case EWorldWalkerMainMotionState::LightLanding:
			return 0.40f;
		case EWorldWalkerMainMotionState::HardLanding:
			return 0.50f;
		case EWorldWalkerMainMotionState::Roll:
			return 0.55f;
		case EWorldWalkerMainMotionState::Attack:
			return 1.0f;
		case EWorldWalkerMainMotionState::LightStop:
			return 0.25f;
		case EWorldWalkerMainMotionState::MediumStop:
			return 0.35f;
		case EWorldWalkerMainMotionState::HardStop:
			return 0.45f;
		default:
			return 0.25f;
		}
	}

	const TCHAR* GetMainMotionStateName(const EWorldWalkerMainMotionState State)
	{
		switch (State)
		{
		case EWorldWalkerMainMotionState::Idle: return TEXT("Idle");
		case EWorldWalkerMainMotionState::Walk: return TEXT("Walk");
		case EWorldWalkerMainMotionState::Run: return TEXT("Run");
		case EWorldWalkerMainMotionState::Sprint: return TEXT("Sprint");
		case EWorldWalkerMainMotionState::Dash: return TEXT("Dash");
		case EWorldWalkerMainMotionState::Jump: return TEXT("Jump");
		case EWorldWalkerMainMotionState::Fall: return TEXT("Fall");
		case EWorldWalkerMainMotionState::LightLanding: return TEXT("LightLanding");
		case EWorldWalkerMainMotionState::HardLanding: return TEXT("HardLanding");
		case EWorldWalkerMainMotionState::Roll: return TEXT("Roll");
		case EWorldWalkerMainMotionState::Attack: return TEXT("Attack");
		case EWorldWalkerMainMotionState::LightStop: return TEXT("LightStop");
		case EWorldWalkerMainMotionState::MediumStop: return TEXT("MediumStop");
		case EWorldWalkerMainMotionState::HardStop: return TEXT("HardStop");
		default: return TEXT("None");
		}
	}

	float GetMainMotionMaximumOneShotDuration(
		const EWorldWalkerMainMotionState State)
	{
		switch (State)
		{
		case EWorldWalkerMainMotionState::Dash:
			return 0.45f;
		case EWorldWalkerMainMotionState::LightLanding:
			return 0.55f;
		case EWorldWalkerMainMotionState::HardLanding:
			return 0.85f;
		case EWorldWalkerMainMotionState::Roll:
			return 0.90f;
		case EWorldWalkerMainMotionState::LightStop:
			return 0.45f;
		case EWorldWalkerMainMotionState::MediumStop:
			return 0.60f;
		case EWorldWalkerMainMotionState::HardStop:
			return 0.75f;
		default:
			return TNumericLimits<float>::Max();
		}
	}

	bool IsMainMotionInterruptedByMovement(
		const EWorldWalkerMainMotionState State)
	{
		return State == EWorldWalkerMainMotionState::LightLanding
			|| State == EWorldWalkerMainMotionState::HardLanding
			|| State == EWorldWalkerMainMotionState::LightStop
			|| State == EWorldWalkerMainMotionState::MediumStop
			|| State == EWorldWalkerMainMotionState::HardStop;
	}

	float GetLumineArmBoneWeightMultiplier(const FName BoneName)
	{
		const FString Name = BoneName.ToString();
		if (Name.Contains(TEXT("Clavicle")))
		{
			return 0.35f;
		}
		if (Name.Contains(TEXT("Forearm")))
		{
			return 0.70f;
		}
		if (Name.EndsWith(TEXT("-Hand")))
		{
			return 0.50f;
		}
		if (Name.Contains(TEXT("-Finger")))
		{
			return 0.35f;
		}
		if (Name.Contains(TEXT("UpperArmTwist")))
		{
			return 0.65f;
		}
		return 1.0f;
	}

	// Target order follows Lumine's hierarchy so component-space transforms are
	// converted back to local transforms after their mapped parents are ready.
	constexpr FLumineBoneNamePair LumineBoneNamePairs[] =
	{
		{TEXT("root"), TEXT("Bip001")},
		{TEXT("pelvis"), TEXT("Bip001-Pelvis")},
		{TEXT("spine_01"), TEXT("Bip001-Spine")},
		{TEXT("spine_02"), TEXT("Bip001-Spine1")},
		{TEXT("spine_03"), TEXT("Bip001-Spine2")},
		{TEXT("neck_01"), TEXT("Bip001-Neck")},
		{TEXT("head"), TEXT("Bip001-Head")},
		{TEXT("Eye_L"), TEXT("_EyeBone-L-A01")},
		{TEXT("Eye_R"), TEXT("_EyeBone-R-A01")},
		{TEXT("clavicle_l"), TEXT("Bip001-L-Clavicle")},
		{TEXT("upperarm_l"), TEXT("Bip001-L-UpperArm")},
		{TEXT("lowerarm_l"), TEXT("Bip001-L-Forearm")},
		{TEXT("hand_l"), TEXT("Bip001-L-Hand")},
		{TEXT("thumb_01_l"), TEXT("Bip001-L-Finger0")},
		{TEXT("thumb_02_l"), TEXT("Bip001-L-Finger01")},
		{TEXT("thumb_03_l"), TEXT("Bip001-L-Finger02")},
		{TEXT("index_01_l"), TEXT("Bip001-L-Finger1")},
		{TEXT("index_02_l"), TEXT("Bip001-L-Finger11")},
		{TEXT("index_03_l"), TEXT("Bip001-L-Finger12")},
		{TEXT("middle_01_l"), TEXT("Bip001-L-Finger2")},
		{TEXT("middle_02_l"), TEXT("Bip001-L-Finger21")},
		{TEXT("middle_03_l"), TEXT("Bip001-L-Finger22")},
		{TEXT("ring_01_l"), TEXT("Bip001-L-Finger3")},
		{TEXT("ring_02_l"), TEXT("Bip001-L-Finger31")},
		{TEXT("ring_03_l"), TEXT("Bip001-L-Finger32")},
		{TEXT("pinky_01_l"), TEXT("Bip001-L-Finger4")},
		{TEXT("pinky_02_l"), TEXT("Bip001-L-Finger41")},
		{TEXT("pinky_03_l"), TEXT("Bip001-L-Finger42")},
		{TEXT("upperarm_twist_01_l"), TEXT("_UpperArmTwist-L-A01")},
		{TEXT("upperarm_twist_01_l"), TEXT("_UpperArmTwist-L-A02")},
		{TEXT("clavicle_r"), TEXT("Bip001-R-Clavicle")},
		{TEXT("upperarm_r"), TEXT("Bip001-R-UpperArm")},
		{TEXT("lowerarm_r"), TEXT("Bip001-R-Forearm")},
		{TEXT("hand_r"), TEXT("Bip001-R-Hand")},
		{TEXT("thumb_01_r"), TEXT("Bip001-R-Finger0")},
		{TEXT("thumb_02_r"), TEXT("Bip001-R-Finger01")},
		{TEXT("thumb_03_r"), TEXT("Bip001-R-Finger02")},
		{TEXT("index_01_r"), TEXT("Bip001-R-Finger1")},
		{TEXT("index_02_r"), TEXT("Bip001-R-Finger11")},
		{TEXT("index_03_r"), TEXT("Bip001-R-Finger12")},
		{TEXT("middle_01_r"), TEXT("Bip001-R-Finger2")},
		{TEXT("middle_02_r"), TEXT("Bip001-R-Finger21")},
		{TEXT("middle_03_r"), TEXT("Bip001-R-Finger22")},
		{TEXT("ring_01_r"), TEXT("Bip001-R-Finger3")},
		{TEXT("ring_02_r"), TEXT("Bip001-R-Finger31")},
		{TEXT("ring_03_r"), TEXT("Bip001-R-Finger32")},
		{TEXT("pinky_01_r"), TEXT("Bip001-R-Finger4")},
		{TEXT("pinky_02_r"), TEXT("Bip001-R-Finger41")},
		{TEXT("pinky_03_r"), TEXT("Bip001-R-Finger42")},
		{TEXT("upperarm_twist_01_r"), TEXT("_UpperArmTwist-R-A01")},
		{TEXT("upperarm_twist_01_r"), TEXT("_UpperArmTwist-R-A02")},
		{TEXT("WeaponR"), TEXT("WeaponR")},
		{TEXT("thigh_l"), TEXT("Bip001-L-Thigh")},
		{TEXT("calf_l"), TEXT("Bip001-L-Calf")},
		{TEXT("foot_l"), TEXT("Bip001-L-Foot")},
		{TEXT("ball_l"), TEXT("Bip001-L-Toe0")},
		{TEXT("thigh_r"), TEXT("Bip001-R-Thigh")},
		{TEXT("calf_r"), TEXT("Bip001-R-Calf")},
		{TEXT("foot_r"), TEXT("Bip001-R-Foot")},
		{TEXT("ball_r"), TEXT("Bip001-R-Toe0")}
	};

	// Wafflus' clips use a Generic Mixamo hierarchy.  Keep this table separate
	// from the Anime Character Pack mapping so the imported source can disappear
	// without breaking the existing local fallback.
	constexpr FLumineBoneNamePair MixamoLumineBoneNamePairs[] =
	{
		{TEXT("mixamorig:Hips"), TEXT("Bip001-Pelvis")},
		{TEXT("mixamorig:Spine"), TEXT("Bip001-Spine")},
		{TEXT("mixamorig:Spine1"), TEXT("Bip001-Spine1")},
		{TEXT("mixamorig:Spine2"), TEXT("Bip001-Spine2")},
		{TEXT("mixamorig:Neck"), TEXT("Bip001-Neck")},
		{TEXT("mixamorig:Head"), TEXT("Bip001-Head")},
		{TEXT("mixamorig:LeftShoulder"), TEXT("Bip001-L-Clavicle")},
		{TEXT("mixamorig:LeftArm"), TEXT("Bip001-L-UpperArm")},
		{TEXT("mixamorig:LeftForeArm"), TEXT("Bip001-L-Forearm")},
		{TEXT("mixamorig:LeftHand"), TEXT("Bip001-L-Hand")},
		{TEXT("mixamorig:LeftHandThumb1"), TEXT("Bip001-L-Finger0")},
		{TEXT("mixamorig:LeftHandThumb2"), TEXT("Bip001-L-Finger01")},
		{TEXT("mixamorig:LeftHandThumb3"), TEXT("Bip001-L-Finger02")},
		{TEXT("mixamorig:LeftHandIndex1"), TEXT("Bip001-L-Finger1")},
		{TEXT("mixamorig:LeftHandIndex2"), TEXT("Bip001-L-Finger11")},
		{TEXT("mixamorig:LeftHandIndex3"), TEXT("Bip001-L-Finger12")},
		{TEXT("mixamorig:LeftHandMiddle1"), TEXT("Bip001-L-Finger2")},
		{TEXT("mixamorig:LeftHandMiddle2"), TEXT("Bip001-L-Finger21")},
		{TEXT("mixamorig:LeftHandMiddle3"), TEXT("Bip001-L-Finger22")},
		{TEXT("mixamorig:LeftHandRing1"), TEXT("Bip001-L-Finger3")},
		{TEXT("mixamorig:LeftHandRing2"), TEXT("Bip001-L-Finger31")},
		{TEXT("mixamorig:LeftHandRing3"), TEXT("Bip001-L-Finger32")},
		{TEXT("mixamorig:LeftHandPinky1"), TEXT("Bip001-L-Finger4")},
		{TEXT("mixamorig:LeftHandPinky2"), TEXT("Bip001-L-Finger41")},
		{TEXT("mixamorig:LeftHandPinky3"), TEXT("Bip001-L-Finger42")},
		{TEXT("mixamorig:RightShoulder"), TEXT("Bip001-R-Clavicle")},
		{TEXT("mixamorig:RightArm"), TEXT("Bip001-R-UpperArm")},
		{TEXT("mixamorig:RightForeArm"), TEXT("Bip001-R-Forearm")},
		{TEXT("mixamorig:RightHand"), TEXT("Bip001-R-Hand")},
		{TEXT("mixamorig:RightHandThumb1"), TEXT("Bip001-R-Finger0")},
		{TEXT("mixamorig:RightHandThumb2"), TEXT("Bip001-R-Finger01")},
		{TEXT("mixamorig:RightHandThumb3"), TEXT("Bip001-R-Finger02")},
		{TEXT("mixamorig:RightHandIndex1"), TEXT("Bip001-R-Finger1")},
		{TEXT("mixamorig:RightHandIndex2"), TEXT("Bip001-R-Finger11")},
		{TEXT("mixamorig:RightHandIndex3"), TEXT("Bip001-R-Finger12")},
		{TEXT("mixamorig:RightHandMiddle1"), TEXT("Bip001-R-Finger2")},
		{TEXT("mixamorig:RightHandMiddle2"), TEXT("Bip001-R-Finger21")},
		{TEXT("mixamorig:RightHandMiddle3"), TEXT("Bip001-R-Finger22")},
		{TEXT("mixamorig:RightHandRing1"), TEXT("Bip001-R-Finger3")},
		{TEXT("mixamorig:RightHandRing2"), TEXT("Bip001-R-Finger31")},
		{TEXT("mixamorig:RightHandRing3"), TEXT("Bip001-R-Finger32")},
		{TEXT("mixamorig:RightHandPinky1"), TEXT("Bip001-R-Finger4")},
		{TEXT("mixamorig:RightHandPinky2"), TEXT("Bip001-R-Finger41")},
		{TEXT("mixamorig:RightHandPinky3"), TEXT("Bip001-R-Finger42")},
		{TEXT("mixamorig:LeftUpLeg"), TEXT("Bip001-L-Thigh")},
		{TEXT("mixamorig:LeftLeg"), TEXT("Bip001-L-Calf")},
		{TEXT("mixamorig:LeftFoot"), TEXT("Bip001-L-Foot")},
		{TEXT("mixamorig:LeftToeBase"), TEXT("Bip001-L-Toe0")},
		{TEXT("mixamorig:RightUpLeg"), TEXT("Bip001-R-Thigh")},
		{TEXT("mixamorig:RightLeg"), TEXT("Bip001-R-Calf")},
		{TEXT("mixamorig:RightFoot"), TEXT("Bip001-R-Foot")},
		{TEXT("mixamorig:RightToeBase"), TEXT("Bip001-R-Toe0")}
	};

	TArray<FTransform> BuildReferenceComponentTransforms(const FReferenceSkeleton& Skeleton)
	{
		const TArray<FTransform>& LocalPose = Skeleton.GetRefBonePose();
		TArray<FTransform> ComponentPose;
		ComponentPose.SetNum(LocalPose.Num());
		for (int32 BoneIndex = 0; BoneIndex < LocalPose.Num(); ++BoneIndex)
		{
			const int32 ParentIndex = Skeleton.GetParentIndex(BoneIndex);
			ComponentPose[BoneIndex] = ParentIndex == INDEX_NONE
				? LocalPose[BoneIndex]
				: LocalPose[BoneIndex] * ComponentPose[ParentIndex];
		}
		return ComponentPose;
	}
}

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
	if (FParse::Param(FCommandLine::Get(), TEXT("WWAvatarCapture")))
	{
		// Deterministic close front camera used by unattended visual QA captures.
		CameraBoom->TargetArmLength = 220.0f;
		CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 55.0f);
		if (Controller)
		{
			Controller->SetControlRotation(FRotator(
				-8.0f,
				GetActorRotation().Yaw + 180.0f,
				0.0f));
		}
		bAvatarCaptureMode = true;
	}

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
	// Inert unless explicitly requested by unattended visual QA. Headless game
	// mode can ignore synthetic Pawn input because it has no focused controller,
	// so drive CharacterMovement directly: enter Run, then clear velocity to
	// exercise exactly the movement-release transition fixed below.
	if (GetWorld()
		&& FParse::Param(FCommandLine::Get(), TEXT("WWMovementReleaseTest")))
	{
		const float TestTime = GetWorld()->GetTimeSeconds();
		if (TestTime >= 0.75f && TestTime < 2.25f && GetCharacterMovement())
		{
			GetCharacterMovement()->Velocity = GetActorForwardVector() * 260.0f;
		}
		else if (TestTime >= 2.25f && TestTime < 2.50f && GetCharacterMovement())
		{
			GetCharacterMovement()->StopMovementImmediately();
		}
	}
	// Deterministic unattended input driver for the real left-click combo path.
	if (GetWorld()
		&& FParse::Param(FCommandLine::Get(), TEXT("WWAttackComboTest"))
		&& AttackComboTestInputsSent < 8)
	{
		const float TestTime = GetWorld()->GetTimeSeconds();
		const float NextInputTime = 0.75f + AttackComboTestInputsSent * 0.28f;
		if (TestTime >= NextInputTime)
		{
			HandleMainWorldAttackPressed();
			++AttackComboTestInputsSent;
		}
	}
	UpdateMainWorldAttackCombo();
	RefreshMainWorldGenshinMotion(DeltaSeconds);
	UpdateMainWorldLuminePose(DeltaSeconds);
	if (!bMotionPickerVisualTestOpened
		&& GetWorld()
		&& GetWorld()->GetTimeSeconds() >= 4.0f
		&& FParse::Param(FCommandLine::Get(), TEXT("WWMotionPickerVisualTest")))
	{
		bMotionPickerVisualTestOpened = true;
		HandleMainWorldNextMotionPressed();
	}
	if (!bMotionPickerActionTestTriggered
		&& GetWorld()
		&& GetWorld()->GetTimeSeconds() >= 3.0f)
	{
		int32 TestActionIndex = INDEX_NONE;
		if (FParse::Value(
			FCommandLine::Get(), TEXT("WWMotionPickerActionTest="), TestActionIndex))
		{
			bMotionPickerActionTestTriggered = true;
			TriggerMainWorldMotionPreview(TestActionIndex);
		}
	}
	if (bAvatarCaptureMode && GetWorld())
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		if (!bAvatarCaptureScreenshotRequested && CurrentTime >= 4.0f)
		{
			FString CaptureMotion(TEXT("Idle"));
			FParse::Value(
				FCommandLine::Get(), TEXT("WWAvatarCaptureMotion="), CaptureMotion);
			const FString ScreenshotPath = FPaths::Combine(
				FPaths::ProjectSavedDir(),
				TEXT("Screenshots"),
				FString::Printf(TEXT("QA_Lumine_%s.png"), *CaptureMotion));
			FScreenshotRequest::RequestScreenshot(
				ScreenshotPath,
				false,
				false);
			if (MainWorldLumineMesh && GetCapsuleComponent())
			{
				auto BoneWorldZ = [&](const FName BoneName)
				{
					const FTransform BoneTransform = MainWorldLumineMesh->GetBoneTransformByName(
						BoneName, EBoneSpaces::ComponentSpace);
					return MainWorldLumineMesh->GetComponentTransform()
						.TransformPosition(BoneTransform.GetLocation()).Z;
				};
				UE_LOG(
					LogTemp,
					Display,
					TEXT("WW_AVATAR_CAPTURE_BOUNDS ActorZ=%.2f CapsuleBottomZ=%.2f MeshZ=%.2f BoundsOriginZ=%.2f BoundsExtentZ=%.2f HeadZ=%.2f LeftFootZ=%.2f RightFootZ=%.2f"),
					GetActorLocation().Z,
					GetCapsuleComponent()->GetComponentLocation().Z
						- GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),
					MainWorldLumineMesh->GetComponentLocation().Z,
					MainWorldLumineMesh->Bounds.Origin.Z,
					MainWorldLumineMesh->Bounds.BoxExtent.Z,
					BoneWorldZ(TEXT("Bip001-Head")),
					BoneWorldZ(TEXT("Bip001-L-Foot")),
					BoneWorldZ(TEXT("Bip001-R-Foot")));
				if (USkeletalMeshComponent* PoseSource = GetMainWorldPoseSource())
				{
					auto SourceBoneZ = [&](const FName BoneName)
					{
						const int32 BoneIndex = PoseSource->GetBoneIndex(BoneName);
						return BoneIndex == INDEX_NONE
							? TNumericLimits<float>::Lowest()
							: PoseSource->GetBoneTransform(
								BoneIndex, FTransform::Identity).GetLocation().Z;
					};
					UE_LOG(
						LogTemp,
						Display,
						TEXT("WW_AVATAR_SOURCE_BONES RootZ=%.3f PelvisZ=%.3f HeadZ=%.3f LeftFootZ=%.3f RightFootZ=%.3f"),
						SourceBoneZ(TEXT("Bip001")),
						SourceBoneZ(TEXT("Bip001-Pelvis")),
						SourceBoneZ(TEXT("Bip001-Head")),
						SourceBoneZ(TEXT("Bip001-L-Foot")),
						SourceBoneZ(TEXT("Bip001-R-Foot")));
				}
			}
			UE_LOG(
				LogTemp,
				Display,
				TEXT("WW_AVATAR_CAPTURE_REQUESTED Motion=%s Path=%s"),
				*CaptureMotion,
				*ScreenshotPath);
			bAvatarCaptureScreenshotRequested = true;
			AvatarCaptureScreenshotTime = CurrentTime;
		}
		else if (bAvatarCaptureScreenshotRequested
			&& CurrentTime >= AvatarCaptureScreenshotTime + 3.0f)
		{
			FPlatformMisc::RequestExit(false);
		}
	}

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
	PlayerInputComponent->BindAction(TEXT("MainWorldDash"), IE_Pressed, this, &AWorldWalkerCharacter::HandleMainWorldDashPressed);
	PlayerInputComponent->BindAction(TEXT("MainWorldRoll"), IE_Pressed, this, &AWorldWalkerCharacter::HandleMainWorldRollPressed);
	PlayerInputComponent->BindAction(TEXT("MainWorldAttack"), IE_Pressed, this, &AWorldWalkerCharacter::HandleMainWorldAttackPressed);
	PlayerInputComponent->BindAction(TEXT("MainWorldNextMotion"), IE_Pressed, this, &AWorldWalkerCharacter::HandleMainWorldNextMotionPressed);
}

void AWorldWalkerCharacter::MoveForward(const float Value)
{
	if (!bCombatLocked && !bMainWorldAttackComboActive
		&& Controller && !FMath::IsNearlyZero(Value))
	{
		bMainWorldMotionPreviewActive = false;
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
	}
}

void AWorldWalkerCharacter::MoveRight(const float Value)
{
	if (!bCombatLocked && !bMainWorldAttackComboActive
		&& Controller && !FMath::IsNearlyZero(Value))
	{
		bMainWorldMotionPreviewActive = false;
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
		CancelMainWorldAttackCombo(TEXT("CombatLocked"));
	}

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

	bMainWorldMotionPreviewActive = false;
	CancelMainWorldAttackCombo(TEXT("Jump"));
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
	bMainWorldGenshinMotionConfigured = ConfigureGenshinMotionSource();
	bMainWorldLumineConfigured = ConfigureLumineVisual(GetMainWorldPoseSource());

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

	bMainWorldAnimeFormConfigured = bMainWorldLumineConfigured ||
		(MainWorldAnimeTop != nullptr
			&& MainWorldAnimeBottom != nullptr
			&& MainWorldAnimeHair != nullptr);
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
	UpdateMainWorldLuminePose(0.0f);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Main world avatar configured. Model=%s BoneLinks=%d Locomotion=%s"),
		bMainWorldLumineConfigured ? TEXT("Lumine") : TEXT("AnimeFallback"),
		LumineBoneLinks.Num(),
		bMainWorldGenshinMotionConfigured ? TEXT("OriginalPlayerGirl28") : TEXT("abp_Human"));
	return true;
}

void AWorldWalkerCharacter::HandleMainWorldDashPressed()
{
	if (bCombatLocked || !bMainWorldGenshinMotionConfigured
		|| !bMainWorldAnimeFormActive || !MainWorldGenshinDash
		|| !GetCharacterMovement() || GetCharacterMovement()->IsFalling())
	{
		return;
	}

	CancelMainWorldAttackCombo(TEXT("Dash"));
	bMainWorldMotionPreviewActive = false;
	FVector DashDirection = GetLastMovementInputVector().GetSafeNormal2D();
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = GetVelocity().GetSafeNormal2D();
	}
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = GetActorForwardVector().GetSafeNormal2D();
	}
	FVector DashVelocity = GetCharacterMovement()->Velocity;
	const float DashSpeed = FMath::Max(DashVelocity.Size2D(), 620.0f);
	DashVelocity.X = DashDirection.X * DashSpeed;
	DashVelocity.Y = DashDirection.Y * DashSpeed;
	GetCharacterMovement()->Velocity = DashVelocity;
	PlayMainWorldGenshinMotion(
		MainWorldGenshinDash,
		EWorldWalkerMainMotionState::Dash,
		false,
		1.0f);
	UE_LOG(LogTemp, Display, TEXT("WW_MAIN_MOTION_TRIGGER Key=F State=Dash"));
}

void AWorldWalkerCharacter::HandleMainWorldRollPressed()
{
	if (bCombatLocked || !bMainWorldGenshinMotionConfigured
		|| !bMainWorldAnimeFormActive || !MainWorldGenshinRoll
		|| !GetCharacterMovement() || GetCharacterMovement()->IsFalling())
	{
		return;
	}

	CancelMainWorldAttackCombo(TEXT("Roll"));
	bMainWorldMotionPreviewActive = false;
	FVector RollDirection = GetVelocity().GetSafeNormal2D();
	if (RollDirection.IsNearlyZero())
	{
		RollDirection = GetLastMovementInputVector().GetSafeNormal2D();
	}
	if (RollDirection.IsNearlyZero())
	{
		RollDirection = GetActorForwardVector().GetSafeNormal2D();
	}
	FVector RollVelocity = GetCharacterMovement()->Velocity;
	const float RollSpeed = FMath::Max(RollVelocity.Size2D(), 360.0f);
	RollVelocity.X = RollDirection.X * RollSpeed;
	RollVelocity.Y = RollDirection.Y * RollSpeed;
	GetCharacterMovement()->Velocity = RollVelocity;
	PlayMainWorldGenshinMotion(
		MainWorldGenshinRoll,
		EWorldWalkerMainMotionState::Roll,
		false,
		1.0f);
	UE_LOG(LogTemp, Display, TEXT("WW_MAIN_MOTION_TRIGGER Key=R State=Roll"));
}

void AWorldWalkerCharacter::HandleMainWorldAttackPressed()
{
	if (bCombatLocked || !bMainWorldGenshinMotionConfigured
		|| !bMainWorldAnimeFormActive || MainWorldAttackAnimations.IsEmpty()
		|| !GetWorld() || !GetCharacterMovement()
		|| GetCharacterMovement()->IsFalling())
	{
		return;
	}

	if (!bMainWorldAttackComboActive)
	{
		PendingMainWorldAttackInputs = 0;
		StartMainWorldAttackComboStep(0);
		UE_LOG(LogTemp, Display, TEXT("WW_ATTACK_COMBO_INPUT Key=LeftMouseButton Action=Start"));
		return;
	}

	PendingMainWorldAttackInputs = FMath::Min(
		PendingMainWorldAttackInputs + 1,
		MainWorldAttackAnimations.Num());
	UE_LOG(
		LogTemp,
		Display,
		TEXT("WW_ATTACK_COMBO_INPUT Key=LeftMouseButton Action=Buffer Pending=%d"),
		PendingMainWorldAttackInputs);

	// A press inside an already-open chain window should feel immediate instead
	// of waiting for the current animation's recovery tail.
	if (GetWorld()->GetTimeSeconds() >= MainWorldAttackChainTime)
	{
		--PendingMainWorldAttackInputs;
		StartMainWorldAttackComboStep(
			(CurrentMainWorldAttackIndex + 1) % MainWorldAttackAnimations.Num());
	}
}

void AWorldWalkerCharacter::StartMainWorldAttackComboStep(const int32 AttackIndex)
{
	if (!GetWorld() || MainWorldAttackAnimations.IsEmpty())
	{
		return;
	}

	const int32 SafeIndex = FMath::Abs(AttackIndex) % MainWorldAttackAnimations.Num();
	UAnimSequence* Animation = MainWorldAttackAnimations[SafeIndex];
	if (!Animation)
	{
		CancelMainWorldAttackCombo(TEXT("MissingAnimation"));
		return;
	}

	constexpr float AttackPlayRate = 1.35f;
	const float EffectiveDuration = FMath::Max(
		0.20f,
		Animation->GetPlayLength() / AttackPlayRate);
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	bMainWorldMotionPreviewActive = false;
	bMainWorldAttackComboActive = true;
	CurrentMainWorldAttackIndex = SafeIndex;
	MainWorldAttackChainTime = CurrentTime + EffectiveDuration * 0.55f;
	MainWorldAttackEndTime = CurrentTime + EffectiveDuration * 0.92f;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
	PlayMainWorldGenshinMotion(
		Animation,
		EWorldWalkerMainMotionState::Attack,
		false,
		AttackPlayRate);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("WW_ATTACK_COMBO_STEP Step=%d/%d Animation=%s ChainAt=%.3f EndAt=%.3f Pending=%d"),
		SafeIndex + 1,
		MainWorldAttackAnimations.Num(),
		*Animation->GetName(),
		MainWorldAttackChainTime,
		MainWorldAttackEndTime,
		PendingMainWorldAttackInputs);
}

void AWorldWalkerCharacter::UpdateMainWorldAttackCombo()
{
	if (!bMainWorldAttackComboActive || !GetWorld())
	{
		return;
	}

	if (bCombatLocked || !bMainWorldAnimeFormActive || !GetCharacterMovement()
		|| GetCharacterMovement()->IsFalling())
	{
		CancelMainWorldAttackCombo(TEXT("InvalidState"));
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (PendingMainWorldAttackInputs > 0 && CurrentTime >= MainWorldAttackChainTime)
	{
		--PendingMainWorldAttackInputs;
		StartMainWorldAttackComboStep(
			(CurrentMainWorldAttackIndex + 1) % MainWorldAttackAnimations.Num());
		return;
	}

	if (CurrentTime >= MainWorldAttackEndTime)
	{
		CancelMainWorldAttackCombo(TEXT("Completed"));
	}
}

void AWorldWalkerCharacter::CancelMainWorldAttackCombo(const TCHAR* Reason)
{
	if (bMainWorldAttackComboActive)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("WW_ATTACK_COMBO_END LastStep=%d Pending=%d Reason=%s"),
			CurrentMainWorldAttackIndex + 1,
			PendingMainWorldAttackInputs,
			Reason ? Reason : TEXT("Unknown"));
	}
	bMainWorldAttackComboActive = false;
	bMainWorldGenshinOneShotPlaying = false;
	CurrentMainWorldAttackIndex = INDEX_NONE;
	PendingMainWorldAttackInputs = 0;
	MainWorldAttackChainTime = 0.0f;
	MainWorldAttackEndTime = 0.0f;
}

void AWorldWalkerCharacter::HandleMainWorldNextMotionPressed()
{
	if (MainWorldMotionPicker)
	{
		CloseMainWorldMotionPicker();
		return;
	}
	if (bCombatLocked || !bMainWorldGenshinMotionConfigured
		|| !bMainWorldAnimeFormActive || MainWorldOriginalActionAnimations.IsEmpty()
		|| !GetWorld() || !GetCharacterMovement()
		|| GetCharacterMovement()->IsFalling())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}
	MainWorldMotionPicker = CreateWidget<UWorldWalkerMotionPickerWidget>(
		PlayerController, UWorldWalkerMotionPickerWidget::StaticClass());
	if (!MainWorldMotionPicker)
	{
		return;
	}
	MainWorldMotionPicker->ConfigureActions(
		MainWorldOriginalActionNames,
		NextMainWorldMotionPreviewIndex);
	MainWorldMotionPicker->OnMotionPicked.AddUObject(
		this, &AWorldWalkerCharacter::HandleMainWorldMotionPicked);
	MainWorldMotionPicker->OnClosed.AddUObject(
		this, &AWorldWalkerCharacter::CloseMainWorldMotionPicker);
	MainWorldMotionPicker->AddToViewport(140);
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MainWorldMotionPicker->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(false);
	MainWorldMotionPicker->SetKeyboardFocus();
	UE_LOG(LogTemp, Display, TEXT("WW_MAIN_MOTION_PICKER_OPEN count=%d"),
		MainWorldOriginalActionNames.Num());
}

void AWorldWalkerCharacter::TriggerMainWorldMotionPreview(const int32 PreviewIndex)
{
	if (bCombatLocked || !bMainWorldGenshinMotionConfigured
		|| !bMainWorldAnimeFormActive || !GetWorld()
		|| !GetCharacterMovement() || GetCharacterMovement()->IsFalling())
	{
		return;
	}

	if (MainWorldOriginalActionAnimations.IsEmpty())
	{
		return;
	}
	const int32 SafeIndex = FMath::Abs(PreviewIndex) % MainWorldOriginalActionAnimations.Num();
	UAnimSequence* Animation = MainWorldOriginalActionAnimations[SafeIndex];
	if (!Animation)
	{
		return;
	}
	const EWorldWalkerMainMotionState State = MainWorldOriginalActionStates.IsValidIndex(SafeIndex)
		? MainWorldOriginalActionStates[SafeIndex]
		: EWorldWalkerMainMotionState::None;
	const bool bLoop = MainWorldOriginalActionLoops.IsValidIndex(SafeIndex)
		&& MainWorldOriginalActionLoops[SafeIndex];

	PlayMainWorldGenshinMotion(Animation, State, bLoop, 1.0f);
	bMainWorldMotionPreviewActive = true;
	MainWorldMotionPreviewEndTime = GetWorld()->GetTimeSeconds()
		+ (bLoop ? 2.5f : FMath::Max(0.35f, Animation->GetPlayLength()));
	NextMainWorldMotionPreviewIndex = SafeIndex;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("WW_MAIN_MOTION_PREVIEW Index=%d/%d Action=%s"),
		SafeIndex + 1,
		MainWorldOriginalActionAnimations.Num(),
		*MainWorldOriginalActionNames[SafeIndex]);
}

void AWorldWalkerCharacter::HandleMainWorldMotionPicked(const int32 PreviewIndex)
{
	CloseMainWorldMotionPicker();
	TriggerMainWorldMotionPreview(PreviewIndex);
}

void AWorldWalkerCharacter::CloseMainWorldMotionPicker()
{
	if (MainWorldMotionPicker)
	{
		MainWorldMotionPicker->OnMotionPicked.RemoveAll(this);
		MainWorldMotionPicker->OnClosed.RemoveAll(this);
		MainWorldMotionPicker->RemoveFromParent();
		MainWorldMotionPicker = nullptr;
	}
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->SetShowMouseCursor(false);
	}
	UE_LOG(LogTemp, Display, TEXT("WW_MAIN_MOTION_PICKER_CLOSED"));
}

bool AWorldWalkerCharacter::ConfigureGenshinMotionSource()
{
	static const TCHAR* MeshPath =
		TEXT("/Game/WorldWalker/Shared/ThirdParty/GenshinExtract/Lumine/OriginalSource/SK_WW_Lumine_OriginalSource.SK_WW_Lumine_OriginalSource");
	static const TCHAR* LegacyMeshPath =
		TEXT("/Game/WorldWalker/Shared/ThirdParty/GenshinExtract/Lumine/SK_WW_Lumine_CurrentGame.SK_WW_Lumine_CurrentGame");
	static const TCHAR* AnimationRoot =
		TEXT("/Game/WorldWalker/Shared/ThirdParty/GenshinExtract/Lumine/OriginalSource/Animations/");
	static const TCHAR* ActionAnimationRoot =
		TEXT("/Game/WorldWalker/Shared/ThirdParty/GenshinExtract/Lumine/OriginalSource/Actions/");

	MainWorldGenshinOriginalMotionMesh = LoadObject<USkeletalMesh>(nullptr, MeshPath);
	MainWorldGenshinLegacyMotionMesh = LoadObject<USkeletalMesh>(nullptr, LegacyMeshPath);
	auto LoadMotion = [&](const TCHAR* Name) -> UAnimSequence*
	{
		const FString ObjectPath = FString::Printf(
			TEXT("%sA_WW_PlayerGirl_%s.A_WW_PlayerGirl_%s"),
			AnimationRoot,
			Name,
			Name);
		return LoadObject<UAnimSequence>(nullptr, *ObjectPath);
	};
	auto LoadActionMotion = [&](const TCHAR* Name) -> UAnimSequence*
	{
		const FString ObjectPath = FString::Printf(
			TEXT("%sA_WW_PlayerGirl_%s.A_WW_PlayerGirl_%s"),
			ActionAnimationRoot,
			Name,
			Name);
		return LoadObject<UAnimSequence>(nullptr, *ObjectPath);
	};

	MainWorldGenshinIdle = LoadMotion(TEXT("Standby"));
	MainWorldGenshinWalk = LoadMotion(TEXT("WalkCycle"));
	MainWorldGenshinRun = LoadMotion(TEXT("RunCycle"));
	MainWorldGenshinSprint = LoadMotion(TEXT("SprintCycle"));
	MainWorldGenshinDash = LoadActionMotion(TEXT("SprintBS"));
	MainWorldGenshinJump = LoadActionMotion(TEXT("Jump"));
	MainWorldGenshinFall = LoadActionMotion(TEXT("FlyNormal"));
	MainWorldGenshinLightLanding = LoadActionMotion(TEXT("FallToGroundL"));
	MainWorldGenshinHardLanding = LoadActionMotion(TEXT("FallToGroundH"));
	MainWorldGenshinRoll = LoadActionMotion(TEXT("CrouchRoll"));
	MainWorldOriginalActionAnimations.Reset();
	MainWorldAttackAnimations.Reset();
	MainWorldOriginalActionNames.Reset();
	MainWorldOriginalActionStates.Reset();
	MainWorldOriginalActionLoops.Reset();
	for (const FPlayerGirlMotionDefinition& Definition : PlayerGirlMotionDefinitions)
	{
		if (UAnimSequence* Animation = LoadActionMotion(Definition.AssetName))
		{
			MainWorldOriginalActionAnimations.Add(Animation);
			MainWorldOriginalActionNames.Add(Definition.DisplayName);
			MainWorldOriginalActionStates.Add(Definition.State);
			MainWorldOriginalActionLoops.Add(Definition.bLoop);
			if (Definition.State == EWorldWalkerMainMotionState::Attack)
			{
				MainWorldAttackAnimations.Add(Animation);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Missing PlayerGirl action: %s"), Definition.AssetName);
		}
	}

	const UAnimSequence* RequiredAnimations[] =
	{
		MainWorldGenshinIdle,
		MainWorldGenshinWalk,
		MainWorldGenshinRun,
		MainWorldGenshinSprint
	};
	if (!MainWorldGenshinOriginalMotionMesh
		|| MainWorldOriginalActionAnimations.Num() != UE_ARRAY_COUNT(PlayerGirlMotionDefinitions)
		|| !Algo::AllOf(RequiredAnimations, [](const UAnimSequence* Animation)
	{
		return Animation != nullptr;
	}))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Original PlayerGirl motion assets are incomplete; retaining animation fallback."));
		return false;
	}

	if (!MainWorldGenshinMotionSource)
	{
		MainWorldGenshinMotionSource =
			NewObject<USkeletalMeshComponent>(this, TEXT("MainWorldGenshinMotionSource"));
		AddInstanceComponent(MainWorldGenshinMotionSource);
		MainWorldGenshinMotionSource->SetupAttachment(GetRootComponent());
		MainWorldGenshinMotionSource->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MainWorldGenshinMotionSource->SetCastShadow(false);
		MainWorldGenshinMotionSource->SetRenderInMainPass(false);
		MainWorldGenshinMotionSource->RegisterComponent();
	}

	MainWorldGenshinMotionSource->SetSkeletalMeshAsset(MainWorldGenshinOriginalMotionMesh);
	MainWorldGenshinMotionSource->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	// Keep the pose source logically visible so its single-node animation advances,
	// while excluding it from every rendering pass.
	MainWorldGenshinMotionSource->SetVisibility(true, true);
	MainWorldGenshinMotionSource->SetHiddenInGame(false, true);
	MainWorldGenshinMotionSource->SetRenderInDepthPass(false);
	MainWorldGenshinMotionSource->SetRenderCustomDepth(false);
	MainWorldGenshinMotionSource->VisibilityBasedAnimTickOption =
		EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	MainWorldGenshinMotionSource->SetComponentTickEnabled(true);
	CurrentMainWorldMotionState = EWorldWalkerMainMotionState::None;
	CancelMainWorldAttackCombo(TEXT("MotionSourceReset"));
	bMainWorldGenshinOneShotPlaying = false;
	PreviousMainWorldVerticalVelocity = 0.0f;
	bMainWorldWasFalling = false;
	PlayMainWorldGenshinMotion(
		MainWorldGenshinIdle,
		EWorldWalkerMainMotionState::Idle,
		true);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("WW_PLAYERGIRL_MOTION_SOURCE_READY locomotion=OriginalGenericSword4 picker=OriginalSwordActionSet%d combo=%d skeleton=%s"),
		MainWorldOriginalActionAnimations.Num(),
		MainWorldAttackAnimations.Num(),
		*MainWorldGenshinOriginalMotionMesh->GetName());
	return true;
}

USkeletalMeshComponent* AWorldWalkerCharacter::GetMainWorldPoseSource() const
{
	return bMainWorldGenshinMotionConfigured && MainWorldGenshinMotionSource
		? MainWorldGenshinMotionSource.Get()
		: GetMesh();
}

void AWorldWalkerCharacter::PlayMainWorldGenshinMotion(
	UAnimSequence* Animation,
	const EWorldWalkerMainMotionState State,
	const bool bLoop,
	const float PlayRate)
{
	if (!Animation || !MainWorldGenshinMotionSource)
	{
		return;
	}

	// AnimationSingleNode does not retarget an AnimSequence whose USkeleton differs
	// from the component mesh. Current locomotion and picker actions all use the
	// OriginalSource skeleton. Keep legacy-carrier support solely for preserved old
	// assets and reject any animation that matches neither skeleton.
	USkeletalMesh* DesiredSourceMesh = nullptr;
	if (MainWorldGenshinOriginalMotionMesh
		&& Animation->GetSkeleton() == MainWorldGenshinOriginalMotionMesh->GetSkeleton())
	{
		DesiredSourceMesh = MainWorldGenshinOriginalMotionMesh;
	}
	else if (MainWorldGenshinLegacyMotionMesh
		&& Animation->GetSkeleton() == MainWorldGenshinLegacyMotionMesh->GetSkeleton())
	{
		DesiredSourceMesh = MainWorldGenshinLegacyMotionMesh;
	}

	if (!DesiredSourceMesh)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("WW_MAIN_MOTION_REJECTED Animation=%s reason=NoMatchingSourceSkeleton"),
			*Animation->GetName());
		return;
	}

	if (MainWorldGenshinMotionSource->GetSkeletalMeshAsset() != DesiredSourceMesh)
	{
		MainWorldGenshinMotionSource->SetSkeletalMeshAsset(DesiredSourceMesh);
		MainWorldGenshinMotionSource->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		MainWorldGenshinMotionSource->VisibilityBasedAnimTickOption =
			EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		MainWorldGenshinMotionSource->SetComponentTickEnabled(true);
		if (bMainWorldLumineConfigured && MainWorldLumineMesh)
		{
			BuildLumineBoneMap(MainWorldGenshinMotionSource);
		}
		UE_LOG(
			LogTemp,
			Display,
			TEXT("WW_MAIN_MOTION_SOURCE_SWITCH Animation=%s Mesh=%s Skeleton=%s"),
			*Animation->GetName(),
			*DesiredSourceMesh->GetName(),
			*Animation->GetSkeleton()->GetName());
	}

	const float SafePlayRate = FMath::Max(0.05f, PlayRate);
	const bool bStateChanged = CurrentMainWorldMotionState != State;
	MainWorldGenshinMotionSource->PlayAnimation(Animation, bLoop);
	MainWorldGenshinMotionSource->SetPlayRate(SafePlayRate);
	CurrentMainWorldMotionState = State;
	bMainWorldGenshinOneShotPlaying = !bLoop;
	const float EffectiveDuration = FMath::Min(
		Animation->GetPlayLength() / SafePlayRate,
		GetMainMotionMaximumOneShotDuration(State));
	MainWorldGenshinOneShotEndTime = GetWorld()
		? GetWorld()->GetTimeSeconds() + EffectiveDuration
		: 0.0f;
	if (bStateChanged)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("WW_MAIN_MOTION_STATE State=%s Animation=%s Loop=%s Rate=%.2f"),
			GetMainMotionStateName(State),
			*Animation->GetName(),
			bLoop ? TEXT("true") : TEXT("false"),
			SafePlayRate);
	}
}

void AWorldWalkerCharacter::RefreshMainWorldGenshinMotion(
	const float DeltaSeconds,
	const bool bForce)
{
	if (!bMainWorldGenshinMotionConfigured || !bMainWorldAnimeFormActive
		|| !MainWorldGenshinMotionSource || !GetCharacterMovement())
	{
		return;
	}

	const FVector Velocity = GetVelocity();
	const float HorizontalSpeed = Velocity.Size2D();
	const bool bIsFalling = GetCharacterMovement()->IsFalling();
	const bool bHasMovementInput =
		GetCharacterMovement()->GetCurrentAcceleration().SizeSquared2D() > 100.0f;
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const bool bJustLanded = bMainWorldWasFalling && !bIsFalling;

	auto RememberMovement = [&]()
	{
		PreviousMainWorldVerticalVelocity = Velocity.Z;
		bMainWorldWasFalling = bIsFalling;
	};

	if (bMainWorldAttackComboActive)
	{
		RememberMovement();
		return;
	}

	if (bMainWorldMotionPreviewActive)
	{
		if (bIsFalling || HorizontalSpeed > 8.0f
			|| CurrentTime >= MainWorldMotionPreviewEndTime)
		{
			bMainWorldMotionPreviewActive = false;
			bMainWorldGenshinOneShotPlaying = false;
		}
		else
		{
			RememberMovement();
			return;
		}
	}

	auto PlayLoop = [&](UAnimSequence* Animation,
		const EWorldWalkerMainMotionState State,
		const float PlayRate)
	{
		if (!Animation)
		{
			return;
		}
		if (bForce || bMainWorldGenshinOneShotPlaying || CurrentMainWorldMotionState != State)
		{
			PlayMainWorldGenshinMotion(Animation, State, true, PlayRate);
		}
		else
		{
			MainWorldGenshinMotionSource->SetPlayRate(PlayRate);
		}
	};

	// Visual-QA-only override. This is inert in normal play and lets unattended
	// captures prove that the authored Walk/Run loops deform the final mesh.
	FString CaptureMotion;
	int32 MotionPickerActionTestIndex = INDEX_NONE;
	const bool bTestingPickerAction = FParse::Value(
		FCommandLine::Get(), TEXT("WWMotionPickerActionTest="), MotionPickerActionTestIndex);
	const bool bTestingMovementRelease = FParse::Param(
		FCommandLine::Get(), TEXT("WWMovementReleaseTest"));
	if (!bTestingPickerAction && !bTestingMovementRelease && FParse::Value(
		FCommandLine::Get(), TEXT("WWAvatarCaptureMotion="), CaptureMotion))
	{
		if (CaptureMotion.Equals(TEXT("Walk"), ESearchCase::IgnoreCase))
		{
			PlayLoop(MainWorldGenshinWalk, EWorldWalkerMainMotionState::Walk, 1.0f);
		}
		else if (CaptureMotion.Equals(TEXT("Run"), ESearchCase::IgnoreCase))
		{
			PlayLoop(MainWorldGenshinRun, EWorldWalkerMainMotionState::Run, 1.0f);
		}
		else
		{
			PlayLoop(MainWorldGenshinIdle, EWorldWalkerMainMotionState::Idle, 1.0f);
		}
		RememberMovement();
		return;
	}

	// Airborne movement always interrupts grounded one-shots so jump controls
	// remain responsive even during a dash or stopping animation.
	if (bIsFalling)
	{
		bMainWorldGenshinOneShotPlaying = false;
		if (Velocity.Z > 70.0f)
		{
			PlayLoop(MainWorldGenshinJump, EWorldWalkerMainMotionState::Jump, 1.0f);
		}
		else
		{
			PlayLoop(MainWorldGenshinFall, EWorldWalkerMainMotionState::Fall, 1.0f);
		}
		RememberMovement();
		return;
	}

	if (bJustLanded)
	{
		UAnimSequence* LandingAnimation = MainWorldGenshinLightLanding;
		EWorldWalkerMainMotionState LandingState =
			EWorldWalkerMainMotionState::LightLanding;
		if (PreviousMainWorldVerticalVelocity < -850.0f)
		{
			if (HorizontalSpeed > 180.0f && MainWorldGenshinRoll)
			{
				LandingAnimation = MainWorldGenshinRoll;
				LandingState = EWorldWalkerMainMotionState::Roll;
			}
			else if (MainWorldGenshinHardLanding)
			{
				LandingAnimation = MainWorldGenshinHardLanding;
				LandingState = EWorldWalkerMainMotionState::HardLanding;
			}
		}
		PlayMainWorldGenshinMotion(LandingAnimation, LandingState, false, 1.0f);
		RememberMovement();
		return;
	}

	if (bMainWorldGenshinOneShotPlaying
		&& bHasMovementInput
		&& IsMainMotionInterruptedByMovement(CurrentMainWorldMotionState))
	{
		bMainWorldGenshinOneShotPlaying = false;
		UE_LOG(
			LogTemp,
			Display,
			TEXT("WW_MAIN_MOTION_INTERRUPTED_BY_MOVEMENT State=%s"),
			GetMainMotionStateName(CurrentMainWorldMotionState));
	}
	if (bMainWorldGenshinOneShotPlaying && CurrentTime < MainWorldGenshinOneShotEndTime)
	{
		RememberMovement();
		return;
	}
	bMainWorldGenshinOneShotPlaying = false;

	if (HorizontalSpeed < 8.0f)
	{
		PlayLoop(MainWorldGenshinIdle, EWorldWalkerMainMotionState::Idle, 1.0f);
	}
	else if (HorizontalSpeed < 165.0f)
	{
		PlayLoop(
			MainWorldGenshinWalk,
			EWorldWalkerMainMotionState::Walk,
			FMath::Clamp(HorizontalSpeed / 125.0f, 0.75f, 1.25f));
	}
	else if (HorizontalSpeed < 540.0f)
	{
		PlayLoop(
			MainWorldGenshinRun,
			EWorldWalkerMainMotionState::Run,
			FMath::Clamp(HorizontalSpeed / 430.0f, 0.75f, 1.15f));
	}
	else
	{
		PlayLoop(
			MainWorldGenshinSprint,
			EWorldWalkerMainMotionState::Sprint,
			FMath::Clamp(HorizontalSpeed / 430.0f, 0.80f, 1.20f));
	}

	RememberMovement();
	(void)DeltaSeconds;
}

bool AWorldWalkerCharacter::ConfigureLumineVisual(USkeletalMeshComponent* PoseSource)
{
	static const TCHAR* LumineMeshPath =
		TEXT("/Game/WorldWalker/Shared/ThirdParty/GenshinExtract/Lumine/SK_WW_Lumine_CurrentGame.SK_WW_Lumine_CurrentGame");
	USkeletalMesh* LumineMesh = LoadObject<USkeletalMesh>(nullptr, LumineMeshPath);
	if (!PoseSource || !LumineMesh)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Lumine visual is unavailable; keeping the modular anime fallback. Mesh=%s"),
			LumineMesh ? TEXT("ok") : TEXT("missing"));
		return false;
	}

	if (!MainWorldLumineMesh)
	{
		MainWorldLumineMesh = NewObject<UPoseableMeshComponent>(this, TEXT("MainWorldLumineMesh"));
		AddInstanceComponent(MainWorldLumineMesh);
		MainWorldLumineMesh->SetupAttachment(GetRootComponent());
		MainWorldLumineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MainWorldLumineMesh->SetCastShadow(true);
		MainWorldLumineMesh->RegisterComponent();
	}

	MainWorldLumineMesh->SetSkinnedAssetAndUpdate(LumineMesh, true);
	const FReferenceSkeleton& LumineSkeleton = LumineMesh->GetRefSkeleton();
	const TArray<FTransform> LumineReferencePose =
		BuildReferenceComponentTransforms(LumineSkeleton);
	const int32 HeadIndex = LumineSkeleton.FindBoneIndex(TEXT("Bip001-Head"));
	const int32 LeftFootIndex = LumineSkeleton.FindBoneIndex(TEXT("Bip001-L-Foot"));
	const int32 RightFootIndex = LumineSkeleton.FindBoneIndex(TEXT("Bip001-R-Foot"));
	if (HeadIndex == INDEX_NONE || LeftFootIndex == INDEX_NONE || RightFootIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Error, TEXT("Lumine visual is missing required scale reference bones."));
		return false;
	}
	const float FootReferenceZ = FMath::Min(
		LumineReferencePose[LeftFootIndex].GetLocation().Z,
		LumineReferencePose[RightFootIndex].GetLocation().Z);
	const float BoneHeight = FMath::Max(
		1.0f,
		LumineReferencePose[HeadIndex].GetLocation().Z - FootReferenceZ);
	// Head-to-ankle height is about 145 cm for the shared movement capsule.
	// FBX helper/effect nodes make the whole-mesh bounds unusable for scaling.
	MainWorldLumineVisualScale = FMath::Clamp(145.0f / BoneHeight, 0.01f, 150.0f);
	MainWorldLumineBaseLocation = FVector(
		0.0f,
		0.0f,
		-72.0f - FootReferenceZ * MainWorldLumineVisualScale);
	MainWorldLumineBaseRotation = FRotator(0.0f, -90.0f, 0.0f);
	MainWorldLumineMesh->SetRelativeLocation(MainWorldLumineBaseLocation);
	MainWorldLumineMesh->SetRelativeRotation(MainWorldLumineBaseRotation);
	MainWorldLumineMesh->SetRelativeScale3D(FVector(MainWorldLumineVisualScale));
	MainWorldLumineMesh->SetVisibility(false, true);
	MainWorldLumineMesh->SetHiddenInGame(true, true);

	static const TCHAR* TravelerSwordPath =
		TEXT("/Game/WorldWalker/Shared/ThirdParty/GenshinExtract/Lumine/Weapon/SM_WW_Lumine_TravelerSword.SM_WW_Lumine_TravelerSword");
	if (UStaticMesh* TravelerSword = LoadObject<UStaticMesh>(nullptr, TravelerSwordPath))
	{
		const int32 SourceWeaponIndex = PoseSource->GetBoneIndex(TEXT("WeaponR"));
		const int32 RightHandIndex = LumineSkeleton.FindBoneIndex(TEXT("Bip001-R-Hand"));
		const int32 WeaponIndex = LumineSkeleton.FindBoneIndex(TEXT("WeaponR"));
		const bool bUseAnimatedWeaponBone =
			SourceWeaponIndex != INDEX_NONE && WeaponIndex != INDEX_NONE;
		const FName GripBone = bUseAnimatedWeaponBone
			? FName(TEXT("WeaponR"))
			: FName(TEXT("Bip001-R-Hand"));
		// Prefer the authored WeaponR animation. The hand-relative reference offset
		// remains a safe fallback for sources that do not carry the helper bone.
		const FTransform SwordGripTransform =
			!bUseAnimatedWeaponBone
				&& RightHandIndex != INDEX_NONE && WeaponIndex != INDEX_NONE
				? LumineReferencePose[WeaponIndex].GetRelativeTransform(
					LumineReferencePose[RightHandIndex])
				: FTransform::Identity;
		if (!MainWorldLumineSword)
		{
			MainWorldLumineSword = NewObject<UStaticMeshComponent>(this, TEXT("MainWorldLumineSword"));
			AddInstanceComponent(MainWorldLumineSword);
			MainWorldLumineSword->SetupAttachment(MainWorldLumineMesh, GripBone);
			MainWorldLumineSword->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			MainWorldLumineSword->SetCastShadow(true);
			MainWorldLumineSword->RegisterComponent();
		}
		else
		{
			MainWorldLumineSword->AttachToComponent(
				MainWorldLumineMesh,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				GripBone);
		}
		MainWorldLumineSword->SetStaticMesh(TravelerSword);
		MainWorldLumineSword->SetRelativeTransform(SwordGripTransform);
		MainWorldLumineSword->SetVisibility(false, true);
		MainWorldLumineSword->SetHiddenInGame(true, true);
		UE_LOG(
			LogTemp,
			Display,
			TEXT("WW_LUMINE_SWORD_READY bone=%s authoredWeaponPose=%s offset=%s mesh=%s"),
			*GripBone.ToString(),
			bUseAnimatedWeaponBone ? TEXT("true") : TEXT("false"),
			*SwordGripTransform.ToHumanReadableString(),
			TravelerSwordPath);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Traveler sword is unavailable: %s"), TravelerSwordPath);
	}
	BuildLumineBoneMap(PoseSource);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Lumine visual scale configured. BoneHeight=%.3f FootZ=%.3f Scale=%.3f BaseZ=%.3f"),
		BoneHeight,
		FootReferenceZ,
		MainWorldLumineVisualScale,
		MainWorldLumineBaseLocation.Z);
	return LumineBoneLinks.Num() >= 40;
}

void AWorldWalkerCharacter::BuildLumineBoneMap(USkeletalMeshComponent* PoseSource)
{
	LumineBoneLinks.Reset();
	LumineLinkIndexByTargetBone.Reset();
	bLuminePoseUsesPlayerGirlSkeleton = false;
	if (!PoseSource || !PoseSource->GetSkeletalMeshAsset() || !MainWorldLumineMesh)
	{
		return;
	}

	USkeletalMesh* SourceMesh = PoseSource->GetSkeletalMeshAsset();
	USkeletalMesh* TargetMesh = Cast<USkeletalMesh>(MainWorldLumineMesh->GetSkinnedAsset());
	if (!SourceMesh || !TargetMesh)
	{
		return;
	}

	const FReferenceSkeleton& SourceSkeleton = SourceMesh->GetRefSkeleton();
	const FReferenceSkeleton& TargetSkeleton = TargetMesh->GetRefSkeleton();
	const TArray<FTransform> SourceReferencePose =
		BuildReferenceComponentTransforms(SourceSkeleton);
	const TArray<FTransform> TargetReferencePose =
		BuildReferenceComponentTransforms(TargetSkeleton);
	LumineLinkIndexByTargetBone.Init(INDEX_NONE, TargetSkeleton.GetNum());

	auto AddBoneLink = [&](const FLumineBoneNamePair& Pair)
	{
		int32 SourceIndex = SourceSkeleton.FindBoneIndex(Pair.Source);
		if (SourceIndex == INDEX_NONE)
		{
			// UE Interchange normalizes FBX namespace separators (':') to '_'.
			// Accept either spelling so exports from legacy FBX and Unity 6 work.
			const FString OriginalSourceName(Pair.Source);
			const FString SanitizedSourceName =
				OriginalSourceName.Replace(TEXT(":"), TEXT("_"));
			SourceIndex = SourceSkeleton.FindBoneIndex(*SanitizedSourceName);
			if (SourceIndex == INDEX_NONE)
			{
				FString NamespaceFreeSourceName;
				if (OriginalSourceName.Split(
					TEXT(":"), nullptr, &NamespaceFreeSourceName))
				{
					SourceIndex =
						SourceSkeleton.FindBoneIndex(*NamespaceFreeSourceName);
				}
			}
		}
		const int32 TargetIndex = TargetSkeleton.FindBoneIndex(Pair.Target);
		if (SourceIndex == INDEX_NONE || TargetIndex == INDEX_NONE)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Lumine bone map skipped. Source=%s Target=%s"),
				Pair.Source,
				Pair.Target);
			return;
		}

		FLumineBoneLink& Link = LumineBoneLinks.AddDefaulted_GetRef();
		Link.SourceBoneIndex = SourceIndex;
		Link.TargetBoneIndex = TargetIndex;
		Link.SourceReferenceComponentTransform = SourceReferencePose[SourceIndex];
		Link.TargetReferenceComponentTransform = TargetReferencePose[TargetIndex];
		LumineLinkIndexByTargetBone[TargetIndex] = LumineBoneLinks.Num() - 1;
	};

	const bool bExactSkeleton = SourceMesh == TargetMesh;
	const bool bCompatiblePlayerGirlSkeleton =
		SourceSkeleton.FindBoneIndex(TEXT("Bip001-Pelvis")) != INDEX_NONE
		&& TargetSkeleton.FindBoneIndex(TEXT("Bip001-Pelvis")) != INDEX_NONE;
	if (bExactSkeleton || bCompatiblePlayerGirlSkeleton)
	{
		for (int32 TargetIndex = 0; TargetIndex < TargetSkeleton.GetNum(); ++TargetIndex)
		{
			const int32 SourceIndex = SourceSkeleton.FindBoneIndex(
				TargetSkeleton.GetBoneName(TargetIndex));
			if (SourceIndex == INDEX_NONE)
			{
				continue;
			}
			FLumineBoneLink& Link = LumineBoneLinks.AddDefaulted_GetRef();
			Link.SourceBoneIndex = SourceIndex;
			Link.TargetBoneIndex = TargetIndex;
			Link.SourceReferenceComponentTransform = SourceReferencePose[SourceIndex];
			Link.TargetReferenceComponentTransform = TargetReferencePose[TargetIndex];
			LumineLinkIndexByTargetBone[TargetIndex] = LumineBoneLinks.Num() - 1;
		}
		bLuminePoseUsesPlayerGirlSkeleton = bCompatiblePlayerGirlSkeleton;
	}

	const bool bMixamoSource = !bExactSkeleton && (
		SourceSkeleton.FindBoneIndex(TEXT("mixamorig:Hips")) != INDEX_NONE
		|| SourceSkeleton.FindBoneIndex(TEXT("mixamorig_Hips")) != INDEX_NONE
		|| SourceSkeleton.FindBoneIndex(TEXT("Hips")) != INDEX_NONE);
	if (bMixamoSource)
	{
		for (const FLumineBoneNamePair& Pair : MixamoLumineBoneNamePairs)
		{
			AddBoneLink(Pair);
		}
	}
	else if (!bExactSkeleton && !bCompatiblePlayerGirlSkeleton)
	{
		for (const FLumineBoneNamePair& Pair : LumineBoneNamePairs)
		{
			AddBoneLink(Pair);
		}
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Lumine bone map built. Source=%s Profile=%s Links=%d"),
		*SourceMesh->GetName(),
		bExactSkeleton ? TEXT("ExactPlayerGirl")
			: (bCompatiblePlayerGirlSkeleton ? TEXT("CompatiblePlayerGirl")
				: (bMixamoSource ? TEXT("Mixamo") : TEXT("AnimeCharacterPack"))),
		LumineBoneLinks.Num());
}

void AWorldWalkerCharacter::UpdateMainWorldLuminePose(const float DeltaSeconds)
{
	USkeletalMeshComponent* PoseSource = GetMainWorldPoseSource();
	if (!bMainWorldLumineConfigured || !bMainWorldAnimeFormActive ||
		!PoseSource || !MainWorldLumineMesh || LumineBoneLinks.IsEmpty())
	{
		return;
	}

	USkeletalMesh* TargetMesh = Cast<USkeletalMesh>(MainWorldLumineMesh->GetSkinnedAsset());
	if (!TargetMesh)
	{
		return;
	}

	const FReferenceSkeleton& TargetSkeleton = TargetMesh->GetRefSkeleton();
	if (LumineLinkIndexByTargetBone.Num() != TargetSkeleton.GetNum())
	{
		return;
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("WWMotionDiagnostics"))
		&& GetWorld()
		&& GetWorld()->GetTimeSeconds() >= NextMainWorldMotionDiagnosticTime)
	{
		NextMainWorldMotionDiagnosticTime = GetWorld()->GetTimeSeconds() + 0.25f;
		auto SourceBoneTransform = [&](const FName BoneName)
		{
			const int32 BoneIndex = PoseSource->GetBoneIndex(BoneName);
			return BoneIndex == INDEX_NONE
				? FTransform::Identity
				: PoseSource->GetBoneTransform(BoneIndex, FTransform::Identity);
		};
		const FTransform RootTransform = SourceBoneTransform(TEXT("Bip001"));
		const FTransform LeftThighTransform = SourceBoneTransform(TEXT("Bip001-L-Thigh"));
		const FTransform LeftCalfTransform = SourceBoneTransform(TEXT("Bip001-L-Calf"));
		const FTransform LeftFootTransform = SourceBoneTransform(TEXT("Bip001-L-Foot"));
		UE_LOG(
			LogTemp,
			Display,
			TEXT("WW_MOTION_BONES State=%s RootRot=%s ThighRot=%s CalfRot=%s FootLocation=%s"),
			GetMainMotionStateName(CurrentMainWorldMotionState),
			*RootTransform.Rotator().ToCompactString(),
			*LeftThighTransform.Rotator().ToCompactString(),
			*LeftCalfTransform.Rotator().ToCompactString(),
			*LeftFootTransform.GetLocation().ToCompactString());
	}

	const float TargetArmRotationWeight = bLuminePoseUsesPlayerGirlSkeleton
		? 1.0f
		: GetLuminePrimaryArmRotationWeight(CurrentMainWorldMotionState);
	CurrentLuminePrimaryArmRotationWeight = DeltaSeconds > UE_SMALL_NUMBER
		? FMath::FInterpTo(
			CurrentLuminePrimaryArmRotationWeight,
			TargetArmRotationWeight,
			DeltaSeconds,
			12.0f)
		: TargetArmRotationWeight;

	// Build the complete target component pose in hierarchy order. The previous
	// implementation queried the poseable component while parent transforms were
	// still being changed in the same frame; arm children could therefore use a
	// stale clavicle/upper-arm transform and appear to slide or swing incorrectly.
	const TArray<FTransform>& TargetReferenceLocalPose = TargetSkeleton.GetRefBonePose();
	TArray<FTransform> TargetAnimatedComponentPose;
	TargetAnimatedComponentPose.SetNum(TargetSkeleton.GetNum());
	for (int32 TargetIndex = 0; TargetIndex < TargetSkeleton.GetNum(); ++TargetIndex)
	{
		const int32 ParentIndex = TargetSkeleton.GetParentIndex(TargetIndex);
		FTransform TargetAnimated = ParentIndex == INDEX_NONE
			? TargetReferenceLocalPose[TargetIndex]
			: TargetReferenceLocalPose[TargetIndex]
				* TargetAnimatedComponentPose[ParentIndex];

		const int32 LinkIndex = LumineLinkIndexByTargetBone[TargetIndex];
		if (LumineBoneLinks.IsValidIndex(LinkIndex))
		{
			const FLumineBoneLink& Link = LumineBoneLinks[LinkIndex];
			const FTransform SourceAnimated =
				PoseSource->GetBoneTransform(Link.SourceBoneIndex, FTransform::Identity);
			// Even with identical bone names, animation FBXs that passed through
			// Blender can contain baked local translations/scales based on Blender's
			// reconstructed rest pose. Copying those values stretches skinned vertices
			// into long spikes. Preserve Lumine's authored reference bone lengths and
			// transfer only the component-space rotation delta from the action.
			FQuat SourceRotationDelta =
				SourceAnimated.GetRotation()
				* Link.SourceReferenceComponentTransform.GetRotation().Inverse();
			const FName TargetBoneName = TargetSkeleton.GetBoneName(TargetIndex);
			const bool bUseUnfilteredPlayerGirlAttack =
				bLuminePoseUsesPlayerGirlSkeleton
				&& CurrentMainWorldMotionState == EWorldWalkerMainMotionState::Attack;
			if (IsLumineArmChainBone(TargetBoneName) && !bUseUnfilteredPlayerGirlAttack)
			{
				// The Mixamo source and Lumine use different shoulder widths, bone axes,
				// limb lengths and hand proportions. Limit every action's complete arm
				// chain, with extra damping on clavicles, wrists and fingers, while still
				// retaining enough source rotation to communicate the action timing.
				const float BoneRotationWeight = FMath::Clamp(
					CurrentLuminePrimaryArmRotationWeight
						* GetLumineArmBoneWeightMultiplier(TargetBoneName),
					0.0f,
					1.0f);
				SourceRotationDelta = FQuat::Slerp(
					FQuat::Identity,
					SourceRotationDelta,
					BoneRotationWeight);
			}
			FQuat TargetRotation =
				SourceRotationDelta
				* Link.TargetReferenceComponentTransform.GetRotation();
			TargetRotation.Normalize();
			TargetAnimated.SetRotation(TargetRotation);
		}
		TargetAnimatedComponentPose[TargetIndex] = TargetAnimated;
	}

	// PoseableMesh converts component-space inputs to parent-local transforms.
	// Submit them in target hierarchy order so each conversion sees the parent
	// already updated for this frame.
	for (int32 TargetIndex = 0; TargetIndex < TargetSkeleton.GetNum(); ++TargetIndex)
	{
		const int32 LinkIndex = LumineLinkIndexByTargetBone[TargetIndex];
		if (!LumineBoneLinks.IsValidIndex(LinkIndex))
		{
			continue;
		}
		MainWorldLumineMesh->SetBoneTransformByName(
			TargetSkeleton.GetBoneName(TargetIndex),
			TargetAnimatedComponentPose[TargetIndex],
			EBoneSpaces::ComponentSpace);
	}
	MainWorldLumineMesh->RefreshBoneTransforms();

	// Blender's baked PlayerGirl clips preserve the correct head-to-foot span,
	// but their component pose is globally shifted down by roughly 0.86 source
	// units while root/pelvis translation remains zero. Reintroducing every
	// baked translation stretches the mesh, so correct only the visual root
	// height: keep the lower animated ankle 16 cm above the capsule floor, where
	// Lumine's shoe sole meets the ground.
	const FTransform LeftFootTransform = MainWorldLumineMesh->GetBoneTransformByName(
		TEXT("Bip001-L-Foot"), EBoneSpaces::ComponentSpace);
	const FTransform RightFootTransform = MainWorldLumineMesh->GetBoneTransformByName(
		TEXT("Bip001-R-Foot"), EBoneSpaces::ComponentSpace);
	const float LowestAnimatedFootZ = FMath::Min(
		LeftFootTransform.GetLocation().Z,
		RightFootTransform.GetLocation().Z);
	MainWorldLumineBaseLocation.Z =
		-72.0f - LowestAnimatedFootZ * MainWorldLumineVisualScale;
	MainWorldLumineMesh->SetRelativeLocation(MainWorldLumineBaseLocation);
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

	if (MainWorldLumineMesh)
	{
		MainWorldLumineMesh->SetRelativeLocation(MainWorldLumineBaseLocation);
		MainWorldLumineMesh->SetRelativeRotation(MainWorldLumineBaseRotation);
		MainWorldLumineMesh->SetRelativeScale3D(FVector(MainWorldLumineVisualScale));
	}
	if (MainWorldGenshinMotionSource)
	{
		MainWorldGenshinMotionSource->GlobalAnimRateScale = 1.0f;
		bMainWorldGenshinOneShotPlaying = false;
		RefreshMainWorldGenshinMotion(0.0f, true);
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
		if (MainWorldLumineMesh)
		{
			MainWorldLumineMesh->SetRelativeLocation(MainWorldLumineBaseLocation + LocationOffset);
			MainWorldLumineMesh->SetRelativeRotation(
				MainWorldLumineBaseRotation + FRotator(PitchOffset, 0.0f, RollOffset));
		}
		if (USkeletalMeshComponent* PoseSource = GetMainWorldPoseSource())
		{
			PoseSource->GlobalAnimRateScale = PoseWeight > 0.01f ? 0.72f : 1.0f;
		}
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

	if (bFantasyFormAvailable && !LoadFantasyPresentationAssets())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("W01 player presentation unavailable; the prototype body remains visible."));
	}

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
	if (bMainWorldLumineConfigured)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("W02 avatar configured. Model=Lumine BoneLinks=%d MaterialProfile=UnlitToon FormToggle=Disabled"),
			LumineBoneLinks.Num());
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("W02 avatar configured. Model=ModularAnimeFallback RimLightIntensity=0.00 ToonTint=0.035 FormToggle=Disabled"));
	}
	return true;
}

void AWorldWalkerCharacter::ApplyW02AnimeMaterialTuning()
{
	if (bMainWorldLumineConfigured)
	{
		UE_LOG(LogTemp, Display, TEXT("W02 Lumine uses local unlit toon materials; modular fallback toon tuning skipped."));
		return;
	}

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

void AWorldWalkerCharacter::ConfigureFantasyProfession(
	const EFantasyPlayerProfession Profession)
{
	RequestedFantasyProfession = Profession == EFantasyPlayerProfession::None
		? EFantasyPlayerProfession::Knight
		: Profession;
	if (bFantasyFormAvailable)
	{
		LoadFantasyPresentationAssets();
		ApplyWorldFormVisibility();
		RefreshFantasyLocomotionAnimation(true);
	}
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
	const bool bShowLumine = bVisible && bMainWorldLumineConfigured && MainWorldLumineMesh;
	if (USkeletalMeshComponent* HeadComponent = GetMesh())
	{
		const bool bShowFallback = bVisible && !bShowLumine;
		HeadComponent->SetVisibility(bShowFallback, true);
		HeadComponent->SetHiddenInGame(!bShowFallback, true);
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
			const bool bShowFallback = bVisible && !bShowLumine;
			Part->SetVisibility(bShowFallback, true);
			Part->SetHiddenInGame(!bShowFallback, true);
		}
	}

	if (MainWorldLumineMesh)
	{
		MainWorldLumineMesh->SetVisibility(bShowLumine, true);
		MainWorldLumineMesh->SetHiddenInGame(!bShowLumine, true);
		MainWorldLumineMesh->SetRenderInMainPass(bShowLumine);
		MainWorldLumineMesh->SetCastShadow(bShowLumine);
	}
	if (MainWorldLumineSword)
	{
		MainWorldLumineSword->SetVisibility(bShowLumine, true);
		MainWorldLumineSword->SetHiddenInGame(!bShowLumine, true);
		MainWorldLumineSword->SetRenderInMainPass(bShowLumine);
		MainWorldLumineSword->SetCastShadow(bShowLumine);
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

	// Ranger and Nun currently use the closest complete compatible presentation
	// (Rogue and Wizard respectively) while retaining distinct decks and UI identity.
	const int32 FirstProfileIndex =
		(RequestedFantasyProfession == EFantasyPlayerProfession::Mage
			|| RequestedFantasyProfession == EFantasyPlayerProfession::Nun) ? 0 : 1;
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
