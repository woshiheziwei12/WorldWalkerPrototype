#include "World/Fantasy/FantasyWorldLayout.h"

#include "Characters/FantasyNPC.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureCube.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "World/Fantasy/Exploration/FantasyFateAltar.h"
#include "World/Fantasy/FantasyAmbientSoundscape.h"
#include "World/Fantasy/FantasyAmbientWispField.h"
#include "World/WorldPortal.h"

const FVector AFantasyWorldLayout::BattleAnchorLocalLocation(2320.0f, 0.0f, 0.0f);
const FVector AFantasyWorldLayout::ReturnPortalLocalLocation(-120.0f, 760.0f, 0.0f);

namespace
{
	const TCHAR* EnvironmentRoot =
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Environment");

	FString EnvironmentAssetPath(const TCHAR* AssetName)
	{
		return FString::Printf(TEXT("%s/%s/%s.%s"), EnvironmentRoot, AssetName, AssetName, AssetName);
	}
}

AFantasyWorldLayout::AFantasyWorldLayout()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// The template map's sky sphere is intentionally hidden at runtime because
	// it is too small for the W01 route and produces the partial-sky warning in
	// off-screen renders. A dedicated, very large sphere keeps every camera
	// inside W01's independently imported CC0 moon-night HDRI without changing
	// the proven moon/sky fill values below.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SkySphereMesh(
		TEXT("/Engine/MapTemplates/Sky/SM_SkySphere.SM_SkySphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SkyDetailSphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	NightSkySphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("W01NightSkySphere"));
	NightSkySphere->SetupAttachment(SceneRoot);
	NightSkySphere->SetRelativeLocation(FVector(1200.0f, 0.0f, -1000.0f));
	NightSkySphere->SetRelativeScale3D(FVector(100.0f));
	NightSkySphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NightSkySphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	NightSkySphere->SetCastShadow(false);
	NightSkySphere->SetReceivesDecals(false);
	NightSkySphere->SetBoundsScale(4.0f);
	NightSkySphere->SetVisibility(false);
	if (SkySphereMesh.Succeeded())
	{
		NightSkySphere->SetStaticMesh(SkySphereMesh.Object);
	}

	NightMoon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("W01NightMoon"));
	NightMoon->SetupAttachment(SceneRoot);
	NightMoon->SetRelativeLocation(FVector(3800.0f, 1550.0f, 1950.0f));
	NightMoon->SetRelativeScale3D(FVector(2.8f));
	NightMoon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NightMoon->SetCollisionResponseToAllChannels(ECR_Ignore);
	NightMoon->SetCastShadow(false);
	NightMoon->SetReceivesDecals(false);
	NightMoon->SetVisibility(false);
	if (SkyDetailSphereMesh.Succeeded())
	{
		NightMoon->SetStaticMesh(SkyDetailSphereMesh.Object);
	}

	NightStars = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("W01NightStars"));
	NightStars->SetupAttachment(SceneRoot);
	NightStars->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NightStars->SetCollisionResponseToAllChannels(ECR_Ignore);
	NightStars->SetCastShadow(false);
	NightStars->SetReceivesDecals(false);
	NightStars->SetBoundsScale(2.0f);
	NightStars->SetVisibility(false);
	if (SkyDetailSphereMesh.Succeeded())
	{
		NightStars->SetStaticMesh(SkyDetailSphereMesh.Object);
		for (int32 StarIndex = 0; StarIndex < 48; ++StarIndex)
		{
			const float Azimuth = FMath::DegreesToRadians(
				FMath::Fmod(17.0f + static_cast<float>(StarIndex) * 137.508f, 360.0f));
			const float Elevation = FMath::DegreesToRadians(
				20.0f + static_cast<float>((StarIndex * 47) % 55));
			const float Distance = 3100.0f + static_cast<float>((StarIndex * 113) % 1400);
			const FVector Direction(
				FMath::Cos(Elevation) * FMath::Cos(Azimuth),
				FMath::Cos(Elevation) * FMath::Sin(Azimuth),
				FMath::Sin(Elevation));
			const float StarScale = 0.060f
				+ static_cast<float>((StarIndex * 19) % 5) * 0.018f;
			NightStars->AddInstance(FTransform(
				FQuat::Identity,
				FVector(1200.0f, 0.0f, 120.0f) + Direction * Distance,
				FVector(StarScale)));
		}
	}
}

