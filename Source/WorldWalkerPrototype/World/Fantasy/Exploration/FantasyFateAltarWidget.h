#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "World/Fantasy/Exploration/FantasyFateAltar.h"
#include "FantasyFateAltarWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;
class UVerticalBox;

DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnFantasyFateAltarChoiceSelected,
	EFantasyFateAltarChoice);
DECLARE_MULTICAST_DELEGATE(FOnFantasyFateAltarClosed);

/** Native Chinese UMG panel for the W01 Ashen Fate Altar exploration event. */
UCLASS()
class WORLDWALKERPROTOTYPE_API UFantasyFateAltarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ConfigureEvent(const FText& InTitle, const FText& InLore);
	void ShowResolution(EFantasyFateAltarChoice Choice, const FString& ResolutionText);

	FOnFantasyFateAltarChoiceSelected OnChoiceSelected;
	FOnFantasyFateAltarClosed OnClosed;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildWidgetTree();
	void RefreshOpening();
	void SelectChoice(EFantasyFateAltarChoice Choice);
	void RequestClose();
	UTextBlock* MakeText(const TCHAR* Name, const FString& Text, int32 FontSize) const;
	UButton* AddChoiceButton(
		UVerticalBox* Parent,
		const TCHAR* ButtonName,
		const TCHAR* LabelName,
		const FString& Text,
		const FLinearColor& Tint);

	UFUNCTION()
	void HandleBloodOathClicked();

	UFUNCTION()
	void HandleAshenGraceClicked();

	UFUNCTION()
	void HandleRuneInsightClicked();

	UFUNCTION()
	void HandleLeaveClicked();

	UFUNCTION()
	void HandleConfirmClicked();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> EventPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LoreText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> ChoiceButtons;

	UPROPERTY(Transient)
	TObjectPtr<UButton> LeaveButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ConfirmButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResultHeadingText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResultBodyText;

	FText CachedTitle;
	FText CachedLore;
	bool bResolved = false;
	bool bCloseRequested = false;
	bool bChoiceSubmitted = false;
};
