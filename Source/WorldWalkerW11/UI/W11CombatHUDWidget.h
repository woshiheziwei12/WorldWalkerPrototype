#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/W11Types.h"
#include "W11CombatHUDWidget.generated.h"

class AW11Character;
class AW11GameState;
class AW11PlayerController;
class AW11PlayerState;
class UBorder;
class UButton;
class UProgressBar;
class UTextBlock;
class UW11AttributeComponent;
class UW11CombatComponent;

/** Native Chinese combat HUD; it observes replicated facts and forwards only intent. */
UCLASS()
class WORLDWALKERW11_API UW11CombatHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	static EW11FacingDirection ResolveIncomingDirection(const FVector& SourceOffset);

private:
	void BuildLayout();
	void BindRuntimeSources();
	void RefreshVitals();
	void RefreshCombatState();
	void RefreshCooldowns();
	void RefreshTeamStatus();
	void RefreshRuntimeState();
	void RefreshManualEvidenceGuide();
	void CaptureForValidation();

	UFUNCTION()
	void HandleHealthChanged(float CurrentValue, float MaxValue);

	UFUNCTION()
	void HandleManaChanged(float CurrentValue, float MaxValue);

	UFUNCTION()
	void HandleBarrierChanged(float CurrentValue);

	UFUNCTION()
	void HandleStatusesChanged();

	UFUNCTION()
	void HandleGameStateChanged();

	UFUNCTION()
	void HandleAbilityResult(const FW11AbilityExecutionResult& Result);

	UFUNCTION()
	void HandleCooldownChanged(EW11AbilitySlot AbilitySlot, float CooldownEndServerTime);

	UFUNCTION()
	void HandleDodgeCooldownChanged(float CooldownEndServerTime);

	UFUNCTION()
	void HandleIncomingCombatCue(const FW11CombatCue& Cue);

	UFUNCTION()
	void HandleRestartClicked();

	TWeakObjectPtr<AW11PlayerController> W11Controller;
	TWeakObjectPtr<AW11PlayerState> W11PlayerState;
	TWeakObjectPtr<AW11Character> W11Character;
	TWeakObjectPtr<AW11GameState> W11GameState;
	TWeakObjectPtr<UW11AttributeComponent> Attributes;
	TWeakObjectPtr<UW11CombatComponent> Combat;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HealthText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ManaText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BarrierText;
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> ManaBar;
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> BarrierBar;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ObjectiveText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CooldownText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ActiveStatusText;
	UPROPERTY(Transient)
	TObjectPtr<UBorder> TeamStatusPanel;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TeamStatusText;
	UPROPERTY(Transient)
	TObjectPtr<UBorder> DamageDirectionPanel;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DamageDirectionText;
	UPROPERTY(Transient)
	TObjectPtr<UBorder> ManualEvidencePanel;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ManualEvidenceText;
	UPROPERTY(Transient)
	TObjectPtr<UBorder> ResultsOverlay;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResultTitle;
	UPROPERTY(Transient)
	TObjectPtr<UButton> RestartButton;

	float RefreshAccumulator = 0.0f;
	float TeamRefreshAccumulator = 0.0f;
	float StatusVisibleUntil = 0.0f;
	float DamageDirectionVisibleUntil = 0.0f;
	float CaptureDelay = 1.2f;
	bool bCapturePending = false;
	bool bManualEvidenceGuideLogged = false;
	FTimerHandle CaptureTimerHandle;
	int32 LastLoggedTeammateCount = -1;
};
