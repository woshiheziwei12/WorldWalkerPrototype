#include "UI/WorldWalkerMotionPickerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

TSharedRef<SWidget> UWorldWalkerMotionPickerWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UWorldWalkerMotionPickerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	RefreshRows();
	SetKeyboardFocus();
}

void UWorldWalkerMotionPickerWidget::ConfigureActions(
	const TArray<FString>& InActionNames,
	const int32 InitialIndex)
{
	ActionNames = InActionNames;
	SelectedIndex = ActionNames.IsEmpty()
		? 0
		: FMath::Clamp(InitialIndex, 0, ActionNames.Num() - 1);
	RebuildActionRows();
	RefreshRows();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("WW_MAIN_MOTION_PICKER_CONFIGURED actions=%d rows=%d selected=%d"),
		ActionNames.Num(),
		ActionRows.Num(),
		SelectedIndex);
}

FReply UWorldWalkerMotionPickerWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Up || Key == EKeys::W)
	{
		MoveSelection(-1);
		return FReply::Handled();
	}
	if (Key == EKeys::Down || Key == EKeys::S)
	{
		MoveSelection(1);
		return FReply::Handled();
	}
	if (Key == EKeys::PageUp)
	{
		MoveSelection(-7);
		return FReply::Handled();
	}
	if (Key == EKeys::PageDown)
	{
		MoveSelection(7);
		return FReply::Handled();
	}
	if (Key == EKeys::Home)
	{
		SetSelection(0);
		return FReply::Handled();
	}
	if (Key == EKeys::End)
	{
		SetSelection(ActionNames.Num() - 1);
		return FReply::Handled();
	}
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
	{
		ConfirmSelection();
		return FReply::Handled();
	}
	if (Key == EKeys::Escape || Key == EKeys::T)
	{
		OnClosed.Broadcast();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UWorldWalkerMotionPickerWidget::BuildWidgetTree()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("MotionPickerRoot"));
	WidgetTree->RootWidget = Root;

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("MotionPickerSize"));
	PanelSize->SetWidthOverride(480.0f);
	PanelSize->SetHeightOverride(680.0f);
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(PanelSize);
	PanelSlot->SetAnchors(FAnchors(0.0f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.0f, 0.5f));
	PanelSlot->SetPosition(FVector2D(42.0f, 0.0f));
	// Canvas children keep their tiny default slot unless their size is explicit.
	// The title can paint outside that slot, but ScrollBox correctly clips to it,
	// which previously made all 28 action rows invisible.
	PanelSlot->SetAutoSize(false);
	PanelSlot->SetSize(FVector2D(480.0f, 680.0f));

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("MotionPickerPanel"));
	Panel->SetBrushColor(FLinearColor(0.018f, 0.030f, 0.055f, 0.96f));
	Panel->SetPadding(FMargin(24.0f, 20.0f));
	PanelSize->AddChild(Panel);

	UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("MotionPickerLayout"));
	Panel->SetContent(Layout);

	auto MakeText = [&](const TCHAR* Name, const FString& Value, const int32 Size)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), FName(Name));
		Text->SetText(FText::FromString(Value));
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.94f, 1.0f)));
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
		return Text;
	};

	UTextBlock* Title = MakeText(TEXT("MotionPickerTitle"), TEXT("荧 · 动作选择"), 27);
	Layout->AddChildToVerticalBox(Title)->SetPadding(FMargin(4.0f, 0.0f, 4.0f, 5.0f));
	UTextBlock* Hint = MakeText(
		TEXT("MotionPickerHint"),
		TEXT("↑↓ / W S 选择    Enter 播放    Esc / T 关闭"),
		14);
	Hint->SetColorAndOpacity(FSlateColor(FLinearColor(0.48f, 0.68f, 0.88f)));
	Layout->AddChildToVerticalBox(Hint)->SetPadding(FMargin(4.0f, 0.0f, 4.0f, 14.0f));

	ActionScroll = WidgetTree->ConstructWidget<UScrollBox>(
		UScrollBox::StaticClass(), TEXT("MotionActionScroll"));
	ActionScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
	Layout->AddChildToVerticalBox(ActionScroll)->SetSize(
		FSlateChildSize(ESlateSizeRule::Fill));
	RebuildActionRows();
}

void UWorldWalkerMotionPickerWidget::RebuildActionRows()
{
	ActionRows.Reset();
	if (!WidgetTree || !ActionScroll)
	{
		return;
	}
	ActionScroll->ClearChildren();
	for (int32 Index = 0; Index < ActionNames.Num(); ++Index)
	{
		const FString RowName = FString::Printf(TEXT("MotionRow_%02d"), Index);
		UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), FName(*RowName));
		Row->SetText(FText::FromString(ActionNames[Index]));
		FSlateFontInfo Font = Row->GetFont();
		Font.Size = 18;
		Row->SetFont(Font);
		Row->SetMargin(FMargin(9.0f, 7.0f));
		ActionScroll->AddChild(Row);
		ActionRows.Add(Row);
	}
	RefreshRows();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("WW_MAIN_MOTION_PICKER_ROWS_REBUILT actions=%d rows=%d complete=%d"),
		ActionNames.Num(),
		ActionRows.Num(),
		ActionRows.Num() == ActionNames.Num() ? 1 : 0);
}

void UWorldWalkerMotionPickerWidget::MoveSelection(const int32 Offset)
{
	if (ActionNames.IsEmpty())
	{
		return;
	}
	SetSelection((SelectedIndex + Offset % ActionNames.Num() + ActionNames.Num()) % ActionNames.Num());
}

void UWorldWalkerMotionPickerWidget::SetSelection(const int32 NewIndex)
{
	if (ActionNames.IsEmpty())
	{
		return;
	}
	SelectedIndex = FMath::Clamp(NewIndex, 0, ActionNames.Num() - 1);
	RefreshRows();
	UE_LOG(
		LogTemp,
		Verbose,
		TEXT("WW_MAIN_MOTION_PICKER_SELECTION index=%d action=%s"),
		SelectedIndex,
		*ActionNames[SelectedIndex]);
	if (ActionScroll && ActionRows.IsValidIndex(SelectedIndex))
	{
		ActionScroll->ScrollWidgetIntoView(ActionRows[SelectedIndex], true, EDescendantScrollDestination::Center);
	}
}

void UWorldWalkerMotionPickerWidget::RefreshRows()
{
	for (int32 Index = 0; Index < ActionRows.Num(); ++Index)
	{
		if (!ActionRows[Index] || !ActionNames.IsValidIndex(Index))
		{
			continue;
		}
		const bool bSelected = Index == SelectedIndex;
		ActionRows[Index]->SetText(FText::FromString(FString::Printf(
			TEXT("%s%02d  %s"),
			bSelected ? TEXT("▶ ") : TEXT("   "),
			Index + 1,
			*ActionNames[Index])));
		ActionRows[Index]->SetColorAndOpacity(FSlateColor(
			bSelected
				? FLinearColor(1.0f, 0.78f, 0.28f)
				: FLinearColor(0.78f, 0.86f, 0.96f)));
	}
}

void UWorldWalkerMotionPickerWidget::ConfirmSelection()
{
	if (ActionNames.IsValidIndex(SelectedIndex))
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("WW_MAIN_MOTION_PICKER_CONFIRM index=%d action=%s"),
			SelectedIndex,
			*ActionNames[SelectedIndex]);
		OnMotionPicked.Broadcast(SelectedIndex);
	}
}
