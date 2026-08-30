#include "Components/W11RunEconomyComponent.h"

#include "Components/W11AttributeComponent.h"
#include "Data/W11Definitions.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UW11RunEconomyComponent::UW11RunEconomyComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UW11RunEconomyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UW11RunEconomyComponent, SpiritStones);
	DOREPLIFETIME(UW11RunEconomyComponent, CultivationLevel);
	DOREPLIFETIME(UW11RunEconomyComponent, CultivationExperience);
	DOREPLIFETIME(UW11RunEconomyComponent, ExperienceToNextLevel);
	DOREPLIFETIME(UW11RunEconomyComponent, OwnedManuals);
	DOREPLIFETIME(UW11RunEconomyComponent, GrantedBehaviors);
	DOREPLIFETIME(UW11RunEconomyComponent, OwnedTreasures);
	DOREPLIFETIME(UW11RunEconomyComponent, ManualSlots);
	DOREPLIFETIME(UW11RunEconomyComponent, TreasureSlots);
}

void UW11RunEconomyComponent::InitializeRun(const int32 InManualSlots, const int32 InTreasureSlots)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	SpiritStones = 0;
	CultivationLevel = 1;
	CultivationExperience = 0;
	ExperienceToNextLevel = 100;
	OwnedManuals.Reset();
	GrantedBehaviors.Reset();
	OwnedTreasures.Reset();
	ManualSlots = FMath::Max(1, InManualSlots);
	TreasureSlots = FMath::Max(1, InTreasureSlots);
	OnRep_Economy();
}

void UW11RunEconomyComponent::RestoreSafeNodeSnapshot(
	const int32 InSpiritStones,
	const int32 InCultivationLevel,
	const int32 InCultivationExperience,
	const int32 InExperienceToNextLevel,
	const TArray<FW11OwnedManual>& InOwnedManuals,
	const TArray<FName>& InGrantedBehaviors,
	const TArray<FName>& InOwnedTreasures,
	const int32 InManualSlots,
	const int32 InTreasureSlots)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	SpiritStones = FMath::Max(0, InSpiritStones);
	CultivationLevel = FMath::Max(1, InCultivationLevel);
	CultivationExperience = FMath::Max(0, InCultivationExperience);
	ExperienceToNextLevel = FMath::Max(1, InExperienceToNextLevel);
	ManualSlots = FMath::Max(1, InManualSlots);
	TreasureSlots = FMath::Max(1, InTreasureSlots);
	OwnedManuals = InOwnedManuals;
	OwnedManuals.SetNum(FMath::Min(OwnedManuals.Num(), ManualSlots));
	GrantedBehaviors.Reset();
	for (const FName BehaviorId : InGrantedBehaviors)
	{
		if (!BehaviorId.IsNone())
		{
			GrantedBehaviors.AddUnique(BehaviorId);
		}
	}
	GrantedBehaviors.Sort([](const FName Left, const FName Right)
	{
		return Left.LexicalLess(Right);
	});
	OwnedTreasures.Reset();
	for (const FName TreasureId : InOwnedTreasures)
	{
		if (!TreasureId.IsNone())
		{
			OwnedTreasures.AddUnique(TreasureId);
		}
	}
	OwnedTreasures.SetNum(FMath::Min(OwnedTreasures.Num(), TreasureSlots));
	OnRep_Economy();
}

void UW11RunEconomyComponent::AddSpiritStones(const int32 Amount)
{
	if (GetOwner() && GetOwner()->HasAuthority() && Amount > 0)
	{
		SpiritStones = FMath::Max(0, SpiritStones + Amount);
		OnRep_Economy();
	}
}

bool UW11RunEconomyComponent::SpendSpiritStones(const int32 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount < 0 || SpiritStones < Amount)
	{
		return false;
	}
	SpiritStones -= Amount;
	OnRep_Economy();
	return true;
}

int32 UW11RunEconomyComponent::AddCultivationExperience(const int32 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0)
	{
		return 0;
	}

	CultivationExperience += Amount;
	int32 EarnedChoices = 0;
	while (CultivationExperience >= ExperienceToNextLevel)
	{
		CultivationExperience -= ExperienceToNextLevel;
		++CultivationLevel;
		++EarnedChoices;
		ExperienceToNextLevel = 100 + (CultivationLevel - 1) * 50;
	}
	OnRep_Economy();
	return EarnedChoices;
}

