#include "UI/W11EntryMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Font.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Texture2D.h"
#include "Data/W11Definitions.h"
#include "Game/W11GameMode.h"
#include "Game/W11GameState.h"
#include "Game/W11PlayerController.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Online/W11SessionSubsystem.h"
#include "UnrealClient.h"
#include "WorldWalkerW11.h"

namespace W11EntryMenu
{
	static const TCHAR* BackgroundPath =
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Entry/T_W11_TitleBackground.T_W11_TitleBackground");
	static const TCHAR* FogPath =
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Entry/T_W11_TitleFog.T_W11_TitleFog");
	static const TCHAR* MenuPanelPath =
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Entry/T_W11_EntryMenuPanel.T_W11_EntryMenuPanel");
	static const TCHAR* TitleBackdropPath =
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Entry/T_W11_EntryTitleBackdrop.T_W11_EntryTitleBackdrop");
	static const TCHAR* HeroSelectionBackgroundPath =
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Entry/T_W11_HeroSelectionBackground.T_W11_HeroSelectionBackground");
	static const TCHAR* HeroSelectionPortraitAuraPath =
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Entry/T_W11_HeroSelectionPortraitAura.T_W11_HeroSelectionPortraitAura");
	static const TCHAR* CalligraphyFontPath =
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/UI/Fonts/FF_W11_MaShanZheng_Font.FF_W11_MaShanZheng_Font");
	static const TCHAR* RuleSetPath =
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Data/DA_W11_RunRules.DA_W11_RunRules");

	static void AddVertical(UVerticalBox* Box, UWidget* Child, const FMargin& Padding = FMargin(0.0f, 2.0f))
	{
		if (Box && Child)
		{
			Box->AddChildToVerticalBox(Child)->SetPadding(Padding);
		}
	}

	static void ApplyCalligraphyFont(UTextBlock* Text, const float Size, const int32 LetterSpacing)
	{
		if (!Text)
		{
			return;
		}

		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Font.LetterSpacing = LetterSpacing;
		if (UFont* CalligraphyFont = LoadObject<UFont>(nullptr, CalligraphyFontPath))
		{
			Font.FontObject = CalligraphyFont;
			Font.TypefaceFontName = TEXT("Default");
		}
		Text->SetFont(Font);
	}
}

void UW11EntryMenuWidget::Configure(AW11PlayerController* InController)
{
	W11Controller = InController;
}

TSharedRef<SWidget> UW11EntryMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}
	return Super::RebuildWidget();
}

void UW11EntryMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	const bool bOrnateMenuCapture = FParse::Param(FCommandLine::Get(), TEXT("W11OrnateMenuCapture"));
	bStoneHoverPreview = FParse::Param(FCommandLine::Get(), TEXT("W11StoneHoverPreview"));
	if (bOrnateMenuCapture || FParse::Param(FCommandLine::Get(), TEXT("W11EntryMenuCapture")))
	{
		AnimationTime = 1.0f;
		bDelayedScreenshotPending = bOrnateMenuCapture;
		SetPage(EEntryPage::Main);
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("W11HeroSelectionCapture")))
	{
		AnimationTime = 1.0f;
		bDelayedScreenshotPending = true;
		SetPage(EEntryPage::HeroSelect);
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("W11SectSelectionCapture")))
	{
		AnimationTime = 1.0f;
		bDelayedScreenshotPending = true;
		SetPage(EEntryPage::SectSelect);
	}
	else
	{
		SetPage(EEntryPage::Main);
	}
	RefreshSettings();

	if (!W11Controller)
	{
		W11Controller = Cast<AW11PlayerController>(GetOwningPlayer());
	}
	if (W11Controller && W11Controller->GetGameInstance())
	{
		if (UW11SessionSubsystem* Sessions = W11Controller->GetGameInstance()->GetSubsystem<UW11SessionSubsystem>())
		{
			Sessions->OnHostCompleted.AddDynamic(this, &UW11EntryMenuWidget::HandleHostCompleted);
			Sessions->OnSearchCompleted.AddDynamic(this, &UW11EntryMenuWidget::HandleSearchCompleted);
			Sessions->OnJoinCompleted.AddDynamic(this, &UW11EntryMenuWidget::HandleJoinCompleted);
			bDelegatesBound = true;
		}
	}
}

void UW11EntryMenuWidget::NativeDestruct()
{
	if (bDelegatesBound && W11Controller && W11Controller->GetGameInstance())
	{
		if (UW11SessionSubsystem* Sessions = W11Controller->GetGameInstance()->GetSubsystem<UW11SessionSubsystem>())
		{
			Sessions->OnHostCompleted.RemoveDynamic(this, &UW11EntryMenuWidget::HandleHostCompleted);
			Sessions->OnSearchCompleted.RemoveDynamic(this, &UW11EntryMenuWidget::HandleSearchCompleted);
			Sessions->OnJoinCompleted.RemoveDynamic(this, &UW11EntryMenuWidget::HandleJoinCompleted);
		}
	}
	bDelegatesBound = false;
	Super::NativeDestruct();
}

