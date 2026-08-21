#include "World/Fantasy/Exploration/FantasyFateAltarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

TSharedRef<SWidget> UFantasyFateAltarWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UFantasyFateAltarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	RefreshOpening();
	SetKeyboardFocus();
}

FReply UFantasyFateAltarWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		RequestClose();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UFantasyFateAltarWidget::ConfigureEvent(
	const FText& InTitle,
	const FText& InLore)
{
	CachedTitle = InTitle;
	CachedLore = InLore;
	bResolved = false;
	bCloseRequested = false;
	bChoiceSubmitted = false;
	SetIsFocusable(true);
	RefreshOpening();
}

void UFantasyFateAltarWidget::ShowResolution(
	const EFantasyFateAltarChoice Choice,
	const FString& ResolutionText)
{
	bResolved = true;
	bChoiceSubmitted = true;

	for (UButton* ChoiceButton : ChoiceButtons)
	{
		if (ChoiceButton)
		{
			ChoiceButton->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (LeaveButton)
	{
		LeaveButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	FString ResultHeading = TEXT("命运已经刻下");
	FLinearColor PanelTint(0.035f, 0.025f, 0.045f, 0.99f);
	switch (Choice)
	{
	case EFantasyFateAltarChoice::BloodOath:
		ResultHeading = TEXT("铁与血接受了你");
		PanelTint = FLinearColor(0.075f, 0.018f, 0.022f, 0.99f);
		break;
	case EFantasyFateAltarChoice::AshenGrace:
		ResultHeading = TEXT("余烬为你重新燃起");
		PanelTint = FLinearColor(0.070f, 0.045f, 0.012f, 0.99f);
		break;
	case EFantasyFateAltarChoice::RuneInsight:
		ResultHeading = TEXT("秘仪揭开了一线未来");
		PanelTint = FLinearColor(0.025f, 0.035f, 0.085f, 0.99f);
		break;
	default:
		break;
	}

	if (EventPanel)
	{
		EventPanel->SetBrushColor(PanelTint);
	}
	if (ResultHeadingText)
	{
		ResultHeadingText->SetText(FText::FromString(ResultHeading));
		ResultHeadingText->SetVisibility(ESlateVisibility::Visible);
	}
	if (ResultBodyText)
	{
		ResultBodyText->SetText(FText::FromString(ResolutionText));
		ResultBodyText->SetVisibility(ESlateVisibility::Visible);
	}
	if (ConfirmButton)
	{
		ConfirmButton->SetVisibility(ESlateVisibility::Visible);
		ConfirmButton->SetKeyboardFocus();
	}
}

UTextBlock* UFantasyFateAltarWidget::MakeText(
	const TCHAR* Name,
	const FString& Text,
	const int32 FontSize) const
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(Name));
	TextBlock->SetText(FText::FromString(Text));
	TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.87f, 0.72f, 1.0f)));
	TextBlock->SetAutoWrapText(true);
	TextBlock->SetShadowOffset(FVector2D(1.0f, 1.0f));
	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	TextBlock->SetFont(Font);
	return TextBlock;
}

UButton* UFantasyFateAltarWidget::AddChoiceButton(
	UVerticalBox* Parent,
	const TCHAR* ButtonName,
	const TCHAR* LabelName,
	const FString& Text,
	const FLinearColor& Tint)
{
	UButton* ChoiceButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(ButtonName));
	ChoiceButton->SetBackgroundColor(Tint);
	UTextBlock* ChoiceLabel = MakeText(LabelName, Text, 19);
	ChoiceLabel->SetJustification(ETextJustify::Left);
	ChoiceLabel->SetMargin(FMargin(5.0f));
	ChoiceButton->AddChild(ChoiceLabel);
	Parent->AddChildToVerticalBox(ChoiceButton)->SetPadding(FMargin(6.0f, 5.0f));
	ChoiceButtons.Add(ChoiceButton);
	return ChoiceButton;
}

