#include "UI/W11CombatHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/W11AttributeComponent.h"
#include "Components/W11CombatComponent.h"
#include "Game/W11Character.h"
#include "Game/W11GameMode.h"
#include "Game/W11GameState.h"
#include "Game/W11PlayerController.h"
#include "Game/W11PlayerState.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "WorldWalkerW11.h"

namespace
{
UTextBlock* AddHudText(UWidgetTree* Tree, UVerticalBox* Parent, const TCHAR* InitialText, const int32 Size)
{
	UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(InitialText));
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = Size;
	Text->SetFont(Font);
	Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.96f, 0.91f, 1.0f)));
	Parent->AddChildToVerticalBox(Text);
	return Text;
}

UProgressBar* AddHudBar(UWidgetTree* Tree, UVerticalBox* Parent, const FLinearColor& Color)
{
	UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>();
	Bar->SetFillColorAndOpacity(Color);
	Bar->SetPercent(1.0f);
	if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Bar))
	{
		Slot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 6.0f));
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	return Bar;
}

void ConfigureCanvasSlot(
	UWidget* Widget,
	const FAnchors& Anchors,
	const FVector2D& Position,
	const FVector2D& Size,
	const FVector2D& Alignment = FVector2D::ZeroVector)
{
	if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot))
	{
		Slot->SetAnchors(Anchors);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
		Slot->SetAlignment(Alignment);
	}
}
}

TSharedRef<SWidget> UW11CombatHUDWidget::RebuildWidget()
{
	// Native UMG trees must exist before Super builds the backing Slate widget.
	// Constructing this tree from NativeConstruct leaves the viewport holding SNullWidget.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UW11CombatHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindRuntimeSources();
	RefreshVitals();
	RefreshCombatState();
	RefreshManualEvidenceGuide();
	bCapturePending = FParse::Param(FCommandLine::Get(), TEXT("W11CombatHUDCapture"));
	FParse::Value(FCommandLine::Get(), TEXT("W11CombatHUDCaptureDelay="), CaptureDelay);
	CaptureDelay = FMath::Max(0.1f, CaptureDelay);
	if (UWorld* World = GetWorld())
	{
		if (bCapturePending)
		{
			World->GetTimerManager().SetTimer(
				CaptureTimerHandle, this, &UW11CombatHUDWidget::CaptureForValidation, CaptureDelay, false);
		}
	}
}

void UW11CombatHUDWidget::NativeDestruct()
{
	if (W11GameState.IsValid())
	{
		W11GameState->OnRuntimeStateChanged.RemoveAll(this);
	}
	if (Attributes.IsValid())
	{
		Attributes->OnHealthChanged.RemoveAll(this);
		Attributes->OnManaChanged.RemoveAll(this);
		Attributes->OnBarrierChanged.RemoveAll(this);
		Attributes->OnStatusesChanged.RemoveAll(this);
	}
	if (Combat.IsValid())
	{
		Combat->OnAbilityResult.RemoveAll(this);
		Combat->OnCooldownChanged.RemoveAll(this);
	}
	if (W11Character.IsValid())
	{
		W11Character->OnIncomingCombatCue.RemoveAll(this);
		W11Character->OnDodgeCooldownChanged.RemoveAll(this);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CaptureTimerHandle);
	}
	Super::NativeDestruct();
}

void UW11CombatHUDWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshAccumulator += InDeltaTime;
	TeamRefreshAccumulator += InDeltaTime;
	if (RefreshAccumulator < 0.05f)
	{
		return;
	}
	RefreshAccumulator = 0.0f;
	if (!Attributes.IsValid() || !Combat.IsValid() || !W11GameState.IsValid())
	{
		BindRuntimeSources();
	}
	if (StatusText && GetWorld() && StatusVisibleUntil > 0.0f && GetWorld()->GetTimeSeconds() >= StatusVisibleUntil)
	{
		StatusText->SetText(FText::GetEmpty());
		StatusVisibleUntil = 0.0f;
	}
	if (DamageDirectionPanel && GetWorld() && DamageDirectionVisibleUntil > 0.0f
		&& GetWorld()->GetTimeSeconds() >= DamageDirectionVisibleUntil)
	{
		DamageDirectionPanel->SetVisibility(ESlateVisibility::Collapsed);
		DamageDirectionVisibleUntil = 0.0f;
	}
	// Only server-time displays and transient expiry need periodic presentation work.
	// Replicated facts themselves refresh through their delegates.
	if (W11GameState.IsValid()
		&& W11GameState->GetRunPhase() == EW11RunPhase::Combat
		&& W11GameState->GetEncounterType() == EW11EncounterType::Survival)
	{
		RefreshCombatState();
	}
	RefreshCooldowns();
	if (TeamRefreshAccumulator >= 0.25f)
	{
		TeamRefreshAccumulator = 0.0f;
		RefreshTeamStatus();
		RefreshManualEvidenceGuide();
	}
}