void UW11EntryMenuWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AnimationTime += InDeltaTime;
	if (bDelayedScreenshotPending && AnimationTime >= 1.35f)
	{
		bDelayedScreenshotPending = false;
		FScreenshotRequest::RequestScreenshot(true);
		UE_LOG(LogTemp, Display, TEXT("W11_ORNATE_MENU_SCREENSHOT_REQUESTED"));
	}

	if (BackgroundImage)
	{
		if (ActivePage == EEntryPage::HeroSelect || ActivePage == EEntryPage::SectSelect)
		{
			// Character-creation pages use authored full-screen compositions. Keep them
			// still so neither the hero staging nor the selected sect artwork drifts or crops.
			BackgroundImage->SetRenderScale(FVector2D(1.0f));
			BackgroundImage->SetRenderTranslation(FVector2D::ZeroVector);
		}
		else
		{
			const float Pulse = 1.045f + FMath::Sin(AnimationTime * 0.13f) * 0.008f;
			BackgroundImage->SetRenderScale(FVector2D(Pulse));
			BackgroundImage->SetRenderTranslation(FVector2D(
				FMath::Sin(AnimationTime * 0.09f) * 13.0f,
				FMath::Cos(AnimationTime * 0.07f) * 7.0f));
		}
	}
	if (FogImageA)
	{
		FogImageA->SetRenderTranslation(FVector2D(FMath::Fmod(AnimationTime * 14.0f, 900.0f) - 450.0f, 18.0f));
		FogImageA->SetRenderOpacity(0.31f + FMath::Sin(AnimationTime * 0.21f) * 0.06f);
	}
	if (FogImageB)
	{
		FogImageB->SetRenderTranslation(FVector2D(420.0f - FMath::Fmod(AnimationTime * 9.0f, 840.0f), -38.0f));
		FogImageB->SetRenderOpacity(0.20f + FMath::Cos(AnimationTime * 0.17f) * 0.05f);
	}
	for (int32 Index = 0; Index < SpiritMotes.Num(); ++Index)
	{
		if (UBorder* Mote = SpiritMotes[Index])
		{
			const float Phase = AnimationTime * (0.22f + Index * 0.013f) + Index * 1.71f;
			Mote->SetRenderTranslation(FVector2D(FMath::Sin(Phase) * 24.0f, -FMath::Fmod(AnimationTime * (7.0f + Index), 180.0f)));
			Mote->SetRenderOpacity(0.22f + 0.34f * FMath::Square(FMath::Sin(Phase * 0.71f)));
		}
	}
	if (MenuChrome)
	{
		MenuChrome->SetRenderOpacity(FMath::Clamp(AnimationTime / 0.75f, 0.0f, 1.0f));
	}

	if (ActivePage == EEntryPage::HeroSelect)
	{
		// The portrait is deliberately static. All ambient life belongs to the aura behind it,
		// so the source art can never pulse, drift, rotate or acquire a page-game squash.
		if (HeroPortraitAuraBase)
		{
			// A shared master fade makes the whole aura visibly approach disappearance.
			// The slower secondary wave keeps the broad foundation from feeling mechanical.
			const float MasterFade = 0.5f + 0.5f * FMath::Sin(AnimationTime * 1.25f);
			const float AuraDrift = 0.5f + 0.5f * FMath::Sin(AnimationTime * 0.43f + 0.8f);
			const float SmoothFade = MasterFade * MasterFade * (3.0f - 2.0f * MasterFade);
			HeroPortraitAuraBase->SetRenderOpacity(
				0.02f + 0.88f * SmoothFade * FMath::Lerp(0.72f, 1.0f, AuraDrift));
			HeroPortraitAuraBase->SetRenderScale(FVector2D(1.0f));
			HeroPortraitAuraBase->SetRenderTransformAngle(0.0f);
		}
		if (HeroPortraitAuraBloom)
		{
			// The bloom uses short, sharper flashes at a different frequency. Multiplying
			// by the master fade prevents this layer from masking the foundation's fade-out.
			const float MasterFade = 0.5f + 0.5f * FMath::Sin(AnimationTime * 1.25f);
			const float BloomWave = 0.5f + 0.5f * FMath::Sin(AnimationTime * 2.05f + 0.8f);
			const float BloomFlash = FMath::Square(FMath::Square(BloomWave));
			HeroPortraitAuraBloom->SetRenderOpacity(
				0.01f + 0.99f * BloomFlash * FMath::Lerp(0.12f, 1.0f, MasterFade));
			HeroPortraitAuraBloom->SetRenderScale(FVector2D(1.035f));
			HeroPortraitAuraBloom->SetRenderTranslation(FVector2D::ZeroVector);
		}

		for (int32 Index = 0; Index < HeroButtons.Num(); ++Index)
		{
			const bool bSelected = Index == SelectedHeroIndex;
			const bool bHovered = HeroButtons[Index] && HeroButtons[Index]->IsHovered();
			if (HeroAvatarImages.IsValidIndex(Index) && HeroAvatarImages[Index])
			{
				const float TargetScale = bSelected ? 1.075f : bHovered ? 1.035f : 0.965f;
				HeroAvatarImages[Index]->SetRenderScale(FVector2D(TargetScale));
				HeroAvatarImages[Index]->SetColorAndOpacity(bSelected
					? FLinearColor(0.92f, 1.0f, 0.98f, 1.0f)
					: bHovered ? FLinearColor(0.75f, 0.90f, 0.87f, 0.96f)
					: FLinearColor(0.43f, 0.51f, 0.50f, 0.74f));
				HeroAvatarImages[Index]->SetRenderTranslation(FVector2D(0.0f, bSelected ? -2.0f : 0.0f));
			}
		}
	}

	// The stone tablet is the only button background. Labels sit "inside" it at rest, then
	// rise into the same cyan-white spirit light as the fissures in the world art on hover.
	const FLinearColor CarvedFace(0.30f, 0.35f, 0.33f, 0.98f);
	const FLinearColor CarvedEdge(0.57f, 0.61f, 0.53f, 0.58f);
	const FLinearColor SpiritFace(0.72f, 1.0f, 0.96f, 1.0f);
	const int32 StoneButtonCount = FMath::Min3(
		StoneMenuButtons.Num(), StoneMenuLabels.Num(), StoneMenuHoverAmounts.Num());
	for (int32 Index = 0; Index < StoneButtonCount; ++Index)
	{
		UButton* Button = StoneMenuButtons[Index];
		UTextBlock* Label = StoneMenuLabels[Index];
		if (!Button || !Label)
		{
			continue;
		}

		const bool bPreviewSolo = bStoneHoverPreview && Button->GetFName() == TEXT("SoloButton");
		const bool bLit = Button->GetIsEnabled() && (Button->IsHovered() || bPreviewSolo);
		StoneMenuHoverAmounts[Index] = FMath::FInterpTo(
			StoneMenuHoverAmounts[Index], bLit ? 1.0f : 0.0f, InDeltaTime, bLit ? 13.0f : 9.0f);
		const float HoverAmount = StoneMenuHoverAmounts[Index];

		if (!Button->GetIsEnabled())
		{
			Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.17f, 0.20f, 0.19f, 0.76f)));
			Label->SetShadowColorAndOpacity(FLinearColor(0.38f, 0.41f, 0.37f, 0.35f));
			Label->SetShadowOffset(FVector2D(1.0f, 1.0f));
		}
		else
		{
			Label->SetColorAndOpacity(FSlateColor(FMath::Lerp(CarvedFace, SpiritFace, HoverAmount)));
			// A cyan offset shadow reads as a duplicate glyph at menu scale. Keep the luminous
			// face colour and lift animation, but remove the carving edge as soon as it is lit.
			Label->SetShadowColorAndOpacity(bLit ? FLinearColor::Transparent : CarvedEdge);
			Label->SetShadowOffset(bLit ? FVector2D::ZeroVector : FVector2D(1.0f, 1.0f));
		}
		Label->SetRenderScale(FVector2D(FMath::Lerp(1.0f, 1.045f, HoverAmount)));
		Label->SetRenderTranslation(FVector2D(0.0f, FMath::Lerp(0.0f, -1.5f, HoverAmount)));
	}

	const AW11GameState* State = GetWorld() ? GetWorld()->GetGameState<AW11GameState>() : nullptr;
	if (State && State->GetRunPhase() != EW11RunPhase::Lobby && W11Controller)
	{
		W11Controller->HideEntryMenu();
	}
}

FReply UW11EntryMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (ActivePage == EEntryPage::SectSelect && !SectDefinitions.IsEmpty())
	{
		if (InKeyEvent.GetKey() == EKeys::Left)
		{
			SelectSect((SelectedSectIndex - 1 + SectDefinitions.Num()) % SectDefinitions.Num());
			return FReply::Handled();
		}
		if (InKeyEvent.GetKey() == EKeys::Right)
		{
			SelectSect((SelectedSectIndex + 1) % SectDefinitions.Num());
			return FReply::Handled();
		}
		if (InKeyEvent.GetKey() == EKeys::Enter)
		{
			HandleConfirmJourneyClicked();
			return FReply::Handled();
		}
	}
	if (InKeyEvent.GetKey() == EKeys::Escape && ActivePage != EEntryPage::Main)
	{
		SetPage(ActivePage == EEntryPage::SectSelect ? EEntryPage::HeroSelect : EEntryPage::Main);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

UTextBlock* UW11EntryMenuWidget::MakeText(
	const TCHAR* Name,
	const FString& Text,
	const int32 FontSize,
	const FLinearColor& Color) const
{
	UTextBlock* Result = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(Name));
	Result->SetText(FText::FromString(Text));
	Result->SetColorAndOpacity(FSlateColor(Color));
	Result->SetAutoWrapText(true);
	Result->SetShadowOffset(FVector2D(1.0f, 2.0f));
	Result->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f));
	FSlateFontInfo Font = Result->GetFont();
	Font.Size = FontSize;
	Result->SetFont(Font);
	return Result;
}

UButton* UW11EntryMenuWidget::MakeMenuButton(
	const TCHAR* Name,
	const FString& Label,
	UTextBlock*& OutLabel,
	const FLinearColor& Tint)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
	FButtonStyle ButtonStyle = Button->GetStyle();
	(void)Tint;
	const auto MakeTransparentBrush = [](FSlateBrush& Brush)
	{
		// The button remains a full-size hit target; the shared stone tablet is its only background.
		Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
		Brush.SetResourceObject(nullptr);
		Brush.TintColor = FSlateColor(FLinearColor::Transparent);
		Brush.OutlineSettings = FSlateBrushOutlineSettings();
	};
	MakeTransparentBrush(ButtonStyle.Normal);
	MakeTransparentBrush(ButtonStyle.Hovered);
	MakeTransparentBrush(ButtonStyle.Pressed);
	MakeTransparentBrush(ButtonStyle.Disabled);
	ButtonStyle.NormalPadding = FMargin(0.0f);
	ButtonStyle.PressedPadding = FMargin(0.0f);
	Button->SetStyle(ButtonStyle);
	Button->SetBackgroundColor(FLinearColor::Transparent);
	Button->SetColorAndOpacity(FLinearColor::White);
	Button->SetTouchMethod(EButtonTouchMethod::PreciseTap);

	USizeBox* ContentSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), *FString(Name).Append(TEXT("ContentSize")));
	ContentSize->SetMinDesiredWidth(410.0f);
	ContentSize->SetHeightOverride(48.0f);
	OutLabel = MakeText(*FString(Name).Append(TEXT("Label")), Label, 24,
		FLinearColor(0.30f, 0.35f, 0.33f, 0.98f));
	W11EntryMenu::ApplyCalligraphyFont(OutLabel, 24.0f, 70);
	OutLabel->SetAutoWrapText(false);
	OutLabel->SetMinDesiredWidth(330.0f);
	OutLabel->SetJustification(ETextJustify::Center);
	OutLabel->SetMargin(FMargin(34.0f, 5.0f));
	OutLabel->SetShadowOffset(FVector2D(1.0f, 1.0f));
	OutLabel->SetShadowColorAndOpacity(FLinearColor(0.57f, 0.61f, 0.53f, 0.58f));
	OutLabel->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	ContentSize->AddChild(OutLabel);
	Button->AddChild(ContentSize);
	StoneMenuButtons.Add(Button);
	StoneMenuLabels.Add(OutLabel);
	StoneMenuHoverAmounts.Add(0.0f);
	return Button;
}

UBorder* UW11EntryMenuWidget::MakePagePanel(const TCHAR* Name) const
{
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(Name));
	if (UTexture2D* PanelTexture = LoadObject<UTexture2D>(nullptr, W11EntryMenu::MenuPanelPath))
	{
		Panel->SetBrushFromTexture(PanelTexture);
		Panel->SetBrushColor(FLinearColor(0.78f, 0.84f, 0.82f, 0.94f));
	}
	else
	{
		Panel->SetBrushColor(FLinearColor(0.012f, 0.035f, 0.048f, 0.79f));
	}
	Panel->SetPadding(FMargin(24.0f, 28.0f));
	return Panel;
}