void AFantasyWorldLayout::BeginPlay()
{
	Super::BeginPlay();

	CubeFallback = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	CylinderFallback = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	ConeFallback = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	GroundMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Environment/SM_W01_Path/Stone_Dark.Stone_Dark"));
	RoadMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Environment/SM_W01_Path/Stone_Light.Stone_Light"));
	UMaterialInterface* AuthoredNightSky = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/PolyHaven/Sky/M_W01_QwantaniMoonNoonSkyNight.M_W01_QwantaniMoonNoonSkyNight"));
	if (NightSkySphere && AuthoredNightSky)
	{
		if (UMaterialInstanceDynamic* NightSkyMaterial = UMaterialInstanceDynamic::Create(
			AuthoredNightSky,
			NightSkySphere))
		{
			NightSkyMaterial->SetScalarParameterValue(TEXT("SkyBrightness"), 0.045f);
			NightSkyMaterial->SetVectorParameterValue(
				TEXT("SkyTint"),
				FLinearColor(0.20f, 0.35f, 1.0f));
			NightSkySphere->SetMaterial(0, NightSkyMaterial);
		}
		else
		{
			NightSkySphere->SetMaterial(0, AuthoredNightSky);
		}
		NightSkySphere->SetVisibility(true);
		UE_LOG(LogTemp, Display, TEXT("W01_HDRI_SKY_READY Material=night-graded"));
	}
	else
	{
		// A missing optional HDRI keeps the already-tested SkyAtmosphere gradient;
		// never expose the engine template material that renders as bright stripes.
		UE_LOG(LogTemp, Warning, TEXT("W01 optional HDRI sky material is missing; using atmosphere fallback."));
	}
	UMaterialInterface* EmissiveSkyDetailMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/Materials/M_W01_EmissiveSkyDetail.M_W01_EmissiveSkyDetail"));
	if (EmissiveSkyDetailMaterial)
	{
		if (UMaterialInstanceDynamic* MoonMaterial = UMaterialInstanceDynamic::Create(
			EmissiveSkyDetailMaterial,
			NightMoon))
		{
			MoonMaterial->SetVectorParameterValue(
				TEXT("Color"),
				FLinearColor(0.52f, 0.68f, 1.0f) * 1.2f);
			NightMoon->SetMaterial(0, MoonMaterial);
			NightMoon->SetVisibility(true);
		}
		if (UMaterialInstanceDynamic* StarMaterial = UMaterialInstanceDynamic::Create(
			EmissiveSkyDetailMaterial,
			NightStars))
		{
			StarMaterial->SetVectorParameterValue(
				TEXT("Color"),
				FLinearColor(0.62f, 0.76f, 1.0f) * 5.0f);
			NightStars->SetMaterial(0, StarMaterial);
			NightStars->SetVisibility(true);
		}
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("W01 optional sky-detail material is missing; moon and stars remain hidden."));
	}

	ConfigureWorldAtmosphere();
	BuildGroundAndRoad();
	BuildVillageDistrict();
	BuildCampAndLandmarks();
	BuildBattleApproach();
	BuildReturnPortalLandmark();
	SpawnResidents();
	SpawnWorldAmbienceAndEvent();
	GetWorldTimerManager().SetTimerForNextTick(
		this,
		&AFantasyWorldLayout::RefreshCapturedSky);
	GetWorldTimerManager().SetTimerForNextTick(
		this,
		&AFantasyWorldLayout::FinalizeReturnPortalPresentation);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Ashen Kingdom exploration layout ready. Meshes=%d Torches=%d Residents=%d"),
		EnvironmentMeshes.Num(),
		TorchLights.Num(),
		Residents.Num());
}

void AFantasyWorldLayout::SpawnWorldAmbienceAndEvent()
{
	if (!GetWorld())
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AmbientWispField = GetWorld()->SpawnActor<AFantasyAmbientWispField>(
		AFantasyAmbientWispField::StaticClass(),
		GetActorTransform(),
		SpawnParameters);
	AmbientSoundscape = GetWorld()->SpawnActor<AFantasyAmbientSoundscape>(
		AFantasyAmbientSoundscape::StaticClass(),
		GetActorTransform(),
		SpawnParameters);

	// Keep the one-shot event in the open clearing between the arrival point and
	// the village gate. It remains visible from the road without overlapping the
	// chapel, market, residents or the return portal's collision volume.
	const FVector AltarLocalLocation(120.0f, 470.0f, 0.0f);
	const FTransform AltarTransform(
		GetActorRotation() + FRotator(0.0f, -90.0f, 0.0f),
		GetActorTransform().TransformPosition(AltarLocalLocation));
	FateAltar = AFantasyFateAltar::SpawnConfigured(
		GetWorld(),
		AltarTransform,
		TEXT("AshenFateAltar"),
		FText::FromString(TEXT("灰烬命运碑")),
		FText::FromString(
			TEXT("焦黑石碑仍有心跳般的微光。三枚古老符文等待你刻下唯一的誓言；一旦选择，命运便不会回头。")));
	if (FateAltar)
	{
		FateAltar->SetOwner(this);
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Ashen Kingdom ambient layer ready. Wisps=%s Soundscape=%s FateAltar=%s"),
		AmbientWispField ? TEXT("spawned") : TEXT("missing"),
		AmbientSoundscape ? TEXT("spawned") : TEXT("missing"),
		FateAltar ? TEXT("spawned") : TEXT("missing"));
}

FVector AFantasyWorldLayout::GetBattleAnchorLocation() const
{
	return GetActorTransform().TransformPosition(BattleAnchorLocalLocation);
}

FVector AFantasyWorldLayout::GetReturnPortalLocation() const
{
	return GetActorTransform().TransformPosition(ReturnPortalLocalLocation);
}

FRotator AFantasyWorldLayout::GetForwardFacingRotation() const
{
	return GetActorRotation();
}

