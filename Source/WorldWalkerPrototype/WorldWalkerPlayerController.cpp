#include "WorldWalkerPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "UI/WorldWalkerHUDWidget.h"
#include "WorldWalkerGameModeBase.h"

void AWorldWalkerPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		HUDWidget = CreateWidget<UWorldWalkerHUDWidget>(this, UWorldWalkerHUDWidget::StaticClass());
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
			HUDWidget->ShowExploration();
			if (const AWorldWalkerGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AWorldWalkerGameModeBase>())
			{
				HUDWidget->SetExplorationMessage(GameMode->GetExplorationMessage());
			}
			UE_LOG(LogTemp, Display, TEXT("WorldWalker HUD widget created successfully."));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("WorldWalker HUD widget creation failed."));
		}

		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
	}
}

void AWorldWalkerPlayerController::EnterCombat(
	const int32 PlayerHealth,
	const int32 PlayerMaxHealth,
	const int32 EnemyHealth,
	const int32 EnemyMaxHealth)
{
	if (HUDWidget)
	{
		HUDWidget->ShowCombat(PlayerHealth, PlayerMaxHealth, EnemyHealth, EnemyMaxHealth);
	}

	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AWorldWalkerPlayerController::RefreshCombat(
	const int32 PlayerHealth,
	const int32 PlayerMaxHealth,
	const int32 EnemyHealth,
	const int32 EnemyMaxHealth,
	const int32 CurrentEnergy,
	const int32 MaxEnergy,
	const int32 CurrentBlock,
	const int32 CurrentValor,
	const int32 MaxValor,
	const int32 DrawPileCount,
	const int32 DiscardPileCount,
	const int32 ExhaustPileCount,
	const FString& SealText,
	const FString& PlayerStatusText,
	const FString& EnemyStatusText,
	const FString& NextIntentText,
	const TArray<FString>& CardLabels,
	const TArray<bool>& CardPlayable,
	const TArray<UTexture2D*>& CardArtworks,
	const TArray<FLinearColor>& CardSchoolTints)
{
	if (HUDWidget)
	{
		HUDWidget->RefreshCombatState(
			PlayerHealth,
			PlayerMaxHealth,
			EnemyHealth,
			EnemyMaxHealth,
			CurrentEnergy,
			MaxEnergy,
			CurrentBlock,
			CurrentValor,
			MaxValor,
			DrawPileCount,
			DiscardPileCount,
			ExhaustPileCount,
			SealText,
			PlayerStatusText,
			EnemyStatusText,
			NextIntentText,
			CardLabels,
			CardPlayable,
			CardArtworks,
			CardSchoolTints);
	}
}

void AWorldWalkerPlayerController::SetCombatMessage(const FString& Message, const bool bCanAttack)
{
	if (HUDWidget)
	{
		HUDWidget->SetCombatMessage(Message, bCanAttack);
	}
}

void AWorldWalkerPlayerController::ShowCombatResult(const bool bPlayerWon)
{
	if (HUDWidget)
	{
		HUDWidget->ShowCombatResult(bPlayerWon);
	}
}

void AWorldWalkerPlayerController::ShowRewardSelection(
	const TArray<FString>& CardLabels,
	const TArray<UTexture2D*>& CardArtworks,
	const TArray<FLinearColor>& CardSchoolTints)
{
	if (HUDWidget)
	{
		HUDWidget->ShowRewardSelection(CardLabels, CardArtworks, CardSchoolTints);
	}
}

void AWorldWalkerPlayerController::ShowRewardConfirmation(const FString& ConfirmationText)
{
	if (HUDWidget)
	{
		HUDWidget->ShowRewardConfirmation(ConfirmationText);
	}
}

void AWorldWalkerPlayerController::ExitCombatToExploration(const FString& ConfirmationText)
{
	if (HUDWidget)
	{
		HUDWidget->ShowExploration();
		HUDWidget->SetExplorationMessage(ConfirmationText);
	}

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}
