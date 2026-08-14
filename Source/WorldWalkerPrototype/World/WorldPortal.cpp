#include "World/WorldPortal.h"

#include "Characters/WorldWalkerCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "World/WorldDefinition.h"
#include "World/WorldTravelSubsystem.h"

AWorldPortal::AWorldPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	LeftPillar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftPillar"));
	LeftPillar->SetupAttachment(SceneRoot);
	LeftPillar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftPillar->SetRelativeLocation(FVector(0.0f, -120.0f, 130.0f));
	LeftPillar->SetRelativeScale3D(FVector(0.28f, 0.28f, 2.6f));

	RightPillar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightPillar"));
	RightPillar->SetupAttachment(SceneRoot);
	RightPillar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightPillar->SetRelativeLocation(FVector(0.0f, 120.0f, 130.0f));
	RightPillar->SetRelativeScale3D(FVector(0.28f, 0.28f, 2.6f));

	TopBeam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopBeam"));
	TopBeam->SetupAttachment(SceneRoot);
	TopBeam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TopBeam->SetRelativeLocation(FVector(0.0f, 0.0f, 260.0f));
	TopBeam->SetRelativeScale3D(FVector(0.28f, 2.7f, 0.28f));

	PortalSurface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalSurface"));
	PortalSurface->SetupAttachment(SceneRoot);
	PortalSurface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PortalSurface->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
	PortalSurface->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	PortalSurface->SetRelativeScale3D(FVector(1.05f, 1.05f, 0.08f));

	if (CubeMesh.Succeeded())
	{
		LeftPillar->SetStaticMesh(CubeMesh.Object);
		RightPillar->SetStaticMesh(CubeMesh.Object);
		TopBeam->SetStaticMesh(CubeMesh.Object);
	}
	if (CylinderMesh.Succeeded())
	{
		PortalSurface->SetStaticMesh(CylinderMesh.Object);
	}

	PortalLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PortalLabel"));
	PortalLabel->SetupAttachment(SceneRoot);
	PortalLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 325.0f));
	PortalLabel->SetHorizontalAlignment(EHTA_Center);
	PortalLabel->SetVerticalAlignment(EVRTA_TextCenter);
	PortalLabel->SetWorldSize(30.0f);
	PortalLabel->SetTextRenderColor(FColor::Cyan);
	PortalLabel->SetText(FText::FromString(TEXT("UNCONFIGURED PORTAL")));
}

void AWorldPortal::BeginPlay()
{
	Super::BeginPlay();

	if (UMaterialInterface* BaseMaterial = PortalSurface->GetMaterial(0))
	{
		PortalMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		PortalSurface->SetMaterial(0, PortalMaterial);
	}
	RefreshPresentation();
}

void AWorldPortal::ConfigurePortal(UWorldDefinition* InDestinationWorld)
{
	DestinationWorld = InDestinationWorld;
	RefreshPresentation();
}

bool AWorldPortal::CanInteract_Implementation(const AWorldWalkerCharacter* InteractingCharacter) const
{
	return InteractingCharacter != nullptr && DestinationWorld != nullptr && !DestinationWorld->EntryMap.IsNull();
}

void AWorldPortal::Interact_Implementation(AWorldWalkerCharacter* InteractingCharacter)
{
	if (!CanInteract_Implementation(InteractingCharacter))
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UWorldTravelSubsystem* TravelSubsystem = GameInstance->GetSubsystem<UWorldTravelSubsystem>())
		{
			TravelSubsystem->TravelToWorld(DestinationWorld);
		}
	}
}

void AWorldPortal::RefreshPresentation()
{
	if (!PortalLabel || !DestinationWorld)
	{
		return;
	}

	PortalLabel->SetText(FText::FromString(FString::Printf(
		TEXT("PORTAL: %s\nApproach + Press E"),
		*DestinationWorld->DisplayName.ToString())));

	const FLinearColor Color = DestinationWorld->PortalColor;
	PortalLabel->SetTextRenderColor(Color.ToFColor(true));
	if (PortalMaterial)
	{
		PortalMaterial->SetVectorParameterValue(TEXT("Color"), Color);
	}
}
