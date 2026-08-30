#include "Game/W11PlayerState.h"

#include "Components/W11AttributeComponent.h"
#include "Components/W11RunEconomyComponent.h"
#include "Net/UnrealNetwork.h"

AW11PlayerState::AW11PlayerState()
{
	bReplicates = true;
	SetNetUpdateFrequency(10.0f);
	AttributeComponent = CreateDefaultSubobject<UW11AttributeComponent>(TEXT("W11Attributes"));
	EconomyComponent = CreateDefaultSubobject<UW11RunEconomyComponent>(TEXT("W11RunEconomy"));
}

void AW11PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AW11PlayerState, CultivationOffers, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AW11PlayerState, ShopOffers, COND_OwnerOnly);
	DOREPLIFETIME(AW11PlayerState, PendingCultivationSelections);
	DOREPLIFETIME(AW11PlayerState, bReadyForNextStage);
	DOREPLIFETIME(AW11PlayerState, SelectedHeroId);
	DOREPLIFETIME(AW11PlayerState, SelectedSectId);
	DOREPLIFETIME(AW11PlayerState, InitialAbilityId);
	DOREPLIFETIME(AW11PlayerState, ActiveAbilitySlots);
	DOREPLIFETIME(AW11PlayerState, UnlockedAbilities);
}

void AW11PlayerState::SetCultivationOffers(const TArray<FW11CultivationOffer>& InOffers)
{
	if (HasAuthority())
	{
		CultivationOffers = InOffers;
		ForceNetUpdate();
	}
}

void AW11PlayerState::SetShopOffers(const TArray<FW11ShopOffer>& InOffers)
{
	if (HasAuthority())
	{
		ShopOffers = InOffers;
		ForceNetUpdate();
	}
}

void AW11PlayerState::SetPendingCultivationSelections(const int32 Count)
{
	if (HasAuthority())
	{
		PendingCultivationSelections = FMath::Max(0, Count);
		ForceNetUpdate();
	}
}

void AW11PlayerState::ConsumeCultivationSelection()
{
	if (HasAuthority())
	{
		PendingCultivationSelections = FMath::Max(0, PendingCultivationSelections - 1);
		CultivationOffers.Reset();
		ForceNetUpdate();
	}
}

void AW11PlayerState::MarkShopOfferPurchased(const int32 OfferIndex)
{
	if (HasAuthority() && ShopOffers.IsValidIndex(OfferIndex))
	{
		ShopOffers[OfferIndex].bPurchased = true;
		ForceNetUpdate();
	}
}

void AW11PlayerState::SetReadyForNextStage(const bool bReady)
{
	if (HasAuthority())
	{
		bReadyForNextStage = bReady;
		ForceNetUpdate();
	}
}

void AW11PlayerState::ClearTransientOffers()
{
	if (HasAuthority())
	{
		CultivationOffers.Reset();
		ShopOffers.Reset();
		PendingCultivationSelections = 0;
		bReadyForNextStage = false;
		ForceNetUpdate();
	}
}

void AW11PlayerState::SetCharacterCreationSelection(
	const FName HeroId,
	const FName SectId,
	const FName AbilityId)
{
	if (HasAuthority())
	{
		SelectedHeroId = HeroId;
		SelectedSectId = SectId;
		InitialAbilityId = AbilityId;
		ActiveAbilitySlots.Init(NAME_None, 4);
		UnlockedAbilities.Reset();
		if (!AbilityId.IsNone())
		{
			FW11EquippedAbility& Unlocked = UnlockedAbilities.AddDefaulted_GetRef();
			Unlocked.AbilityId = AbilityId;
			Unlocked.Level = 1;
			ActiveAbilitySlots[0] = AbilityId;
		}
		ForceNetUpdate();
	}
}

void AW11PlayerState::ClearCharacterCreationSelection()
{
	if (HasAuthority())
	{
		SelectedHeroId = NAME_None;
		SelectedSectId = NAME_None;
		InitialAbilityId = NAME_None;
		ActiveAbilitySlots.Init(NAME_None, 4);
		UnlockedAbilities.Reset();
		ForceNetUpdate();
	}
}