void UW11EntryMenuWidget::BuildWidgetTree()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("W11EntryRoot"));
	WidgetTree->RootWidget = Root;

	BackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TitleBackground"));
	if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, W11EntryMenu::BackgroundPath))
	{
		BackgroundImage->SetBrushFromTexture(Texture, true);
	}
	BackgroundImage->SetRenderTransformPivot(FVector2D(0.5f));
	UCanvasPanelSlot* BackgroundSlot = Root->AddChildToCanvas(BackgroundImage);
	BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackgroundSlot->SetOffsets(FMargin(-34.0f));

	UTexture2D* FogTexture = LoadObject<UTexture2D>(nullptr, W11EntryMenu::FogPath);
	FogImageA = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TitleFogA"));
	FogImageB = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TitleFogB"));
	if (FogTexture)
	{
		FogImageA->SetBrushFromTexture(FogTexture, true);
		FogImageB->SetBrushFromTexture(FogTexture, true);
	}
	FogImageB->SetRenderScale(FVector2D(-1.18f, 1.28f));
	for (UImage* Fog : { FogImageA.Get(), FogImageB.Get() })
	{
		UCanvasPanelSlot* FogSlot = Root->AddChildToCanvas(Fog);
		FogSlot->SetAnchors(FAnchors(0.0f, 0.42f, 1.0f, 1.0f));
		FogSlot->SetOffsets(FMargin(-360.0f, 0.0f, -360.0f, -10.0f));
	}

	for (int32 Index = 0; Index < 18; ++Index)
	{
		UBorder* Mote = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),
			FName(*FString::Printf(TEXT("SpiritMote%d"), Index)));
		Mote->SetBrushColor(Index % 4 == 0
			? FLinearColor(0.90f, 0.66f, 0.27f, 0.65f)
			: FLinearColor(0.22f, 0.93f, 0.86f, 0.58f));
		UCanvasPanelSlot* MoteSlot = Root->AddChildToCanvas(Mote);
		const float X = 0.08f + FMath::Fmod(Index * 0.173f, 0.84f);
		const float Y = 0.18f + FMath::Fmod(Index * 0.127f, 0.70f);
		MoteSlot->SetAnchors(FAnchors(X, Y));
		const float Size = 2.0f + static_cast<float>(Index % 4) * 1.5f;
		MoteSlot->SetOffsets(FMargin(0.0f, 0.0f, Size, Size));
		SpiritMotes.Add(Mote);
	}

	UBorder* Vignette = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuVignette"));
	Vignette->SetBrushColor(FLinearColor(0.003f, 0.012f, 0.02f, 0.20f));
	Vignette->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* VignetteSlot = Root->AddChildToCanvas(Vignette);
	VignetteSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	VignetteSlot->SetOffsets(FMargin(0.0f));

	MenuChrome = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuChrome"));
	// This border is layout-only. The title plaque and menu panel provide their own silhouettes;
	// keeping a tinted brush here would create an unwanted rectangular shadow behind both.
	MenuChrome->SetBrushColor(FLinearColor::Transparent);
	MenuChrome->SetPadding(FMargin(32.0f, 22.0f));
	MenuSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MenuSize"));
	MenuSizeBox->SetWidthOverride(526.0f);
	MenuSizeBox->SetHeightOverride(730.0f);
	MenuSizeBox->AddChild(MenuChrome);
	UCanvasPanelSlot* MenuSlot = Root->AddChildToCanvas(MenuSizeBox);
	MenuSlot->SetAnchors(FAnchors(0.04f, 0.50f));
	MenuSlot->SetAlignment(FVector2D(0.0f, 0.5f));
	MenuSlot->SetAutoSize(true);

	UVerticalBox* ChromeBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ChromeBox"));
	MenuChrome->SetContent(ChromeBox);

	// The title uses an irregular transparent ink-cloud backdrop, never a panel or frame.
	TitlePlaqueSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TitlePlaqueSize"));
	TitlePlaqueSizeBox->SetWidthOverride(462.0f);
	TitlePlaqueSizeBox->SetHeightOverride(154.0f);
	TitlePlaqueSizeBox->SetRenderTranslation(FVector2D(0.0f, -44.0f));
	UOverlay* TitleOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("TitleBackdropOverlay"));
	TitlePlaqueSizeBox->AddChild(TitleOverlay);
	if (UTexture2D* BackdropTexture = LoadObject<UTexture2D>(nullptr, W11EntryMenu::TitleBackdropPath))
	{
		UImage* TitleBackdrop = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TitleBackdrop"));
		TitleBackdrop->SetBrushFromTexture(BackdropTexture, false);
		TitleBackdrop->SetColorAndOpacity(FLinearColor(0.66f, 0.82f, 0.84f, 0.58f));
		TitleBackdrop->SetVisibility(ESlateVisibility::HitTestInvisible);
		UOverlaySlot* BackdropSlot = TitleOverlay->AddChildToOverlay(TitleBackdrop);
		BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
		BackdropSlot->SetVerticalAlignment(VAlign_Fill);
	}
	UVerticalBox* TitleTextBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TitlePlaqueText"));
	UTextBlock* Title = MakeText(TEXT("Title"), TEXT("太虚问道"), 64,
		FLinearColor(0.94f, 0.89f, 0.72f, 1.0f));
	W11EntryMenu::ApplyCalligraphyFont(Title, 64.0f, 110);
	Title->SetAutoWrapText(false);
	Title->SetJustification(ETextJustify::Center);
	W11EntryMenu::AddVertical(TitleTextBox, Title, FMargin(24.0f, 0.0f, 24.0f, 0.0f));
	UTextBlock* Subtitle = MakeText(TEXT("Subtitle"), TEXT("一念入太虚，万法皆由心"), 17,
		FLinearColor(0.68f, 0.80f, 0.80f, 1.0f));
	Subtitle->SetAutoWrapText(false);
	Subtitle->SetJustification(ETextJustify::Center);
	W11EntryMenu::AddVertical(TitleTextBox, Subtitle, FMargin(24.0f, 0.0f, 24.0f, 0.0f));
	UOverlaySlot* TitleTextSlot = TitleOverlay->AddChildToOverlay(TitleTextBox);
	TitleTextSlot->SetHorizontalAlignment(HAlign_Fill);
	TitleTextSlot->SetVerticalAlignment(VAlign_Center);
	W11EntryMenu::AddVertical(ChromeBox, TitlePlaqueSizeBox, FMargin(0.0f, 0.0f, 0.0f, 2.0f));

	PageSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("EntryPages"));
	PageSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("EntryPagesSize"));
	PageSizeBox->SetWidthOverride(462.0f);
	// Preserve breathing room around both the first and eighth entries while keeping the
	// eight 48-unit hit targets and their spacing unchanged.
	PageSizeBox->SetHeightOverride(526.0f);
	PageSizeBox->AddChild(PageSwitcher);
	W11EntryMenu::AddVertical(ChromeBox, PageSizeBox, FMargin(0.0f));
	ChromeBox->SetIsEnabled(true);

	// Main page.
	UBorder* MainPanel = MakePagePanel(TEXT("MainPage"));
	MainPanel->SetPadding(FMargin(24.0f, 44.0f, 24.0f, 28.0f));
	UVerticalBox* MainBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainButtons"));
	MainPanel->SetContent(MainBox);
	UTextBlock* Label = nullptr;
	UTextBlock* ContinueLabelWidget = nullptr;
	ContinueButton = MakeMenuButton(TEXT("ContinueButton"), TEXT("继续问道　暂无存档"), ContinueLabelWidget,
		FLinearColor(0.022f, 0.070f, 0.068f, 0.82f));
	ContinueLabel = ContinueLabelWidget;
	const bool bCanResume = W11Controller && W11Controller->CanResumeSoloRun();
	ContinueButton->SetIsEnabled(bCanResume);
	ContinueLabel->SetText(FText::FromString(
		bCanResume ? TEXT("继续问道　安全节点") : TEXT("继续问道　暂无存档")));
	ContinueButton->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleContinueClicked);
	W11EntryMenu::AddVertical(MainBox, ContinueButton);
	UButton* Solo = MakeMenuButton(TEXT("SoloButton"), TEXT("独自启程"), Label,
		FLinearColor(0.030f, 0.225f, 0.185f, 0.97f));
	Solo->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleSoloClicked);
	W11EntryMenu::AddVertical(MainBox, Solo);
	UButton* Coop = MakeMenuButton(TEXT("CoopButton"), TEXT("联机共修"), Label);
	Coop->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleCoopClicked);
	W11EntryMenu::AddVertical(MainBox, Coop);
	UButton* Compendium = MakeMenuButton(TEXT("CompendiumButton"), TEXT("万法图鉴"), Label);
	Compendium->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleCompendiumClicked);
	W11EntryMenu::AddVertical(MainBox, Compendium);
	UButton* Settings = MakeMenuButton(TEXT("SettingsButton"), TEXT("修行设置"), Label);
	Settings->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleSettingsClicked);
	W11EntryMenu::AddVertical(MainBox, Settings);
	UButton* Credits = MakeMenuButton(TEXT("CreditsButton"), TEXT("制作名单"), Label);
	Credits->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleCreditsClicked);
	W11EntryMenu::AddVertical(MainBox, Credits);
	UButton* ReturnWorld = MakeMenuButton(TEXT("ReturnWorldButton"), TEXT("返回诸界"), Label,
		FLinearColor(0.030f, 0.075f, 0.105f, 0.94f));
	ReturnWorld->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleReturnWorldClicked);
	W11EntryMenu::AddVertical(MainBox, ReturnWorld);
	UButton* Quit = MakeMenuButton(TEXT("QuitButton"), TEXT("退出游戏"), Label,
		FLinearColor(0.115f, 0.048f, 0.040f, 0.94f));
	Quit->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleQuitClicked);
	W11EntryMenu::AddVertical(MainBox, Quit);
	PageSwitcher->AddChild(MainPanel);

	LoadCharacterCreationDefinitions();
	BuildHeroSelectionPage();
	BuildSectSelectionPage();

	// Co-op page.
	UBorder* CoopPanel = MakePagePanel(TEXT("CoopPage"));
	UVerticalBox* CoopBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CoopBox"));
	CoopPanel->SetContent(CoopBox);
	W11EntryMenu::AddVertical(CoopBox, MakeText(TEXT("CoopHeading"), TEXT("联机共修"), 28,
		FLinearColor(0.56f, 0.96f, 0.90f, 1.0f)), FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	W11EntryMenu::AddVertical(CoopBox, MakeText(TEXT("CoopHelp"),
		TEXT("创建最多 4 人的 PvE 旅途，或寻找同道加入。房主掌握敌人、奖励与随机种子。"), 16,
		FLinearColor(0.70f, 0.78f, 0.80f, 1.0f)), FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	UButton* Host = MakeMenuButton(TEXT("HostButton"), TEXT("创建共修房间"), Label,
		FLinearColor(0.05f, 0.28f, 0.28f, 0.97f));
	Host->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleHostClicked);
	W11EntryMenu::AddVertical(CoopBox, Host);
	UButton* Find = MakeMenuButton(TEXT("FindButton"), TEXT("刷新可加入房间"), Label);
	Find->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleFindClicked);
	W11EntryMenu::AddVertical(CoopBox, Find);
	SessionResultsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SessionResults"));
	W11EntryMenu::AddVertical(CoopBox, SessionResultsBox, FMargin(0.0f, 8.0f));
	for (int32 Index = 0; Index < 4; ++Index)
	{
		UTextBlock* SessionLabel = nullptr;
		UButton* SessionButton = MakeMenuButton(
			*FString::Printf(TEXT("SessionButton%d"), Index), TEXT("等待搜索……"), SessionLabel,
			FLinearColor(0.04f, 0.12f, 0.15f, 0.95f));
		SessionButton->SetVisibility(ESlateVisibility::Collapsed);
		switch (Index)
		{
		case 0: SessionButton->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleJoin0Clicked); break;
		case 1: SessionButton->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleJoin1Clicked); break;
		case 2: SessionButton->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleJoin2Clicked); break;
		case 3: SessionButton->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleJoin3Clicked); break;
		default: break;
		}
		W11EntryMenu::AddVertical(SessionResultsBox, SessionButton, FMargin(0.0f, 3.0f));
		SessionButtons.Add(SessionButton);
		SessionLabels.Add(SessionLabel);
	}
	StatusText = MakeText(TEXT("OnlineStatus"), TEXT("尚未搜索房间"), 14,
		FLinearColor(0.62f, 0.74f, 0.75f, 1.0f));
	W11EntryMenu::AddVertical(CoopBox, StatusText, FMargin(3.0f, 8.0f));
	UButton* CoopBack = MakeMenuButton(TEXT("CoopBack"), TEXT("返回"), Label,
		FLinearColor(0.10f, 0.12f, 0.16f, 0.94f));
	CoopBack->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleBackClicked);
	W11EntryMenu::AddVertical(CoopBox, CoopBack, FMargin(0.0f, 10.0f, 0.0f, 0.0f));
	PageSwitcher->AddChild(CoopPanel);

	// Settings page.
	UBorder* SettingsPanel = MakePagePanel(TEXT("SettingsPage"));
	UVerticalBox* SettingsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsBox"));
	SettingsPanel->SetContent(SettingsBox);
	W11EntryMenu::AddVertical(SettingsBox, MakeText(TEXT("SettingsHeading"), TEXT("修行设置"), 28,
		FLinearColor(0.56f, 0.96f, 0.90f, 1.0f)), FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	W11EntryMenu::AddVertical(SettingsBox, MakeText(TEXT("MusicLabel"), TEXT("背景音乐"), 17));
	MusicSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("MusicVolume"));
	MusicSlider->SetMinValue(0.0f);
	MusicSlider->SetMaxValue(1.0f);
	MusicSlider->SetStepSize(0.05f);
	MusicSlider->SetSliderBarColor(FLinearColor(0.15f, 0.52f, 0.50f, 1.0f));
	MusicSlider->SetSliderHandleColor(FLinearColor(0.82f, 0.91f, 0.76f, 1.0f));
	MusicSlider->OnValueChanged.AddDynamic(this, &UW11EntryMenuWidget::HandleMusicVolumeChanged);
	W11EntryMenu::AddVertical(SettingsBox, MusicSlider, FMargin(0.0f, 2.0f, 0.0f, 16.0f));
	UHorizontalBox* VSyncRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("VSyncRow"));
	VSyncCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("VSyncCheck"));
	VSyncCheckBox->OnCheckStateChanged.AddDynamic(this, &UW11EntryMenuWidget::HandleVSyncChanged);
	VSyncRow->AddChildToHorizontalBox(VSyncCheckBox)->SetPadding(FMargin(0.0f, 4.0f, 12.0f, 4.0f));
	VSyncRow->AddChildToHorizontalBox(MakeText(TEXT("VSyncLabel"), TEXT("垂直同步"), 17));
	W11EntryMenu::AddVertical(SettingsBox, VSyncRow, FMargin(0.0f, 3.0f, 0.0f, 10.0f));
	UButton* WindowMode = MakeMenuButton(TEXT("WindowModeButton"), TEXT("显示模式"), Label);
	WindowModeText = Label;
	WindowMode->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleWindowModeClicked);
	W11EntryMenu::AddVertical(SettingsBox, WindowMode);
	UButton* Apply = MakeMenuButton(TEXT("ApplySettingsButton"), TEXT("应用设置"), Label,
		FLinearColor(0.05f, 0.28f, 0.28f, 0.97f));
	Apply->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleApplySettingsClicked);
	W11EntryMenu::AddVertical(SettingsBox, Apply, FMargin(0.0f, 14.0f, 0.0f, 4.0f));
	W11EntryMenu::AddVertical(SettingsBox, MakeText(TEXT("ControlHint"),
		TEXT("战斗操作：方向键移动 · F 普攻 · Q/W/E/R 四技能 · Space 闪避 · Enter 确认"), 14,
		FLinearColor(0.58f, 0.69f, 0.70f, 1.0f)), FMargin(0.0f, 12.0f));
	UButton* SettingsBack = MakeMenuButton(TEXT("SettingsBack"), TEXT("返回"), Label,
		FLinearColor(0.10f, 0.12f, 0.16f, 0.94f));
	SettingsBack->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleBackClicked);
	W11EntryMenu::AddVertical(SettingsBox, SettingsBack);
	PageSwitcher->AddChild(SettingsPanel);

	// Compendium page.
	UBorder* CompendiumPanel = MakePagePanel(TEXT("CompendiumPage"));
	UVerticalBox* CompendiumBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CompendiumBox"));
	CompendiumPanel->SetContent(CompendiumBox);
	W11EntryMenu::AddVertical(CompendiumBox, MakeText(TEXT("CompendiumHeading"), TEXT("万法图鉴"), 28,
		FLinearColor(0.91f, 0.80f, 0.48f, 1.0f)), FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	W11EntryMenu::AddVertical(CompendiumBox, MakeText(TEXT("CompendiumBody"),
		TEXT("【悟道属性】气血、真元、回元、攻伐、护体、身法、出手、行气、会心、气运、神识、法域。\n\n"
			"【秘籍九途】炼体、吐纳、剑武、术法、身法、神识、阵法、气运、神通。\n\n"
			"【宝物】法器、护身宝物、奇物、阵器与消耗品。\n\n"
			"每重试炼击败敌人可得灵石与修为；突破时三选一悟道，战后进入仙坊购买秘籍、宝物或服务。"),
		18, FLinearColor(0.80f, 0.86f, 0.82f, 1.0f)), FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	UButton* CompendiumBack = MakeMenuButton(TEXT("CompendiumBack"), TEXT("返回"), Label,
		FLinearColor(0.10f, 0.12f, 0.16f, 0.94f));
	CompendiumBack->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleBackClicked);
	W11EntryMenu::AddVertical(CompendiumBox, CompendiumBack);
	PageSwitcher->AddChild(CompendiumPanel);

	// Credits page.
	UBorder* CreditsPanel = MakePagePanel(TEXT("CreditsPage"));
	UVerticalBox* CreditsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CreditsBox"));
	CreditsPanel->SetContent(CreditsBox);
	W11EntryMenu::AddVertical(CreditsBox, MakeText(TEXT("CreditsHeading"), TEXT("制作名单"), 28,
		FLinearColor(0.91f, 0.80f, 0.48f, 1.0f)), FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	W11EntryMenu::AddVertical(CreditsBox, MakeText(TEXT("CreditsBody"),
		TEXT("世界与玩法设计\nWorld Walker Prototype\n\n程序与系统\nUnreal Engine 5.8 · C++ · Paper2D\n\nW11 原型视觉与音乐\n项目原创生成式概念美术 · 可复现原创合成配乐\n\n版本\nW11 Entry Flow v1 · 2026.08.27"),
		18, FLinearColor(0.78f, 0.84f, 0.82f, 1.0f)), FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	UButton* CreditsBack = MakeMenuButton(TEXT("CreditsBack"), TEXT("返回"), Label,
		FLinearColor(0.10f, 0.12f, 0.16f, 0.94f));
	CreditsBack->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleBackClicked);
	W11EntryMenu::AddVertical(CreditsBox, CreditsBack);
	PageSwitcher->AddChild(CreditsPanel);

	const AW11GameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AW11GameMode>() : nullptr;
	const FName ManualRole = GameMode ? GameMode->GetActiveManualEvidenceRole() : NAME_None;
	if (!ManualRole.IsNone())
	{
		const FName RejectionReason = GameMode->GetManualEvidenceSessionRejectionReason();
		const bool bAccepted = RejectionReason.IsNone();
		UBorder* GuidePanel = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("ManualEvidenceEntryGuide"));
		GuidePanel->SetBrushColor(bAccepted
			? FLinearColor(0.018f, 0.13f, 0.10f, 0.93f)
			: FLinearColor(0.28f, 0.035f, 0.025f, 0.94f));
		GuidePanel->SetPadding(FMargin(18.0f, 14.0f));
		GuidePanel->SetVisibility(ESlateVisibility::HitTestInvisible);
		Root->AddChild(GuidePanel);
		if (UCanvasPanelSlot* GuideSlot = Cast<UCanvasPanelSlot>(GuidePanel->Slot))
		{
			GuideSlot->SetAnchors(FAnchors(1.0f, 0.0f));
			GuideSlot->SetPosition(FVector2D(-28.0f, 28.0f));
			GuideSlot->SetSize(FVector2D(540.0f, 188.0f));
			GuideSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		}
		const FString Header = bAccepted
			? FString::Printf(TEXT("人工验收 · %s"),
				*AW11GameMode::GetManualEvidenceRoleDisplayName(ManualRole))
			: FString::Printf(TEXT("人工验收无效 · %s"), *RejectionReason.ToString());
		const FString Footer = bAccepted
			? TEXT("请选择角色与门派并独自启程；本局固定 Seed 424242。")
			: TEXT("本局不会被证据聚合器接受，请退出并使用标准人工入口重启。");
		UTextBlock* GuideText = MakeText(TEXT("ManualEvidenceEntryGuideText"),
			FString::Printf(TEXT("%s\n%s\n%s"), *Header,
				*AW11GameMode::GetManualEvidenceRoleInstruction(ManualRole), *Footer),
			18, bAccepted ? FLinearColor(0.88f, 0.96f, 0.83f, 1.0f)
				: FLinearColor(1.0f, 0.78f, 0.70f, 1.0f));
		GuidePanel->SetContent(GuideText);
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_MANUAL_EVIDENCE_GUIDE_READY Surface=EntryMenu Role=%s Accepted=%d Reason=%s"),
			*ManualRole.ToString(), bAccepted ? 1 : 0,
			RejectionReason.IsNone() ? TEXT("None") : *RejectionReason.ToString());
	}
}