void UW11CombatHUDWidget::RefreshRuntimeState()
{
	if (!Attributes.IsValid() || !Combat.IsValid() || !W11GameState.IsValid())
	{
		BindRuntimeSources();
	}
	RefreshVitals();
	RefreshCombatState();
	RefreshCooldowns();
	RefreshTeamStatus();
	RefreshManualEvidenceGuide();
	if (GetWorld())
	{
		const float Now = GetWorld()->GetTimeSeconds();
		if (StatusText && StatusVisibleUntil > 0.0f && Now >= StatusVisibleUntil)
		{
			StatusText->SetText(FText::GetEmpty());
			StatusVisibleUntil = 0.0f;
		}
		if (DamageDirectionPanel && DamageDirectionVisibleUntil > 0.0f
			&& Now >= DamageDirectionVisibleUntil)
		{
			DamageDirectionPanel->SetVisibility(ESlateVisibility::Collapsed);
			DamageDirectionVisibleUntil = 0.0f;
		}
	}
}

void UW11CombatHUDWidget::CaptureForValidation()
{
	bCapturePending = false;
	RefreshRuntimeState();
	const int32 Phase = W11GameState.IsValid() ? static_cast<int32>(W11GameState->GetRunPhase()) : -1;
	const FVector2D GeometrySize = GetCachedGeometry().GetLocalSize();
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_COMBAT_HUD_CAPTURE Phase=%d Visibility=%d Opacity=%.1f Geometry=%.0fx%.0f Delay=%.2f"),
		Phase, static_cast<int32>(GetVisibility()), GetRenderOpacity(), GeometrySize.X,
		GeometrySize.Y, CaptureDelay);
	FScreenshotRequest::RequestScreenshot(true);
}

