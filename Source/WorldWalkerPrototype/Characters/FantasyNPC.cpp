#include "Characters/FantasyNPC.h"

#include "Animation/AnimSequence.h"
#include "Characters/WorldWalkerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Dialogue/FantasyDialogueLibrary.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UI/WorldWalkerDialogueWidget.h"
#include "UObject/ConstructorHelpers.h"

AFantasyNPC::AFantasyNPC()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 88.0f);
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = 0.0f;

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	// RPG Character Pack meshes are approximately twice human scale. Keep the
	// actor/capsule unchanged and normalize only their presentation component.
	GetMesh()->SetRelativeScale3D(FVector(0.53f));
	GetMesh()->SetVisibility(false, true);
	GetMesh()->SetHiddenInGame(true, true);

	PrototypeBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeBody"));
	PrototypeBody->SetupAttachment(RootComponent);
	PrototypeBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PrototypeBody->SetRelativeScale3D(FVector(0.42f, 0.42f, 1.55f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (BodyMesh.Succeeded())
	{
		PrototypeBody->SetStaticMesh(BodyMesh.Object);
	}

	PromptWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionPrompt"));
	PromptWidgetComponent->SetupAttachment(RootComponent);
	PromptWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 145.0f));
	PromptWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	PromptWidgetComponent->SetDrawSize(FVector2D(300.0f, 84.0f));
	PromptWidgetComponent->SetDrawAtDesiredSize(true);
	PromptWidgetComponent->SetCullDistance(800.0f);
	PromptWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PromptWidgetComponent->SetWidgetClass(UWorldWalkerNPCPromptWidget::StaticClass());
}

void AFantasyNPC::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->DisableMovement();
	RestingRotation = GetActorRotation();
	ActiveDialogue = FFantasyDialogueLibrary::BuildScript(NPCArchetype);
	if (!ConfiguredNameOverride.IsEmpty())
	{
		ActiveDialogue.SpeakerName = ConfiguredNameOverride;
	}
	ApplyArchetypePresentation();
	PromptWidgetComponent->InitWidget();
	RefreshPrompt();
	ScheduleAmbientGesture();
}

void AFantasyNPC::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	GetWorldTimerManager().ClearTimer(AmbientGestureTimer);
	GetWorldTimerManager().ClearTimer(ReturnToIdleTimer);
	FinishDialogue();
	Super::EndPlay(EndPlayReason);
}

void AFantasyNPC::ConfigureNPC(
	const EFantasyNPCArchetype Archetype,
	const FText& NameOverride)
{
	NPCArchetype = Archetype;
	ConfiguredNameOverride = NameOverride;
	ActiveDialogue = FFantasyDialogueLibrary::BuildScript(NPCArchetype);
	if (!NameOverride.IsEmpty())
	{
		ActiveDialogue.SpeakerName = NameOverride;
	}

	if (HasActorBegunPlay())
	{
		RestingRotation = GetActorRotation();
		ApplyArchetypePresentation();
		RefreshPrompt();
	}
}

bool AFantasyNPC::CanInteract_Implementation(const AWorldWalkerCharacter* InteractingCharacter) const
{
	return InteractingCharacter != nullptr
		&& !bDialogueActive
		&& InteractingCharacter->GetCharacterMovement() != nullptr
		&& InteractingCharacter->GetCharacterMovement()->MovementMode != MOVE_None
		&& !IsActorBeingDestroyed()
		&& !IsHidden();
}

void AFantasyNPC::Interact_Implementation(AWorldWalkerCharacter* InteractingCharacter)
{
	if (CanInteract_Implementation(InteractingCharacter))
	{
		BeginDialogue(InteractingCharacter);
	}
}

void AFantasyNPC::ApplyArchetypePresentation()
{
	IdleAnimation = nullptr;
	GestureAnimation = nullptr;

	USkeletalMesh* CharacterMesh = LoadFirstMesh(
		FFantasyDialogueLibrary::GetMeshAssetCandidates(NPCArchetype));
	if (CharacterMesh)
	{
		GetMesh()->SetSkeletalMeshAsset(CharacterMesh);
		GetMesh()->SetVisibility(true, true);
		GetMesh()->SetHiddenInGame(false, true);
		PrototypeBody->SetVisibility(false, true);
		PrototypeBody->SetHiddenInGame(true, true);

		IdleAnimation = LoadFirstAnimation(
			FFantasyDialogueLibrary::GetIdleAnimationCandidates(NPCArchetype));
		GestureAnimation = LoadFirstAnimation(
			FFantasyDialogueLibrary::GetGestureAnimationCandidates(NPCArchetype));
		ResumeIdle();
	}
	else
	{
		GetMesh()->SetSkeletalMeshAsset(nullptr);
		GetMesh()->SetVisibility(false, true);
		GetMesh()->SetHiddenInGame(true, true);
		PrototypeBody->SetVisibility(true, true);
		PrototypeBody->SetHiddenInGame(false, true);

		if (UMaterialInterface* BaseMaterial = PrototypeBody->GetMaterial(0))
		{
			UMaterialInstanceDynamic* BodyMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			BodyMaterial->SetVectorParameterValue(
				TEXT("Color"),
				FFantasyDialogueLibrary::GetFallbackBodyColor(NPCArchetype));
			PrototypeBody->SetMaterial(0, BodyMaterial);
		}
	}
}

void AFantasyNPC::RefreshPrompt()
{
	if (!PromptWidgetComponent)
	{
		return;
	}

	PromptWidgetComponent->InitWidget();
	if (UWorldWalkerNPCPromptWidget* PromptWidget =
		Cast<UWorldWalkerNPCPromptWidget>(PromptWidgetComponent->GetUserWidgetObject()))
	{
		PromptWidget->SetPrompt(ActiveDialogue.SpeakerName);
	}
}