void AFantasyWorldLayout::ConfigureWorldAtmosphere()
{
	HideTemplateFloor();
	UTextureCube* AuthoredNightCubemap = LoadObject<UTextureCube>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/PolyHaven/Sky/T_W01_QwantaniMoonNoon.T_W01_QwantaniMoonNoon"));

	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		if (ULightComponent* Light = It->GetLightComponent())
		{
			// The template lights are authored as static components. Runtime-spawned
			// characters and scenery therefore received almost no moon fill until the
			// light was made movable. A higher moon angle keeps faces, the road and the
			// enemy readable while retaining long, distinctly nocturnal shadows.
			Light->SetMobility(EComponentMobility::Movable);
			Light->SetVisibility(true);
			It->SetActorHiddenInGame(false);
			It->SetActorRotation(FRotator(-36.0f, -32.0f, 0.0f));
			Light->SetIntensity(1.65f);
			Light->SetLightColor(FLinearColor(0.42f, 0.52f, 0.86f));
			Light->SetIndirectLightingIntensity(0.80f);
			Light->SetVolumetricScatteringIntensity(0.38f);
			if (UDirectionalLightComponent* Directional = Cast<UDirectionalLightComponent>(Light))
			{
				Directional->SetAtmosphereSunLight(true);
				Directional->SetAtmosphereSunDiskColorScale(FLinearColor(0.32f, 0.38f, 0.62f));
			}
		}
	}

	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
	{
		if (USkyLightComponent* SkyLight = It->GetLightComponent())
		{
			SkyLight->SetMobility(EComponentMobility::Movable);
			SkyLight->SetVisibility(true);
			It->SetActorHiddenInGame(false);
			SkyLight->SourceType = AuthoredNightCubemap
				? SLS_SpecifiedCubemap
				: SLS_CapturedScene;
			if (AuthoredNightCubemap)
			{
				// Keep character fill independent from the deliberately dark visible
				// dome.  The same verified HDRI supplies stable cool ambient light.
				SkyLight->SetCubemap(AuthoredNightCubemap);
			}
			SkyLight->SetRealTimeCapture(false);
			SkyLight->bLowerHemisphereIsBlack = true;
			SkyLight->SetIntensity(AuthoredNightCubemap ? 0.09f : 0.62f);
			SkyLight->SetIndirectLightingIntensity(1.0f);
			SkyLight->SetVolumetricScatteringIntensity(0.32f);
			SkyLight->SetLightColor(FLinearColor(0.36f, 0.48f, 0.82f));
			SkyLight->SetLowerHemisphereColor(FLinearColor(0.025f, 0.035f, 0.075f));
		}
	}

	for (TActorIterator<ASkyAtmosphere> It(GetWorld()); It; ++It)
	{
		if (USkyAtmosphereComponent* Atmosphere = It->GetComponent())
		{
			Atmosphere->SetSkyAndAerialPerspectiveLuminanceFactor(
				FLinearColor(0.22f, 0.28f, 0.48f));
			Atmosphere->SetRayleighScatteringScale(0.55f);
			Atmosphere->SetMieScatteringScale(1.35f);
			Atmosphere->SetHeightFogContribution(0.85f);
		}
	}

	// The template cloud layer remains bright even with a low sun and reads as
	// a blue-white daytime sky.  Dense height fog supplies the W01 cloud bank.
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
		Fog->SetFogDensity(0.009f);
		Fog->SetFogHeightFalloff(0.14f);
		Fog->SetFogMaxOpacity(0.48f);
		Fog->SetFogInscatteringColor(FLinearColor(0.018f, 0.025f, 0.055f));
		Fog->SetVolumetricFog(true);
		Fog->SetVolumetricFogExtinctionScale(0.58f);
		Fog->SetVolumetricFogAlbedo(FColor(50, 63, 92));
		Fog->SetVolumetricFogDistance(5200.0f);
	}

	FActorSpawnParameters PostProcessSpawnParameters;
	PostProcessSpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnedPostProcess = GetWorld()->SpawnActor<APostProcessVolume>(
		APostProcessVolume::StaticClass(),
		GetActorLocation(),
		FRotator::ZeroRotator,
		PostProcessSpawnParameters);
	if (SpawnedPostProcess)
	{
		SpawnedPostProcess->bUnbound = true;
		SpawnedPostProcess->bEnabled = true;
		SpawnedPostProcess->Priority = 50.0f;
		SpawnedPostProcess->BlendWeight = 1.0f;

		FPostProcessSettings& Settings = SpawnedPostProcess->Settings;
		Settings.bOverride_AutoExposureMethod = true;
		// Histogram exposure adapts the road and characters into a readable
		// night range. Manual mode with sub-lux moon values rendered almost black.
		Settings.AutoExposureMethod = AEM_Histogram;
		Settings.bOverride_AutoExposureBias = true;
		Settings.AutoExposureBias = 0.45f;
		Settings.bOverride_ColorSaturation = true;
		Settings.ColorSaturation = FVector4(0.88f, 0.91f, 0.96f, 1.0f);
		Settings.bOverride_ColorContrast = true;
		Settings.ColorContrast = FVector4(0.98f, 0.99f, 1.0f, 1.0f);
		Settings.bOverride_SceneColorTint = true;
		Settings.SceneColorTint = FLinearColor(0.94f, 0.97f, 1.0f);
		Settings.bOverride_BloomIntensity = true;
		Settings.BloomIntensity = 0.28f;
		Settings.bOverride_VignetteIntensity = true;
		Settings.VignetteIntensity = 0.18f;
	}
}