void UW11CombatHUDWidget::BuildLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Root;

	UBorder* VitalsPanel = WidgetTree->ConstructWidget<UBorder>();
	VitalsPanel->SetBrushColor(FLinearColor(0.015f, 0.04f, 0.035f, 0.86f));
	VitalsPanel->SetPadding(FMargin(18.0f, 12.0f));
	Root->AddChild(VitalsPanel);
	ConfigureCanvasSlot(VitalsPanel, FAnchors(0.0f, 1.0f), FVector2D(28.0f, -196.0f), FVector2D(500.0f, 168.0f));
	UVerticalBox* Vitals = WidgetTree->ConstructWidget<UVerticalBox>();
	VitalsPanel->SetContent(Vitals);
	HealthText = AddHudText(WidgetTree, Vitals, TEXT("气血"), 21);
	HealthBar = AddHudBar(WidgetTree, Vitals, FLinearColor(0.72f, 0.09f, 0.08f, 1.0f));
	ManaText = AddHudText(WidgetTree, Vitals, TEXT("真元"), 19);
	ManaBar = AddHudBar(WidgetTree, Vitals, FLinearColor(0.08f, 0.42f, 0.78f, 1.0f));
	BarrierText = AddHudText(WidgetTree, Vitals, TEXT("护障"), 18);
	BarrierBar = AddHudBar(WidgetTree, Vitals, FLinearColor(0.10f, 0.72f, 0.82f, 1.0f));

	UBorder* ObjectivePanel = WidgetTree->ConstructWidget<UBorder>();
	ObjectivePanel->SetBrushColor(FLinearColor(0.02f, 0.05f, 0.04f, 0.78f));
	ObjectivePanel->SetPadding(FMargin(20.0f, 10.0f));
	Root->AddChild(ObjectivePanel);
	ConfigureCanvasSlot(ObjectivePanel, FAnchors(0.5f, 0.0f), FVector2D(0.0f, 26.0f), FVector2D(760.0f, 66.0f), FVector2D(0.5f, 0.0f));
	ObjectiveText = WidgetTree->ConstructWidget<UTextBlock>();
	ObjectiveText->SetJustification(ETextJustify::Center);
	FSlateFontInfo ObjectiveFont = ObjectiveText->GetFont();
	ObjectiveFont.Size = 24;
	ObjectiveText->SetFont(ObjectiveFont);
	ObjectivePanel->SetContent(ObjectiveText);

	TeamStatusPanel = WidgetTree->ConstructWidget<UBorder>();
	TeamStatusPanel->SetBrushColor(FLinearColor(0.015f, 0.04f, 0.035f, 0.78f));
	TeamStatusPanel->SetPadding(FMargin(16.0f, 10.0f));
	Root->AddChild(TeamStatusPanel);
	ConfigureCanvasSlot(TeamStatusPanel, FAnchors(1.0f, 0.0f), FVector2D(-28.0f, 28.0f),
		FVector2D(460.0f, 132.0f), FVector2D(1.0f, 0.0f));
	TeamStatusText = WidgetTree->ConstructWidget<UTextBlock>();
	FSlateFontInfo TeamFont = TeamStatusText->GetFont();
	TeamFont.Size = 18;
	TeamStatusText->SetFont(TeamFont);
	TeamStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.92f, 0.86f, 1.0f)));
	TeamStatusPanel->SetContent(TeamStatusText);
	TeamStatusPanel->SetVisibility(ESlateVisibility::Collapsed);

	UBorder* CooldownPanel = WidgetTree->ConstructWidget<UBorder>();
	CooldownPanel->SetBrushColor(FLinearColor(0.015f, 0.035f, 0.03f, 0.82f));
	CooldownPanel->SetPadding(FMargin(18.0f, 10.0f));
	Root->AddChild(CooldownPanel);
	ConfigureCanvasSlot(CooldownPanel, FAnchors(0.5f, 1.0f), FVector2D(0.0f, -156.0f), FVector2D(1040.0f, 128.0f), FVector2D(0.5f, 0.0f));
	UVerticalBox* Cooldowns = WidgetTree->ConstructWidget<UVerticalBox>();
	CooldownPanel->SetContent(Cooldowns);
	CooldownText = AddHudText(WidgetTree, Cooldowns, TEXT("F 普攻 · Q 术法一 · W 术法二 · E 术法三 · R 术法四 · Space 闪避"), 18);
	CooldownText->SetJustification(ETextJustify::Center);
	ActiveStatusText = AddHudText(WidgetTree, Cooldowns, TEXT(""), 17);
	ActiveStatusText->SetJustification(ETextJustify::Center);
	ActiveStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.82f, 1.0f, 1.0f)));
	StatusText = AddHudText(WidgetTree, Cooldowns, TEXT(""), 19);
	StatusText->SetJustification(ETextJustify::Center);
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.68f, 0.22f, 1.0f)));

	DamageDirectionPanel = WidgetTree->ConstructWidget<UBorder>();
	DamageDirectionPanel->SetBrushColor(FLinearColor(0.18f, 0.015f, 0.01f, 0.78f));
	DamageDirectionPanel->SetPadding(FMargin(16.0f, 8.0f));
	Root->AddChild(DamageDirectionPanel);
	ConfigureCanvasSlot(DamageDirectionPanel, FAnchors(0.5f, 0.5f), FVector2D(0.0f, -190.0f),
		FVector2D(420.0f, 54.0f), FVector2D(0.5f, 0.5f));
	DamageDirectionText = WidgetTree->ConstructWidget<UTextBlock>();
	DamageDirectionText->SetJustification(ETextJustify::Center);
	FSlateFontInfo DirectionFont = DamageDirectionText->GetFont();
	DirectionFont.Size = 23;
	DamageDirectionText->SetFont(DirectionFont);
	DamageDirectionText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.56f, 0.28f, 1.0f)));
	DamageDirectionPanel->SetContent(DamageDirectionText);
	DamageDirectionPanel->SetVisibility(ESlateVisibility::Collapsed);

	ManualEvidencePanel = WidgetTree->ConstructWidget<UBorder>();
	ManualEvidencePanel->SetPadding(FMargin(16.0f, 12.0f));
	ManualEvidencePanel->SetBrushColor(FLinearColor(0.025f, 0.15f, 0.11f, 0.92f));
	Root->AddChild(ManualEvidencePanel);
	ConfigureCanvasSlot(ManualEvidencePanel, FAnchors(1.0f, 0.0f), FVector2D(-28.0f, 110.0f),
		FVector2D(540.0f, 212.0f), FVector2D(1.0f, 0.0f));
	ManualEvidenceText = WidgetTree->ConstructWidget<UTextBlock>();
	ManualEvidenceText->SetAutoWrapText(true);
	FSlateFontInfo ManualEvidenceFont = ManualEvidenceText->GetFont();
	ManualEvidenceFont.Size = 17;
	ManualEvidenceText->SetFont(ManualEvidenceFont);
	ManualEvidenceText->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.96f, 0.83f, 1.0f)));
	ManualEvidencePanel->SetContent(ManualEvidenceText);
	ManualEvidencePanel->SetVisibility(ESlateVisibility::Collapsed);

	ResultsOverlay = WidgetTree->ConstructWidget<UBorder>();
	ResultsOverlay->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.82f));
	ResultsOverlay->SetHorizontalAlignment(HAlign_Center);
	ResultsOverlay->SetVerticalAlignment(VAlign_Center);
	Root->AddChild(ResultsOverlay);
	ConfigureCanvasSlot(ResultsOverlay, FAnchors(0.0f, 0.0f, 1.0f, 1.0f), FVector2D::ZeroVector, FVector2D::ZeroVector);
	UVerticalBox* ResultBox = WidgetTree->ConstructWidget<UVerticalBox>();
	ResultsOverlay->SetContent(ResultBox);
	ResultTitle = AddHudText(WidgetTree, ResultBox, TEXT("道途断绝"), 42);
	ResultTitle->SetJustification(ETextJustify::Center);
	RestartButton = WidgetTree->ConstructWidget<UButton>();
	RestartButton->SetBackgroundColor(FLinearColor(0.08f, 0.26f, 0.20f, 1.0f));
	RestartButton->OnClicked.AddDynamic(this, &UW11CombatHUDWidget::HandleRestartClicked);
	ResultBox->AddChildToVerticalBox(RestartButton);
	UTextBlock* RestartText = WidgetTree->ConstructWidget<UTextBlock>();
	RestartText->SetText(FText::FromString(TEXT("再入轮回")));
	FSlateFontInfo RestartFont = RestartText->GetFont();
	RestartFont.Size = 26;
	RestartText->SetFont(RestartFont);
	RestartText->SetJustification(ETextJustify::Center);
	RestartButton->SetContent(RestartText);
	ResultsOverlay->SetVisibility(ESlateVisibility::Collapsed);
}

