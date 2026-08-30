#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WorldWalkerMotionPickerWidget.generated.h"

class UBorder;
class UScrollBox;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnWorldWalkerMotionPicked, int32);
DECLARE_MULTICAST_DELEGATE(FOnWorldWalkerMotionPickerClosed);

/** Keyboard-driven action browser opened with T in the main world. */
UCLASS()
class WORLDWALKERPROTOTYPE_API UWorldWalkerMotionPickerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ConfigureActions(const TArray<FString>& InActionNames, int32 InitialIndex);

	FOnWorldWalkerMotionPicked OnMotionPicked;
	FOnWorldWalkerMotionPickerClosed OnClosed;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildWidgetTree();
	void RebuildActionRows();
	void MoveSelection(int32 Offset);
	void SetSelection(int32 NewIndex);
	void RefreshRows();
	void ConfirmSelection();

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> ActionScroll;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> ActionRows;

	TArray<FString> ActionNames;
	int32 SelectedIndex = 0;
};
