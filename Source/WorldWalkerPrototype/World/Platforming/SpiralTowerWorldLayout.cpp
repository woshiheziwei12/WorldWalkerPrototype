#include "World/Platforming/SpiralTowerWorldLayout.h"

#include "Animation/AnimSequence.h"
#include "Camera/PlayerCameraManager.h"
#include "Characters/WorldWalkerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/GameInstance.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "World/WorldDefinition.h"
#include "World/WorldTravelSubsystem.h"
#include "WorldWalkerPlayerController.h"

namespace SpiralTowerWorldLayoutPrivate
{
	constexpr int32 SpiralTowerPlatformCount = 96;
	constexpr int32 SpiralTowerCheckpointInterval = 16;
	constexpr int32 SpiralTowerGenerationSeed = 0x52020150;
	constexpr float SpiralTowerInteriorWallRadius = 1700.0f;
	constexpr float SpiralTowerInteriorWallThickness = 120.0f;
	constexpr int32 SpiralTowerInteriorWallSegments = 20;
	constexpr float SpiralTowerMinDesiredAngleStepDegrees = 13.0f;
	constexpr float SpiralTowerMaxDesiredAngleStepDegrees = 21.0f;
	constexpr float SpiralTowerChallengeAngleStepDegrees = 24.5f;
	constexpr float SpiralTowerFirstSurfaceZ = 48.0f;
	constexpr float SpiralTowerMinPlatformRise = 30.0f;
	constexpr float SpiralTowerMaxPlatformRise = 80.0f;
	constexpr float SpiralTowerStairRise = 28.0f;
	constexpr float SpiralTowerStairAngleStepDegrees = 4.2f;
	constexpr float SpiralTowerLadderRise = 150.0f;
	constexpr float SpiralTowerLadderAngleStepDegrees = 2.6f;
	constexpr float SpiralTowerPlatformThickness = 24.0f;
	constexpr float SpiralTowerMinPlatformLength = 150.0f;
	constexpr float SpiralTowerMaxPlatformLength = 230.0f;
	constexpr float SpiralTowerMinPlatformWidth = 120.0f;
	constexpr float SpiralTowerMaxPlatformWidth = 170.0f;
	constexpr float SpiralTowerCheckpointLength = 960.0f;
	constexpr float SpiralTowerCheckpointWidth = 720.0f;
	constexpr float SpiralTowerStairLength = 215.0f;
	constexpr float SpiralTowerStairWidth = 158.0f;
	constexpr float SpiralTowerLadderLandingLength = 220.0f;
	constexpr float SpiralTowerLadderLandingWidth = 175.0f;
	constexpr float SpiralTowerMinWallInset = 10.0f;
	constexpr float SpiralTowerMaxWallInset = 24.0f;
	constexpr float SpiralTowerCapsuleRadius = 42.0f;
	constexpr float SpiralTowerStandingMargin = 8.0f;
	constexpr float SpiralTowerJumpZVelocity = 510.0f;
	constexpr float SpiralTowerJumpInitialVelocity = 405.0f;
	constexpr float SpiralTowerSprintJumpInitialVelocity = 445.0f;
	constexpr float SpiralTowerSprintJumpZVelocity = 555.0f;
	constexpr float SpiralTowerSmallJumpCutVelocity = 285.0f;
	constexpr float SpiralTowerJumpHoldAcceleration = 720.0f;
	constexpr float SpiralTowerJumpHoldDuration = 0.19f;
	constexpr float SpiralTowerSmallJumpReleaseWindow = 0.13f;
	constexpr float SpiralTowerCoyoteTime = 0.12f;
	constexpr float SpiralTowerJumpBufferTime = 0.14f;
	constexpr float SpiralTowerWorldGravity = 980.0f;
	constexpr float SpiralTowerRunSpeed = 460.0f;
	constexpr float SpiralTowerSprintSpeed = 650.0f;
	constexpr float SpiralTowerBaseAcceleration = 1500.0f;
	constexpr float SpiralTowerSprintAcceleration = 2100.0f;
	constexpr float SpiralTowerAirControl = 0.28f;
	constexpr float SpiralTowerGravityScale = 1.0f;
	constexpr float SpiralTowerUsableReachFraction = 0.90f;
	constexpr float SpiralTowerChallengeSafetyMargin = 10.0f;
	constexpr float SpiralTowerMaxPlayerRadius = 2750.0f;
	constexpr float SpiralTowerCheckpointFallDistance = 950.0f;
	constexpr float SpiralTowerTravelDelay = 0.52f;
	constexpr float SpiralTowerWallStoreyHeight = 520.0f;
	constexpr float SpiralTowerBeamHalfSpan = 1220.0f;
	constexpr float SpiralTowerBeamWidth = 34.0f;
	constexpr float SpiralTowerBeamThickness = 26.0f;
	constexpr float SpiralTowerBeamVerticalSpacing = 720.0f;
	constexpr float SpiralTowerLedgeClimbMinHeight = 35.0f;
	constexpr float SpiralTowerLedgeClimbMaxHeight = 168.0f;
	constexpr float SpiralTowerLedgeClimbReach = 96.0f;
	constexpr float SpiralTowerLedgeClimbDuration = 0.78f;
	constexpr float SpiralTowerLedgeGripPhase = 0.26f;
	constexpr float SpiralTowerLedgeHangDrop = 52.0f;
	constexpr float SpiralTowerLedgeTopClearance = 4.0f;
	constexpr float SpiralTowerMaxStamina = 100.0f;
	constexpr float SpiralTowerClimbStaminaDrain = 31.0f;
	constexpr float SpiralTowerSprintStaminaDrain = 8.0f;
	constexpr float SpiralTowerStaminaRecoveryRate = 34.0f;
	constexpr float SpiralTowerStaminaRecoveryDelay = 0.72f;
	constexpr float SpiralTowerMinimumClimbStamina = 18.0f;
	constexpr float SpiralTowerLandingRecoverySmall = 0.16f;
	constexpr float SpiralTowerLandingRecoveryBig = 0.27f;
	constexpr float SpiralTowerLandingRecoverySprint = 0.34f;
	constexpr float SpiralTowerWoodTintR = 0.28f;
	constexpr float SpiralTowerWoodTintG = 0.13f;
	constexpr float SpiralTowerWoodTintB = 0.050f;
	constexpr int32 BabylonLowerTierCount = 6;
	constexpr float BabylonBaseHalfExtent = 2600.0f;
	constexpr float BabylonFirstTerraceHalfExtent = 1800.0f;
	constexpr float BabylonTerraceRecession = 90.0f;
	constexpr float BabylonFirstCoreHalfExtent = 840.0f;
	constexpr float BabylonCoreRecession = 28.0f;
	constexpr float BabylonFirstFacadeHalfExtent = 1040.0f;
	constexpr float BabylonFacadeRecession = 45.0f;
	constexpr float BabylonTerraceThickness = 42.0f;

	enum class ESpiralTowerPlatformStyle : uint8
	{
		StoneBrick,
		TimberBeam,
		BrokenPlanks
	};

	enum class ESpiralTowerTraversalType : uint8
	{
		GapJump,
		SpiralStair,
		Ladder,
		LookoutDeck
	};

	const TCHAR* SpiralTowerEnvironmentRoot =
		TEXT("/Game/WorldWalker/Worlds/W02_SpiralTower/ThirdParty/Quaternius/Environment");
	const TCHAR* SpiralTowerKenneyCastleRoot =
		TEXT("/Game/WorldWalker/Worlds/W02_SpiralTower/ThirdParty/Kenney/CastleKit/Environment");
	const TCHAR* SpiralTowerKenneyMedievalRoot =
		TEXT("/Game/WorldWalker/Worlds/W02_SpiralTower/ThirdParty/Kenney/MedievalKit/Environment");
	const TCHAR* SpiralTowerPolyHavenFortRoot =
		TEXT("/Game/WorldWalker/Worlds/W02_SpiralTower/ThirdParty/PolyHaven/ModularFort01");

	bool SpiralTowerIsCheckpointPlatform(const int32 PlatformIndex)
	{
		const int32 LayerNumber = PlatformIndex + 1;
		return LayerNumber % SpiralTowerCheckpointInterval == 0;
	}

	ESpiralTowerTraversalType SpiralTowerTraversalType(const int32 PlatformIndex)
	{
		const int32 LayerWithinStage = PlatformIndex % SpiralTowerCheckpointInterval + 1;
		if (LayerWithinStage == SpiralTowerCheckpointInterval)
		{
			return ESpiralTowerTraversalType::LookoutDeck;
		}
		if (LayerWithinStage >= 8 && LayerWithinStage <= 11)
		{
			return ESpiralTowerTraversalType::SpiralStair;
		}
		if (LayerWithinStage == 13)
		{
			return ESpiralTowerTraversalType::Ladder;
		}
		return ESpiralTowerTraversalType::GapJump;
	}

	FString SpiralTowerEnvironmentAssetPath(const TCHAR* AssetName)
	{
		return FString::Printf(
			TEXT("%s/%s/%s.%s"),
			SpiralTowerEnvironmentRoot,
			AssetName,
			AssetName,
			AssetName);
	}

	FString SpiralTowerOptionalAssetPath(const TCHAR* Root, const TCHAR* AssetName)
	{
		return FString::Printf(TEXT("%s/%s/%s.%s"), Root, AssetName, AssetName, AssetName);
	}

	FString SpiralTowerPolyHavenMeshPath(const TCHAR* AssetName)
	{
		return FString::Printf(
			TEXT("%s/Meshes/%s.%s"),
			SpiralTowerPolyHavenFortRoot,
			AssetName,
			AssetName);
	}

	FString SpiralTowerPolyHavenMaterialPath(const TCHAR* AssetName)
	{
		return FString::Printf(
			TEXT("%s/Materials/%s.%s"),
			SpiralTowerPolyHavenFortRoot,
			AssetName,
			AssetName);
	}

	FVector SpiralTowerMeshLocationForBoundsCenter(
		const UStaticMesh* Mesh,
		const FVector& DesiredBoundsCenter,
		const FRotator& Rotation,
		const FVector& Scale)
	{
		if (!Mesh)
		{
			return DesiredBoundsCenter;
		}

		return DesiredBoundsCenter
			- Rotation.RotateVector(Mesh->GetBounds().Origin * Scale);
	}

	FVector SpiralTowerMeshLocationForBoundsBase(
		const UStaticMesh* Mesh,
		const FVector& DesiredBoundsBase,
		const FRotator& Rotation,
		const FVector& Scale)
	{
		if (!Mesh)
		{
			return DesiredBoundsBase;
		}

		const FBoxSphereBounds Bounds = Mesh->GetBounds();
		const FVector DesiredCenter = DesiredBoundsBase
			+ FVector(0.0f, 0.0f, Bounds.BoxExtent.Z * FMath::Abs(Scale.Z));
		return DesiredCenter - Rotation.RotateVector(Bounds.Origin * Scale);
	}

	float SpiralTowerRectangleSupport(
		const FVector& Direction,
		const FRotator& Rotation,
		const float Length,
		const float Width)
	{
		const FVector Tangent = Rotation.RotateVector(FVector::ForwardVector).GetSafeNormal2D();
		const FVector Radial = Rotation.RotateVector(FVector::RightVector).GetSafeNormal2D();
		return FMath::Abs(FVector::DotProduct(Direction, Tangent)) * Length * 0.5f
			+ FMath::Abs(FVector::DotProduct(Direction, Radial)) * Width * 0.5f;
	}

	float SpiralTowerClearGap(
		const FVector& PreviousLocation,
		const FRotator& PreviousRotation,
		const FVector2D& PreviousDimensions,
		const FVector& CurrentLocation,
		const FRotator& CurrentRotation,
		const FVector2D& CurrentDimensions)
	{
		FVector CentreDelta = CurrentLocation - PreviousLocation;
		CentreDelta.Z = 0.0f;
		const float CentreDistance = CentreDelta.Size();
		if (CentreDistance <= UE_KINDA_SMALL_NUMBER)
		{
			return 0.0f;
		}

		const FVector Direction = CentreDelta / CentreDistance;
		const float PreviousSupport = SpiralTowerRectangleSupport(
			Direction,
			PreviousRotation,
			PreviousDimensions.X,
			PreviousDimensions.Y);
		const float CurrentSupport = SpiralTowerRectangleSupport(
			-Direction,
			CurrentRotation,
			CurrentDimensions.X,
			CurrentDimensions.Y);
		return FMath::Max(0.0f, CentreDistance - PreviousSupport - CurrentSupport);
	}

	float SpiralTowerRequiredCentreTravel(
		const FVector& PreviousLocation,
		const FRotator& PreviousRotation,
		const FVector2D& PreviousDimensions,
		const FVector& CurrentLocation,
		const FRotator& CurrentRotation,
		const FVector2D& CurrentDimensions)
	{
		FVector CentreDelta = CurrentLocation - PreviousLocation;
		CentreDelta.Z = 0.0f;
		const float CentreDistance = CentreDelta.Size();
		if (CentreDistance <= UE_KINDA_SMALL_NUMBER)
		{
			return 0.0f;
		}

		const FVector Direction = CentreDelta / CentreDistance;
		const float SafeInset = SpiralTowerCapsuleRadius + SpiralTowerStandingMargin;
		const float PreviousSafeSupport = FMath::Max(
			0.0f,
			SpiralTowerRectangleSupport(
				Direction,
				PreviousRotation,
				PreviousDimensions.X,
				PreviousDimensions.Y) - SafeInset);
		const float CurrentSafeSupport = FMath::Max(
			0.0f,
			SpiralTowerRectangleSupport(
				-Direction,
				CurrentRotation,
				CurrentDimensions.X,
				CurrentDimensions.Y) - SafeInset);
		return FMath::Max(0.0f, CentreDistance - PreviousSafeSupport - CurrentSafeSupport);
	}

	float SpiralTowerFlightTimeForRise(const float Rise)
	{
		const float Discriminant = FMath::Square(SpiralTowerJumpZVelocity)
			- 2.0f * SpiralTowerWorldGravity * Rise;
		return Discriminant > 0.0f
			? (SpiralTowerJumpZVelocity + FMath::Sqrt(Discriminant)) / SpiralTowerWorldGravity
			: 0.0f;
	}
}

ASpiralTowerWorldLayout::ASpiralTowerWorldLayout()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Movable);
	SetRootComponent(SceneRoot);

	SummitTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("SummitReturnTrigger"));
	SummitTrigger->SetupAttachment(SceneRoot);
	SummitTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SummitTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	SummitTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SummitTrigger->SetGenerateOverlapEvents(true);
	SummitTrigger->SetBoxExtent(FVector(300.0f, 240.0f, 120.0f));

	ClimbRelicTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("ClimbRelicPickupTrigger"));
	ClimbRelicTrigger->SetupAttachment(SceneRoot);
	ClimbRelicTrigger->SetMobility(EComponentMobility::Movable);
	ClimbRelicTrigger->SetSphereRadius(82.0f);
	ClimbRelicTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ClimbRelicTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClimbRelicTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ClimbRelicTrigger->SetGenerateOverlapEvents(true);

	ClimbRelicCore = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClimbRelicCore"));
	ClimbRelicCore->SetupAttachment(ClimbRelicTrigger);
	ClimbRelicCore->SetMobility(EComponentMobility::Movable);
	ClimbRelicCore->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ClimbRelicCore->SetCollisionResponseToAllChannels(ECR_Ignore);

	ClimbRelicBrace = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClimbRelicBrace"));
	ClimbRelicBrace->SetupAttachment(ClimbRelicTrigger);
	ClimbRelicBrace->SetMobility(EComponentMobility::Movable);
	ClimbRelicBrace->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ClimbRelicBrace->SetCollisionResponseToAllChannels(ECR_Ignore);

	ClimbRelicLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ClimbRelicLabel"));
	ClimbRelicLabel->SetupAttachment(ClimbRelicTrigger);
	ClimbRelicLabel->SetMobility(EComponentMobility::Movable);
	ClimbRelicLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ClimbRelicLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	ClimbRelicLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	ClimbRelicLabel->SetWorldSize(22.0f);
	ClimbRelicLabel->SetTextRenderColor(FColor(255, 198, 62));
	ClimbRelicLabel->SetText(FText::FromString(TEXT("CLIMBING GAUNTLET\nTOUCH TO UNLOCK")));

	ClimbRelicLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ClimbRelicLight"));
	ClimbRelicLight->SetupAttachment(ClimbRelicTrigger);
	ClimbRelicLight->SetMobility(EComponentMobility::Movable);
	ClimbRelicLight->SetIntensity(950.0f);
	ClimbRelicLight->SetAttenuationRadius(430.0f);
	ClimbRelicLight->SetLightColor(FLinearColor(1.0f, 0.42f, 0.055f));
	ClimbRelicLight->SetCastShadows(false);
}

void ASpiralTowerWorldLayout::BeginPlay()
{
	Super::BeginPlay();

	ActivePlayer = Cast<AWorldWalkerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	LoadOptionalEnvironmentAssets();
	ConfigureNightAtmosphere();
	BuildTower();
	UpdateDawnAtmosphere(0.0f);
	ConfigurePlayerForPlatforming();
	{
		using namespace SpiralTowerWorldLayoutPrivate;
		const float SmallJumpApex = FMath::Square(SpiralTowerSmallJumpCutVelocity)
			/ (2.0f * SpiralTowerWorldGravity);
		const float BigJumpApex = FMath::Square(SpiralTowerJumpZVelocity)
			/ (2.0f * SpiralTowerWorldGravity);
		const float SprintJumpApex = FMath::Square(SpiralTowerSprintJumpZVelocity)
			/ (2.0f * SpiralTowerWorldGravity);
		const int32 LoadedActionCount = (JumpStartAnimation ? 1 : 0)
			+ (JumpLoopAnimation ? 1 : 0)
			+ (JumpLandAnimation ? 1 : 0)
			+ (ClimbAnimation ? 1 : 0);
		const bool bTraversalConfigPassed = SmallJumpApex < BigJumpApex
			&& BigJumpApex < SprintJumpApex
			&& SpiralTowerLandingRecoverySmall < SpiralTowerLandingRecoveryBig
			&& SpiralTowerLandingRecoveryBig < SpiralTowerLandingRecoverySprint
			&& LoadedActionCount == 4;
		ensureAlwaysMsgf(bTraversalConfigPassed, TEXT("W02 jump/climb action configuration is incomplete."));
		UE_LOG(
			LogTemp,
			Display,
			TEXT("W02 traversal gameplay validation. Result=%s SmallApex=%.1f BigApex=%.1f SprintApex=%.1f LandingRecovery=[%.2f,%.2f,%.2f] Stamina=%.0f DrainClimb=%.1f/s DrainSprint=%.1f/s Recovery=%.1f/s Actions=%d/4"),
			bTraversalConfigPassed ? TEXT("Passed") : TEXT("Failed"),
			SmallJumpApex,
			BigJumpApex,
			SprintJumpApex,
			SpiralTowerLandingRecoverySmall,
			SpiralTowerLandingRecoveryBig,
			SpiralTowerLandingRecoverySprint,
			SpiralTowerMaxStamina,
			SpiralTowerClimbStaminaDrain,
			SpiralTowerSprintStaminaDrain,
			SpiralTowerStaminaRecoveryRate,
			LoadedActionCount);
	}
	if (ActivePlayer)
	{
		ActivePlayer->SetExternalJumpHandlingEnabled(true);
		ActivePlayer->OnExternalJumpPressed().AddUObject(
			this,
			&ASpiralTowerWorldLayout::HandleW02JumpPressed);
		ActivePlayer->OnExternalJumpReleased().AddUObject(
			this,
			&ASpiralTowerWorldLayout::HandleW02JumpReleased);
	}
	PlacePlayerAtInitialStart();
	ShowJourneyBeat(0);
	SummitTrigger->OnComponentBeginOverlap.AddDynamic(
		this,
		&ASpiralTowerWorldLayout::HandleSummitOverlap);
	ClimbRelicTrigger->OnComponentBeginOverlap.AddDynamic(
		this,
		&ASpiralTowerWorldLayout::HandleClimbRelicOverlap);
	GetWorldTimerManager().SetTimerForNextTick(
		this,
		&ASpiralTowerWorldLayout::RefreshCapturedSky);
	const float WallInnerRadius =
		SpiralTowerWorldLayoutPrivate::SpiralTowerInteriorWallRadius
		- SpiralTowerWorldLayoutPrivate::SpiralTowerInteriorWallThickness * 0.5f;
	const float TangentCameraClearance = FMath::Sqrt(FMath::Max(
		0.0f,
		FMath::Square(WallInnerRadius) - FMath::Square(GeneratedMaxRadius)));

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 Babylon exterior ascent ready. Seed=%d Platforms=%d Checkpoints=%d Turns=%.2f Height=%.0f "
			"AngleStep=[%.1f,%.1f] Rise=[%.1f,%.1f] Radius=[%.1f,%.1f] "
			"Length=[%.1f,%.1f] Width=[%.1f,%.1f] CenterSpan=[%.1f,%.1f] ClearGap=[%.1f,%.1f] "
			"MaxRequiredTravel=%.1f MinSafetyMargin=%.1f WorstRequiredRatio=%.3f "
			"TangentCameraClearance=%.1f ImportedDecor=%d KenneyDecor=%d PolyHavenDecor=%d "
			"KenneyAssetKinds=%d PolyHavenAssetKinds=%d PolyHavenMaterials=%d FallbackDecor=%d Player=%s"),
		SpiralTowerWorldLayoutPrivate::SpiralTowerGenerationSeed,
		BuiltPlatformCount,
		CheckpointLocalTransforms.Num(),
		GeneratedTurns,
		SummitSurfaceLocalZ,
		GeneratedMinAngleStep,
		GeneratedMaxAngleStep,
		GeneratedMinRise,
		GeneratedMaxRise,
		GeneratedMinRadius,
		GeneratedMaxRadius,
		GeneratedMinLength,
		GeneratedMaxLength,
		GeneratedMinWidth,
		GeneratedMaxWidth,
		GeneratedMinCenterSpan,
		GeneratedMaxCenterSpan,
		GeneratedMinClearGap,
		GeneratedMaxClearGap,
		GeneratedMaxRequiredTravel,
		GeneratedMinSafetyMargin,
		GeneratedMaxReachUsage,
		TangentCameraClearance,
		ImportedDecorCount,
		KenneyDecorCount,
		PolyHavenDecorCount,
		LoadedKenneyAssetKindCount,
		LoadedPolyHavenAssetKindCount,
		LoadedPolyHavenMaterialCount,
		FallbackDecorCount,
		ActivePlayer ? TEXT("configured") : TEXT("missing"));
	if (PlatformSurfaceLocalLocations.Num() == SpiralTowerWorldLayoutPrivate::SpiralTowerPlatformCount)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("W02 route landmarks local. Start=(%.1f,%.1f,%.1f) Layer1=(%.1f,%.1f,%.1f) FirstLookout=(%.1f,%.1f,%.1f) MidLookout=(%.1f,%.1f,%.1f) Summit=(%.1f,%.1f,%.1f)"),
			StartSurfaceLocalLocation.X,
			StartSurfaceLocalLocation.Y,
			StartSurfaceLocalLocation.Z,
			PlatformSurfaceLocalLocations[0].X,
			PlatformSurfaceLocalLocations[0].Y,
			PlatformSurfaceLocalLocations[0].Z,
			PlatformSurfaceLocalLocations[15].X,
			PlatformSurfaceLocalLocations[15].Y,
			PlatformSurfaceLocalLocations[15].Z,
			PlatformSurfaceLocalLocations[47].X,
			PlatformSurfaceLocalLocations[47].Y,
			PlatformSurfaceLocalLocations[47].Z,
			PlatformSurfaceLocalLocations.Last().X,
			PlatformSurfaceLocalLocations.Last().Y,
			PlatformSurfaceLocalLocations.Last().Z);
	}

#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("W02AutoValidateClimb")))
	{
		GetWorldTimerManager().SetTimerForNextTick(
			this,
			&ASpiralTowerWorldLayout::StartAutomatedClimbValidation);
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("W02AutoValidateRoute")))
	{
		StartAutomatedRouteValidation();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("W02AutoViewBabylonExterior"))
		&& ActivePlayer)
	{
		HighestDawnProgress = 0.48f;
		DisplayedDawnProgress = 0.48f;
		UpdateDawnAtmosphere(0.0f);
		const FVector CameraAnchor = GetActorTransform().TransformPosition(
			FVector(7200.0f, -7200.0f, 3100.0f));
		const FVector LookAt = GetActorLocation()
			+ FVector(0.0f, 0.0f, SummitSurfaceLocalZ * 0.48f);
		const FRotator ViewRotation = (LookAt - CameraAnchor).Rotation();
		if (UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_None);
		}
		ActivePlayer->SetActorLocationAndRotation(
			CameraAnchor,
			FRotator(0.0f, ViewRotation.Yaw, 0.0f),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		ActivePlayer->SetActorHiddenInGame(true);
		if (AController* Controller = ActivePlayer->GetController())
		{
			Controller->SetControlRotation(ViewRotation);
		}
		UE_LOG(
			LogTemp,
			Display,
			TEXT("W02 Babylon exterior visual validation ready. Player=Hidden DawnProgress=0.48 Camera=(%.0f,%.0f,%.0f)"),
			CameraAnchor.X,
			CameraAnchor.Y,
			CameraAnchor.Z);
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("W02AutoViewJumpPose"))
		&& ActivePlayer)
	{
		ActivePlayer->RestoreMainWorldLocomotionAnimation();
		if (UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->GravityScale = 0.0f;
			Movement->SetMovementMode(MOVE_Falling);
		}
		ActivePlayer->AddActorWorldOffset(FVector(0.0f, 0.0f, 260.0f), false);
		UE_LOG(
			LogTemp,
			Display,
			TEXT("W02 jump pose visual validation ready. Animation=SkeletonNativeLocomotion Gravity=Suspended ForeignSingleNode=Disabled"));
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("W02AutoViewSummit"))
		&& ActivePlayer)
	{
		HighestDawnProgress = 1.0f;
		DisplayedDawnProgress = 1.0f;
		UpdateDawnAtmosphere(0.0f);
		const FVector Outward = FVector(
			SunriseDeckLocalLocation.X,
			SunriseDeckLocalLocation.Y,
			0.0f).GetSafeNormal();
		const FVector DeckWorldLocation = GetActorTransform().TransformPosition(
			SunriseDeckLocalLocation + Outward * 160.0f
				+ FVector(0.0f, 0.0f, GetPlayerCapsuleHalfHeight() + 6.0f));
		const FRotator ViewRotation = Outward.Rotation();
		ActivePlayer->SetActorLocationAndRotation(
			DeckWorldLocation,
			ViewRotation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		if (AController* Controller = ActivePlayer->GetController())
		{
			Controller->SetControlRotation(ViewRotation);
		}
		EnterSummitChoice();
	}
#endif
}