void AFantasyWorldLayout::RefreshCapturedSky()
{
	// Capture after the runtime village and atmosphere exist. Capturing in the
	// middle of ConfigureWorldAtmosphere would preserve the template map's old
	// sky contribution and leave newly spawned characters as black silhouettes.
	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
	{
		if (USkyLightComponent* SkyLight = It->GetLightComponent())
		{
			if (SkyLight->SourceType == SLS_CapturedScene)
			{
				SkyLight->RecaptureSky();
			}
		}
	}
}

void AFantasyWorldLayout::HideTemplateFloor()
{
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		AStaticMeshActor* StaticMeshActor = *It;
		UStaticMeshComponent* MeshComponent = StaticMeshActor
			? StaticMeshActor->GetStaticMeshComponent()
			: nullptr;
		const UStaticMesh* Mesh = MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
		const bool bIsTemplateSkyDome = Mesh
			&& Mesh->GetName().Contains(TEXT("SM_SkySphere"));
		const bool bIsTemplateFloor = StaticMeshActor
			&& (StaticMeshActor->GetName().StartsWith(TEXT("Floor"))
				|| (Mesh && Mesh->GetName().Contains(TEXT("Template_Map_Floor"))));
		if (bIsTemplateFloor || bIsTemplateSkyDome)
		{
			StaticMeshActor->SetActorHiddenInGame(true);
			StaticMeshActor->SetActorEnableCollision(false);
		}
	}
}

void AFantasyWorldLayout::BuildGroundAndRoad()
{
	const FLinearColor GroundTint(0.065f, 0.055f, 0.075f);
	UStaticMeshComponent* DistantGround = AddEnvironmentMesh(
		TEXT("DistantGround"), TEXT(""), FVector(1100.0f, 0.0f, -24.0f),
		FRotator::ZeroRotator, FVector(58.0f, 58.0f, 0.10f), false,
		CubeFallback, FLinearColor(0.018f, 0.016f, 0.026f));
	if (DistantGround && GroundMaterial)
	{
		DistantGround->SetMaterial(0, GroundMaterial);
	}

	for (int32 GroundIndex = 0; GroundIndex < 3; ++GroundIndex)
	{
		UStaticMeshComponent* Ground = AddEnvironmentMesh(
			FString::Printf(TEXT("Ground_%d"), GroundIndex),
			TEXT(""),
			FVector(static_cast<float>(GroundIndex) * 1100.0f, 0.0f, -12.0f),
			FRotator::ZeroRotator,
			FVector(11.5f, 21.0f, 0.28f),
			true,
			CubeFallback,
			GroundTint);
		if (Ground && GroundMaterial)
		{
			Ground->SetMaterial(0, GroundMaterial);
		}
	}

	for (int32 StoneIndex = 0; StoneIndex < 20; ++StoneIndex)
	{
		const float X = 80.0f + static_cast<float>(StoneIndex) * 112.0f;
		const float Y = (StoneIndex % 2 == 0 ? -1.0f : 1.0f) * 20.0f;
		const float Yaw = static_cast<float>((StoneIndex * 17) % 14) - 7.0f;
		UStaticMeshComponent* RoadStone = AddEnvironmentMesh(
			FString::Printf(TEXT("RoadStone_%02d"), StoneIndex),
			TEXT(""),
			FVector(X, Y, 2.0f),
			FRotator(0.0f, Yaw, 0.0f),
			FVector(1.15f, 1.55f, 0.055f),
			false,
			CubeFallback,
			FLinearColor(0.17f, 0.16f, 0.19f));
		if (RoadStone && RoadMaterial)
		{
			RoadStone->SetMaterial(0, RoadMaterial);
		}
	}
}

void AFantasyWorldLayout::BuildVillageDistrict()
{
	const FString HouseA = EnvironmentAssetPath(TEXT("SM_W01_HouseA"));
	const FString HouseB = EnvironmentAssetPath(TEXT("SM_W01_HouseB"));
	const FString Tower = EnvironmentAssetPath(TEXT("SM_W01_Tower"));
	const FString Arch = EnvironmentAssetPath(TEXT("SM_W01_Arch"));
	const FString Wall = EnvironmentAssetPath(TEXT("SM_W01_Wall"));

	AddEnvironmentMesh(TEXT("VillageGate"), *Arch, FVector(430.0f, 0.0f, 0.0f), FRotator(0.0f, 90.0f, 0.0f),
		FVector(1.25f), false, CubeFallback, FLinearColor(0.11f, 0.10f, 0.13f));
	AddEnvironmentMesh(TEXT("Inn"), *HouseA, FVector(760.0f, -510.0f, 0.0f), FRotator(0.0f, 28.0f, 0.0f),
		FVector(1.15f), true, CubeFallback, FLinearColor(0.13f, 0.085f, 0.055f));
	AddEnvironmentMesh(TEXT("Chapel"), *HouseB, FVector(1030.0f, 520.0f, 0.0f), FRotator(0.0f, -32.0f, 0.0f),
		FVector(1.10f), true, CubeFallback, FLinearColor(0.12f, 0.10f, 0.13f));
	AddEnvironmentMesh(TEXT("WatchTower"), *Tower, FVector(1480.0f, -620.0f, 0.0f), FRotator(0.0f, 18.0f, 0.0f),
		FVector(1.2f), true, CylinderFallback, FLinearColor(0.09f, 0.085f, 0.11f));
	AddEnvironmentMesh(TEXT("NorthCottage"), *HouseA, FVector(1540.0f, 610.0f, 0.0f),
		FRotator(0.0f, -18.0f, 0.0f), FVector(0.88f), true, CubeFallback,
		FLinearColor(0.12f, 0.075f, 0.05f));
	AddEnvironmentMesh(TEXT("RuinedWayhouse"), *HouseB, FVector(1890.0f, -640.0f, 0.0f),
		FRotator(0.0f, 34.0f, 0.0f), FVector(0.82f), true, CubeFallback,
		FLinearColor(0.09f, 0.075f, 0.09f));

	for (int32 WallIndex = 0; WallIndex < 5; ++WallIndex)
	{
		const float X = 780.0f + static_cast<float>(WallIndex) * 310.0f;
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			AddEnvironmentMesh(
				FString::Printf(TEXT("VillageWall_%d_%d"), WallIndex, Side),
				*Wall,
				FVector(X, static_cast<float>(Side) * 890.0f, 0.0f),
				FRotator(0.0f, Side > 0 ? 180.0f : 0.0f, 0.0f),
				FVector(1.0f),
				true,
				CubeFallback,
				FLinearColor(0.085f, 0.08f, 0.10f));
		}
	}

}

