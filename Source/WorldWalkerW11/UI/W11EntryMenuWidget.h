#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W11EntryMenuWidget.generated.h"

class AW11PlayerController;
class UBorder;
class UButton;
class UCheckBox;
class UImage;
class USizeBox;
class USlider;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;
class UW11HeroDefinition;
class UW11SectDefinition;

/** Native, resolution-independent W11 title/menu flow with a layered animated background. */
UCLASS()
class WORLDWALKERW11_API UW11EntryMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configure(AW11PlayerController* InController);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	enum class EEntryPage : uint8
	{
		Main,
		HeroSelect,
		SectSelect,
		Coop,
		Settings,
		Compendium,
		Credits
	};

	void BuildWidgetTree();
	void LoadCharacterCreationDefinitions();
	void BuildHeroSelectionPage();
	void BuildSectSelectionPage();
	void SetPage(EEntryPage Page);
	void SelectHero(int32 Index);
	void SelectSect(int32 Index);
	void RefreshHeroSelection();
	void RefreshSectSelection();
	FString FormatHeroModifiers(const UW11HeroDefinition* Hero) const;
	void RefreshSettings();
	void RefreshSessionResults();
	void SetStatus(const FString& Message, bool bError = false);
	UTextBlock* MakeText(const TCHAR* Name, const FString& Text, int32 FontSize,
		const FLinearColor& Color = FLinearColor::White) const;
	UButton* MakeMenuButton(const TCHAR* Name, const FString& Label, UTextBlock*& OutLabel,
		const FLinearColor& Tint = FLinearColor(0.018f, 0.105f, 0.105f, 0.94f));
	UBorder* MakePagePanel(const TCHAR* Name) const;

	UFUNCTION() void HandleSoloClicked();
	UFUNCTION() void HandleContinueClicked();
	UFUNCTION() void HandleHero0Clicked();
	UFUNCTION() void HandleHero1Clicked();
	UFUNCTION() void HandleHero2Clicked();
	UFUNCTION() void HandleHero3Clicked();
	UFUNCTION() void HandleHero4Clicked();
	UFUNCTION() void HandleHero5Clicked();
	UFUNCTION() void HandleHero6Clicked();
	UFUNCTION() void HandleHero7Clicked();
	UFUNCTION() void HandleHero8Clicked();
	UFUNCTION() void HandleHero9Clicked();
	UFUNCTION() void HandleHeroNextClicked();
	UFUNCTION() void HandlePreviousSectClicked();
	UFUNCTION() void HandleNextSectClicked();
	UFUNCTION() void HandleSectBackClicked();
	UFUNCTION() void HandleConfirmJourneyClicked();
	UFUNCTION() void HandleCoopClicked();
	UFUNCTION() void HandleSettingsClicked();
	UFUNCTION() void HandleCompendiumClicked();
	UFUNCTION() void HandleCreditsClicked();
	UFUNCTION() void HandleReturnWorldClicked();
	UFUNCTION() void HandleQuitClicked();
	UFUNCTION() void HandleBackClicked();
	UFUNCTION() void HandleHostClicked();
	UFUNCTION() void HandleFindClicked();
	UFUNCTION() void HandleJoin0Clicked();
	UFUNCTION() void HandleJoin1Clicked();
	UFUNCTION() void HandleJoin2Clicked();
	UFUNCTION() void HandleJoin3Clicked();
	UFUNCTION() void HandleMusicVolumeChanged(float Value);
	UFUNCTION() void HandleVSyncChanged(bool bChecked);
	UFUNCTION() void HandleWindowModeClicked();
	UFUNCTION() void HandleApplySettingsClicked();
	UFUNCTION() void HandleHostCompleted(bool bSuccess, const FString& Message);
	UFUNCTION() void HandleSearchCompleted(int32 ResultCount);
	UFUNCTION() void HandleJoinCompleted(bool bSuccess, const FString& Message);

	UPROPERTY(Transient)
	TObjectPtr<AW11PlayerController> W11Controller;

	UPROPERTY(Transient)
	TObjectPtr<UImage> BackgroundImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FogImageA;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FogImageB;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> SpiritMotes;

	/** Transparent hit targets whose labels are rendered as carvings in the shared stone tablet. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> StoneMenuButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> StoneMenuLabels;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> MenuChrome;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> MenuSizeBox;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> PageSizeBox;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> TitlePlaqueSizeBox;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> PageSwitcher;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ContinueButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ContinueLabel;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> SessionResultsBox;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> SessionButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> SessionLabels;

	UPROPERTY(Transient)
	TObjectPtr<USlider> MusicSlider;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> VSyncCheckBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> WindowModeText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UW11HeroDefinition>> HeroDefinitions;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UW11SectDefinition>> SectDefinitions;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> HeroButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> HeroAvatarImages;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HeroDetailText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HeroNameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HeroEpithetText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> HeroPortraitImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> HeroPortraitAuraBase;

	UPROPERTY(Transient)
	TObjectPtr<UImage> HeroPortraitAuraBloom;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SectDetailText;

	float AnimationTime = 0.0f;
	TArray<float> StoneMenuHoverAmounts;
	bool bDelayedScreenshotPending = false;
	bool bStoneHoverPreview = false;
	bool bDelegatesBound = false;
	int32 SelectedHeroIndex = 0;
	int32 SelectedSectIndex = 0;
	EEntryPage ActivePage = EEntryPage::Main;
};