void UFantasyFateAltarWidget::BuildWidgetTree()
{
	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(),
		TEXT("FateAltarRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* ScreenShade = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("FateAltarScreenShade"));
	ScreenShade->SetBrushColor(FLinearColor(0.002f, 0.003f, 0.008f, 0.69f));
	UCanvasPanelSlot* ShadeSlot = RootCanvas->AddChildToCanvas(ScreenShade);
	ShadeSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	ShadeSlot->SetOffsets(FMargin(0.0f));

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(),
		TEXT("FateAltarPanelSize"));
	PanelSize->SetWidthOverride(920.0f);
	PanelSize->SetMinDesiredHeight(520.0f);

	EventPanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("FateAltarPanel"));
	EventPanel->SetBrushColor(FLinearColor(0.030f, 0.022f, 0.040f, 0.99f));
	EventPanel->SetPadding(FMargin(38.0f, 28.0f));
	PanelSize->AddChild(EventPanel);

	UVerticalBox* EventBox = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("FateAltarEventBox"));
	EventPanel->SetContent(EventBox);

	UTextBlock* EventKind = MakeText(TEXT("EventKind"), TEXT("探索事件 · 命运只能刻下一次"), 15);
	EventKind->SetColorAndOpacity(FSlateColor(FLinearColor(0.58f, 0.50f, 0.38f, 1.0f)));
	EventKind->SetJustification(ETextJustify::Center);
	EventBox->AddChildToVerticalBox(EventKind)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

	TitleText = MakeText(TEXT("FateAltarTitle"), TEXT("灰烬命运碑"), 32);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.64f, 0.22f, 1.0f)));
	TitleText->SetJustification(ETextJustify::Center);
	EventBox->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 13.0f));

	LoreText = MakeText(TEXT("FateAltarLore"), TEXT("……"), 20);
	LoreText->SetJustification(ETextJustify::Left);
	LoreText->SetMinDesiredWidth(820.0f);
	EventBox->AddChildToVerticalBox(LoreText)->SetPadding(FMargin(12.0f, 2.0f, 12.0f, 17.0f));

	UButton* BloodButton = AddChoiceButton(
		EventBox,
		TEXT("BloodOathButton"),
		TEXT("BloodOathLabel"),
		TEXT("血之铁誓\n立刻失去至多 12 点生命；下场战斗开始时获得 2 点英勇"),
		FLinearColor(0.35f, 0.075f, 0.065f, 1.0f));
	BloodButton->OnClicked.AddDynamic(this, &UFantasyFateAltarWidget::HandleBloodOathClicked);

	UButton* GraceButton = AddChoiceButton(
		EventBox,
		TEXT("AshenGraceButton"),
		TEXT("AshenGraceLabel"),
		TEXT("圣烬守护\n立刻恢复至多 30 点生命；下场战斗开始时获得 10 点格挡"),
		FLinearColor(0.34f, 0.22f, 0.060f, 1.0f));
	GraceButton->OnClicked.AddDynamic(this, &UFantasyFateAltarWidget::HandleAshenGraceClicked);

	UButton* InsightButton = AddChoiceButton(
		EventBox,
		TEXT("RuneInsightButton"),
		TEXT("RuneInsightLabel"),
		TEXT("秘仪洞察\n下场战斗开始时获得 1 点英勇与 6 点格挡"),
		FLinearColor(0.075f, 0.15f, 0.38f, 1.0f));
	InsightButton->OnClicked.AddDynamic(this, &UFantasyFateAltarWidget::HandleRuneInsightClicked);

	ResultHeadingText = MakeText(TEXT("FateResultHeading"), TEXT("命运已经刻下"), 27);
	ResultHeadingText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.72f, 0.28f, 1.0f)));
	ResultHeadingText->SetJustification(ETextJustify::Center);
	ResultHeadingText->SetVisibility(ESlateVisibility::Collapsed);
	EventBox->AddChildToVerticalBox(ResultHeadingText)->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 10.0f));

	ResultBodyText = MakeText(TEXT("FateResultBody"), TEXT("……"), 21);
	ResultBodyText->SetJustification(ETextJustify::Center);
	ResultBodyText->SetVisibility(ESlateVisibility::Collapsed);
	EventBox->AddChildToVerticalBox(ResultBodyText)->SetPadding(FMargin(35.0f, 6.0f, 35.0f, 18.0f));

	ConfirmButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("FateConfirmButton"));
	ConfirmButton->SetBackgroundColor(FLinearColor(0.42f, 0.25f, 0.065f, 1.0f));
	UTextBlock* ConfirmLabel = MakeText(TEXT("FateConfirmLabel"), TEXT("接受命运"), 20);
	ConfirmLabel->SetJustification(ETextJustify::Center);
	ConfirmButton->AddChild(ConfirmLabel);
	ConfirmButton->OnClicked.AddDynamic(this, &UFantasyFateAltarWidget::HandleConfirmClicked);
	ConfirmButton->SetVisibility(ESlateVisibility::Collapsed);
	EventBox->AddChildToVerticalBox(ConfirmButton)->SetPadding(FMargin(260.0f, 7.0f));

	LeaveButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("FateLeaveButton"));
	LeaveButton->SetBackgroundColor(FLinearColor(0.10f, 0.085f, 0.12f, 1.0f));
	UTextBlock* LeaveLabel = MakeText(TEXT("FateLeaveLabel"), TEXT("暂不触碰"), 18);
	LeaveLabel->SetJustification(ETextJustify::Center);
	LeaveButton->AddChild(LeaveLabel);
	LeaveButton->OnClicked.AddDynamic(this, &UFantasyFateAltarWidget::HandleLeaveClicked);
	EventBox->AddChildToVerticalBox(LeaveButton)->SetPadding(FMargin(285.0f, 7.0f));

	UTextBlock* EscapeHint = MakeText(TEXT("FateEscapeHint"), TEXT("Esc 离开石碑"), 14);
	EscapeHint->SetJustification(ETextJustify::Right);
	EscapeHint->SetColorAndOpacity(FSlateColor(FLinearColor(0.50f, 0.45f, 0.42f, 1.0f)));
	EventBox->AddChildToVerticalBox(EscapeHint)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));

	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelSize);
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.52f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetAutoSize(true);
}