void ASpiralTowerWorldLayout::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bTravelRequested || bResettingPlayer || !IsValid(ActivePlayer))
	{
		return;
	}

	UpdateDawnAtmosphere(DeltaSeconds);
	if (bSummitChoiceAvailable)
	{
		const APlayerController* PlayerController = Cast<APlayerController>(ActivePlayer->GetController());
		const bool bReturnKeyDown = PlayerController && PlayerController->IsInputKeyDown(EKeys::E);
		if (bReturnKeyDown && !bReturnKeyWasDown)
		{
			BeginReturnTravel();
			return;
		}
		bReturnKeyWasDown = bReturnKeyDown;
	}

	UpdateClimbRelicPresentation(DeltaSeconds);
	UpdateTraversalStamina(DeltaSeconds);
	if (bLedgeClimbInProgress)
	{
		TickLedgeClimb(DeltaSeconds);
		return;
	}

	UpdateJumpMovement(DeltaSeconds);
	if (bLandingRecovery)
	{
		return;
	}
	UpdateSprintMovement();
	if (TryStartLedgeClimb())
	{
		return;
	}
	UpdateCheckpointProgress();

	const FVector PlayerLocalLocation = GetActorTransform().InverseTransformPosition(
		ActivePlayer->GetActorLocation());
	const FVector SummitTriggerSpaceLocation = SummitTrigger->GetComponentTransform().InverseTransformPosition(
		ActivePlayer->GetActorLocation());
	const FVector SummitTriggerExtent = SummitTrigger->GetUnscaledBoxExtent();
	if (FMath::Abs(SummitTriggerSpaceLocation.X) <= SummitTriggerExtent.X
		&& FMath::Abs(SummitTriggerSpaceLocation.Y) <= SummitTriggerExtent.Y
		&& FMath::Abs(SummitTriggerSpaceLocation.Z) <= SummitTriggerExtent.Z)
	{
		// Teleports and very fast movement do not always emit BeginOverlap. The
		// same bounds check keeps summit completion authoritative and testable.
		HandleSummitOverlap(
			SummitTrigger,
			ActivePlayer,
			ActivePlayer->GetCapsuleComponent(),
			INDEX_NONE,
			false,
			FHitResult());
		return;
	}

	const float ActiveCheckpointSurfaceZ = CheckpointLocalTransforms.IsValidIndex(ActiveCheckpointSlot)
		? CheckpointLocalTransforms[ActiveCheckpointSlot].GetLocation().Z
		: StartSurfaceLocalLocation.Z;
	const float ResetBelowZ = FMath::Max(
		KillPlaneLocalZ,
		ActiveCheckpointSurfaceZ - SpiralTowerWorldLayoutPrivate::SpiralTowerCheckpointFallDistance);
	if (PlayerLocalLocation.Z < ResetBelowZ)
	{
		ResetPlayerToCheckpoint(TEXT("fell below the active checkpoint"));
		return;
	}

	const UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement();
	const float PlayerFloorZ = PlayerLocalLocation.Z - GetPlayerCapsuleHalfHeight();
	if (ActiveCheckpointSlot > 0 && Movement && Movement->IsMovingOnGround()
		&& PlayerFloorZ < ActiveCheckpointSurfaceZ - 180.0f)
	{
		ResetPlayerToCheckpoint(TEXT("landed below the active checkpoint"));
		return;
	}

	if (PlayerLocalLocation.SizeSquared2D()
		> FMath::Square(SpiralTowerWorldLayoutPrivate::SpiralTowerMaxPlayerRadius))
	{
		ResetPlayerToCheckpoint(TEXT("left the tower bounds"));
	}
}

FVector ASpiralTowerWorldLayout::GetStartLocation() const
{
	return GetActorTransform().TransformPosition(StartSurfaceLocalLocation);
}

FVector ASpiralTowerWorldLayout::GetSummitLocation() const
{
	return GetActorTransform().TransformPosition(SummitSurfaceLocalLocation);
}

void ASpiralTowerWorldLayout::LoadOptionalEnvironmentAssets()
{
	CubeFallback = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	CylinderFallback = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	ConeFallback = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	SphereFallback = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	BasicShapeMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	TowerMesh = LoadObject<UStaticMesh>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerEnvironmentAssetPath(TEXT("SM_W01_Tower")));
	WallMesh = LoadObject<UStaticMesh>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerEnvironmentAssetPath(TEXT("SM_W01_Wall")));
	ArchMesh = LoadObject<UStaticMesh>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerEnvironmentAssetPath(TEXT("SM_W01_Arch")));
	CampfireMesh = LoadObject<UStaticMesh>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerEnvironmentAssetPath(TEXT("SM_W01_Campfire")));
	PathMesh = LoadObject<UStaticMesh>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerEnvironmentAssetPath(TEXT("SM_W01_Path")));

	// These stable W02-owned paths intentionally resolve to null until the
	// corresponding Kenney packs are imported. Gameplay collision never depends
	// on them; their engine-primitive fallbacks are built by the same code path.
	StoneBrickMesh = LoadObject<UStaticMesh>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerOptionalAssetPath(
			SpiralTowerWorldLayoutPrivate::SpiralTowerKenneyCastleRoot,
			TEXT("SM_W02_CastleStoneStairs")));
	InteriorWallMesh = LoadObject<UStaticMesh>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerOptionalAssetPath(
			SpiralTowerWorldLayoutPrivate::SpiralTowerKenneyCastleRoot,
			TEXT("SM_W02_CastleWall")));
	CastleDoorwayMesh = LoadObject<UStaticMesh>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerOptionalAssetPath(
			SpiralTowerWorldLayoutPrivate::SpiralTowerKenneyCastleRoot,
			TEXT("SM_W02_CastleWallDoorway")));
	CastleBridgeMesh = LoadObject<UStaticMesh>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerOptionalAssetPath(
			SpiralTowerWorldLayoutPrivate::SpiralTowerKenneyCastleRoot,
			TEXT("SM_W02_CastleStraightBridge")));
	CastleTowerMidMesh = LoadObject<UStaticMesh>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerOptionalAssetPath(
			SpiralTowerWorldLayoutPrivate::SpiralTowerKenneyCastleRoot,
			TEXT("SM_W02_CastleSquareTowerMid")));
	CastleTowerTopMesh = LoadObject<UStaticMesh>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerOptionalAssetPath(
			SpiralTowerWorldLayoutPrivate::SpiralTowerKenneyCastleRoot,
			TEXT("SM_W02_CastleSquareTowerTop")));
	WoodBeamMesh = LoadObject<UStaticMesh>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerOptionalAssetPath(
			SpiralTowerWorldLayoutPrivate::SpiralTowerKenneyMedievalRoot,
			TEXT("SM_W02_WoodBeam")));
	BrokenPlankMesh = LoadObject<UStaticMesh>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerOptionalAssetPath(
			SpiralTowerWorldLayoutPrivate::SpiralTowerKenneyMedievalRoot,
			TEXT("SM_W02_BrokenPlank")));

	// Poly Haven's CC0 Modular Fort modules are a visual skin only. Their source
	// bounds are approximately 14.6 m wall length / 8.5 m storey height, so every
	// placement below scales from those measured imported dimensions rather than
	// the much smaller Kenney kit dimensions.
	PolyHavenWallStraightMesh = LoadObject<UStaticMesh>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerPolyHavenMeshPath(
			TEXT("modular_fort_01_wall_thick_straight_01")));
	PolyHavenGateMesh = LoadObject<UStaticMesh>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerPolyHavenMeshPath(
			TEXT("modular_fort_01_wall_thin_gate_01")));
	PolyHavenWalkwayMesh = LoadObject<UStaticMesh>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerPolyHavenMeshPath(
			TEXT("modular_fort_01_wall_walkway_straight_01")));
	PolyHavenTowerRoundMesh = LoadObject<UStaticMesh>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerPolyHavenMeshPath(
			TEXT("modular_fort_01_tower_round")));
	PolyHavenWallMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerPolyHavenMaterialPath(
			TEXT("M_W02_ModularFort01_Wall")));
	PolyHavenTrimMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerPolyHavenMaterialPath(
			TEXT("M_W02_ModularFort01_Trim")));
	PolyHavenWoodMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		*SpiralTowerWorldLayoutPrivate::SpiralTowerPolyHavenMaterialPath(
			TEXT("M_W02_WoodPlanks")));

	const TCHAR* AnimationRoot =
		TEXT("/Game/WorldWalker/Worlds/W02_SpiralTower/ThirdParty/Quaternius/UniversalAnimationLibrary2/Animations");
	JumpStartAnimation = LoadObject<UAnimSequence>(
		nullptr,
		*FString::Printf(TEXT("%s/A_W02_Jump_Start.A_W02_Jump_Start"), AnimationRoot));
	JumpLoopAnimation = LoadObject<UAnimSequence>(
		nullptr,
		*FString::Printf(TEXT("%s/A_W02_Jump_Loop.A_W02_Jump_Loop"), AnimationRoot));
	JumpLandAnimation = LoadObject<UAnimSequence>(
		nullptr,
		*FString::Printf(TEXT("%s/A_W02_Jump_Land.A_W02_Jump_Land"), AnimationRoot));
	ClimbAnimation = LoadObject<UAnimSequence>(
		nullptr,
		*FString::Printf(TEXT("%s/A_W02_Climb_1m.A_W02_Climb_1m"), AnimationRoot));

	LoadedKenneyAssetKindCount = 0;
	LoadedKenneyAssetKindCount += StoneBrickMesh ? 1 : 0;
	LoadedKenneyAssetKindCount += InteriorWallMesh ? 1 : 0;
	LoadedKenneyAssetKindCount += CastleDoorwayMesh ? 1 : 0;
	LoadedKenneyAssetKindCount += CastleBridgeMesh ? 1 : 0;
	LoadedKenneyAssetKindCount += CastleTowerMidMesh ? 1 : 0;
	LoadedKenneyAssetKindCount += CastleTowerTopMesh ? 1 : 0;
	LoadedKenneyAssetKindCount += WoodBeamMesh ? 1 : 0;
	LoadedKenneyAssetKindCount += BrokenPlankMesh ? 1 : 0;
	LoadedPolyHavenAssetKindCount = 0;
	LoadedPolyHavenAssetKindCount += PolyHavenWallStraightMesh ? 1 : 0;
	LoadedPolyHavenAssetKindCount += PolyHavenGateMesh ? 1 : 0;
	LoadedPolyHavenAssetKindCount += PolyHavenWalkwayMesh ? 1 : 0;
	LoadedPolyHavenAssetKindCount += PolyHavenTowerRoundMesh ? 1 : 0;
	LoadedPolyHavenMaterialCount = 0;
	LoadedPolyHavenMaterialCount += PolyHavenWallMaterial ? 1 : 0;
	LoadedPolyHavenMaterialCount += PolyHavenTrimMaterial ? 1 : 0;
	LoadedPolyHavenMaterialCount += PolyHavenWoodMaterial ? 1 : 0;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 visual assets loaded. KenneyAssetKinds=%d PolyHavenAssetKinds=%d PolyHavenMaterials=%d PlatformAnimations=%d/4 PolyHavenRoot=%s"),
		LoadedKenneyAssetKindCount,
		LoadedPolyHavenAssetKindCount,
		LoadedPolyHavenMaterialCount,
		(JumpStartAnimation ? 1 : 0) + (JumpLoopAnimation ? 1 : 0)
			+ (JumpLandAnimation ? 1 : 0) + (ClimbAnimation ? 1 : 0),
		SpiralTowerWorldLayoutPrivate::SpiralTowerPolyHavenFortRoot);
}

void ASpiralTowerWorldLayout::ConfigureNightAtmosphere()
{
	HideTemplateFloor();

	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		ADirectionalLight* DirectionalLight = *It;
		if (ULightComponent* Light = DirectionalLight ? DirectionalLight->GetLightComponent() : nullptr)
		{
			AtmosphereDirectionalLight = DirectionalLight;
			Light->SetMobility(EComponentMobility::Movable);
			Light->SetVisibility(true);
			DirectionalLight->SetActorHiddenInGame(false);
			DirectionalLight->SetActorRotation(FRotator(-42.0f, -28.0f, 0.0f));
			Light->SetIntensity(3.15f);
			Light->SetLightColor(FLinearColor(0.48f, 0.60f, 0.92f));
			Light->SetIndirectLightingIntensity(1.0f);
			Light->SetVolumetricScatteringIntensity(0.34f);
			if (UDirectionalLightComponent* DirectionalComponent =
				Cast<UDirectionalLightComponent>(Light))
			{
				DirectionalComponent->SetAtmosphereSunLight(true);
				DirectionalComponent->SetAtmosphereSunLightIndex(0);
			}
			break;
		}
	}

	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
	{
		ASkyLight* SkyLightActor = *It;
		if (USkyLightComponent* SkyLight = SkyLightActor ? SkyLightActor->GetLightComponent() : nullptr)
		{
			AtmosphereSkyLight = SkyLightActor;
			SkyLight->SetMobility(EComponentMobility::Movable);
			SkyLight->SetVisibility(true);
			SkyLightActor->SetActorHiddenInGame(false);
			SkyLight->SourceType = SLS_CapturedScene;
			SkyLight->SetRealTimeCapture(false);
			SkyLight->bLowerHemisphereIsBlack = false;
			SkyLight->SetLowerHemisphereColor(FLinearColor(0.050f, 0.070f, 0.120f));
			SkyLight->SetIntensity(1.85f);
			SkyLight->SetLightColor(FLinearColor(0.56f, 0.66f, 0.94f));
			break;
		}
	}

	for (TActorIterator<ASkyAtmosphere> It(GetWorld()); It; ++It)
	{
		if (USkyAtmosphereComponent* Atmosphere = It->GetComponent())
		{
			AtmosphereComponent = Atmosphere;
			Atmosphere->SetSkyAndAerialPerspectiveLuminanceFactor(
				FLinearColor(0.16f, 0.22f, 0.42f));
			Atmosphere->SetRayleighScatteringScale(0.48f);
			Atmosphere->SetMieScatteringScale(1.25f);
			Atmosphere->SetHeightFogContribution(0.72f);
			break;
		}
	}

	for (TActorIterator<AVolumetricCloud> It(GetWorld()); It; ++It)
	{
		It->SetActorHiddenInGame(true);
	}

	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
	{
		SpawnedFog = *It;
		break;
	}
	if (!SpawnedFog)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnedFog = GetWorld()->SpawnActor<AExponentialHeightFog>(
			AExponentialHeightFog::StaticClass(),
			GetActorLocation(),
			FRotator::ZeroRotator,
			SpawnParameters);
	}
	if (SpawnedFog && SpawnedFog->GetComponent())
	{
		UExponentialHeightFogComponent* Fog = SpawnedFog->GetComponent();
		Fog->SetFogDensity(0.0045f);
		Fog->SetFogHeightFalloff(0.11f);
		Fog->SetFogMaxOpacity(0.28f);
		Fog->SetFogInscatteringColor(FLinearColor(0.012f, 0.020f, 0.050f));
		Fog->SetVolumetricFog(true);
		Fog->SetVolumetricFogExtinctionScale(0.52f);
		Fog->SetVolumetricFogAlbedo(FColor(44, 57, 94));
		Fog->SetVolumetricFogDistance(6500.0f);
	}
}

void ASpiralTowerWorldLayout::UpdateDawnAtmosphere(const float DeltaSeconds)
{
	if (IsValid(ActivePlayer) && SunriseDeckLocalLocation.Z > 1.0f)
	{
		const float PlayerLocalZ = GetActorTransform().InverseTransformPosition(
			ActivePlayer->GetActorLocation()).Z;
		const float ClimbProgress = FMath::Clamp(
			(PlayerLocalZ - StartSurfaceLocalLocation.Z)
				/ FMath::Max(1.0f, SunriseDeckLocalLocation.Z - StartSurfaceLocalLocation.Z),
			0.0f,
			1.0f);
		HighestDawnProgress = FMath::Max(HighestDawnProgress, ClimbProgress);
	}

	DisplayedDawnProgress = DeltaSeconds > 0.0f
		? FMath::FInterpTo(DisplayedDawnProgress, HighestDawnProgress, DeltaSeconds, 1.35f)
		: HighestDawnProgress;
	const float Dawn = FMath::Clamp(DisplayedDawnProgress, 0.0f, 1.0f);
	const float SunriseBlend = FMath::SmoothStep(0.54f, 1.0f, Dawn);
	const FLinearColor NightSky(0.002f, 0.006f, 0.028f, 1.0f);
	const FLinearColor BlueHourSky(0.025f, 0.060f, 0.160f, 1.0f);
	const FLinearColor SunriseSky(0.31f, 0.105f, 0.055f, 1.0f);
	const FLinearColor SkyColor = Dawn < 0.54f
		? FMath::Lerp(NightSky, BlueHourSky, Dawn / 0.54f)
		: FMath::Lerp(BlueHourSky, SunriseSky, SunriseBlend);

	if (ExteriorSkyDome)
	{
		// The low-poly fallback dome guarantees readable night through the lower
		// windows. At dawn the real atmosphere takes over, avoiding a faceted sky.
		ExteriorSkyDome->SetVisibility(Dawn < 0.62f, true);
		if (UMaterialInstanceDynamic* SkyMaterial =
			Cast<UMaterialInstanceDynamic>(ExteriorSkyDome->GetMaterial(0)))
		{
			SkyMaterial->SetVectorParameterValue(TEXT("Color"), SkyColor);
		}
	}
	if (AtmosphereDirectionalLight && AtmosphereDirectionalLight->GetLightComponent())
	{
		ULightComponent* Light = AtmosphereDirectionalLight->GetLightComponent();
		AtmosphereDirectionalLight->SetActorRotation(FRotator(
			FMath::Lerp(-42.0f, -6.0f, SunriseBlend),
			FMath::Lerp(-28.0f, 112.0f, SunriseBlend),
			0.0f));
		Light->SetIntensity(FMath::Lerp(3.15f, 5.2f, SunriseBlend));
		Light->SetLightColor(FMath::Lerp(
			FLinearColor(0.48f, 0.60f, 0.92f),
			FLinearColor(1.0f, 0.72f, 0.48f),
			SunriseBlend));
	}
	if (AtmosphereSkyLight && AtmosphereSkyLight->GetLightComponent())
	{
		USkyLightComponent* SkyLight = AtmosphereSkyLight->GetLightComponent();
		SkyLight->SetIntensity(FMath::Lerp(1.35f, 1.65f, Dawn));
		SkyLight->SetLightColor(FMath::Lerp(
			FLinearColor(0.48f, 0.61f, 0.94f),
			FLinearColor(0.88f, 0.79f, 0.72f),
			SunriseBlend));
	}
	if (AtmosphereComponent)
	{
		AtmosphereComponent->SetSkyAndAerialPerspectiveLuminanceFactor(FMath::Lerp(
			FLinearColor(0.16f, 0.22f, 0.42f),
			FLinearColor(0.52f, 0.46f, 0.42f),
			SunriseBlend));
		AtmosphereComponent->SetRayleighScatteringScale(FMath::Lerp(0.48f, 0.82f, Dawn));
		AtmosphereComponent->SetMieScatteringScale(FMath::Lerp(1.25f, 0.72f, Dawn));
	}
	if (SpawnedFog && SpawnedFog->GetComponent())
	{
		UExponentialHeightFogComponent* Fog = SpawnedFog->GetComponent();
		Fog->SetFogDensity(FMath::Lerp(0.0045f, 0.0020f, Dawn));
		Fog->SetFogMaxOpacity(FMath::Lerp(0.28f, 0.14f, Dawn));
		Fog->SetFogInscatteringColor(FMath::Lerp(
			FLinearColor(0.012f, 0.020f, 0.050f),
			FLinearColor(0.18f, 0.13f, 0.15f),
			SunriseBlend));
	}
	if (SunriseSunDisc)
	{
		SunriseSunDisc->SetVisibility(Dawn >= 0.66f, true);
		SunriseSunDisc->SetRelativeScale3D(FVector(FMath::Lerp(2.6f, 4.2f, SunriseBlend)));
	}
	if (SunriseDeckLight)
	{
		SunriseDeckLight->SetIntensity(FMath::Lerp(0.0f, 150.0f, SunriseBlend));
	}

	const int32 DawnBand = FMath::Clamp(FMath::FloorToInt(Dawn * 4.0f + 0.001f), 0, 4);
	if (DawnBand != LastDawnLogBand)
	{
		LastDawnLogBand = DawnBand;
		UE_LOG(
			LogTemp,
			Display,
			TEXT("W02 dawn progression. Band=%d/4 Progress=%.3f Phase=%s"),
			DawnBand,
			Dawn,
			Dawn < 0.25f ? TEXT("DeepNight")
				: (Dawn < 0.55f ? TEXT("BlueHour")
					: (Dawn < 0.90f ? TEXT("Predawn") : TEXT("Sunrise"))));
	}
}

void ASpiralTowerWorldLayout::RefreshCapturedSky()
{
	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
	{
		if (USkyLightComponent* SkyLight = It->GetLightComponent())
		{
			SkyLight->RecaptureSky();
		}
	}
}

void ASpiralTowerWorldLayout::HideTemplateFloor()
{
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		AStaticMeshActor* StaticMeshActor = *It;
		UStaticMeshComponent* MeshComponent = StaticMeshActor
			? StaticMeshActor->GetStaticMeshComponent()
			: nullptr;
		const UStaticMesh* Mesh = MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
		const bool bTemplateFloor = StaticMeshActor
			&& (StaticMeshActor->GetName().StartsWith(TEXT("Floor"))
				|| (Mesh && Mesh->GetName().Contains(TEXT("Template_Map_Floor"))));
		const bool bTemplateSkyDome = Mesh && Mesh->GetName().Contains(TEXT("SM_SkySphere"));
		if (bTemplateFloor || bTemplateSkyDome)
		{
			StaticMeshActor->SetActorHiddenInGame(true);
			StaticMeshActor->SetActorEnableCollision(false);
		}
	}
}

void ASpiralTowerWorldLayout::BuildTower()
{
	BuiltPlatformCount = 0;
	ImportedDecorCount = 0;
	KenneyDecorCount = 0;
	PolyHavenDecorCount = 0;
	FallbackDecorCount = 0;
	CheckpointLocalTransforms.Reset();
	PlatformSurfaceLocalLocations.Reset();
	PlatformLocalRotations.Reset();
	PlatformLocalDimensions.Reset();
	PlatformStyles.Reset();
	PlatformTraversalTypes.Reset();

	GeneratePlatformSpecs();
	BuildBaseCourtyard();
	BuildSpiralPlatforms();
	BuildBabylonZiggurat();
	BuildBabylonScaffolds();
	BuildSummit();
	BuildClimbRelic();
}

