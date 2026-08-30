#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Core/W11Types.h"
#include "W11PlayerState.generated.h"

class UW11AttributeComponent;
class UW11RunEconomyComponent;

UCLASS()
class WORLDWALKERW11_API AW11PlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AW11PlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="W11|Player")
	UW11AttributeComponent* GetAttributeComponent() const { return AttributeComponent; }

	UFUNCTION(BlueprintPure, Category="W11|Player")
	UW11RunEconomyComponent* GetEconomyComponent() const { return EconomyComponent; }

	UFUNCTION(BlueprintPure, Category="W11|Cultivation")
	const TArray<FW11CultivationOffer>& GetCultivationOffers() const { return CultivationOffers; }

	UFUNCTION(BlueprintPure, Category="W11|Shop")
	const TArray<FW11ShopOffer>& GetShopOffers() const { return ShopOffers; }

	UFUNCTION(BlueprintPure, Category="W11|Cultivation")
	int32 GetPendingCultivationSelections() const { return PendingCultivationSelections; }

	UFUNCTION(BlueprintPure, Category="W11|Run")
	bool IsReadyForNextStage() const { return bReadyForNextStage; }

	UFUNCTION(BlueprintPure, Category="W11|Character Creation")
	FName GetSelectedHeroId() const { return SelectedHeroId; }

	UFUNCTION(BlueprintPure, Category="W11|Character Creation")
	FName GetSelectedSectId() const { return SelectedSectId; }

	UFUNCTION(BlueprintPure, Category="W11|Character Creation")
	FName GetInitialAbilityId() const { return InitialAbilityId; }

	UFUNCTION(BlueprintPure, Category="W11|Abilities")
	FName GetActiveAbilityId(EW11AbilitySlot Slot) const;

	UFUNCTION(BlueprintPure, Category="W11|Abilities")
	int32 GetActiveAbilityLevel(EW11AbilitySlot Slot) const;

	UFUNCTION(BlueprintPure, Category="W11|Abilities")
	const TArray<FName>& GetActiveAbilitySlots() const { return ActiveAbilitySlots; }

	UFUNCTION(BlueprintPure, Category="W11|Abilities")
	const TArray<FW11EquippedAbility>& GetUnlockedAbilities() const { return UnlockedAbilities; }

	bool UnlockOrUpgradeAbility(FName AbilityId, int32 MaxLevel = 5);
	bool EquipActiveAbility(EW11AbilitySlot Slot, FName AbilityId);
	bool IsAbilityUnlocked(FName AbilityId) const;

	UFUNCTION(BlueprintPure, Category="W11|Character Creation")
	bool HasCompletedCharacterCreation() const
	{
		return !SelectedHeroId.IsNone() && !SelectedSectId.IsNone() && !InitialAbilityId.IsNone();
	}

	void SetCultivationOffers(const TArray<FW11CultivationOffer>& InOffers);
	void SetShopOffers(const TArray<FW11ShopOffer>& InOffers);
	void SetPendingCultivationSelections(int32 Count);
	void ConsumeCultivationSelection();
	void MarkShopOfferPurchased(int32 OfferIndex);
	void SetReadyForNextStage(bool bReady);
	void ClearTransientOffers();
	void SetCharacterCreationSelection(FName HeroId, FName SectId, FName AbilityId);
	void ClearCharacterCreationSelection();
	void RestoreSafeNodeAbilitySnapshot(
		const TArray<FName>& InActiveAbilitySlots,
		const TArray<FW11EquippedAbility>& InUnlockedAbilities);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="W11|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UW11AttributeComponent> AttributeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="W11|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UW11RunEconomyComponent> EconomyComponent;

	UPROPERTY(Replicated, VisibleAnywhere, Category="W11|Cultivation")
	TArray<FW11CultivationOffer> CultivationOffers;

	UPROPERTY(Replicated, VisibleAnywhere, Category="W11|Shop")
	TArray<FW11ShopOffer> ShopOffers;

	UPROPERTY(Replicated, VisibleAnywhere, Category="W11|Cultivation")
	int32 PendingCultivationSelections = 0;

	UPROPERTY(Replicated, VisibleAnywhere, Category="W11|Run")
	bool bReadyForNextStage = false;

	UPROPERTY(Replicated, VisibleAnywhere, Category="W11|Character Creation")
	FName SelectedHeroId = NAME_None;

	UPROPERTY(Replicated, VisibleAnywhere, Category="W11|Character Creation")
	FName SelectedSectId = NAME_None;

	UPROPERTY(Replicated, VisibleAnywhere, Category="W11|Character Creation")
	FName InitialAbilityId = NAME_None;

	/** Active1..Active4, fixed to four entries on authority. */
	UPROPERTY(Replicated, VisibleAnywhere, Category="W11|Abilities")
	TArray<FName> ActiveAbilitySlots;

	UPROPERTY(Replicated, VisibleAnywhere, Category="W11|Abilities")
	TArray<FW11EquippedAbility> UnlockedAbilities;
};
