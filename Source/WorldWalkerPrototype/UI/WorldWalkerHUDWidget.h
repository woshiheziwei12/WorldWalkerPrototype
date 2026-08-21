#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WorldWalkerHUDWidget.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UImage;
class UProgressBar;
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
	void SetPlatformingStatus(float CurrentStamina, float MaxStamina, const FString& StateText);
	void HidePlatformingStatus();
	void ShowJourneyMessage(const FString& Title, const FString& Body, const FString& Prompt);
	void HideJourneyMessage();
	void ShowCombat(int32 PlayerHealth, int32 PlayerMaxHealth, int32 EnemyHealth, int32 EnemyMaxHealth);
	void RefreshCombatState(
		int32 PlayerHealth,
		int32 PlayerMaxHealth,
		int32 EnemyHealth,
		int32 EnemyMaxHealth,
		int32 CurrentEnergy,
		int32 MaxEnergy,
		int32 CurrentBlock,
		int32 CurrentValor,
		int32 MaxValor,
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

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();
	UTextBlock* MakeText(const TCHAR* Name, const FString& Text, int32 FontSize);

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

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ExplorationPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ExplorationText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> PlatformingStatusPanel;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> PlatformingStaminaBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PlatformingStatusText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> JourneyPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> JourneyTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> JourneyBodyText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> JourneyPromptText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> CombatPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PlayerHealthText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EnemyHealthText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EnergyText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BlockText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ValorText;

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

	TArray<bool> CachedCardPlayable;
	bool bPlayerCanAct = false;
};