void AFantasyWorldLayout::BuildCampAndLandmarks()
{
	const FString Tree = EnvironmentAssetPath(TEXT("SM_W01_Tree"));
	const FString Crate = EnvironmentAssetPath(TEXT("SM_W01_Crate"));
	const FString Barrel = EnvironmentAssetPath(TEXT("SM_W01_Barrel"));
	const FString Cart = EnvironmentAssetPath(TEXT("SM_W01_Cart"));
	const FString Path = EnvironmentAssetPath(TEXT("SM_W01_Path"));
	const FString Fence = EnvironmentAssetPath(TEXT("SM_W01_Fence"));
	const FString Gazebo = EnvironmentAssetPath(TEXT("SM_W01_Gazebo"));
	const FString MarketStand = EnvironmentAssetPath(TEXT("SM_W01_MarketStand"));
	const FString Well = EnvironmentAssetPath(TEXT("SM_W01_Well"));
	const FString Bush = EnvironmentAssetPath(TEXT("SM_W01_Bush"));
	const FString Grass = EnvironmentAssetPath(TEXT("SM_W01_Grass"));
	const FString Rock = EnvironmentAssetPath(TEXT("SM_W01_Rock"));

	for (int32 PathIndex = 0; PathIndex < 9; ++PathIndex)
	{
		AddEnvironmentMesh(
			FString::Printf(TEXT("ImportedRoad_%02d"), PathIndex), *Path,
			FVector(410.0f + static_cast<float>(PathIndex) * 205.0f, 0.0f, 3.0f),
			FRotator::ZeroRotator, FVector(1.0f), false, CubeFallback,
			FLinearColor(0.17f, 0.16f, 0.19f));
	}

	const TArray<FVector> TreeLocations = {
		FVector(-420.0f, -980.0f, 0.0f), FVector(-260.0f, 1120.0f, 0.0f),
		FVector(260.0f, -720.0f, 0.0f), FVector(380.0f, 710.0f, 0.0f),
		FVector(850.0f, -980.0f, 0.0f), FVector(1220.0f, 860.0f, 0.0f),
		FVector(1600.0f, -880.0f, 0.0f), FVector(1820.0f, 820.0f, 0.0f),
		FVector(2080.0f, -1050.0f, 0.0f), FVector(2350.0f, 980.0f, 0.0f),
		FVector(2700.0f, -920.0f, 0.0f), FVector(2860.0f, 1040.0f, 0.0f)};
	for (int32 TreeIndex = 0; TreeIndex < TreeLocations.Num(); ++TreeIndex)
	{
		AddEnvironmentMesh(
			FString::Printf(TEXT("AshTree_%02d"), TreeIndex), *Tree, TreeLocations[TreeIndex],
			FRotator(0.0f, static_cast<float>((TreeIndex * 47) % 360), 0.0f),
			FVector(0.82f + static_cast<float>(TreeIndex % 4) * 0.17f), false,
			ConeFallback, FLinearColor(0.055f, 0.105f, 0.065f));
	}

	for (int32 PropIndex = 0; PropIndex < 4; ++PropIndex)
	{
		AddEnvironmentMesh(
			FString::Printf(TEXT("CampCrate_%d"), PropIndex), *Crate,
			FVector(820.0f + PropIndex * 48.0f, 390.0f + (PropIndex % 2) * 55.0f, 0.0f),
			FRotator(0.0f, static_cast<float>(PropIndex) * 21.0f, 0.0f), FVector(0.9f), true,
			CubeFallback, FLinearColor(0.20f, 0.10f, 0.035f));
	}
	for (int32 BarrelIndex = 0; BarrelIndex < 3; ++BarrelIndex)
	{
		AddEnvironmentMesh(
			FString::Printf(TEXT("CampBarrel_%d"), BarrelIndex), *Barrel,
			FVector(710.0f + BarrelIndex * 60.0f, -360.0f, 0.0f), FRotator::ZeroRotator,
			FVector(0.85f), true, CylinderFallback, FLinearColor(0.16f, 0.075f, 0.028f));
	}
	AddEnvironmentMesh(TEXT("RefugeeCart"), *Cart, FVector(1180.0f, -420.0f, 0.0f),
		FRotator(0.0f, 16.0f, 0.0f), FVector(1.0f), true, CubeFallback,
		FLinearColor(0.18f, 0.08f, 0.025f));
	AddEnvironmentMesh(TEXT("CampGazebo"), *Gazebo, FVector(930.0f, 560.0f, 0.0f),
		FRotator(0.0f, -18.0f, 0.0f), FVector(1.0f), true, CubeFallback,
		FLinearColor(0.16f, 0.09f, 0.05f));
	AddEnvironmentMesh(TEXT("VillageMarket"), *MarketStand, FVector(1260.0f, 390.0f, 0.0f),
		FRotator(0.0f, 195.0f, 0.0f), FVector(1.0f), true, CubeFallback,
		FLinearColor(0.18f, 0.08f, 0.04f));
	AddEnvironmentMesh(TEXT("VillageWell"), *Well, FVector(1320.0f, -360.0f, 0.0f),
		FRotator::ZeroRotator, FVector(1.0f), true, CylinderFallback,
		FLinearColor(0.10f, 0.09f, 0.11f));
	for (int32 FenceIndex = 0; FenceIndex < 4; ++FenceIndex)
	{
		AddEnvironmentMesh(
			FString::Printf(TEXT("CampFence_%d"), FenceIndex), *Fence,
			FVector(610.0f + FenceIndex * 225.0f, 730.0f, 0.0f), FRotator::ZeroRotator,
			FVector(1.0f), true, CubeFallback, FLinearColor(0.17f, 0.09f, 0.04f));
	}
	for (int32 DetailIndex = 0; DetailIndex < 6; ++DetailIndex)
	{
		const FVector DetailLocation(
			520.0f + static_cast<float>(DetailIndex) * 290.0f,
			DetailIndex % 2 == 0 ? -690.0f : 690.0f,
			0.0f);
		AddEnvironmentMesh(
			FString::Printf(TEXT("RoadsideBush_%d"), DetailIndex), *Bush, DetailLocation,
			FRotator(0.0f, static_cast<float>(DetailIndex * 37), 0.0f), FVector(0.75f), false,
			ConeFallback, FLinearColor(0.05f, 0.14f, 0.055f));
		AddEnvironmentMesh(
			FString::Printf(TEXT("RoadsideRock_%d"), DetailIndex), *Rock,
			DetailLocation + FVector(65.0f, DetailIndex % 2 == 0 ? 95.0f : -95.0f, 0.0f),
			FRotator(0.0f, static_cast<float>(DetailIndex * 61), 0.0f), FVector(0.55f), false,
			CubeFallback, FLinearColor(0.10f, 0.095f, 0.12f));
		AddEnvironmentMesh(
			FString::Printf(TEXT("AshGrass_%d"), DetailIndex), *Grass,
			DetailLocation + FVector(-80.0f, DetailIndex % 2 == 0 ? -70.0f : 70.0f, 0.0f),
			FRotator(0.0f, static_cast<float>(DetailIndex * 83), 0.0f), FVector(0.9f), false,
			ConeFallback, FLinearColor(0.045f, 0.11f, 0.045f));
	}

	// Broad, irregular rock silhouettes conceal the edge of the playable slab
	// and give the otherwise flat template map a distant valley profile.
	for (int32 RidgeIndex = 0; RidgeIndex < 10; ++RidgeIndex)
	{
		const float X = -350.0f + static_cast<float>(RidgeIndex) * 370.0f;
		const float Side = RidgeIndex % 2 == 0 ? -1.0f : 1.0f;
		AddEnvironmentMesh(
			FString::Printf(TEXT("DistantRidge_%02d"), RidgeIndex), *Rock,
			FVector(X, Side * (1250.0f + static_cast<float>((RidgeIndex * 73) % 260)), -8.0f),
			FRotator(0.0f, static_cast<float>((RidgeIndex * 41) % 360), 0.0f),
			FVector(1.5f + static_cast<float>(RidgeIndex % 3) * 0.45f), false,
			CubeFallback, FLinearColor(0.035f, 0.032f, 0.050f));
	}

	AddTorch(FVector(330.0f, -260.0f, 0.0f), 0);
	AddTorch(FVector(330.0f, 260.0f, 0.0f), 1);
	AddTorch(FVector(860.0f, -210.0f, 0.0f), 2);
	AddTorch(FVector(860.0f, 210.0f, 0.0f), 3);
	AddTorch(FVector(1420.0f, -220.0f, 0.0f), 4);
	AddTorch(FVector(1420.0f, 220.0f, 0.0f), 5);
	AddTorch(FVector(1940.0f, -250.0f, 0.0f), 6);
	AddTorch(FVector(1940.0f, 250.0f, 0.0f), 7);
}

