#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WorldWalkerPlayerController.generated.h"

class UWorldWalkerHUDWidget;
class UTexture2D;

UCLASS()
class WORLDWALKERPROTOTYPE_API AWorldWalkerPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	void EnterCombat(
		int32 PlayerHealth,
		int32 PlayerMaxHealth,
		int32 EnemyHealth,
		int32 EnemyMaxHealth,
		const FString& EnemyDisplayName,
		UTexture2D* EnemyPortrait);
	void RefreshCombat(
		int32 PlayerHealth,
		int32 PlayerMaxHealth,
		int32 EnemyHealth,
		int32 EnemyMaxHealth,
		const FString& EnemyDisplayName,
		UTexture2D* EnemyPortrait,
		int32 CurrentActionPoints,
		int32 MaxActionPoints,
		int32 CurrentMana,
		int32 EquipmentCount,
		int32 CurrentBlock,
		int32 DrawPileCount,
		int32 DiscardPileCount,
		int32 ExhaustPileCount,
		const FString& SealText,
		const FString& PlayerStatusText,
		const FString& EnemyStatusText,
		const FString& NextIntentText,
		const TArray<FString>& CardLabels,
		const TArray<bool>& CardPlayable,
		const TArray<UTexture2D*>& CardArtworks,
		const TArray<FLinearColor>& CardSchoolTints);
	void SetCombatMessage(const FString& Message, bool bCanAttack);
	void ShowCombatResult(bool bPlayerWon);
	void ShowRewardSelection(
		const TArray<FString>& CardLabels,
		const TArray<UTexture2D*>& CardArtworks,
		const TArray<FLinearColor>& CardSchoolTints);
	void ShowRewardConfirmation(const FString& ConfirmationText);
	void ShowProfessionSelection(const TArray<FString>& ChoiceLabels);
	void ShowRouteSelection(const FString& RunSummary, const TArray<FString>& ChoiceLabels);
	void ShowEventSelection(
		const FString& Title,
		const FString& Lore,
		const TArray<FString>& ChoiceLabels);
	void ShowNodeResolution(const FString& Message, bool bChapterComplete);
	void ExitCombatToExploration(const FString& ConfirmationText);
	void SetProfessionLabel(const FString& ProfessionLabel);
	void SetDeckAccessEnabled(bool bEnabled);
	void ShowDeckViewer(const FString& Title, const FString& DeckSummary);
	void HideDeckViewer(bool bReturnToCombatInput, bool bReturnToUIOnly);

private:
	void ToggleDeckViewer();

	UPROPERTY(Transient)
	TObjectPtr<UWorldWalkerHUDWidget> HUDWidget;

	bool bDeckViewerOpen = false;
};