void UW11EntryMenuWidget::LoadCharacterCreationDefinitions()
{
	HeroDefinitions.Reset();
	SectDefinitions.Reset();
	if (const UW11RunRuleSet* Rules = LoadObject<UW11RunRuleSet>(nullptr, W11EntryMenu::RuleSetPath))
	{
		for (const TSoftObjectPtr<UW11HeroDefinition>& Asset : Rules->Heroes)
		{
			if (UW11HeroDefinition* Hero = Asset.LoadSynchronous())
			{
				HeroDefinitions.Add(Hero);
			}
		}
		for (const TSoftObjectPtr<UW11SectDefinition>& Asset : Rules->Sects)
		{
			if (UW11SectDefinition* Sect = Asset.LoadSynchronous())
			{
				SectDefinitions.Add(Sect);
			}
		}
	}
}

void UW11EntryMenuWidget::BuildHeroSelectionPage()
{
	UOverlay* Page = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HeroSelectPage"));

	auto MakeTransparentButton = [this](const TCHAR* WidgetName)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(WidgetName));
		FButtonStyle Style = Button->GetStyle();
		const auto ClearBrush = [](FSlateBrush& Brush)
		{
			Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
			Brush.SetResourceObject(nullptr);
			Brush.TintColor = FSlateColor(FLinearColor::Transparent);
		};
		ClearBrush(Style.Normal);
		ClearBrush(Style.Hovered);
		ClearBrush(Style.Pressed);
		ClearBrush(Style.Disabled);
		Style.NormalPadding = FMargin(0.0f);
		Style.PressedPadding = FMargin(0.0f);
		Button->SetStyle(Style);
		Button->SetBackgroundColor(FLinearColor::Transparent);
		return Button;
	};

	UHorizontalBox* Layout = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeroSelectLayout"));
	UOverlaySlot* LayoutSlot = Page->AddChildToOverlay(Layout);
	LayoutSlot->SetHorizontalAlignment(HAlign_Fill);
	LayoutSlot->SetVerticalAlignment(VAlign_Fill);
	LayoutSlot->SetPadding(FMargin(44.0f, 36.0f, 38.0f, 30.0f));

	USizeBox* SelectorSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HeroSelectorSize"));
	SelectorSize->SetWidthOverride(430.0f);
	UBorder* SelectorPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HeroSelectorPanel"));
	SelectorPanel->SetBrushColor(FLinearColor(0.005f, 0.025f, 0.033f, 0.72f));
	SelectorPanel->SetPadding(FMargin(27.0f, 24.0f, 25.0f, 20.0f));
	SelectorSize->AddChild(SelectorPanel);
	Layout->AddChildToHorizontalBox(SelectorSize)->SetPadding(FMargin(0.0f, 0.0f, 22.0f, 0.0f));

	UVerticalBox* Selector = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HeroSelector"));
	SelectorPanel->SetContent(Selector);
	UTextBlock* Heading = MakeText(TEXT("HeroSelectHeading"), TEXT("择此世之身"), 38,
		FLinearColor(0.87f, 0.82f, 0.63f, 1.0f));
	W11EntryMenu::ApplyCalligraphyFont(Heading, 38.0f, 85);
	Heading->SetAutoWrapText(false);
	W11EntryMenu::AddVertical(Selector, Heading, FMargin(2.0f, 0.0f, 0.0f, 2.0f));
	UTextBlock* Help = MakeText(TEXT("HeroSelectHelp"), TEXT("十命同途，禀赋各异。择一化身，再入仙门。"), 14,
		FLinearColor(0.53f, 0.69f, 0.67f, 1.0f));
	W11EntryMenu::AddVertical(Selector, Help, FMargin(2.0f, 0.0f, 0.0f, 14.0f));

	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("HeroChoiceGrid"));
	Grid->SetSlotPadding(FMargin(4.0f, 3.0f));
	Selector->AddChildToVerticalBox(Grid)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	HeroButtons.Reset();
	HeroAvatarImages.Reset();
	for (int32 Index = 0; Index < HeroDefinitions.Num(); ++Index)
	{
		UW11HeroDefinition* Hero = HeroDefinitions[Index];
		UButton* Card = MakeTransparentButton(*FString::Printf(TEXT("HeroCard%d"), Index));
		UImage* Avatar = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
			FName(*FString::Printf(TEXT("HeroAvatar%d"), Index)));
		if (Hero)
		{
			if (UTexture2D* AvatarTexture = Hero->Avatar.LoadSynchronous())
			{
				Avatar->SetBrushFromTexture(AvatarTexture, false);
			}
		}
		Avatar->SetRenderTransformPivot(FVector2D(0.5f));
		Avatar->SetVisibility(ESlateVisibility::HitTestInvisible);
		USizeBox* AvatarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),
			FName(*FString::Printf(TEXT("HeroAvatarSize%d"), Index)));
		AvatarSize->SetWidthOverride(108.0f);
		AvatarSize->SetHeightOverride(108.0f);
		AvatarSize->AddChild(Avatar);
		UOverlay* AvatarCenter = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(),
			FName(*FString::Printf(TEXT("HeroAvatarCenter%d"), Index)));
		UOverlaySlot* AvatarSlot = AvatarCenter->AddChildToOverlay(AvatarSize);
		AvatarSlot->SetHorizontalAlignment(HAlign_Center);
		AvatarSlot->SetVerticalAlignment(VAlign_Center);
		Card->AddChild(AvatarCenter);
		switch (Index)
		{
		case 0: Card->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleHero0Clicked); break;
		case 1: Card->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleHero1Clicked); break;
		case 2: Card->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleHero2Clicked); break;
		case 3: Card->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleHero3Clicked); break;
		case 4: Card->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleHero4Clicked); break;
		case 5: Card->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleHero5Clicked); break;
		case 6: Card->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleHero6Clicked); break;
		case 7: Card->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleHero7Clicked); break;
		case 8: Card->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleHero8Clicked); break;
		case 9: Card->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleHero9Clicked); break;
		default: break;
		}
		USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),
			FName(*FString::Printf(TEXT("HeroCardSize%d"), Index)));
		CardSize->SetWidthOverride(174.0f);
		CardSize->SetHeightOverride(116.0f);
		CardSize->AddChild(Card);
		UUniformGridSlot* CardSlot = Grid->AddChildToUniformGrid(CardSize, Index / 2, Index % 2);
		CardSlot->SetHorizontalAlignment(HAlign_Fill);
		CardSlot->SetVerticalAlignment(VAlign_Fill);
		HeroButtons.Add(Card);
		HeroAvatarImages.Add(Avatar);
	}

	UHorizontalBox* Nav = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeroNav"));
	W11EntryMenu::AddVertical(Selector, Nav, FMargin(0.0f, 13.0f, 0.0f, 0.0f));
	auto MakeNavButton = [&](const TCHAR* Name, const FString& LabelText, UTextBlock*& OutText)
	{
		UButton* Button = MakeTransparentButton(Name);
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),
			*FString(Name).Append(TEXT("Size")));
		Size->SetWidthOverride(178.0f);
		Size->SetHeightOverride(48.0f);
		OutText = MakeText(*FString(Name).Append(TEXT("Label")), LabelText, 19,
			FLinearColor(0.63f, 0.75f, 0.68f, 1.0f));
		W11EntryMenu::ApplyCalligraphyFont(OutText, 19.0f, 45);
		OutText->SetAutoWrapText(false);
		OutText->SetJustification(ETextJustify::Center);
		Size->AddChild(OutText);
		Button->AddChild(Size);
		return Button;
	};
	UTextBlock* Label = nullptr;
	UButton* Back = MakeNavButton(TEXT("HeroBack"), TEXT("返回主界"), Label);
	Back->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleBackClicked);
	Nav->AddChildToHorizontalBox(Back)->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
	UButton* Next = MakeNavButton(TEXT("HeroNext"), TEXT("选定此身"), Label);
	Next->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleHeroNextClicked);
	Nav->AddChildToHorizontalBox(Next)->SetPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f));

	UOverlay* Stage = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HeroPortraitStage"));
	UHorizontalBoxSlot* StageSlot = Layout->AddChildToHorizontalBox(Stage);
	StageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	USizeBox* PortraitSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SelectedHeroPortraitSize"));
	// 1024x1536 portraits use an immutable 2:3 display rectangle. No ScaleBox and no
	// render transform ever touch the portrait; only texture sampling changes resolution.
	PortraitSize->SetWidthOverride(526.6667f);
	PortraitSize->SetHeightOverride(790.0f);

	auto AddAuraLayer = [&](const TCHAR* Name, const float Width, const float Height,
		const float RightPadding, const FLinearColor& Tint) -> UImage*
	{
		USizeBox* AuraSize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), *FString(Name).Append(TEXT("Size")));
		AuraSize->SetWidthOverride(Width);
		AuraSize->SetHeightOverride(Height);
		UImage* Aura = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		if (UTexture2D* AuraTexture = LoadObject<UTexture2D>(nullptr, W11EntryMenu::HeroSelectionPortraitAuraPath))
		{
			Aura->SetBrushFromTexture(AuraTexture, false);
		}
		Aura->SetColorAndOpacity(Tint);
		Aura->SetVisibility(ESlateVisibility::HitTestInvisible);
		Aura->SetRenderTransformPivot(FVector2D(0.5f));
		AuraSize->AddChild(Aura);
		UOverlaySlot* AuraSlot = Stage->AddChildToOverlay(AuraSize);
		AuraSlot->SetHorizontalAlignment(HAlign_Right);
		AuraSlot->SetVerticalAlignment(VAlign_Bottom);
		AuraSlot->SetPadding(FMargin(0.0f, 0.0f, RightPadding, 0.0f));
		return Aura;
	};
	// Keep the aura 68 Slate units right of the original composition, then move the static
	// portrait by the same amount below so both layers remain concentric.
	constexpr float AuraCircleAlignmentShiftX = 68.0f;
	HeroPortraitAuraBase = AddAuraLayer(TEXT("HeroPortraitAuraBase"), 610.0f, 840.0f,
		103.0f - AuraCircleAlignmentShiftX,
		FLinearColor(0.72f, 1.0f, 0.94f, 1.0f));
	HeroPortraitAuraBloom = AddAuraLayer(TEXT("HeroPortraitAuraBloom"), 560.0f, 820.0f,
		128.0f - AuraCircleAlignmentShiftX,
		FLinearColor(0.64f, 0.94f, 1.0f, 1.0f));

	HeroPortraitImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SelectedHeroPortrait"));
	HeroPortraitImage->SetRenderScale(FVector2D(1.0f));
	HeroPortraitImage->SetRenderTranslation(FVector2D::ZeroVector);
	HeroPortraitImage->SetRenderTransformAngle(0.0f);
	HeroPortraitImage->SetRenderOpacity(1.0f);
	HeroPortraitImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	PortraitSize->AddChild(HeroPortraitImage);
	UOverlaySlot* PortraitSlot = Stage->AddChildToOverlay(PortraitSize);
	PortraitSlot->SetHorizontalAlignment(HAlign_Right);
	PortraitSlot->SetVerticalAlignment(VAlign_Bottom);
	constexpr float PortraitCircleAlignmentShiftX = 68.0f;
	PortraitSlot->SetPadding(FMargin(0.0f, 8.0f,
		145.0f - PortraitCircleAlignmentShiftX, 2.0f));

	USizeBox* DetailSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SelectedHeroDetailSize"));
	DetailSize->SetWidthOverride(410.0f);
	DetailSize->SetHeightOverride(250.0f);
	UBorder* DetailPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SelectedHeroDetailPanel"));
	DetailPanel->SetBrushColor(FLinearColor(0.004f, 0.025f, 0.033f, 0.66f));
	DetailPanel->SetPadding(FMargin(22.0f, 15.0f, 22.0f, 14.0f));
	DetailSize->AddChild(DetailPanel);
	UVerticalBox* DetailBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SelectedHeroDetails"));
	DetailPanel->SetContent(DetailBox);
	HeroNameText = MakeText(TEXT("SelectedHeroName"), TEXT(""), 52,
		FLinearColor(0.86f, 0.93f, 0.82f, 1.0f));
	W11EntryMenu::ApplyCalligraphyFont(HeroNameText, 52.0f, 95);
	HeroNameText->SetAutoWrapText(false);
	W11EntryMenu::AddVertical(DetailBox, HeroNameText, FMargin(0.0f, 0.0f, 0.0f, 0.0f));
	HeroEpithetText = MakeText(TEXT("SelectedHeroEpithet"), TEXT(""), 18,
		FLinearColor(0.61f, 0.81f, 0.76f, 1.0f));
	HeroEpithetText->SetAutoWrapText(false);
	W11EntryMenu::AddVertical(DetailBox, HeroEpithetText, FMargin(2.0f, 0.0f, 0.0f, 12.0f));
	HeroDetailText = MakeText(TEXT("HeroDetail"), TEXT(""), 15,
		FLinearColor(0.73f, 0.80f, 0.75f, 1.0f));
	W11EntryMenu::AddVertical(DetailBox, HeroDetailText);
	UOverlaySlot* DetailSlot = Stage->AddChildToOverlay(DetailSize);
	DetailSlot->SetHorizontalAlignment(HAlign_Left);
	DetailSlot->SetVerticalAlignment(VAlign_Bottom);
	DetailSlot->SetPadding(FMargin(26.0f, 0.0f, 0.0f, 34.0f));

	PageSwitcher->AddChild(Page);
}

