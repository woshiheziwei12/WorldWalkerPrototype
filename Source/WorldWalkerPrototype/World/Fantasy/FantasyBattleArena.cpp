#include "World/Fantasy/FantasyBattleArena.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const TCHAR* ArenaEnvironmentRoot =
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Environment");

	FString ArenaEnvironmentAssetPath(const TCHAR* AssetName)
	{
		return FString::Printf(
			TEXT("%s/%s/%s.%s"),
			ArenaEnvironmentRoot,
			AssetName,
			AssetName,
			AssetName);
	}

	const FLinearColor CharcoalStone(0.055f, 0.050f, 0.070f, 1.0f);
	const FLinearColor WeatheredStone(0.085f, 0.080f, 0.105f, 1.0f);
	const FLinearColor DeadWood(0.065f, 0.085f, 0.060f, 1.0f);
	const FLinearColor AshRock(0.080f, 0.075f, 0.090f, 1.0f);
}

AFantasyBattleArena::AFantasyBattleArena()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void AFantasyBattleArena::BeginPlay()
{
	Super::BeginPlay();

	CubeFallback = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	CylinderFallback = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	ConeFallback = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));

	BuildCourtyard();

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Blackthorn courtyard ready. Imported=%d Fallback=%d Pieces=%d Lights=%d OpenLaneWidth=700"),
		ImportedSceneryCount,
		FallbackSceneryCount,
		SceneryPieces.Num(),
		BrazierLights.Num());
}

void AFantasyBattleArena::BuildCourtyard()
{
	// Local +X points from the arriving player through the enemy. The entire
	// x=-500..+500, y=-350..+350 corridor deliberately contains no scenery.
	BuildKeepBackdrop();
	BuildSideRuins();
	BuildNaturalFrame();

	AddBrazier(FVector(70.0f, -455.0f, 0.0f), 0);
	AddBrazier(FVector(70.0f, 455.0f, 0.0f), 1);

	// A few flat road slabs visually connect the exploration road to the duel
	// without creating a raised dais or affecting character movement.
	for (int32 SlabIndex = 0; SlabIndex < 4; ++SlabIndex)
	{
		AddScenery(
			FString::Printf(TEXT("CourtPath_%02d"), SlabIndex),
			TEXT("SM_W01_Path"),
			FVector(-390.0f + static_cast<float>(SlabIndex) * 215.0f, 0.0f, 2.0f),
			FRotator::ZeroRotator,
			FVector(1.02f),
			CubeFallback,
			FVector(1.65f, 1.35f, 0.025f),
			WeatheredStone);
	}
}

void AFantasyBattleArena::BuildKeepBackdrop()
{
	// The gate sits five metres behind the enemy rather than between the
	// combatants. It is decorative and non-colliding, so retreat remains safe.
	if (!AddScenery(
		TEXT("BlackthornKeepArch"),
		TEXT("SM_W01_Arch"),
		FVector(650.0f, 0.0f, 0.0f),
		FRotator(0.0f, 90.0f, 0.0f),
		FVector(1.10f),
		nullptr,
		FVector::OneVector,
		CharcoalStone))
	{
		// An open, three-piece silhouette is safer than the old solid gate bars.
		AddPrimitive(
			TEXT("FallbackArchLeft"), CubeFallback,
			FVector(650.0f, -330.0f, 150.0f), FRotator::ZeroRotator,
			FVector(0.55f, 0.55f, 1.50f), CharcoalStone);
		AddPrimitive(
			TEXT("FallbackArchRight"), CubeFallback,
			FVector(650.0f, 330.0f, 150.0f), FRotator::ZeroRotator,
			FVector(0.55f, 0.55f, 1.50f), CharcoalStone);
		AddPrimitive(
			TEXT("FallbackArchLintel"), CubeFallback,
			FVector(650.0f, 0.0f, 325.0f), FRotator::ZeroRotator,
			FVector(0.55f, 3.85f, 0.25f), CharcoalStone);
	}

	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		AddScenery(
			FString::Printf(TEXT("RearTower_%d"), Side),
			TEXT("SM_W01_Tower"),
			FVector(710.0f, static_cast<float>(Side) * 650.0f, 0.0f),
			FRotator(0.0f, Side > 0 ? -8.0f : 8.0f, 0.0f),
			FVector(0.88f),
			CylinderFallback,
			FVector(1.05f, 1.05f, 2.45f),
			CharcoalStone);
	}
}

void AFantasyBattleArena::BuildSideRuins()
{
	// Broken flanks suggest a courtyard boundary while staying well outside
	// both the movement lane and the third-person spring-arm camera sweep.
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		const float SideY = static_cast<float>(Side) * 720.0f;
		for (int32 SegmentIndex = 0; SegmentIndex < 2; ++SegmentIndex)
		{
			AddScenery(
				FString::Printf(TEXT("SideWall_%d_%d"), Side, SegmentIndex),
				TEXT("SM_W01_Wall"),
				FVector(-170.0f + static_cast<float>(SegmentIndex) * 470.0f, SideY, 0.0f),
				FRotator(0.0f, Side > 0 ? 180.0f : 0.0f, 0.0f),
				FVector(0.92f),
				CubeFallback,
				FVector(3.00f, 0.18f, 0.72f),
				WeatheredStone);
		}

		AddScenery(
			FString::Printf(TEXT("OuterRock_%d"), Side),
			TEXT("SM_W01_Rock"),
			FVector(350.0f, static_cast<float>(Side) * 905.0f, 0.0f),
			FRotator(0.0f, Side > 0 ? 48.0f : -37.0f, 0.0f),
			FVector(0.72f),
			CubeFallback,
			FVector(0.65f, 0.90f, 0.42f),
			AshRock);
	}
}