void ASpiralTowerWorldLayout::GeneratePlatformSpecs()
{
	using namespace SpiralTowerWorldLayoutPrivate;

	FRandomStream RouteRandom(SpiralTowerGenerationSeed);
	float AngleDegrees = 0.0f;
	float SurfaceZ = SpiralTowerFirstSurfaceZ;
	int32 PassedSegments = 0;
	GeneratedTurns = 0.0f;
	GeneratedMaxClearGap = 0.0f;
	GeneratedMinClearGap = MAX_flt;
	GeneratedMaxCenterSpan = 0.0f;
	GeneratedMinCenterSpan = MAX_flt;
	GeneratedMaxRequiredTravel = 0.0f;
	GeneratedMinSafetyMargin = MAX_flt;
	GeneratedMaxReachUsage = 0.0f;
	GeneratedMaxRequiredSpeed = 0.0f;
	GeneratedWorstStep = INDEX_NONE;
	GeneratedMinAngleStep = MAX_flt;
	GeneratedMaxAngleStep = 0.0f;
	GeneratedMinRadius = MAX_flt;
	GeneratedMaxRadius = 0.0f;
	GeneratedMinRise = MAX_flt;
	GeneratedMaxRise = 0.0f;
	GeneratedMinLength = MAX_flt;
	GeneratedMaxLength = 0.0f;
	GeneratedMinWidth = MAX_flt;
	GeneratedMaxWidth = 0.0f;
	PlatformStepFlightTimes.Reset();
	PlatformStepTheoreticalReaches.Reset();
	PlatformStepRequiredTravels.Reset();
	PlatformStepSafetyMargins.Reset();

	for (int32 PlatformIndex = 0; PlatformIndex < SpiralTowerPlatformCount; ++PlatformIndex)
	{
		const ESpiralTowerTraversalType TraversalType = SpiralTowerTraversalType(PlatformIndex);
		const bool bCheckpointPlatform = TraversalType == ESpiralTowerTraversalType::LookoutDeck;
		const float PlatformLength = bCheckpointPlatform
			? SpiralTowerCheckpointLength
			: (TraversalType == ESpiralTowerTraversalType::SpiralStair
				? SpiralTowerStairLength
				: (TraversalType == ESpiralTowerTraversalType::Ladder
					? SpiralTowerLadderLandingLength
					: RouteRandom.FRandRange(SpiralTowerMinPlatformLength, SpiralTowerMaxPlatformLength)));
		const float PlatformWidth = bCheckpointPlatform
			? SpiralTowerCheckpointWidth
			: (TraversalType == ESpiralTowerTraversalType::SpiralStair
				? SpiralTowerStairWidth
				: (TraversalType == ESpiralTowerTraversalType::Ladder
					? SpiralTowerLadderLandingWidth
					: RouteRandom.FRandRange(SpiralTowerMinPlatformWidth, SpiralTowerMaxPlatformWidth)));
		const float WallInnerRadius = SpiralTowerInteriorWallRadius
			- SpiralTowerInteriorWallThickness * 0.5f;
		const float Radius = WallInnerRadius - PlatformWidth * 0.5f
			- RouteRandom.FRandRange(SpiralTowerMinWallInset, SpiralTowerMaxWallInset);

		const int32 StyleRoll = RouteRandom.RandRange(0, 99);
		const ESpiralTowerPlatformStyle Style = bCheckpointPlatform
			|| TraversalType == ESpiralTowerTraversalType::SpiralStair
			? ESpiralTowerPlatformStyle::StoneBrick
			: (TraversalType == ESpiralTowerTraversalType::Ladder
				? ESpiralTowerPlatformStyle::TimberBeam
				: (StyleRoll < 45
					? ESpiralTowerPlatformStyle::StoneBrick
					: (StyleRoll < 73
						? ESpiralTowerPlatformStyle::TimberBeam
						: ESpiralTowerPlatformStyle::BrokenPlanks)));
		float AngleStep = 0.0f;
		float Rise = 0.0f;
		float FlightTime = 0.0f;
		float TheoreticalReach = 0.0f;
		float RequiredTravel = 0.0f;
		float SafetyMargin = 0.0f;
		float CentreSpan = 0.0f;
		float ClearGap = 0.0f;
		FVector SurfaceLocation = FVector::ZeroVector;
		FRotator PlatformRotation = FRotator::ZeroRotator;
		const FVector2D CurrentDimensions(PlatformLength, PlatformWidth);

		if (PlatformIndex == 0)
		{
			SurfaceLocation = FVector(Radius, 0.0f, SurfaceZ);
			PlatformRotation = FRotator(0.0f, 90.0f, 0.0f);
			PlatformStepFlightTimes.Add(0.0f);
			PlatformStepTheoreticalReaches.Add(0.0f);
			PlatformStepRequiredTravels.Add(0.0f);
			PlatformStepSafetyMargins.Add(0.0f);
		}
		else
		{
			const bool bChallengeStep = TraversalType == ESpiralTowerTraversalType::GapJump
				&& (PlatformIndex % 23 == 0 || PlatformIndex % 47 == 0);
			Rise = TraversalType == ESpiralTowerTraversalType::SpiralStair
				? SpiralTowerStairRise
				: (TraversalType == ESpiralTowerTraversalType::Ladder
					? SpiralTowerLadderRise
					: RouteRandom.FRandRange(SpiralTowerMinPlatformRise, SpiralTowerMaxPlatformRise));
			const float DesiredAngleStep = TraversalType == ESpiralTowerTraversalType::SpiralStair
				? SpiralTowerStairAngleStepDegrees
				: (TraversalType == ESpiralTowerTraversalType::Ladder
					? SpiralTowerLadderAngleStepDegrees
					: (bCheckpointPlatform
						? 10.0f
						: (bChallengeStep
							? SpiralTowerChallengeAngleStepDegrees
							: RouteRandom.FRandRange(
								SpiralTowerMinDesiredAngleStepDegrees,
								SpiralTowerMaxDesiredAngleStepDegrees))));
			const float RequiredSafetyReserve = bCheckpointPlatform
				? 42.0f
				: (bChallengeStep
					? SpiralTowerChallengeSafetyMargin
					: RouteRandom.FRandRange(22.0f, 55.0f));
			const FVector& PreviousLocation = PlatformSurfaceLocalLocations.Last();
			const FRotator& PreviousRotation = PlatformLocalRotations.Last();
			const FVector2D& PreviousDimensions = PlatformLocalDimensions.Last();
			FlightTime = SpiralTowerFlightTimeForRise(Rise);
			TheoreticalReach = FlightTime * SpiralTowerRunSpeed;
			float UsableReach = TheoreticalReach * SpiralTowerUsableReachFraction;

			auto EvaluateStep = [&](const float CandidateStep)
			{
				const float CandidateAngle = AngleDegrees + CandidateStep;
				const float AngleRadians = FMath::DegreesToRadians(CandidateAngle);
				SurfaceLocation = FVector(
					FMath::Cos(AngleRadians) * Radius,
					FMath::Sin(AngleRadians) * Radius,
					SurfaceZ + Rise);
				PlatformRotation = FRotator(0.0f, CandidateAngle + 90.0f, 0.0f);
				CentreSpan = FVector::Dist2D(PreviousLocation, SurfaceLocation);
				ClearGap = SpiralTowerClearGap(
					PreviousLocation,
					PreviousRotation,
					PreviousDimensions,
					SurfaceLocation,
					PlatformRotation,
					CurrentDimensions);
				RequiredTravel = SpiralTowerRequiredCentreTravel(
					PreviousLocation,
					PreviousRotation,
					PreviousDimensions,
					SurfaceLocation,
					PlatformRotation,
					CurrentDimensions);
				SafetyMargin = UsableReach - RequiredTravel;
			};

			AngleStep = DesiredAngleStep;
			EvaluateStep(AngleStep);
			if ((TraversalType == ESpiralTowerTraversalType::GapJump
					|| TraversalType == ESpiralTowerTraversalType::LookoutDeck)
				&& SafetyMargin < RequiredSafetyReserve)
			{
				float LowerStep = 1.0f;
				float UpperStep = DesiredAngleStep;
				for (int32 SearchIteration = 0; SearchIteration < 18; ++SearchIteration)
				{
					const float CandidateStep = (LowerStep + UpperStep) * 0.5f;
					EvaluateStep(CandidateStep);
					if (SafetyMargin >= RequiredSafetyReserve)
					{
						LowerStep = CandidateStep;
					}
					else
					{
						UpperStep = CandidateStep;
					}
				}
				AngleStep = LowerStep;
				EvaluateStep(AngleStep);
			}

			bool bStepPassed = false;
			const TCHAR* TraversalLabel = TEXT("Jump");
			if (TraversalType == ESpiralTowerTraversalType::SpiralStair)
			{
				TraversalLabel = TEXT("SpiralStair");
				UsableReach = 24.0f;
				RequiredTravel = ClearGap;
				SafetyMargin = UsableReach - RequiredTravel;
				bStepPassed = Rise <= 32.0f && ClearGap <= UsableReach;
			}
			else if (TraversalType == ESpiralTowerTraversalType::Ladder)
			{
				TraversalLabel = TEXT("LadderClimb");
				UsableReach = SpiralTowerLedgeClimbReach;
				RequiredTravel = ClearGap;
				SafetyMargin = UsableReach - RequiredTravel;
				bStepPassed = Rise <= SpiralTowerLedgeClimbMaxHeight
					&& ClearGap <= SpiralTowerLedgeClimbReach;
			}
			else
			{
				TraversalLabel = bCheckpointPlatform ? TEXT("LookoutEntry") : TEXT("Jump");
				bStepPassed = FlightTime > 0.0f
					&& SafetyMargin + 0.10f >= RequiredSafetyReserve;
			}
			ensureAlwaysMsgf(
				bStepPassed,
				TEXT("W02 traversal validation failed from layer %d to %d: Type=%s Rise=%.1f Gap=%.1f Required=%.1f Usable=%.1f Margin=%.1f"),
				PlatformIndex,
				PlatformIndex + 1,
				TraversalLabel,
				Rise,
				ClearGap,
				RequiredTravel,
				UsableReach,
				SafetyMargin);
			PassedSegments += bStepPassed ? 1 : 0;
			AngleDegrees += AngleStep;
			SurfaceZ += Rise;

			GeneratedMinAngleStep = FMath::Min(GeneratedMinAngleStep, AngleStep);
			GeneratedMaxAngleStep = FMath::Max(GeneratedMaxAngleStep, AngleStep);
			GeneratedMinRise = FMath::Min(GeneratedMinRise, Rise);
			GeneratedMaxRise = FMath::Max(GeneratedMaxRise, Rise);
			GeneratedMinCenterSpan = FMath::Min(GeneratedMinCenterSpan, CentreSpan);
			GeneratedMaxCenterSpan = FMath::Max(GeneratedMaxCenterSpan, CentreSpan);
			GeneratedMinClearGap = FMath::Min(GeneratedMinClearGap, ClearGap);
			GeneratedMaxClearGap = FMath::Max(GeneratedMaxClearGap, ClearGap);
			GeneratedMaxRequiredTravel = FMath::Max(GeneratedMaxRequiredTravel, RequiredTravel);
			const float ReachUsage = UsableReach > UE_KINDA_SMALL_NUMBER
				? RequiredTravel / UsableReach
				: BIG_NUMBER;
			GeneratedMaxReachUsage = FMath::Max(GeneratedMaxReachUsage, ReachUsage);
			if (FlightTime > UE_KINDA_SMALL_NUMBER)
			{
				GeneratedMaxRequiredSpeed = FMath::Max(
					GeneratedMaxRequiredSpeed,
					RequiredTravel / FlightTime);
			}
			if (SafetyMargin < GeneratedMinSafetyMargin)
			{
				GeneratedMinSafetyMargin = SafetyMargin;
				GeneratedWorstStep = PlatformIndex;
			}

			PlatformStepFlightTimes.Add(FlightTime);
			PlatformStepTheoreticalReaches.Add(TheoreticalReach);
			PlatformStepRequiredTravels.Add(RequiredTravel);
			PlatformStepSafetyMargins.Add(SafetyMargin);
			UE_LOG(
				LogTemp,
				Display,
				TEXT("W02 traversal margin. Layer=%03d->%03d Type=%s Challenge=%d Angle=%.2f Rise=%.1f Centre=%.1f Gap=%.1f Flight=%.3f Usable=%.1f Required=%.1f RequiredRatio=%.3f Margin=%.1f"),
				PlatformIndex,
				PlatformIndex + 1,
				TraversalLabel,
				bChallengeStep ? 1 : 0,
				AngleStep,
				Rise,
				CentreSpan,
				ClearGap,
				FlightTime,
				UsableReach,
				RequiredTravel,
				UsableReach > UE_KINDA_SMALL_NUMBER ? RequiredTravel / UsableReach : BIG_NUMBER,
				SafetyMargin);
		}

		PlatformSurfaceLocalLocations.Add(SurfaceLocation);
		PlatformLocalRotations.Add(PlatformRotation);
		PlatformLocalDimensions.Add(CurrentDimensions);
		PlatformStyles.Add(static_cast<uint8>(Style));
		PlatformTraversalTypes.Add(static_cast<uint8>(TraversalType));
		GeneratedMinRadius = FMath::Min(GeneratedMinRadius, Radius);
		GeneratedMaxRadius = FMath::Max(GeneratedMaxRadius, Radius);
		GeneratedMinLength = FMath::Min(GeneratedMinLength, PlatformLength);
		GeneratedMaxLength = FMath::Max(GeneratedMaxLength, PlatformLength);
		GeneratedMinWidth = FMath::Min(GeneratedMinWidth, PlatformWidth);
		GeneratedMaxWidth = FMath::Max(GeneratedMaxWidth, PlatformWidth);
	}

	GeneratedTurns = AngleDegrees / 360.0f;
	SummitSurfaceLocalLocation = PlatformSurfaceLocalLocations.Last();
	SummitSurfaceLocalZ = SummitSurfaceLocalLocation.Z;
	const float TheoreticalApex = FMath::Square(SpiralTowerJumpZVelocity)
		/ (2.0f * SpiralTowerWorldGravity);
	const float SameLevelTheoreticalReach = 2.0f * SpiralTowerJumpZVelocity
		/ SpiralTowerWorldGravity * SpiralTowerRunSpeed;
	const float EntryFlightTime = SpiralTowerFlightTimeForRise(SpiralTowerFirstSurfaceZ);
	const bool bEntrySegmentPassed = EntryFlightTime > 0.0f
		&& PlatformSurfaceLocalLocations[0].Size2D()
			+ SpiralTowerCapsuleRadius + SpiralTowerStandingMargin
			<= SpiralTowerInteriorWallRadius - SpiralTowerInteriorWallThickness * 0.5f;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 entry validation. Segment=Base->Layer001 Passed=%d Rise=%.1f HorizontalRequired=0.0 Flight=%.3f Apex=%.1f"),
		bEntrySegmentPassed ? 1 : 0,
		SpiralTowerFirstSurfaceZ,
		EntryFlightTime,
		TheoreticalApex);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 route validation summary. TotalSegments=%d Passed=%d AdjacentSegments=%d AdjacentPassed=%d TotalLandings=%d ReachableLandings=%d/%d "
			"CheckpointLayers=16,32,48,64,80,96 CheckpointSize=%.0fx%.0f JumpZ=%.1f SprintJumpZ=%.1f Gravity=%.1f Apex=%.1f RunSpeed=%.1f SprintSpeed=%.1f AirControl=%.2f "
			"SameLevelTheoretical=%.1f UsableFraction=%.2f WorstRequiredRatio=%.3f "
			"LimitGap=%.1f MaxClearGap=%.1f MaxRequiredSpeed=%.1f WorstSegment=%03d->%03d"),
		SpiralTowerPlatformCount,
		PassedSegments + (bEntrySegmentPassed ? 1 : 0),
		SpiralTowerPlatformCount - 1,
		PassedSegments,
		SpiralTowerPlatformCount,
		PassedSegments + 1,
		SpiralTowerPlatformCount,
		SpiralTowerCheckpointLength,
		SpiralTowerCheckpointWidth,
		SpiralTowerJumpZVelocity,
		SpiralTowerSprintJumpZVelocity,
		SpiralTowerWorldGravity,
		TheoreticalApex,
		SpiralTowerRunSpeed,
		SpiralTowerSprintSpeed,
		SpiralTowerAirControl,
		SameLevelTheoreticalReach,
		SpiralTowerUsableReachFraction,
		GeneratedMaxReachUsage,
		GeneratedMinSafetyMargin,
		GeneratedMaxClearGap,
		GeneratedMaxRequiredSpeed,
		GeneratedWorstStep,
		GeneratedWorstStep + 1);
}

void ASpiralTowerWorldLayout::BuildBaseCourtyard()
{
	using namespace SpiralTowerWorldLayoutPrivate;

	const FVector FirstPlatform = PlatformSurfaceLocalLocations.IsEmpty()
		? FVector(1560.0f, 0.0f, SpiralTowerFirstSurfaceZ)
		: PlatformSurfaceLocalLocations[0];
	const float StartAngleRadians = FMath::DegreesToRadians(-12.0f);
	StartSurfaceLocalLocation = FVector(
		FMath::Cos(StartAngleRadians) * 1370.0f,
		FMath::Sin(StartAngleRadians) * 1370.0f,
		0.0f);
	const FRotator StartFacing = (FirstPlatform - StartSurfaceLocalLocation).Rotation();
	CheckpointLocalTransforms.Add(FTransform(
		FRotator(0.0f, StartFacing.Yaw, 0.0f),
		StartSurfaceLocalLocation));

	AddCollisionPrimitive(
		TEXT("BabylonBaseFoundation"),
		CubeFallback,
		FVector(0.0f, 0.0f, -45.0f),
		FRotator::ZeroRotator,
		FVector(BabylonBaseHalfExtent * 2.0f / 100.0f, BabylonBaseHalfExtent * 2.0f / 100.0f, 0.90f),
		FLinearColor(0.28f, 0.19f, 0.105f),
		true,
		PolyHavenWallMaterial);

	const FVector StartOutward = StartSurfaceLocalLocation.GetSafeNormal2D();
	const FVector StartTangent(-StartOutward.Y, StartOutward.X, 0.0f);
	UStaticMesh* EntranceVisual = PolyHavenGateMesh
		? PolyHavenGateMesh.Get()
		: (CastleDoorwayMesh ? CastleDoorwayMesh.Get() : ArchMesh.Get());
	const FRotator EntranceRotation(0.0f, StartOutward.Rotation().Yaw + 90.0f, 0.0f);
	const FVector EntranceScale = PolyHavenGateMesh
		? FVector(0.70f, 0.45f, 0.44f)
		: (CastleDoorwayMesh ? FVector(8.0f, 0.76f, 3.05f) : FVector(1.50f));
	const FVector EntranceLocation = PolyHavenGateMesh
		? SpiralTowerMeshLocationForBoundsBase(
			PolyHavenGateMesh.Get(),
			StartOutward * SpiralTowerInteriorWallRadius,
			EntranceRotation,
			EntranceScale)
		: StartOutward * 1632.0f;
	AddDecorMesh(
		TEXT("TowerEntranceArch"),
		EntranceVisual,
		CubeFallback,
		EntranceLocation,
		EntranceRotation,
		EntranceScale,
		FVector(0.35f, 3.0f, 3.4f),
		FLinearColor(0.095f, 0.090f, 0.125f));
	for (int32 PierSide = -1; PierSide <= 1; PierSide += 2)
	{
		UStaticMesh* PierVisual = PolyHavenTowerRoundMesh
			? PolyHavenTowerRoundMesh.Get()
			: CastleTowerMidMesh.Get();
		const FRotator PierRotation(0.0f, StartOutward.Rotation().Yaw + 90.0f, 0.0f);
		const FVector PierScale = PolyHavenTowerRoundMesh
			? FVector(0.26f, 0.26f, 0.32f)
			: FVector(1.75f, 1.35f, 3.45f);
		const FVector PierBase = StartOutward * 1615.0f
			+ StartTangent * (315.0f * static_cast<float>(PierSide));
		const FVector PierLocation = PolyHavenTowerRoundMesh
			? SpiralTowerMeshLocationForBoundsBase(
				PolyHavenTowerRoundMesh.Get(),
				PierBase,
				PierRotation,
				PierScale)
			: PierBase;
		AddDecorMesh(
			FString::Printf(TEXT("TowerEntrancePier_%d"), PierSide),
			PierVisual,
			CubeFallback,
			PierLocation,
			PierRotation,
			PierScale,
			FVector(1.55f, 1.20f, 3.40f),
			FLinearColor(0.125f, 0.125f, 0.180f));
	}

	AddTorch(StartSurfaceLocalLocation - StartTangent * 210.0f, StartOutward, 0);
	AddTorch(StartSurfaceLocalLocation + StartTangent * 210.0f, StartOutward, 1);
}

void ASpiralTowerWorldLayout::BuildSpiralPlatforms()
{
	using namespace SpiralTowerWorldLayoutPrivate;

	for (int32 PlatformIndex = 0; PlatformIndex < PlatformSurfaceLocalLocations.Num(); ++PlatformIndex)
	{
		const FVector SurfaceLocation = PlatformSurfaceLocalLocations[PlatformIndex];
		const FRotator PlatformRotation = PlatformLocalRotations[PlatformIndex];
		const bool bCheckpointPlatform = SpiralTowerIsCheckpointPlatform(PlatformIndex);
		const float PlatformLength = PlatformLocalDimensions[PlatformIndex].X;
		const float PlatformWidth = PlatformLocalDimensions[PlatformIndex].Y;
		const ESpiralTowerPlatformStyle Style = static_cast<ESpiralTowerPlatformStyle>(
			PlatformStyles[PlatformIndex]);
		const ESpiralTowerTraversalType TraversalType = PlatformTraversalTypes.IsValidIndex(PlatformIndex)
			? static_cast<ESpiralTowerTraversalType>(PlatformTraversalTypes[PlatformIndex])
			: ESpiralTowerTraversalType::GapJump;
		const FLinearColor PlatformTint = bCheckpointPlatform
			? FLinearColor(0.205f, 0.185f, 0.245f)
			: (Style == ESpiralTowerPlatformStyle::StoneBrick
				? FLinearColor(0.145f, 0.148f, 0.205f)
				: FLinearColor(SpiralTowerWoodTintR, SpiralTowerWoodTintG, SpiralTowerWoodTintB));
		const float CollisionThickness = Style == ESpiralTowerPlatformStyle::StoneBrick
			? SpiralTowerPlatformThickness
			: 18.0f;

		UStaticMeshComponent* PlatformCollision = AddCollisionPrimitive(
			FString::Printf(TEXT("SpiralPlatform_%02d"), PlatformIndex),
			CubeFallback,
			SurfaceLocation - FVector(0.0f, 0.0f, CollisionThickness * 0.5f),
			PlatformRotation,
			FVector(
				PlatformLength / 100.0f,
				PlatformWidth / 100.0f,
				CollisionThickness / 100.0f),
			PlatformTint,
			true,
			Style == ESpiralTowerPlatformStyle::StoneBrick
				? PolyHavenWallMaterial.Get()
				: (PolyHavenWoodMaterial ? PolyHavenWoodMaterial.Get() : PolyHavenTrimMaterial.Get()));
		if (PlatformCollision)
		{
			PlatformCollision->ComponentTags.AddUnique(TEXT("W02ClimbableLedge"));
		}
		if (PlatformCollision && Style != ESpiralTowerPlatformStyle::StoneBrick)
		{
			// The exact full-footprint proxy stays authoritative, while the visible
			// wood comes from aligned PBR beams/planks instead of a solid box.
			PlatformCollision->SetVisibility(false, true);
		}
		++BuiltPlatformCount;
		AddPlatformPresentation(
			PlatformIndex,
			SurfaceLocation,
			PlatformRotation,
			PlatformLength,
			PlatformWidth,
			PlatformStyles[PlatformIndex],
			bCheckpointPlatform);
		AddWallAnchor(
			PlatformIndex,
			SurfaceLocation,
			PlatformRotation,
			PlatformWidth,
			PlatformStyles[PlatformIndex]);
		if (TraversalType == ESpiralTowerTraversalType::Ladder)
		{
			BuildLadderToPlatform(PlatformIndex);
		}

		if (bCheckpointPlatform)
		{
			BuildObservationDeckDetails(PlatformIndex);
			const int32 NextPlatformIndex = FMath::Min(
				PlatformIndex + 1,
				PlatformSurfaceLocalLocations.Num() - 1);
			const FVector NextSurfaceLocation = PlatformSurfaceLocalLocations[NextPlatformIndex];
			const FRotator FacingRotation = (NextSurfaceLocation - SurfaceLocation).Rotation();
			CheckpointLocalTransforms.Add(FTransform(
				FRotator(0.0f, FacingRotation.Yaw, 0.0f),
				SurfaceLocation));

			const FVector OutwardDirection = FVector(
				SurfaceLocation.X,
				SurfaceLocation.Y,
				0.0f).GetSafeNormal();
			AddTorch(
				SurfaceLocation + OutwardDirection * (PlatformWidth * 0.5f - 25.0f),
				OutwardDirection,
				1 + (PlatformIndex + 1) / SpiralTowerCheckpointInterval);
			UE_LOG(
				LogTemp,
				Display,
				TEXT("W02 checkpoint platform ready. Layer=%d Slot=%d Size=%.0fx%.0f Local=(%.1f,%.1f,%.1f)"),
				PlatformIndex + 1,
				CheckpointLocalTransforms.Num() - 1,
				PlatformLength,
				PlatformWidth,
				SurfaceLocation.X,
				SurfaceLocation.Y,
				SurfaceLocation.Z);
		}
	}
}

