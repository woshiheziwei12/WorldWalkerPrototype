#include "World/Fantasy/Exploration/FantasyFateAltar.h"

#include "Cards/CardCombatComponent.h"
#include "Characters/WorldWalkerCharacter.h"
#include "Combat/CombatantComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UI/WorldWalkerDialogueWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "World/Fantasy/Exploration/FantasyFateAltarWidget.h"

namespace
{
	const FLinearColor DormantRuneColor(0.36f, 0.19f, 0.055f, 1.0f);
	const FLinearColor BloodRuneColor(0.82f, 0.035f, 0.025f, 1.0f);
	const FLinearColor GraceRuneColor(1.0f, 0.38f, 0.055f, 1.0f);
	const FLinearColor InsightRuneColor(0.08f, 0.31f, 1.0f, 1.0f);
}

AFantasyFateAltar::AFantasyFateAltar()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f / 30.0f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AltarBase"));
	BaseMesh->SetupAttachment(SceneRoot);
	BaseMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 8.0f));
	BaseMesh->SetRelativeScale3D(FVector(1.45f, 1.45f, 0.16f));

	StepMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AltarStep"));
	StepMesh->SetupAttachment(SceneRoot);
	StepMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 22.0f));
	StepMesh->SetRelativeScale3D(FVector(1.10f, 1.10f, 0.12f));

	MonolithMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FateMonolith"));
	MonolithMesh->SetupAttachment(SceneRoot);
	MonolithMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 92.0f));
	MonolithMesh->SetRelativeRotation(FRotator(0.0f, 45.0f, 0.0f));
	MonolithMesh->SetRelativeScale3D(FVector(0.44f, 0.44f, 1.28f));

	CrownMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FateCrown"));
	CrownMesh->SetupAttachment(SceneRoot);
	CrownMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 161.0f));
	CrownMesh->SetRelativeScale3D(FVector(0.31f));

	RunePivot = CreateDefaultSubobject<USceneComponent>(TEXT("RunePivot"));
	RunePivot->SetupAttachment(SceneRoot);
	RunePivot->SetRelativeLocation(FVector(0.0f, 0.0f, 108.0f));

	RuneShardA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RuneShardA"));
	RuneShardA->SetupAttachment(RunePivot);
	RuneShardA->SetRelativeLocation(FVector(76.0f, 0.0f, 0.0f));
	RuneShardA->SetRelativeRotation(FRotator(22.0f, 12.0f, 34.0f));
	RuneShardA->SetRelativeScale3D(FVector(0.08f, 0.035f, 0.18f));

	RuneShardB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RuneShardB"));
	RuneShardB->SetupAttachment(RunePivot);
	RuneShardB->SetRelativeLocation(FVector(-38.0f, 65.8f, 0.0f));
	RuneShardB->SetRelativeRotation(FRotator(-18.0f, 48.0f, -28.0f));
	RuneShardB->SetRelativeScale3D(FVector(0.08f, 0.035f, 0.18f));

	RuneShardC = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RuneShardC"));
	RuneShardC->SetupAttachment(RunePivot);
	RuneShardC->SetRelativeLocation(FVector(-38.0f, -65.8f, 0.0f));
	RuneShardC->SetRelativeRotation(FRotator(15.0f, -42.0f, 31.0f));
	RuneShardC->SetRelativeScale3D(FVector(0.08f, 0.035f, 0.18f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	if (CylinderMesh.Succeeded())
	{
		BaseMesh->SetStaticMesh(CylinderMesh.Object);
		StepMesh->SetStaticMesh(CylinderMesh.Object);
	}
	if (CubeMesh.Succeeded())
	{
		MonolithMesh->SetStaticMesh(CubeMesh.Object);
		RuneShardA->SetStaticMesh(CubeMesh.Object);
		RuneShardB->SetStaticMesh(CubeMesh.Object);
		RuneShardC->SetStaticMesh(CubeMesh.Object);
	}
	if (SphereMesh.Succeeded())
	{
		CrownMesh->SetStaticMesh(SphereMesh.Object);
	}

	BaseMesh->SetCollisionProfileName(TEXT("BlockAll"));
	StepMesh->SetCollisionProfileName(TEXT("BlockAll"));
	MonolithMesh->SetCollisionProfileName(TEXT("BlockAll"));
	CrownMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RuneShardA->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RuneShardB->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RuneShardC->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FateLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FateLight"));
	FateLight->SetupAttachment(SceneRoot);
	FateLight->SetRelativeLocation(FVector(0.0f, 0.0f, 132.0f));
	FateLight->SetLightColor(DormantRuneColor);
	FateLight->SetIntensity(2200.0f);
	FateLight->SetAttenuationRadius(650.0f);
	FateLight->SetSourceRadius(22.0f);
	FateLight->SetCastShadows(true);
	FateLight->SetVolumetricScatteringIntensity(0.65f);

	PromptWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionPrompt"));
	PromptWidgetComponent->SetupAttachment(SceneRoot);
	PromptWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 225.0f));
	PromptWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	PromptWidgetComponent->SetDrawSize(FVector2D(360.0f, 92.0f));
	PromptWidgetComponent->SetDrawAtDesiredSize(true);
	PromptWidgetComponent->SetCullDistance(900.0f);
	PromptWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PromptWidgetComponent->SetWidgetClass(UWorldWalkerNPCPromptWidget::StaticClass());
}

