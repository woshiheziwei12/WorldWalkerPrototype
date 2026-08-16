#include "UI/WorldWalkerDialogueWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

TSharedRef<SWidget> UWorldWalkerNPCPromptWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UWorldWalkerNPCPromptWidget::SetPrompt(const FText& SpeakerName, const FText& ActionText)
{
	CachedSpeakerName = SpeakerName;
	CachedActionText = ActionText;
	RefreshPrompt();
}

void UWorldWalkerNPCPromptWidget::BuildWidgetTree()
{
	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PromptBorder"));
	RootBorder->SetBrushColor(FLinearColor(0.025f, 0.018f, 0.028f, 0.88f));
	RootBorder->SetPadding(FMargin(14.0f, 7.0f));
	WidgetTree->RootWidget = RootBorder;

	PromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PromptText"));
	PromptText->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.83f, 0.58f, 1.0f)));
	PromptText->SetJustification(ETextJustify::Center);
	PromptText->SetAutoWrapText(true);
	PromptText->SetMinDesiredWidth(320.0f);
	PromptText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	FSlateFontInfo Font = PromptText->GetFont();
	Font.Size = 17;
	PromptText->SetFont(Font);
	RootBorder->SetContent(PromptText);
	RefreshPrompt();
}

void UWorldWalkerNPCPromptWidget::RefreshPrompt()
{
	if (!PromptText)
	{
		return;
	}

	const FText Name = CachedSpeakerName.IsEmpty()
		? FText::FromString(TEXT("陌生旅人"))
		: CachedSpeakerName;
	const FText Action = CachedActionText.IsEmpty()
		? FText::FromString(TEXT("按 E 交谈"))
		: CachedActionText;
	PromptText->SetText(FText::Format(
		FText::FromString(TEXT("{0}\n{1}")),
		Name,
		Action));
}

TSharedRef<SWidget> UWorldWalkerDialogueWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UWorldWalkerDialogueWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	RefreshOpening();
	SetKeyboardFocus();
}

FReply UWorldWalkerDialogueWidget::NativeOnKeyDown(
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

void UWorldWalkerDialogueWidget::ConfigureDialogue(const FFantasyDialogueScript& InDialogue)
{
	Dialogue = InDialogue;
	bShowingFarewell = false;
	bCloseRequested = false;
	SetIsFocusable(true);
	RefreshOpening();
}

UTextBlock* UWorldWalkerDialogueWidget::MakeText(
	const TCHAR* Name,
	const FString& Text,
	const int32 FontSize) const
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(Name));
	TextBlock->SetText(FText::FromString(Text));
	TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.93f, 0.86f, 0.70f, 1.0f)));
	TextBlock->SetAutoWrapText(true);
	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	TextBlock->SetFont(Font);
	return TextBlock;
}