void UFantasyFateAltarWidget::RefreshOpening()
{
	if (!TitleText || !LoreText || !EventPanel || bResolved)
	{
		return;
	}

	TitleText->SetText(CachedTitle.IsEmpty()
		? FText::FromString(TEXT("灰烬命运碑"))
		: CachedTitle);
	LoreText->SetText(CachedLore.IsEmpty()
		? FText::FromString(TEXT("三枚古老符文等待你刻下唯一的誓言。"))
		: CachedLore);
	EventPanel->SetBrushColor(FLinearColor(0.030f, 0.022f, 0.040f, 0.99f));

	for (UButton* ChoiceButton : ChoiceButtons)
	{
		if (ChoiceButton)
		{
			ChoiceButton->SetVisibility(ESlateVisibility::Visible);
			ChoiceButton->SetIsEnabled(!bChoiceSubmitted);
		}
	}
	if (LeaveButton)
	{
		LeaveButton->SetVisibility(ESlateVisibility::Visible);
	}
	if (ConfirmButton)
	{
		ConfirmButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ResultHeadingText)
	{
		ResultHeadingText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ResultBodyText)
	{
		ResultBodyText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFantasyFateAltarWidget::SelectChoice(const EFantasyFateAltarChoice Choice)
{
	if (bChoiceSubmitted || bCloseRequested || Choice == EFantasyFateAltarChoice::None)
	{
		return;
	}

	bChoiceSubmitted = true;
	for (UButton* ChoiceButton : ChoiceButtons)
	{
		if (ChoiceButton)
		{
			ChoiceButton->SetIsEnabled(false);
		}
	}
	OnChoiceSelected.Broadcast(Choice);
}

void UFantasyFateAltarWidget::RequestClose()
{
	if (bCloseRequested)
	{
		return;
	}
	bCloseRequested = true;
	OnClosed.Broadcast();
}

void UFantasyFateAltarWidget::HandleBloodOathClicked()
{
	SelectChoice(EFantasyFateAltarChoice::BloodOath);
}

void UFantasyFateAltarWidget::HandleAshenGraceClicked()
{
	SelectChoice(EFantasyFateAltarChoice::AshenGrace);
}

void UFantasyFateAltarWidget::HandleRuneInsightClicked()
{
	SelectChoice(EFantasyFateAltarChoice::RuneInsight);
}

void UFantasyFateAltarWidget::HandleLeaveClicked()
{
	RequestClose();
}

void UFantasyFateAltarWidget::HandleConfirmClicked()
{
	RequestClose();
}
