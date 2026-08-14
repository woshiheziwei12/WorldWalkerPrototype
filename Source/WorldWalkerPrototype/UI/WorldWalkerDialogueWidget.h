#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Dialogue/FantasyDialogueTypes.h"
#include "WorldWalkerDialogueWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;
class UVerticalBox;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnFantasyDialogueChoiceSelected, int32);
DECLARE_MULTICAST_DELEGATE(FOnFantasyDialogueClosed);

/** Small screen-space prompt used by AFantasyNPC's WidgetComponent. */
UCLASS()
class WORLDWALKERPROTOTYPE_API UWorldWalkerNPCPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPrompt(const FText& SpeakerName, const FText& ActionText = FText::GetEmpty());

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildWidgetTree();
	void RefreshPrompt();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PromptText;

	FText CachedSpeakerName;
	FText CachedActionText;
};

/** Native UMG conversation panel. It owns presentation only; dialogue never mutates combat state. */
UCLASS()
class WORLDWALKERPROTOTYPE_API UWorldWalkerDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ConfigureDialogue(const FFantasyDialogueScript& InDialogue);

	FOnFantasyDialogueChoiceSelected OnChoiceSelected;
	FOnFantasyDialogueClosed OnDialogueClosed;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildWidgetTree();
	void RefreshOpening();
	void ShowChoiceReply(int32 ChoiceIndex);
	void RequestClose();
	UTextBlock* MakeText(const TCHAR* Name, const FString& Text, int32 FontSize) const;

	UFUNCTION()
	void HandleChoice0Clicked();

	UFUNCTION()
	void HandleChoice1Clicked();

	UFUNCTION()
	void HandleChoice2Clicked();

	UFUNCTION()
	void HandleContinueClicked();

	UFUNCTION()
	void HandleEndClicked();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> DialoguePanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SpeakerNameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BodyText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> ChoiceButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> ChoiceLabels;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ContinueButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> EndButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EndButtonLabel;

	FFantasyDialogueScript Dialogue;
	bool bShowingFarewell = false;
	bool bCloseRequested = false;
};