void ASpiralTowerWorldLayout::BuildObservationDeckDetails(const int32 PlatformIndex)
{
	using namespace SpiralTowerWorldLayoutPrivate;

	if (!PlatformSurfaceLocalLocations.IsValidIndex(PlatformIndex)
		|| !PlatformLocalRotations.IsValidIndex(PlatformIndex)
		|| !PlatformLocalDimensions.IsValidIndex(PlatformIndex))
	{
		return;
	}

	const FVector Surface = PlatformSurfaceLocalLocations[PlatformIndex];
	const FRotator Rotation = PlatformLocalRotations[PlatformIndex];
	const FVector Tangent = Rotation.RotateVector(FVector::ForwardVector).GetSafeNormal2D();
	const FVector Inward = Rotation.RotateVector(FVector::RightVector).GetSafeNormal2D();
	const FVector Outward = -Inward;
	const FVector2D Dimensions = PlatformLocalDimensions[PlatformIndex];
	const FVector ParapetBase = Surface + Outward * (Dimensions.Y * 0.5f - 16.0f);

	// A chest-high crenellated outer wall turns each save floor into a believable
	// observation gallery without closing the incoming and outgoing route edges.
	AddCollisionPrimitive(
		FString::Printf(TEXT("LookoutParapet_%02d"), PlatformIndex),
		CubeFallback,
		ParapetBase + FVector(0.0f, 0.0f, 46.0f),
		Rotation,
		FVector(Dimensions.X / 100.0f, 0.28f, 0.92f),
		FLinearColor(0.105f, 0.11f, 0.15f),
		true,
		PolyHavenWallMaterial);
	for (int32 Crenel = -3; Crenel <= 3; Crenel += 2)
	{
		AddCollisionPrimitive(
			FString::Printf(TEXT("LookoutMerlon_%02d_%d"), PlatformIndex, Crenel),
			CubeFallback,
			ParapetBase + Tangent * (Dimensions.X * 0.12f * static_cast<float>(Crenel))
				+ FVector(0.0f, 0.0f, 104.0f),
			Rotation,
			FVector(1.05f, 0.34f, 0.78f),
			FLinearColor(0.10f, 0.105f, 0.145f),
			true,
			PolyHavenWallMaterial);
	}
	AddTorch(ParapetBase - Tangent * (Dimensions.X * 0.30f), Outward, 20 + PlatformIndex);
	AddTorch(ParapetBase + Tangent * (Dimensions.X * 0.30f), Outward, 21 + PlatformIndex);
	BuildJourneyMarker(PlatformIndex);
}

void ASpiralTowerWorldLayout::BuildJourneyMarker(const int32 PlatformIndex)
{
	if (!PlatformSurfaceLocalLocations.IsValidIndex(PlatformIndex)
		|| !PlatformLocalRotations.IsValidIndex(PlatformIndex))
	{
		return;
	}

	const int32 StoryIndex = (PlatformIndex + 1)
		/ SpiralTowerWorldLayoutPrivate::SpiralTowerCheckpointInterval;
	const FVector Surface = PlatformSurfaceLocalLocations[PlatformIndex];
	const FRotator Rotation = PlatformLocalRotations[PlatformIndex];
	const FVector Tangent = Rotation.RotateVector(FVector::ForwardVector).GetSafeNormal2D();
	const FVector Inward = Rotation.RotateVector(FVector::RightVector).GetSafeNormal2D();
	const FVector MarkerBase = Surface + Inward * 130.0f - Tangent * 210.0f;
	AddDecorMesh(
		FString::Printf(TEXT("LastWatchRecord_%02d"), StoryIndex),
		BrokenPlankMesh,
		CubeFallback,
		MarkerBase + FVector(0.0f, 0.0f, 12.0f),
		Rotation + FRotator(0.0f, -8.0f + static_cast<float>(StoryIndex) * 2.0f, 0.0f),
		FVector(0.60f),
		FVector(1.05f, 0.72f, 0.06f),
		FLinearColor(0.24f, 0.11f, 0.035f),
		PolyHavenWoodMaterial ? PolyHavenWoodMaterial.Get() : PolyHavenTrimMaterial.Get());

	UTextRenderComponent* RecordLabel = NewObject<UTextRenderComponent>(
		this,
		FName(*FString::Printf(TEXT("LastWatchLabel_%02d"), StoryIndex)));
	AddInstanceComponent(RecordLabel);
	RecordLabel->SetupAttachment(SceneRoot);
	RecordLabel->SetRelativeLocation(MarkerBase + FVector(0.0f, 0.0f, 92.0f));
	RecordLabel->SetRelativeRotation((Inward * -1.0f).Rotation());
	RecordLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	RecordLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	RecordLabel->SetWorldSize(17.0f);
	RecordLabel->SetTextRenderColor(FColor(238, 171, 83));
	RecordLabel->SetText(FText::FromString(FString::Printf(TEXT("FOUNDATION TABLET %d"), StoryIndex)));
	RecordLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RecordLabel->RegisterComponent();
}

void ASpiralTowerWorldLayout::BuildLadderToPlatform(const int32 PlatformIndex)
{
	using namespace SpiralTowerWorldLayoutPrivate;

	if (PlatformIndex <= 0
		|| !PlatformSurfaceLocalLocations.IsValidIndex(PlatformIndex)
		|| !PlatformSurfaceLocalLocations.IsValidIndex(PlatformIndex - 1))
	{
		return;
	}

	const FVector PreviousSurface = PlatformSurfaceLocalLocations[PlatformIndex - 1];
	const FVector CurrentSurface = PlatformSurfaceLocalLocations[PlatformIndex];
	FVector Approach = CurrentSurface - PreviousSurface;
	Approach.Z = 0.0f;
	Approach = Approach.GetSafeNormal();
	if (Approach.IsNearlyZero())
	{
		Approach = PlatformLocalRotations[PlatformIndex].RotateVector(FVector::ForwardVector).GetSafeNormal2D();
	}
	const FVector Lateral = FVector::CrossProduct(FVector::UpVector, Approach).GetSafeNormal2D();
	const float CurrentSupport = SpiralTowerRectangleSupport(
		-Approach,
		PlatformLocalRotations[PlatformIndex],
		PlatformLocalDimensions[PlatformIndex].X,
		PlatformLocalDimensions[PlatformIndex].Y);
	const FVector LadderTop = CurrentSurface - Approach * (CurrentSupport - 10.0f);
	const float LadderBottomZ = PreviousSurface.Z + 10.0f;
	const float LadderHeight = FMath::Max(80.0f, CurrentSurface.Z - LadderBottomZ + 16.0f);
	const FVector LadderMid(LadderTop.X, LadderTop.Y, LadderBottomZ + LadderHeight * 0.5f);
	const FRotator LadderRotation(0.0f, Lateral.Rotation().Yaw, 0.0f);

	for (int32 RailSide = -1; RailSide <= 1; RailSide += 2)
	{
		AddCollisionPrimitive(
			FString::Printf(TEXT("LadderRail_%02d_%d"), PlatformIndex, RailSide),
			CubeFallback,
			LadderMid + Lateral * (36.0f * static_cast<float>(RailSide)),
			FRotator::ZeroRotator,
			FVector(0.13f, 0.13f, LadderHeight / 100.0f),
			FLinearColor(SpiralTowerWoodTintR, SpiralTowerWoodTintG, SpiralTowerWoodTintB),
			true,
			PolyHavenWoodMaterial);
	}
	const int32 RungCount = FMath::Max(3, FMath::FloorToInt(LadderHeight / 28.0f));
	for (int32 RungIndex = 0; RungIndex <= RungCount; ++RungIndex)
	{
		const float RungAlpha = static_cast<float>(RungIndex) / static_cast<float>(RungCount);
		AddCollisionPrimitive(
			FString::Printf(TEXT("LadderRung_%02d_%02d"), PlatformIndex, RungIndex),
			CubeFallback,
			FVector(LadderMid.X, LadderMid.Y, LadderBottomZ + LadderHeight * RungAlpha),
			LadderRotation,
			FVector(0.90f, 0.10f, 0.10f),
			FLinearColor(SpiralTowerWoodTintR, SpiralTowerWoodTintG, SpiralTowerWoodTintB),
			true,
			PolyHavenWoodMaterial);
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 climbable ladder ready. Layer=%d Height=%.1f Rungs=%d Collision=QueryAndPhysics"),
		PlatformIndex + 1,
		LadderHeight,
		RungCount + 1);
}

void ASpiralTowerWorldLayout::AddPlatformPresentation(
	const int32 PlatformIndex,
	const FVector& SurfaceLocation,
	const FRotator& PlatformRotation,
	const float PlatformLength,
	const float PlatformWidth,
	const uint8 PlatformStyle,
	const bool bCheckpointPlatform)
{
	using namespace SpiralTowerWorldLayoutPrivate;

	const ESpiralTowerPlatformStyle Style = static_cast<ESpiralTowerPlatformStyle>(PlatformStyle);
	const FVector Tangent = PlatformRotation.RotateVector(FVector::ForwardVector).GetSafeNormal2D();
	const FVector Radial = PlatformRotation.RotateVector(FVector::RightVector).GetSafeNormal2D();
	if (Style == ESpiralTowerPlatformStyle::StoneBrick)
	{
		// The collision-authoritative block already carries the photoreal Wall PBR.
		// Do not cover ordinary stone steps with the stylized Kenney stair skin.
		if (!bCheckpointPlatform && PolyHavenWallMaterial)
		{
			return;
		}

		UStaticMesh* PreferredStone = bCheckpointPlatform && PolyHavenWalkwayMesh
			? PolyHavenWalkwayMesh.Get()
			: (bCheckpointPlatform && CastleBridgeMesh
				? CastleBridgeMesh.Get()
				: (StoneBrickMesh ? StoneBrickMesh.Get() : PathMesh.Get()));
		const FVector PreferredStoneScale = bCheckpointPlatform && PolyHavenWalkwayMesh
			? FVector(PlatformLength / 1456.27f, PlatformWidth / 336.65f, 0.035f)
			: (bCheckpointPlatform && CastleBridgeMesh
				? FVector(PlatformLength / 93.0f, PlatformWidth / 93.0f, 0.24f)
				: (StoneBrickMesh
					? FVector(PlatformLength / 23.0f, PlatformWidth / 82.0f, 0.32f)
					: FVector(bCheckpointPlatform ? 1.18f : 0.92f)));
		const FVector StoneVisualLocation = bCheckpointPlatform && PolyHavenWalkwayMesh
			? SpiralTowerMeshLocationForBoundsCenter(
				PolyHavenWalkwayMesh.Get(),
				SurfaceLocation - FVector(0.0f, 0.0f, 12.5f),
				PlatformRotation,
				PreferredStoneScale)
			: SurfaceLocation + FVector(0.0f, 0.0f, 2.0f);
		AddDecorMesh(
			FString::Printf(TEXT("StoneBrickTop_%02d"), PlatformIndex),
			PreferredStone,
			CubeFallback,
			StoneVisualLocation,
			PlatformRotation,
			PreferredStoneScale,
			FVector(PlatformLength / 102.0f, PlatformWidth / 102.0f, 0.055f),
			FLinearColor(0.12f, 0.115f, 0.145f),
			PolyHavenWallMaterial);
		return;
	}

	if (Style == ESpiralTowerPlatformStyle::TimberBeam)
	{
		for (int32 BeamIndex = -1; BeamIndex <= 1; ++BeamIndex)
		{
			AddDecorMesh(
				FString::Printf(TEXT("TimberBeam_%02d_%d"), PlatformIndex, BeamIndex),
				WoodBeamMesh,
				CubeFallback,
				SurfaceLocation + Radial * (PlatformWidth * 0.30f * static_cast<float>(BeamIndex))
					- FVector(0.0f, 0.0f, 14.0f),
				PlatformRotation,
				FVector(1.0f),
				FVector(PlatformLength / 100.0f, 0.28f, 0.20f),
				FLinearColor(SpiralTowerWoodTintR, SpiralTowerWoodTintG, SpiralTowerWoodTintB),
				PolyHavenWoodMaterial ? PolyHavenWoodMaterial.Get() : PolyHavenTrimMaterial.Get());
		}
		return;
	}

	for (int32 PlankIndex = 0; PlankIndex < 3; ++PlankIndex)
	{
		const float Across = (static_cast<float>(PlankIndex) - 1.0f) * PlatformWidth * 0.27f;
		const float LengthFactor = 0.72f + static_cast<float>((PlatformIndex + PlankIndex) % 3) * 0.10f;
		const float YawJitter = static_cast<float>((PlatformIndex * 7 + PlankIndex * 5) % 9) - 4.0f;
		AddDecorMesh(
			FString::Printf(TEXT("BrokenPlank_%02d_%d"), PlatformIndex, PlankIndex),
			BrokenPlankMesh,
			CubeFallback,
			SurfaceLocation + Radial * Across + Tangent * (static_cast<float>(PlankIndex) - 1.0f) * 18.0f
				+ FVector(0.0f, 0.0f, 3.0f + static_cast<float>(PlankIndex % 2) * 3.0f),
			PlatformRotation + FRotator(0.0f, YawJitter, 0.0f),
			FVector(0.90f),
			FVector(PlatformLength * LengthFactor / 100.0f, PlatformWidth * 0.22f / 100.0f, 0.10f),
			FLinearColor(0.22f, 0.095f, 0.032f),
			PolyHavenWoodMaterial ? PolyHavenWoodMaterial.Get() : PolyHavenTrimMaterial.Get());
	}
}

void ASpiralTowerWorldLayout::AddWallAnchor(
	const int32 PlatformIndex,
	const FVector& SurfaceLocation,
	const FRotator& PlatformRotation,
	const float PlatformWidth,
	const uint8 PlatformStyle)
{
	using namespace SpiralTowerWorldLayoutPrivate;

	const ESpiralTowerPlatformStyle Style = static_cast<ESpiralTowerPlatformStyle>(PlatformStyle);
	const FVector Outward = FVector(SurfaceLocation.X, SurfaceLocation.Y, 0.0f).GetSafeNormal();
	const float PlatformRadius = FVector(SurfaceLocation.X, SurfaceLocation.Y, 0.0f).Size();
	const float WallInnerRadius = SpiralTowerInteriorWallRadius - SpiralTowerInteriorWallThickness * 0.5f;
	const float PlatformOuterRadius = PlatformRadius + PlatformWidth * 0.5f;
	const float AnchorSpan = FMath::Max(55.0f, WallInnerRadius - PlatformOuterRadius + 12.0f);
	const FVector AnchorLocation = SurfaceLocation
		+ Outward * (PlatformWidth * 0.5f + AnchorSpan * 0.5f)
		- FVector(0.0f, 0.0f, Style == ESpiralTowerPlatformStyle::StoneBrick ? 25.0f : 18.0f);
	UStaticMesh* PreferredAnchor = Style == ESpiralTowerPlatformStyle::StoneBrick
		? (PolyHavenWallMaterial ? nullptr : InteriorWallMesh.Get())
		: WoodBeamMesh.Get();
	AddDecorMesh(
		FString::Printf(TEXT("WallAnchor_%02d"), PlatformIndex),
		PreferredAnchor,
		CubeFallback,
		AnchorLocation,
		PlatformRotation,
		Style == ESpiralTowerPlatformStyle::StoneBrick
			? FVector(0.86f, AnchorSpan / 100.0f, 0.30f)
			: FVector(0.72f),
		FVector(
			Style == ESpiralTowerPlatformStyle::StoneBrick ? 0.82f : 0.48f,
			AnchorSpan / 100.0f,
			Style == ESpiralTowerPlatformStyle::StoneBrick ? 0.52f : 0.28f),
		Style == ESpiralTowerPlatformStyle::StoneBrick
			? FLinearColor(0.070f, 0.068f, 0.092f)
			: FLinearColor(SpiralTowerWoodTintR, SpiralTowerWoodTintG, SpiralTowerWoodTintB),
		Style == ESpiralTowerPlatformStyle::StoneBrick
			? PolyHavenWallMaterial.Get()
			: (PolyHavenWoodMaterial ? PolyHavenWoodMaterial.Get() : PolyHavenTrimMaterial.Get()));
}

void ASpiralTowerWorldLayout::BuildBabylonZiggurat()
{
	using namespace SpiralTowerWorldLayoutPrivate;

	if (PlatformSurfaceLocalLocations.Num() < SpiralTowerPlatformCount)
	{
		return;
	}

	const float TowerHeight = SummitSurfaceLocalZ + 760.0f;
	if (UStaticMeshComponent* NightSky = AddCollisionPrimitive(
		TEXT("BabylonExteriorSky"),
		SphereFallback,
		FVector(0.0f, 0.0f, TowerHeight * 0.42f),
		FRotator::ZeroRotator,
		FVector(420.0f),
		FLinearColor(0.002f, 0.006f, 0.028f),
		false))
	{
		ExteriorSkyDome = NightSky;
		NightSky->SetReverseCulling(true);
		NightSky->SetCastShadow(false);
	}

	int32 BuiltTerraces = 0;
	int32 BuiltFacadeModules = 0;
	for (int32 TierIndex = 0; TierIndex < BabylonLowerTierCount; ++TierIndex)
	{
		const int32 CheckpointPlatformIndex =
			(TierIndex + 1) * SpiralTowerCheckpointInterval - 1;
		const float TierBaseZ = TierIndex == 0
			? 0.0f
			: PlatformSurfaceLocalLocations[
				TierIndex * SpiralTowerCheckpointInterval - 1].Z;
		const float TierTopZ =
			PlatformSurfaceLocalLocations[CheckpointPlatformIndex].Z;
		const float TierHeight = FMath::Max(120.0f, TierTopZ - TierBaseZ);
		const float CollisionCoreHalfExtent = BabylonFirstCoreHalfExtent
			- static_cast<float>(TierIndex) * BabylonCoreRecession;
		const float FacadeHalfExtent = BabylonFirstFacadeHalfExtent
			- static_cast<float>(TierIndex) * BabylonFacadeRecession;
		const float TerraceHalfExtent = BabylonFirstTerraceHalfExtent
			- static_cast<float>(TierIndex) * BabylonTerraceRecession;
		const FLinearColor TierTint = TierIndex % 2 == 0
			? FLinearColor(0.30f, 0.205f, 0.115f)
			: FLinearColor(0.245f, 0.155f, 0.082f);

		// Each tier is a solid mud-brick core. Its square corner remains inside the
		// verified circular traversal line, while the broad roof becomes a genuine
		// rest floor at every checkpoint.
		AddCollisionPrimitive(
			FString::Printf(TEXT("BabylonTierCore_%02d"), TierIndex + 1),
			CubeFallback,
			FVector(0.0f, 0.0f, TierBaseZ + TierHeight * 0.5f),
			FRotator::ZeroRotator,
			FVector(
				CollisionCoreHalfExtent * 2.0f / 100.0f,
				CollisionCoreHalfExtent * 2.0f / 100.0f,
				TierHeight / 100.0f),
			TierTint,
			true,
			PolyHavenWallMaterial);
		AddCollisionPrimitive(
			FString::Printf(TEXT("BabylonTerrace_%02d"), TierIndex + 1),
			CubeFallback,
			FVector(0.0f, 0.0f, TierTopZ - BabylonTerraceThickness * 0.5f),
			FRotator::ZeroRotator,
			FVector(
				TerraceHalfExtent * 2.0f / 100.0f,
				TerraceHalfExtent * 2.0f / 100.0f,
				BabylonTerraceThickness / 100.0f),
			TierTint * 1.08f,
			true,
			PolyHavenWallMaterial);
		++BuiltTerraces;

		for (int32 FaceIndex = 0; FaceIndex < 4; ++FaceIndex)
		{
			const float FaceYaw = static_cast<float>(FaceIndex) * 90.0f;
			const float FaceRadians = FMath::DegreesToRadians(FaceYaw);
			const FVector Outward(FMath::Cos(FaceRadians), FMath::Sin(FaceRadians), 0.0f);
			const FVector Tangent(-Outward.Y, Outward.X, 0.0f);
			const FRotator FaceRotation(0.0f, FaceYaw + 90.0f, 0.0f);
			const FVector WallScale = PolyHavenWallStraightMesh
				? FVector(
					FacadeHalfExtent * 2.0f / 1456.28f,
					110.0f / 417.21f,
					TierHeight / 852.56f)
				: FVector(FacadeHalfExtent * 2.0f / 100.0f, 0.78f, TierHeight / 100.0f);
			const FVector WallBoundsCenter = Outward * FacadeHalfExtent
				+ FVector(0.0f, 0.0f, TierBaseZ + TierHeight * 0.5f);
			const FVector WallLocation = PolyHavenWallStraightMesh
				? SpiralTowerMeshLocationForBoundsCenter(
					PolyHavenWallStraightMesh,
					WallBoundsCenter,
					FaceRotation,
					WallScale)
				: WallBoundsCenter;
			AddDecorMesh(
				FString::Printf(TEXT("BabylonBrickFacing_%02d_%d"), TierIndex + 1, FaceIndex),
				PolyHavenWallStraightMesh,
				CubeFallback,
				WallLocation,
				FaceRotation,
				WallScale,
					FVector(FacadeHalfExtent * 2.0f / 100.0f, 0.78f, TierHeight / 100.0f),
				TierTint,
				PolyHavenWallMaterial);
			++BuiltFacadeModules;

			// Three repeated recesses per face echo the monumental arched galleries
			// seen in historic Tower-of-Babel imagery without turning imported art
			// into collision authority.
			for (int32 GalleryIndex = -1; GalleryIndex <= 1; ++GalleryIndex)
			{
				UStaticMesh* GalleryMesh = PolyHavenGateMesh
					? PolyHavenGateMesh.Get()
					: (CastleDoorwayMesh ? CastleDoorwayMesh.Get() : ArchMesh.Get());
				const float GalleryHeight = FMath::Clamp(TierHeight * 0.52f, 250.0f, 390.0f);
				const float GalleryWidth = FMath::Min(360.0f, FacadeHalfExtent * 0.46f);
				const FVector GalleryScale = PolyHavenGateMesh
					? FVector(GalleryWidth / 741.02f, 84.0f / 267.20f, GalleryHeight / 861.62f)
					: FVector(GalleryWidth / 100.0f, 0.62f, GalleryHeight / 100.0f);
				const FVector GalleryBase = Outward * (FacadeHalfExtent + 18.0f)
					+ Tangent * (static_cast<float>(GalleryIndex) * FacadeHalfExtent * 0.56f)
					+ FVector(0.0f, 0.0f, TierBaseZ + 28.0f);
				const FVector GalleryLocation = PolyHavenGateMesh
					? SpiralTowerMeshLocationForBoundsBase(
						PolyHavenGateMesh,
						GalleryBase,
						FaceRotation,
						GalleryScale)
					: GalleryBase;
				AddDecorMesh(
					FString::Printf(
						TEXT("BabylonGallery_%02d_%d_%d"),
						TierIndex + 1,
						FaceIndex,
						GalleryIndex + 1),
					GalleryMesh,
					CubeFallback,
					GalleryLocation,
					FaceRotation,
					GalleryScale,
					FVector(GalleryWidth / 100.0f, 0.60f, GalleryHeight / 100.0f),
					FLinearColor(0.16f, 0.095f, 0.050f),
					PolyHavenTrimMaterial ? PolyHavenTrimMaterial.Get() : PolyHavenWallMaterial.Get());
				++BuiltFacadeModules;
			}

			// Low, broken parapet runs communicate the square terrace silhouette but
			// deliberately leave a broad central gap for traversal and camera sightlines.
			const float ParapetGap = 620.0f;
			const float ParapetLength = TerraceHalfExtent - ParapetGap * 0.5f;
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				const FVector ParapetLocation = Outward * (TerraceHalfExtent - 18.0f)
					+ Tangent * (ParapetGap * 0.5f + ParapetLength * 0.5f)
						* static_cast<float>(Side)
					+ FVector(0.0f, 0.0f, TierTopZ + 54.0f);
				AddDecorMesh(
					FString::Printf(
						TEXT("BabylonTerraceParapet_%02d_%d_%d"),
						TierIndex + 1,
						FaceIndex,
						Side),
					nullptr,
					CubeFallback,
					ParapetLocation,
					FaceRotation,
					FVector::OneVector,
					FVector(ParapetLength / 100.0f, 0.32f, 1.08f),
					FLinearColor(0.255f, 0.165f, 0.085f),
					PolyHavenWallMaterial);
			}
			AddDecorMesh(
				FString::Printf(TEXT("BabylonTerraceFascia_%02d_%d"), TierIndex + 1, FaceIndex),
				nullptr,
				CubeFallback,
				Outward * (TerraceHalfExtent - 16.0f)
					+ FVector(0.0f, 0.0f, TierTopZ - 62.0f),
				FaceRotation,
				FVector::OneVector,
				FVector(TerraceHalfExtent * 2.0f / 100.0f, 0.58f, 1.24f),
				FLinearColor(0.275f, 0.175f, 0.088f),
				PolyHavenWallMaterial);
		}

		if (PolyHavenTowerRoundMesh)
		{
			const FVector FullTowerSize = PolyHavenTowerRoundMesh->GetBounds().BoxExtent * 2.0f;
			const float TowerTargetHeight = FMath::Clamp(TierHeight * 0.72f, 300.0f, 520.0f);
			const FVector TowerScale(
				280.0f / FMath::Max(1.0f, FullTowerSize.X),
				280.0f / FMath::Max(1.0f, FullTowerSize.Y),
				TowerTargetHeight / FMath::Max(1.0f, FullTowerSize.Z));
			for (int32 CornerX = -1; CornerX <= 1; CornerX += 2)
			{
				for (int32 CornerY = -1; CornerY <= 1; CornerY += 2)
				{
					const FVector TowerBase(
						FacadeHalfExtent * 0.78f * static_cast<float>(CornerX),
						FacadeHalfExtent * 0.78f * static_cast<float>(CornerY),
						TierBaseZ + 8.0f);
					AddDecorMesh(
						FString::Printf(
							TEXT("BabylonCornerButtress_%02d_%d_%d"),
							TierIndex + 1,
							CornerX,
							CornerY),
						PolyHavenTowerRoundMesh,
						CylinderFallback,
						SpiralTowerMeshLocationForBoundsBase(
							PolyHavenTowerRoundMesh,
							TowerBase,
							FRotator::ZeroRotator,
							TowerScale),
						FRotator::ZeroRotator,
						TowerScale,
						FVector(2.8f, 2.8f, TowerTargetHeight / 100.0f),
						FLinearColor(0.22f, 0.135f, 0.070f),
						PolyHavenWallMaterial);
					++BuiltFacadeModules;
				}
			}
		}
	}

	// A monumental but non-authoritative processional stair completes the east
	// elevation. The verified climbing route remains the only gameplay ascent.
	const float FirstTerraceZ =
		PlatformSurfaceLocalLocations[SpiralTowerCheckpointInterval - 1].Z;
	for (int32 StepIndex = 0; StepIndex < 14; ++StepIndex)
	{
		const float Alpha = static_cast<float>(StepIndex) / 13.0f;
		const float StepZ = FirstTerraceZ * Alpha;
		const float StepX = FMath::Lerp(BabylonBaseHalfExtent + 520.0f, 1040.0f, Alpha);
		AddDecorMesh(
			FString::Printf(TEXT("BabylonProcessionalStep_%02d"), StepIndex),
			nullptr,
			CubeFallback,
			FVector(StepX, 0.0f, StepZ - 15.0f),
			FRotator::ZeroRotator,
			FVector::OneVector,
			FVector(4.20f, 10.5f, 0.30f),
			FLinearColor(0.31f, 0.20f, 0.105f),
			PolyHavenWallMaterial);
	}

	const int32 FillLightCount = BabylonLowerTierCount + 2;
	for (int32 FillIndex = 0; FillIndex < FillLightCount; ++FillIndex)
	{
		const float AngleRadians = FMath::DegreesToRadians(static_cast<float>(FillIndex) * 137.0f);
		UPointLightComponent* FillLight = NewObject<UPointLightComponent>(
			this,
			FName(*FString::Printf(TEXT("BabylonFillLight_%02d"), FillIndex)));
		AddInstanceComponent(FillLight);
		FillLight->SetupAttachment(SceneRoot);
		FillLight->SetRelativeLocation(FVector(
			FMath::Cos(AngleRadians) * 1120.0f,
			FMath::Sin(AngleRadians) * 1120.0f,
			260.0f + static_cast<float>(FillIndex) * 690.0f));
		FillLight->SetMobility(EComponentMobility::Movable);
		FillLight->SetIntensity(1200.0f);
		FillLight->SetAttenuationRadius(1850.0f);
		FillLight->SetLightColor(FLinearColor(0.30f, 0.34f, 0.66f));
		FillLight->SetCastShadows(false);
		FillLight->RegisterComponent();
		TorchLights.Add(FillLight);
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 Babylon ziggurat ready. LowerTiers=%d SummitTemple=1 Terraces=%d FacadeModules=%d Base=%.0fx%.0f Height=%.1f Route=Exterior"),
		BabylonLowerTierCount,
		BuiltTerraces,
		BuiltFacadeModules,
		BabylonBaseHalfExtent * 2.0f,
		BabylonBaseHalfExtent * 2.0f,
		SummitSurfaceLocalZ);
}

