#include "UI/WorldWalkerHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "WorldWalkerGameModeBase.h"

TSharedRef<SWidget> UWorldWalkerHUDWidget::RebuildWidget()
{
	// Native-only widgets must create their UObject tree before Super builds
	// the matching Slate hierarchy. NativeConstruct runs after that point.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}

	return Super::RebuildWidget();
}

void UWorldWalkerHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	ShowExploration();
	UE_LOG(LogTemp, Display, TEXT("WorldWalker HUD constructed and added to the viewport."));
}

UTextBlock* UWorldWalkerHUDWidget::MakeText(
	const TCHAR* Name,
	const FString& Text,
	const int32 FontSize)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(Name));
	TextBlock->SetText(FText::FromString(Text));
	TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TextBlock->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	TextBlock->SetFont(Font);
	return TextBlock;
}

void UWorldWalkerHUDWidget::BuildWidgetTree()
{
	const FLinearColor NearBlack(0.012f, 0.009f, 0.015f, 0.97f);
	const FLinearColor DeepWine(0.075f, 0.018f, 0.025f, 0.96f);
	const FLinearColor AntiqueGold(0.62f, 0.42f, 0.12f, 1.0f);
	const FLinearColor Parchment(0.91f, 0.82f, 0.61f, 1.0f);
	const FLinearColor MutedParchment(0.68f, 0.61f, 0.48f, 1.0f);

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	ExplorationPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ExplorationPanel"));
	ExplorationPanel->SetBrushColor(FLinearColor(0.018f, 0.012f, 0.02f, 0.88f));
	ExplorationPanel->SetPadding(FMargin(22.0f, 12.0f));
	ExplorationText = MakeText(
		TEXT("ExplorationText"),
		TEXT("WASD 移动 | 鼠标观察 | 空格跳跃 | E 交互"),
		20);
	ExplorationText->SetColorAndOpacity(FSlateColor(Parchment));
	ExplorationPanel->SetContent(ExplorationText);

	UCanvasPanelSlot* ExplorationSlot = RootCanvas->AddChildToCanvas(ExplorationPanel);
	ExplorationSlot->SetAnchors(FAnchors(0.5f, 0.03f));
	ExplorationSlot->SetAlignment(FVector2D(0.5f, 0.0f));
	ExplorationSlot->SetAutoSize(true);

	ChoicePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ChoicePanel"));
	ChoicePanel->SetBrushColor(FLinearColor(0.006f, 0.005f, 0.010f, 0.91f));
	ChoicePanel->SetHorizontalAlignment(HAlign_Center);
	ChoicePanel->SetVerticalAlignment(VAlign_Center);

	USizeBox* ChoiceSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ChoiceSize"));
	ChoiceSize->SetWidthOverride(860.0f);
	ChoicePanel->SetContent(ChoiceSize);

	UBorder* ChoiceCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ChoiceCard"));
	ChoiceCard->SetBrushColor(FLinearColor(0.055f, 0.020f, 0.028f, 0.985f));
	ChoiceCard->SetPadding(FMargin(42.0f, 32.0f));
	ChoiceSize->AddChild(ChoiceCard);

	UVerticalBox* ChoiceBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ChoiceBox"));
	ChoiceCard->SetContent(ChoiceBox);

	ChoiceTitleText = MakeText(TEXT("ChoiceTitle"), TEXT("月圆之路"), 31);
	ChoiceTitleText->SetColorAndOpacity(FSlateColor(AntiqueGold));
	ChoiceBox->AddChildToVerticalBox(ChoiceTitleText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 9.0f));

	ChoiceSummaryText = MakeText(TEXT("ChoiceSummary"), TEXT("灰烬章节 1/4"), 17);
	ChoiceSummaryText->SetColorAndOpacity(FSlateColor(MutedParchment));
	ChoiceSummaryText->SetAutoWrapText(true);
	ChoiceBox->AddChildToVerticalBox(ChoiceSummaryText)->SetPadding(FMargin(18.0f, 0.0f, 18.0f, 8.0f));

	ChoiceLoreText = MakeText(
		TEXT("ChoiceLore"),
		TEXT("月光将道路分成数条命运。你的选择会改变本次旅途。"),
		19);
	ChoiceLoreText->SetColorAndOpacity(FSlateColor(Parchment));
	ChoiceLoreText->SetAutoWrapText(true);
	ChoiceBox->AddChildToVerticalBox(ChoiceLoreText)->SetPadding(FMargin(18.0f, 4.0f, 18.0f, 16.0f));

	for (int32 ChoiceIndex = 0; ChoiceIndex < 3; ++ChoiceIndex)
	{
		const FName ButtonName(*FString::Printf(TEXT("ChapterChoiceButton_%d"), ChoiceIndex));
		const FString LabelName = FString::Printf(TEXT("ChapterChoiceLabel_%d"), ChoiceIndex);
		UButton* ChoiceButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		ChoiceButton->SetBackgroundColor(FLinearColor(0.27f, 0.16f, 0.055f, 1.0f));
		UTextBlock* ChoiceLabel = MakeText(*LabelName, TEXT("未显现的命运"), 18);
		ChoiceLabel->SetColorAndOpacity(FSlateColor(Parchment));
		ChoiceLabel->SetAutoWrapText(true);
		ChoiceButton->AddChild(ChoiceLabel);

		switch (ChoiceIndex)
		{
		case 0: ChoiceButton->OnClicked.AddDynamic(this, &UWorldWalkerHUDWidget::HandleChoice0Clicked); break;
		case 1: ChoiceButton->OnClicked.AddDynamic(this, &UWorldWalkerHUDWidget::HandleChoice1Clicked); break;
		case 2: ChoiceButton->OnClicked.AddDynamic(this, &UWorldWalkerHUDWidget::HandleChoice2Clicked); break;
		default: break;
		}

		ChoiceBox->AddChildToVerticalBox(ChoiceButton)->SetPadding(FMargin(28.0f, 6.0f));
		ChoiceButtons.Add(ChoiceButton);
		ChoiceButtonLabels.Add(ChoiceLabel);
	}

	ChoiceContinueButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("ChoiceContinueButton"));
	ChoiceContinueButton->SetBackgroundColor(FLinearColor(0.42f, 0.10f, 0.075f, 1.0f));
	ChoiceContinueLabel = MakeText(TEXT("ChoiceContinueLabel"), TEXT("继续旅途"), 20);
	ChoiceContinueLabel->SetColorAndOpacity(FSlateColor(Parchment));
	ChoiceContinueButton->AddChild(ChoiceContinueLabel);
	ChoiceContinueButton->OnClicked.AddDynamic(this, &UWorldWalkerHUDWidget::HandleChoiceContinueClicked);
	ChoiceBox->AddChildToVerticalBox(ChoiceContinueButton)->SetPadding(FMargin(210.0f, 14.0f, 210.0f, 0.0f));

	UCanvasPanelSlot* ChoiceSlot = RootCanvas->AddChildToCanvas(ChoicePanel);
	ChoiceSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	ChoiceSlot->SetOffsets(FMargin(0.0f));

	CombatPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CombatPanel"));
	CombatPanel->SetBrushColor(NearBlack);
	CombatPanel->SetPadding(FMargin(24.0f, 16.0f));

	UVerticalBox* CombatBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CombatBox"));
	CombatPanel->SetContent(CombatBox);

	UTextBlock* Title = MakeText(TEXT("CombatTitle"), TEXT("月圆旅途 · 卡牌战斗"), 27);
	Title->SetColorAndOpacity(FSlateColor(AntiqueGold));
	CombatBox->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 7.0f));

	UHorizontalBox* HealthRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HealthRow"));
	CombatBox->AddChildToVerticalBox(HealthRow)->SetHorizontalAlignment(HAlign_Center);

	UVerticalBox* PlayerSummary = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("PlayerSummary"));
	PlayerHealthText = MakeText(TEXT("PlayerHealth"), TEXT("红斗篷骑士 100 / 100"), 20);
	PlayerHealthText->SetColorAndOpacity(FSlateColor(Parchment));
	PlayerSummary->AddChildToVerticalBox(PlayerHealthText);
	PlayerHealthBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(), TEXT("PlayerHealthBar"));
	PlayerHealthBar->SetPercent(1.0f);
	PlayerHealthBar->SetFillColorAndOpacity(FLinearColor(0.18f, 0.72f, 0.42f, 1.0f));
	PlayerSummary->AddChildToVerticalBox(PlayerHealthBar)->SetPadding(FMargin(12.0f, 4.0f));
	HealthRow->AddChildToHorizontalBox(PlayerSummary)->SetPadding(FMargin(20.0f, 8.0f));

	UTextBlock* VersusLabel = MakeText(TEXT("VersusLabel"), TEXT("VS"), 24);
	VersusLabel->SetColorAndOpacity(FSlateColor(AntiqueGold));
	HealthRow->AddChildToHorizontalBox(VersusLabel)->SetPadding(FMargin(28.0f, 34.0f));

	USizeBox* PortraitSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("EnemyPortraitSize"));
	PortraitSize->SetWidthOverride(112.0f);
	PortraitSize->SetHeightOverride(112.0f);
	UBorder* PortraitFrame = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("EnemyPortraitFrame"));
	PortraitFrame->SetBrushColor(FLinearColor(0.36f, 0.075f, 0.06f, 1.0f));
	PortraitFrame->SetPadding(FMargin(4.0f));
	EnemyPortraitImage = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), TEXT("EnemyPortraitImage"));
	EnemyPortraitImage->SetColorAndOpacity(FLinearColor(0.28f, 0.08f, 0.07f, 1.0f));
	PortraitFrame->SetContent(EnemyPortraitImage);
	PortraitSize->AddChild(PortraitFrame);
	HealthRow->AddChildToHorizontalBox(PortraitSize)->SetPadding(FMargin(8.0f, 1.0f));

	UVerticalBox* EnemySummary = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("EnemySummary"));
	UTextBlock* EnemyKind = MakeText(TEXT("EnemyKind"), TEXT("对手图鉴"), 14);
	EnemyKind->SetColorAndOpacity(FSlateColor(MutedParchment));
	EnemySummary->AddChildToVerticalBox(EnemyKind);
	EnemyHealthText = MakeText(TEXT("EnemyHealth"), TEXT("对手 92 / 92"), 20);
	EnemyHealthText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.42f, 0.31f, 1.0f)));
	EnemySummary->AddChildToVerticalBox(EnemyHealthText);
	EnemyHealthBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(), TEXT("EnemyHealthBar"));
	EnemyHealthBar->SetPercent(1.0f);
	EnemyHealthBar->SetFillColorAndOpacity(FLinearColor(0.82f, 0.12f, 0.09f, 1.0f));
	EnemySummary->AddChildToVerticalBox(EnemyHealthBar)->SetPadding(FMargin(12.0f, 4.0f));
	HealthRow->AddChildToHorizontalBox(EnemySummary)->SetPadding(FMargin(12.0f, 8.0f, 20.0f, 8.0f));

	UHorizontalBox* ResourceRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ResourceRow"));
	CombatBox->AddChildToVerticalBox(ResourceRow)->SetHorizontalAlignment(HAlign_Center);

	EnergyText = MakeText(TEXT("EnergyText"), TEXT("行动力 1 / 1"), 18);
	EnergyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.44f, 0.69f, 0.94f, 1.0f)));
	ResourceRow->AddChildToHorizontalBox(EnergyText)->SetPadding(FMargin(10.0f, 1.0f));

	BlockText = MakeText(TEXT("BlockText"), TEXT("格挡 0"), 18);
	BlockText->SetColorAndOpacity(FSlateColor(FLinearColor(0.60f, 0.73f, 0.81f, 1.0f)));
	ResourceRow->AddChildToHorizontalBox(BlockText)->SetPadding(FMargin(10.0f, 1.0f));

	ValorText = MakeText(TEXT("ValorText"), TEXT("法力 0  |  装备 0 / 3"), 18);
	ValorText->SetColorAndOpacity(FSlateColor(AntiqueGold));
	ResourceRow->AddChildToHorizontalBox(ValorText)->SetPadding(FMargin(10.0f, 1.0f));

	PileText = MakeText(TEXT("PileText"), TEXT("牌堆 5  |  弃牌 0  |  消耗 0"), 16);
	PileText->SetColorAndOpacity(FSlateColor(MutedParchment));
	CombatBox->AddChildToVerticalBox(PileText)->SetPadding(FMargin(0.0f, 2.0f));

	SealTextBlock = MakeText(TEXT("SealText"), TEXT("旅途牌组：基础牌"), 17);
	SealTextBlock->SetColorAndOpacity(FSlateColor(MutedParchment));
	CombatBox->AddChildToVerticalBox(SealTextBlock)->SetPadding(FMargin(0.0f, 2.0f));

	PlayerStatusTextBlock = MakeText(TEXT("PlayerStatusText"), TEXT("我方：无状态"), 15);
	PlayerStatusTextBlock->SetColorAndOpacity(FSlateColor(MutedParchment));
	PlayerStatusTextBlock->SetAutoWrapText(true);
	CombatBox->AddChildToVerticalBox(PlayerStatusTextBlock)->SetPadding(FMargin(48.0f, 1.0f));
	EnemyStatusTextBlock = MakeText(TEXT("EnemyStatusText"), TEXT("敌方：无状态"), 15);
	EnemyStatusTextBlock->SetColorAndOpacity(FSlateColor(MutedParchment));
	EnemyStatusTextBlock->SetAutoWrapText(true);
	CombatBox->AddChildToVerticalBox(EnemyStatusTextBlock)->SetPadding(FMargin(48.0f, 1.0f));

	UBorder* IntentPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("IntentPanel"));
	IntentPanel->SetBrushColor(DeepWine);
	IntentPanel->SetPadding(FMargin(12.0f, 4.0f));
	IntentTextBlock = MakeText(TEXT("IntentText"), TEXT("敌方公开牌：正在观察敌人……"), 17);
	IntentTextBlock->SetColorAndOpacity(FSlateColor(Parchment));
	IntentTextBlock->SetAutoWrapText(true);
	IntentPanel->SetContent(IntentTextBlock);
	CombatBox->AddChildToVerticalBox(IntentPanel)->SetPadding(FMargin(70.0f, 4.0f, 70.0f, 3.0f));

	CombatMessageText = MakeText(TEXT("CombatMessage"), TEXT("你的回合。"), 19);
	CombatMessageText->SetColorAndOpacity(FSlateColor(Parchment));
	CombatBox->AddChildToVerticalBox(CombatMessageText)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 5.0f));

	CardRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CardRow"));
	UVerticalBoxSlot* CardRowSlot = CombatBox->AddChildToVerticalBox(CardRow);
	CardRowSlot->SetHorizontalAlignment(HAlign_Center);
	CardRowSlot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 5.0f));

	for (int32 CardIndex = 0; CardIndex < 5; ++CardIndex)
	{
		const FName ButtonName(*FString::Printf(TEXT("CardButton_%d"), CardIndex));
		const FString LabelObjectName = FString::Printf(TEXT("CardButtonLabel_%d"), CardIndex);
		const FName CardSizeName(*FString::Printf(TEXT("CardSize_%d"), CardIndex));
		const FName CardContentName(*FString::Printf(TEXT("CardContent_%d"), CardIndex));
		const FName ArtSizeName(*FString::Printf(TEXT("CardArtSize_%d"), CardIndex));
		const FName ArtFrameName(*FString::Printf(TEXT("CardArtFrame_%d"), CardIndex));
		const FName ArtImageName(*FString::Printf(TEXT("CardArtImage_%d"), CardIndex));

		USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), CardSizeName);
		CardSize->SetWidthOverride(194.0f);
		CardSize->SetHeightOverride(214.0f);

		UButton* CardButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		CardButton->SetBackgroundColor(FLinearColor(0.22f, 0.15f, 0.07f, 1.0f));
		CardSize->AddChild(CardButton);

		UVerticalBox* CardContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), CardContentName);
		CardButton->AddChild(CardContent);

		USizeBox* ArtSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), ArtSizeName);
		ArtSize->SetWidthOverride(164.0f);
		ArtSize->SetHeightOverride(104.0f);
		UBorder* ArtFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), ArtFrameName);
		ArtFrame->SetBrushColor(FLinearColor(0.025f, 0.018f, 0.022f, 1.0f));
		ArtFrame->SetPadding(FMargin(3.0f));
		UImage* ArtImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), ArtImageName);
		ArtImage->SetColorAndOpacity(FLinearColor::Transparent);
		ArtFrame->SetContent(ArtImage);
		ArtSize->AddChild(ArtFrame);
		UVerticalBoxSlot* ArtSlot = CardContent->AddChildToVerticalBox(ArtSize);
		ArtSlot->SetHorizontalAlignment(HAlign_Center);
		ArtSlot->SetPadding(FMargin(4.0f, 6.0f, 4.0f, 4.0f));

		UTextBlock* CardLabel = MakeText(*LabelObjectName, TEXT("空卡槽"), 15);
		CardLabel->SetColorAndOpacity(FSlateColor(Parchment));
		CardLabel->SetAutoWrapText(true);
		UVerticalBoxSlot* LabelSlot = CardContent->AddChildToVerticalBox(CardLabel);
		LabelSlot->SetHorizontalAlignment(HAlign_Fill);
		LabelSlot->SetPadding(FMargin(7.0f, 3.0f));

		switch (CardIndex)
		{
		case 0: CardButton->OnClicked.AddDynamic(this, &UWorldWalkerHUDWidget::HandleCard0Clicked); break;
		case 1: CardButton->OnClicked.AddDynamic(this, &UWorldWalkerHUDWidget::HandleCard1Clicked); break;
		case 2: CardButton->OnClicked.AddDynamic(this, &UWorldWalkerHUDWidget::HandleCard2Clicked); break;
		case 3: CardButton->OnClicked.AddDynamic(this, &UWorldWalkerHUDWidget::HandleCard3Clicked); break;
		case 4: CardButton->OnClicked.AddDynamic(this, &UWorldWalkerHUDWidget::HandleCard4Clicked); break;
		default: break;
		}

		CardRow->AddChildToHorizontalBox(CardSize)->SetPadding(FMargin(4.0f, 0.0f));
		CardButtons.Add(CardButton);
		CardButtonLabels.Add(CardLabel);
		CardArtworkImages.Add(ArtImage);
		CardArtworkFrames.Add(ArtFrame);
	}

	EndTurnButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EndTurnButton"));
	EndTurnButton->SetBackgroundColor(FLinearColor(0.42f, 0.10f, 0.075f, 1.0f));
	UTextBlock* EndTurnLabel = MakeText(TEXT("EndTurnButtonLabel"), TEXT("结束回合"), 19);
	EndTurnLabel->SetColorAndOpacity(FSlateColor(Parchment));
	EndTurnButton->AddChild(EndTurnLabel);
	EndTurnButton->OnClicked.AddDynamic(this, &UWorldWalkerHUDWidget::HandleEndTurnClicked);
	CombatBox->AddChildToVerticalBox(EndTurnButton)->SetPadding(FMargin(420.0f, 2.0f));

	RestartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RestartButton"));
	RestartButton->SetBackgroundColor(FLinearColor(0.34f, 0.24f, 0.08f, 1.0f));
	RestartButtonLabel = MakeText(TEXT("RestartButtonLabel"), TEXT("重新挑战"), 19);
	RestartButtonLabel->SetColorAndOpacity(FSlateColor(Parchment));
	RestartButton->AddChild(RestartButtonLabel);
	RestartButton->OnClicked.AddDynamic(this, &UWorldWalkerHUDWidget::HandleRestartClicked);
	CombatBox->AddChildToVerticalBox(RestartButton)->SetPadding(FMargin(25.0f, 6.0f));

	UCanvasPanelSlot* CombatSlot = RootCanvas->AddChildToCanvas(CombatPanel);
	CombatSlot->SetAnchors(FAnchors(0.5f, 0.95f));
	CombatSlot->SetAlignment(FVector2D(0.5f, 1.0f));
	CombatSlot->SetSize(FVector2D(1220.0f, 760.0f));
}