void UW11CombatHUDWidget::RefreshManualEvidenceGuide()
{
	if (!ManualEvidencePanel || !ManualEvidenceText)
	{
		return;
	}
	const AW11GameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AW11GameMode>() : nullptr;
	const FName Role = GameMode ? GameMode->GetActiveManualEvidenceRole() : NAME_None;
	if (Role.IsNone())
	{
		ManualEvidencePanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const FName RejectionReason = GameMode->GetManualEvidenceSessionRejectionReason();
	const bool bAccepted = RejectionReason.IsNone();
	const FString Header = bAccepted
		? FString::Printf(TEXT("人工验收 · %s"), *AW11GameMode::GetManualEvidenceRoleDisplayName(Role))
		: FString::Printf(TEXT("人工验收无效 · %s"), *RejectionReason.ToString());
	const FString Footer = bAccepted
		? TEXT("完成一场后退出游戏；不要启用脚本、自动开始或测试参数。")
		: TEXT("本局不会进入证据集合，请退出并使用 RunW11.ps1 重新启动。");
	const FW11ManualEvidenceProgress Progress = GameMode->GetManualEvidenceProgress();
	const FString ProgressLine = bAccepted && Progress.TotalSteps > 0
		? FString::Printf(TEXT("实时客观进度：%d/%d%s%s · %s"),
			Progress.CompletedSteps, Progress.TotalSteps,
			Progress.bComplete ? TEXT(" 已完成") : TEXT(""),
			Progress.bViolation ? TEXT(" 已违反角色限制") : TEXT(""), *Progress.Detail)
		: FString();
	ManualEvidenceText->SetText(FText::FromString(FString::Printf(
		TEXT("%s\n%s%s\n%s"), *Header,
		*AW11GameMode::GetManualEvidenceRoleInstruction(Role),
		ProgressLine.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("\n%s"), *ProgressLine),
		*Footer)));
	ManualEvidencePanel->SetBrushColor(bAccepted
		? FLinearColor(0.025f, 0.15f, 0.11f, 0.92f)
		: FLinearColor(0.28f, 0.035f, 0.025f, 0.94f));
	ManualEvidencePanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (!bManualEvidenceGuideLogged)
	{
		bManualEvidenceGuideLogged = true;
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_MANUAL_EVIDENCE_GUIDE_READY Surface=CombatHUD Role=%s Accepted=%d Reason=%s"),
			*Role.ToString(), bAccepted ? 1 : 0,
			RejectionReason.IsNone() ? TEXT("None") : *RejectionReason.ToString());
	}
}