void UW11EntryMenuWidget::BuildSectSelectionPage()
{
	UCanvasPanel* Page = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SectSelectPage"));

	const auto MakeMinimalButton = [this](const TCHAR* Name, const FString& LabelText,
		const float FontSize, const FLinearColor& Color)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
		FButtonStyle Style = Button->GetStyle();
		const auto ClearBrush = [](FSlateBrush& Brush)
		{
			Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
			Brush.SetResourceObject(nullptr);
			Brush.TintColor = FSlateColor(FLinearColor::Transparent);
		};
		ClearBrush(Style.Normal);
		ClearBrush(Style.Hovered);
		ClearBrush(Style.Pressed);
		ClearBrush(Style.Disabled);
		Style.NormalPadding = FMargin(0.0f);
		Style.PressedPadding = FMargin(0.0f);
		Button->SetStyle(Style);
		Button->SetBackgroundColor(FLinearColor::Transparent);

		UTextBlock* Label = MakeText(*FString(Name).Append(TEXT("Label")), LabelText,
			FMath::RoundToInt(FontSize), Color);
		W11EntryMenu::ApplyCalligraphyFont(Label, FontSize, 65);
		Label->SetAutoWrapText(false);
		Label->SetJustification(ETextJustify::Center);
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		Button->AddChild(Label);
		return Button;
	};

	// The full-screen sect painting is the page. Only a small bottom carousel remains on top.
	UBorder* SelectorPlate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SectSelectorPlate"));
	SelectorPlate->SetBrushColor(FLinearColor(0.004f, 0.018f, 0.022f, 0.68f));
	SelectorPlate->SetPadding(FMargin(6.0f, 4.0f));
	UHorizontalBox* Selector = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SectSelector"));
	SelectorPlate->SetContent(Selector);

	USizeBox* PreviousSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PreviousSectSize"));
	PreviousSize->SetWidthOverride(54.0f);
	PreviousSize->SetHeightOverride(56.0f);
	UButton* Previous = MakeMinimalButton(TEXT("PreviousSect"), TEXT("〈"), 34.0f,
		FLinearColor(0.82f, 0.92f, 0.88f, 1.0f));
	Previous->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandlePreviousSectClicked);
	PreviousSize->AddChild(Previous);
	Selector->AddChildToHorizontalBox(PreviousSize);

	USizeBox* NameSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SectNameSize"));
	NameSize->SetWidthOverride(360.0f);
	NameSize->SetHeightOverride(56.0f);
	SectDetailText = MakeText(TEXT("SectName"), TEXT(""), 30,
		FLinearColor(0.95f, 0.91f, 0.74f, 1.0f));
	W11EntryMenu::ApplyCalligraphyFont(SectDetailText, 30.0f, 85);
	SectDetailText->SetAutoWrapText(false);
	SectDetailText->SetJustification(ETextJustify::Center);
	SectDetailText->SetMargin(FMargin(4.0f, 7.0f));
	SectDetailText->SetVisibility(ESlateVisibility::HitTestInvisible);
	NameSize->AddChild(SectDetailText);
	Selector->AddChildToHorizontalBox(NameSize);

	USizeBox* NextSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("NextSectSize"));
	NextSize->SetWidthOverride(54.0f);
	NextSize->SetHeightOverride(56.0f);
	UButton* Next = MakeMinimalButton(TEXT("NextSect"), TEXT("〉"), 34.0f,
		FLinearColor(0.82f, 0.92f, 0.88f, 1.0f));
	Next->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleNextSectClicked);
	NextSize->AddChild(Next);
	Selector->AddChildToHorizontalBox(NextSize);

	UCanvasPanelSlot* SelectorSlot = Page->AddChildToCanvas(SelectorPlate);
	SelectorSlot->SetAnchors(FAnchors(0.5f, 1.0f));
	SelectorSlot->SetAlignment(FVector2D(0.5f, 1.0f));
	SelectorSlot->SetPosition(FVector2D(0.0f, -22.0f));
	SelectorSlot->SetSize(FVector2D(480.0f, 64.0f));

	UButton* Back = MakeMinimalButton(TEXT("SectBack"), TEXT("返回择人"), 22.0f,
		FLinearColor(0.70f, 0.78f, 0.76f, 0.96f));
	Back->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleSectBackClicked);
	UCanvasPanelSlot* BackSlot = Page->AddChildToCanvas(Back);
	BackSlot->SetAnchors(FAnchors(0.0f, 1.0f));
	BackSlot->SetAlignment(FVector2D(0.0f, 1.0f));
	BackSlot->SetPosition(FVector2D(18.0f, -28.0f));
	BackSlot->SetSize(FVector2D(170.0f, 50.0f));

	UButton* Confirm = MakeMinimalButton(TEXT("ConfirmJourney"), TEXT("确认门派"), 26.0f,
		FLinearColor(0.98f, 0.86f, 0.54f, 1.0f));
	Confirm->OnClicked.AddDynamic(this, &UW11EntryMenuWidget::HandleConfirmJourneyClicked);
	UCanvasPanelSlot* ConfirmSlot = Page->AddChildToCanvas(Confirm);
	ConfirmSlot->SetAnchors(FAnchors(1.0f, 1.0f));
	ConfirmSlot->SetAlignment(FVector2D(1.0f, 1.0f));
	ConfirmSlot->SetPosition(FVector2D(-18.0f, -28.0f));
	ConfirmSlot->SetSize(FVector2D(210.0f, 54.0f));

	PageSwitcher->AddChild(Page);
}

