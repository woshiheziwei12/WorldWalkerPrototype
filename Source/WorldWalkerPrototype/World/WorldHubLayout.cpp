#include "World/WorldHubLayout.h"

#include "Camera/PlayerCameraManager.h"
#include "Characters/WorldWalkerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/GameInstance.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "World/WorldDefinition.h"
#include "World/WorldPortal.h"
#include "World/WorldTravelSubsystem.h"

const FVector AWorldHubLayout::ActivePortalLocalLocation(-330.0f, 0.0f, 0.0f);
const FVector AWorldHubLayout::SpiralTowerPortalLocalLocation(-630.0f, 720.0f, 0.0f);

AWorldHubLayout::AWorldHubLayout()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SkySphereMesh(
		TEXT("/Engine/MapTemplates/Sky/SM_SkySphere.SM_SkySphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> NightSkyMaterial(
		TEXT("/Engine/MapTemplates/Sky/M_BlackBackground.M_BlackBackground"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PortalFoundationMesh(
		TEXT("/Game/Asian_Village/meshes/props/SM_statue_platform_01.SM_statue_platform_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> EasternArchMesh(
		TEXT("/Game/Asian_Village/meshes/building/SM_wall_wooden_arch_01.SM_wall_wooden_arch_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CanopyMesh(
		TEXT("/Game/Asian_Village/meshes/building/SM_roof_canopy_01.SM_roof_canopy_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PortalDiskMesh(
		TEXT("/Game/Portals/StaticMesh/SM_RadialDisk.SM_RadialDisk"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PortalWaterMaterial(
		TEXT("/Game/Portals/Materials/MI_PortalWater.MI_PortalWater"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LanternMesh(
		TEXT("/Game/Asian_Village/meshes/props/SM_lantern_01.SM_lantern_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StatueMesh(
		TEXT("/Game/Asian_Village/meshes/props/SM_statue_01.SM_statue_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BannerMesh(
		TEXT("/Game/Asian_Village/meshes/props/SM_flag_banner_01.SM_flag_banner_01"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> WaterPortalSystem(
		TEXT("/Game/Portals/Rounded/WaterPortal/NS_WaterPortal.NS_WaterPortal"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> WindPortalSystem(
		TEXT("/Game/Portals/Rounded/WindPortal/NS_WindPortal.NS_WindPortal"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> LightningPortalSystem(
		TEXT("/Game/Portals/Rounded/LightningPortal/NS_Lightning.NS_Lightning"));

	NightSkySphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NightSkySphere"));
	NightSkySphere->SetupAttachment(SceneRoot);
	NightSkySphere->SetRelativeLocation(FVector(0.0f, 0.0f, -1000.0f));
	NightSkySphere->SetRelativeScale3D(FVector(100.0f));
	NightSkySphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NightSkySphere->SetCastShadow(false);
	NightSkySphere->SetBoundsScale(4.0f);
	if (SkySphereMesh.Succeeded())
	{
		NightSkySphere->SetStaticMesh(SkySphereMesh.Object);
	}
	if (NightSkyMaterial.Succeeded())
	{
		NightSkySphere->SetMaterial(0, NightSkyMaterial.Object);
	}

	MoonLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("MoonLight"));
	MoonLight->SetupAttachment(SceneRoot);
	MoonLight->SetRelativeRotation(FRotator(-34.0f, -42.0f, 0.0f));
	MoonLight->SetMobility(EComponentMobility::Movable);
	MoonLight->SetIntensity(0.60f);
	MoonLight->SetLightColor(FLinearColor(0.43f, 0.58f, 1.0f));
	MoonLight->SetCastShadows(true);

	NightSkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("NightSkyLight"));
	NightSkyLight->SetupAttachment(SceneRoot);
	NightSkyLight->SetMobility(EComponentMobility::Movable);
	NightSkyLight->SourceType = SLS_CapturedScene;
	NightSkyLight->bRealTimeCapture = false;
	NightSkyLight->bLowerHemisphereIsBlack = false;
	NightSkyLight->SetLowerHemisphereColor(FLinearColor(0.008f, 0.014f, 0.035f));
	NightSkyLight->SetLightColor(FLinearColor(0.42f, 0.55f, 1.0f));
	NightSkyLight->SetIntensity(0.20f);

	NightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("NightFog"));
	NightFog->SetupAttachment(SceneRoot);
	NightFog->SetFogDensity(0.0065f);
	NightFog->SetFogHeightFalloff(0.08f);
	NightFog->SetStartDistance(1600.0f);
	NightFog->SetFogInscatteringColor(FLinearColor(0.025f, 0.055f, 0.13f));
	NightFog->SetVolumetricFog(true);
	NightFog->SetVolumetricFogScatteringDistribution(0.35f);
	NightFog->SetVolumetricFogExtinctionScale(0.7f);
	NightFog->SetVolumetricFogAlbedo(FColor(78, 105, 155));
	NightFog->SetVolumetricFogEmissive(FLinearColor(0.002f, 0.004f, 0.012f));
	NightFog->SetVolumetricFogDistance(36000.0f);

	NightPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("NightPostProcess"));
	NightPostProcess->SetupAttachment(SceneRoot);
	NightPostProcess->bUnbound = true;
	NightPostProcess->bEnabled = true;
	NightPostProcess->Priority = 100.0f;
	NightPostProcess->BlendWeight = 1.0f;
	NightPostProcess->Settings.bOverride_AutoExposureBias = true;
	NightPostProcess->Settings.AutoExposureBias = -0.85f;
	NightPostProcess->Settings.bOverride_ColorGain = true;
	NightPostProcess->Settings.ColorGain = FVector4(0.78f, 0.84f, 1.0f, 1.0f);
	NightPostProcess->Settings.bOverride_SceneColorTint = true;
	NightPostProcess->Settings.SceneColorTint = FLinearColor(0.92f, 0.96f, 1.0f);
	NightPostProcess->Settings.bOverride_BloomIntensity = true;
	NightPostProcess->Settings.BloomIntensity = 0.20f;
	NightPostProcess->Settings.bOverride_VignetteIntensity = true;
	NightPostProcess->Settings.VignetteIntensity = 0.12f;

	PortalFoundation = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalFoundation"));
	PortalFoundation->SetupAttachment(SceneRoot);
	PortalFoundation->SetRelativeLocation(ActivePortalLocalLocation + FVector(0.0f, 0.0f, 4.0f));
	PortalFoundation->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	PortalFoundation->SetRelativeScale3D(FVector(1.20f, 1.20f, 0.65f));
	PortalFoundation->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (PortalFoundationMesh.Succeeded())
	{
		PortalFoundation->SetStaticMesh(PortalFoundationMesh.Object);
	}

	PortalArch = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalArch"));
	PortalArch->SetupAttachment(SceneRoot);
	PortalArch->SetRelativeLocation(ActivePortalLocalLocation + FVector(0.0f, 0.0f, 63.0f));
	PortalArch->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	PortalArch->SetRelativeScale3D(FVector(0.80f));
	PortalArch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (EasternArchMesh.Succeeded())
	{
		PortalArch->SetStaticMesh(EasternArchMesh.Object);
	}

	PortalCanopy = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalCanopy"));
	PortalCanopy->SetupAttachment(SceneRoot);
	PortalCanopy->SetRelativeLocation(ActivePortalLocalLocation + FVector(-8.0f, 0.0f, 335.0f));
	PortalCanopy->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	PortalCanopy->SetRelativeScale3D(FVector(0.72f, 0.72f, 0.62f));
	PortalCanopy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CanopyMesh.Succeeded())
	{
		PortalCanopy->SetStaticMesh(CanopyMesh.Object);
	}

	PortalDisk = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalDisk"));
	PortalDisk->SetupAttachment(SceneRoot);
	PortalDisk->SetRelativeLocation(ActivePortalLocalLocation + FVector(8.0f, 0.0f, 150.0f));
	PortalDisk->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	PortalDisk->SetRelativeScale3D(FVector(1.48f));
	PortalDisk->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalDisk->SetCastShadow(false);
	PortalDisk->SetTranslucentSortPriority(5);
	if (PortalDiskMesh.Succeeded())
	{
		PortalDisk->SetStaticMesh(PortalDiskMesh.Object);
	}
	if (PortalWaterMaterial.Succeeded())
	{
		PortalDisk->SetMaterial(0, PortalWaterMaterial.Object);
	}

	PortalVortex = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalVortex"));
	PortalVortex->SetupAttachment(SceneRoot);
	PortalVortex->SetRelativeLocation(ActivePortalLocalLocation + FVector(0.0f, 0.0f, 150.0f));
	PortalVortex->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	PortalVortex->SetRelativeScale3D(FVector(0.19f));
	PortalVortex->SetAutoActivate(true);
	if (WaterPortalSystem.Succeeded())
	{
		PortalVortex->SetAsset(WaterPortalSystem.Object);
	}
	PortalVortex->SetSystemFixedBounds(FBox(FVector(-700.0f), FVector(700.0f)));

	PortalLightning = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalLightning"));
	PortalLightning->SetupAttachment(SceneRoot);
	PortalLightning->SetRelativeLocation(ActivePortalLocalLocation + FVector(-4.0f, 0.0f, 150.0f));
	PortalLightning->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	PortalLightning->SetRelativeScale3D(FVector(0.225f));
	// The full rounded lightning system is authored for a twelve-metre demo
	// portal. Even after scaling, its long additive ribbons cross the camera
	// frustum and paint the whole sky. Keep the component available for a later
	// emitter-level variant, but leave it inactive in the compact W00 shrine.
	PortalLightning->SetAutoActivate(false);
	if (LightningPortalSystem.Succeeded())
	{
		PortalLightning->SetAsset(LightningPortalSystem.Object);
	}
	PortalLightning->SetSystemFixedBounds(FBox(FVector(-700.0f), FVector(700.0f)));

	PortalTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("PortalTrigger"));
	PortalTrigger->SetupAttachment(SceneRoot);
	PortalTrigger->SetRelativeLocation(ActivePortalLocalLocation + FVector(0.0f, 0.0f, 150.0f));
	PortalTrigger->SetBoxExtent(FVector(90.0f, 120.0f, 155.0f));
	PortalTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PortalTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	PortalTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	PortalTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PortalTrigger->OnComponentBeginOverlap.AddDynamic(this, &AWorldHubLayout::HandlePortalOverlap);

	PortalLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortalLight"));
	PortalLight->SetupAttachment(SceneRoot);
	PortalLight->SetRelativeLocation(ActivePortalLocalLocation + FVector(-70.0f, 0.0f, 155.0f));
	PortalLight->SetIntensity(1500.0f);
	PortalLight->SetAttenuationRadius(460.0f);
	PortalLight->SetLightColor(FLinearColor(0.10f, 0.72f, 1.0f));
	PortalLight->SetCastShadows(true);

	PortalInstruction = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PortalInstruction"));
	PortalInstruction->SetupAttachment(SceneRoot);
	PortalInstruction->SetRelativeLocation(ActivePortalLocalLocation + FVector(-55.0f, 0.0f, 385.0f));
	PortalInstruction->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	PortalInstruction->SetHorizontalAlignment(EHTA_Center);
	PortalInstruction->SetVerticalAlignment(EVRTA_TextCenter);
	PortalInstruction->SetWorldSize(17.0f);
	PortalInstruction->SetTextRenderColor(FColor(135, 225, 255));
	PortalInstruction->SetText(FText::FromString(TEXT("ENTER THE RIFT")));

	auto CreateShrineMesh = [this](
		const FName ComponentName,
		UStaticMesh* Mesh,
		const FVector& RelativeLocation,
		const FRotator& RelativeRotation,
		const FVector& RelativeScale)
	{
		UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(ComponentName);
		Component->SetupAttachment(SceneRoot);
		Component->SetStaticMesh(Mesh);
		Component->SetRelativeLocation(RelativeLocation);
		Component->SetRelativeRotation(RelativeRotation);
		Component->SetRelativeScale3D(RelativeScale);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PortalShrineScenery.Add(Component);
	};

	CreateShrineMesh(
		TEXT("PortalLanternLeft"),
		LanternMesh.Succeeded() ? LanternMesh.Object : nullptr,
		ActivePortalLocalLocation + FVector(-42.0f, -245.0f, 315.0f),
		FRotator::ZeroRotator,
		FVector(1.15f));
	CreateShrineMesh(
		TEXT("PortalLanternRight"),
		LanternMesh.Succeeded() ? LanternMesh.Object : nullptr,
		ActivePortalLocalLocation + FVector(-42.0f, 245.0f, 315.0f),
		FRotator::ZeroRotator,
		FVector(1.15f));
	CreateShrineMesh(
		TEXT("PortalGuardianLeft"),
		StatueMesh.Succeeded() ? StatueMesh.Object : nullptr,
		ActivePortalLocalLocation + FVector(35.0f, -340.0f, 58.0f),
		FRotator(0.0f, 75.0f, 0.0f),
		FVector(0.72f));
	CreateShrineMesh(
		TEXT("PortalGuardianRight"),
		StatueMesh.Succeeded() ? StatueMesh.Object : nullptr,
		ActivePortalLocalLocation + FVector(35.0f, 340.0f, 58.0f),
		FRotator(0.0f, -75.0f, 0.0f),
		FVector(0.72f));
	CreateShrineMesh(
		TEXT("PortalBannerLeft"),
		BannerMesh.Succeeded() ? BannerMesh.Object : nullptr,
		ActivePortalLocalLocation + FVector(50.0f, -425.0f, 35.0f),
		FRotator(0.0f, 90.0f, 0.0f),
		FVector(0.90f));
	CreateShrineMesh(
		TEXT("PortalBannerRight"),
		BannerMesh.Succeeded() ? BannerMesh.Object : nullptr,
		ActivePortalLocalLocation + FVector(50.0f, 425.0f, 35.0f),
		FRotator(0.0f, -90.0f, 0.0f),
		FVector(0.90f));

	const FVector ShrineLightLocations[] =
	{
		ActivePortalLocalLocation + FVector(-70.0f, -245.0f, 265.0f),
		ActivePortalLocalLocation + FVector(-70.0f, 245.0f, 265.0f)
	};
	for (int32 LightIndex = 0; LightIndex < UE_ARRAY_COUNT(ShrineLightLocations); ++LightIndex)
	{
		UPointLightComponent* ShrineLight = CreateDefaultSubobject<UPointLightComponent>(
			*FString::Printf(TEXT("ShrineLight_%d"), LightIndex));
		ShrineLight->SetupAttachment(SceneRoot);
		ShrineLight->SetRelativeLocation(ShrineLightLocations[LightIndex]);
		ShrineLight->SetIntensity(700.0f);
		ShrineLight->SetAttenuationRadius(320.0f);
		ShrineLight->SetLightColor(FLinearColor(1.0f, 0.34f, 0.07f));
		ShrineLight->SetCastShadows(false);
		ShrineLights.Add(ShrineLight);
	}

	// W02 keeps the W01 shrine silhouette and circular portal proportions, but
	// uses the package's rounded wind system plus purple/gold lighting so the
	// interaction-only destination reads differently from the automatic rift.
	const FVector PlayerLocalLocation(-850.0f, 0.0f, 0.0f);
	const FVector SpiralFacing =
		(PlayerLocalLocation - SpiralTowerPortalLocalLocation).GetSafeNormal2D();
	const FVector SpiralSide(-SpiralFacing.Y, SpiralFacing.X, 0.0f);
	const float SpiralFacingYaw = SpiralFacing.Rotation().Yaw;
	const float SpiralFrameYaw = SpiralFacingYaw - 90.0f;

	SpiralTowerPortalFoundation = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("SpiralTowerPortalFoundation"));
	SpiralTowerPortalFoundation->SetupAttachment(SceneRoot);
	SpiralTowerPortalFoundation->SetRelativeLocation(
		SpiralTowerPortalLocalLocation + FVector(0.0f, 0.0f, 4.0f));
	SpiralTowerPortalFoundation->SetRelativeRotation(FRotator(0.0f, SpiralFrameYaw, 0.0f));
	SpiralTowerPortalFoundation->SetRelativeScale3D(FVector(1.20f, 1.20f, 0.65f));
	SpiralTowerPortalFoundation->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (PortalFoundationMesh.Succeeded())
	{
		SpiralTowerPortalFoundation->SetStaticMesh(PortalFoundationMesh.Object);
	}

	SpiralTowerPortalArch = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpiralTowerPortalArch"));
	SpiralTowerPortalArch->SetupAttachment(SceneRoot);
	SpiralTowerPortalArch->SetRelativeLocation(
		SpiralTowerPortalLocalLocation + FVector(0.0f, 0.0f, 63.0f));
	SpiralTowerPortalArch->SetRelativeRotation(FRotator(0.0f, SpiralFrameYaw, 0.0f));
	SpiralTowerPortalArch->SetRelativeScale3D(FVector(0.80f));
	SpiralTowerPortalArch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (EasternArchMesh.Succeeded())
	{
		SpiralTowerPortalArch->SetStaticMesh(EasternArchMesh.Object);
	}

	SpiralTowerPortalCanopy = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpiralTowerPortalCanopy"));
	SpiralTowerPortalCanopy->SetupAttachment(SceneRoot);
	SpiralTowerPortalCanopy->SetRelativeLocation(
		SpiralTowerPortalLocalLocation + SpiralFacing * 8.0f + FVector(0.0f, 0.0f, 335.0f));
	SpiralTowerPortalCanopy->SetRelativeRotation(FRotator(0.0f, SpiralFrameYaw, 0.0f));
	SpiralTowerPortalCanopy->SetRelativeScale3D(FVector(0.72f, 0.72f, 0.62f));
	SpiralTowerPortalCanopy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CanopyMesh.Succeeded())
	{
		SpiralTowerPortalCanopy->SetStaticMesh(CanopyMesh.Object);
	}

	SpiralTowerPortalDisk = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpiralTowerPortalDisk"));
	SpiralTowerPortalDisk->SetupAttachment(SceneRoot);
	SpiralTowerPortalDisk->SetRelativeLocation(
		SpiralTowerPortalLocalLocation - SpiralFacing * 8.0f + FVector(0.0f, 0.0f, 150.0f));
	SpiralTowerPortalDisk->SetRelativeRotation(FRotator(90.0f, SpiralFacingYaw - 180.0f, 0.0f));
	SpiralTowerPortalDisk->SetRelativeScale3D(FVector(1.48f));
	SpiralTowerPortalDisk->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpiralTowerPortalDisk->SetCastShadow(false);
	SpiralTowerPortalDisk->SetTranslucentSortPriority(5);
	if (PortalDiskMesh.Succeeded())
	{
		SpiralTowerPortalDisk->SetStaticMesh(PortalDiskMesh.Object);
	}
	if (PortalWaterMaterial.Succeeded())
	{
		SpiralTowerPortalDisk->SetMaterial(0, PortalWaterMaterial.Object);
	}

	SpiralTowerPortalVortex = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SpiralTowerPortalVortex"));
	SpiralTowerPortalVortex->SetupAttachment(SceneRoot);
	SpiralTowerPortalVortex->SetRelativeLocation(
		SpiralTowerPortalLocalLocation + FVector(0.0f, 0.0f, 150.0f));
	SpiralTowerPortalVortex->SetRelativeRotation(FRotator(0.0f, SpiralFacingYaw, 0.0f));
	SpiralTowerPortalVortex->SetRelativeScale3D(FVector(0.19f));
	SpiralTowerPortalVortex->SetAutoActivate(true);
	if (WindPortalSystem.Succeeded())
	{
		SpiralTowerPortalVortex->SetAsset(WindPortalSystem.Object);
	}
	SpiralTowerPortalVortex->SetSystemFixedBounds(FBox(FVector(-700.0f), FVector(700.0f)));

	SpiralTowerPortalLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("SpiralTowerPortalLight"));
	SpiralTowerPortalLight->SetupAttachment(SceneRoot);
	SpiralTowerPortalLight->SetRelativeLocation(
		SpiralTowerPortalLocalLocation + SpiralFacing * 70.0f + FVector(0.0f, 0.0f, 155.0f));
	SpiralTowerPortalLight->SetIntensity(1750.0f);
	SpiralTowerPortalLight->SetAttenuationRadius(480.0f);
	SpiralTowerPortalLight->SetLightColor(FLinearColor(0.72f, 0.08f, 1.0f));
	SpiralTowerPortalLight->SetCastShadows(true);

	SpiralTowerPortalInstruction = CreateDefaultSubobject<UTextRenderComponent>(
		TEXT("SpiralTowerPortalInstruction"));
	SpiralTowerPortalInstruction->SetupAttachment(SceneRoot);
	SpiralTowerPortalInstruction->SetRelativeLocation(
		SpiralTowerPortalLocalLocation + SpiralFacing * 55.0f + FVector(0.0f, 0.0f, 385.0f));
	SpiralTowerPortalInstruction->SetRelativeRotation(FRotator(0.0f, SpiralFacingYaw, 0.0f));
	SpiralTowerPortalInstruction->SetHorizontalAlignment(EHTA_Center);
	SpiralTowerPortalInstruction->SetVerticalAlignment(EVRTA_TextCenter);
	SpiralTowerPortalInstruction->SetWorldSize(17.0f);
	SpiralTowerPortalInstruction->SetTextRenderColor(FColor(255, 176, 70));
	SpiralTowerPortalInstruction->SetText(FText::FromString(TEXT("SPIRAL TOWER\nPRESS E")));

	auto CreateSpiralShrineMesh = [this](
		const FName ComponentName,
		UStaticMesh* Mesh,
		const FVector& RelativeLocation,
		const FRotator& RelativeRotation,
		const FVector& RelativeScale)
	{
		UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(ComponentName);
		Component->SetupAttachment(SceneRoot);
		Component->SetStaticMesh(Mesh);
		Component->SetRelativeLocation(RelativeLocation);
		Component->SetRelativeRotation(RelativeRotation);
		Component->SetRelativeScale3D(RelativeScale);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SpiralTowerPortalScenery.Add(Component);
	};

	CreateSpiralShrineMesh(
		TEXT("SpiralTowerPortalLanternLeft"),
		LanternMesh.Succeeded() ? LanternMesh.Object : nullptr,
		SpiralTowerPortalLocalLocation + SpiralFacing * 42.0f + SpiralSide * 245.0f
			+ FVector(0.0f, 0.0f, 315.0f),
		FRotator(0.0f, SpiralFacingYaw - 180.0f, 0.0f),
		FVector(1.15f));
	CreateSpiralShrineMesh(
		TEXT("SpiralTowerPortalLanternRight"),
		LanternMesh.Succeeded() ? LanternMesh.Object : nullptr,
		SpiralTowerPortalLocalLocation + SpiralFacing * 42.0f - SpiralSide * 245.0f
			+ FVector(0.0f, 0.0f, 315.0f),
		FRotator(0.0f, SpiralFacingYaw - 180.0f, 0.0f),
		FVector(1.15f));
	CreateSpiralShrineMesh(
		TEXT("SpiralTowerPortalGuardianLeft"),
		StatueMesh.Succeeded() ? StatueMesh.Object : nullptr,
		SpiralTowerPortalLocalLocation - SpiralFacing * 35.0f + SpiralSide * 340.0f
			+ FVector(0.0f, 0.0f, 58.0f),
		FRotator(0.0f, SpiralFacingYaw - 105.0f, 0.0f),
		FVector(0.72f));
	CreateSpiralShrineMesh(
		TEXT("SpiralTowerPortalGuardianRight"),
		StatueMesh.Succeeded() ? StatueMesh.Object : nullptr,
		SpiralTowerPortalLocalLocation - SpiralFacing * 35.0f - SpiralSide * 340.0f
			+ FVector(0.0f, 0.0f, 58.0f),
		FRotator(0.0f, SpiralFacingYaw + 105.0f, 0.0f),
		FVector(0.72f));
	CreateSpiralShrineMesh(
		TEXT("SpiralTowerPortalBannerLeft"),
		BannerMesh.Succeeded() ? BannerMesh.Object : nullptr,
		SpiralTowerPortalLocalLocation - SpiralFacing * 50.0f + SpiralSide * 425.0f
			+ FVector(0.0f, 0.0f, 35.0f),
		FRotator(0.0f, SpiralFacingYaw - 90.0f, 0.0f),
		FVector(0.90f));
	CreateSpiralShrineMesh(
		TEXT("SpiralTowerPortalBannerRight"),
		BannerMesh.Succeeded() ? BannerMesh.Object : nullptr,
		SpiralTowerPortalLocalLocation - SpiralFacing * 50.0f - SpiralSide * 425.0f
			+ FVector(0.0f, 0.0f, 35.0f),
		FRotator(0.0f, SpiralFacingYaw + 90.0f, 0.0f),
		FVector(0.90f));

	const FVector SpiralShrineLightLocations[] =
	{
		SpiralTowerPortalLocalLocation + SpiralFacing * 70.0f + SpiralSide * 245.0f
			+ FVector(0.0f, 0.0f, 265.0f),
		SpiralTowerPortalLocalLocation + SpiralFacing * 70.0f - SpiralSide * 245.0f
			+ FVector(0.0f, 0.0f, 265.0f)
	};
	for (int32 LightIndex = 0; LightIndex < UE_ARRAY_COUNT(SpiralShrineLightLocations); ++LightIndex)
	{
		UPointLightComponent* ShrineLight = CreateDefaultSubobject<UPointLightComponent>(
			*FString::Printf(TEXT("SpiralTowerShrineLight_%d"), LightIndex));
		ShrineLight->SetupAttachment(SceneRoot);
		ShrineLight->SetRelativeLocation(SpiralShrineLightLocations[LightIndex]);
		ShrineLight->SetIntensity(850.0f);
		ShrineLight->SetAttenuationRadius(340.0f);
		ShrineLight->SetLightColor(FLinearColor(1.0f, 0.30f, 0.025f));
		ShrineLight->SetCastShadows(false);
		SpiralTowerShrineLights.Add(ShrineLight);
	}

	// Imported shrine meshes can have coarse convex collision that closes the
	// visual archway. Use explicit query-only shapes so the stone and wood read
	// as solid to the player/camera while the portal opening stays traversable.
	auto CreatePortalCollisionBox = [this](
		const FName ComponentName,
		const FVector& RelativeLocation,
		const float FacingYaw,
		const FVector& BoxExtent,
		const bool bBlockPawn,
		const bool bBlockCamera)
	{
		UBoxComponent* CollisionBox = CreateDefaultSubobject<UBoxComponent>(ComponentName);
		CollisionBox->SetupAttachment(SceneRoot);
		CollisionBox->SetRelativeLocation(RelativeLocation);
		CollisionBox->SetRelativeRotation(FRotator(0.0f, FacingYaw, 0.0f));
		CollisionBox->SetBoxExtent(BoxExtent);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		CollisionBox->SetCollisionObjectType(ECC_WorldStatic);
		CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		CollisionBox->SetCollisionResponseToChannel(
			ECC_Pawn,
			bBlockPawn ? ECR_Block : ECR_Ignore);
		CollisionBox->SetCollisionResponseToChannel(
			ECC_Camera,
			bBlockCamera ? ECR_Block : ECR_Ignore);
		CollisionBox->SetGenerateOverlapEvents(false);
		CollisionBox->SetCanEverAffectNavigation(false);
		PortalCollisionShapes.Add(CollisionBox);
	};

	auto CreateShrineCollision = [&CreatePortalCollisionBox](
		const TCHAR* NamePrefix,
		const FVector& PortalLocation,
		const FVector& Facing,
		const FVector& Side,
		const float FacingYaw)
	{
		auto MakeName = [NamePrefix](const TCHAR* Suffix)
		{
			return FName(*FString::Printf(TEXT("%s%s"), NamePrefix, Suffix));
		};

		// A shallow, stepable base supports the arch without becoming a curb.
		CreatePortalCollisionBox(
			MakeName(TEXT("FoundationCollision")),
			PortalLocation + FVector(0.0f, 0.0f, 15.0f),
			FacingYaw,
			FVector(90.0f, 235.0f, 15.0f),
			true,
			true);

		// The inner pillar faces sit at +/-165uu, leaving a 330uu-wide opening.
		CreatePortalCollisionBox(
			MakeName(TEXT("LeftPillarCollision")),
			PortalLocation + Side * 205.0f + FVector(0.0f, 0.0f, 155.0f),
			FacingYaw,
			FVector(55.0f, 40.0f, 155.0f),
			true,
			true);
		CreatePortalCollisionBox(
			MakeName(TEXT("RightPillarCollision")),
			PortalLocation - Side * 205.0f + FVector(0.0f, 0.0f, 155.0f),
			FacingYaw,
			FVector(55.0f, 40.0f, 155.0f),
			true,
			true);
		CreatePortalCollisionBox(
			MakeName(TEXT("UpperArchCollision")),
			PortalLocation + FVector(0.0f, 0.0f, 300.0f),
			FacingYaw,
			FVector(55.0f, 245.0f, 28.0f),
			true,
			true);

		// High or hanging pieces should pull the camera forward, but must never
		// snag a jumping character passing through the center of the shrine.
		CreatePortalCollisionBox(
			MakeName(TEXT("CanopyCollision")),
			PortalLocation + Facing * 8.0f + FVector(0.0f, 0.0f, 350.0f),
			FacingYaw,
			FVector(115.0f, 300.0f, 30.0f),
			false,
			true);
		CreatePortalCollisionBox(
			MakeName(TEXT("LeftLanternCollision")),
			PortalLocation + Facing * 42.0f + Side * 245.0f + FVector(0.0f, 0.0f, 315.0f),
			FacingYaw,
			FVector(35.0f, 35.0f, 60.0f),
			false,
			true);
		CreatePortalCollisionBox(
			MakeName(TEXT("RightLanternCollision")),
			PortalLocation + Facing * 42.0f - Side * 245.0f + FVector(0.0f, 0.0f, 315.0f),
			FacingYaw,
			FVector(35.0f, 35.0f, 60.0f),
			false,
			true);

		CreatePortalCollisionBox(
			MakeName(TEXT("LeftGuardianCollision")),
			PortalLocation - Facing * 35.0f + Side * 340.0f + FVector(0.0f, 0.0f, 85.0f),
			FacingYaw,
			FVector(70.0f, 55.0f, 85.0f),
			true,
			true);
		CreatePortalCollisionBox(
			MakeName(TEXT("RightGuardianCollision")),
			PortalLocation - Facing * 35.0f - Side * 340.0f + FVector(0.0f, 0.0f, 85.0f),
			FacingYaw,
			FVector(70.0f, 55.0f, 85.0f),
			true,
			true);
	};

	const FVector ActivePortalFacing(-1.0f, 0.0f, 0.0f);
	const FVector ActivePortalSide(0.0f, -1.0f, 0.0f);
	CreateShrineCollision(
		TEXT("ActivePortal"),
		ActivePortalLocalLocation,
		ActivePortalFacing,
		ActivePortalSide,
		180.0f);
	CreateShrineCollision(
		TEXT("SpiralTowerPortal"),
		SpiralTowerPortalLocalLocation,
		SpiralFacing,
		SpiralSide,
		SpiralFacingYaw);
}

void AWorldHubLayout::BeginPlay()
{
	Super::BeginPlay();

	ConfigureMainWorldEnvironment();
	ConfigureMainWorldAvatar();
	GetWorldTimerManager().SetTimerForNextTick(this, &AWorldHubLayout::HideLegacyPortal);
	GetWorldTimerManager().SetTimerForNextTick(this, &AWorldHubLayout::RefreshNightSky);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W00 night presentation ready. Avatar=%s W01Portal=%s W02Portal=%s"),
		MainWorldCharacter ? TEXT("anime-animated") : TEXT("missing"),
		*GetActivePortalLocation().ToCompactString(),
		*GetSpiralTowerPortalLocation().ToCompactString());
}

void AWorldHubLayout::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateMainWorldMovement();
	PortalPulseTime += DeltaSeconds;

	if (PortalDisk)
	{
		PortalDisk->AddLocalRotation(FRotator(0.0f, -28.0f * DeltaSeconds, 0.0f));
	}
	if (SpiralTowerPortalDisk)
	{
		SpiralTowerPortalDisk->AddLocalRotation(FRotator(0.0f, 32.0f * DeltaSeconds, 0.0f));
	}
	if (PortalLight && !bTravelRequested)
	{
		PortalLight->SetIntensity(1500.0f + FMath::Sin(PortalPulseTime * 2.4f) * 250.0f);
	}
	if (SpiralTowerPortalLight)
	{
		SpiralTowerPortalLight->SetIntensity(
			1750.0f + FMath::Sin(PortalPulseTime * 2.1f + 1.2f) * 300.0f);
	}
	for (int32 LightIndex = 0; LightIndex < ShrineLights.Num(); ++LightIndex)
	{
		if (UPointLightComponent* ShrineLight = ShrineLights[LightIndex])
		{
			const float Flicker = FMath::Sin(PortalPulseTime * (3.7f + LightIndex * 0.35f)) * 65.0f;
			ShrineLight->SetIntensity(700.0f + Flicker);
		}
	}
	for (int32 LightIndex = 0; LightIndex < SpiralTowerShrineLights.Num(); ++LightIndex)
	{
		if (UPointLightComponent* ShrineLight = SpiralTowerShrineLights[LightIndex])
		{
			const float Flicker =
				FMath::Sin(PortalPulseTime * (4.1f + LightIndex * 0.4f) + 0.8f) * 80.0f;
			ShrineLight->SetIntensity(850.0f + Flicker);
		}
	}
}

void AWorldHubLayout::ConfigureMainWorldEnvironment()
{
	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		if (UDirectionalLightComponent* ExistingSun = It->GetComponent())
		{
			ExistingSun->SetMobility(EComponentMobility::Movable);
			ExistingSun->SetIntensity(0.0f);
			ExistingSun->SetIndirectLightingIntensity(0.0f);
			ExistingSun->SetVisibility(false);
		}
	}

	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
	{
		if (USkyLightComponent* ExistingSkyLight = It->GetLightComponent())
		{
			ExistingSkyLight->SetMobility(EComponentMobility::Movable);
			ExistingSkyLight->SetIntensity(0.0f);
			ExistingSkyLight->SetIndirectLightingIntensity(0.0f);
			ExistingSkyLight->SetVisibility(false);
		}
	}

	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
	{
		if (UExponentialHeightFogComponent* ExistingFog = It->GetComponent())
		{
			ExistingFog->SetVisibility(false);
		}
	}

	// The vendor demo ships with a cinematic unbound volume tuned for its
	// daylight showcase.  Leaving it active stacks depth/bloom treatment over
	// the W00 night grade and turns the portal particles into a grey screen-edge
	// veil, so the W00-owned post-process component is the sole authority.
	for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
	{
		It->bEnabled = false;
		It->BlendWeight = 0.0f;
	}

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Candidate = *It;
		if (!Candidate || Candidate == this)
		{
			continue;
		}

		if (Candidate->ActorHasTag(TEXT("W00NightSky")))
		{
			Candidate->SetActorHiddenInGame(true);
			continue;
		}

		if (Candidate->GetClass()->GetName().Contains(TEXT("BP_Sky_Sphere")))
		{
			Candidate->SetActorHiddenInGame(true);
			continue;
		}

		TArray<UStaticMeshComponent*> StaticMeshComponents;
		Candidate->GetComponents(StaticMeshComponents);
		for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
		{
			const UStaticMesh* StaticMesh = StaticMeshComponent
				? StaticMeshComponent->GetStaticMesh()
				: nullptr;
			if (StaticMesh && StaticMesh->GetPathName().Contains(
				TEXT("/Game/Asian_Village/meshes/sky/SM_sky")))
			{
				StaticMeshComponent->SetVisibility(false, true);
				StaticMeshComponent->SetHiddenInGame(true, true);
			}
		}
	}
}

void AWorldHubLayout::ConfigureMainWorldAvatar()
{
	MainWorldCharacter = Cast<AWorldWalkerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!MainWorldCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("W00 anime avatar was not configured: player character is missing."));
		return;
	}

	if (!MainWorldCharacter->ConfigureMainWorldAnimeForm())
	{
		UE_LOG(LogTemp, Error, TEXT("W00 anime avatar configuration is incomplete."));
	}

	if (UCharacterMovementComponent* Movement = MainWorldCharacter->GetCharacterMovement())
	{
		DefaultWalkSpeed = Movement->MaxWalkSpeed;
		Movement->MaxAcceleration = 1850.0f;
		Movement->BrakingDecelerationWalking = 1250.0f;
		Movement->GroundFriction = 6.0f;
		Movement->JumpZVelocity = 520.0f;
		Movement->AirControl = 0.45f;
	}
}

void AWorldHubLayout::RefreshNightSky()
{
	if (NightSkyLight)
	{
		NightSkyLight->RecaptureSky();
	}
}

void AWorldHubLayout::HideLegacyPortal()
{
	const FVector PortalLocations[] =
	{
		GetActivePortalLocation(),
		GetSpiralTowerPortalLocation()
	};
	for (TActorIterator<AWorldPortal> It(GetWorld()); It; ++It)
	{
		AWorldPortal* Portal = *It;
		if (!Portal)
		{
			continue;
		}

		for (const FVector& PortalLocation : PortalLocations)
		{
			if (FVector::DistSquared(Portal->GetActorLocation(), PortalLocation) < FMath::Square(350.0f))
			{
				Portal->SetActorHiddenInGame(true);
				Portal->SetActorEnableCollision(false);
				break;
			}
		}
	}
}

void AWorldHubLayout::UpdateMainWorldMovement()
{
	if (!MainWorldCharacter || bTravelRequested)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(MainWorldCharacter->GetController());
	UCharacterMovementComponent* Movement = MainWorldCharacter->GetCharacterMovement();
	if (!PlayerController || !Movement)
	{
		return;
	}

	const bool bSprintRequested =
		PlayerController->IsInputKeyDown(EKeys::LeftShift)
		|| PlayerController->IsInputKeyDown(EKeys::RightShift);
	const bool bMoving = MainWorldCharacter->GetVelocity().SizeSquared2D() > FMath::Square(10.0f);
	const bool bShouldSprint = bSprintRequested && bMoving && !Movement->IsFalling();
	if (bShouldSprint == bSprintActive)
	{
		return;
	}

	bSprintActive = bShouldSprint;
	Movement->MaxWalkSpeed = bSprintActive ? 680.0f : DefaultWalkSpeed;
	Movement->MaxAcceleration = bSprintActive ? 2350.0f : 1850.0f;
}

void AWorldHubLayout::HandlePortalOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	const int32 OtherBodyIndex,
	const bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bTravelRequested || OtherActor != UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		return;
	}

	UWorldTravelSubsystem* TravelSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UWorldTravelSubsystem>()
		: nullptr;
	UWorldDefinition* DestinationWorld = TravelSubsystem
		? TravelSubsystem->GetWorldDefinition(UWorldTravelSubsystem::EasternHorrorWorldId)
		: nullptr;
	if (!TravelSubsystem || !DestinationWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("Automatic W00 portal travel failed: destination is unavailable."));
		return;
	}

	bTravelRequested = true;
	PortalTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalLight->SetIntensity(18000.0f);
	PendingDestinationWorld = DestinationWorld;
	if (ACharacter* PlayerCharacter = Cast<ACharacter>(OtherActor))
	{
		PlayerCharacter->GetCharacterMovement()->StopMovementImmediately();
		PlayerCharacter->GetCharacterMovement()->DisableMovement();
	}
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(
				0.0f,
				1.0f,
				0.42f,
				FLinearColor(0.03f, 0.58f, 1.0f),
				false,
				true);
		}
	}
	GetWorldTimerManager().SetTimer(
		PortalTravelTimer,
		this,
		&AWorldHubLayout::CompletePortalTravel,
		0.42f,
		false);
}

void AWorldHubLayout::CompletePortalTravel()
{
	UWorldTravelSubsystem* TravelSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UWorldTravelSubsystem>()
		: nullptr;
	if (TravelSubsystem && PendingDestinationWorld && TravelSubsystem->TravelToWorld(PendingDestinationWorld))
	{
		return;
	}

	UE_LOG(LogTemp, Error, TEXT("Automatic W00 portal travel failed after the transition started."));
	bTravelRequested = false;
	PendingDestinationWorld = nullptr;
	PortalTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PortalLight->SetIntensity(1500.0f);
	if (ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		PlayerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraFade(
				1.0f,
				0.0f,
				0.2f,
				FLinearColor::Black,
				false,
				false);
		}
	}
}

FVector AWorldHubLayout::GetActivePortalLocation() const
{
	return GetActorTransform().TransformPosition(ActivePortalLocalLocation);
}

FVector AWorldHubLayout::GetSpiralTowerPortalLocation() const
{
	return GetActorTransform().TransformPosition(SpiralTowerPortalLocalLocation);
}