bool UW11RunEconomyComponent::PurchaseManual(const UW11ManualDefinition* Manual, const int32 Price)
{
	int32 ExistingIndex = INDEX_NONE;
	if (!CanAddManual(Manual, ExistingIndex) || Price < 0 || SpiritStones < Price)
	{
		return false;
	}

	SpiritStones -= Price;
	EW11ComprehensionRank NewRank = EW11ComprehensionRank::Initiate;
	if (ExistingIndex != INDEX_NONE)
	{
		FW11OwnedManual& Owned = OwnedManuals[ExistingIndex];
		NewRank = static_cast<EW11ComprehensionRank>(static_cast<uint8>(Owned.Rank) + 1);
		Owned.Rank = NewRank;
		Owned.InvestedSpiritStones += Price;
	}
	else
	{
		FW11OwnedManual& Owned = OwnedManuals.AddDefaulted_GetRef();
		Owned.ManualId = Manual->DefinitionId;
		Owned.Rank = NewRank;
		Owned.InvestedSpiritStones = Price;
	}
	ApplyManualRank(Manual, NewRank);
	OnRep_Economy();
	return true;
}

bool UW11RunEconomyComponent::PurchaseTreasure(const UW11TreasureDefinition* Treasure, const int32 Price)
{
	if (!CanAddTreasure(Treasure) || Price < 0 || SpiritStones < Price)
	{
		return false;
	}
	SpiritStones -= Price;
	OwnedTreasures.Add(Treasure->DefinitionId);
	if (UW11AttributeComponent* Attributes = ResolveAttributes())
	{
		for (const FW11StatModifier& Modifier : Treasure->StatModifiers)
		{
			Attributes->ApplyPermanentModifier(Modifier);
		}
	}
	OnRep_Economy();
	return true;
}

bool UW11RunEconomyComponent::PurchaseService(const UW11ShopServiceDefinition* Service, const int32 Price)
{
	if (!Service || Price < 0 || SpiritStones < Price)
	{
		return false;
	}
	UW11AttributeComponent* Attributes = ResolveAttributes();
	if (!Attributes)
	{
		return false;
	}

	SpiritStones -= Price;
	for (const FW11StatModifier& Modifier : Service->PermanentModifiers)
	{
		Attributes->ApplyPermanentModifier(Modifier);
	}
	Attributes->Heal(Attributes->GetStats().MaxHealth * FMath::Max(0.0f, Service->HealFraction));
	Attributes->RestoreMana(Attributes->GetStats().MaxMana * FMath::Max(0.0f, Service->RestoreManaFraction));
	OnRep_Economy();
	return true;
}

bool UW11RunEconomyComponent::OwnsTreasure(const FName TreasureId) const
{
	return OwnedTreasures.Contains(TreasureId);
}

void UW11RunEconomyComponent::OnRep_Economy()
{
	OnEconomyChanged.Broadcast();
}

UW11AttributeComponent* UW11RunEconomyComponent::ResolveAttributes() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UW11AttributeComponent>() : nullptr;
}

bool UW11RunEconomyComponent::CanAddManual(const UW11ManualDefinition* Manual, int32& ExistingIndex) const
{
	ExistingIndex = INDEX_NONE;
	if (!Manual || Manual->DefinitionId.IsNone())
	{
		return false;
	}
	ExistingIndex = OwnedManuals.IndexOfByPredicate([Manual](const FW11OwnedManual& Entry)
	{
		return Entry.ManualId == Manual->DefinitionId;
	});
	if (ExistingIndex != INDEX_NONE)
	{
		return OwnedManuals[ExistingIndex].Rank < EW11ComprehensionRank::Perfected;
	}
	return OwnedManuals.Num() < ManualSlots;
}

bool UW11RunEconomyComponent::CanAddTreasure(const UW11TreasureDefinition* Treasure) const
{
	return Treasure
		&& !Treasure->DefinitionId.IsNone()
		&& OwnedTreasures.Num() < TreasureSlots
		&& (!Treasure->bUnique || !OwnsTreasure(Treasure->DefinitionId));
}

void UW11RunEconomyComponent::ApplyManualRank(
	const UW11ManualDefinition* Manual,
	const EW11ComprehensionRank Rank)
{
	UW11AttributeComponent* Attributes = ResolveAttributes();
	if (!Attributes || !Manual)
	{
		return;
	}
	const FW11ManualRankEffects* Effects = Manual->RankEffects.FindByPredicate([Rank](const FW11ManualRankEffects& Entry)
	{
		return Entry.Rank == Rank;
	});
	if (!Effects)
	{
		return;
	}
	for (const FW11StatModifier& Modifier : Effects->StatModifiers)
	{
		Attributes->ApplyPermanentModifier(Modifier);
	}
	for (const FName BehaviorId : Effects->GrantedBehaviors)
	{
		if (!BehaviorId.IsNone())
		{
			GrantedBehaviors.AddUnique(BehaviorId);
		}
	}
}