FString UW11EntryMenuWidget::FormatHeroModifiers(const UW11HeroDefinition* Hero) const
{
	if (!Hero)
	{
		return TEXT("先天禀赋未载入");
	}
	TArray<FString> Parts;
	for (const FW11StatModifier& Modifier : Hero->StartingModifiers)
	{
		FString StatName;
		switch (Modifier.Stat)
		{
		case EW11StatType::MaxHealth: StatName = TEXT("气血"); break;
		case EW11StatType::MaxMana: StatName = TEXT("真元"); break;
		case EW11StatType::ManaRegen: StatName = TEXT("回元"); break;
		case EW11StatType::Power: StatName = TEXT("攻伐"); break;
		case EW11StatType::Armor: StatName = TEXT("护体"); break;
		case EW11StatType::MoveSpeed: StatName = TEXT("身法"); break;
		case EW11StatType::AttackSpeed: StatName = TEXT("出手"); break;
		case EW11StatType::AbilityHaste: StatName = TEXT("行气"); break;
		case EW11StatType::CritChance: StatName = TEXT("会心"); break;
		case EW11StatType::CritMultiplier: StatName = TEXT("会心威能"); break;
		case EW11StatType::Luck: StatName = TEXT("气运"); break;
		case EW11StatType::AttackRange: StatName = TEXT("神识"); break;
		case EW11StatType::Area: StatName = TEXT("法域"); break;
		case EW11StatType::PickupRadius: StatName = TEXT("感灵"); break;
		case EW11StatType::Duration: StatName = TEXT("延法"); break;
		case EW11StatType::HealingReceived: StatName = TEXT("纳药"); break;
		default: StatName = TEXT("道行"); break;
		}
		if (Modifier.Operation == EW11ModifierOperation::Multiply)
		{
			Parts.Add(FString::Printf(TEXT("%s +%.0f%%"), *StatName, (Modifier.Magnitude - 1.0f) * 100.0f));
		}
		else
		{
			Parts.Add(FString::Printf(TEXT("%s +%g"), *StatName, Modifier.Magnitude));
		}
	}
	return FString::Join(Parts, TEXT(" · "));
}