AFantasyFateAltar* AFantasyFateAltar::SpawnConfigured(
	UWorld* World,
	const FTransform& SpawnTransform,
	const FName EventId,
	const FText& Title,
	const FText& Lore)
{
	if (!World)
	{
		return nullptr;
	}

	AFantasyFateAltar* Altar = World->SpawnActorDeferred<AFantasyFateAltar>(
		StaticClass(),
		SpawnTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Altar)
	{
		return nullptr;
	}

	Altar->ConfigureAltar(EventId, Title, Lore);
	Altar->FinishSpawning(SpawnTransform);
	return Altar;
}

void AFantasyFateAltar::ConfigureAltar(
	const FName InEventId,
	const FText& InTitle,
	const FText& InLore)
{
	if (!InEventId.IsNone())
	{
		EventId = InEventId;
	}
	if (!InTitle.IsEmpty())
	{
		EventTitle = InTitle;
	}
	if (!InLore.IsEmpty())
	{
		EventLore = InLore;
	}

	if (HasActorBegunPlay())
	{
		RefreshPrompt();
	}
}

void AFantasyFateAltar::BeginPlay()
{
	Super::BeginPlay();

	UMaterialInterface* DarkStoneMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Environment/SM_W01_Path/Stone_Dark.Stone_Dark"));
	UMaterialInterface* LightStoneMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/Environment/SM_W01_Path/Stone_Light.Stone_Light"));
	UMaterialInterface* EmissiveMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	if (DarkStoneMaterial)
	{
		BaseMesh->SetMaterial(0, DarkStoneMaterial);
		MonolithMesh->SetMaterial(0, DarkStoneMaterial);
	}
	if (LightStoneMaterial)
	{
		StepMesh->SetMaterial(0, LightStoneMaterial);
		CrownMesh->SetMaterial(0, LightStoneMaterial);
	}
	if (EmissiveMaterial)
	{
		RuneShardA->SetMaterial(0, EmissiveMaterial);
		RuneShardB->SetMaterial(0, EmissiveMaterial);
		RuneShardC->SetMaterial(0, EmissiveMaterial);
	}

	ApplyMeshTint(BaseMesh, FLinearColor(0.025f, 0.021f, 0.032f, 1.0f));
	ApplyMeshTint(StepMesh, FLinearColor(0.055f, 0.044f, 0.060f, 1.0f));

	MonolithMaterial = MonolithMesh
		? MonolithMesh->CreateAndSetMaterialInstanceDynamic(0)
		: nullptr;
	if (MonolithMaterial)
	{
		MonolithMaterial->SetVectorParameterValue(
			TEXT("Color"),
			FLinearColor(0.065f, 0.052f, 0.082f, 1.0f));
	}

	const TArray<UStaticMeshComponent*> RuneMeshes =
	{
		RuneShardA.Get(),
		RuneShardB.Get(),
		RuneShardC.Get()
	};
	for (UStaticMeshComponent* RuneMesh : RuneMeshes)
	{
		if (UMaterialInstanceDynamic* RuneMaterial =
			RuneMesh ? RuneMesh->CreateAndSetMaterialInstanceDynamic(0) : nullptr)
		{
			RuneMaterial->SetVectorParameterValue(TEXT("Color"), DormantRuneColor);
			RuneMaterials.Add(RuneMaterial);
		}
	}

	PromptWidgetComponent->InitWidget();
	RefreshPrompt();

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_FATE_ALTAR_READY Event=%s Choices=3 OneShot=true"),
		*EventId.ToString());
}

