#include "World/WorldHubLayout.h"

#include "Animation/AnimInstance.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/GameInstance.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkeletalMesh.h"
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
#include "UObject/UObjectGlobals.h"
#include "World/WorldDefinition.h"
#include "World/WorldPortal.h"
#include "World/WorldTravelSubsystem.h"

const FVector AWorldHubLayout::ActivePortalLocalLocation(-330.0f, 0.0f, 0.0f);

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
		TEXT("W00 night presentation ready. Avatar=%s Portal=%s"),
		MainWorldCharacter ? TEXT("anime-animated") : TEXT("missing"),
		*GetActivePortalLocation().ToCompactString());
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
	if (PortalLight && !bTravelRequested)
	{
		PortalLight->SetIntensity(1500.0f + FMath::Sin(PortalPulseTime * 2.4f) * 250.0f);
	}
	for (int32 LightIndex = 0; LightIndex < ShrineLights.Num(); ++LightIndex)
	{
		if (UPointLightComponent* ShrineLight = ShrineLights[LightIndex])
		{
			const float Flicker = FMath::Sin(PortalPulseTime * (3.7f + LightIndex * 0.35f)) * 65.0f;
			ShrineLight->SetIntensity(700.0f + Flicker);
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
	MainWorldCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!MainWorldCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("W00 anime avatar was not configured: player character is missing."));
		return;
	}

	TArray<UStaticMeshComponent*> PlayerStaticMeshes;
	MainWorldCharacter->GetComponents(PlayerStaticMeshes);
	for (UStaticMeshComponent* StaticMeshComponent : PlayerStaticMeshes)
	{
		if (StaticMeshComponent && StaticMeshComponent->GetFName() == TEXT("PrototypeBody"))
		{
			StaticMeshComponent->SetVisibility(false, true);
			StaticMeshComponent->SetHiddenInGame(true, true);
		}
	}

	static const TCHAR* HeadMeshPath =
		TEXT("/Game/AnimeCharacters/Blueprints/Characters/Female_Average/Head/Starter/skl_AnimeF_Head_1.skl_AnimeF_Head_1");
	static const TCHAR* HumanAnimBlueprintPath =
		TEXT("/Game/AnimeCharacters/Animations/abp_Human.abp_Human_C");

	USkeletalMeshComponent* HeadComponent = MainWorldCharacter->GetMesh();
	USkeletalMesh* HeadMesh = LoadObject<USkeletalMesh>(nullptr, HeadMeshPath);
	UClass* HumanAnimClass = LoadClass<UAnimInstance>(nullptr, HumanAnimBlueprintPath);
	if (!HeadComponent || !HeadMesh || !HumanAnimClass)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("W00 anime avatar failed to load head or locomotion AnimBP. Head=%s AnimBP=%s"),
			HeadMesh ? TEXT("ok") : TEXT("missing"),
			HumanAnimClass ? TEXT("ok") : TEXT("missing"));
		return;
	}

	HeadComponent->SetSkeletalMeshAsset(HeadMesh);
	HeadComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
	HeadComponent->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	HeadComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadComponent->SetVisibility(true, true);
	HeadComponent->SetHiddenInGame(false, true);
	HeadComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	HeadComponent->SetAnimInstanceClass(HumanAnimClass);
	HeadComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	MainWorldAvatarParts.Add(HeadComponent);

	CreateLinkedAvatarPart(
		MainWorldCharacter,
		TEXT("MainWorldAvatarTop"),
		TEXT("/Game/AnimeCharacters/Blueprints/Characters/Female_Average/tops/Starter_TurtleNeck/skl_AnimeF_top_TurtleNeck.skl_AnimeF_top_TurtleNeck"),
		HeadComponent);
	CreateLinkedAvatarPart(
		MainWorldCharacter,
		TEXT("MainWorldAvatarBottom"),
		TEXT("/Game/AnimeCharacters/Blueprints/Characters/Female_Average/bottoms/Starter_Skirt/skl_AnimeF_bottom_skirt.skl_AnimeF_bottom_skirt"),
		HeadComponent);
	CreateHairPart(MainWorldCharacter, HeadComponent);

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

USkeletalMeshComponent* AWorldHubLayout::CreateLinkedAvatarPart(
	ACharacter* PlayerCharacter,
	const FName ComponentName,
	const TCHAR* MeshPath,
	USkeletalMeshComponent* PoseLeader)
{
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, MeshPath);
	if (!PlayerCharacter || !PoseLeader || !Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("W00 avatar part failed to load: %s"), MeshPath);
		return nullptr;
	}

	USkeletalMeshComponent* Part = NewObject<USkeletalMeshComponent>(PlayerCharacter, ComponentName);
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
	USkeletalMeshComponent* HeadComponent)
{
	static const TCHAR* HairMeshPath =
		TEXT("/Game/AnimeCharacters/Blueprints/Characters/Female_Average/Hair/Starter2/skl_AnimeHair_F2.skl_AnimeHair_F2");
	USkeletalMesh* HairMesh = LoadObject<USkeletalMesh>(nullptr, HairMeshPath);
	if (!PlayerCharacter || !HeadComponent || !HairMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("W00 avatar hair failed to load: %s"), HairMeshPath);
		return nullptr;
	}

	USkeletalMeshComponent* Hair = NewObject<USkeletalMeshComponent>(PlayerCharacter, TEXT("MainWorldAvatarHair"));
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
		NightSkyLight->RecaptureSky();
	}
}

void AWorldHubLayout::HideLegacyPortal()
{
	const FVector PortalLocation = GetActivePortalLocation();
	for (TActorIterator<AWorldPortal> It(GetWorld()); It; ++It)
	{
		AWorldPortal* Portal = *It;
		if (Portal && FVector::DistSquared(Portal->GetActorLocation(), PortalLocation) < FMath::Square(350.0f))
		{
			Portal->SetActorHiddenInGame(true);
			Portal->SetActorEnableCollision(false);
			return;
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