void AFantasyBattleArena::BuildNaturalFrame()
{
	const FVector TreeLocations[] = {
		FVector(-340.0f, -870.0f, 0.0f),
		FVector(-340.0f, 870.0f, 0.0f),
		FVector(760.0f, -930.0f, 0.0f),
		FVector(760.0f, 930.0f, 0.0f)};

	for (int32 TreeIndex = 0; TreeIndex < UE_ARRAY_COUNT(TreeLocations); ++TreeIndex)
	{
		AddScenery(
			FString::Printf(TEXT("CourtTree_%02d"), TreeIndex),
			TEXT("SM_W01_Tree"),
			TreeLocations[TreeIndex],
			FRotator(0.0f, static_cast<float>((TreeIndex * 83 + 17) % 360), 0.0f),
			FVector(TreeIndex < 2 ? 0.78f : 0.90f),
			ConeFallback,
			FVector(0.72f, 0.72f, 2.85f),
			DeadWood);
	}
}

void AFantasyBattleArena::AddBrazier(const FVector& LocalLocation, const int32 Index)
{
	AddScenery(
		FString::Printf(TEXT("CourtBrazier_%d"), Index),
		TEXT("SM_W01_Campfire"),
		LocalLocation,
		FRotator(0.0f, static_cast<float>(Index) * 180.0f, 0.0f),
		FVector(0.72f),
		ConeFallback,
		FVector(0.24f, 0.24f, 0.42f),
		FLinearColor(0.75f, 0.10f, 0.018f, 1.0f));

	UPointLightComponent* Light = NewObject<UPointLightComponent>(
		this,
		FName(*FString::Printf(TEXT("CourtBrazierLight_%d"), Index)));
	AddInstanceComponent(Light);
	Light->SetupAttachment(SceneRoot);
	Light->SetRelativeLocation(LocalLocation + FVector(0.0f, 0.0f, 105.0f));
	Light->SetIntensity(1500.0f);
	Light->SetAttenuationRadius(470.0f);
	Light->SetSourceRadius(12.0f);
	Light->SetLightColor(FLinearColor(1.0f, 0.25f, 0.055f));
	Light->SetCastShadows(true);
	Light->RegisterComponent();
	BrazierLights.Add(Light);
}

UStaticMesh* AFantasyBattleArena::LoadEnvironmentMesh(const TCHAR* AssetName) const
{
	if (!AssetName || FCString::Strlen(AssetName) == 0)
	{
		return nullptr;
	}

	const FString ObjectPath = ArenaEnvironmentAssetPath(AssetName);
	return LoadObject<UStaticMesh>(nullptr, *ObjectPath);
}

UStaticMeshComponent* AFantasyBattleArena::AddScenery(
	const FString& ComponentName,
	const TCHAR* ImportedAssetName,
	const FVector& LocalLocation,
	const FRotator& LocalRotation,
	const FVector& ImportedScale,
	UStaticMesh* FallbackMesh,
	const FVector& FallbackScale,
	const FLinearColor& FallbackTint)
{
	UStaticMesh* ImportedMesh = LoadEnvironmentMesh(ImportedAssetName);
	if (!ImportedMesh && !FallbackMesh)
	{
		return nullptr;
	}

	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, FName(*ComponentName));
	AddInstanceComponent(Component);
	Component->SetupAttachment(SceneRoot);
	Component->SetStaticMesh(ImportedMesh ? ImportedMesh : FallbackMesh);
	Component->SetRelativeLocation(LocalLocation);
	Component->SetRelativeRotation(LocalRotation);
	Component->SetRelativeScale3D(ImportedMesh ? ImportedScale : FallbackScale);
	// The arena is assembled after its runtime-spawned movable root registers.
	// Matching mobility prevents UE from rejecting the attachment.
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetCollisionResponseToAllChannels(ECR_Ignore);
	Component->SetCastShadow(true);
	Component->RegisterComponent();
	SceneryPieces.Add(Component);

	if (ImportedMesh)
	{
		++ImportedSceneryCount;
	}
	else
	{
		++FallbackSceneryCount;
		ApplyTint(Component, FallbackTint);
	}
	return Component;
}

UStaticMeshComponent* AFantasyBattleArena::AddPrimitive(
	const FString& ComponentName,
	UStaticMesh* Mesh,
	const FVector& LocalLocation,
	const FRotator& LocalRotation,
	const FVector& LocalScale,
	const FLinearColor& Tint)
{
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
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetCollisionResponseToAllChannels(ECR_Ignore);
	Component->SetCastShadow(true);
	Component->RegisterComponent();
	SceneryPieces.Add(Component);
	++FallbackSceneryCount;
	ApplyTint(Component, Tint);
	return Component;
}

void AFantasyBattleArena::ApplyTint(
	UStaticMeshComponent* Component,
	const FLinearColor& Color) const
{
	if (!Component || !Component->GetMaterial(0))
	{
		return;
	}

	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(
		Component->GetMaterial(0),
		Component);
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Component->SetMaterial(0, Material);
}