void UW11EntryMenuWidget::SelectHero(const int32 Index)
{
	if (HeroDefinitions.IsValidIndex(Index))
	{
		SelectedHeroIndex = Index;
		RefreshHeroSelection();
	}
}

void UW11EntryMenuWidget::SelectSect(const int32 Index)
{
	if (SectDefinitions.IsValidIndex(Index))
	{
		SelectedSectIndex = Index;
		RefreshSectSelection();
	}
}

void UW11EntryMenuWidget::RefreshHeroSelection()
{
	if (!HeroDefinitions.IsValidIndex(SelectedHeroIndex))
	{
		return;
	}

	const UW11HeroDefinition* Hero = HeroDefinitions[SelectedHeroIndex];
	if (HeroNameText)
	{
		HeroNameText->SetText(Hero->DisplayName);
	}
	if (HeroEpithetText)
	{
		HeroEpithetText->SetText(FText::FromString(FString::Printf(TEXT("号 · %s"), *Hero->Epithet.ToString())));
	}
	if (HeroDetailText)
	{
		HeroDetailText->SetText(FText::FromString(FString::Printf(TEXT("先天禀赋\n%s\n\n%s"),
			*FormatHeroModifiers(Hero), *Hero->Biography.ToString())));
	}
	if (HeroPortraitImage)
	{
		if (UTexture2D* Texture = Hero->Portrait.LoadSynchronous())
		{
			HeroPortraitImage->SetBrushFromTexture(Texture, false);
		}
	}
}

void UW11EntryMenuWidget::RefreshSectSelection()
{
	if (SectDefinitions.IsValidIndex(SelectedSectIndex))
	{
		const UW11SectDefinition* Sect = SectDefinitions[SelectedSectIndex];
		if (BackgroundImage)
		{
			if (UTexture2D* ArtworkTexture = Sect->SelectionArtwork.LoadSynchronous())
			{
				// The root image is viewport-anchored. Reusing it avoids a second page-sized
				// layer and makes every sect selection immediately become the full background.
				BackgroundImage->SetBrushFromTexture(ArtworkTexture, false);
			}
		}
		if (SectDetailText)
		{
			SectDetailText->SetText(Sect->DisplayName);
		}
	}
}