void UWorldWalkerHUDWidget::ShowExploration()
{
	if (!ExplorationPanel || !CombatPanel || !ChoicePanel)
	{
		return;
	}

	ExplorationPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	CombatPanel->SetVisibility(ESlateVisibility::Collapsed);
	ChoicePanel->SetVisibility(ESlateVisibility::Collapsed);
	ChoicePanelMode = EChoicePanelMode::Hidden;
	bChoiceInputLocked = false;
	bShowingRewardChoices = false;
	bRewardConfirmed = false;
	bPlayerCanAct = false;
}

void UWorldWalkerHUDWidget::SetExplorationMessage(const FString& Message)
{
	if (ExplorationText)
	{
		ExplorationText->SetText(FText::FromString(Message));
	}
}

void UWorldWalkerHUDWidget::ShowCombat(
	const int32 PlayerHealth,
	const int32 PlayerMaxHealth,
	const int32 EnemyHealth,
	const int32 EnemyMaxHealth,
	const FString& EnemyDisplayName,
	UTexture2D* EnemyPortrait)
{
	ExplorationPanel->SetVisibility(ESlateVisibility::Collapsed);
	CombatPanel->SetVisibility(ESlateVisibility::Visible);
	ChoicePanel->SetVisibility(ESlateVisibility::Collapsed);
	ChoicePanelMode = EChoicePanelMode::Hidden;
	bChoiceInputLocked = false;
	bShowingRewardChoices = false;
	bRewardConfirmed = false;
	RestartButton->SetVisibility(ESlateVisibility::Collapsed);
	RestartButtonLabel->SetText(FText::FromString(TEXT("重新挑战")));
	CardRow->SetVisibility(ESlateVisibility::Visible);
	EndTurnButton->SetVisibility(ESlateVisibility::Visible);
	PlayerHealthText->SetText(FText::FromString(FString::Printf(
		TEXT("红斗篷骑士  %d / %d"), PlayerHealth, PlayerMaxHealth)));
	EnemyHealthText->SetText(FText::FromString(FString::Printf(
		TEXT("%s  %d / %d"), *EnemyDisplayName, EnemyHealth, EnemyMaxHealth)));
	PlayerHealthBar->SetPercent(PlayerMaxHealth > 0
		? static_cast<float>(PlayerHealth) / static_cast<float>(PlayerMaxHealth)
		: 0.0f);
	EnemyHealthBar->SetPercent(EnemyMaxHealth > 0
		? static_cast<float>(EnemyHealth) / static_cast<float>(EnemyMaxHealth)
		: 0.0f);
	if (EnemyPortrait)
	{
		EnemyPortraitImage->SetBrushFromTexture(EnemyPortrait, true);
		EnemyPortraitImage->SetColorAndOpacity(FLinearColor::White);
	}
	else
	{
		EnemyPortraitImage->SetBrushFromTexture(nullptr);
		EnemyPortraitImage->SetColorAndOpacity(FLinearColor(0.28f, 0.08f, 0.07f, 1.0f));
	}
	EnergyText->SetText(FText::FromString(TEXT("行动力 --")));
	BlockText->SetText(FText::FromString(TEXT("格挡 0")));
	ValorText->SetText(FText::FromString(TEXT("法力 0  |  装备 0 / 3")));
	PileText->SetText(FText::FromString(TEXT("牌堆 --  |  弃牌 0  |  消耗 0")));
	SealTextBlock->SetText(FText::FromString(TEXT("旅途牌组：基础牌")));
	PlayerStatusTextBlock->SetText(FText::FromString(TEXT("我方：无状态")));
	EnemyStatusTextBlock->SetText(FText::FromString(TEXT("敌方：无状态")));
	IntentTextBlock->SetText(FText::FromString(TEXT("敌方公开牌：正在观察敌人……")));
	CachedCardPlayable.Reset();
	SetCombatMessage(TEXT("正在整理起始手牌……"), false);
}