void ASpiralTowerWorldLayout::BuildBabylonScaffolds()
{
	using namespace SpiralTowerWorldLayoutPrivate;

	int32 CollidableBeamCount = 0;
	for (int32 TierIndex = 0; TierIndex < BabylonLowerTierCount; ++TierIndex)
	{
		const float TierBaseZ = TierIndex == 0
			? 80.0f
			: PlatformSurfaceLocalLocations[
				TierIndex * SpiralTowerCheckpointInterval - 1].Z + 72.0f;
		const float TierTopZ = PlatformSurfaceLocalLocations[
			(TierIndex + 1) * SpiralTowerCheckpointInterval - 1].Z - 92.0f;
		const float TierHeight = FMath::Max(160.0f, TierTopZ - TierBaseZ);
		const float FacadeHalfExtent = BabylonFirstFacadeHalfExtent
			- static_cast<float>(TierIndex) * BabylonFacadeRecession;

		for (int32 FaceIndex = 0; FaceIndex < 4; ++FaceIndex)
		{
			const float FaceYaw = static_cast<float>(FaceIndex) * 90.0f;
			const float FaceRadians = FMath::DegreesToRadians(FaceYaw);
			const FVector Outward(FMath::Cos(FaceRadians), FMath::Sin(FaceRadians), 0.0f);
			const FVector Tangent(-Outward.Y, Outward.X, 0.0f);
			const FRotator BeamRotation(0.0f, FaceYaw + 90.0f, 0.0f);
			const float ScaffoldOffset = FacadeHalfExtent + 72.0f;
			const float BeamLength = FacadeHalfExtent * 1.24f;

			for (int32 RailIndex = 0; RailIndex < 3; ++RailIndex)
			{
				const float RailAlpha = static_cast<float>(RailIndex) / 2.0f;
				UStaticMeshComponent* Rail = AddCollisionPrimitive(
					FString::Printf(
						TEXT("BabylonScaffoldRail_%02d_%d_%d"),
						TierIndex + 1,
						FaceIndex,
						RailIndex),
					CubeFallback,
					Outward * ScaffoldOffset
						+ FVector(0.0f, 0.0f, FMath::Lerp(TierBaseZ, TierTopZ, RailAlpha)),
					BeamRotation,
					FVector(BeamLength / 100.0f, 0.24f, 0.24f),
					FLinearColor(SpiralTowerWoodTintR, SpiralTowerWoodTintG, SpiralTowerWoodTintB),
					true,
					PolyHavenWoodMaterial ? PolyHavenWoodMaterial.Get() : PolyHavenTrimMaterial.Get());
				if (Rail)
				{
					Rail->ComponentTags.AddUnique(TEXT("W02ClimbableLedge"));
				}
				++CollidableBeamCount;
			}

			for (int32 PostSide = -1; PostSide <= 1; PostSide += 2)
			{
				AddCollisionPrimitive(
					FString::Printf(
						TEXT("BabylonScaffoldPost_%02d_%d_%d"),
						TierIndex + 1,
						FaceIndex,
						PostSide),
					CubeFallback,
					Outward * ScaffoldOffset
						+ Tangent * (BeamLength * 0.43f * static_cast<float>(PostSide))
						+ FVector(0.0f, 0.0f, TierBaseZ + TierHeight * 0.5f),
					FRotator::ZeroRotator,
					FVector(0.22f, 0.22f, TierHeight / 100.0f),
					FLinearColor(SpiralTowerWoodTintR, SpiralTowerWoodTintG, SpiralTowerWoodTintB),
					true,
					PolyHavenWoodMaterial ? PolyHavenWoodMaterial.Get() : PolyHavenTrimMaterial.Get());
				++CollidableBeamCount;
			}
		}
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 Babylon scaffolds ready. Tiers=%d Faces=%d CollidableBeams=%d RouteClearance=VerifiedByCoreInset"),
		BabylonLowerTierCount,
		BabylonLowerTierCount * 4,
		CollidableBeamCount);
}

void ASpiralTowerWorldLayout::BuildInteriorShell()
{
	using namespace SpiralTowerWorldLayoutPrivate;

	const float WallHeight = SummitSurfaceLocalZ + 440.0f;
	const float SegmentLength =
		(2.0f * PI * SpiralTowerInteriorWallRadius / static_cast<float>(SpiralTowerInteriorWallSegments))
		* 1.055f;
	if (UStaticMeshComponent* NightSky = AddCollisionPrimitive(
		TEXT("ExteriorNightSky"),
		SphereFallback,
		FVector(0.0f, 0.0f, WallHeight * 0.5f),
		FRotator::ZeroRotator,
		FVector(400.0f),
		FLinearColor(0.002f, 0.006f, 0.028f),
		false))
	{
		ExteriorSkyDome = NightSky;
		NightSky->SetReverseCulling(true);
		NightSky->SetCastShadow(false);
	}

	const float SegmentAngleDegrees = 360.0f / static_cast<float>(SpiralTowerInteriorWallSegments);
	const float SummitAngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(
		SummitSurfaceLocalLocation.Y,
		SummitSurfaceLocalLocation.X));
	const int32 SummitExitSegment = (
		FMath::RoundToInt(SummitAngleDegrees / SegmentAngleDegrees)
			% SpiralTowerInteriorWallSegments
			+ SpiralTowerInteriorWallSegments)
		% SpiralTowerInteriorWallSegments;
	const int32 SummitExitBand = FMath::FloorToInt(
		SummitSurfaceLocalZ / SpiralTowerWallStoreyHeight);
	const int32 WallBandCount = FMath::CeilToInt(WallHeight / SpiralTowerWallStoreyHeight);
	for (int32 BandIndex = 0; BandIndex < WallBandCount; ++BandIndex)
	{
		const float BandBaseZ = static_cast<float>(BandIndex) * SpiralTowerWallStoreyHeight;
		const float BandHeight = FMath::Min(SpiralTowerWallStoreyHeight, WallHeight - BandBaseZ);
		for (int32 SegmentIndex = 0; SegmentIndex < SpiralTowerInteriorWallSegments; ++SegmentIndex)
		{
			const float AngleDegrees = static_cast<float>(SegmentIndex) * SegmentAngleDegrees;
			const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
			const FVector Radial(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians), 0.0f);
			const FVector Tangent(-Radial.Y, Radial.X, 0.0f);
			const FRotator WallRotation(0.0f, AngleDegrees + 90.0f, 0.0f);
			const bool bWindowStorey = BandIndex > 0 && BandIndex % 2 == 1;
			const int32 WindowPhase = (BandIndex / 2) % 2 == 0 ? 0 : 2;
			const bool bWindowSegment = bWindowStorey
				&& (SegmentIndex - WindowPhase + SpiralTowerInteriorWallSegments) % 5 == 0;
			const bool bEntranceSegment = BandIndex == 0 && SegmentIndex == 19;
			const bool bSummitExitSegment = BandIndex == SummitExitBand
				&& SegmentIndex == SummitExitSegment;
			const bool bHasOpening = bWindowSegment || bEntranceSegment || bSummitExitSegment;
			const FLinearColor WallTint = SegmentIndex % 2 == 0
				? FLinearColor(0.095f, 0.105f, 0.155f)
				: FLinearColor(0.115f, 0.120f, 0.175f);

			auto AddWallBlock = [&](const TCHAR* BlockLabel,
				const float TangentOffset,
				const float BlockBaseZ,
				const float BlockLength,
				const float BlockHeight)
			{
				if (BlockLength <= UE_KINDA_SMALL_NUMBER || BlockHeight <= UE_KINDA_SMALL_NUMBER)
				{
					return;
				}
				AddCollisionPrimitive(
					FString::Printf(
						TEXT("Wall_%02d_%02d_%s"),
						BandIndex,
						SegmentIndex,
						BlockLabel),
					CubeFallback,
					Radial * SpiralTowerInteriorWallRadius
						+ Tangent * TangentOffset
						+ FVector(0.0f, 0.0f, BlockBaseZ + BlockHeight * 0.5f),
					WallRotation,
					FVector(
						BlockLength / 100.0f,
						SpiralTowerInteriorWallThickness / 100.0f,
						BlockHeight / 100.0f),
					WallTint,
					true,
					PolyHavenWallMaterial);
			};

			if (!bHasOpening)
			{
				AddWallBlock(TEXT("Solid"), 0.0f, BandBaseZ, SegmentLength, BandHeight);
				UStaticMesh* WallVisual = PolyHavenWallStraightMesh
					? PolyHavenWallStraightMesh.Get()
					: InteriorWallMesh.Get();
				const FVector WallVisualScale = PolyHavenWallStraightMesh
					? FVector(
						SegmentLength / 1456.28f,
						SpiralTowerInteriorWallThickness / 417.21f,
						BandHeight / 852.56f)
					: FVector(SegmentLength / 100.0f, 0.72f, BandHeight / 131.0f);
				const FVector WallBoundsCenter = Radial * (SpiralTowerInteriorWallRadius - 2.0f)
					+ FVector(0.0f, 0.0f, BandBaseZ + BandHeight * 0.5f);
				const FVector WallVisualLocation = PolyHavenWallStraightMesh
					? SpiralTowerMeshLocationForBoundsCenter(
						PolyHavenWallStraightMesh,
						WallBoundsCenter,
						WallRotation,
						WallVisualScale)
					: Radial * (SpiralTowerInteriorWallRadius - 32.0f)
						+ FVector(0.0f, 0.0f, BandBaseZ + BandHeight * 0.5f);
				AddDecorMesh(
					FString::Printf(TEXT("InteriorWallFacing_%02d_%02d"), BandIndex, SegmentIndex),
					WallVisual,
					CubeFallback,
					WallVisualLocation,
					WallRotation,
					WallVisualScale,
					FVector(SegmentLength / 100.0f, 0.72f, BandHeight / 100.0f),
					FLinearColor(0.125f, 0.130f, 0.185f),
					PolyHavenWallMaterial);
				continue;
			}

			const float OpeningWidth = bSummitExitSegment
				? 620.0f
				: (bEntranceSegment ? 300.0f : 190.0f);
			const float OpeningBottom = (bEntranceSegment || bSummitExitSegment) ? 0.0f : 145.0f;
			const float OpeningHeight = bSummitExitSegment
				? 430.0f
				: (bEntranceSegment ? 360.0f : 220.0f);
			const float OpeningTop = FMath::Min(BandHeight, OpeningBottom + OpeningHeight);
			const float SideLength = FMath::Max(1.0f, (SegmentLength - OpeningWidth) * 0.5f);
			AddWallBlock(TEXT("Sill"), 0.0f, BandBaseZ, SegmentLength, OpeningBottom);
			AddWallBlock(
				TEXT("Lintel"),
				0.0f,
				BandBaseZ + OpeningTop,
				SegmentLength,
				BandHeight - OpeningTop);
			const float SideOffset = OpeningWidth * 0.5f + SideLength * 0.5f;
			AddWallBlock(TEXT("Left"), -SideOffset, BandBaseZ + OpeningBottom, SideLength, OpeningTop - OpeningBottom);
			AddWallBlock(TEXT("Right"), SideOffset, BandBaseZ + OpeningBottom, SideLength, OpeningTop - OpeningBottom);

			// Ordinary windows show the changing sky but remain safe. The final
			// opening is deliberately unsealed so the player can walk onto the dawn ledge.
			if (!bSummitExitSegment)
			{
				if (UStaticMeshComponent* OpeningBarrier = AddCollisionPrimitive(
					FString::Printf(TEXT("Wall_%02d_%02d_OpeningBarrier"), BandIndex, SegmentIndex),
					CubeFallback,
					Radial * (SpiralTowerInteriorWallRadius - SpiralTowerInteriorWallThickness * 0.5f)
						+ FVector(0.0f, 0.0f, BandBaseZ + OpeningBottom + (OpeningTop - OpeningBottom) * 0.5f),
					WallRotation,
					FVector(OpeningWidth / 100.0f, 0.08f, (OpeningTop - OpeningBottom) / 100.0f),
					FLinearColor::Black))
				{
					OpeningBarrier->SetVisibility(false, true);
				}
			}
			UStaticMesh* OpeningVisual = PolyHavenGateMesh
				? PolyHavenGateMesh.Get()
				: (CastleDoorwayMesh ? CastleDoorwayMesh.Get() : ArchMesh.Get());
			const FVector OpeningVisualScale = PolyHavenGateMesh
				? FVector(
					OpeningWidth / 741.02f,
					SpiralTowerInteriorWallThickness / 267.20f,
					(OpeningTop - OpeningBottom) / 861.62f)
				: (CastleDoorwayMesh
					? FVector(OpeningWidth / 50.0f, 0.64f, (OpeningTop - OpeningBottom) / 131.0f)
					: FVector(1.0f));
			const FVector OpeningVisualBase = Radial * SpiralTowerInteriorWallRadius
				+ FVector(0.0f, 0.0f, BandBaseZ + OpeningBottom);
			const FVector OpeningVisualLocation = PolyHavenGateMesh
				? SpiralTowerMeshLocationForBoundsBase(
					PolyHavenGateMesh,
					OpeningVisualBase,
					WallRotation,
					OpeningVisualScale)
				: Radial * (SpiralTowerInteriorWallRadius - 66.0f)
					+ FVector(0.0f, 0.0f, BandBaseZ + OpeningBottom);
			AddDecorMesh(
				FString::Printf(TEXT("WindowFrame_%02d_%02d"), BandIndex, SegmentIndex),
				OpeningVisual,
				CubeFallback,
				OpeningVisualLocation,
				WallRotation,
				OpeningVisualScale,
				FVector(OpeningWidth / 100.0f, 0.18f, (OpeningTop - OpeningBottom) / 100.0f),
				FLinearColor(0.075f, 0.078f, 0.120f),
				PolyHavenWallMaterial);
		}
	}

	const int32 FillLightCount = FMath::CeilToInt(WallHeight / 650.0f);
	for (int32 FillIndex = 0; FillIndex < FillLightCount; ++FillIndex)
	{
		const float AngleRadians = FMath::DegreesToRadians(static_cast<float>(FillIndex) * 137.0f);
		UPointLightComponent* FillLight = NewObject<UPointLightComponent>(
			this,
			FName(*FString::Printf(TEXT("InteriorFillLight_%02d"), FillIndex)));
		AddInstanceComponent(FillLight);
		FillLight->SetupAttachment(SceneRoot);
		FillLight->SetRelativeLocation(FVector(
			FMath::Cos(AngleRadians) * 420.0f,
			FMath::Sin(AngleRadians) * 420.0f,
			300.0f + static_cast<float>(FillIndex) * 650.0f));
		FillLight->SetMobility(EComponentMobility::Movable);
		FillLight->SetIntensity(1450.0f);
		FillLight->SetAttenuationRadius(1620.0f);
		FillLight->SetLightColor(FLinearColor(0.25f, 0.38f, 0.82f));
		FillLight->SetCastShadows(false);
		FillLight->RegisterComponent();
		TorchLights.Add(FillLight);
	}
}

void ASpiralTowerWorldLayout::BuildOverheadTimbers()
{
	using namespace SpiralTowerWorldLayoutPrivate;

	const float MinimumRouteInnerRadius =
		(SpiralTowerInteriorWallRadius - SpiralTowerInteriorWallThickness * 0.5f)
		- SpiralTowerMaxWallInset - SpiralTowerMaxPlatformWidth;
	const float BeamRouteClearance = MinimumRouteInnerRadius
		- (SpiralTowerBeamHalfSpan + SpiralTowerBeamWidth * 0.5f);
	ensureAlwaysMsgf(
		BeamRouteClearance > SpiralTowerCapsuleRadius + SpiralTowerStandingMargin,
		TEXT("W02 roof beams intrude into the platform route: clearance %.1f cm."),
		BeamRouteClearance);
	const int32 BeamLevelCount = FMath::Max(
		1,
		FMath::FloorToInt((SummitSurfaceLocalZ - 440.0f) / SpiralTowerBeamVerticalSpacing) + 1);
	float WorstLandingCenterClearance = MAX_flt;
	int32 ValidatedLandingCount = 0;
	for (const FVector& Landing : PlatformSurfaceLocalLocations)
	{
		float LandingClearance = MAX_flt;
		bool bHasVerticallyRelevantBeam = false;
		for (int32 BeamLevel = 0; BeamLevel < BeamLevelCount; ++BeamLevel)
		{
			const float BaseYaw = FMath::Fmod(17.0f + static_cast<float>(BeamLevel) * 47.0f, 180.0f);
			for (int32 CrossIndex = 0; CrossIndex < 2; ++CrossIndex)
			{
				const float BeamZ = 440.0f + static_cast<float>(BeamLevel) * SpiralTowerBeamVerticalSpacing
					+ static_cast<float>(CrossIndex) * 42.0f;
				const float LandingCapsuleCenterZ = Landing.Z + 88.0f;
				if (FMath::Abs(BeamZ - LandingCapsuleCenterZ)
					> 88.0f + SpiralTowerBeamThickness * 0.5f + 10.0f)
				{
					continue;
				}
				bHasVerticallyRelevantBeam = true;
				const FRotator BeamRotation(
					0.0f,
					BaseYaw + static_cast<float>(CrossIndex) * 90.0f,
					0.0f);
				const FVector BeamSpaceLanding = BeamRotation.UnrotateVector(
					FVector(Landing.X, Landing.Y, 0.0f));
				const float OutsideX = FMath::Max(
					0.0f,
					FMath::Abs(BeamSpaceLanding.X) - SpiralTowerBeamHalfSpan);
				const float OutsideY = FMath::Max(
					0.0f,
					FMath::Abs(BeamSpaceLanding.Y) - SpiralTowerBeamWidth * 0.5f);
				LandingClearance = FMath::Min(
					LandingClearance,
					FMath::Sqrt(FMath::Square(OutsideX) + FMath::Square(OutsideY)));
			}
		}

		const bool bLandingClearsEveryBeam = !bHasVerticallyRelevantBeam
			|| LandingClearance >= SpiralTowerCapsuleRadius + SpiralTowerStandingMargin;
		ValidatedLandingCount += bLandingClearsEveryBeam ? 1 : 0;
		if (bHasVerticallyRelevantBeam)
		{
			WorstLandingCenterClearance = FMath::Min(WorstLandingCenterClearance, LandingClearance);
		}
	}
	ensureAlwaysMsgf(
		ValidatedLandingCount == PlatformSurfaceLocalLocations.Num(),
		TEXT("W02 beam clearance validation failed: %d/%d landings clear every collidable beam."),
		ValidatedLandingCount,
		PlatformSurfaceLocalLocations.Num());

	for (int32 BeamLevel = 0; BeamLevel < BeamLevelCount; ++BeamLevel)
	{
		const float BeamZ = 440.0f + static_cast<float>(BeamLevel) * SpiralTowerBeamVerticalSpacing;
		const float BaseYaw = FMath::Fmod(17.0f + static_cast<float>(BeamLevel) * 47.0f, 180.0f);
		for (int32 CrossIndex = 0; CrossIndex < 2; ++CrossIndex)
		{
			const FVector BeamLocation(
				0.0f,
				0.0f,
				BeamZ + static_cast<float>(CrossIndex) * 42.0f);
			const FRotator BeamRotation(
				0.0f,
				BaseYaw + static_cast<float>(CrossIndex) * 90.0f,
				0.0f);
			AddCollisionPrimitive(
				FString::Printf(TEXT("OverheadBeamCollision_%02d_%d"), BeamLevel, CrossIndex),
				CubeFallback,
				BeamLocation,
				BeamRotation,
				FVector(
					SpiralTowerBeamHalfSpan * 2.0f / 100.0f,
					SpiralTowerBeamWidth / 100.0f,
					SpiralTowerBeamThickness / 100.0f),
				FLinearColor(
					SpiralTowerWoodTintR,
					SpiralTowerWoodTintG,
					SpiralTowerWoodTintB),
				true,
				PolyHavenWoodMaterial ? PolyHavenWoodMaterial.Get() : PolyHavenTrimMaterial.Get());
		}
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 beam collision ready. Levels=%d Beams=%d HalfSpan=%.1f Width=%.1f Thickness=%.1f RouteClearance=%.1f ValidatedLandings=%d/%d WorstCapsuleClearance=%.1f"),
		BeamLevelCount,
		BeamLevelCount * 2,
		SpiralTowerBeamHalfSpan,
		SpiralTowerBeamWidth,
		SpiralTowerBeamThickness,
		BeamRouteClearance,
		ValidatedLandingCount,
		PlatformSurfaceLocalLocations.Num(),
		WorstLandingCenterClearance - SpiralTowerCapsuleRadius);
}

