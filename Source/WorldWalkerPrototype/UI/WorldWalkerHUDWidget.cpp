#include "UI/WorldWalkerHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
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

	CombatPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CombatPanel"));
	CombatPanel->SetBrushColor(NearBlack);
	CombatPanel->SetPadding(FMargin(24.0f, 16.0f));

	UVerticalBox* CombatBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CombatBox"));
	CombatPanel->SetContent(CombatBox);

	UTextBlock* Title = MakeText(TEXT("CombatTitle"), TEXT("灰烬盟誓 · 黑棘决斗"), 27);
	Title->SetColorAndOpacity(FSlateColor(AntiqueGold));
	CombatBox->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 7.0f));

	UHorizontalBox* HealthRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HealthRow"));
	CombatBox->AddChildToVerticalBox(HealthRow)->SetHorizontalAlignment(HAlign_Center);

	PlayerHealthText = MakeText(TEXT("PlayerHealth"), TEXT("符文骑士 100 / 100"), 20);
	PlayerHealthText->SetColorAndOpacity(FSlateColor(Parchment));
	HealthRow->AddChildToHorizontalBox(PlayerHealthText)->SetPadding(FMargin(24.0f, 2.0f));

	EnemyHealthText = MakeText(TEXT("EnemyHealth"), TEXT("黑棘骑士 92 / 92"), 20);
	EnemyHealthText->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.34f, 0.27f, 1.0f)));
	HealthRow->AddChildToHorizontalBox(EnemyHealthText)->SetPadding(FMargin(24.0f, 2.0f));

	UHorizontalBox* ResourceRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ResourceRow"));
	CombatBox->AddChildToVerticalBox(ResourceRow)->SetHorizontalAlignment(HAlign_Center);

	EnergyText = MakeText(TEXT("EnergyText"), TEXT("能量 3 / 3"), 18);
	EnergyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.44f, 0.69f, 0.94f, 1.0f)));
	ResourceRow->AddChildToHorizontalBox(EnergyText)->SetPadding(FMargin(18.0f, 1.0f));

	BlockText = MakeText(TEXT("BlockText"), TEXT("格挡 0"), 18);
	BlockText->SetColorAndOpacity(FSlateColor(FLinearColor(0.60f, 0.73f, 0.81f, 1.0f)));
	ResourceRow->AddChildToHorizontalBox(BlockText)->SetPadding(FMargin(18.0f, 1.0f));

	ValorText = MakeText(TEXT("ValorText"), TEXT("英勇 0 / 3"), 18);
	ValorText->SetColorAndOpacity(FSlateColor(AntiqueGold));
	ResourceRow->AddChildToHorizontalBox(ValorText)->SetPadding(FMargin(18.0f, 1.0f));

	SealTextBlock = MakeText(TEXT("SealText"), TEXT("三印  [钢铁] [圣徽] [奥术]"), 17);
	SealTextBlock->SetColorAndOpacity(FSlateColor(MutedParchment));
	CombatBox->AddChildToVerticalBox(SealTextBlock)->SetPadding(FMargin(0.0f, 2.0f));

	UHorizontalBox* StatusRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StatusRow"));
	CombatBox->AddChildToVerticalBox(StatusRow)->SetHorizontalAlignment(HAlign_Center);
	PlayerStatusTextBlock = MakeText(TEXT("PlayerStatusText"), TEXT("我方：无状态"), 15);
	PlayerStatusTextBlock->SetColorAndOpacity(FSlateColor(MutedParchment));
	StatusRow->AddChildToHorizontalBox(PlayerStatusTextBlock)->SetPadding(FMargin(18.0f, 1.0f));
	EnemyStatusTextBlock = MakeText(TEXT("EnemyStatusText"), TEXT("敌方：无状态"), 15);
	EnemyStatusTextBlock->SetColorAndOpacity(FSlateColor(MutedParchment));
	StatusRow->AddChildToHorizontalBox(EnemyStatusTextBlock)->SetPadding(FMargin(18.0f, 1.0f));

	UBorder* IntentPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("IntentPanel"));
	IntentPanel->SetBrushColor(DeepWine);
	IntentPanel->SetPadding(FMargin(12.0f, 4.0f));
	IntentTextBlock = MakeText(TEXT("IntentText"), TEXT("下一意图：正在观察敌人……"), 17);
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
	UTextBlock* RestartLabel = MakeText(TEXT("RestartButtonLabel"), TEXT("返回灰烬隘口"), 19);
	RestartLabel->SetColorAndOpacity(FSlateColor(Parchment));
	RestartButton->AddChild(RestartLabel);
	RestartButton->OnClicked.AddDynamic(this, &UWorldWalkerHUDWidget::HandleRestartClicked);
	CombatBox->AddChildToVerticalBox(RestartButton)->SetPadding(FMargin(25.0f, 6.0f));

	UCanvasPanelSlot* CombatSlot = RootCanvas->AddChildToCanvas(CombatPanel);
	CombatSlot->SetAnchors(FAnchors(0.5f, 0.95f));
	CombatSlot->SetAlignment(FVector2D(0.5f, 1.0f));
	CombatSlot->SetSize(FVector2D(1120.0f, 650.0f));
}