void UW11CombatHUDWidget::BindRuntimeSources()
{
	AW11PlayerController* Controller = Cast<AW11PlayerController>(GetOwningPlayer());
	AW11PlayerState* PlayerState = Controller ? Controller->GetPlayerState<AW11PlayerState>() : nullptr;
	AW11Character* Character = Controller ? Cast<AW11Character>(Controller->GetPawn()) : nullptr;
	AW11GameState* GameState = GetWorld() ? GetWorld()->GetGameState<AW11GameState>() : nullptr;
	W11Controller = Controller;
	W11PlayerState = PlayerState;
	if (W11GameState.Get() != GameState)
	{
		if (W11GameState.IsValid())
		{
			W11GameState->OnRuntimeStateChanged.RemoveAll(this);
		}
		W11GameState = GameState;
		if (GameState)
		{
			GameState->OnRuntimeStateChanged.AddUniqueDynamic(
				this, &UW11CombatHUDWidget::HandleGameStateChanged);
		}
	}
	UW11AttributeComponent* NewAttributes = PlayerState ? PlayerState->GetAttributeComponent() : nullptr;
	if (Attributes.Get() != NewAttributes)
	{
		if (Attributes.IsValid())
		{
			Attributes->OnHealthChanged.RemoveAll(this);
			Attributes->OnManaChanged.RemoveAll(this);
			Attributes->OnBarrierChanged.RemoveAll(this);
			Attributes->OnStatusesChanged.RemoveAll(this);
		}
		Attributes = NewAttributes;
		if (Attributes.IsValid())
		{
			Attributes->OnHealthChanged.AddUniqueDynamic(this, &UW11CombatHUDWidget::HandleHealthChanged);
			Attributes->OnManaChanged.AddUniqueDynamic(this, &UW11CombatHUDWidget::HandleManaChanged);
			Attributes->OnBarrierChanged.AddUniqueDynamic(this, &UW11CombatHUDWidget::HandleBarrierChanged);
			Attributes->OnStatusesChanged.AddUniqueDynamic(this, &UW11CombatHUDWidget::HandleStatusesChanged);
		}
	}
	if (W11Character.Get() != Character)
	{
		if (W11Character.IsValid())
		{
			W11Character->OnIncomingCombatCue.RemoveAll(this);
			W11Character->OnDodgeCooldownChanged.RemoveAll(this);
		}
		W11Character = Character;
		if (Character)
		{
			Character->OnIncomingCombatCue.AddUniqueDynamic(
				this, &UW11CombatHUDWidget::HandleIncomingCombatCue);
			Character->OnDodgeCooldownChanged.AddUniqueDynamic(
				this, &UW11CombatHUDWidget::HandleDodgeCooldownChanged);
		}
	}
	UW11CombatComponent* NewCombat = Character ? Character->GetCombatComponent() : nullptr;
	if (Combat.Get() != NewCombat)
	{
		if (Combat.IsValid())
		{
			Combat->OnAbilityResult.RemoveAll(this);
			Combat->OnCooldownChanged.RemoveAll(this);
		}
		Combat = NewCombat;
		if (Combat.IsValid())
		{
			Combat->OnAbilityResult.AddUniqueDynamic(this, &UW11CombatHUDWidget::HandleAbilityResult);
			Combat->OnCooldownChanged.AddUniqueDynamic(this, &UW11CombatHUDWidget::HandleCooldownChanged);
		}
	}
}

void UW11CombatHUDWidget::RefreshVitals()
{
	if (!Attributes.IsValid())
	{
		return;
	}
	HandleHealthChanged(Attributes->GetHealth(), Attributes->GetStats().MaxHealth);
	HandleManaChanged(Attributes->GetMana(), Attributes->GetStats().MaxMana);
	HandleBarrierChanged(Attributes->GetBarrier());
}