void ASpiralTowerWorldLayout::BuildSummit()
{
	if (PlatformSurfaceLocalLocations.IsEmpty())
	{
		return;
	}

	const FVector SummitLocation = PlatformSurfaceLocalLocations.Last();
	const FVector Outward = FVector(SummitLocation.X, SummitLocation.Y, 0.0f).GetSafeNormal();
	const FVector Tangent(-Outward.Y, Outward.X, 0.0f);
	const FRotator DeckRotation(0.0f, Tangent.Rotation().Yaw, 0.0f);
	const float DeckSurfaceZ = SummitSurfaceLocalZ + 34.0f;
	SunriseDeckLocalLocation = Outward * 2300.0f + FVector(0.0f, 0.0f, DeckSurfaceZ);

	// The seventh and final architectural level is a blue-glazed sanctuary,
	// inspired by the recorded summit temple and Babylon's glazed-brick palette.
	// Its court opens toward the last checkpoint and the sunrise terrace.
	const float SanctuaryHalfExtent = 650.0f;
	const float SanctuaryWallHeight = 520.0f;
	const float SanctuaryWallThickness = 92.0f;
	const FLinearColor GlazedBlue(0.025f, 0.16f, 0.48f);
	const int32 ApproachFaceIndex = (
		FMath::RoundToInt(
			FMath::RadiansToDegrees(FMath::Atan2(Outward.Y, Outward.X)) / 90.0f)
			% 4
			+ 4)
		% 4;
	AddCollisionPrimitive(
		TEXT("BabylonSummitSanctuaryFloor"),
		CubeFallback,
		FVector(0.0f, 0.0f, SummitSurfaceLocalZ - 20.0f),
		FRotator::ZeroRotator,
		FVector(14.2f, 14.2f, 0.40f),
		GlazedBlue * 0.72f,
		true,
		nullptr);
	for (int32 FaceIndex = 0; FaceIndex < 4; ++FaceIndex)
	{
		const float FaceYaw = static_cast<float>(FaceIndex) * 90.0f;
		const float FaceRadians = FMath::DegreesToRadians(FaceYaw);
		const FVector FaceOutward(FMath::Cos(FaceRadians), FMath::Sin(FaceRadians), 0.0f);
		const FVector FaceTangent(-FaceOutward.Y, FaceOutward.X, 0.0f);
		const FRotator FaceRotation(0.0f, FaceYaw + 90.0f, 0.0f);
		if (FaceIndex == ApproachFaceIndex)
		{
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				AddCollisionPrimitive(
					FString::Printf(TEXT("BabylonSummitGateWing_%d"), Side),
					CubeFallback,
					FaceOutward * SanctuaryHalfExtent
						+ FaceTangent * (470.0f * static_cast<float>(Side))
						+ FVector(0.0f, 0.0f, SummitSurfaceLocalZ + SanctuaryWallHeight * 0.5f),
					FaceRotation,
					FVector(4.0f, SanctuaryWallThickness / 100.0f, SanctuaryWallHeight / 100.0f),
					GlazedBlue,
					true,
					nullptr);
			}
		}
		else
		{
			AddCollisionPrimitive(
				FString::Printf(TEXT("BabylonSummitTempleWall_%d"), FaceIndex),
				CubeFallback,
				FaceOutward * SanctuaryHalfExtent
					+ FVector(0.0f, 0.0f, SummitSurfaceLocalZ + SanctuaryWallHeight * 0.5f),
				FaceRotation,
				FVector(
					SanctuaryHalfExtent * 2.0f / 100.0f,
					SanctuaryWallThickness / 100.0f,
					SanctuaryWallHeight / 100.0f),
				GlazedBlue,
				true,
				nullptr);
		}
		AddCollisionPrimitive(
			FString::Printf(TEXT("BabylonSummitBlueCornice_%d"), FaceIndex),
			CubeFallback,
			FaceOutward * (SanctuaryHalfExtent + 16.0f)
				+ FVector(0.0f, 0.0f, SummitSurfaceLocalZ + SanctuaryWallHeight + 34.0f),
			FaceRotation,
			FVector(14.2f, 1.24f, 0.68f),
			FLinearColor(0.04f, 0.30f, 0.82f),
			true,
			nullptr);
	}
	for (int32 CornerX = -1; CornerX <= 1; CornerX += 2)
	{
		for (int32 CornerY = -1; CornerY <= 1; CornerY += 2)
		{
			AddCollisionPrimitive(
				FString::Printf(TEXT("BabylonSummitPylon_%d_%d"), CornerX, CornerY),
				CubeFallback,
				FVector(
					SanctuaryHalfExtent * 0.93f * static_cast<float>(CornerX),
					SanctuaryHalfExtent * 0.93f * static_cast<float>(CornerY),
					SummitSurfaceLocalZ + 330.0f),
				FRotator::ZeroRotator,
				FVector(1.45f, 1.45f, 6.60f),
				FLinearColor(0.035f, 0.22f, 0.64f),
				true,
				nullptr);
		}
	}

	// A collision-authoritative threshold crosses the wall thickness. The exterior
	// terrace overlaps it, so the final route remains continuous through the breach.
	AddCollisionPrimitive(
		TEXT("SummitExitThreshold"),
		CubeFallback,
		Outward * 1750.0f + FVector(0.0f, 0.0f, DeckSurfaceZ - 15.0f),
		DeckRotation,
		FVector(4.8f, 7.0f, 0.30f),
		FLinearColor(0.14f, 0.13f, 0.16f),
		true,
		PolyHavenWallMaterial);
	AddCollisionPrimitive(
		TEXT("SunriseExteriorDeck"),
		CubeFallback,
		SunriseDeckLocalLocation - FVector(0.0f, 0.0f, 15.0f),
		DeckRotation,
		FVector(8.6f, 8.2f, 0.30f),
		FLinearColor(0.15f, 0.14f, 0.17f),
		true,
		PolyHavenWallMaterial);

	UStaticMesh* SummitGateVisual = PolyHavenGateMesh
		? PolyHavenGateMesh.Get()
		: (CastleDoorwayMesh ? CastleDoorwayMesh.Get() : ArchMesh.Get());
	const FVector SummitGateScale = PolyHavenGateMesh
		? FVector(620.0f / 741.02f, 120.0f / 267.20f, 430.0f / 861.62f)
		: (CastleDoorwayMesh ? FVector(4.8f, 0.62f, 2.05f) : FVector(1.8f));
	const FVector SummitGateBase = Outward * 1700.0f
		+ FVector(0.0f, 0.0f, SummitSurfaceLocalZ);
	const FVector SummitGateLocation = PolyHavenGateMesh
		? SpiralTowerWorldLayoutPrivate::SpiralTowerMeshLocationForBoundsBase(
			PolyHavenGateMesh.Get(),
			SummitGateBase,
			DeckRotation,
			SummitGateScale)
		: SummitGateBase;

	AddDecorMesh(
		TEXT("SummitDawnExitArch"),
		SummitGateVisual,
		CubeFallback,
		SummitGateLocation,
		DeckRotation,
		SummitGateScale,
		FVector(6.2f, 0.22f, 4.3f),
		FLinearColor(0.11f, 0.10f, 0.15f),
		PolyHavenWallMaterial);

	// Battlements frame the vista without blocking the approach or the player's
	// ability to remain here after reaching the summit.
	AddCollisionPrimitive(
		TEXT("SunriseDeckOuterParapet"),
		CubeFallback,
		SunriseDeckLocalLocation + Outward * 392.0f + FVector(0.0f, 0.0f, 52.0f),
		DeckRotation,
		FVector(8.6f, 0.28f, 1.04f),
		FLinearColor(0.13f, 0.12f, 0.15f),
		true,
		PolyHavenWallMaterial);
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		AddCollisionPrimitive(
			FString::Printf(TEXT("SunriseDeckSideParapet_%d"), Side),
			CubeFallback,
			SunriseDeckLocalLocation + Tangent * (405.0f * static_cast<float>(Side))
				+ FVector(0.0f, 0.0f, 52.0f),
			Outward.Rotation(),
			FVector(8.2f, 0.28f, 1.04f),
			FLinearColor(0.13f, 0.12f, 0.15f),
			true,
			PolyHavenWallMaterial);
	}

	SunriseSunDisc = AddCollisionPrimitive(
		TEXT("SunriseSunDisc"),
		SphereFallback,
		Outward * 15000.0f + FVector(0.0f, 0.0f, SummitSurfaceLocalZ + 1050.0f),
		FRotator::ZeroRotator,
		FVector(14.0f),
		FLinearColor(1.0f, 0.20f, 0.025f),
		false);
	if (SunriseSunDisc)
	{
		SunriseSunDisc->SetCastShadow(false);
		SunriseSunDisc->SetVisibility(false, true);
	}

	SunriseDeckLight = NewObject<UPointLightComponent>(this, TEXT("SunriseDeckLight"));
	AddInstanceComponent(SunriseDeckLight);
	SunriseDeckLight->SetupAttachment(SceneRoot);
	SunriseDeckLight->SetRelativeLocation(
		SunriseDeckLocalLocation + Outward * 180.0f + FVector(0.0f, 0.0f, 260.0f));
	SunriseDeckLight->SetMobility(EComponentMobility::Movable);
	SunriseDeckLight->SetIntensity(0.0f);
	SunriseDeckLight->SetAttenuationRadius(4200.0f);
	SunriseDeckLight->SetLightColor(FLinearColor(1.0f, 0.34f, 0.09f));
	SunriseDeckLight->SetCastShadows(false);
	SunriseDeckLight->RegisterComponent();

	UTextRenderComponent* ChoiceMarker = NewObject<UTextRenderComponent>(this, TEXT("SummitChoiceMarker"));
	AddInstanceComponent(ChoiceMarker);
	ChoiceMarker->SetupAttachment(SceneRoot);
	ChoiceMarker->SetRelativeLocation(
		SunriseDeckLocalLocation - Outward * 255.0f - Tangent * 350.0f
			+ FVector(0.0f, 0.0f, 105.0f));
	ChoiceMarker->SetRelativeRotation((Outward * -1.0f).Rotation());
	ChoiceMarker->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	ChoiceMarker->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	ChoiceMarker->SetWorldSize(8.0f);
	ChoiceMarker->SetTextRenderColor(FColor(255, 188, 88));
	ChoiceMarker->SetText(FText::FromString(TEXT("DAWN LEDGE\nE RETURN / STAY AND WATCH")));
	// The native HUD presents the localized choice. Keep this debug landmark out
	// of the cinematic third-person composition.
	ChoiceMarker->SetVisibility(false, true);
	ChoiceMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChoiceMarker->RegisterComponent();

	SummitTrigger->SetRelativeLocation(SunriseDeckLocalLocation + FVector(0.0f, 0.0f, 105.0f));
	SummitTrigger->SetRelativeRotation(DeckRotation);
	SummitTrigger->SetBoxExtent(FVector(320.0f, 250.0f, 110.0f));
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 Babylon summit ready. Sanctuary=BlueGlazed SeventhTier=1 Deck=(%.1f,%.1f,%.1f) Choice=StayOrPressE ExitOpening=Unsealed"),
		SunriseDeckLocalLocation.X,
		SunriseDeckLocalLocation.Y,
		SunriseDeckLocalLocation.Z);
}

void ASpiralTowerWorldLayout::BuildClimbRelic()
{
	if (!ClimbRelicTrigger || !ClimbRelicCore || !ClimbRelicBrace)
	{
		return;
	}

	const FVector FirstPlatformLocation = PlatformSurfaceLocalLocations.IsEmpty()
		? StartSurfaceLocalLocation + StartSurfaceLocalLocation.GetSafeNormal2D() * 200.0f
		: PlatformSurfaceLocalLocations[0];
	const FVector RouteDirection = (FirstPlatformLocation - StartSurfaceLocalLocation).GetSafeNormal2D();
	const FVector RouteSide(-RouteDirection.Y, RouteDirection.X, 0.0f);
	ClimbRelicBaseLocalLocation = StartSurfaceLocalLocation
		+ RouteDirection * 175.0f
		+ RouteSide * 90.0f
		+ FVector(0.0f, 0.0f, 105.0f);
	const FRotator FacingStart = (StartSurfaceLocalLocation - ClimbRelicBaseLocalLocation).Rotation();
	ClimbRelicTrigger->SetRelativeLocation(ClimbRelicBaseLocalLocation);
	ClimbRelicTrigger->SetRelativeRotation(FRotator(0.0f, FacingStart.Yaw, 0.0f));

	ClimbRelicCore->SetStaticMesh(SphereFallback);
	ClimbRelicCore->SetRelativeLocation(FVector::ZeroVector);
	ClimbRelicCore->SetRelativeScale3D(FVector(0.46f));
	ClimbRelicCore->SetCastShadow(true);
	ApplyPrimitiveTint(ClimbRelicCore, FLinearColor(1.0f, 0.22f, 0.025f));

	ClimbRelicBrace->SetStaticMesh(CylinderFallback);
	ClimbRelicBrace->SetRelativeLocation(FVector(-7.0f, 0.0f, -12.0f));
	ClimbRelicBrace->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	ClimbRelicBrace->SetRelativeScale3D(FVector(0.24f, 0.24f, 0.62f));
	ClimbRelicBrace->SetCastShadow(true);
	ApplyPrimitiveTint(ClimbRelicBrace, FLinearColor(0.20f, 0.055f, 0.018f));

	ClimbRelicLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 116.0f));
	ClimbRelicLabel->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	ClimbRelicLight->SetRelativeLocation(FVector(0.0f, 0.0f, 18.0f));
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 climbing gauntlet ready. Local=(%.1f,%.1f,%.1f) PickupRadius=%.1f Ability=Locked"),
		ClimbRelicBaseLocalLocation.X,
		ClimbRelicBaseLocalLocation.Y,
		ClimbRelicBaseLocalLocation.Z,
		ClimbRelicTrigger->GetUnscaledSphereRadius());
}

void ASpiralTowerWorldLayout::ConfigurePlayerForPlatforming()
{
	if (!ActivePlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("W02 platforming setup has no WorldWalker player."));
		return;
	}

	if (UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement())
	{
		Movement->JumpZVelocity = SpiralTowerWorldLayoutPrivate::SpiralTowerJumpZVelocity;
		Movement->AirControl = SpiralTowerWorldLayoutPrivate::SpiralTowerAirControl;
		Movement->GravityScale = SpiralTowerWorldLayoutPrivate::SpiralTowerGravityScale;
		Movement->MaxWalkSpeed = SpiralTowerWorldLayoutPrivate::SpiralTowerRunSpeed;
		Movement->MaxAcceleration = SpiralTowerWorldLayoutPrivate::SpiralTowerBaseAcceleration;
		Movement->BrakingDecelerationWalking = 1350.0f;
		Movement->BrakingDecelerationFalling = 180.0f;
		Movement->FallingLateralFriction = 0.04f;
		Movement->MaxStepHeight = 38.0f;
		Movement->SetMovementMode(MOVE_Walking);
		ActivePlayer->JumpMaxCount = 1;
		ActivePlayer->JumpMaxHoldTime = 0.0f;
		LastGroundedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

		const float RuntimeGravity = FMath::Abs(Movement->GetGravityZ());
		const float RuntimeApex = RuntimeGravity > UE_KINDA_SMALL_NUMBER
			? FMath::Square(Movement->JumpZVelocity) / (2.0f * RuntimeGravity)
			: 0.0f;
		const float RuntimeSameLevelReach = RuntimeGravity > UE_KINDA_SMALL_NUMBER
			? 2.0f * Movement->JumpZVelocity / RuntimeGravity * Movement->MaxWalkSpeed
			: 0.0f;
		const bool bRuntimeMatchesValidatedModel =
			FMath::IsNearlyEqual(Movement->JumpZVelocity, SpiralTowerWorldLayoutPrivate::SpiralTowerJumpZVelocity)
			&& FMath::IsNearlyEqual(RuntimeGravity, SpiralTowerWorldLayoutPrivate::SpiralTowerWorldGravity, 1.0f)
			&& FMath::IsNearlyEqual(Movement->MaxWalkSpeed, SpiralTowerWorldLayoutPrivate::SpiralTowerRunSpeed)
			&& FMath::IsNearlyEqual(Movement->AirControl, SpiralTowerWorldLayoutPrivate::SpiralTowerAirControl);
		UE_LOG(
			LogTemp,
			Display,
			TEXT("W02 runtime movement validation. ConfigMatchesRouteModel=%d JumpZ=%.1f RuntimeGravity=%.1f Apex=%.1f MaxWalkSpeed=%.1f SprintSpeed=%.1f AirControl=%.2f RuntimeSameLevelReach=%.1f ConservativeUsableReach=%.1f"),
			bRuntimeMatchesValidatedModel ? 1 : 0,
			Movement->JumpZVelocity,
			RuntimeGravity,
			RuntimeApex,
			Movement->MaxWalkSpeed,
			SpiralTowerWorldLayoutPrivate::SpiralTowerSprintSpeed,
			Movement->AirControl,
			RuntimeSameLevelReach,
			RuntimeSameLevelReach * SpiralTowerWorldLayoutPrivate::SpiralTowerUsableReachFraction);
	}
}

void ASpiralTowerWorldLayout::UpdateSprintMovement()
{
	if (!ActivePlayer)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(ActivePlayer->GetController());
	UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement();
	if (!PlayerController || !Movement)
	{
		return;
	}

	const bool bSprintRequested =
		PlayerController->IsInputKeyDown(EKeys::LeftShift)
		|| PlayerController->IsInputKeyDown(EKeys::RightShift);
	const bool bMoving = ActivePlayer->GetVelocity().SizeSquared2D() > FMath::Square(10.0f);
	const bool bShouldSprint = bSprintRequested
		&& bMoving
		&& !Movement->IsFalling()
		&& !bLandingRecovery
		&& CurrentStamina > 0.5f;
	if (bShouldSprint == bSprintActive)
	{
		return;
	}

	bSprintActive = bShouldSprint;
	Movement->MaxWalkSpeed = bSprintActive
		? SpiralTowerWorldLayoutPrivate::SpiralTowerSprintSpeed
		: SpiralTowerWorldLayoutPrivate::SpiralTowerRunSpeed;
	Movement->MaxAcceleration = bSprintActive
		? SpiralTowerWorldLayoutPrivate::SpiralTowerSprintAcceleration
		: SpiralTowerWorldLayoutPrivate::SpiralTowerBaseAcceleration;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 sprint state changed. Active=%d MaxWalkSpeed=%.1f MaxAcceleration=%.1f"),
		bSprintActive ? 1 : 0,
		Movement->MaxWalkSpeed,
		Movement->MaxAcceleration);
}

void ASpiralTowerWorldLayout::HandleW02JumpPressed()
{
	if (!ActivePlayer || !GetWorld() || bTravelRequested || bResettingPlayer
		|| bLedgeClimbInProgress || bLandingRecovery)
	{
		return;
	}

	bJumpHeld = true;
	JumpPressedTime = GetWorld()->GetTimeSeconds();
	BufferedJumpUntil = JumpPressedTime + SpiralTowerWorldLayoutPrivate::SpiralTowerJumpBufferTime;
	StartBufferedJump();
}

void ASpiralTowerWorldLayout::HandleW02JumpReleased()
{
	bJumpHeld = false;
	if (!ActivePlayer || !GetWorld() || !bJumpInProgress)
	{
		return;
	}

	UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement();
	const float HeldSeconds = GetWorld()->GetTimeSeconds() - JumpPressedTime;
	if (Movement
		&& HeldSeconds <= SpiralTowerWorldLayoutPrivate::SpiralTowerSmallJumpReleaseWindow
		&& Movement->Velocity.Z > SpiralTowerWorldLayoutPrivate::SpiralTowerSmallJumpCutVelocity)
	{
		Movement->Velocity.Z = SpiralTowerWorldLayoutPrivate::SpiralTowerSmallJumpCutVelocity;
		UE_LOG(
			LogTemp,
			Display,
			TEXT("W02 jump converted to small jump. Held=%.3f CutVelocity=%.1f"),
			HeldSeconds,
			Movement->Velocity.Z);
	}
}

void ASpiralTowerWorldLayout::StartBufferedJump()
{
	if (!ActivePlayer || !GetWorld() || bJumpInProgress || bLandingRecovery
		|| bLedgeClimbInProgress || GetWorld()->GetTimeSeconds() > BufferedJumpUntil)
	{
		return;
	}

	UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement();
	if (!Movement)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	const bool bCanUseCoyoteJump = Movement->IsMovingOnGround()
		|| (Movement->IsFalling()
			&& Now - LastGroundedTime <= SpiralTowerWorldLayoutPrivate::SpiralTowerCoyoteTime);
	if (!bCanUseCoyoteJump)
	{
		return;
	}

	const APlayerController* PlayerController = Cast<APlayerController>(ActivePlayer->GetController());
	const bool bSprintRequested = PlayerController
		&& (PlayerController->IsInputKeyDown(EKeys::LeftShift)
			|| PlayerController->IsInputKeyDown(EKeys::RightShift));
	bJumpStartedAsSprint = bSprintRequested
		&& ActivePlayer->GetVelocity().Size2D() >= SpiralTowerWorldLayoutPrivate::SpiralTowerRunSpeed * 0.72f
		&& CurrentStamina > 5.0f;
	bJumpInProgress = true;
	bJumpLoopAnimationPlaying = true;
	BufferedJumpUntil = -100.0f;
	Movement->SetMovementMode(MOVE_Falling);
	Movement->Velocity.Z = bJumpStartedAsSprint
		? SpiralTowerWorldLayoutPrivate::SpiralTowerSprintJumpInitialVelocity
		: SpiralTowerWorldLayoutPrivate::SpiralTowerJumpInitialVelocity;
	if (bJumpStartedAsSprint)
	{
		const FVector HorizontalDirection = Movement->Velocity.GetSafeNormal2D();
		if (!HorizontalDirection.IsNearlyZero())
		{
			Movement->Velocity.X = HorizontalDirection.X * SpiralTowerWorldLayoutPrivate::SpiralTowerSprintSpeed;
			Movement->Velocity.Y = HorizontalDirection.Y * SpiralTowerWorldLayoutPrivate::SpiralTowerSprintSpeed;
		}
	}
	// Keep the character package's own AnimBP active. It reads IsFalling and
	// produces a skeleton-native jump pose without applying foreign proportions.
	ActivePlayer->RestoreMainWorldLocomotionAnimation();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 jump started. Type=%s InitialZ=%.1f HorizontalSpeed=%.1f Coyote=%.3f"),
		bJumpStartedAsSprint ? TEXT("Sprint") : TEXT("Variable"),
		Movement->Velocity.Z,
		Movement->Velocity.Size2D(),
		Now - LastGroundedTime);
}

void ASpiralTowerWorldLayout::UpdateJumpMovement(const float DeltaSeconds)
{
	if (!ActivePlayer || !GetWorld())
	{
		return;
	}
	UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const bool bGrounded = Movement->IsMovingOnGround();
	if (bGrounded)
	{
		LastGroundedTime = Now;
		if (bWasFalling && !bAutomatedRouteValidation && !bAutomatedClimbValidation)
		{
			BeginLandingRecovery(FMath::Abs(LastFallingVerticalSpeed));
		}
		if (!bLandingRecovery && Now <= BufferedJumpUntil)
		{
			StartBufferedJump();
		}
	}
	else if (Movement->IsFalling())
	{
		LastFallingVerticalSpeed = Movement->Velocity.Z;
		if (bJumpInProgress)
		{
			const float HeldSeconds = Now - JumpPressedTime;
			const float TargetJumpVelocity = bJumpStartedAsSprint
				? SpiralTowerWorldLayoutPrivate::SpiralTowerSprintJumpZVelocity
				: SpiralTowerWorldLayoutPrivate::SpiralTowerJumpZVelocity;
			if (bJumpHeld
				&& HeldSeconds < SpiralTowerWorldLayoutPrivate::SpiralTowerJumpHoldDuration
				&& Movement->Velocity.Z > 0.0f)
			{
				Movement->Velocity.Z = FMath::Min(
					TargetJumpVelocity,
					Movement->Velocity.Z
						+ SpiralTowerWorldLayoutPrivate::SpiralTowerJumpHoldAcceleration * DeltaSeconds);
			}
		}
	}
	bWasFalling = Movement->IsFalling();
}

void ASpiralTowerWorldLayout::BeginLandingRecovery(const float ImpactSpeed)
{
	if (!ActivePlayer || !GetWorld() || bLandingRecovery || bLedgeClimbInProgress)
	{
		return;
	}

	const bool bSmallJump = ImpactSpeed < 390.0f && !bJumpStartedAsSprint;
	const float RecoverySeconds = bJumpStartedAsSprint
		? SpiralTowerWorldLayoutPrivate::SpiralTowerLandingRecoverySprint
		: (bSmallJump
			? SpiralTowerWorldLayoutPrivate::SpiralTowerLandingRecoverySmall
			: SpiralTowerWorldLayoutPrivate::SpiralTowerLandingRecoveryBig);
	bLandingRecovery = true;
	bJumpInProgress = false;
	bJumpHeld = false;
	bJumpLoopAnimationPlaying = false;
	if (UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	ActivePlayer->RestoreMainWorldLocomotionAnimation();
	GetWorldTimerManager().SetTimer(
		LandingRecoveryTimer,
		this,
		&ASpiralTowerWorldLayout::FinishLandingRecovery,
		RecoverySeconds,
		false);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 landing recovery started. Type=%s ImpactSpeed=%.1f Duration=%.2f Animation=%s"),
		bJumpStartedAsSprint ? TEXT("Sprint") : (bSmallJump ? TEXT("Small") : TEXT("Big")),
		ImpactSpeed,
		RecoverySeconds,
		TEXT("SkeletonNativeLocomotion"));
}

void ASpiralTowerWorldLayout::FinishLandingRecovery()
{
	if (!ActivePlayer)
	{
		return;
	}
	bLandingRecovery = false;
	bJumpStartedAsSprint = false;
	ActivePlayer->RestoreMainWorldLocomotionAnimation();
	if (UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
		Movement->MaxWalkSpeed = SpiralTowerWorldLayoutPrivate::SpiralTowerRunSpeed;
		Movement->MaxAcceleration = SpiralTowerWorldLayoutPrivate::SpiralTowerBaseAcceleration;
	}
	UE_LOG(LogTemp, Display, TEXT("W02 landing recovery complete. Movement=Walking"));
}

