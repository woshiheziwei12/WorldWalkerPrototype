#pragma once

#include "CoreMinimal.h"
#include "Core/W11Types.h"
#include "GameFramework/PlayerController.h"
#include "W11PlayerController.generated.h"

UCLASS()
class WORLDWALKERW11_API AW11PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AW11PlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintCallable, Category="W11|Entry")
	void ShowEntryMenu();

	UFUNCTION(BlueprintCallable, Category="W11|Entry")
	void HideEntryMenu();

	UFUNCTION(BlueprintCallable, Category="W11|Entry")
	void StartSoloRun(FName HeroId, FName SectId);

	UFUNCTION(BlueprintCallable, Category="W11|Entry")
	void ResumeSoloRun();

	UFUNCTION(BlueprintPure, Category="W11|Entry")
	bool CanResumeSoloRun() const;

	UFUNCTION(BlueprintCallable, Category="W11|Entry")
	void HostCoopRun();

	UFUNCTION(BlueprintCallable, Category="W11|Entry")
	void FindCoopRuns();

	UFUNCTION(BlueprintCallable, Category="W11|Entry")
	void JoinCoopRun(int32 ResultIndex);

	UFUNCTION(BlueprintPure, Category="W11|Entry")
	TArray<FString> GetSessionResultLabels() const;

	UFUNCTION(BlueprintCallable, Category="W11|Entry")
	void ReturnToMainWorld();

	UFUNCTION(BlueprintCallable, Category="W11|Entry")
	void QuitGameFromMenu();

	UFUNCTION(BlueprintPure, Category="W11|Audio")
	float GetMenuMusicVolume() const;

	UFUNCTION(BlueprintCallable, Category="W11|Audio")
	void SetMenuMusicVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category="W11|Audio")
	void SetHeroSelectionMusic(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category="W11|Cultivation")
	void SelectCultivationOffer(int32 OfferIndex);

	UFUNCTION(BlueprintCallable, Category="W11|Shop")
	void PurchaseShopOffer(int32 OfferIndex);

	UFUNCTION(BlueprintCallable, Category="W11|Abilities")
	void EquipAbility(EW11AbilitySlot Slot, FName AbilityId);

	UFUNCTION(BlueprintCallable, Category="W11|Run")
	void SetReadyForNextStage(bool bReady);

	UFUNCTION(BlueprintCallable, Category="W11|Run")
	void RequestNewRun();

	UFUNCTION(Client, Reliable)
	void ClientReturnToEntryMenu();

protected:
	void InitializeEntryFlow();
	void StartEntryMusic();
	void SwitchEntryMusic(const TCHAR* MusicAssetPath, float FadeInDuration = 1.2f);
	void StopEntryMusic();
	void HandleChoice(int32 OfferIndex);
	void HandleChoice1();
	void HandleChoice2();
	void HandleChoice3();
	void HandleChoice4();
	void HandleChoice5();
	void HandleChoice6();
	void HandleReady();

	UFUNCTION(Server, Reliable)
	void ServerSelectCultivationOffer(int32 OfferIndex);

	UFUNCTION(Server, Reliable)
	void ServerPurchaseShopOffer(int32 OfferIndex);

	UFUNCTION(Server, Reliable)
	void ServerEquipAbility(EW11AbilitySlot Slot, FName AbilityId);

	UFUNCTION(Server, Reliable)
	void ServerSetReadyForNextStage(bool bReady);

	UFUNCTION(Server, Reliable)
	void ServerStartRun(FName HeroId, FName SectId);

	UFUNCTION(Server, Reliable)
	void ServerResumeRun();

	UFUNCTION(Server, Reliable)
	void ServerRequestNewRun();

private:
	UPROPERTY(Transient)
	TObjectPtr<class UW11EntryMenuWidget> EntryMenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> EntryMusicComponent;

	FString ActiveEntryMusicPath;
};
