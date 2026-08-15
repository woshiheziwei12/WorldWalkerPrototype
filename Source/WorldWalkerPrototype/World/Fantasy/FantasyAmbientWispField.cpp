#include "World/Fantasy/FantasyAmbientWispField.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

AFantasyAmbientWispField::AFantasyAmbientWispField()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f / 30.0f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void AFantasyAmbientWispField::BeginPlay()
{
	Super::BeginPlay();

	// Keep every wisp off the walkable centre line. Their asymmetric placement
	// gives the long W01 route motion and depth without obscuring interaction
	// prompts or the battle camera.
	const FVector WispLocations[] = {
		FVector(420.0f, -610.0f, 155.0f),
		FVector(790.0f, 570.0f, 125.0f),
		FVector(1190.0f, -620.0f, 185.0f),
		FVector(1510.0f, 610.0f, 145.0f),
		FVector(1880.0f, -590.0f, 200.0f),
		FVector(2180.0f, 650.0f, 175.0f),
		FVector(-170.0f, 1010.0f, 160.0f)
	};
	const FLinearColor Colors[] = {
		FLinearColor(0.07f, 0.48f, 1.0f),
		FLinearColor(0.34f, 0.15f, 1.0f),
		FLinearColor(0.05f, 0.68f, 0.82f),
		FLinearColor(0.42f, 0.20f, 1.0f),
		FLinearColor(0.08f, 0.42f, 0.95f),
		FLinearColor(0.20f, 0.58f, 1.0f),
		FLinearColor(0.08f, 0.34f, 1.0f)
	};

	for (int32 WispIndex = 0; WispIndex < UE_ARRAY_COUNT(WispLocations); ++WispIndex)
	{
		BuildWisp(WispIndex, WispLocations[WispIndex], Colors[WispIndex]);
	}

	SetActorTickEnabled(!WispMeshes.IsEmpty());
	UE_LOG(LogTemp, Display, TEXT("Ashen Kingdom ambient wisps ready. Wisps=%d"), WispMeshes.Num());
}

void AFantasyAmbientWispField::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AmbientTime += DeltaSeconds;

	for (int32 WispIndex = 0; WispIndex < WispMeshes.Num(); ++WispIndex)
	{
		UStaticMeshComponent* WispMesh = WispMeshes[WispIndex];
		if (!WispMesh || !BaseLocations.IsValidIndex(WispIndex))
		{
			continue;
		}

		const float Phase = AmbientTime * (0.72f + static_cast<float>(WispIndex % 3) * 0.11f)
			+ static_cast<float>(WispIndex) * 1.37f;
		const FVector Drift(
			FMath::Sin(Phase * 0.61f) * 24.0f,
			FMath::Cos(Phase * 0.47f) * 18.0f,
			FMath::Sin(Phase) * 22.0f);
		WispMesh->SetRelativeLocation(BaseLocations[WispIndex] + Drift);

		if (WispLights.IsValidIndex(WispIndex) && WispLights[WispIndex])
		{
			WispLights[WispIndex]->SetRelativeLocation(WispMesh->GetRelativeLocation());
			WispLights[WispIndex]->SetIntensity(
				90.0f + FMath::Sin(Phase * 2.3f) * 22.0f);
		}
	}
}

void AFantasyAmbientWispField::BuildWisp(
	const int32 WispIndex,
	const FVector& LocalLocation,
	const FLinearColor& Color)
{
	UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* EmissiveMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	if (!SphereMesh)
	{
		return;
	}

	UStaticMeshComponent* WispMesh = NewObject<UStaticMeshComponent>(
		this,
		FName(*FString::Printf(TEXT("ArcaneWisp_%02d"), WispIndex)));
	AddInstanceComponent(WispMesh);
	WispMesh->SetupAttachment(SceneRoot);
	WispMesh->SetMobility(EComponentMobility::Movable);
	WispMesh->SetStaticMesh(SphereMesh);
	WispMesh->SetRelativeLocation(LocalLocation);
	WispMesh->SetRelativeScale3D(FVector(0.055f, 0.055f, 0.085f));
	WispMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WispMesh->SetCastShadow(false);
	WispMesh->RegisterComponent();

	if (EmissiveMaterial)
	{
		UMaterialInstanceDynamic* WispMaterial = UMaterialInstanceDynamic::Create(
			EmissiveMaterial,
			WispMesh);
		WispMaterial->SetVectorParameterValue(TEXT("Color"), Color * 8.0f);
		WispMesh->SetMaterial(0, WispMaterial);
	}

	UPointLightComponent* WispLight = NewObject<UPointLightComponent>(
		this,
		FName(*FString::Printf(TEXT("ArcaneWispLight_%02d"), WispIndex)));
	AddInstanceComponent(WispLight);
	WispLight->SetupAttachment(SceneRoot);
	WispLight->SetMobility(EComponentMobility::Movable);
	WispLight->SetRelativeLocation(LocalLocation);
	WispLight->SetLightColor(Color);
	WispLight->SetIntensity(90.0f);
	WispLight->SetAttenuationRadius(185.0f);
	WispLight->SetCastShadows(false);
	WispLight->RegisterComponent();

	BaseLocations.Add(LocalLocation);
	WispColors.Add(Color);
	WispMeshes.Add(WispMesh);
	WispLights.Add(WispLight);
}