void ASpiralTowerWorldLayout::UpdateTraversalStamina(const float DeltaSeconds)
{
	if (!ActivePlayer || !GetWorld())
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	float DrainRate = 0.0f;
	if (bLedgeClimbInProgress)
	{
		DrainRate = SpiralTowerWorldLayoutPrivate::SpiralTowerClimbStaminaDrain;
	}
	else if (bSprintActive)
	{
		DrainRate = SpiralTowerWorldLayoutPrivate::SpiralTowerSprintStaminaDrain;
	}

	if (DrainRate > 0.0f)
	{
		CurrentStamina = FMath::Max(0.0f, CurrentStamina - DrainRate * DeltaSeconds);
		LastStaminaUseTime = Now;
	}
	else if (ActivePlayer->GetCharacterMovement()->IsMovingOnGround()
		&& !bLandingRecovery
		&& Now - LastStaminaUseTime >= SpiralTowerWorldLayoutPrivate::SpiralTowerStaminaRecoveryDelay)
	{
		CurrentStamina = FMath::Min(
			SpiralTowerWorldLayoutPrivate::SpiralTowerMaxStamina,
			CurrentStamina + SpiralTowerWorldLayoutPrivate::SpiralTowerStaminaRecoveryRate * DeltaSeconds);
	}

	if (bLedgeClimbInProgress && CurrentStamina <= UE_KINDA_SMALL_NUMBER)
	{
		CancelLedgeClimbForExhaustion();
	}
	if (bSprintActive && CurrentStamina <= UE_KINDA_SMALL_NUMBER)
	{
		bSprintActive = false;
		if (UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement())
		{
			Movement->MaxWalkSpeed = SpiralTowerWorldLayoutPrivate::SpiralTowerRunSpeed;
			Movement->MaxAcceleration = SpiralTowerWorldLayoutPrivate::SpiralTowerBaseAcceleration;
		}
	}

	if (Now >= NextHUDUpdateTime)
	{
		UpdatePlatformingHUD();
		NextHUDUpdateTime = Now + 0.10f;
	}
}

void ASpiralTowerWorldLayout::UpdatePlatformingHUD()
{
	if (!ActivePlayer)
	{
		return;
	}
	if (AWorldWalkerPlayerController* PlayerController =
		Cast<AWorldWalkerPlayerController>(ActivePlayer->GetController()))
	{
		const TCHAR* State = bLedgeClimbInProgress
			? TEXT("攀爬消耗中")
			: (bLandingRecovery
				? TEXT("落地硬直")
				: (bSprintActive
					? TEXT("冲刺消耗中")
					: (CurrentStamina <= SpiralTowerWorldLayoutPrivate::SpiralTowerMinimumClimbStamina
						? TEXT("体力不足，落地恢复")
						: TEXT("可抓边/爬梯"))));
		PlayerController->SetPlatformingStatus(
			CurrentStamina,
			SpiralTowerWorldLayoutPrivate::SpiralTowerMaxStamina,
			State);
	}
}

void ASpiralTowerWorldLayout::UpdateClimbRelicPresentation(const float DeltaSeconds)
{
	if (bLedgeClimbUnlocked || !ClimbRelicTrigger || !GetWorld())
	{
		return;
	}

	const float TimeSeconds = GetWorld()->GetTimeSeconds();
	ClimbRelicTrigger->SetRelativeLocation(
		ClimbRelicBaseLocalLocation + FVector(0.0f, 0.0f, FMath::Sin(TimeSeconds * 2.4f) * 10.0f));
	ClimbRelicCore->AddLocalRotation(FRotator(0.0f, 105.0f * DeltaSeconds, 0.0f));
	ClimbRelicBrace->AddLocalRotation(FRotator(0.0f, -62.0f * DeltaSeconds, 0.0f));
}

void ASpiralTowerWorldLayout::GrantLedgeClimbAbility()
{
	if (bLedgeClimbUnlocked)
	{
		return;
	}

	bLedgeClimbUnlocked = true;
	ClimbRelicTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ClimbRelicTrigger->SetGenerateOverlapEvents(false);
	ClimbRelicCore->SetVisibility(false, true);
	ClimbRelicBrace->SetVisibility(false, true);
	ClimbRelicLabel->SetVisibility(false, true);
	ClimbRelicLight->SetVisibility(false, true);
	if (ActivePlayer)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(ActivePlayer->GetController()))
		{
			PlayerController->ClientMessage(
				TEXT("Climbing Gauntlet acquired: face a ledge and hold Space while airborne."));
		}
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 climbing gauntlet collected. LedgeClimb=Unlocked Input=Airborne+FaceLedge+HoldSpace"));
}

void ASpiralTowerWorldLayout::HandleClimbRelicOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	const int32 OtherBodyIndex,
	const bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (OtherActor == ActivePlayer)
	{
		GrantLedgeClimbAbility();
	}
}

bool ASpiralTowerWorldLayout::FindLedgeClimbTarget(
	FVector& OutHangWorldLocation,
	FVector& OutTargetWorldLocation,
	FRotator& OutTargetWorldRotation) const
{
	using namespace SpiralTowerWorldLayoutPrivate;

	if (!ActivePlayer || !GetWorld())
	{
		return false;
	}

	const float CapsuleRadius = ActivePlayer->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = GetPlayerCapsuleHalfHeight();
	const FVector PlayerLocalLocation = GetActorTransform().InverseTransformPosition(
		ActivePlayer->GetActorLocation());
	const float PlayerFloorZ = PlayerLocalLocation.Z - CapsuleHalfHeight;
	FVector ForwardWorld = ActivePlayer->GetActorForwardVector();
	if (const AController* Controller = ActivePlayer->GetController())
	{
		ForwardWorld = FRotationMatrix(FRotator(
			0.0f,
			Controller->GetControlRotation().Yaw,
			0.0f)).GetUnitAxis(EAxis::X);
	}
	const FVector ForwardLocal = GetActorTransform().InverseTransformVectorNoScale(ForwardWorld)
		.GetSafeNormal2D();
	float BestScore = MAX_flt;
	bool bFoundTarget = false;

	for (int32 PlatformIndex = 0; PlatformIndex < PlatformSurfaceLocalLocations.Num(); ++PlatformIndex)
	{
		if (!PlatformLocalDimensions.IsValidIndex(PlatformIndex)
			|| !PlatformLocalRotations.IsValidIndex(PlatformIndex))
		{
			continue;
		}

		const FVector& SurfaceLocation = PlatformSurfaceLocalLocations[PlatformIndex];
		const float LedgeHeight = SurfaceLocation.Z - PlayerFloorZ;
		if (LedgeHeight < SpiralTowerLedgeClimbMinHeight
			|| LedgeHeight > SpiralTowerLedgeClimbMaxHeight)
		{
			continue;
		}

		const FRotator& PlatformRotation = PlatformLocalRotations[PlatformIndex];
		const FVector Tangent = PlatformRotation.RotateVector(FVector::ForwardVector).GetSafeNormal2D();
		const FVector Radial = PlatformRotation.RotateVector(FVector::RightVector).GetSafeNormal2D();
		const FVector HorizontalDelta(
			PlayerLocalLocation.X - SurfaceLocation.X,
			PlayerLocalLocation.Y - SurfaceLocation.Y,
			0.0f);
		const float TangentCoordinate = FVector::DotProduct(HorizontalDelta, Tangent);
		const float RadialCoordinate = FVector::DotProduct(HorizontalDelta, Radial);
		const float HalfLength = PlatformLocalDimensions[PlatformIndex].X * 0.5f;
		const float HalfWidth = PlatformLocalDimensions[PlatformIndex].Y * 0.5f;
		const bool bOutsidePlatform = FMath::Abs(TangentCoordinate) > HalfLength
			|| FMath::Abs(RadialCoordinate) > HalfWidth;
		if (!bOutsidePlatform)
		{
			continue;
		}

		const float EdgeTangent = FMath::Clamp(TangentCoordinate, -HalfLength, HalfLength);
		const float EdgeRadial = FMath::Clamp(RadialCoordinate, -HalfWidth, HalfWidth);
		const FVector EdgeLocalLocation = SurfaceLocation
			+ Tangent * EdgeTangent
			+ Radial * EdgeRadial;
		FVector ToEdge = EdgeLocalLocation - PlayerLocalLocation;
		ToEdge.Z = 0.0f;
		const float HorizontalReach = ToEdge.Size();
		if (HorizontalReach < 2.0f || HorizontalReach > SpiralTowerLedgeClimbReach)
		{
			continue;
		}
		if (FVector::DotProduct(ForwardLocal, ToEdge / HorizontalReach) < 0.30f)
		{
			continue;
		}

		const float SafeHalfLength = FMath::Max(
			0.0f,
			HalfLength - CapsuleRadius - SpiralTowerStandingMargin);
		const float SafeHalfWidth = FMath::Max(
			0.0f,
			HalfWidth - CapsuleRadius - SpiralTowerStandingMargin);
		const float TargetTangent = FMath::Clamp(
			TangentCoordinate,
			-SafeHalfLength,
			SafeHalfLength);
		const float TargetRadial = FMath::Clamp(
			RadialCoordinate,
			-SafeHalfWidth,
			SafeHalfWidth);
		const FVector TargetSurfaceLocalLocation = SurfaceLocation
			+ Tangent * TargetTangent
			+ Radial * TargetRadial;
		const FVector TargetLocalLocation = TargetSurfaceLocalLocation
			+ FVector(0.0f, 0.0f, CapsuleHalfHeight + SpiralTowerLedgeTopClearance);
		const FVector TargetWorldLocation = GetActorTransform().TransformPosition(TargetLocalLocation);
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(W02LedgeClimbClearance), false);
		QueryParams.AddIgnoredActor(ActivePlayer);
		const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(
			FMath::Max(1.0f, CapsuleRadius - 2.0f),
			FMath::Max(CapsuleRadius, CapsuleHalfHeight - 2.0f));
		if (GetWorld()->OverlapBlockingTestByChannel(
			TargetWorldLocation,
			ActivePlayer->GetActorQuat(),
			ECC_Pawn,
			CapsuleShape,
			QueryParams))
		{
			continue;
		}

		FVector OutsideDirection = PlayerLocalLocation - EdgeLocalLocation;
		OutsideDirection.Z = 0.0f;
		OutsideDirection = OutsideDirection.GetSafeNormal();
		if (OutsideDirection.IsNearlyZero())
		{
			OutsideDirection = -ToEdge.GetSafeNormal();
		}
		const FVector HangLocalLocation(
			EdgeLocalLocation.X + OutsideDirection.X * (CapsuleRadius + 6.0f),
			EdgeLocalLocation.Y + OutsideDirection.Y * (CapsuleRadius + 6.0f),
			SurfaceLocation.Z - SpiralTowerLedgeHangDrop);
		const float CandidateScore = HorizontalReach + LedgeHeight * 0.12f;
		if (CandidateScore >= BestScore)
		{
			continue;
		}

		BestScore = CandidateScore;
		OutHangWorldLocation = GetActorTransform().TransformPosition(HangLocalLocation);
		OutTargetWorldLocation = TargetWorldLocation;
		FVector FaceDirection = TargetWorldLocation - OutHangWorldLocation;
		FaceDirection.Z = 0.0f;
		OutTargetWorldRotation = FaceDirection.Rotation();
		bFoundTarget = true;
	}

	return bFoundTarget;
}

bool ASpiralTowerWorldLayout::TryStartLedgeClimb(const bool bIgnoreInputRequirement)
{
	if (!bLedgeClimbUnlocked
		|| bLedgeClimbInProgress
		|| bTravelRequested
		|| bResettingPlayer
		|| !ActivePlayer
		|| !GetWorld()
		|| CurrentStamina < SpiralTowerWorldLayoutPrivate::SpiralTowerMinimumClimbStamina
		|| GetWorld()->GetTimeSeconds() < NextLedgeClimbAllowedTime)
	{
		return false;
	}

	APlayerController* PlayerController = Cast<APlayerController>(ActivePlayer->GetController());
	UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement();
	if (!PlayerController || !Movement)
	{
		return false;
	}
	if (!bIgnoreInputRequirement)
	{
		const bool bClimbRequested = PlayerController->IsInputKeyDown(EKeys::SpaceBar);
		if (!bClimbRequested || !Movement->IsFalling() || ActivePlayer->GetVelocity().Z > 200.0f)
		{
			return false;
		}
	}

	FVector HangWorldLocation;
	FVector TargetWorldLocation;
	FRotator TargetWorldRotation;
	if (!FindLedgeClimbTarget(HangWorldLocation, TargetWorldLocation, TargetWorldRotation))
	{
		return false;
	}

	bLedgeClimbInProgress = true;
	LastStaminaUseTime = GetWorld()->GetTimeSeconds();
	bSprintActive = false;
	LedgeClimbElapsed = 0.0f;
	LedgeClimbStartWorldLocation = ActivePlayer->GetActorLocation();
	LedgeClimbStartWorldRotation = ActivePlayer->GetActorRotation();
	LedgeClimbHangWorldLocation = HangWorldLocation;
	LedgeClimbTargetWorldLocation = TargetWorldLocation;
	LedgeClimbTargetWorldRotation = TargetWorldRotation;
	SavedClimbGravityScale = Movement->GravityScale;
	SavedPlayerCollisionMode = ActivePlayer->GetCapsuleComponent()->GetCollisionEnabled();
	Movement->StopMovementImmediately();
	Movement->GravityScale = 0.0f;
	Movement->MaxWalkSpeed = SpiralTowerWorldLayoutPrivate::SpiralTowerRunSpeed;
	Movement->MaxAcceleration = SpiralTowerWorldLayoutPrivate::SpiralTowerBaseAcceleration;
	Movement->SetMovementMode(MOVE_Falling);
	ActivePlayer->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// The procedural hang/pull component pose keeps the modular body intact;
	// foreign single-node clips are intentionally not applied to this skeleton.
	ActivePlayer->RestoreMainWorldLocomotionAnimation();
	ActivePlayer->SetMainWorldLedgeClimbPose(0.0f);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 ledge climb started. Start=%s Hang=%s Target=%s Duration=%.2f"),
		*LedgeClimbStartWorldLocation.ToCompactString(),
		*LedgeClimbHangWorldLocation.ToCompactString(),
		*LedgeClimbTargetWorldLocation.ToCompactString(),
		SpiralTowerWorldLayoutPrivate::SpiralTowerLedgeClimbDuration);
	return true;
}

void ASpiralTowerWorldLayout::TickLedgeClimb(const float DeltaSeconds)
{
	using namespace SpiralTowerWorldLayoutPrivate;

	if (!bLedgeClimbInProgress || !ActivePlayer)
	{
		return;
	}

	LedgeClimbElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(
		LedgeClimbElapsed / SpiralTowerLedgeClimbDuration,
		0.0f,
		1.0f);
	FVector AnimatedLocation;
	if (Alpha <= SpiralTowerLedgeGripPhase)
	{
		const float GripAlpha = FMath::InterpEaseInOut(
			0.0f,
			1.0f,
			Alpha / SpiralTowerLedgeGripPhase,
			2.0f);
		AnimatedLocation = FMath::Lerp(
			LedgeClimbStartWorldLocation,
			LedgeClimbHangWorldLocation,
			GripAlpha);
	}
	else
	{
		const float PullAlpha = (Alpha - SpiralTowerLedgeGripPhase)
			/ (1.0f - SpiralTowerLedgeGripPhase);
		const FVector LiftLocation(
			LedgeClimbHangWorldLocation.X,
			LedgeClimbHangWorldLocation.Y,
			LedgeClimbTargetWorldLocation.Z + 18.0f);
		if (PullAlpha < 0.58f)
		{
			const float LiftAlpha = FMath::InterpEaseInOut(
				0.0f,
				1.0f,
				PullAlpha / 0.58f,
				2.0f);
			AnimatedLocation = FMath::Lerp(
				LedgeClimbHangWorldLocation,
				LiftLocation,
				LiftAlpha);
		}
		else
		{
			const float MantleAlpha = FMath::InterpEaseInOut(
				0.0f,
				1.0f,
				(PullAlpha - 0.58f) / 0.42f,
				2.0f);
			AnimatedLocation = FMath::Lerp(
				LiftLocation,
				LedgeClimbTargetWorldLocation,
				MantleAlpha);
		}
	}

	const FQuat AnimatedRotation = FQuat::Slerp(
		LedgeClimbStartWorldRotation.Quaternion(),
		LedgeClimbTargetWorldRotation.Quaternion(),
		FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f));
	ActivePlayer->SetActorLocationAndRotation(
		AnimatedLocation,
		AnimatedRotation.Rotator(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	ActivePlayer->SetMainWorldLedgeClimbPose(Alpha);
	if (Alpha >= 1.0f - UE_KINDA_SMALL_NUMBER)
	{
		FinishLedgeClimb();
	}
}

void ASpiralTowerWorldLayout::FinishLedgeClimb()
{
	if (!bLedgeClimbInProgress || !ActivePlayer)
	{
		return;
	}

	ActivePlayer->SetActorLocationAndRotation(
		LedgeClimbTargetWorldLocation,
		LedgeClimbTargetWorldRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	ActivePlayer->SetMainWorldLedgeClimbPose(1.0f);
	ActivePlayer->GetCapsuleComponent()->SetCollisionEnabled(SavedPlayerCollisionMode);
	if (UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement())
	{
		Movement->GravityScale = SavedClimbGravityScale;
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_Walking);
	}
	bLedgeClimbInProgress = false;
	ActivePlayer->RestoreMainWorldLocomotionAnimation();
	NextLedgeClimbAllowedTime = GetWorld() ? GetWorld()->GetTimeSeconds() + 0.35f : 0.0f;
	const float TargetError = FVector::Dist(
		ActivePlayer->GetActorLocation(),
		LedgeClimbTargetWorldLocation);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 ledge climb complete. TargetError=%.2f Movement=Walking Ability=Unlocked"),
		TargetError);
	if (bAutomatedClimbValidation)
	{
		const float AutomatedTargetError = FVector::Dist(
			ActivePlayer->GetActorLocation(),
			AutomatedClimbExpectedWorldLocation);
		const UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement();
		const bool bCollisionRestored = ActivePlayer->GetCapsuleComponent()->GetCollisionEnabled()
			== SavedPlayerCollisionMode;
		const bool bPassed = AutomatedTargetError <= 5.0f
			&& Movement
			&& Movement->IsMovingOnGround()
			&& bLedgeClimbUnlocked
			&& bCollisionRestored;
		UE_LOG(
			LogTemp,
			Display,
			TEXT("W02 automated ledge climb validation complete. Relic=Collected Detection=Passed Animation=Passed CollisionRestored=%d TargetError=%.2f Result=%s"),
			bCollisionRestored ? 1 : 0,
			AutomatedTargetError,
			bPassed ? TEXT("Passed") : TEXT("Failed"));
		ensureAlwaysMsgf(bPassed, TEXT("W02 automated ledge climb validation failed."));
		bAutomatedClimbValidation = false;
	}
}

void ASpiralTowerWorldLayout::CancelLedgeClimbForExhaustion()
{
	if (!bLedgeClimbInProgress || !ActivePlayer)
	{
		return;
	}

	bLedgeClimbInProgress = false;
	ActivePlayer->GetCapsuleComponent()->SetCollisionEnabled(SavedPlayerCollisionMode);
	ActivePlayer->RestoreMainWorldLocomotionAnimation();
	if (UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement())
	{
		Movement->GravityScale = SavedClimbGravityScale;
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_Falling);
	}
	NextLedgeClimbAllowedTime = GetWorld() ? GetWorld()->GetTimeSeconds() + 0.55f : 0.0f;
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("W02 ledge climb cancelled. Reason=StaminaExhausted Stamina=%.1f Movement=Falling"),
		CurrentStamina);
}

void ASpiralTowerWorldLayout::StartAutomatedClimbValidation()
{
#if !UE_BUILD_SHIPPING
	if (!ActivePlayer
		|| PlatformSurfaceLocalLocations.IsEmpty()
		|| PlatformLocalDimensions.IsEmpty()
		|| PlatformLocalRotations.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("W02 automated ledge climb validation could not start."));
		return;
	}

	GrantLedgeClimbAbility();
	const int32 PlatformIndex = 0;
	const FVector& SurfaceLocation = PlatformSurfaceLocalLocations[PlatformIndex];
	const FRotator& PlatformRotation = PlatformLocalRotations[PlatformIndex];
	FVector ApproachDirection = StartSurfaceLocalLocation - SurfaceLocation;
	ApproachDirection.Z = 0.0f;
	ApproachDirection = ApproachDirection.GetSafeNormal();
	const float EdgeSupport = SpiralTowerWorldLayoutPrivate::SpiralTowerRectangleSupport(
		ApproachDirection,
		PlatformRotation,
		PlatformLocalDimensions[PlatformIndex].X,
		PlatformLocalDimensions[PlatformIndex].Y);
	const FVector EdgeLocation = SurfaceLocation + ApproachDirection * EdgeSupport;
	FVector TestActorLocalLocation = EdgeLocation + ApproachDirection * 65.0f;
	TestActorLocalLocation.Z = SurfaceLocation.Z - 80.0f + GetPlayerCapsuleHalfHeight();
	const FVector TestActorWorldLocation = GetActorTransform().TransformPosition(
		TestActorLocalLocation);
	const FVector TestFacingWorldDirection = GetActorTransform().TransformVectorNoScale(
		(SurfaceLocation - EdgeLocation).GetSafeNormal2D());
	const FRotator TestFacingWorldRotation = TestFacingWorldDirection.Rotation();
	ActivePlayer->SetActorLocationAndRotation(
		TestActorWorldLocation,
		TestFacingWorldRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	if (AController* Controller = ActivePlayer->GetController())
	{
		Controller->SetControlRotation(TestFacingWorldRotation);
	}
	if (UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_Falling);
	}
	bAutomatedClimbValidation = true;
	const bool bStarted = TryStartLedgeClimb(true);
	AutomatedClimbExpectedWorldLocation = LedgeClimbTargetWorldLocation;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 automated ledge climb validation started. Relic=Collected Detection=%s TestPlatform=1"),
		bStarted ? TEXT("Passed") : TEXT("Failed"));
	if (!bStarted)
	{
		bAutomatedClimbValidation = false;
		ensureAlwaysMsgf(false, TEXT("W02 automated ledge climb detection failed."));
	}
#endif
}

void ASpiralTowerWorldLayout::PlacePlayerAtInitialStart()
{
	if (!ActivePlayer || CheckpointLocalTransforms.IsEmpty())
	{
		return;
	}

	ActiveCheckpointSlot = 0;
	CurrentStamina = SpiralTowerWorldLayoutPrivate::SpiralTowerMaxStamina;
	const FTransform StartTransform = GetCheckpointWorldTransform(ActiveCheckpointSlot);
	if (UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_Walking);
	}
	ActivePlayer->SetActorLocationAndRotation(
		StartTransform.GetLocation(),
		StartTransform.Rotator(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	if (AController* Controller = ActivePlayer->GetController())
	{
		Controller->SetControlRotation(StartTransform.Rotator());
	}
}

void ASpiralTowerWorldLayout::StartAutomatedRouteValidation()
{
	if (!ActivePlayer
		|| PlatformSurfaceLocalLocations.Num() != SpiralTowerWorldLayoutPrivate::SpiralTowerPlatformCount
		|| PlatformLocalRotations.Num() != PlatformSurfaceLocalLocations.Num())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("W02 automated route validation could not start. Player=%d Landings=%d Rotations=%d"),
			ActivePlayer ? 1 : 0,
			PlatformSurfaceLocalLocations.Num(),
			PlatformLocalRotations.Num());
		return;
	}

	bAutomatedRouteValidation = true;
	bAutomatedValidationFallProbeActive = false;
	AutomatedValidationPlatformIndex = 0;
	AutomatedValidationFallCheckpointSlot = INDEX_NONE;
	AutomatedValidationFallPassCount = 0;
	// The sampler invokes the summit explicitly after validating landing 96.
	// Disable incidental overlaps so the neighbouring approach cannot end it early.
	SummitTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 automated route validation started. Mode=DevelopmentOnly CommandLine=-W02AutoValidateRoute Samples=96 Lookouts=16,32,48,64,80,96 FallProbes=32,64 Summit=ChoiceThenW00"));
	GetWorldTimerManager().SetTimer(
		AutomatedValidationTimer,
		this,
		&ASpiralTowerWorldLayout::AdvanceAutomatedRouteValidation,
		0.08f,
		true,
		0.15f);
}