void AFantasyWorldLayout::BuildBattleApproach()
{
	const FString Arch = EnvironmentAssetPath(TEXT("SM_W01_Arch"));
	const FString Wall = EnvironmentAssetPath(TEXT("SM_W01_Wall"));
	AddEnvironmentMesh(TEXT("BlackthornGate"), *Arch, FVector(2050.0f, 0.0f, 0.0f),
		FRotator(0.0f, 90.0f, 0.0f), FVector(1.55f), false, CubeFallback,
		FLinearColor(0.075f, 0.055f, 0.085f));
	AddEnvironmentMesh(TEXT("BlackthornWallL"), *Wall, FVector(2100.0f, -610.0f, 0.0f),
		FRotator::ZeroRotator, FVector(1.2f), true, CubeFallback,
		FLinearColor(0.075f, 0.065f, 0.08f));
	AddEnvironmentMesh(TEXT("BlackthornWallR"), *Wall, FVector(2100.0f, 610.0f, 0.0f),
		FRotator(0.0f, 180.0f, 0.0f), FVector(1.2f), true, CubeFallback,
		FLinearColor(0.075f, 0.065f, 0.08f));
}

void AFantasyWorldLayout::BuildReturnPortalLandmark()
{
	const FString Arch = EnvironmentAssetPath(TEXT("SM_W01_Arch"));
	const FString Rock = EnvironmentAssetPath(TEXT("SM_W01_Rock"));
	const FString Campfire = EnvironmentAssetPath(TEXT("SM_W01_Campfire"));
	const FVector Facing = (-ReturnPortalLocalLocation).GetSafeNormal2D();
	const FVector Side(-Facing.Y, Facing.X, 0.0f);
	// The imported arch spans its local +X axis. Rotate that span across the
	// approach vector, leaving the portal opening facing the player.
	const FRotator GateRotation(0.0f, Facing.Rotation().Yaw + 90.0f, 0.0f);

	AddEnvironmentMesh(TEXT("ReturnGateArch"), *Arch, ReturnPortalLocalLocation,
		GateRotation, FVector(1.12f), false, CubeFallback,
		FLinearColor(0.045f, 0.050f, 0.075f));
	AddEnvironmentMesh(TEXT("ReturnGateCore"), TEXT(""),
		ReturnPortalLocalLocation + FVector(0.0f, 0.0f, 145.0f),
		FRotator(90.0f, Facing.Rotation().Yaw, 0.0f), FVector(1.45f, 1.05f, 0.055f),
		false, CylinderFallback, FLinearColor(0.015f, 0.060f, 0.12f));

	for (int32 SideIndex = -1; SideIndex <= 1; SideIndex += 2)
	{
		const FVector FlankLocation = ReturnPortalLocalLocation
			+ Side * (static_cast<float>(SideIndex) * 230.0f);
		AddEnvironmentMesh(
			FString::Printf(TEXT("ReturnGateRock_%d"), SideIndex), *Rock,
			FlankLocation + Facing * 25.0f, FRotator(0.0f, SideIndex * 37.0f, 0.0f),
			FVector(0.9f), false, CubeFallback, FLinearColor(0.045f, 0.045f, 0.065f));
		AddEnvironmentMesh(
			FString::Printf(TEXT("ReturnGateBrazier_%d"), SideIndex), *Campfire,
			FlankLocation - Facing * 55.0f, GateRotation, FVector(0.42f), false,
			ConeFallback, FLinearColor(0.45f, 0.10f, 0.02f));
		AddAccentLight(
			FString::Printf(TEXT("ReturnGateLight_%d"), SideIndex),
			FlankLocation - Facing * 55.0f + FVector(0.0f, 0.0f, 80.0f),
			FLinearColor(0.82f, 0.22f, 0.055f), 720.0f, 300.0f, false);
	}
	AddAccentLight(TEXT("ReturnGateCoreLight"),
		ReturnPortalLocalLocation + Facing * 40.0f + FVector(0.0f, 0.0f, 145.0f),
		FLinearColor(0.06f, 0.30f, 0.72f), 520.0f, 360.0f, false);
}

