#include "World/Fantasy/Exploration/FantasyWorldChoiceActor.h"

#include "Characters/WorldWalkerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UI/WorldWalkerDialogueWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "WorldWalkerGameModeBase.h"

AFantasyWorldChoiceActor::AFantasyWorldChoiceActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f / 30.0f;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PedestalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChoicePedestal"));
	PedestalMesh->SetupAttachment(SceneRoot);
	PedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 12.0f));
	PedestalMesh->SetRelativeScale3D(FVector(0.86f, 0.86f, 0.18f));
	PedestalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RuneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChoiceRune"));
	RuneMesh->SetupAttachment(SceneRoot);
	RuneMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 118.0f));
	RuneMesh->SetRelativeRotation(FRotator(0.0f, 45.0f, 0.0f));
	RuneMesh->SetRelativeScale3D(FVector(0.23f, 0.23f, 1.02f));
	RuneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	HaloMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChoiceHalo"));
	HaloMesh->SetupAttachment(SceneRoot);
	HaloMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 154.0f));
	HaloMesh->SetRelativeScale3D(FVector(0.42f));
	HaloMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (CylinderMesh.Succeeded())
	{
		PedestalMesh->SetStaticMesh(CylinderMesh.Object);
	}
	if (CubeMesh.Succeeded())
	{
		RuneMesh->SetStaticMesh(CubeMesh.Object);
	}
	if (SphereMesh.Succeeded())
	{
		HaloMesh->SetStaticMesh(SphereMesh.Object);
	}

	ChoiceLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ChoiceLight"));
	ChoiceLight->SetupAttachment(SceneRoot);
	ChoiceLight->SetRelativeLocation(FVector(0.0f, 0.0f, 132.0f));
	ChoiceLight->SetIntensity(1500.0f);
	ChoiceLight->SetAttenuationRadius(430.0f);
	ChoiceLight->SetSourceRadius(18.0f);
	ChoiceLight->SetCastShadows(false);

	ChoiceTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ChoiceTrigger"));
	ChoiceTrigger->SetupAttachment(SceneRoot);
	ChoiceTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, 96.0f));
	ChoiceTrigger->SetBoxExtent(FVector(105.0f, 105.0f, 125.0f));
	ChoiceTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChoiceTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	ChoiceTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ChoiceTrigger->SetGenerateOverlapEvents(true);
	ChoiceTrigger->OnComponentBeginOverlap.AddDynamic(
		this,
		&AFantasyWorldChoiceActor::HandleTriggerBeginOverlap);

	ChoiceLabel = CreateDefaultSubobject<UWidgetComponent>(TEXT("ChoiceLabel"));
	ChoiceLabel->SetupAttachment(SceneRoot);
	ChoiceLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 242.0f));
	ChoiceLabel->SetWidgetSpace(EWidgetSpace::Screen);
	ChoiceLabel->SetDrawSize(FVector2D(520.0f, 128.0f));
	ChoiceLabel->SetDrawAtDesiredSize(true);
	ChoiceLabel->SetCullDistance(1800.0f);
	ChoiceLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChoiceLabel->SetWidgetClass(UWorldWalkerNPCPromptWidget::StaticClass());
}

void AFantasyWorldChoiceActor::ConfigureChoice(
	AWorldWalkerGameModeBase* InGameMode,
	const EFantasyWorldChoiceKind InKind,
	const int32 InChoiceIndex,
	const FText& InTitle,
	const FText& InDescription,
	const FLinearColor& InColor)
{
	OwningGameMode = InGameMode;
	ChoiceKind = InKind;
	ChoiceIndex = InChoiceIndex;
	ChoiceTitle = InTitle;
	ChoiceDescription = InDescription;
	ChoiceColor = InColor;
	RefreshPresentation();
}

void AFantasyWorldChoiceActor::BeginPlay()
{
	Super::BeginPlay();
	ChoiceLabel->InitWidget();
	RefreshPresentation();
	ArmedAtTime = GetWorld() ? GetWorld()->GetTimeSeconds() + 0.65f : 0.65f;
	SetChoiceEnabled(true);
}

void AFantasyWorldChoiceActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bChoiceEnabled || !GetWorld())
	{
		return;
	}

	const float Time = GetWorld()->GetTimeSeconds();
	RuneMesh->AddLocalRotation(FRotator(0.0f, 42.0f * DeltaSeconds, 0.0f));
	HaloMesh->SetRelativeScale3D(FVector(0.38f + 0.045f * FMath::Sin(Time * 2.7f)));
	ChoiceLight->SetIntensity(1320.0f + 280.0f * FMath::Sin(Time * 3.1f));
}

void AFantasyWorldChoiceActor::SetChoiceEnabled(const bool bEnabled)
{
	bChoiceEnabled = bEnabled;
	ChoiceTrigger->SetCollisionEnabled(
		bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	if (ChoiceLabel)
	{
		ChoiceLabel->SetVisibility(bEnabled);
	}
}

void AFantasyWorldChoiceActor::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bChoiceEnabled || ChoiceIndex == INDEX_NONE || !OwningGameMode
		|| !Cast<AWorldWalkerCharacter>(OtherActor)
		|| (GetWorld() && GetWorld()->GetTimeSeconds() < ArmedAtTime))
	{
		return;
	}

	SetChoiceEnabled(false);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_3D_CHOICE_TOUCHED Kind=%s Index=%d Title=%s"),
		ChoiceKind == EFantasyWorldChoiceKind::Route ? TEXT("Route") : TEXT("Event"),
		ChoiceIndex,
		*ChoiceTitle.ToString());
	if (ChoiceKind == EFantasyWorldChoiceKind::Route)
	{
		OwningGameMode->HandleRouteSelection(ChoiceIndex);
	}
	else
	{
		OwningGameMode->HandleEventSelection(ChoiceIndex);
	}
}

void AFantasyWorldChoiceActor::RefreshPresentation()
{
	if (ChoiceLight)
	{
		ChoiceLight->SetLightColor(ChoiceColor);
	}

	UMaterialInterface* EmissiveMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/Materials/M_W01_EmissiveSkyDetail.M_W01_EmissiveSkyDetail"));
	if (!EmissiveMaterial)
	{
		EmissiveMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	}

	DynamicMaterials.Reset();
	for (UStaticMeshComponent* Mesh : {RuneMesh.Get(), HaloMesh.Get()})
	{
		if (!Mesh || !EmissiveMaterial)
		{
			continue;
		}
		Mesh->SetMaterial(0, EmissiveMaterial);
		if (UMaterialInstanceDynamic* DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0))
		{
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), ChoiceColor * 3.0f);
			DynamicMaterials.Add(DynamicMaterial);
		}
	}

	if (ChoiceLabel)
	{
		ChoiceLabel->InitWidget();
		if (UWorldWalkerNPCPromptWidget* LabelWidget =
			Cast<UWorldWalkerNPCPromptWidget>(ChoiceLabel->GetWidget()))
		{
			LabelWidget->SetPrompt(
				ChoiceTitle.IsEmpty() ? FText::FromString(TEXT("未显现的命运")) : ChoiceTitle,
				ChoiceDescription.IsEmpty()
					? FText::FromString(TEXT("走入光柱选择"))
					: ChoiceDescription);
		}
	}
}