void AFantasyFateAltar::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!GetWorld())
	{
		return;
	}

	const float Time = GetWorld()->GetTimeSeconds();
	const float RotationSpeed = bBattleBoonApplied ? 7.0f : (bResolved ? 18.0f : 31.0f);
	if (RunePivot)
	{
		RunePivot->AddLocalRotation(FRotator(0.0f, RotationSpeed * DeltaSeconds, 0.0f));
	}

	const float Pulse = 0.88f + 0.12f * FMath::Sin(Time * (bResolved ? 3.4f : 2.2f));
	if (FateLight)
	{
		const float BaseIntensity = bBattleBoonApplied
			? 1050.0f
			: (bResolved ? 2850.0f : 2200.0f);
		FateLight->SetIntensity(BaseIntensity * Pulse);
	}
	if (CrownMesh)
	{
		CrownMesh->SetRelativeScale3D(FVector(0.29f + Pulse * 0.025f));
	}
}

void AFantasyFateAltar::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	GetWorldTimerManager().ClearTimer(BattleWatchTimer);
	FinishEventUI();
	BoonRecipient = nullptr;
	Super::EndPlay(EndPlayReason);
}

bool AFantasyFateAltar::CanInteract_Implementation(
	const AWorldWalkerCharacter* InteractingCharacter) const
{
	return InteractingCharacter != nullptr
		&& !bResolved
		&& !bEventUIActive
		&& !IsActorBeingDestroyed()
		&& !IsHidden()
		&& InteractingCharacter->GetCharacterMovement() != nullptr
		&& InteractingCharacter->GetCharacterMovement()->MovementMode != MOVE_None;
}

void AFantasyFateAltar::Interact_Implementation(AWorldWalkerCharacter* InteractingCharacter)
{
	if (CanInteract_Implementation(InteractingCharacter))
	{
		BeginEvent(InteractingCharacter);
	}
}

void AFantasyFateAltar::BeginEvent(AWorldWalkerCharacter* InteractingCharacter)
{
	APlayerController* PlayerController = InteractingCharacter
		? Cast<APlayerController>(InteractingCharacter->GetController())
		: nullptr;
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	UFantasyFateAltarWidget* NewWidget = CreateWidget<UFantasyFateAltarWidget>(
		PlayerController,
		UFantasyFateAltarWidget::StaticClass());
	if (!NewWidget)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("W01 fate altar widget creation failed. Event=%s"),
			*EventId.ToString());
		return;
	}

	bEventUIActive = true;
	ActiveInteractingPlayer = InteractingCharacter;
	EventWidget = NewWidget;
	ActiveInteractingPlayer->SetCombatLocked(true);
	SetPromptVisible(false);

	const FVector ToAltar = GetActorLocation() - ActiveInteractingPlayer->GetActorLocation();
	if (!ToAltar.IsNearlyZero())
	{
		ActiveInteractingPlayer->SetActorRotation(
			FRotator(0.0f, ToAltar.Rotation().Yaw, 0.0f));
	}

	EventWidget->ConfigureEvent(EventTitle, EventLore);
	EventWidget->OnChoiceSelected.AddUObject(this, &AFantasyFateAltar::HandleChoiceSelected);
	EventWidget->OnClosed.AddUObject(this, &AFantasyFateAltar::HandleWidgetClosed);
	EventWidget->AddToViewport(125);

	PlayerController->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(EventWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	EventWidget->SetKeyboardFocus();

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_FATE_ALTAR_OPENED Event=%s"),
		*EventId.ToString());
}