void AFantasyWorldLayout::SpawnResidents()
{
	struct FResidentPlacement
	{
		EFantasyNPCArchetype Archetype;
		FVector Location;
		float Yaw;
	};
	const FResidentPlacement Placements[] = {
		{EFantasyNPCArchetype::GateVeteran, FVector(520.0f, -225.0f, 0.0f), 145.0f},
		{EFantasyNPCArchetype::ExiledSister, FVector(980.0f, 330.0f, 0.0f), -145.0f},
		{EFantasyNPCArchetype::ArcaneScholar, FVector(1510.0f, -280.0f, 0.0f), 140.0f},
		{EFantasyNPCArchetype::AshenRanger, FVector(1780.0f, 370.0f, 0.0f), -145.0f}};

	for (const FResidentPlacement& Placement : Placements)
	{
		const FTransform ResidentTransform(
			GetActorRotation() + FRotator(0.0f, Placement.Yaw, 0.0f),
			GetActorTransform().TransformPosition(Placement.Location + FVector(0.0f, 0.0f, 88.0f)));
		AFantasyNPC* Resident = GetWorld()->SpawnActorDeferred<AFantasyNPC>(
			AFantasyNPC::StaticClass(),
			ResidentTransform,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (Resident)
		{
			Resident->ConfigureNPC(Placement.Archetype);
			Resident->FinishSpawning(ResidentTransform);
			Residents.Add(Resident);
		}
	}
}

void AFantasyWorldLayout::AddTorch(const FVector& LocalLocation, const int32 TorchIndex)
{
	const FString Campfire = EnvironmentAssetPath(TEXT("SM_W01_Campfire"));
	const bool bCampfire = TorchIndex == 2 || TorchIndex == 5;
	FVector DisplayLocation = LocalLocation;
	if (!bCampfire)
	{
		// Keep the smaller guide flames outside the playable road instead of
		// placing metre-wide primitive cylinders next to the player camera.
		DisplayLocation.Y += FMath::Sign(DisplayLocation.Y) * 120.0f;
	}
	AddEnvironmentMesh(
		FString::Printf(TEXT("TorchMesh_%d"), TorchIndex),
		*Campfire,
		DisplayLocation,
		FRotator::ZeroRotator,
		bCampfire ? FVector(0.64f) : FVector(0.34f),
		false,
		ConeFallback,
		FLinearColor(0.65f, 0.12f, 0.015f));

	UPointLightComponent* Light = NewObject<UPointLightComponent>(this,
		FName(*FString::Printf(TEXT("AshenTorchLight_%d"), TorchIndex)));
	AddInstanceComponent(Light);
	Light->SetupAttachment(SceneRoot);
	Light->SetRelativeLocation(DisplayLocation + FVector(0.0f, 0.0f, bCampfire ? 90.0f : 70.0f));
	Light->SetIntensity(bCampfire ? 1650.0f : 950.0f);
	Light->SetAttenuationRadius(bCampfire ? 460.0f : 340.0f);
	Light->SetLightColor(FLinearColor(1.0f, 0.22f, 0.045f));
	Light->SetCastShadows(bCampfire);
	Light->RegisterComponent();
	TorchLights.Add(Light);
}

void AFantasyWorldLayout::AddAccentLight(
	const FString& ComponentName,
	const FVector& LocalLocation,
	const FLinearColor& Color,
	const float Intensity,
	const float Radius,
	const bool bCastShadows)
{
	UPointLightComponent* Light = NewObject<UPointLightComponent>(this, FName(*ComponentName));
	AddInstanceComponent(Light);
	Light->SetupAttachment(SceneRoot);
	Light->SetRelativeLocation(LocalLocation);
	Light->SetIntensity(Intensity);
	Light->SetAttenuationRadius(Radius);
	Light->SetLightColor(Color);
	Light->SetCastShadows(bCastShadows);
	Light->RegisterComponent();
	TorchLights.Add(Light);
}

void AFantasyWorldLayout::FinalizeReturnPortalPresentation()
{
	const FVector ReturnPortalWorldLocation = GetReturnPortalLocation();
	for (TActorIterator<AWorldPortal> It(GetWorld()); It; ++It)
	{
		AWorldPortal* Portal = *It;
		if (!Portal || FVector::DistSquared(Portal->GetActorLocation(), ReturnPortalWorldLocation)
			> FMath::Square(320.0f))
		{
			continue;
		}

		// The generic actor remains authoritative for E interaction.  Only its
		// prototype meshes are hidden behind the W01-specific stone landmark.
		TArray<UStaticMeshComponent*> PortalMeshes;
		Portal->GetComponents<UStaticMeshComponent>(PortalMeshes);
		for (UStaticMeshComponent* PortalMesh : PortalMeshes)
		{
			PortalMesh->SetVisibility(false, true);
			PortalMesh->SetHiddenInGame(true, true);
		}

		TArray<UTextRenderComponent*> PortalLabels;
		Portal->GetComponents<UTextRenderComponent>(PortalLabels);
		for (UTextRenderComponent* PortalLabel : PortalLabels)
		{
			PortalLabel->SetText(FText::FromString(TEXT("返回主世界\n按 E 进入")));
			PortalLabel->SetWorldSize(22.0f);
			PortalLabel->SetTextRenderColor(FColor(94, 178, 225));
		}
		break;
	}
}

UStaticMeshComponent* AFantasyWorldLayout::AddEnvironmentMesh(
	const FString& ComponentName,
	const TCHAR* ImportedMeshPath,
	const FVector& LocalLocation,
	const FRotator& LocalRotation,
	const FVector& LocalScale,
	const bool bEnableCollision,
	UStaticMesh* FallbackMesh,
	const FLinearColor& FallbackTint)
{
	UStaticMesh* ImportedMesh = ImportedMeshPath && FCString::Strlen(ImportedMeshPath) > 0
		? LoadObject<UStaticMesh>(nullptr, ImportedMeshPath)
		: nullptr;
	UStaticMesh* Mesh = ImportedMesh ? ImportedMesh : FallbackMesh;
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
	Component->SetRelativeScale3D(LocalScale);
	Component->SetCollisionEnabled(bEnableCollision
		? ECollisionEnabled::QueryAndPhysics
		: ECollisionEnabled::NoCollision);
	Component->SetCollisionResponseToAllChannels(bEnableCollision ? ECR_Block : ECR_Ignore);
	Component->SetCastShadow(true);
	Component->RegisterComponent();
	EnvironmentMeshes.Add(Component);

	if (!ImportedMesh)
	{
		// Engine basic shapes use WorldGridMaterial, whose public parameters do
		// not include the legacy "Color" tint. Prefer the imported W01 stone so
		// intentional primitives (ground, torch posts and portal core) never
		// expose the gray checkerboard seen in the prototype screenshots.
		if (GroundMaterial)
		{
			Component->SetMaterial(0, GroundMaterial);
		}
		else
		{
			ApplyTint(Component, FallbackTint);
		}
	}
	return Component;
}

void AFantasyWorldLayout::ApplyTint(
	UStaticMeshComponent* Component,
	const FLinearColor& Color) const
{
	if (!Component || !Component->GetMaterial(0))
	{
		return;
	}

	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Component->GetMaterial(0), Component);
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Component->SetMaterial(0, Material);
}
