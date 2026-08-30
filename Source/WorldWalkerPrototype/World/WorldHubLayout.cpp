#include "World/WorldHubLayout.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Characters/WorldWalkerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
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
#include "Engine/TextureCube.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "World/WorldDefinition.h"
#include "World/WorldPortal.h"
#include "World/WorldTravelSubsystem.h"

const FVector AWorldHubLayout::ActivePortalLocalLocation(-100.0f, 0.0f, 0.0f);
const FVector AWorldHubLayout::SpiralTowerPortalLocalLocation(-400.0f, 720.0f, 0.0f);

AWorldHubLayout::AWorldHubLayout()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SkySphereMesh(
		TEXT("/Engine/MapTemplates/Sky/SM_SkySphere.SM_SkySphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SkyDetailSphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> NightSkyMaterial(
		TEXT("/Game/WorldWalker/Worlds/W00_MainWorld/ThirdParty/PolyHaven/Sky/M_W00_QwantaniMoonNoonSky.M_W00_QwantaniMoonNoonSky"));
	static ConstructorHelpers::FObjectFinder<UTextureCube> NightSkyCubemap(
		TEXT("/Game/WorldWalker/Worlds/W00_MainWorld/ThirdParty/PolyHaven/Sky/T_W00_QwantaniMoonNoon.T_W00_QwantaniMoonNoon"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PortalFoundationMesh(
		TEXT("/Game/Asian_Village/meshes/props/SM_statue_platform_01.SM_statue_platform_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> EasternArchMesh(
		TEXT("/Game/Asian_Village/meshes/building/SM_wall_wooden_arch_01.SM_wall_wooden_arch_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> EasternColumnMesh(
		TEXT("/Game/Asian_Village/meshes/building/SM_column_06.SM_column_06"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CanopyMesh(
		TEXT("/Game/Asian_Village/meshes/building/SM_roof_canopy_01.SM_roof_canopy_01"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PortalDiskMesh(
		TEXT("/Game/Portals/StaticMesh/SM_RadialDisk.SM_RadialDisk"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PortalOuterMaterial(
		TEXT("/Game/Portals/Materials/MI_OuterPortal.MI_OuterPortal"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PortalWaterMaterial(
		TEXT("/Game/Portals/Materials/MI_PortalWater.MI_PortalWater"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PortalBackdropBaseMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
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
	static ConstructorHelpers::FObjectFinder<UParticleSystem> FirefliesSystem(
		TEXT("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Environment/P_Fireflies.P_Fireflies"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> EmissiveSkyDetailMaterial(
		TEXT("/Game/WorldWalker/Worlds/W00_MainWorld/Materials/M_W00_EmissiveSkyDetail.M_W00_EmissiveSkyDetail"));

	NightSkySphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NightSkySphere"));
	NightSkySphere->SetupAttachment(SceneRoot);
	NightSkySphere->SetRelativeLocation(FVector(0.0f, 0.0f, -1000.0f));
	NightSkySphere->SetRelativeRotation(FRotator(0.0f, 18.0f, 0.0f));
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

	NightMoon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("W00NightMoon"));
	NightMoon->SetupAttachment(SceneRoot);
	NightMoon->SetRelativeLocation(FVector(2500.0f, 4000.0f, 3200.0f));
	NightMoon->SetRelativeScale3D(FVector(2.7f));
	NightMoon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NightMoon->SetCollisionResponseToAllChannels(ECR_Ignore);
	NightMoon->SetCastShadow(false);
	NightMoon->SetReceivesDecals(false);
	if (SkyDetailSphereMesh.Succeeded())
	{
		NightMoon->SetStaticMesh(SkyDetailSphereMesh.Object);
	}
	if (EmissiveSkyDetailMaterial.Succeeded())
	{
		NightMoon->SetMaterial(0, EmissiveSkyDetailMaterial.Object);
	}

	NightStars = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("W00NightStars"));
	NightStars->SetupAttachment(SceneRoot);
	NightStars->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NightStars->SetCollisionResponseToAllChannels(ECR_Ignore);
	NightStars->SetCastShadow(false);
	NightStars->SetReceivesDecals(false);
	NightStars->SetBoundsScale(2.0f);
	if (SkyDetailSphereMesh.Succeeded())
	{
		NightStars->SetStaticMesh(SkyDetailSphereMesh.Object);
		for (int32 StarIndex = 0; StarIndex < 56; ++StarIndex)
		{
			const float Azimuth = FMath::DegreesToRadians(
				FMath::Fmod(29.0f + static_cast<float>(StarIndex) * 137.508f, 360.0f));
			const float Elevation = FMath::DegreesToRadians(
				18.0f + static_cast<float>((StarIndex * 43) % 59));
			const float Distance = 3300.0f + static_cast<float>((StarIndex * 127) % 1600);
			const FVector Direction(
				FMath::Cos(Elevation) * FMath::Cos(Azimuth),
				FMath::Cos(Elevation) * FMath::Sin(Azimuth),
				FMath::Sin(Elevation));
			const float StarScale = 0.042f
				+ static_cast<float>((StarIndex * 23) % 6) * 0.011f;
			NightStars->AddInstance(FTransform(
				FQuat::Identity,
				FVector(0.0f, 0.0f, 160.0f) + Direction * Distance,
				FVector(StarScale)));
		}
	}
	if (EmissiveSkyDetailMaterial.Succeeded())
	{
		NightStars->SetMaterial(0, EmissiveSkyDetailMaterial.Object);
	}

	MoonLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("MoonLight"));
	MoonLight->SetupAttachment(SceneRoot);
	MoonLight->SetRelativeRotation(FRotator(-34.0f, -42.0f, 0.0f));
	MoonLight->SetMobility(EComponentMobility::Movable);
	MoonLight->SetIntensity(0.55f);
	MoonLight->SetLightColor(FLinearColor(0.43f, 0.58f, 1.0f));
	MoonLight->SetCastShadows(true);

	NightSkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("NightSkyLight"));
	NightSkyLight->SetupAttachment(SceneRoot);
	NightSkyLight->SetMobility(EComponentMobility::Movable);
	NightSkyLight->SourceType = SLS_SpecifiedCubemap;
	NightSkyLight->bRealTimeCapture = false;
	NightSkyLight->bLowerHemisphereIsBlack = false;
	NightSkyLight->SetLowerHemisphereColor(FLinearColor(0.014f, 0.022f, 0.052f));
	NightSkyLight->SetLightColor(FLinearColor(0.66f, 0.76f, 1.0f));
	NightSkyLight->SetIntensity(0.20f);
	if (NightSkyCubemap.Succeeded())
	{
		NightSkyLight->SetCubemap(NightSkyCubemap.Object);
	}

	NightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("NightFog"));
	NightFog->SetupAttachment(SceneRoot);
	NightFog->SetFogDensity(0.0035f);
	NightFog->SetFogHeightFalloff(0.08f);
	NightFog->SetStartDistance(1600.0f);
	NightFog->SetFogInscatteringColor(FLinearColor(0.025f, 0.055f, 0.13f));
	NightFog->SetVolumetricFog(true);
	NightFog->SetVolumetricFogScatteringDistribution(0.35f);
	NightFog->SetVolumetricFogExtinctionScale(0.7f);
	NightFog->SetVolumetricFogAlbedo(FColor(78, 105, 155));
	NightFog->SetVolumetricFogEmissive(FLinearColor(0.002f, 0.004f, 0.012f));
	NightFog->SetVolumetricFogDistance(90000.0f);

	NightPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("NightPostProcess"));
	NightPostProcess->SetupAttachment(SceneRoot);
	NightPostProcess->bUnbound = true;
	NightPostProcess->bEnabled = true;
	NightPostProcess->Priority = 100.0f;
	NightPostProcess->BlendWeight = 1.0f;
	NightPostProcess->Settings.bOverride_AutoExposureBias = true;
	NightPostProcess->Settings.bOverride_AutoExposureMethod = true;
	NightPostProcess->Settings.AutoExposureMethod = AEM_Histogram;
	NightPostProcess->Settings.AutoExposureBias = -1.55f;
	NightPostProcess->Settings.bOverride_ColorGain = true;
	NightPostProcess->Settings.ColorGain = FVector4(0.78f, 0.82f, 0.94f, 1.0f);
	NightPostProcess->Settings.bOverride_SceneColorTint = true;
	NightPostProcess->Settings.SceneColorTint = FLinearColor(0.92f, 0.96f, 1.0f);
	NightPostProcess->Settings.bOverride_BloomIntensity = true;
	NightPostProcess->Settings.BloomIntensity = 0.12f;
	NightPostProcess->Settings.bOverride_VignetteIntensity = true;
	NightPostProcess->Settings.VignetteIntensity = 0.12f;

	PortalFoundation = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalFoundation"));
	PortalFoundation->SetupAttachment(SceneRoot);
	PortalFoundation->SetRelativeLocation(ActivePortalLocalLocation + FVector(0.0f, 0.0f, 4.0f));
	PortalFoundation->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	PortalFoundation->SetRelativeScale3D(FVector(1.25f, 1.25f, 0.62f));
	PortalFoundation->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (PortalFoundationMesh.Succeeded())
	{
		PortalFoundation->SetStaticMesh(PortalFoundationMesh.Object);
	}

	PortalPillarLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalPillarLeft"));
	PortalPillarLeft->SetupAttachment(SceneRoot);
	PortalPillarLeft->SetRelativeLocation(ActivePortalLocalLocation + FVector(0.0f, -225.0f, 8.0f));
	PortalPillarLeft->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	PortalPillarLeft->SetRelativeScale3D(FVector(0.78f));
	PortalPillarLeft->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (EasternColumnMesh.Succeeded())
	{
		PortalPillarLeft->SetStaticMesh(EasternColumnMesh.Object);
	}

	PortalPillarRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalPillarRight"));
	PortalPillarRight->SetupAttachment(SceneRoot);
	PortalPillarRight->SetRelativeLocation(ActivePortalLocalLocation + FVector(0.0f, 225.0f, 8.0f));
	PortalPillarRight->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	PortalPillarRight->SetRelativeScale3D(FVector(0.78f));
	PortalPillarRight->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (EasternColumnMesh.Succeeded())
	{
		PortalPillarRight->SetStaticMesh(EasternColumnMesh.Object);
	}

	PortalCanopy = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalCanopy"));
	PortalCanopy->SetupAttachment(SceneRoot);
	PortalCanopy->SetRelativeLocation(ActivePortalLocalLocation + FVector(-4.0f, 0.0f, 285.0f));
	PortalCanopy->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	PortalCanopy->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.52f));
	PortalCanopy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CanopyMesh.Succeeded())
	{
		PortalCanopy->SetStaticMesh(CanopyMesh.Object);
	}

	PortalBackdrop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalBackdrop"));
	PortalBackdrop->SetupAttachment(SceneRoot);
	PortalBackdrop->SetRelativeLocation(ActivePortalLocalLocation + FVector(11.0f, 0.0f, 135.0f));
	PortalBackdrop->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	PortalBackdrop->SetRelativeScale3D(FVector(0.92f));
	PortalBackdrop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalBackdrop->SetCastShadow(false);
	PortalBackdrop->SetTranslucentSortPriority(3);
	if (PortalDiskMesh.Succeeded())
	{
		PortalBackdrop->SetStaticMesh(PortalDiskMesh.Object);
	}
	if (PortalBackdropBaseMaterial.Succeeded())
	{
		PortalBackdrop->SetMaterial(0, PortalBackdropBaseMaterial.Object);
	}

	PortalDisk = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalDisk"));
	PortalDisk->SetupAttachment(SceneRoot);
	PortalDisk->SetRelativeLocation(ActivePortalLocalLocation + FVector(7.0f, 0.0f, 135.0f));
	PortalDisk->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	PortalDisk->SetRelativeScale3D(FVector(0.92f));
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

	PortalOuterRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalOuterRing"));
	PortalOuterRing->SetupAttachment(SceneRoot);
	PortalOuterRing->SetRelativeLocation(ActivePortalLocalLocation + FVector(5.0f, 0.0f, 135.0f));
	PortalOuterRing->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	PortalOuterRing->SetRelativeScale3D(FVector(1.18f));
	PortalOuterRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalOuterRing->SetCastShadow(false);
	PortalOuterRing->SetTranslucentSortPriority(4);
	if (PortalDiskMesh.Succeeded())
	{
		PortalOuterRing->SetStaticMesh(PortalDiskMesh.Object);
	}
	if (PortalOuterMaterial.Succeeded())
	{
		PortalOuterRing->SetMaterial(0, PortalOuterMaterial.Object);
	}

	PortalVortex = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalVortex"));
	PortalVortex->SetupAttachment(SceneRoot);
	PortalVortex->SetRelativeLocation(ActivePortalLocalLocation + FVector(0.0f, 0.0f, 135.0f));
	PortalVortex->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	PortalVortex->SetRelativeScale3D(FVector(0.10f));
	PortalVortex->SetAutoActivate(false);
	if (WaterPortalSystem.Succeeded())
	{
		PortalVortex->SetAsset(WaterPortalSystem.Object);
	}
	PortalVortex->SetSystemFixedBounds(FBox(FVector(-700.0f), FVector(700.0f)));

	PortalLightning = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalLightning"));
	PortalLightning->SetupAttachment(SceneRoot);
	PortalLightning->SetRelativeLocation(ActivePortalLocalLocation + FVector(-4.0f, 0.0f, 135.0f));
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
	PortalTrigger->SetRelativeLocation(ActivePortalLocalLocation + FVector(0.0f, 0.0f, 130.0f));
	PortalTrigger->SetBoxExtent(FVector(78.0f, 92.0f, 130.0f));
	PortalTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PortalTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	PortalTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	PortalTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PortalTrigger->OnComponentBeginOverlap.AddDynamic(this, &AWorldHubLayout::HandlePortalOverlap);

	PortalLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortalLight"));
	PortalLight->SetupAttachment(SceneRoot);
	PortalLight->SetRelativeLocation(ActivePortalLocalLocation + FVector(-55.0f, 0.0f, 140.0f));
	PortalLight->SetIntensity(250.0f);
	PortalLight->SetAttenuationRadius(390.0f);
	PortalLight->SetLightColor(FLinearColor(0.10f, 0.72f, 1.0f));
	PortalLight->SetCastShadows(false);

	PortalInstruction = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PortalInstruction"));
	PortalInstruction->SetupAttachment(SceneRoot);
	PortalInstruction->SetRelativeLocation(ActivePortalLocalLocation + FVector(-30.0f, 0.0f, 325.0f));
	PortalInstruction->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	PortalInstruction->SetHorizontalAlignment(EHTA_Center);
	PortalInstruction->SetVerticalAlignment(EVRTA_TextCenter);
	PortalInstruction->SetWorldSize(15.0f);
	PortalInstruction->SetTextRenderColor(FColor(135, 225, 255));
	PortalInstruction->SetText(FText::FromString(TEXT("ASHEN KINGDOM")));
	PortalInstruction->SetVisibility(false);
	PortalInstruction->SetHiddenInGame(true);

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
		ActivePortalLocalLocation + FVector(-30.0f, -165.0f, 278.0f),
		FRotator::ZeroRotator,
		FVector(0.82f));
	CreateShrineMesh(
		TEXT("PortalLanternRight"),
		LanternMesh.Succeeded() ? LanternMesh.Object : nullptr,
		ActivePortalLocalLocation + FVector(-30.0f, 165.0f, 278.0f),
		FRotator::ZeroRotator,
		FVector(0.82f));
	CreateShrineMesh(
		TEXT("PortalGuardianLeft"),
		StatueMesh.Succeeded() ? StatueMesh.Object : nullptr,
		ActivePortalLocalLocation + FVector(25.0f, -260.0f, 32.0f),
		FRotator(0.0f, 75.0f, 0.0f),
		FVector(0.48f));
	CreateShrineMesh(
		TEXT("PortalGuardianRight"),
		StatueMesh.Succeeded() ? StatueMesh.Object : nullptr,
		ActivePortalLocalLocation + FVector(25.0f, 260.0f, 32.0f),
		FRotator(0.0f, -75.0f, 0.0f),
		FVector(0.48f));
	CreateShrineMesh(
		TEXT("PortalBannerLeft"),
		BannerMesh.Succeeded() ? BannerMesh.Object : nullptr,
		ActivePortalLocalLocation + FVector(45.0f, -350.0f, 24.0f),
		FRotator(0.0f, 90.0f, 0.0f),
		FVector(0.65f));
	CreateShrineMesh(
		TEXT("PortalBannerRight"),
		BannerMesh.Succeeded() ? BannerMesh.Object : nullptr,
		ActivePortalLocalLocation + FVector(45.0f, 350.0f, 24.0f),
		FRotator(0.0f, -90.0f, 0.0f),
		FVector(0.65f));
	if (PortalShrineScenery.Num() >= 6)
	{
		// The distant flags amplified terrain-height differences and made the
		// compact shrine read as a cluttered prop pile. Keep the authored assets
		// available but hide them in this W00 composition.
		PortalShrineScenery[4]->SetVisibility(false);
		PortalShrineScenery[4]->SetHiddenInGame(true);
		PortalShrineScenery[5]->SetVisibility(false);
		PortalShrineScenery[5]->SetHiddenInGame(true);
	}

	const FVector ShrineLightLocations[] =
	{
		ActivePortalLocalLocation + FVector(-50.0f, -165.0f, 220.0f),
		ActivePortalLocalLocation + FVector(-50.0f, 165.0f, 220.0f)
	};
	for (int32 LightIndex = 0; LightIndex < UE_ARRAY_COUNT(ShrineLightLocations); ++LightIndex)
	{
		UPointLightComponent* ShrineLight = CreateDefaultSubobject<UPointLightComponent>(
			*FString::Printf(TEXT("ShrineLight_%d"), LightIndex));
		ShrineLight->SetupAttachment(SceneRoot);
		ShrineLight->SetRelativeLocation(ShrineLightLocations[LightIndex]);
		ShrineLight->SetIntensity(300.0f);
		ShrineLight->SetAttenuationRadius(270.0f);
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

	const FVector FireflyLocations[] =
	{
		ActivePortalLocalLocation + FVector(-190.0f, -360.0f, 120.0f),
		ActivePortalLocalLocation + FVector(-170.0f, 360.0f, 135.0f),
		ActivePortalLocalLocation + FVector(210.0f, 0.0f, 165.0f)
	};
	for (int32 FireflyIndex = 0; FireflyIndex < UE_ARRAY_COUNT(FireflyLocations); ++FireflyIndex)
	{
		UParticleSystemComponent* Fireflies = CreateDefaultSubobject<UParticleSystemComponent>(
			*FString::Printf(TEXT("AmbientFireflies_%d"), FireflyIndex));
		Fireflies->SetupAttachment(SceneRoot);
		Fireflies->SetRelativeLocation(FireflyLocations[FireflyIndex]);
		Fireflies->SetRelativeScale3D(FVector(0.78f));
		Fireflies->SetAutoActivate(true);
		Fireflies->SetBoundsScale(4.0f);
		if (FirefliesSystem.Succeeded())
		{
			Fireflies->SetTemplate(FirefliesSystem.Object);
		}
		AmbientFireflies.Add(Fireflies);
	}
}

void AWorldHubLayout::BeginPlay()
{
	Super::BeginPlay();

	SnapPortalToGround();
	ConfigureMainWorldEnvironment();
	if (NightSkySphere)
	{
		if (UMaterialInstanceDynamic* NightSkyMaterialInstance =
			NightSkySphere->CreateAndSetMaterialInstanceDynamic(0))
		{
			// Keep the HDR stars and moon highlights while preventing the diffuse
			// blue dome from reading as daytime after exposure adapts.
			NightSkyMaterialInstance->SetScalarParameterValue(TEXT("SkyBrightness"), 0.075f);
		}
	}
	if (NightMoon && NightMoon->GetMaterial(0))
	{
		if (UMaterialInstanceDynamic* MoonMaterial = UMaterialInstanceDynamic::Create(
			NightMoon->GetMaterial(0),
			NightMoon))
		{
			MoonMaterial->SetVectorParameterValue(
				TEXT("Color"),
				FLinearColor(0.48f, 0.64f, 1.0f) * 3.2f);
			NightMoon->SetMaterial(0, MoonMaterial);
		}
	}
	if (NightStars && NightStars->GetMaterial(0))
	{
		if (UMaterialInstanceDynamic* StarMaterial = UMaterialInstanceDynamic::Create(
			NightStars->GetMaterial(0),
			NightStars))
		{
			StarMaterial->SetVectorParameterValue(
				TEXT("Color"),
				FLinearColor(0.60f, 0.76f, 1.0f) * 12.0f);
			NightStars->SetMaterial(0, StarMaterial);
		}
	}
	ConfigureMainWorldAvatar();
	if (PortalBackdrop && PortalBackdrop->GetMaterial(0))
	{
		PortalBackdropMaterial = UMaterialInstanceDynamic::Create(
			PortalBackdrop->GetMaterial(0),
			this);
		if (PortalBackdropMaterial)
		{
			PortalBackdropMaterial->SetVectorParameterValue(
				TEXT("Color"),
				FLinearColor(0.006f, 0.025f, 0.085f));
			PortalBackdrop->SetMaterial(0, PortalBackdropMaterial);
		}
	}
	GetWorldTimerManager().SetTimerForNextTick(this, &AWorldHubLayout::HideLegacyPortal);
	GetWorldTimerManager().SetTimerForNextTick(this, &AWorldHubLayout::RefreshNightSky);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W00 night presentation ready. Avatar=%s W01Portal=%s W02Portal=%s Fireflies=%d Stars=%d Moon=%s"),
		bAvatarConfigured ? TEXT("anime-animated") : TEXT("prototype-fallback"),
		*GetActivePortalLocation().ToCompactString(),
		*GetSpiralTowerPortalLocation().ToCompactString(),
		AmbientFireflies.Num(),
		NightStars ? NightStars->GetInstanceCount() : 0,
		NightMoon && NightMoon->GetStaticMesh() ? TEXT("ready") : TEXT("missing"));
}

void AWorldHubLayout::ConfigureDestination(UWorldDefinition* InDestinationWorld)
{
	DestinationWorld = InDestinationWorld;
	if (PortalTrigger)
	{
		PortalTrigger->SetCollisionEnabled(
			DestinationWorld ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
	if (PortalInstruction && DestinationWorld)
	{
		PortalInstruction->SetText(DestinationWorld->DisplayName);
		PortalInstruction->SetTextRenderColor(DestinationWorld->PortalColor.ToFColor(true));
	}
	if (PortalLight && DestinationWorld)
	{
		PortalLight->SetLightColor(DestinationWorld->PortalColor);
	}
}

void AWorldHubLayout::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(PortalTravelTimer);
	ReleasePortalTransition(bTravelRequested && EndPlayReason != EEndPlayReason::LevelTransition);
	RestoreMainWorldAvatar();
	if (EndPlayReason != EEndPlayReason::LevelTransition && EndPlayReason != EEndPlayReason::Quit)
	{
		RestoreMainWorldEnvironment();
	}
	Super::EndPlay(EndPlayReason);
}

void AWorldHubLayout::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bTravelRequested)
	{
		ACharacter* CurrentCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
		if (CurrentCharacter && CurrentCharacter != MainWorldCharacter)
		{
			RestoreMainWorldAvatar();
			ConfigureMainWorldAvatar();
		}

		const bool bPlayerInsidePortal = CurrentCharacter
			&& PortalTrigger
			&& PortalTrigger->GetCollisionEnabled() != ECollisionEnabled::NoCollision
			&& PortalTrigger->Bounds.GetBox().IsInsideOrOn(CurrentCharacter->GetActorLocation());
		if (bPlayerInsidePortal && !bPortalProximityLatched)
		{
			FHitResult ProximityHit;
			HandlePortalOverlap(
				PortalTrigger,
				CurrentCharacter,
				CurrentCharacter->GetCapsuleComponent(),
				0,
				false,
				ProximityHit);
		}
		else if (!bPlayerInsidePortal)
		{
			bPortalProximityLatched = false;
		}
	}

	PortalPulseTime += DeltaSeconds;
	UpdateMainWorldMovement(DeltaSeconds);
	UpdatePortalTransition(DeltaSeconds);
	UpdatePortalPresentation(DeltaSeconds);
}

void AWorldHubLayout::UpdatePortalPresentation(const float DeltaSeconds)
{
	const float DistanceToPortal = MainWorldCharacter
		? FVector::Dist2D(MainWorldCharacter->GetActorLocation(), GetActivePortalLocation())
		: 1200.0f;
	const float ProximityAlpha = 1.0f - FMath::GetMappedRangeValueClamped(
		FVector2D(120.0f, 950.0f),
		FVector2D(0.0f, 1.0f),
		DistanceToPortal);
	const float TransitionAlpha = bTravelRequested
		? FMath::Clamp(PortalTransitionElapsed / PortalTransitionDuration, 0.0f, 1.0f)
		: 0.0f;

	const float DiskSpeed = 18.0f + ProximityAlpha * 44.0f + TransitionAlpha * 150.0f;
	if (PortalDisk)
	{
		PortalDisk->AddLocalRotation(FRotator(0.0f, -DiskSpeed * DeltaSeconds, 0.0f));
		const float DiskPulse = 0.92f
			+ FMath::Sin(PortalPulseTime * 1.9f) * 0.018f
			+ ProximityAlpha * 0.035f;
		PortalDisk->SetRelativeScale3D(FVector(DiskPulse));
	}
	if (SpiralTowerPortalDisk)
	{
		SpiralTowerPortalDisk->AddLocalRotation(FRotator(0.0f, 32.0f * DeltaSeconds, 0.0f));
	}
	if (PortalOuterRing)
	{
		PortalOuterRing->AddLocalRotation(FRotator(0.0f, DiskSpeed * 0.85f * DeltaSeconds, 0.0f));
		const float RingPulse = 1.18f
			+ FMath::Sin(PortalPulseTime * 1.35f + 0.7f) * 0.014f
			+ ProximityAlpha * 0.025f;
		PortalOuterRing->SetRelativeScale3D(FVector(RingPulse));
	}
	if (PortalBackdrop)
	{
		PortalBackdrop->AddLocalRotation(FRotator(0.0f, DiskSpeed * 0.32f * DeltaSeconds, 0.0f));
	}
	if (PortalVortex)
	{
		if (PortalVortex->IsActive())
		{
			PortalVortex->Deactivate();
		}
	}
	if (PortalLight)
	{
		const float Pulse = FMath::Sin(PortalPulseTime * 2.4f) * 90.0f;
		PortalLight->SetIntensity(
			250.0f + ProximityAlpha * 170.0f + TransitionAlpha * 600.0f + Pulse * 0.32f);
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
			const float Flicker = FMath::Sin(PortalPulseTime * (3.7f + LightIndex * 0.35f)) * 38.0f;
			ShrineLight->SetIntensity(300.0f + Flicker * 0.65f);
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

void AWorldHubLayout::SnapPortalToGround()
{
	if (!GetWorld())
	{
		return;
	}

	const FVector InitialPortalLocation = GetActivePortalLocation();
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(W00PortalGround), false, this);
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (PlayerCharacter)
	{
		QueryParams.AddIgnoredActor(PlayerCharacter);
	}

	auto TraceGround = [this, &QueryParams](const FVector& SampleLocation, FHitResult& OutHit)
	{
		return GetWorld()->LineTraceSingleByChannel(
			OutHit,
			SampleLocation + FVector(0.0f, 0.0f, 1200.0f),
			SampleLocation - FVector(0.0f, 0.0f, 2500.0f),
			ECC_Visibility,
			QueryParams);
	};

	FHitResult GroundHit;
	bool bGroundFound = false;
	if (PlayerCharacter)
	{
		const float CapsuleHalfHeight = PlayerCharacter->GetCapsuleComponent()
			? PlayerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
			: 88.0f;
		const FVector PlayerGround = PlayerCharacter->GetActorLocation()
			- FVector(0.0f, 0.0f, CapsuleHalfHeight);
		const FVector PlayerForward = PlayerCharacter->GetActorForwardVector().GetSafeNormal2D();
		const FVector PlayerRight = FVector::CrossProduct(FVector::UpVector, PlayerForward).GetSafeNormal();
		struct FPortalPlacementCandidate
		{
			float ForwardDistance;
			float LateralDistance;
		};
		const FPortalPlacementCandidate Candidates[] =
		{
			{1800.0f, 0.0f},
			{1675.0f, -650.0f},
			{1675.0f, 650.0f},
			{1500.0f, -650.0f},
			{1500.0f, 650.0f},
			{1200.0f, -500.0f},
			{1200.0f, 500.0f},
			{1200.0f, 0.0f}
		};

		for (const FPortalPlacementCandidate& Candidate : Candidates)
		{
			FVector CandidatePoint = PlayerGround
				+ PlayerForward * Candidate.ForwardDistance
				+ PlayerRight * Candidate.LateralDistance;
			FHitResult CenterHit;
			if (!TraceGround(CandidatePoint, CenterHit) || CenterHit.ImpactNormal.Z < 0.72f)
			{
				continue;
			}

			const FVector ApproachDirection = (CenterHit.ImpactPoint - PlayerGround).GetSafeNormal2D();
			const FVector ShrineRight = FVector::CrossProduct(FVector::UpVector, ApproachDirection).GetSafeNormal();
			FHitResult LeftHit;
			FHitResult RightHit;
			if (!TraceGround(CandidatePoint - ShrineRight * 285.0f, LeftHit)
				|| !TraceGround(CandidatePoint + ShrineRight * 285.0f, RightHit)
				|| LeftHit.ImpactNormal.Z < 0.72f
				|| RightHit.ImpactNormal.Z < 0.72f
				|| FMath::Abs(LeftHit.ImpactPoint.Z - CenterHit.ImpactPoint.Z) > 55.0f
				|| FMath::Abs(RightHit.ImpactPoint.Z - CenterHit.ImpactPoint.Z) > 55.0f)
			{
				continue;
			}

			FHitResult PathHit;
			const FVector PathStart = PlayerGround + FVector(0.0f, 0.0f, 115.0f);
			const FVector PathEnd = CenterHit.ImpactPoint + FVector(0.0f, 0.0f, 115.0f);
			if (GetWorld()->LineTraceSingleByChannel(
				PathHit,
				PathStart,
				PathEnd,
				ECC_Visibility,
				QueryParams)
				&& FVector::DistSquared2D(PathHit.ImpactPoint, CenterHit.ImpactPoint) > FMath::Square(220.0f))
			{
				continue;
			}

			const FRotator ShrineRotation(0.0f, ApproachDirection.Rotation().Yaw, 0.0f);
			const FVector LocalPortalOffset = ShrineRotation.RotateVector(ActivePortalLocalLocation);
			SetActorLocationAndRotation(
				CenterHit.ImpactPoint - LocalPortalOffset,
				ShrineRotation,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
			GroundHit = CenterHit;
			bGroundFound = true;
			UE_LOG(
				LogTemp,
				Display,
				TEXT("W00 portal placement selected. Forward=%.0f Lateral=%.0f Distance=%.0f"),
				Candidate.ForwardDistance,
				Candidate.LateralDistance,
				FVector::Dist2D(PlayerGround, CenterHit.ImpactPoint));
			break;
		}
	}

	if (!bGroundFound)
	{
		bGroundFound = TraceGround(InitialPortalLocation, GroundHit);
	}
	if (!bGroundFound || GroundHit.ImpactNormal.Z < 0.55f)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("W00 portal ground alignment skipped. GroundHit=%s NormalZ=%.2f Portal=%s"),
			bGroundFound ? TEXT("invalid") : TEXT("none"),
			GroundHit.ImpactNormal.Z,
			*InitialPortalLocation.ToCompactString());
		return;
	}

	FVector AlignedActorLocation = GetActorLocation();
	AlignedActorLocation.Z += GroundHit.ImpactPoint.Z - GetActivePortalLocation().Z;
	SetActorLocation(AlignedActorLocation, false, nullptr, ETeleportType::TeleportPhysics);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W00 portal ground aligned. PortalGroundHit=%s NormalZ=%.2f"),
		*GetActivePortalLocation().ToCompactString(),
		GroundHit.ImpactNormal.Z);
}

void AWorldHubLayout::ConfigureMainWorldEnvironment()
{
	if (bEnvironmentConfigured)
	{
		return;
	}
	bEnvironmentConfigured = true;

	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		if (UDirectionalLightComponent* ExistingSun = Cast<UDirectionalLightComponent>(It->GetLightComponent()))
		{
			EnvironmentDirectionalLights.Add(ExistingSun);
			OriginalDirectionalLightVisibility.Add(ExistingSun->GetVisibleFlag());
			ExistingSun->SetVisibility(false);
		}
	}

	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
	{
		if (USkyLightComponent* ExistingSkyLight = It->GetLightComponent())
		{
			EnvironmentSkyLights.Add(ExistingSkyLight);
			OriginalSkyLightVisibility.Add(ExistingSkyLight->GetVisibleFlag());
			ExistingSkyLight->SetVisibility(false);
		}
	}

	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
	{
		if (UExponentialHeightFogComponent* ExistingFog = It->GetComponent())
		{
			EnvironmentFogComponents.Add(ExistingFog);
			OriginalFogVisibility.Add(ExistingFog->GetVisibleFlag());
			ExistingFog->SetVisibility(false);
		}
	}

	// The vendor demo ships with a cinematic unbound volume tuned for its
	// daylight showcase.  Leaving it active stacks depth/bloom treatment over
	// the W00 night grade and turns the portal particles into a grey screen-edge
	// veil, so the W00-owned post-process component is the sole authority.
	for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
	{
		EnvironmentPostProcessVolumes.Add(*It);
		OriginalPostProcessEnabled.Add(It->bEnabled);
		OriginalPostProcessBlendWeights.Add(It->BlendWeight);
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
			HiddenEnvironmentActors.Add(Candidate);
			OriginalEnvironmentActorHidden.Add(Candidate->IsHidden());
			Candidate->SetActorHiddenInGame(true);
			continue;
		}

		if (Candidate->GetClass()->GetName().Contains(TEXT("BP_Sky_Sphere")))
		{
			HiddenEnvironmentActors.Add(Candidate);
			OriginalEnvironmentActorHidden.Add(Candidate->IsHidden());
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
				HiddenEnvironmentMeshes.Add(StaticMeshComponent);
				OriginalEnvironmentMeshVisibility.Add(StaticMeshComponent->GetVisibleFlag());
				OriginalEnvironmentMeshHiddenInGame.Add(StaticMeshComponent->bHiddenInGame);
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

	bAvatarConfigured = MainWorldCharacter->ConfigureMainWorldAnimeForm();
	if (!bAvatarConfigured)
	{
		UE_LOG(LogTemp, Error, TEXT("W00 anime avatar configuration is incomplete."));
	}
	ConfigureMainWorldMovementAndCamera();

}

void AWorldHubLayout::ConfigureMainWorldMovementAndCamera()
{
	if (!MainWorldCharacter)
	{
		return;
	}

	if (UCharacterMovementComponent* Movement = MainWorldCharacter->GetCharacterMovement())
	{
		if (!bMovementSnapshotValid)
		{
			DefaultWalkSpeed = Movement->MaxWalkSpeed;
			DefaultMaxAcceleration = Movement->MaxAcceleration;
			DefaultBrakingDeceleration = Movement->BrakingDecelerationWalking;
			DefaultGroundFriction = Movement->GroundFriction;
			DefaultJumpZVelocity = Movement->JumpZVelocity;
			DefaultAirControl = Movement->AirControl;
			bMovementSnapshotValid = true;
		}
		Movement->MaxAcceleration = 1850.0f;
		Movement->BrakingDecelerationWalking = 1250.0f;
		Movement->GroundFriction = 6.0f;
		Movement->JumpZVelocity = 520.0f;
		Movement->AirControl = 0.45f;
	}

	MainWorldCamera = MainWorldCharacter->FindComponentByClass<UCameraComponent>();
	MainWorldCameraBoom = MainWorldCharacter->FindComponentByClass<USpringArmComponent>();
	if (MainWorldCamera && MainWorldCameraBoom)
	{
		if (!bCameraSnapshotValid)
		{
			DefaultCameraFOV = MainWorldCamera->FieldOfView;
			DefaultCameraArmLength = MainWorldCameraBoom->TargetArmLength;
			DefaultCameraLagSpeed = MainWorldCameraBoom->CameraLagSpeed;
			DefaultCameraLagMaxDistance = MainWorldCameraBoom->CameraLagMaxDistance;
			DefaultCameraRotationLagSpeed = MainWorldCameraBoom->CameraRotationLagSpeed;
			bOriginalCameraLagEnabled = MainWorldCameraBoom->bEnableCameraLag;
			bOriginalCameraRotationLagEnabled = MainWorldCameraBoom->bEnableCameraRotationLag;
			bCameraSnapshotValid = true;
		}
		const bool bAvatarCapture = FParse::Param(
			FCommandLine::Get(), TEXT("WWAvatarCapture"));
		MainWorldCameraBoom->TargetArmLength = bAvatarCapture ? 120.0f : 410.0f;
		if (bAvatarCapture && MainWorldCharacter->GetController())
		{
			MainWorldCharacter->GetController()->SetControlRotation(FRotator(
				-8.0f,
				MainWorldCharacter->GetActorRotation().Yaw + 180.0f,
				0.0f));
		}
		MainWorldCameraBoom->bEnableCameraLag = !bAvatarCapture;
		MainWorldCameraBoom->CameraLagSpeed = 12.0f;
		MainWorldCameraBoom->CameraLagMaxDistance = 28.0f;
		MainWorldCameraBoom->bEnableCameraRotationLag = !bAvatarCapture;
		MainWorldCameraBoom->CameraRotationLagSpeed = 16.0f;
	}
}

void AWorldHubLayout::RestoreMainWorldEnvironment()
{
	for (int32 Index = 0; Index < EnvironmentDirectionalLights.Num(); ++Index)
	{
		if (EnvironmentDirectionalLights[Index])
		{
			EnvironmentDirectionalLights[Index]->SetVisibility(
				OriginalDirectionalLightVisibility.IsValidIndex(Index)
					? OriginalDirectionalLightVisibility[Index]
					: true);
		}
	}
	for (int32 Index = 0; Index < EnvironmentSkyLights.Num(); ++Index)
	{
		if (EnvironmentSkyLights[Index])
		{
			EnvironmentSkyLights[Index]->SetVisibility(
				OriginalSkyLightVisibility.IsValidIndex(Index)
					? OriginalSkyLightVisibility[Index]
					: true);
		}
	}
	for (int32 Index = 0; Index < EnvironmentFogComponents.Num(); ++Index)
	{
		if (EnvironmentFogComponents[Index])
		{
			EnvironmentFogComponents[Index]->SetVisibility(
				OriginalFogVisibility.IsValidIndex(Index)
					? OriginalFogVisibility[Index]
					: true);
		}
	}
	for (int32 Index = 0; Index < EnvironmentPostProcessVolumes.Num(); ++Index)
	{
		if (EnvironmentPostProcessVolumes[Index])
		{
			EnvironmentPostProcessVolumes[Index]->bEnabled =
				OriginalPostProcessEnabled.IsValidIndex(Index)
					? OriginalPostProcessEnabled[Index]
					: true;
			EnvironmentPostProcessVolumes[Index]->BlendWeight =
				OriginalPostProcessBlendWeights.IsValidIndex(Index)
					? OriginalPostProcessBlendWeights[Index]
					: 1.0f;
		}
	}
	for (int32 Index = 0; Index < HiddenEnvironmentActors.Num(); ++Index)
	{
		if (HiddenEnvironmentActors[Index])
		{
			HiddenEnvironmentActors[Index]->SetActorHiddenInGame(
				OriginalEnvironmentActorHidden.IsValidIndex(Index)
					? OriginalEnvironmentActorHidden[Index]
					: false);
		}
	}
	for (int32 Index = 0; Index < HiddenEnvironmentMeshes.Num(); ++Index)
	{
		if (HiddenEnvironmentMeshes[Index])
		{
			HiddenEnvironmentMeshes[Index]->SetVisibility(
				OriginalEnvironmentMeshVisibility.IsValidIndex(Index)
					? OriginalEnvironmentMeshVisibility[Index]
					: true,
				true);
			HiddenEnvironmentMeshes[Index]->SetHiddenInGame(
				OriginalEnvironmentMeshHiddenInGame.IsValidIndex(Index)
					? OriginalEnvironmentMeshHiddenInGame[Index]
					: false,
				true);
		}
	}

	EnvironmentDirectionalLights.Reset();
	EnvironmentSkyLights.Reset();
	EnvironmentFogComponents.Reset();
	EnvironmentPostProcessVolumes.Reset();
	HiddenEnvironmentActors.Reset();
	HiddenEnvironmentMeshes.Reset();
	OriginalDirectionalLightVisibility.Reset();
	OriginalSkyLightVisibility.Reset();
	OriginalFogVisibility.Reset();
	OriginalPostProcessEnabled.Reset();
	OriginalPostProcessBlendWeights.Reset();
	OriginalEnvironmentActorHidden.Reset();
	OriginalEnvironmentMeshVisibility.Reset();
	OriginalEnvironmentMeshHiddenInGame.Reset();
	bEnvironmentConfigured = false;
}

void AWorldHubLayout::RestoreMainWorldAvatar()
{
	if (IsValid(MainWorldCharacter))
	{
		USkeletalMeshComponent* HeadComponent = MainWorldCharacter->GetMesh();
		for (USkeletalMeshComponent* AvatarPart : MainWorldAvatarParts)
		{
			if (AvatarPart && AvatarPart != HeadComponent)
			{
				AvatarPart->DestroyComponent();
			}
		}
		if (HeadComponent && bHeadSnapshotValid)
		{
			RestoreOriginalCharacterMesh(HeadComponent);
		}
		if (MainWorldPrototypeBody)
		{
			MainWorldPrototypeBody->SetVisibility(bOriginalPrototypeBodyVisible, true);
			MainWorldPrototypeBody->SetHiddenInGame(bOriginalPrototypeBodyHiddenInGame, true);
		}
		if (bMovementSnapshotValid)
		{
			if (UCharacterMovementComponent* Movement = MainWorldCharacter->GetCharacterMovement())
			{
				Movement->MaxWalkSpeed = DefaultWalkSpeed;
				Movement->MaxAcceleration = DefaultMaxAcceleration;
				Movement->BrakingDecelerationWalking = DefaultBrakingDeceleration;
				Movement->GroundFriction = DefaultGroundFriction;
				Movement->JumpZVelocity = DefaultJumpZVelocity;
				Movement->AirControl = DefaultAirControl;
			}
		}
		if (bCameraSnapshotValid && MainWorldCamera && MainWorldCameraBoom)
		{
			MainWorldCamera->SetFieldOfView(DefaultCameraFOV);
			MainWorldCameraBoom->TargetArmLength = DefaultCameraArmLength;
			MainWorldCameraBoom->CameraLagSpeed = DefaultCameraLagSpeed;
			MainWorldCameraBoom->CameraLagMaxDistance = DefaultCameraLagMaxDistance;
			MainWorldCameraBoom->CameraRotationLagSpeed = DefaultCameraRotationLagSpeed;
			MainWorldCameraBoom->bEnableCameraLag = bOriginalCameraLagEnabled;
			MainWorldCameraBoom->bEnableCameraRotationLag = bOriginalCameraRotationLagEnabled;
		}
	}

	MainWorldAvatarParts.Reset();
	MainWorldCamera = nullptr;
	MainWorldCameraBoom = nullptr;
	MainWorldPrototypeBody = nullptr;
	MainWorldCharacter = nullptr;
	OriginalCharacterMesh = nullptr;
	OriginalCharacterAnimClass = nullptr;
	bMovementSnapshotValid = false;
	bCameraSnapshotValid = false;
	bHeadSnapshotValid = false;
	bAvatarConfigured = false;
	SprintBlend = 0.0f;
	LandingResponse = 0.0f;
	bWasFalling = false;
	bSprintActive = false;
}

void AWorldHubLayout::RestoreOriginalCharacterMesh(USkeletalMeshComponent* HeadComponent)
{
	if (!HeadComponent || !bHeadSnapshotValid)
	{
		return;
	}

	HeadComponent->SetSkeletalMeshAsset(OriginalCharacterMesh);
	HeadComponent->SetRelativeTransform(OriginalCharacterMeshTransform);
	const EAnimationMode::Type OriginalAnimationMode =
		static_cast<EAnimationMode::Type>(OriginalCharacterAnimationMode);
	if (OriginalAnimationMode == EAnimationMode::AnimationBlueprint)
	{
		HeadComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		HeadComponent->SetAnimInstanceClass(OriginalCharacterAnimClass);
	}
	else
	{
		HeadComponent->SetAnimInstanceClass(nullptr);
		HeadComponent->SetAnimationMode(OriginalAnimationMode);
	}
	HeadComponent->VisibilityBasedAnimTickOption =
		static_cast<EVisibilityBasedAnimTickOption>(OriginalVisibilityBasedAnimTickOption);
	HeadComponent->SetCollisionEnabled(
		static_cast<ECollisionEnabled::Type>(OriginalCharacterCollisionEnabled));
	HeadComponent->SetVisibility(bOriginalCharacterMeshVisible, true);
	HeadComponent->SetHiddenInGame(bOriginalCharacterMeshHiddenInGame, true);
	bHeadSnapshotValid = false;
}

void AWorldHubLayout::ReleasePortalTransition(const bool bRestoreMovementAndFade)
{
	if (bRestoreMovementAndFade && IsValid(MainWorldCharacter))
	{
		MainWorldCharacter->SetActorLocationAndRotation(
			PortalEntryStartLocation,
			PortalEntryStartRotation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		if (UCharacterMovementComponent* Movement = MainWorldCharacter->GetCharacterMovement())
		{
			Movement->SetMovementMode(
				static_cast<EMovementMode>(PortalEntryMovementMode),
				PortalEntryCustomMovementMode);
			Movement->Velocity = PortalEntryVelocity;
		}
	}

	if (PortalTransitionController)
	{
		if (bPortalLookInputIgnored)
		{
			PortalTransitionController->SetIgnoreLookInput(false);
		}
		if (bRestoreMovementAndFade && PortalTransitionController->PlayerCameraManager)
		{
			PortalTransitionController->PlayerCameraManager->StartCameraFade(
				1.0f,
				0.0f,
				0.2f,
				FLinearColor(0.03f, 0.58f, 1.0f),
				false,
				false);
		}
	}
	bPortalLookInputIgnored = false;
	PortalTransitionController = nullptr;
}

USkeletalMeshComponent* AWorldHubLayout::CreateLinkedAvatarPart(
	ACharacter* PlayerCharacter,
	const FName ComponentName,
	USkeletalMesh* Mesh,
	USkeletalMeshComponent* PoseLeader)
{
	if (!PlayerCharacter || !PoseLeader || !Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("W00 avatar linked part could not be created: %s"), *ComponentName.ToString());
		return nullptr;
	}

	USkeletalMeshComponent* Part = NewObject<USkeletalMeshComponent>(PlayerCharacter, ComponentName);
	if (!Part)
	{
		return nullptr;
	}
	PlayerCharacter->AddInstanceComponent(Part);
	Part->SetupAttachment(PlayerCharacter->GetRootComponent());
	Part->SetSkeletalMeshAsset(Mesh);
	Part->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	Part->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetCastShadow(true);
	Part->RegisterComponent();
	Part->SetLeaderPoseComponent(PoseLeader, true, false);

	MainWorldAvatarParts.Add(Part);
	return Part;
}

USkeletalMeshComponent* AWorldHubLayout::CreateHairPart(
	ACharacter* PlayerCharacter,
	USkeletalMeshComponent* HeadComponent,
	USkeletalMesh* HairMesh)
{
	if (!PlayerCharacter || !HeadComponent || !HairMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("W00 avatar hair could not be created."));
		return nullptr;
	}

	USkeletalMeshComponent* Hair = NewObject<USkeletalMeshComponent>(PlayerCharacter, TEXT("MainWorldAvatarHair"));
	if (!Hair)
	{
		return nullptr;
	}
	PlayerCharacter->AddInstanceComponent(Hair);
	Hair->SetupAttachment(HeadComponent, TEXT("HeadAttachment"));
	Hair->SetSkeletalMeshAsset(HairMesh);
	Hair->SetRelativeTransform(FTransform::Identity);
	Hair->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hair->SetCastShadow(true);
	Hair->RegisterComponent();

	MainWorldAvatarParts.Add(Hair);
	return Hair;
}

void AWorldHubLayout::RefreshNightSky()
{
	if (NightSkyLight)
	{
		// Queue processing after the component is registered. This works for both
		// captured scenes and specified cubemaps, whereas assigning the same
		// Cubemap pointer again is intentionally a no-op in UE.
		NightSkyLight->RecaptureSky();
	}
}

void AWorldHubLayout::HideLegacyPortal()
{
	TArray<AWorldPortal*> LegacyPortals;
	TArray<AWorldPortal*> SpiralTowerPortals;
	for (TActorIterator<AWorldPortal> It(GetWorld()); It; ++It)
	{
		AWorldPortal* Portal = *It;
		if (!Portal)
		{
			continue;
		}

		if (FVector::DistSquared(Portal->GetActorLocation(), GetActivePortalLocation()) < FMath::Square(350.0f))
		{
			LegacyPortals.Add(Portal);
		}
		else if (FVector::DistSquared(Portal->GetActorLocation(), GetSpiralTowerPortalLocation()) < FMath::Square(350.0f))
		{
			SpiralTowerPortals.Add(Portal);
		}
	}

	for (AWorldPortal* LegacyPortal : LegacyPortals)
	{
		LegacyPortal->ConfigurePortal(nullptr);
		LegacyPortal->SetActorHiddenInGame(true);
		LegacyPortal->SetActorEnableCollision(false);
		LegacyPortal->SetActorTickEnabled(false);
	}
	for (AWorldPortal* SpiralTowerPortal : SpiralTowerPortals)
	{
		// W02's generic portal remains the interaction authority. Hide only its
		// fallback presentation so the custom W02 shrine can provide the visuals.
		SpiralTowerPortal->SetActorHiddenInGame(true);
		SpiralTowerPortal->SetActorEnableCollision(false);
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W00 disabled %d legacy W01 portal(s) and hid %d W02 fallback portal(s)."),
		LegacyPortals.Num(),
		SpiralTowerPortals.Num());
}

void AWorldHubLayout::UpdateMainWorldMovement(const float DeltaSeconds)
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
	bSprintActive = bShouldSprint;
	SprintBlend = FMath::FInterpTo(
		SprintBlend,
		bSprintActive ? 1.0f : 0.0f,
		DeltaSeconds,
		6.5f);

	const bool bFalling = Movement->IsFalling();
	if (bWasFalling && !bFalling)
	{
		LandingResponse = 1.0f;
	}
	bWasFalling = bFalling;
	LandingResponse = FMath::FInterpTo(LandingResponse, 0.0f, DeltaSeconds, 7.5f);

	Movement->MaxWalkSpeed = FMath::Lerp(DefaultWalkSpeed, 680.0f, SprintBlend);
	Movement->MaxAcceleration = FMath::Lerp(1850.0f, 2350.0f, SprintBlend);
	if (MainWorldCamera)
	{
		const float TargetFOV = DefaultCameraFOV + SprintBlend * 5.0f - LandingResponse * 1.5f;
		MainWorldCamera->SetFieldOfView(FMath::FInterpTo(
			MainWorldCamera->FieldOfView,
			TargetFOV,
			DeltaSeconds,
			8.0f));
	}
	if (MainWorldCameraBoom
		&& !FParse::Param(FCommandLine::Get(), TEXT("WWAvatarCapture")))
	{
		const float TargetArmLength = 410.0f + SprintBlend * 22.0f - LandingResponse * 10.0f;
		MainWorldCameraBoom->TargetArmLength = FMath::FInterpTo(
			MainWorldCameraBoom->TargetArmLength,
			TargetArmLength,
			DeltaSeconds,
			8.0f);
	}
}

void AWorldHubLayout::UpdatePortalTransition(const float DeltaSeconds)
{
	if (!bTravelRequested || !MainWorldCharacter)
	{
		return;
	}

	PortalTransitionElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(
		PortalTransitionElapsed / PortalTransitionDuration,
		0.0f,
		1.0f);
	const float SmoothAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
	FVector PortalEntryTarget = GetActivePortalLocation() + GetActorForwardVector() * 35.0f;
	PortalEntryTarget.Z = PortalEntryStartLocation.Z;
	MainWorldCharacter->SetActorLocation(
		FMath::Lerp(PortalEntryStartLocation, PortalEntryTarget, SmoothAlpha),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	const FVector PortalDirection = (GetActivePortalLocation() - PortalEntryStartLocation).GetSafeNormal2D();
	if (!PortalDirection.IsNearlyZero())
	{
		const FQuat TargetRotation = PortalDirection.Rotation().Quaternion();
		MainWorldCharacter->SetActorRotation(FQuat::Slerp(
			PortalEntryStartRotation.Quaternion(),
			TargetRotation,
			SmoothAlpha));
	}
	if (MainWorldCamera)
	{
		MainWorldCamera->SetFieldOfView(FMath::Lerp(DefaultCameraFOV, DefaultCameraFOV - 8.0f, SmoothAlpha));
	}
	if (MainWorldCameraBoom)
	{
		MainWorldCameraBoom->TargetArmLength = FMath::Lerp(410.0f, 355.0f, SmoothAlpha);
	}
}

void AWorldHubLayout::HandlePortalOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	const int32 OtherBodyIndex,
	const bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bTravelRequested
		|| bPortalProximityLatched
		|| OtherActor != UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		return;
	}
	bPortalProximityLatched = true;

	UWorldTravelSubsystem* TravelSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UWorldTravelSubsystem>()
		: nullptr;
	if (!TravelSubsystem || !DestinationWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("Automatic W00 portal travel failed: destination is unavailable."));
		bPortalProximityLatched = false;
		return;
	}

	bTravelRequested = true;
	PortalTransitionElapsed = 0.0f;
	PortalEntryStartLocation = OtherActor->GetActorLocation();
	PortalEntryStartRotation = OtherActor->GetActorRotation();
	if (PortalTrigger)
	{
		PortalTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	PendingDestinationWorld = DestinationWorld;
	if (ACharacter* PlayerCharacter = Cast<ACharacter>(OtherActor))
	{
		if (UCharacterMovementComponent* Movement = PlayerCharacter->GetCharacterMovement())
		{
			PortalEntryMovementMode = static_cast<uint8>(Movement->MovementMode);
			PortalEntryCustomMovementMode = Movement->CustomMovementMode;
			PortalEntryVelocity = Movement->Velocity;
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
	}
	PortalTransitionController = UGameplayStatics::GetPlayerController(this, 0);
	if (PortalTransitionController)
	{
		PortalTransitionController->SetIgnoreLookInput(true);
		bPortalLookInputIgnored = true;
		if (PortalTransitionController->PlayerCameraManager)
		{
			PortalTransitionController->PlayerCameraManager->StartCameraFade(
				0.0f,
				1.0f,
				PortalTransitionDuration,
				FLinearColor(0.03f, 0.58f, 1.0f),
				false,
				true);
		}
	}
	GetWorldTimerManager().SetTimer(
		PortalTravelTimer,
		this,
		&AWorldHubLayout::CompletePortalTravel,
		PortalTransitionDuration,
		false);
	UE_LOG(LogTemp, Display, TEXT("W00 automatic portal transition started."));
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
	ReleasePortalTransition(true);
	bTravelRequested = false;
	PortalTransitionElapsed = 0.0f;
	PendingDestinationWorld = nullptr;
	if (PortalTrigger)
	{
		PortalTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	if (PortalLight)
	{
		PortalLight->SetIntensity(250.0f);
	}
	if (MainWorldCamera)
	{
		MainWorldCamera->SetFieldOfView(DefaultCameraFOV);
	}
	if (MainWorldCameraBoom)
	{
		MainWorldCameraBoom->TargetArmLength = 410.0f;
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
