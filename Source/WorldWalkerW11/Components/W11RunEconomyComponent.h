#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/W11Types.h"
#include "W11RunEconomyComponent.generated.h"

class UW11AttributeComponent;
class UW11ManualDefinition;
class UW11TreasureDefinition;
class UW11ShopServiceDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FW11EconomyChanged);

UCLASS(ClassGroup=(W11), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class WORLDWALKERW11_API UW11RunEconomyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UW11RunEconomyComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Economy")
	void InitializeRun(int32 InManualSlots = 6, int32 InTreasureSlots = 4);

	void RestoreSafeNodeSnapshot(
		int32 InSpiritStones,
		int32 InCultivationLevel,
		int32 InCultivationExperience,
		int32 InExperienceToNextLevel,
		const TArray<FW11OwnedManual>& InOwnedManuals,
		const TArray<FName>& InGrantedBehaviors,
		const TArray<FName>& InOwnedTreasures,
		int32 InManualSlots,
		int32 InTreasureSlots);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Economy")
	void AddSpiritStones(int32 Amount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Economy")
	bool SpendSpiritStones(int32 Amount);

	/** Returns the number of new cultivation choices earned. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Cultivation")
	int32 AddCultivationExperience(int32 Amount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Economy")
	bool PurchaseManual(const UW11ManualDefinition* Manual, int32 Price);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Economy")
	bool PurchaseTreasure(const UW11TreasureDefinition* Treasure, int32 Price);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="W11|Economy")
	bool PurchaseService(const UW11ShopServiceDefinition* Service, int32 Price);

	UFUNCTION(BlueprintPure, Category="W11|Economy")
	int32 GetSpiritStones() const { return SpiritStones; }

	UFUNCTION(BlueprintPure, Category="W11|Cultivation")
	int32 GetCultivationLevel() const { return CultivationLevel; }

	UFUNCTION(BlueprintPure, Category="W11|Cultivation")
	int32 GetCultivationExperience() const { return CultivationExperience; }

	UFUNCTION(BlueprintPure, Category="W11|Cultivation")
	int32 GetExperienceToNextLevel() const { return ExperienceToNextLevel; }

	UFUNCTION(BlueprintPure, Category="W11|Economy")
	const TArray<FW11OwnedManual>& GetOwnedManuals() const { return OwnedManuals; }

	UFUNCTION(BlueprintPure, Category="W11|Economy")
	const TArray<FName>& GetOwnedTreasures() const { return OwnedTreasures; }

	UFUNCTION(BlueprintPure, Category="W11|Economy")
	int32 GetManualSlots() const { return ManualSlots; }

	UFUNCTION(BlueprintPure, Category="W11|Manuals")
	bool HasBehavior(FName BehaviorId) const { return GrantedBehaviors.Contains(BehaviorId); }

	UFUNCTION(BlueprintPure, Category="W11|Manuals")
	const TArray<FName>& GetGrantedBehaviors() const { return GrantedBehaviors; }

	UFUNCTION(BlueprintPure, Category="W11|Economy")
	int32 GetTreasureSlots() const { return TreasureSlots; }

	UFUNCTION(BlueprintPure, Category="W11|Economy")
	bool OwnsTreasure(FName TreasureId) const;

	UPROPERTY(BlueprintAssignable)
	FW11EconomyChanged OnEconomyChanged;

private:
	UFUNCTION()
	void OnRep_Economy();

	UW11AttributeComponent* ResolveAttributes() const;
	bool CanAddManual(const UW11ManualDefinition* Manual, int32& ExistingIndex) const;
	bool CanAddTreasure(const UW11TreasureDefinition* Treasure) const;
	void ApplyManualRank(const UW11ManualDefinition* Manual, EW11ComprehensionRank Rank);

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_Economy, Category="W11|Economy")
	int32 SpiritStones = 0;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_Economy, Category="W11|Cultivation")
	int32 CultivationLevel = 1;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_Economy, Category="W11|Cultivation")
	int32 CultivationExperience = 0;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_Economy, Category="W11|Cultivation")
	int32 ExperienceToNextLevel = 100;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_Economy, Category="W11|Economy")
	TArray<FW11OwnedManual> OwnedManuals;

	UPROPERTY(Replicated, VisibleAnywhere, Category="W11|Manuals")
	TArray<FName> GrantedBehaviors;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_Economy, Category="W11|Economy")
	TArray<FName> OwnedTreasures;

	UPROPERTY(VisibleAnywhere, Replicated, Category="W11|Economy")
	int32 ManualSlots = 6;

	UPROPERTY(VisibleAnywhere, Replicated, Category="W11|Economy")
	int32 TreasureSlots = 4;
};