void UWorldWalkerHUDWidget::ShowExploration()
{
	if (!ExplorationPanel || !CombatPanel)
	{
		return;
	}

	ExplorationPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	CombatPanel->SetVisibility(ESlateVisibility::Collapsed);
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
	const int32 EnemyMaxHealth)
{
	ExplorationPanel->SetVisibility(ESlateVisibility::Collapsed);
	CombatPanel->SetVisibility(ESlateVisibility::Visible);
	RestartButton->SetVisibility(ESlateVisibility::Collapsed);
	CardRow->SetVisibility(ESlateVisibility::Visible);
	EndTurnButton->SetVisibility(ESlateVisibility::Visible);
	PlayerHealthText->SetText(FText::FromString(FString::Printf(
		TEXT("符文骑士  %d / %d"), PlayerHealth, PlayerMaxHealth)));
	EnemyHealthText->SetText(FText::FromString(FString::Printf(
		TEXT("黑棘骑士  %d / %d"), EnemyHealth, EnemyMaxHealth)));
	EnergyText->SetText(FText::FromString(TEXT("能量 --")));
	BlockText->SetText(FText::FromString(TEXT("格挡 0")));
	ValorText->SetText(FText::FromString(TEXT("英勇 0 / 3")));
	SealTextBlock->SetText(FText::FromString(TEXT("三印  [钢铁] [圣徽] [奥术]")));
	PlayerStatusTextBlock->SetText(FText::FromString(TEXT("我方：无状态")));
	EnemyStatusTextBlock->SetText(FText::FromString(TEXT("敌方：无状态")));
	IntentTextBlock->SetText(FText::FromString(TEXT("下一意图：正在观察敌人……")));
	CachedCardPlayable.Reset();
	SetCombatMessage(TEXT("正在整理起始手牌……"), false);
}

void UWorldWalkerHUDWidget::RefreshCombatState(
	const int32 PlayerHealth,
	const int32 PlayerMaxHealth,
	const int32 EnemyHealth,
	const int32 EnemyMaxHealth,
	const int32 CurrentEnergy,
	const int32 MaxEnergy,
	const int32 CurrentBlock,
	const int32 CurrentValor,
	const int32 MaxValor,
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
		TEXT("符文骑士  %d / %d"), PlayerHealth, PlayerMaxHealth)));
	EnemyHealthText->SetText(FText::FromString(FString::Printf(
		TEXT("黑棘骑士  %d / %d"), EnemyHealth, EnemyMaxHealth)));
	EnergyText->SetText(FText::FromString(FString::Printf(
		TEXT("能量  %d / %d"), CurrentEnergy, MaxEnergy)));
	BlockText->SetText(FText::FromString(FString::Printf(
		TEXT("格挡  %d"), CurrentBlock)));
	ValorText->SetText(FText::FromString(FString::Printf(
		TEXT("英勇  %d / %d"), CurrentValor, MaxValor)));
	SealTextBlock->SetText(FText::FromString(
		SealText.IsEmpty() ? TEXT("三印：尚未点亮") : SealText));
	PlayerStatusTextBlock->SetText(FText::FromString(FString::Printf(
		TEXT("我方：%s"), PlayerStatusText.IsEmpty() ? TEXT("无状态") : *PlayerStatusText)));
	EnemyStatusTextBlock->SetText(FText::FromString(FString::Printf(
		TEXT("敌方：%s"), EnemyStatusText.IsEmpty() ? TEXT("无状态") : *EnemyStatusText)));
	IntentTextBlock->SetText(FText::FromString(FString::Printf(
		TEXT("下一意图：%s"), NextIntentText.IsEmpty() ? TEXT("未知") : *NextIntentText)));

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
	SetCombatMessage(
		bPlayerWon ? TEXT("胜利！黑棘誓约已经破除。") : TEXT("战败。灰烬隘口又吞没了一位立誓者。"),
		false);
	CardRow->SetVisibility(ESlateVisibility::Collapsed);
	EndTurnButton->SetVisibility(ESlateVisibility::Collapsed);
	RestartButton->SetVisibility(ESlateVisibility::Visible);
}

void UWorldWalkerHUDWidget::HandleCard0Clicked() { HandleCardClicked(0); }
void UWorldWalkerHUDWidget::HandleCard1Clicked() { HandleCardClicked(1); }
void UWorldWalkerHUDWidget::HandleCard2Clicked() { HandleCardClicked(2); }
void UWorldWalkerHUDWidget::HandleCard3Clicked() { HandleCardClicked(3); }
void UWorldWalkerHUDWidget::HandleCard4Clicked() { HandleCardClicked(4); }

void UWorldWalkerHUDWidget::HandleCardClicked(const int32 HandIndex)
{
	if (AWorldWalkerGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AWorldWalkerGameModeBase>())
	{
		GameMode->HandlePlayCard(HandIndex);
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
		GameMode->RestartDemo();
	}
}