void AW11PlayerState::RestoreSafeNodeAbilitySnapshot(
	const TArray<FName>& InActiveAbilitySlots,
	const TArray<FW11EquippedAbility>& InUnlockedAbilities)
{
	if (!HasAuthority())
	{
		return;
	}
	UnlockedAbilities.Reset();
	TSet<FName> SeenAbilities;
	for (const FW11EquippedAbility& Entry : InUnlockedAbilities)
	{
		if (Entry.AbilityId.IsNone() || SeenAbilities.Contains(Entry.AbilityId))
		{
			continue;
		}
		FW11EquippedAbility& Restored = UnlockedAbilities.AddDefaulted_GetRef();
		Restored.AbilityId = Entry.AbilityId;
		Restored.Level = FMath::Max(1, Entry.Level);
		SeenAbilities.Add(Entry.AbilityId);
	}
	ActiveAbilitySlots.Init(NAME_None, 4);
	TSet<FName> EquippedAbilities;
	for (int32 Index = 0; Index < FMath::Min(4, InActiveAbilitySlots.Num()); ++Index)
	{
		const FName AbilityId = InActiveAbilitySlots[Index];
		if (!AbilityId.IsNone() && SeenAbilities.Contains(AbilityId)
			&& !EquippedAbilities.Contains(AbilityId))
		{
			ActiveAbilitySlots[Index] = AbilityId;
			EquippedAbilities.Add(AbilityId);
		}
	}
	ForceNetUpdate();
}

FName AW11PlayerState::GetActiveAbilityId(const EW11AbilitySlot Slot) const
{
	const int32 Index = static_cast<int32>(Slot) - static_cast<int32>(EW11AbilitySlot::Active1);
	return ActiveAbilitySlots.IsValidIndex(Index) ? ActiveAbilitySlots[Index] : NAME_None;
}

int32 AW11PlayerState::GetActiveAbilityLevel(const EW11AbilitySlot Slot) const
{
	const FName AbilityId = GetActiveAbilityId(Slot);
	const FW11EquippedAbility* Found = UnlockedAbilities.FindByPredicate([AbilityId](const FW11EquippedAbility& Entry)
	{
		return Entry.AbilityId == AbilityId;
	});
	return Found ? FMath::Max(1, Found->Level) : 0;
}

bool AW11PlayerState::IsAbilityUnlocked(const FName AbilityId) const
{
	return !AbilityId.IsNone() && UnlockedAbilities.ContainsByPredicate([AbilityId](const FW11EquippedAbility& Entry)
	{
		return Entry.AbilityId == AbilityId;
	});
}

bool AW11PlayerState::UnlockOrUpgradeAbility(const FName AbilityId, const int32 MaxLevel)
{
	if (!HasAuthority() || AbilityId.IsNone())
	{
		return false;
	}
	FW11EquippedAbility* Found = UnlockedAbilities.FindByPredicate([AbilityId](const FW11EquippedAbility& Entry)
	{
		return Entry.AbilityId == AbilityId;
	});
	if (Found)
	{
		if (Found->Level >= FMath::Max(1, MaxLevel))
		{
			return false;
		}
		++Found->Level;
	}
	else
	{
		FW11EquippedAbility& Added = UnlockedAbilities.AddDefaulted_GetRef();
		Added.AbilityId = AbilityId;
		Added.Level = 1;
		if (ActiveAbilitySlots.Num() != 4)
		{
			ActiveAbilitySlots.Init(NAME_None, 4);
		}
		const int32 EmptyIndex = ActiveAbilitySlots.IndexOfByKey(NAME_None);
		if (EmptyIndex != INDEX_NONE)
		{
			ActiveAbilitySlots[EmptyIndex] = AbilityId;
		}
	}
	ForceNetUpdate();
	return true;
}

bool AW11PlayerState::EquipActiveAbility(const EW11AbilitySlot Slot, const FName AbilityId)
{
	const int32 Index = static_cast<int32>(Slot) - static_cast<int32>(EW11AbilitySlot::Active1);
	if (!HasAuthority() || Index < 0 || Index >= 4 || !IsAbilityUnlocked(AbilityId))
	{
		return false;
	}
	if (ActiveAbilitySlots.Num() != 4)
	{
		ActiveAbilitySlots.Init(NAME_None, 4);
	}
	for (FName& EquippedId : ActiveAbilitySlots)
	{
		if (EquippedId == AbilityId)
		{
			EquippedId = NAME_None;
		}
	}
	ActiveAbilitySlots[Index] = AbilityId;
	ForceNetUpdate();
	return true;
}