void UWorldWalkerDialogueWidget::BuildWidgetTree()
{
	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DialogueRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* ScreenShade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ScreenShade"));
	ScreenShade->SetBrushColor(FLinearColor(0.005f, 0.004f, 0.008f, 0.56f));
	UCanvasPanelSlot* ShadeSlot = RootCanvas->AddChildToCanvas(ScreenShade);
	ShadeSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	ShadeSlot->SetOffsets(FMargin(0.0f));

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DialoguePanelSize"));
	PanelSize->SetWidthOverride(900.0f);
	PanelSize->SetMinDesiredHeight(390.0f);

	DialoguePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DialoguePanel"));
	DialoguePanel->SetBrushColor(FLinearColor(0.030f, 0.018f, 0.025f, 0.985f));
	DialoguePanel->SetPadding(FMargin(34.0f, 25.0f));
	PanelSize->AddChild(DialoguePanel);

	UVerticalBox* DialogueBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DialogueBox"));
	DialoguePanel->SetContent(DialogueBox);

	SpeakerNameText = MakeText(TEXT("SpeakerName"), TEXT("旅人"), 29);
	SpeakerNameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.83f, 0.60f, 0.22f, 1.0f)));
	DialogueBox->AddChildToVerticalBox(SpeakerNameText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));

	BodyText = MakeText(TEXT("DialogueBody"), TEXT("……"), 22);
	BodyText->SetJustification(ETextJustify::Left);
	BodyText->SetMinDesiredWidth(810.0f);
	UVerticalBoxSlot* BodySlot = DialogueBox->AddChildToVerticalBox(BodyText);
	BodySlot->SetPadding(FMargin(8.0f, 4.0f, 8.0f, 17.0f));

	for (int32 ChoiceIndex = 0; ChoiceIndex < 3; ++ChoiceIndex)
	{
		const FName ButtonName(*FString::Printf(TEXT("DialogueChoice_%d"), ChoiceIndex));
		const FString LabelName = FString::Printf(TEXT("DialogueChoiceLabel_%d"), ChoiceIndex);
		UButton* ChoiceButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		ChoiceButton->SetBackgroundColor(FLinearColor(0.16f, 0.10f, 0.055f, 1.0f));
		UTextBlock* ChoiceLabel = MakeText(*LabelName, TEXT("询问……"), 19);
		ChoiceLabel->SetJustification(ETextJustify::Left);
		ChoiceButton->AddChild(ChoiceLabel);
		DialogueBox->AddChildToVerticalBox(ChoiceButton)->SetPadding(FMargin(4.0f, 4.0f));

		switch (ChoiceIndex)
		{
		case 0: ChoiceButton->OnClicked.AddDynamic(this, &UWorldWalkerDialogueWidget::HandleChoice0Clicked); break;
		case 1: ChoiceButton->OnClicked.AddDynamic(this, &UWorldWalkerDialogueWidget::HandleChoice1Clicked); break;
		case 2: ChoiceButton->OnClicked.AddDynamic(this, &UWorldWalkerDialogueWidget::HandleChoice2Clicked); break;
		default: break;
		}

		ChoiceButtons.Add(ChoiceButton);
		ChoiceLabels.Add(ChoiceLabel);
	}

	ContinueButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ContinueDialogueButton"));
	ContinueButton->SetBackgroundColor(FLinearColor(0.13f, 0.10f, 0.18f, 1.0f));
	UTextBlock* ContinueLabel = MakeText(TEXT("ContinueDialogueLabel"), TEXT("继续询问"), 19);
	ContinueLabel->SetJustification(ETextJustify::Center);
	ContinueButton->AddChild(ContinueLabel);
	ContinueButton->OnClicked.AddDynamic(this, &UWorldWalkerDialogueWidget::HandleContinueClicked);
	DialogueBox->AddChildToVerticalBox(ContinueButton)->SetPadding(FMargin(220.0f, 7.0f));

	EndButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EndDialogueButton"));
	EndButton->SetBackgroundColor(FLinearColor(0.34f, 0.12f, 0.075f, 1.0f));
	EndButtonLabel = MakeText(TEXT("EndDialogueLabel"), TEXT("结束交谈"), 19);
	EndButtonLabel->SetJustification(ETextJustify::Center);
	EndButton->AddChild(EndButtonLabel);
	EndButton->OnClicked.AddDynamic(this, &UWorldWalkerDialogueWidget::HandleEndClicked);
	DialogueBox->AddChildToVerticalBox(EndButton)->SetPadding(FMargin(220.0f, 7.0f, 220.0f, 0.0f));

	UTextBlock* EscapeHint = MakeText(TEXT("DialogueEscapeHint"), TEXT("Esc 直接离开"), 14);
	EscapeHint->SetJustification(ETextJustify::Right);
	EscapeHint->SetColorAndOpacity(FSlateColor(FLinearColor(0.56f, 0.50f, 0.43f, 1.0f)));
	DialogueBox->AddChildToVerticalBox(EscapeHint)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));

	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelSize);
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.82f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 1.0f));
	PanelSlot->SetAutoSize(true);
}

void UWorldWalkerDialogueWidget::RefreshOpening()
{
	if (!SpeakerNameText || !BodyText || !DialoguePanel)
	{
		return;
	}

	SpeakerNameText->SetText(Dialogue.SpeakerName);
	SpeakerNameText->SetColorAndOpacity(FSlateColor(Dialogue.AccentColor));
	BodyText->SetText(Dialogue.OpeningText);
	bShowingFarewell = false;

	for (int32 Index = 0; Index < ChoiceButtons.Num(); ++Index)
	{
		const bool bHasChoice = Dialogue.Choices.IsValidIndex(Index);
		ChoiceButtons[Index]->SetVisibility(bHasChoice ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bHasChoice)
		{
			ChoiceLabels[Index]->SetText(Dialogue.Choices[Index].ChoiceText);
		}
	}

	ContinueButton->SetVisibility(ESlateVisibility::Collapsed);
	EndButtonLabel->SetText(FText::FromString(TEXT("结束交谈")));
}

void UWorldWalkerDialogueWidget::ShowChoiceReply(const int32 ChoiceIndex)
{
	if (!Dialogue.Choices.IsValidIndex(ChoiceIndex) || bCloseRequested)
	{
		return;
	}

	BodyText->SetText(Dialogue.Choices[ChoiceIndex].ReplyText);
	for (UButton* ChoiceButton : ChoiceButtons)
	{
		ChoiceButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	ContinueButton->SetVisibility(ESlateVisibility::Visible);
	OnChoiceSelected.Broadcast(ChoiceIndex);
}

void UWorldWalkerDialogueWidget::RequestClose()
{
	if (bCloseRequested)
	{
		return;
	}
	bCloseRequested = true;
	OnDialogueClosed.Broadcast();
}

void UWorldWalkerDialogueWidget::HandleChoice0Clicked() { ShowChoiceReply(0); }
void UWorldWalkerDialogueWidget::HandleChoice1Clicked() { ShowChoiceReply(1); }
void UWorldWalkerDialogueWidget::HandleChoice2Clicked() { ShowChoiceReply(2); }

void UWorldWalkerDialogueWidget::HandleContinueClicked()
{
	RefreshOpening();
}

void UWorldWalkerDialogueWidget::HandleEndClicked()
{
	if (bShowingFarewell)
	{
		RequestClose();
		return;
	}

	bShowingFarewell = true;
	BodyText->SetText(Dialogue.FarewellText);
	for (UButton* ChoiceButton : ChoiceButtons)
	{
		ChoiceButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	ContinueButton->SetVisibility(ESlateVisibility::Collapsed);
	EndButtonLabel->SetText(FText::FromString(TEXT("离开")));
}