void AFantasyNPC::BeginDialogue(AWorldWalkerCharacter* InteractingCharacter)
{
	APlayerController* PlayerController = InteractingCharacter
		? Cast<APlayerController>(InteractingCharacter->GetController())
		: nullptr;
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	UWorldWalkerDialogueWidget* NewWidget = CreateWidget<UWorldWalkerDialogueWidget>(
		PlayerController,
		UWorldWalkerDialogueWidget::StaticClass());
	if (!NewWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("W01 dialogue widget creation failed for %s."), *GetName());
		return;
	}

	bDialogueActive = true;
	ActiveInteractingPlayer = InteractingCharacter;
	DialogueWidget = NewWidget;
	GetWorldTimerManager().ClearTimer(AmbientGestureTimer);
	ActiveInteractingPlayer->SetCombatLocked(true);
	SetPromptVisible(false);

	const FVector ToPlayer = ActiveInteractingPlayer->GetActorLocation() - GetActorLocation();
	if (!ToPlayer.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.0f, ToPlayer.Rotation().Yaw, 0.0f));
		ActiveInteractingPlayer->SetActorRotation(FRotator(0.0f, (-ToPlayer).Rotation().Yaw, 0.0f));
	}

	DialogueWidget->ConfigureDialogue(ActiveDialogue);
	DialogueWidget->OnChoiceSelected.AddUObject(this, &AFantasyNPC::HandleChoiceSelected);
	DialogueWidget->OnDialogueClosed.AddUObject(this, &AFantasyNPC::FinishDialogue);
	DialogueWidget->AddToViewport(120);

	PlayerController->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(DialogueWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	DialogueWidget->SetKeyboardFocus();
	PlayGesture(false);
}

void AFantasyNPC::FinishDialogue()
{
	if (!bDialogueActive && !DialogueWidget)
	{
		return;
	}

	bDialogueActive = false;
	GetWorldTimerManager().ClearTimer(ReturnToIdleTimer);

	UWorldWalkerDialogueWidget* WidgetToRemove = DialogueWidget;
	DialogueWidget = nullptr;
	if (WidgetToRemove)
	{
		WidgetToRemove->OnChoiceSelected.RemoveAll(this);
		WidgetToRemove->OnDialogueClosed.RemoveAll(this);
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
		SetActorRotation(RestingRotation);
		SetPromptVisible(true);
		ResumeIdle();
		ScheduleAmbientGesture();
	}
}

void AFantasyNPC::HandleChoiceSelected(const int32 ChoiceIndex)
{
	if (ActiveDialogue.Choices.IsValidIndex(ChoiceIndex))
	{
		PlayGesture(false);
	}
}

void AFantasyNPC::HandleAmbientGesture()
{
	if (!bDialogueActive)
	{
		PlayGesture(true);
	}
	ScheduleAmbientGesture();
}

void AFantasyNPC::PlayGesture(const bool bAmbient)
{
	if (bAmbient)
	{
		const float YawOffset = FMath::FRandRange(-9.0f, 9.0f);
		SetActorRotation(RestingRotation + FRotator(0.0f, YawOffset, 0.0f));
	}

	if (GestureAnimation && GetMesh()->GetSkeletalMeshAsset())
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		GetMesh()->PlayAnimation(GestureAnimation, false);
		const float ReturnDelay = FMath::Clamp(GestureAnimation->GetPlayLength(), 0.75f, 2.4f);
		GetWorldTimerManager().SetTimer(
			ReturnToIdleTimer,
			this,
			&AFantasyNPC::ResumeIdle,
			ReturnDelay,
			false);
	}
	else if (bAmbient)
	{
		GetWorldTimerManager().SetTimer(
			ReturnToIdleTimer,
			this,
			&AFantasyNPC::ResumeIdle,
			1.2f,
			false);
	}
}

void AFantasyNPC::ResumeIdle()
{
	if (!bDialogueActive)
	{
		SetActorRotation(RestingRotation);
	}

	if (IdleAnimation && GetMesh()->GetSkeletalMeshAsset())
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		GetMesh()->PlayAnimation(IdleAnimation, true);
	}
}

void AFantasyNPC::ScheduleAmbientGesture()
{
	if (!GetWorld() || bDialogueActive)
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		AmbientGestureTimer,
		this,
		&AFantasyNPC::HandleAmbientGesture,
		FMath::FRandRange(5.5f, 10.5f),
		false);
}

void AFantasyNPC::SetPromptVisible(const bool bVisible)
{
	if (PromptWidgetComponent)
	{
		PromptWidgetComponent->SetVisibility(bVisible, true);
		PromptWidgetComponent->SetHiddenInGame(!bVisible, true);
	}
}

USkeletalMesh* AFantasyNPC::LoadFirstMesh(const TArray<FString>& Candidates) const
{
	for (const FString& Candidate : Candidates)
	{
		if (USkeletalMesh* LoadedMesh = LoadObject<USkeletalMesh>(nullptr, *Candidate))
		{
			return LoadedMesh;
		}
	}
	return nullptr;
}

UAnimSequence* AFantasyNPC::LoadFirstAnimation(const TArray<FString>& Candidates) const
{
	for (const FString& Candidate : Candidates)
	{
		if (UAnimSequence* Animation = LoadObject<UAnimSequence>(nullptr, *Candidate))
		{
			return Animation;
		}
	}
	return nullptr;
}