void UW11CombatHUDWidget::RefreshCombatState()
{
	if (!W11GameState.IsValid())
	{
		return;
	}
	const EW11RunPhase Phase = W11GameState->GetRunPhase();
	// Keep the root tickable while the replicated run phase advances out of Lobby.
	// A Collapsed root cannot tick itself back to Visible after the phase changes.
	SetVisibility(ESlateVisibility::Visible);
	SetRenderOpacity(Phase == EW11RunPhase::Lobby ? 0.0f : 1.0f);
	FString Objective;
	if (Phase == EW11RunPhase::ChapterTransition)
	{
		const TCHAR* NodeLabel = W11GameState->GetChapterNodeType() == EW11ChapterNodeType::Rest
			? TEXT("休整") : TEXT("事件");
		ObjectiveText->SetText(FText::FromString(FString::Printf(
			TEXT("第 %d 章 · %s　按整备键继续"),
			W11GameState->GetChapterIndex() + 1, NodeLabel)));
		ResultsOverlay->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	if (Phase == EW11RunPhase::ImmortalMarket)
	{
		ObjectiveText->SetText(FText::FromString(FString::Printf(
			TEXT("第 %d 章 · 仙坊　购买后按整备键继续"),
			W11GameState->GetChapterIndex() + 1)));
		ResultsOverlay->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	if (Phase == EW11RunPhase::Cultivation)
	{
		ObjectiveText->SetText(FText::FromString(FString::Printf(
			TEXT("第 %d 章 · 修为精进"), W11GameState->GetChapterIndex() + 1)));
		ResultsOverlay->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	switch (W11GameState->GetEncounterType())
	{
	case EW11EncounterType::Waves:
		Objective = FString::Printf(TEXT("波次 %d/%d　余敌 %d"), W11GameState->GetCurrentWave(),
			W11GameState->GetWaveCount(), W11GameState->GetRemainingEnemyCount());
		break;
	case EW11EncounterType::Survival:
		Objective = FString::Printf(TEXT("坚守 %.1fs　当前威胁 %d"),
			W11GameState->GetObjectiveTimeRemaining(), W11GameState->GetRemainingEnemyCount());
		break;
	case EW11EncounterType::Elite:
		Objective = FString::Printf(TEXT("精英试炼　余敌 %d"), W11GameState->GetRemainingEnemyCount());
		break;
	case EW11EncounterType::Boss:
		Objective = FString::Printf(TEXT("镇关之战　余敌 %d"), W11GameState->GetRemainingEnemyCount());
		break;
	default:
		Objective = FString::Printf(TEXT("荡清妖邪　余敌 %d"), W11GameState->GetRemainingEnemyCount());
		break;
	}
	ObjectiveText->SetText(FText::FromString(FString::Printf(
		TEXT("第 %d 重试炼　%s"), W11GameState->GetStageIndex(), *Objective)));
	const bool bDefeat = Phase == EW11RunPhase::Results
		&& W11GameState->GetCombatOutcome() == EW11CombatOutcome::Defeat;
	ResultsOverlay->SetVisibility(bDefeat ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (Phase != EW11RunPhase::Combat && W11GameState->GetCombatOutcome() == EW11CombatOutcome::Victory)
	{
		StatusText->SetText(FText::FromString(TEXT("试炼已破")));
	}
}

void UW11CombatHUDWidget::RefreshCooldowns()
{
	if (!Combat.IsValid() || !W11Character.IsValid() || !W11GameState.IsValid())
	{
		return;
	}
	const float Now = W11GameState->GetServerWorldTimeSeconds();
	const float Basic = FMath::Max(0.0f, Combat->GetBasicAttackCooldownEndTime() - Now);
	const float Dodge = FMath::Max(0.0f, W11Character->GetDodgeCooldownEndTime() - Now);
	const auto Label = [](const TCHAR* Name, const float Remaining)
	{
		return Remaining <= 0.01f ? FString::Printf(TEXT("%s 可用"), Name)
			: FString::Printf(TEXT("%s %.1fs"), Name, Remaining);
	};
	TArray<FString> AbilityLabels;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const EW11AbilitySlot AbilitySlot = static_cast<EW11AbilitySlot>(
			static_cast<int32>(EW11AbilitySlot::Active1) + Index);
		const FName AbilityId = W11PlayerState.IsValid()
			? W11PlayerState->GetActiveAbilityId(AbilitySlot) : NAME_None;
		FString ShortName = AbilityId.IsNone() ? TEXT("未装备") : AbilityId.ToString();
		ShortName.RemoveFromStart(TEXT("Ability."));
		const int32 Level = W11PlayerState.IsValid() ? W11PlayerState->GetActiveAbilityLevel(AbilitySlot) : 0;
		const float Remaining = FMath::Max(0.0f, Combat->GetAbilityCooldownEndTime(AbilitySlot) - Now);
		static const TCHAR* AbilityKeys[] = { TEXT("Q"), TEXT("W"), TEXT("E"), TEXT("R") };
		AbilityLabels.Add(FString::Printf(TEXT("%s:%s Lv%d %s"), AbilityKeys[Index], *ShortName,
			Level, Remaining <= 0.01f ? TEXT("可用") : *FString::Printf(TEXT("%.1fs"), Remaining)));
	}
	CooldownText->SetText(FText::FromString(FString::Printf(TEXT("%s　%s　%s　%s　%s　%s"),
		*Label(TEXT("F普攻"), Basic), *AbilityLabels[0], *AbilityLabels[1],
		*AbilityLabels[2], *AbilityLabels[3], *Label(TEXT("闪避"), Dodge))));

	TArray<FString> StatusLabels;
	if (Attributes.IsValid())
	{
		for (const FW11ActiveStatus& Status : Attributes->GetActiveStatuses())
		{
			const TCHAR* TypeName = Status.Type == EW11StatusType::Burn ? TEXT("灼烧") : TEXT("减速");
			StatusLabels.Add(FString::Printf(TEXT("%s×%d %.1fs"), TypeName, Status.Stacks,
				FMath::Max(0.0f, Status.EndServerTime - Now)));
		}
	}
	ActiveStatusText->SetText(FText::FromString(StatusLabels.IsEmpty()
		? TEXT("状态：无") : FString::Printf(TEXT("状态：%s"), *FString::Join(StatusLabels, TEXT("　")))));
}

void UW11CombatHUDWidget::RefreshTeamStatus()
{
	if (!TeamStatusPanel || !TeamStatusText || !W11GameState.IsValid())
	{
		return;
	}
	TArray<FString> Lines;
	for (APlayerState* BasePlayerState : W11GameState->PlayerArray)
	{
		AW11PlayerState* Teammate = Cast<AW11PlayerState>(BasePlayerState);
		if (!Teammate || Teammate == W11PlayerState.Get() || !Teammate->GetAttributeComponent())
		{
			continue;
		}
		const UW11AttributeComponent* TeamAttributes = Teammate->GetAttributeComponent();
		const bool bCanFight = !TeamAttributes->IsDefeated();
		Lines.Add(FString::Printf(TEXT("同修 %d　气血 %d/%d　%s%s"),
			Teammate->GetPlayerId(), FMath::RoundToInt(TeamAttributes->GetHealth()),
			FMath::RoundToInt(TeamAttributes->GetStats().MaxHealth),
			bCanFight ? TEXT("可战斗") : TEXT("不可战斗"),
			Teammate->IsReadyForNextStage() ? TEXT("　已整备") : TEXT("")));
	}
	TeamStatusText->SetText(FText::FromString(FString::Join(Lines, TEXT("\n"))));
	TeamStatusPanel->SetVisibility(Lines.IsEmpty()
		? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	if (FParse::Param(FCommandLine::Get(), TEXT("W11NetworkSmoke"))
		&& LastLoggedTeammateCount != Lines.Num())
	{
		LastLoggedTeammateCount = Lines.Num();
		UE_LOG(LogWorldWalkerW11, Display, TEXT("W11_HUD_TEAM_STATUS Teammates=%d"), Lines.Num());
	}
}

EW11FacingDirection UW11CombatHUDWidget::ResolveIncomingDirection(const FVector& SourceOffset)
{
	if (FMath::Abs(SourceOffset.Y) > FMath::Abs(SourceOffset.X))
	{
		return SourceOffset.Y >= 0.0f ? EW11FacingDirection::Right : EW11FacingDirection::Left;
	}
	return SourceOffset.X >= 0.0f ? EW11FacingDirection::Up : EW11FacingDirection::Down;
}

void UW11CombatHUDWidget::HandleHealthChanged(const float CurrentValue, const float MaxValue)
{
	HealthText->SetText(FText::FromString(FString::Printf(TEXT("气血　%d / %d"),
		FMath::RoundToInt(CurrentValue), FMath::RoundToInt(MaxValue))));
	HealthBar->SetPercent(MaxValue > 0.0f ? CurrentValue / MaxValue : 0.0f);
}

void UW11CombatHUDWidget::HandleManaChanged(const float CurrentValue, const float MaxValue)
{
	ManaText->SetText(FText::FromString(FString::Printf(TEXT("真元　%d / %d"),
		FMath::RoundToInt(CurrentValue), FMath::RoundToInt(MaxValue))));
	ManaBar->SetPercent(MaxValue > 0.0f ? CurrentValue / MaxValue : 0.0f);
}

void UW11CombatHUDWidget::HandleBarrierChanged(const float CurrentValue)
{
	const float MaxHealth = Attributes.IsValid() ? Attributes->GetStats().MaxHealth : 1.0f;
	BarrierText->SetText(FText::FromString(FString::Printf(TEXT("护障　%d"), FMath::RoundToInt(CurrentValue))));
	BarrierBar->SetPercent(FMath::Clamp(CurrentValue / FMath::Max(1.0f, MaxHealth), 0.0f, 1.0f));
}

void UW11CombatHUDWidget::HandleStatusesChanged()
{
	RefreshCooldowns();
}

void UW11CombatHUDWidget::HandleGameStateChanged()
{
	RefreshCombatState();
	RefreshTeamStatus();
	if (FParse::Param(FCommandLine::Get(), TEXT("W11NetworkSmoke")) && W11GameState.IsValid())
	{
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_HUD_RUNTIME_STATE_EVENT Phase=%d Stage=%d Encounter=%d Enemies=%d Outcome=%d"),
			static_cast<int32>(W11GameState->GetRunPhase()), W11GameState->GetStageIndex(),
			W11GameState->GetEncounterInstanceId(), W11GameState->GetRemainingEnemyCount(),
			static_cast<int32>(W11GameState->GetCombatOutcome()));
	}
}

void UW11CombatHUDWidget::HandleAbilityResult(const FW11AbilityExecutionResult& Result)
{
	if (Result.bAccepted)
	{
		for (const FW11EffectResult& Effect : Result.Effects)
		{
			if (Effect.Type == EW11EffectResultType::Mana && Effect.ActualAmount > 0.0f)
			{
				StatusText->SetText(FText::FromString(FString::Printf(
					TEXT("真元 +%d"), FMath::RoundToInt(Effect.ActualAmount))));
				StatusVisibleUntil = GetWorld() ? GetWorld()->GetTimeSeconds() + 1.5f : 0.0f;
				break;
			}
		}
		return;
	}
	const TCHAR* Message = TEXT("无法施展");
	switch (Result.RejectionReason)
	{
	case EW11AbilityRejectionReason::Cooldown: Message = TEXT("调息未毕"); break;
	case EW11AbilityRejectionReason::InsufficientMana: Message = TEXT("真元不足"); break;
	case EW11AbilityRejectionReason::NoLegalTarget: Message = TEXT("无合法目标"); break;
	case EW11AbilityRejectionReason::Defeated: Message = TEXT("已不可战斗"); break;
	case EW11AbilityRejectionReason::StaleEncounter: Message = TEXT("旧试炼请求已失效"); break;
	default: break;
	}
	StatusText->SetText(FText::FromString(Message));
	StatusVisibleUntil = GetWorld() ? GetWorld()->GetTimeSeconds() + 1.5f : 0.0f;
}

void UW11CombatHUDWidget::HandleCooldownChanged(
	const EW11AbilitySlot AbilitySlot,
	const float CooldownEndServerTime)
{
	(void)AbilitySlot;
	(void)CooldownEndServerTime;
	RefreshCooldowns();
}

void UW11CombatHUDWidget::HandleDodgeCooldownChanged(const float CooldownEndServerTime)
{
	RefreshCooldowns();
	if (FParse::Param(FCommandLine::Get(), TEXT("W11NetworkSmoke")))
	{
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_HUD_DODGE_COOLDOWN_EVENT EndServerTime=%.3f"), CooldownEndServerTime);
	}
}

void UW11CombatHUDWidget::HandleIncomingCombatCue(const FW11CombatCue& Cue)
{
	if (!DamageDirectionPanel || !DamageDirectionText || !W11Character.IsValid())
	{
		return;
	}
	FVector SourceOffset = Cue.Source
		? Cue.Source->GetActorLocation() - W11Character->GetActorLocation()
		: -FVector(Cue.Direction);
	SourceOffset.Z = 0.0f;
	if (SourceOffset.IsNearlyZero())
	{
		SourceOffset = -FVector(Cue.Direction);
	}
	const EW11FacingDirection Direction = ResolveIncomingDirection(SourceOffset);
	const TCHAR* Arrow = Direction == EW11FacingDirection::Right ? TEXT("→")
		: Direction == EW11FacingDirection::Left ? TEXT("←")
		: Direction == EW11FacingDirection::Up ? TEXT("↑") : TEXT("↓");
	const TCHAR* Side = Direction == EW11FacingDirection::Right ? TEXT("右侧")
		: Direction == EW11FacingDirection::Left ? TEXT("左侧")
		: Direction == EW11FacingDirection::Up ? TEXT("上方") : TEXT("下方");
	FString ResultText = Cue.Type == EW11CombatCueType::Immune
		? TEXT("免疫") : Cue.PrimaryAmount > 0.0f
			? FString::Printf(TEXT("气血 -%d"), FMath::RoundToInt(Cue.PrimaryAmount))
			: FString::Printf(TEXT("护障 -%d"), FMath::RoundToInt(Cue.SecondaryAmount));
	DamageDirectionText->SetText(FText::FromString(FString::Printf(
		TEXT("受击 %s %s　%s"), Arrow, Side, *ResultText)));
	DamageDirectionPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	DamageDirectionVisibleUntil = GetWorld() ? GetWorld()->GetTimeSeconds() + 1.0f : 0.0f;
	if (FParse::Param(FCommandLine::Get(), TEXT("W11NetworkSmoke")))
	{
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_HUD_DAMAGE_DIRECTION Side=%s Damage=%.1f Barrier=%.1f"),
			Side, Cue.PrimaryAmount, Cue.SecondaryAmount);
	}
}

void UW11CombatHUDWidget::HandleRestartClicked()
{
	if (W11Controller.IsValid())
	{
		W11Controller->RequestNewRun();
	}
}
