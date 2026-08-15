#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/WorldWalkerInteractable.h"
#include "FantasyFateAltar.generated.h"

class APlayerController;
class AWorldWalkerCharacter;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UWorld;
class UFantasyFateAltarWidget;

/** The three deterministic outcomes offered by the W01 Ashen Fate Altar. */
UENUM(BlueprintType)
enum class EFantasyFateAltarChoice : uint8
{
	None UMETA(Hidden),
	BloodOath UMETA(DisplayName="Blood Oath"),
	AshenGrace UMETA(DisplayName="Ashen Grace"),
	RuneInsight UMETA(DisplayName="Rune Insight")
};

/**
 * W01-only exploration event placed between the arrival camp and Blackthorn courtyard.
 *
 * The altar is a self-contained interactable: it owns its native Chinese choice UI,
 * presentation, one-shot state, and delayed application of the chosen combat boon.
 * It deliberately depends only on public character/combat component APIs, so the
 * world layout can spawn it without changing shared input, HUD, or combat flow.
 */
UCLASS()
class WORLDWALKERPROTOTYPE_API AFantasyFateAltar
	: public AActor
	, public IWorldWalkerInteractable
{
	GENERATED_BODY()

public:
	AFantasyFateAltar();

	/** Convenience entry point for W01 layout code. Returns nullptr when World is invalid. */
	static AFantasyFateAltar* SpawnConfigured(
		UWorld* World,
		const FTransform& SpawnTransform,
		FName EventId = FName(TEXT("AshenFateAltar")),
		const FText& Title = FText::GetEmpty(),
		const FText& Lore = FText::GetEmpty());

	/** Safe both before BeginPlay (deferred spawn) and after a normal SpawnActor call. */
	UFUNCTION(BlueprintCallable, Category="World Walker|W01|Exploration")
	void ConfigureAltar(
		FName InEventId,
		const FText& InTitle = FText::GetEmpty(),
		const FText& InLore = FText::GetEmpty());

	UFUNCTION(BlueprintPure, Category="World Walker|W01|Exploration")
	bool IsResolved() const { return bResolved; }

	UFUNCTION(BlueprintPure, Category="World Walker|W01|Exploration")
	EFantasyFateAltarChoice GetChosenOutcome() const { return ChosenOutcome; }

	virtual bool CanInteract_Implementation(const AWorldWalkerCharacter* InteractingCharacter) const override;
	virtual void Interact_Implementation(AWorldWalkerCharacter* InteractingCharacter) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void BeginEvent(AWorldWalkerCharacter* InteractingCharacter);
	void HandleChoiceSelected(EFantasyFateAltarChoice Choice);
	void HandleWidgetClosed();
	void FinishEventUI();
	void WatchForBattleStart();
	void TryApplyPendingBattleBoon();
	void ApplyResolvedPresentation(EFantasyFateAltarChoice Choice);
	void RefreshPrompt();
	void SetPromptVisible(bool bVisible);
	void ApplyMeshTint(UStaticMeshComponent* Mesh, const FLinearColor& Color);
	FString BuildResolutionText(EFantasyFateAltarChoice Choice, int32 ImmediateAmount) const;
	static const TCHAR* ChoiceToLogName(EFantasyFateAltarChoice Choice);

	UPROPERTY(VisibleAnywhere, Category="Altar")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category="Altar")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere, Category="Altar")
	TObjectPtr<UStaticMeshComponent> StepMesh;

	UPROPERTY(VisibleAnywhere, Category="Altar")
	TObjectPtr<UStaticMeshComponent> MonolithMesh;

	UPROPERTY(VisibleAnywhere, Category="Altar")
	TObjectPtr<UStaticMeshComponent> CrownMesh;

	UPROPERTY(VisibleAnywhere, Category="Altar")
	TObjectPtr<USceneComponent> RunePivot;

	UPROPERTY(VisibleAnywhere, Category="Altar")
	TObjectPtr<UStaticMeshComponent> RuneShardA;

	UPROPERTY(VisibleAnywhere, Category="Altar")
	TObjectPtr<UStaticMeshComponent> RuneShardB;

	UPROPERTY(VisibleAnywhere, Category="Altar")
	TObjectPtr<UStaticMeshComponent> RuneShardC;

	UPROPERTY(VisibleAnywhere, Category="Altar")
	TObjectPtr<UPointLightComponent> FateLight;

	UPROPERTY(VisibleAnywhere, Category="Interaction")
	TObjectPtr<UWidgetComponent> PromptWidgetComponent;

	UPROPERTY(Transient)
	TObjectPtr<UFantasyFateAltarWidget> EventWidget;

	UPROPERTY(Transient)
	TObjectPtr<AWorldWalkerCharacter> ActiveInteractingPlayer;

	UPROPERTY(Transient)
	TObjectPtr<AWorldWalkerCharacter> BoonRecipient;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MonolithMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RuneMaterials;

	UPROPERTY(EditAnywhere, Category="Event")
	FName EventId = TEXT("AshenFateAltar");

	UPROPERTY(EditAnywhere, Category="Event")
	FText EventTitle = FText::FromString(TEXT("灰烬命运碑"));

	UPROPERTY(EditAnywhere, Category="Event", meta=(MultiLine="true"))
	FText EventLore = FText::FromString(
		TEXT("焦黑的石碑仍有心跳般的微光。三枚古老符文等待你刻下唯一的誓言；一旦选择，命运便不会回头。"));

	FTimerHandle BattleWatchTimer;
	EFantasyFateAltarChoice ChosenOutcome = EFantasyFateAltarChoice::None;
	int32 PendingValor = 0;
	int32 PendingBlock = 0;
	bool bResolved = false;
	bool bBattleBoonApplied = false;
	bool bEventUIActive = false;
	bool bEndingPlay = false;
};
