#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WorldWalkerHUDWidget.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UImage;
class UProgressBar;
class UScrollBox;
class UTextBlock;
class UTexture2D;
class UVerticalBox;

UCLASS()
class WORLDWALKERPROTOTYPE_API UWorldWalkerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowExploration();
	void SetExplorationMessage(const FString& Message);
	void ShowCombat(
		int32 PlayerHealth,
		int32 PlayerMaxHealth,
		int32 EnemyHealth,
		int32 EnemyMaxHealth,
		const FString& EnemyDisplayName,
		UTexture2D* EnemyPortrait);
	void RefreshCombatState(
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
	void SetCombatMessage(const FString& Message, bool bCanAct);
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
	void SetPlayerDisplayName(const FString& DisplayName);
	void SetDeckAccessEnabled(bool bEnabled);
	void ShowDeckViewer(const FString& Title, const FString& DeckSummary);
	void HideDeckViewer();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeConstruct() override;

private:
	enum class EChoicePanelMode : uint8
	{
		Hidden,
		Profession,
		Route,
		Event,
		Resolution
	};

	void BuildWidgetTree();
	UTextBlock* MakeText(const TCHAR* Name, const FString& Text, int32 FontSize);
	void ShowChoicePanel(
		EChoicePanelMode InMode,
		const FString& Title,
		const FString& Summary,
		const FString& Lore,
		const TArray<FString>& ChoiceLabels);
	void HandleChoiceClicked(int32 ChoiceIndex);

	UFUNCTION()
	void HandleDeckAccessClicked();

	UFUNCTION()
	void HandleDeckCloseClicked();

	UFUNCTION()
	void HandleCard0Clicked();

	UFUNCTION()
	void HandleCard1Clicked();

	UFUNCTION()
	void HandleCard2Clicked();

	UFUNCTION()
	void HandleCard3Clicked();

	UFUNCTION()
	void HandleCard4Clicked();

	void HandleCardClicked(int32 HandIndex);

	UFUNCTION()
	void HandleEndTurnClicked();

	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleChoice0Clicked();

	UFUNCTION()
	void HandleChoice1Clicked();

	UFUNCTION()
	void HandleChoice2Clicked();

	UFUNCTION()
	void HandleChoiceContinueClicked();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ExplorationPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ExplorationText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ChoicePanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ChoiceTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ChoiceSummaryText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ChoiceLoreText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> ChoiceButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> ChoiceButtonLabels;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ChoiceContinueButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ChoiceContinueLabel;

	UPROPERTY(Transient)
	TObjectPtr<UButton> DeckAccessButton;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> DeckPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DeckTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DeckListText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> DeckCloseButton;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> CombatPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PlayerHealthText;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> PlayerHealthBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EnemyHealthText;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> EnemyHealthBar;

	UPROPERTY(Transient)
	TObjectPtr<UImage> EnemyPortraitImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EnergyText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BlockText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ValorText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PileText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SealTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PlayerStatusTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EnemyStatusTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> IntentTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CombatMessageText;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> CardRow;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> CardButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> CardButtonLabels;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> CardArtworkImages;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> CardArtworkFrames;

	UPROPERTY(Transient)
	TObjectPtr<UButton> EndTurnButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RestartButtonLabel;

	TArray<bool> CachedCardPlayable;
	bool bPlayerCanAct = false;
	bool bShowingRewardChoices = false;
	bool bRewardConfirmed = false;
	bool bChoiceInputLocked = false;
	FString PlayerDisplayName = TEXT("女骑士");
	EChoicePanelMode ChoicePanelMode = EChoicePanelMode::Hidden;
};