void UWorldWalkerHUDWidget::RefreshCombatState(
	const int32 PlayerHealth,
	const int32 PlayerMaxHealth,
	const int32 EnemyHealth,
	const int32 EnemyMaxHealth,
	const FString& EnemyDisplayName,
	UTexture2D* EnemyPortrait,
	const int32 CurrentActionPoints,
	const int32 MaxActionPoints,
	const int32 CurrentMana,
	const int32 EquipmentCount,
	const int32 CurrentBlock,
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
	PlayerHealthText->SetText(FText::FromString(FString::Printf(
		TEXT("红斗篷骑士  %d / %d"), PlayerHealth, PlayerMaxHealth)));
	EnemyHealthText->SetText(FText::FromString(FString::Printf(
		TEXT("%s  %d / %d"), *EnemyDisplayName, EnemyHealth, EnemyMaxHealth)));
	PlayerHealthBar->SetPercent(PlayerMaxHealth > 0
		? static_cast<float>(PlayerHealth) / static_cast<float>(PlayerMaxHealth)
		: 0.0f);
	EnemyHealthBar->SetPercent(EnemyMaxHealth > 0
		? static_cast<float>(EnemyHealth) / static_cast<float>(EnemyMaxHealth)
		: 0.0f);
	if (EnemyPortrait)
	{
		EnemyPortraitImage->SetBrushFromTexture(EnemyPortrait, true);
		EnemyPortraitImage->SetColorAndOpacity(FLinearColor::White);
	}
	EnergyText->SetText(FText::FromString(FString::Printf(
		TEXT("行动力  %d / %d"), CurrentActionPoints, MaxActionPoints)));
	BlockText->SetText(FText::FromString(FString::Printf(
		TEXT("格挡  %d"), CurrentBlock)));
	ValorText->SetText(FText::FromString(FString::Printf(
		TEXT("法力  %d  |  装备 %d / 3"), CurrentMana, EquipmentCount)));
	PileText->SetText(FText::FromString(FString::Printf(
		TEXT("牌堆 %d  |  弃牌 %d  |  消耗 %d"),
		DrawPileCount,
		DiscardPileCount,
		ExhaustPileCount)));
	SealTextBlock->SetText(FText::FromString(
		SealText.IsEmpty() ? TEXT("旅途牌组：基础牌") : SealText));
	PlayerStatusTextBlock->SetText(FText::FromString(FString::Printf(
		TEXT("我方：%s"), PlayerStatusText.IsEmpty() ? TEXT("无状态") : *PlayerStatusText)));
	EnemyStatusTextBlock->SetText(FText::FromString(FString::Printf(
		TEXT("敌方：%s"), EnemyStatusText.IsEmpty() ? TEXT("无状态") : *EnemyStatusText)));
	IntentTextBlock->SetText(FText::FromString(FString::Printf(
		TEXT("敌方公开牌：%s"), NextIntentText.IsEmpty() ? TEXT("未知") : *NextIntentText)));

	CachedCardPlayable = CardPlayable;
	for (int32 CardIndex = 0; CardIndex < CardButtons.Num(); ++CardIndex)
	{
		const bool bHasCard = CardLabels.IsValidIndex(CardIndex);
		CardButtons[CardIndex]->SetVisibility(bHasCard ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bHasCard)
		{
			CardButtonLabels[CardIndex]->SetText(FText::FromString(CardLabels[CardIndex]));

			const FLinearColor SchoolTint = CardSchoolTints.IsValidIndex(CardIndex)
				? CardSchoolTints[CardIndex]
				: FLinearColor(0.42f, 0.29f, 0.10f, 1.0f);
			CardButtons[CardIndex]->SetBackgroundColor(FLinearColor(
				0.035f + SchoolTint.R * 0.48f,
				0.025f + SchoolTint.G * 0.48f,
				0.025f + SchoolTint.B * 0.48f,
				1.0f));
			CardArtworkFrames[CardIndex]->SetBrushColor(FLinearColor(
				0.014f + SchoolTint.R * 0.16f,
				0.010f + SchoolTint.G * 0.16f,
				0.014f + SchoolTint.B * 0.16f,
				1.0f));

			if (CardArtworks.IsValidIndex(CardIndex) && CardArtworks[CardIndex])
			{
				CardArtworkImages[CardIndex]->SetBrushFromTexture(CardArtworks[CardIndex], true);
				CardArtworkImages[CardIndex]->SetColorAndOpacity(FLinearColor::White);
			}
			else
			{
				CardArtworkImages[CardIndex]->SetBrushFromTexture(nullptr);
				CardArtworkImages[CardIndex]->SetColorAndOpacity(FLinearColor::Transparent);
				CardArtworkFrames[CardIndex]->SetBrushColor(FLinearColor(0.018f, 0.014f, 0.018f, 1.0f));
			}

			CardButtons[CardIndex]->SetIsEnabled(
				bPlayerCanAct && CachedCardPlayable.IsValidIndex(CardIndex) && CachedCardPlayable[CardIndex]);
		}
	}
}