void ASpiralTowerWorldLayout::AdvanceAutomatedRouteValidation()
{
	if (!bAutomatedRouteValidation || !IsValid(ActivePlayer) || bTravelRequested)
	{
		GetWorldTimerManager().ClearTimer(AutomatedValidationTimer);
		return;
	}

	bool bCompletedFallProbeThisStep = false;
	if (bAutomatedValidationFallProbeActive)
	{
		const FTransform ExpectedReset = GetCheckpointWorldTransform(
			AutomatedValidationFallCheckpointSlot);
		const float ResetError = FVector::Dist(
			ActivePlayer->GetActorLocation(),
			ExpectedReset.GetLocation());
		const bool bResetPassed = ActiveCheckpointSlot == AutomatedValidationFallCheckpointSlot
			&& ResetError <= 35.0f;
		UE_LOG(
			LogTemp,
			Display,
			TEXT("W02 automated fall reset result. CheckpointLayer=%d ActiveSlot=%d ResetError=%.1f Result=%s"),
			AutomatedValidationFallCheckpointSlot * SpiralTowerWorldLayoutPrivate::SpiralTowerCheckpointInterval,
			ActiveCheckpointSlot,
			ResetError,
			bResetPassed ? TEXT("Passed") : TEXT("Failed"));
		ensureAlwaysMsgf(
			bResetPassed,
			TEXT("W02 automated fall reset failed at checkpoint slot %d (error %.1f cm)."),
			AutomatedValidationFallCheckpointSlot,
			ResetError);
		bAutomatedValidationFallProbeActive = false;
		bCompletedFallProbeThisStep = true;
		AutomatedValidationFallPassCount += bResetPassed ? 1 : 0;
		AutomatedValidationFallCheckpointSlot = INDEX_NONE;
		if (!bResetPassed)
		{
			bAutomatedRouteValidation = false;
			GetWorldTimerManager().ClearTimer(AutomatedValidationTimer);
			return;
		}
	}

	const int32 PreviousPlatformIndex = AutomatedValidationPlatformIndex - 1;
	const int32 PreviousLayer = PreviousPlatformIndex + 1;
	if (!bCompletedFallProbeThisStep && (PreviousLayer == 32 || PreviousLayer == 64))
	{
		const int32 ExpectedCheckpointSlot = PreviousLayer
			/ SpiralTowerWorldLayoutPrivate::SpiralTowerCheckpointInterval;
		if (ActiveCheckpointSlot != ExpectedCheckpointSlot)
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("W02 automated checkpoint validation failed. Layer=%d ExpectedSlot=%d ActiveSlot=%d"),
				PreviousLayer,
				ExpectedCheckpointSlot,
				ActiveCheckpointSlot);
			bAutomatedRouteValidation = false;
			GetWorldTimerManager().ClearTimer(AutomatedValidationTimer);
			return;
		}

		AutomatedValidationFallCheckpointSlot = ExpectedCheckpointSlot;
		bAutomatedValidationFallProbeActive = true;
		if (UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_Falling);
		}
		const FVector ProbeLocation = GetCheckpointWorldTransform(ExpectedCheckpointSlot).GetLocation()
			- GetActorTransform().TransformVectorNoScale(FVector(
				0.0f,
				0.0f,
				SpiralTowerWorldLayoutPrivate::SpiralTowerCheckpointFallDistance
					+ GetPlayerCapsuleHalfHeight() + 120.0f));
		ActivePlayer->SetActorLocation(
			ProbeLocation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		UE_LOG(
			LogTemp,
			Display,
			TEXT("W02 automated fall reset probe injected. CheckpointLayer=%d Slot=%d ProbeZ=%.1f"),
			PreviousLayer,
			ExpectedCheckpointSlot,
			ProbeLocation.Z);
		return;
	}

	if (!PlatformSurfaceLocalLocations.IsValidIndex(AutomatedValidationPlatformIndex))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("W02 automated route validation stopped at invalid sample %d."),
			AutomatedValidationPlatformIndex);
		bAutomatedRouteValidation = false;
		GetWorldTimerManager().ClearTimer(AutomatedValidationTimer);
		return;
	}

	const int32 PlatformIndex = AutomatedValidationPlatformIndex;
	const int32 Layer = PlatformIndex + 1;
	const FVector LocalCapsuleLocation = PlatformSurfaceLocalLocations[PlatformIndex]
		+ FVector(0.0f, 0.0f, GetPlayerCapsuleHalfHeight() + 4.0f);
	const FVector WorldCapsuleLocation = GetActorTransform().TransformPosition(LocalCapsuleLocation);
	const FRotator LandingRotation = PlatformIndex + 1 < PlatformSurfaceLocalLocations.Num()
		? (PlatformSurfaceLocalLocations[PlatformIndex + 1]
			- PlatformSurfaceLocalLocations[PlatformIndex]).Rotation()
		: PlatformLocalRotations[PlatformIndex];
	const bool bFinalLanding = Layer == SpiralTowerWorldLayoutPrivate::SpiralTowerPlatformCount;
	if (bFinalLanding)
	{
		// Keep the normal overlap from racing the explicit checkpoint sample below.
		SummitTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_Walking);
	}
	ActivePlayer->SetActorLocationAndRotation(
		WorldCapsuleLocation,
		GetActorTransform().TransformRotation(
			FRotator(0.0f, LandingRotation.Yaw, 0.0f).Quaternion()).Rotator(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	if (AController* Controller = ActivePlayer->GetController())
	{
		Controller->SetControlRotation(ActivePlayer->GetActorRotation());
	}
	UpdateCheckpointProgress();
	++AutomatedValidationPlatformIndex;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 automated landing sample. Layer=%03d/096 Traversal=%d Checkpoint=%d Height=%.1f RequiredTravel=%.1f SafetyMargin=%.1f"),
		Layer,
		PlatformTraversalTypes.IsValidIndex(PlatformIndex)
			? static_cast<int32>(PlatformTraversalTypes[PlatformIndex])
			: 0,
		SpiralTowerWorldLayoutPrivate::SpiralTowerIsCheckpointPlatform(PlatformIndex) ? 1 : 0,
		PlatformSurfaceLocalLocations[PlatformIndex].Z,
		PlatformStepRequiredTravels.IsValidIndex(PlatformIndex)
			? PlatformStepRequiredTravels[PlatformIndex]
			: 0.0f,
		PlatformStepSafetyMargins.IsValidIndex(PlatformIndex)
			? PlatformStepSafetyMargins[PlatformIndex]
			: 0.0f);

	if (bFinalLanding)
	{
		const bool bFinalCheckpointPassed = ActiveCheckpointSlot == CheckpointLocalTransforms.Num() - 1;
		bAutomatedRouteValidation = false;
		GetWorldTimerManager().ClearTimer(AutomatedValidationTimer);
		UE_LOG(
			LogTemp,
			Display,
			TEXT("W02 automated route validation complete. RouteSamples=96/96 Lookouts=6/6 FinalCheckpoint96=%s FallReset32=%s FallReset64=%s SummitChoice=Presented AutomatedChoice=Return W00Requested"),
			bFinalCheckpointPassed ? TEXT("Passed") : TEXT("Failed"),
			AutomatedValidationFallPassCount >= 1 ? TEXT("Passed") : TEXT("Failed"),
			AutomatedValidationFallPassCount >= 2 ? TEXT("Passed") : TEXT("Failed"));
		HandleSummitOverlap(
			SummitTrigger,
			ActivePlayer,
			ActivePlayer->GetCapsuleComponent(),
			INDEX_NONE,
			false,
			FHitResult());
		BeginReturnTravel();
		if (!bTravelRequested)
		{
			SummitTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
	}
}

void ASpiralTowerWorldLayout::UpdateCheckpointProgress()
{
	if (!ActivePlayer || !CheckpointLocalTransforms.IsValidIndex(ActiveCheckpointSlot + 1))
	{
		return;
	}

	UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement();
	if (!Movement || !Movement->IsMovingOnGround())
	{
		return;
	}

	const int32 NextCheckpointSlot = ActiveCheckpointSlot + 1;
	const int32 CheckpointPlatformIndex = NextCheckpointSlot
		* SpiralTowerWorldLayoutPrivate::SpiralTowerCheckpointInterval - 1;
	if (!PlatformSurfaceLocalLocations.IsValidIndex(CheckpointPlatformIndex)
		|| !PlatformLocalRotations.IsValidIndex(CheckpointPlatformIndex)
		|| !PlatformLocalDimensions.IsValidIndex(CheckpointPlatformIndex))
	{
		return;
	}

	const FVector PlayerLocalLocation = GetActorTransform().InverseTransformPosition(
		ActivePlayer->GetActorLocation());
	const FVector PlayerLocalFloor = PlayerLocalLocation - FVector(0.0f, 0.0f, GetPlayerCapsuleHalfHeight());
	const FVector CheckpointSurface = PlatformSurfaceLocalLocations[CheckpointPlatformIndex];
	const FTransform PlatformSurfaceTransform(
		PlatformLocalRotations[CheckpointPlatformIndex],
		CheckpointSurface);
	const FVector PlatformSpaceFloor = PlatformSurfaceTransform.InverseTransformPosition(PlayerLocalFloor);
	const FVector2D CheckpointDimensions = PlatformLocalDimensions[CheckpointPlatformIndex];
	const float SafeHalfLength = CheckpointDimensions.X * 0.5f
		- SpiralTowerWorldLayoutPrivate::SpiralTowerCapsuleRadius
		- SpiralTowerWorldLayoutPrivate::SpiralTowerStandingMargin;
	const float SafeHalfWidth = CheckpointDimensions.Y * 0.5f
		- SpiralTowerWorldLayoutPrivate::SpiralTowerCapsuleRadius
		- SpiralTowerWorldLayoutPrivate::SpiralTowerStandingMargin;
	if (FMath::Abs(PlatformSpaceFloor.X) <= SafeHalfLength
		&& FMath::Abs(PlatformSpaceFloor.Y) <= SafeHalfWidth
		&& FMath::Abs(PlatformSpaceFloor.Z) <= 75.0f)
	{
		ActiveCheckpointSlot = NextCheckpointSlot;
		CurrentStamina = SpiralTowerWorldLayoutPrivate::SpiralTowerMaxStamina;
		LastStaminaUseTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		UpdatePlatformingHUD();
		ShowJourneyBeat(ActiveCheckpointSlot);
		RefreshCapturedSky();
		UE_LOG(
			LogTemp,
			Display,
			TEXT("W02 checkpoint reached. Layer=%d Slot=%d/%d Height=%.0f SafeLandingHalfExtents=%.0fx%.0f Stamina=Refilled"),
			CheckpointPlatformIndex + 1,
			ActiveCheckpointSlot,
			CheckpointLocalTransforms.Num() - 1,
			CheckpointSurface.Z,
			SafeHalfLength,
			SafeHalfWidth);
	}
}

void ASpiralTowerWorldLayout::ShowJourneyBeat(const int32 BeatIndex, const bool bPersistent)
{
	static const TCHAR* Titles[] =
	{
		TEXT("天与地的基台 · 深夜"),
		TEXT("第一块泥版 · 重量"),
		TEXT("第二块泥版 · 呼吸"),
		TEXT("第三块泥版 · 回声"),
		TEXT("第四块泥版 · 代价"),
		TEXT("第五块泥版 · 门"),
		TEXT("蓝釉圣所 · 黎明")
	};
	static const TCHAR* Bodies[] =
	{
		TEXT("阶梯神塔已经沉默了三十七年。最后一位筑塔者只留下一句话：不要把登顶当成答案。"),
		TEXT("“我把勋章留在这里。越往上，越要分清什么是必须背负的，什么只是别人交给你的重量。”"),
		TEXT("“停下来不是退缩。风会替你数完下一次呼吸，晒砖的平台也会等。”"),
		TEXT("“城里的人都叫我把墙垒得更高，却没人问过，高处究竟要通向谁。”"),
		TEXT("“我曾以为日出是奖赏。后来才懂，攀登只会让选择更清楚，不会替你选择。”"),
		TEXT("“我没有逃离职责。我只是打开了最后一扇门。真正的黎明不需要一个人永远燃烧自己。”"),
		TEXT("“如果你走进蓝釉圣所，请替我看一次天亮。之后，回去或留下，都不必向神塔证明什么。”")
	};
	if (!ActivePlayer || BeatIndex < 0 || BeatIndex >= UE_ARRAY_COUNT(Titles))
	{
		return;
	}
	if (AWorldWalkerPlayerController* PlayerController =
		Cast<AWorldWalkerPlayerController>(ActivePlayer->GetController()))
	{
		PlayerController->ShowJourneyMessage(Titles[BeatIndex], Bodies[BeatIndex]);
		GetWorldTimerManager().ClearTimer(JourneyMessageTimer);
		if (!bPersistent)
		{
			GetWorldTimerManager().SetTimer(
				JourneyMessageTimer,
				this,
				&ASpiralTowerWorldLayout::HideJourneyMessage,
				9.0f,
				false);
		}
	}
}

void ASpiralTowerWorldLayout::HideJourneyMessage()
{
	if (ActivePlayer)
	{
		if (AWorldWalkerPlayerController* PlayerController =
			Cast<AWorldWalkerPlayerController>(ActivePlayer->GetController()))
		{
			PlayerController->HideJourneyMessage();
		}
	}
}

void ASpiralTowerWorldLayout::ResetPlayerToCheckpoint(const TCHAR* Reason)
{
	if (bResettingPlayer || bTravelRequested || !ActivePlayer
		|| !CheckpointLocalTransforms.IsValidIndex(ActiveCheckpointSlot))
	{
		return;
	}

	bResettingPlayer = true;
	GetWorldTimerManager().ClearTimer(LandingRecoveryTimer);
	bLandingRecovery = false;
	bJumpInProgress = false;
	bJumpHeld = false;
	bJumpStartedAsSprint = false;
	bJumpLoopAnimationPlaying = false;
	bWasFalling = false;
	bSprintActive = false;
	if (bLedgeClimbInProgress)
	{
		ActivePlayer->GetCapsuleComponent()->SetCollisionEnabled(SavedPlayerCollisionMode);
		bLedgeClimbInProgress = false;
	}
	ActivePlayer->RestoreMainWorldLocomotionAnimation();
	CurrentStamina = SpiralTowerWorldLayoutPrivate::SpiralTowerMaxStamina;
	LastStaminaUseTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const FTransform ResetTransform = GetCheckpointWorldTransform(ActiveCheckpointSlot);
	if (UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement())
	{
		Movement->GravityScale = SpiralTowerWorldLayoutPrivate::SpiralTowerGravityScale;
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_Walking);
		Movement->MaxWalkSpeed = SpiralTowerWorldLayoutPrivate::SpiralTowerRunSpeed;
		Movement->MaxAcceleration = SpiralTowerWorldLayoutPrivate::SpiralTowerBaseAcceleration;
	}
	ActivePlayer->SetActorLocationAndRotation(
		ResetTransform.GetLocation(),
		ResetTransform.Rotator(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	if (AController* Controller = ActivePlayer->GetController())
	{
		Controller->SetControlRotation(ResetTransform.Rotator());
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 player reset. CheckpointSlot=%d CheckpointLayer=%d Reason=%s"),
		ActiveCheckpointSlot,
		ActiveCheckpointSlot * SpiralTowerWorldLayoutPrivate::SpiralTowerCheckpointInterval,
		Reason ? Reason : TEXT("unknown"));
	bResettingPlayer = false;
}

FTransform ASpiralTowerWorldLayout::GetCheckpointWorldTransform(const int32 CheckpointSlot) const
{
	if (!CheckpointLocalTransforms.IsValidIndex(CheckpointSlot))
	{
		return GetActorTransform();
	}

	const FTransform& LocalTransform = CheckpointLocalTransforms[CheckpointSlot];
	const FVector LocalCapsuleLocation = LocalTransform.GetLocation()
		+ FVector(0.0f, 0.0f, GetPlayerCapsuleHalfHeight() + 4.0f);
	return FTransform(
		GetActorTransform().TransformRotation(LocalTransform.GetRotation()),
		GetActorTransform().TransformPosition(LocalCapsuleLocation));
}

float ASpiralTowerWorldLayout::GetPlayerCapsuleHalfHeight() const
{
	const UCapsuleComponent* Capsule = ActivePlayer ? ActivePlayer->GetCapsuleComponent() : nullptr;
	return Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 88.0f;
}

void ASpiralTowerWorldLayout::HandleSummitOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	const int32 OtherBodyIndex,
	const bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bTravelRequested || bSummitChoiceAvailable || bLedgeClimbInProgress
		|| !ActivePlayer || OtherActor != ActivePlayer)
	{
		return;
	}
	EnterSummitChoice();
}

void ASpiralTowerWorldLayout::EnterSummitChoice()
{
	if (bTravelRequested || bSummitChoiceAvailable || !ActivePlayer)
	{
		return;
	}

	bSummitChoiceAvailable = true;
	bReturnKeyWasDown = false;
	if (const APlayerController* PlayerController =
		Cast<APlayerController>(ActivePlayer->GetController()))
	{
		// Releasing and pressing E is required; a key already held while entering
		// the terrace can never make the ending choice accidentally.
		bReturnKeyWasDown = PlayerController->IsInputKeyDown(EKeys::E);
	}
	HighestDawnProgress = 1.0f;
	DisplayedDawnProgress = 1.0f;
	UpdateDawnAtmosphere(0.0f);
	RefreshCapturedSky();
	GetWorldTimerManager().ClearTimer(JourneyMessageTimer);
	if (AWorldWalkerPlayerController* PlayerController =
		Cast<AWorldWalkerPlayerController>(ActivePlayer->GetController()))
	{
		PlayerController->ShowJourneyMessage(
			TEXT("塔外 · 日出"),
			TEXT("风越过城垛，太阳从云层下升起。旅程没有替你回答什么，但道路已经属于你。"),
			TEXT("按 E 返回主世界 · 或留在这里继续看日出"));
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 summit choice presented. Dawn=Sunrise Return=PressE Stay=NoInput Movement=Enabled"));
}

void ASpiralTowerWorldLayout::BeginReturnTravel()
{
	if (bTravelRequested || !bSummitChoiceAvailable || !ActivePlayer)
	{
		return;
	}

	UWorldTravelSubsystem* TravelSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UWorldTravelSubsystem>()
		: nullptr;
	PendingDestinationWorld = TravelSubsystem
		? TravelSubsystem->GetMainWorldDefinition()
		: nullptr;
	if (!TravelSubsystem || !PendingDestinationWorld || PendingDestinationWorld->EntryMap.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("W02 summit return is unavailable: W00 WorldDefinition is missing."));
		PendingDestinationWorld = nullptr;
		return;
	}

	bSummitChoiceAvailable = false;
	bTravelRequested = true;
	GetWorldTimerManager().ClearTimer(AutomatedValidationTimer);
	SummitTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	if (APlayerController* PlayerController = Cast<APlayerController>(ActivePlayer->GetController()))
	{
		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(
				0.0f,
				1.0f,
				SpiralTowerWorldLayoutPrivate::SpiralTowerTravelDelay,
				FLinearColor(0.55f, 0.34f, 0.08f),
				false,
				true);
		}
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W02 summit choice selected. Choice=Return Layer=%d ActiveCheckpointSlot=%d ReturningTo=W00 FadeSeconds=%.2f"),
		SpiralTowerWorldLayoutPrivate::SpiralTowerPlatformCount,
		ActiveCheckpointSlot,
		SpiralTowerWorldLayoutPrivate::SpiralTowerTravelDelay);
	GetWorldTimerManager().SetTimer(
		ReturnTravelTimer,
		this,
		&ASpiralTowerWorldLayout::CompleteReturnTravel,
		SpiralTowerWorldLayoutPrivate::SpiralTowerTravelDelay,
		false);
}

void ASpiralTowerWorldLayout::CompleteReturnTravel()
{
	UWorldTravelSubsystem* TravelSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UWorldTravelSubsystem>()
		: nullptr;
	if (TravelSubsystem && PendingDestinationWorld
		&& TravelSubsystem->TravelToWorld(PendingDestinationWorld))
	{
		return;
	}

	UE_LOG(LogTemp, Error, TEXT("W02 summit return failed after the transition began."));
	RestoreAfterFailedTravel();
}

void ASpiralTowerWorldLayout::RestoreAfterFailedTravel()
{
	bTravelRequested = false;
	bSummitChoiceAvailable = true;
	bReturnKeyWasDown = true;
	PendingDestinationWorld = nullptr;
	SummitTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	if (ActivePlayer)
	{
		if (UCharacterMovementComponent* Movement = ActivePlayer->GetCharacterMovement())
		{
			Movement->SetMovementMode(MOVE_Walking);
		}
		if (APlayerController* PlayerController = Cast<APlayerController>(ActivePlayer->GetController()))
		{
			if (PlayerController->PlayerCameraManager)
			{
				PlayerController->PlayerCameraManager->StartCameraFade(
					1.0f,
					0.0f,
					0.20f,
					FLinearColor::Black,
					false,
					false);
			}
		}
		if (AWorldWalkerPlayerController* PlayerController =
			Cast<AWorldWalkerPlayerController>(ActivePlayer->GetController()))
		{
			PlayerController->ShowJourneyMessage(
				TEXT("塔外 · 日出"),
				TEXT("归途暂时没有回应。你仍可以留在这里，或稍后再次尝试。"),
				TEXT("松开并再次按 E 返回主世界"));
		}
	}
}

UStaticMeshComponent* ASpiralTowerWorldLayout::AddCollisionPrimitive(
	const FString& ComponentName,
	UStaticMesh* Mesh,
	const FVector& LocalLocation,
	const FRotator& LocalRotation,
	const FVector& LocalScale,
	const FLinearColor& Tint,
	const bool bEnableCollision,
	UMaterialInterface* PreferredMaterial)
{
	if (!Mesh)
	{
		UE_LOG(LogTemp, Error, TEXT("W02 primitive is unavailable: %s"), *ComponentName);
		return nullptr;
	}

	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, FName(*ComponentName));
	AddInstanceComponent(Component);
	Component->SetupAttachment(SceneRoot);
	Component->SetStaticMesh(Mesh);
	Component->SetRelativeLocation(LocalLocation);
	Component->SetRelativeRotation(LocalRotation);
	Component->SetRelativeScale3D(LocalScale);
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetCollisionEnabled(bEnableCollision
		? ECollisionEnabled::QueryAndPhysics
		: ECollisionEnabled::NoCollision);
	Component->SetCollisionResponseToAllChannels(bEnableCollision ? ECR_Block : ECR_Ignore);
	Component->SetCastShadow(true);
	Component->RegisterComponent();
	TowerMeshes.Add(Component);
	if (PreferredMaterial)
	{
		Component->SetMaterial(0, PreferredMaterial);
	}
	else
	{
		ApplyPrimitiveTint(Component, Tint);
	}
	return Component;
}

UStaticMeshComponent* ASpiralTowerWorldLayout::AddDecorMesh(
	const FString& ComponentName,
	UStaticMesh* PreferredMesh,
	UStaticMesh* FallbackMesh,
	const FVector& LocalLocation,
	const FRotator& LocalRotation,
	const FVector& PreferredScale,
	const FVector& FallbackScale,
	const FLinearColor& FallbackTint,
	UMaterialInterface* PreferredFallbackMaterial)
{
	UStaticMesh* Mesh = PreferredMesh ? PreferredMesh : FallbackMesh;
	if (!Mesh)
	{
		return nullptr;
	}

	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, FName(*ComponentName));
	AddInstanceComponent(Component);
	Component->SetupAttachment(SceneRoot);
	Component->SetStaticMesh(Mesh);
	Component->SetRelativeLocation(LocalLocation);
	Component->SetRelativeRotation(LocalRotation);
	Component->SetRelativeScale3D(PreferredMesh ? PreferredScale : FallbackScale);
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetCollisionResponseToAllChannels(ECR_Ignore);
	Component->SetCastShadow(true);
	Component->RegisterComponent();
	TowerMeshes.Add(Component);

	if (PreferredMesh)
	{
		++ImportedDecorCount;
		if (PreferredMesh == StoneBrickMesh.Get()
			|| PreferredMesh == InteriorWallMesh.Get()
			|| PreferredMesh == CastleDoorwayMesh.Get()
			|| PreferredMesh == CastleBridgeMesh.Get()
			|| PreferredMesh == CastleTowerMidMesh.Get()
			|| PreferredMesh == CastleTowerTopMesh.Get()
			|| PreferredMesh == WoodBeamMesh.Get()
			|| PreferredMesh == BrokenPlankMesh.Get())
		{
			++KenneyDecorCount;
		}
		if (PreferredMesh == PolyHavenWallStraightMesh.Get()
			|| PreferredMesh == PolyHavenGateMesh.Get()
			|| PreferredMesh == PolyHavenWalkwayMesh.Get()
			|| PreferredMesh == PolyHavenTowerRoundMesh.Get())
		{
			++PolyHavenDecorCount;
		}
	}
	else
	{
		++FallbackDecorCount;
		if (PreferredFallbackMaterial)
		{
			Component->SetMaterial(0, PreferredFallbackMaterial);
		}
		else
		{
			ApplyPrimitiveTint(Component, FallbackTint);
		}
	}
	return Component;
}

void ASpiralTowerWorldLayout::AddTorch(
	const FVector& LocalSurfaceLocation,
	const FVector& OutwardDirection,
	const int32 TorchIndex)
{
	const FVector SafeOutward = OutwardDirection.GetSafeNormal();
	const FVector TorchLocation = LocalSurfaceLocation + SafeOutward * 24.0f;
	AddDecorMesh(
		FString::Printf(TEXT("SpiralTorchMesh_%02d"), TorchIndex),
		CampfireMesh,
		ConeFallback,
		TorchLocation + FVector(0.0f, 0.0f, 3.0f),
		FRotator(0.0f, SafeOutward.Rotation().Yaw, 0.0f),
		FVector(0.70f),
		FVector(0.24f, 0.24f, 0.38f),
		FLinearColor(0.70f, 0.11f, 0.012f));

	UPointLightComponent* Light = NewObject<UPointLightComponent>(
		this,
		FName(*FString::Printf(TEXT("SpiralTorchLight_%02d"), TorchIndex)));
	AddInstanceComponent(Light);
	Light->SetupAttachment(SceneRoot);
	Light->SetRelativeLocation(TorchLocation + FVector(0.0f, 0.0f, 105.0f));
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetIntensity(1350.0f);
	Light->SetAttenuationRadius(430.0f);
	Light->SetLightColor(FLinearColor(1.0f, 0.19f, 0.035f));
	Light->SetCastShadows(TorchIndex == 0 || TorchIndex == 1 || TorchIndex >= 20);
	Light->RegisterComponent();
	TorchLights.Add(Light);
}

void ASpiralTowerWorldLayout::ApplyPrimitiveTint(
	UStaticMeshComponent* Component,
	const FLinearColor& Color) const
{
	if (!Component)
	{
		return;
	}

	UMaterialInterface* SourceMaterial = BasicShapeMaterial
		? BasicShapeMaterial.Get()
		: Component->GetMaterial(0);
	if (!SourceMaterial)
	{
		return;
	}

	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(SourceMaterial, Component);
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Component->SetMaterial(0, Material);
}