void UW11EntryMenuWidget::SetPage(const EEntryPage Page)
{
	ActivePage = Page;
	const bool bCharacterCreation = Page == EEntryPage::HeroSelect || Page == EEntryPage::SectSelect;
	const bool bHeroSelection = Page == EEntryPage::HeroSelect;
	const bool bSectSelection = Page == EEntryPage::SectSelect;
	if (BackgroundImage)
	{
		const TCHAR* DesiredBackground = bHeroSelection
			? W11EntryMenu::HeroSelectionBackgroundPath
			: W11EntryMenu::BackgroundPath;
		if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, DesiredBackground))
		{
			BackgroundImage->SetBrushFromTexture(Texture, false);
		}
	}
	if (FogImageA)
	{
		FogImageA->SetVisibility(bCharacterCreation ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (FogImageB)
	{
		FogImageB->SetVisibility(bCharacterCreation ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	for (UBorder* Mote : SpiritMotes)
	{
		if (Mote)
		{
			Mote->SetVisibility(bCharacterCreation ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		}
	}
	if (W11Controller)
	{
		// Hero and sect selection are one continuous character-creation sequence.
		// The controller ignores an already-playing asset, so this preserves the same
		// music across the page transition without restarting or cross-fading it.
		W11Controller->SetHeroSelectionMusic(bCharacterCreation);
	}
	if (MenuSizeBox)
	{
		MenuSizeBox->SetWidthOverride(bCharacterCreation ? 1740.0f : 510.0f);
		MenuSizeBox->SetHeightOverride(bHeroSelection ? 920.0f : (bSectSelection ? 980.0f : 730.0f));
	}
	if (PageSizeBox)
	{
		PageSizeBox->SetWidthOverride(bCharacterCreation ? 1676.0f : 446.0f);
		PageSizeBox->SetHeightOverride(bHeroSelection ? 850.0f : (bSectSelection ? 910.0f : 526.0f));
	}
	if (TitlePlaqueSizeBox)
	{
		TitlePlaqueSizeBox->SetVisibility(bCharacterCreation
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}
	if (PageSwitcher)
	{
		PageSwitcher->SetActiveWidgetIndex(static_cast<int32>(Page));
	}
	if (Page == EEntryPage::Settings)
	{
		RefreshSettings();
	}
	else if (Page == EEntryPage::Coop)
	{
		RefreshSessionResults();
	}
	else if (Page == EEntryPage::HeroSelect)
	{
		RefreshHeroSelection();
	}
	else if (Page == EEntryPage::SectSelect)
	{
		RefreshSectSelection();
	}
}

void UW11EntryMenuWidget::RefreshSettings()
{
	if (MusicSlider && W11Controller)
	{
		MusicSlider->SetValue(W11Controller->GetMenuMusicVolume());
	}
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings)
	{
		return;
	}
	if (VSyncCheckBox)
	{
		VSyncCheckBox->SetIsChecked(Settings->IsVSyncEnabled());
	}
	if (WindowModeText)
	{
		FString Label = TEXT("显示模式：窗口");
		switch (Settings->GetFullscreenMode())
		{
		case EWindowMode::Fullscreen: Label = TEXT("显示模式：全屏"); break;
		case EWindowMode::WindowedFullscreen: Label = TEXT("显示模式：无边框窗口"); break;
		default: break;
		}
		WindowModeText->SetText(FText::FromString(Label));
	}
}

void UW11EntryMenuWidget::RefreshSessionResults()
{
	TArray<FString> Labels;
	if (W11Controller)
	{
		Labels = W11Controller->GetSessionResultLabels();
	}
	for (int32 Index = 0; Index < SessionButtons.Num(); ++Index)
	{
		const bool bVisible = Labels.IsValidIndex(Index);
		SessionButtons[Index]->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bVisible && SessionLabels.IsValidIndex(Index))
		{
			SessionLabels[Index]->SetText(FText::FromString(Labels[Index]));
		}
	}
}

void UW11EntryMenuWidget::SetStatus(const FString& Message, const bool bError)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Message));
		StatusText->SetColorAndOpacity(FSlateColor(bError
			? FLinearColor(0.95f, 0.42f, 0.35f, 1.0f)
			: FLinearColor(0.45f, 0.90f, 0.82f, 1.0f)));
	}
}

void UW11EntryMenuWidget::HandleSoloClicked()
{
	SetPage(EEntryPage::HeroSelect);
}

void UW11EntryMenuWidget::HandleContinueClicked()
{
	if (W11Controller && W11Controller->CanResumeSoloRun())
	{
		W11Controller->ResumeSoloRun();
	}
	else
	{
		SetStatus(TEXT("当前没有与内容版本匹配的安全节点存档。"), true);
	}
}
void UW11EntryMenuWidget::HandleHero0Clicked() { SelectHero(0); }
void UW11EntryMenuWidget::HandleHero1Clicked() { SelectHero(1); }
void UW11EntryMenuWidget::HandleHero2Clicked() { SelectHero(2); }
void UW11EntryMenuWidget::HandleHero3Clicked() { SelectHero(3); }
void UW11EntryMenuWidget::HandleHero4Clicked() { SelectHero(4); }
void UW11EntryMenuWidget::HandleHero5Clicked() { SelectHero(5); }
void UW11EntryMenuWidget::HandleHero6Clicked() { SelectHero(6); }
void UW11EntryMenuWidget::HandleHero7Clicked() { SelectHero(7); }
void UW11EntryMenuWidget::HandleHero8Clicked() { SelectHero(8); }
void UW11EntryMenuWidget::HandleHero9Clicked() { SelectHero(9); }
void UW11EntryMenuWidget::HandleHeroNextClicked() { SetPage(EEntryPage::SectSelect); }
void UW11EntryMenuWidget::HandlePreviousSectClicked()
{
	if (!SectDefinitions.IsEmpty())
	{
		SelectSect((SelectedSectIndex - 1 + SectDefinitions.Num()) % SectDefinitions.Num());
	}
}
void UW11EntryMenuWidget::HandleNextSectClicked()
{
	if (!SectDefinitions.IsEmpty())
	{
		SelectSect((SelectedSectIndex + 1) % SectDefinitions.Num());
	}
}
void UW11EntryMenuWidget::HandleSectBackClicked() { SetPage(EEntryPage::HeroSelect); }
void UW11EntryMenuWidget::HandleConfirmJourneyClicked()
{
	if (W11Controller && HeroDefinitions.IsValidIndex(SelectedHeroIndex)
		&& SectDefinitions.IsValidIndex(SelectedSectIndex))
	{
		W11Controller->StartSoloRun(HeroDefinitions[SelectedHeroIndex]->DefinitionId,
			SectDefinitions[SelectedSectIndex]->DefinitionId);
	}
}
void UW11EntryMenuWidget::HandleCoopClicked() { SetPage(EEntryPage::Coop); }
void UW11EntryMenuWidget::HandleSettingsClicked() { SetPage(EEntryPage::Settings); }
void UW11EntryMenuWidget::HandleCompendiumClicked() { SetPage(EEntryPage::Compendium); }
void UW11EntryMenuWidget::HandleCreditsClicked() { SetPage(EEntryPage::Credits); }
void UW11EntryMenuWidget::HandleReturnWorldClicked() { if (W11Controller) W11Controller->ReturnToMainWorld(); }
void UW11EntryMenuWidget::HandleQuitClicked() { if (W11Controller) W11Controller->QuitGameFromMenu(); }
void UW11EntryMenuWidget::HandleBackClicked() { SetPage(EEntryPage::Main); }

void UW11EntryMenuWidget::HandleHostClicked()
{
	SetStatus(TEXT("正在创建共修房间……"));
	if (W11Controller) W11Controller->HostCoopRun();
}

void UW11EntryMenuWidget::HandleFindClicked()
{
	SetStatus(TEXT("正在寻找同道……"));
	if (W11Controller) W11Controller->FindCoopRuns();
}

void UW11EntryMenuWidget::HandleJoin0Clicked() { if (W11Controller) W11Controller->JoinCoopRun(0); }
void UW11EntryMenuWidget::HandleJoin1Clicked() { if (W11Controller) W11Controller->JoinCoopRun(1); }
void UW11EntryMenuWidget::HandleJoin2Clicked() { if (W11Controller) W11Controller->JoinCoopRun(2); }
void UW11EntryMenuWidget::HandleJoin3Clicked() { if (W11Controller) W11Controller->JoinCoopRun(3); }

void UW11EntryMenuWidget::HandleMusicVolumeChanged(const float Value)
{
	if (W11Controller) W11Controller->SetMenuMusicVolume(Value);
}

void UW11EntryMenuWidget::HandleVSyncChanged(const bool bChecked)
{
	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		Settings->SetVSyncEnabled(bChecked);
	}
}

void UW11EntryMenuWidget::HandleWindowModeClicked()
{
	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		const EWindowMode::Type Current = Settings->GetFullscreenMode();
		const EWindowMode::Type Next = Current == EWindowMode::Windowed
			? EWindowMode::WindowedFullscreen
			: Current == EWindowMode::WindowedFullscreen ? EWindowMode::Fullscreen : EWindowMode::Windowed;
		Settings->SetFullscreenMode(Next);
		RefreshSettings();
	}
}

void UW11EntryMenuWidget::HandleApplySettingsClicked()
{
	if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
	{
		Settings->ApplySettings(false);
		Settings->SaveSettings();
	}
	RefreshSettings();
}

void UW11EntryMenuWidget::HandleHostCompleted(const bool bSuccess, const FString& Message)
{
	SetStatus(bSuccess ? TEXT("房间创建成功，正在进入……") : Message, !bSuccess);
}

void UW11EntryMenuWidget::HandleSearchCompleted(const int32 ResultCount)
{
	RefreshSessionResults();
	SetStatus(ResultCount > 0
		? FString::Printf(TEXT("找到 %d 个共修房间"), ResultCount)
		: TEXT("没有发现可加入的房间"), ResultCount <= 0);
}

void UW11EntryMenuWidget::HandleJoinCompleted(const bool bSuccess, const FString& Message)
{
	SetStatus(bSuccess ? TEXT("连接成功，正在加入旅途……") : Message, !bSuccess);
}