void UWorldWalkerHUDWidget::SetCombatMessage(const FString& Message, const bool bCanAct)
{
	CombatMessageText->SetText(FText::FromString(Message));
	bPlayerCanAct = bCanAct;
	for (int32 CardIndex = 0; CardIndex < CardButtons.Num(); ++CardIndex)
	{
		CardButtons[CardIndex]->SetIsEnabled(
			bCanAct && CachedCardPlayable.IsValidIndex(CardIndex) && CachedCardPlayable[CardIndex]);
	}
	EndTurnButton->SetIsEnabled(bCanAct);
}

void UWorldWalkerHUDWidget::ShowCombatResult(const bool bPlayerWon)
{
	bShowingRewardChoices = false;
	bRewardConfirmed = false;
	SetCombatMessage(
		bPlayerWon ? TEXT("胜利！对手已经倒下。") : TEXT("战败。本次旅途将从头开始。"),
		false);
	CardRow->SetVisibility(ESlateVisibility::Collapsed);
	EndTurnButton->SetVisibility(ESlateVisibility::Collapsed);
	RestartButtonLabel->SetText(FText::FromString(TEXT("重新挑战")));
	RestartButton->SetVisibility(ESlateVisibility::Visible);
}

void UWorldWalkerHUDWidget::ShowRewardSelection(
	const TArray<FString>& CardLabels,
	const TArray<UTexture2D*>& CardArtworks,
	const TArray<FLinearColor>& CardSchoolTints)
{
	bShowingRewardChoices = true;
	bRewardConfirmed = false;
	bPlayerCanAct = true;
	CachedCardPlayable.Init(true, CardLabels.Num());
	CardRow->SetVisibility(ESlateVisibility::Visible);
	EndTurnButton->SetVisibility(ESlateVisibility::Collapsed);
	RestartButton->SetVisibility(ESlateVisibility::Collapsed);
	CombatMessageText->SetText(FText::FromString(
		TEXT("胜利！从三张骑士牌中选择一张，加入本次旅途牌组。")));

	for (int32 CardIndex = 0; CardIndex < CardButtons.Num(); ++CardIndex)
	{
		const bool bHasReward = CardLabels.IsValidIndex(CardIndex);
		CardButtons[CardIndex]->SetVisibility(
			bHasReward ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (!bHasReward)
		{
			continue;
		}

		CardButtonLabels[CardIndex]->SetText(FText::FromString(CardLabels[CardIndex]));
		const FLinearColor SchoolTint = CardSchoolTints.IsValidIndex(CardIndex)
			? CardSchoolTints[CardIndex]
			: FLinearColor(0.42f, 0.29f, 0.10f, 1.0f);
		CardButtons[CardIndex]->SetBackgroundColor(FLinearColor(
			0.035f + SchoolTint.R * 0.48f,
			0.025f + SchoolTint.G * 0.48f,
			0.025f + SchoolTint.B * 0.48f,
			1.0f));
		CardArtworkFrames[CardIndex]->SetBrushColor(FLinearColor(
			0.014f + SchoolTint.R * 0.16f,
			0.010f + SchoolTint.G * 0.16f,
			0.014f + SchoolTint.B * 0.16f,
			1.0f));

		if (CardArtworks.IsValidIndex(CardIndex) && CardArtworks[CardIndex])
		{
			CardArtworkImages[CardIndex]->SetBrushFromTexture(CardArtworks[CardIndex], true);
			CardArtworkImages[CardIndex]->SetColorAndOpacity(FLinearColor::White);
		}
		else
		{
			CardArtworkImages[CardIndex]->SetBrushFromTexture(nullptr);
			CardArtworkImages[CardIndex]->SetColorAndOpacity(FLinearColor::Transparent);
		}
		CardButtons[CardIndex]->SetIsEnabled(true);
	}
}

void UWorldWalkerHUDWidget::ShowRewardConfirmation(const FString& ConfirmationText)
{
	bShowingRewardChoices = false;
	bRewardConfirmed = true;
	bPlayerCanAct = false;
	CardRow->SetVisibility(ESlateVisibility::Collapsed);
	EndTurnButton->SetVisibility(ESlateVisibility::Collapsed);
	CombatMessageText->SetText(FText::FromString(ConfirmationText));
	RestartButtonLabel->SetText(FText::FromString(TEXT("确认战利品并继续路线")));
	RestartButton->SetVisibility(ESlateVisibility::Visible);
	RestartButton->SetIsEnabled(true);
}

void UWorldWalkerHUDWidget::ShowRouteSelection(
	const FString& RunSummary,
	const TArray<FString>& ChoiceLabels)
{
	ShowChoicePanel(
		EChoicePanelMode::Route,
		TEXT("月圆之路 · 选择下一处命运"),
		RunSummary,
		TEXT("每次只能踏上一条路。击败对手后，你可以从三张牌中选择一张加入本次旅途牌组。"),
		ChoiceLabels);
}

void UWorldWalkerHUDWidget::ShowEventSelection(
	const FString& Title,
	const FString& Lore,
	const TArray<FString>& ChoiceLabels)
{
	ShowChoicePanel(
		EChoicePanelMode::Event,
		Title.IsEmpty() ? TEXT("月下奇遇") : Title,
		TEXT("旅途事件 · 选择后立即生效"),
		Lore,
		ChoiceLabels);
}

void UWorldWalkerHUDWidget::ShowNodeResolution(
	const FString& Message,
	const bool bChapterComplete)
{
	ShowChoicePanel(
		EChoicePanelMode::Resolution,
		bChapterComplete ? TEXT("灰烬章节完成") : TEXT("命运已定"),
		bChapterComplete ? TEXT("月轮见证了这场旅途") : TEXT("前方的道路再次显现"),
		Message,
		TArray<FString>());
	ChoiceContinueLabel->SetText(FText::FromString(
		bChapterComplete ? TEXT("返回灰烬世界") : TEXT("继续旅途")));
}

void UWorldWalkerHUDWidget::ShowChoicePanel(
	const EChoicePanelMode InMode,
	const FString& Title,
	const FString& Summary,
	const FString& Lore,
	const TArray<FString>& ChoiceLabels)
{
	if (!ChoicePanel || !ExplorationPanel || !CombatPanel)
	{
		return;
	}

	ChoicePanelMode = InMode;
	bChoiceInputLocked = false;
	ExplorationPanel->SetVisibility(ESlateVisibility::Collapsed);
	CombatPanel->SetVisibility(ESlateVisibility::Collapsed);
	ChoicePanel->SetVisibility(ESlateVisibility::Visible);
	ChoiceTitleText->SetText(FText::FromString(Title));
	ChoiceSummaryText->SetText(FText::FromString(Summary));
	ChoiceSummaryText->SetVisibility(
		Summary.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	ChoiceLoreText->SetText(FText::FromString(Lore));

	const bool bShowingChoices = InMode == EChoicePanelMode::Route || InMode == EChoicePanelMode::Event;
	for (int32 ChoiceIndex = 0; ChoiceIndex < ChoiceButtons.Num(); ++ChoiceIndex)
	{
		const bool bHasChoice = bShowingChoices && ChoiceLabels.IsValidIndex(ChoiceIndex);
		ChoiceButtons[ChoiceIndex]->SetVisibility(
			bHasChoice ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		ChoiceButtons[ChoiceIndex]->SetIsEnabled(bHasChoice);
		if (bHasChoice)
		{
			ChoiceButtonLabels[ChoiceIndex]->SetText(FText::FromString(ChoiceLabels[ChoiceIndex]));
		}
	}

	ChoiceContinueButton->SetVisibility(
		InMode == EChoicePanelMode::Resolution ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	ChoiceContinueButton->SetIsEnabled(InMode == EChoicePanelMode::Resolution);
	if (InMode != EChoicePanelMode::Resolution)
	{
		ChoiceContinueLabel->SetText(FText::FromString(TEXT("继续旅途")));
	}
}

void UWorldWalkerHUDWidget::HandleCard0Clicked() { HandleCardClicked(0); }
void UWorldWalkerHUDWidget::HandleCard1Clicked() { HandleCardClicked(1); }
void UWorldWalkerHUDWidget::HandleCard2Clicked() { HandleCardClicked(2); }
void UWorldWalkerHUDWidget::HandleCard3Clicked() { HandleCardClicked(3); }
void UWorldWalkerHUDWidget::HandleCard4Clicked() { HandleCardClicked(4); }

void UWorldWalkerHUDWidget::HandleChoice0Clicked() { HandleChoiceClicked(0); }
void UWorldWalkerHUDWidget::HandleChoice1Clicked() { HandleChoiceClicked(1); }
void UWorldWalkerHUDWidget::HandleChoice2Clicked() { HandleChoiceClicked(2); }

void UWorldWalkerHUDWidget::HandleChoiceClicked(const int32 ChoiceIndex)
{
	if (bChoiceInputLocked
		|| (ChoicePanelMode != EChoicePanelMode::Route && ChoicePanelMode != EChoicePanelMode::Event)
		|| !ChoiceButtons.IsValidIndex(ChoiceIndex)
		|| ChoiceButtons[ChoiceIndex]->GetVisibility() != ESlateVisibility::Visible)
	{
		return;
	}

	bChoiceInputLocked = true;
	for (UButton* ChoiceButton : ChoiceButtons)
	{
		if (ChoiceButton)
		{
			ChoiceButton->SetIsEnabled(false);
		}
	}

	if (AWorldWalkerGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AWorldWalkerGameModeBase>())
	{
		if (ChoicePanelMode == EChoicePanelMode::Route)
		{
			GameMode->HandleRouteSelection(ChoiceIndex);
		}
		else
		{
			GameMode->HandleEventSelection(ChoiceIndex);
		}
		return;
	}

	// Keep the prototype recoverable when the widget is previewed without its game mode.
	bChoiceInputLocked = false;
	for (UButton* ChoiceButton : ChoiceButtons)
	{
		if (ChoiceButton && ChoiceButton->GetVisibility() == ESlateVisibility::Visible)
		{
			ChoiceButton->SetIsEnabled(true);
		}
	}
}

void UWorldWalkerHUDWidget::HandleChoiceContinueClicked()
{
	if (bChoiceInputLocked || ChoicePanelMode != EChoicePanelMode::Resolution)
	{
		return;
	}

	bChoiceInputLocked = true;
	ChoiceContinueButton->SetIsEnabled(false);
	if (AWorldWalkerGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AWorldWalkerGameModeBase>())
	{
		GameMode->HandleNodeResolutionContinue();
		return;
	}

	bChoiceInputLocked = false;
	ChoiceContinueButton->SetIsEnabled(true);
}

void UWorldWalkerHUDWidget::HandleCardClicked(const int32 HandIndex)
{
	if (AWorldWalkerGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AWorldWalkerGameModeBase>())
	{
		if (bShowingRewardChoices)
		{
			GameMode->HandleRewardSelection(HandIndex);
		}
		else
		{
			GameMode->HandlePlayCard(HandIndex);
		}
	}
}

void UWorldWalkerHUDWidget::HandleEndTurnClicked()
{
	if (AWorldWalkerGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AWorldWalkerGameModeBase>())
	{
		GameMode->HandleEndPlayerTurn();
	}
}

void UWorldWalkerHUDWidget::HandleRestartClicked()
{
	if (AWorldWalkerGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AWorldWalkerGameModeBase>())
	{
		if (bRewardConfirmed)
		{
			GameMode->HandleReturnToExploration();
		}
		else
		{
			GameMode->RestartDemo();
		}
	}
}