void AFantasyFateAltar::HandleChoiceSelected(const EFantasyFateAltarChoice Choice)
{
	if (bResolved || !IsValid(ActiveInteractingPlayer)
		|| Choice == EFantasyFateAltarChoice::None)
	{
		return;
	}

	BoonRecipient = ActiveInteractingPlayer;
	int32 ImmediateAmount = 0;
	PendingValor = 0;
	PendingBlock = 0;

	if (UCombatantComponent* Combatant = BoonRecipient->GetCombatantComponent())
	{
		const int32 HealthBefore = Combatant->GetCurrentHealth();
		switch (Choice)
		{
		case EFantasyFateAltarChoice::BloodOath:
		{
			const int32 SafeDamage = FMath::Min(12, FMath::Max(0, HealthBefore - 1));
			Combatant->ReceiveDamage(SafeDamage);
			ImmediateAmount = HealthBefore - Combatant->GetCurrentHealth();
			PendingValor = 2;
			BoonRecipient->PlayFantasyHitReactionAnimation();
			break;
		}
		case EFantasyFateAltarChoice::AshenGrace:
			Combatant->RestoreHealth(30);
			ImmediateAmount = Combatant->GetCurrentHealth() - HealthBefore;
			PendingBlock = 10;
			break;
		case EFantasyFateAltarChoice::RuneInsight:
			PendingValor = 1;
			PendingBlock = 6;
			break;
		default:
			return;
		}
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("W01 fate altar recipient lacks CombatantComponent. Event=%s"),
			*EventId.ToString());
		return;
	}

	ChosenOutcome = Choice;
	bResolved = true;
	ApplyResolvedPresentation(Choice);
	RefreshPrompt();

	const FString ResolutionText = BuildResolutionText(Choice, ImmediateAmount);
	if (EventWidget)
	{
		EventWidget->ShowResolution(Choice, ResolutionText);
	}
	WatchForBattleStart();

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_FATE_ALTAR_RESOLVED Event=%s Choice=%s Immediate=%d PendingValor=%d PendingBlock=%d"),
		*EventId.ToString(),
		ChoiceToLogName(Choice),
		ImmediateAmount,
		PendingValor,
		PendingBlock);
}

void AFantasyFateAltar::HandleWidgetClosed()
{
	FinishEventUI();
}

void AFantasyFateAltar::FinishEventUI()
{
	if (!bEventUIActive && !EventWidget)
	{
		return;
	}

	bEventUIActive = false;
	UFantasyFateAltarWidget* WidgetToRemove = EventWidget;
	EventWidget = nullptr;
	if (WidgetToRemove)
	{
		WidgetToRemove->OnChoiceSelected.RemoveAll(this);
		WidgetToRemove->OnClosed.RemoveAll(this);
		WidgetToRemove->RemoveFromParent();
	}

	if (IsValid(ActiveInteractingPlayer))
	{
		if (APlayerController* PlayerController =
			Cast<APlayerController>(ActiveInteractingPlayer->GetController()))
		{
			PlayerController->bShowMouseCursor = false;
			PlayerController->SetInputMode(FInputModeGameOnly());
		}
		ActiveInteractingPlayer->SetCombatLocked(false);
	}
	ActiveInteractingPlayer = nullptr;

	if (!bEndingPlay)
	{
		SetPromptVisible(true);
		RefreshPrompt();
	}
}

void AFantasyFateAltar::WatchForBattleStart()
{
	if (!GetWorld() || bBattleBoonApplied || !IsValid(BoonRecipient))
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		BattleWatchTimer,
		this,
		&AFantasyFateAltar::TryApplyPendingBattleBoon,
		0.15f,
		true,
		0.15f);
}

void AFantasyFateAltar::TryApplyPendingBattleBoon()
{
	if (bBattleBoonApplied)
	{
		GetWorldTimerManager().ClearTimer(BattleWatchTimer);
		return;
	}
	if (!IsValid(BoonRecipient))
	{
		GetWorldTimerManager().ClearTimer(BattleWatchTimer);
		return;
	}

	UCardCombatComponent* CardCombat = BoonRecipient->GetCardCombatComponent();
	if (!CardCombat || CardCombat->GetCurrentEnergy() <= 0 || CardCombat->GetHand().IsEmpty())
	{
		return;
	}

	const int32 GrantedValor = CardCombat->AddValor(PendingValor);
	CardCombat->AddBlock(PendingBlock);
	bBattleBoonApplied = true;
	GetWorldTimerManager().ClearTimer(BattleWatchTimer);
	RefreshPrompt();

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_FATE_ALTAR_BATTLE_BOON_APPLIED Event=%s Choice=%s Valor=%d Block=%d Hand=%d"),
		*EventId.ToString(),
		ChoiceToLogName(ChosenOutcome),
		GrantedValor,
		PendingBlock,
		CardCombat->GetHand().Num());
}

void AFantasyFateAltar::ApplyResolvedPresentation(const EFantasyFateAltarChoice Choice)
{
	FLinearColor ResolvedColor = DormantRuneColor;
	switch (Choice)
	{
	case EFantasyFateAltarChoice::BloodOath:
		ResolvedColor = BloodRuneColor;
		break;
	case EFantasyFateAltarChoice::AshenGrace:
		ResolvedColor = GraceRuneColor;
		break;
	case EFantasyFateAltarChoice::RuneInsight:
		ResolvedColor = InsightRuneColor;
		break;
	default:
		break;
	}

	if (FateLight)
	{
		FateLight->SetLightColor(ResolvedColor);
		FateLight->SetIntensity(3200.0f);
	}
	if (MonolithMaterial)
	{
		MonolithMaterial->SetVectorParameterValue(
			TEXT("Color"),
			FMath::Lerp(FLinearColor(0.045f, 0.040f, 0.060f, 1.0f), ResolvedColor, 0.20f));
	}
	for (const TObjectPtr<UMaterialInstanceDynamic>& RuneMaterial : RuneMaterials)
	{
		if (RuneMaterial)
		{
			RuneMaterial->SetVectorParameterValue(TEXT("Color"), ResolvedColor);
		}
	}
}

void AFantasyFateAltar::RefreshPrompt()
{
	if (!PromptWidgetComponent)
	{
		return;
	}

	PromptWidgetComponent->InitWidget();
	if (UWorldWalkerNPCPromptWidget* PromptWidget =
		Cast<UWorldWalkerNPCPromptWidget>(PromptWidgetComponent->GetUserWidgetObject()))
	{
		FText ActionText = FText::FromString(TEXT("按 E 聆听命运"));
		if (bBattleBoonApplied)
		{
			ActionText = FText::FromString(TEXT("命运已兑现 · 石碑沉寂"));
		}
		else if (bResolved)
		{
			switch (ChosenOutcome)
			{
			case EFantasyFateAltarChoice::BloodOath:
				ActionText = FText::FromString(TEXT("血誓已立 · 等待战斗"));
				break;
			case EFantasyFateAltarChoice::AshenGrace:
				ActionText = FText::FromString(TEXT("圣烬已赐福 · 等待战斗"));
				break;
			case EFantasyFateAltarChoice::RuneInsight:
				ActionText = FText::FromString(TEXT("秘仪已铭刻 · 等待战斗"));
				break;
			default:
				break;
			}
		}
		PromptWidget->SetPrompt(EventTitle, ActionText);
	}
}

void AFantasyFateAltar::SetPromptVisible(const bool bVisible)
{
	if (PromptWidgetComponent)
	{
		PromptWidgetComponent->SetVisibility(bVisible, true);
		PromptWidgetComponent->SetHiddenInGame(!bVisible, true);
	}
}

void AFantasyFateAltar::ApplyMeshTint(
	UStaticMeshComponent* Mesh,
	const FLinearColor& Color)
{
	if (!Mesh)
	{
		return;
	}

	if (UMaterialInstanceDynamic* DynamicMaterial =
		Mesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

FString AFantasyFateAltar::BuildResolutionText(
	const EFantasyFateAltarChoice Choice,
	const int32 ImmediateAmount) const
{
	switch (Choice)
	{
	case EFantasyFateAltarChoice::BloodOath:
		return FString::Printf(
			TEXT("石碑饮下了 %d 点生命。黑红符文已经烙入你的誓言：下场战斗开始时获得 2 点英勇。"),
			ImmediateAmount);
	case EFantasyFateAltarChoice::AshenGrace:
		return FString::Printf(
			TEXT("圣烬修复了 %d 点生命。温暖的灰光化作护盾：下场战斗开始时获得 10 点格挡。"),
			ImmediateAmount);
	case EFantasyFateAltarChoice::RuneInsight:
		return TEXT("三枚符文在你眼前重排了未来。下场战斗开始时获得 1 点英勇与 6 点格挡。");
	default:
		return TEXT("命运保持沉默。");
	}
}

const TCHAR* AFantasyFateAltar::ChoiceToLogName(const EFantasyFateAltarChoice Choice)
{
	switch (Choice)
	{
	case EFantasyFateAltarChoice::BloodOath: return TEXT("BloodOath");
	case EFantasyFateAltarChoice::AshenGrace: return TEXT("AshenGrace");
	case EFantasyFateAltarChoice::RuneInsight: return TEXT("RuneInsight");
	default: return TEXT("None");
	}
}
