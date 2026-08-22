#include "Cards/Fantasy/FantasyBlessingDefinition.h"

const FPrimaryAssetType UFantasyBlessingDefinition::PrimaryAssetType(TEXT("FantasyBlessingDefinition"));

FPrimaryAssetId UFantasyBlessingDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, BlessingId.IsNone() ? GetFName() : BlessingId);
}

TArray<FString> UFantasyBlessingDefinition::ValidateDefinition() const
{
	TArray<FString> Errors;
	if (BlessingId.IsNone()) Errors.Add(TEXT("BlessingId is empty."));
	if (DisplayName.IsEmpty()) Errors.Add(TEXT("DisplayName is empty."));
	if (Description.IsEmpty()) Errors.Add(TEXT("Description is empty."));
	if (ShopPrice < 0) Errors.Add(TEXT("ShopPrice cannot be negative."));
	if (Effects.IsEmpty()) Errors.Add(TEXT("Blessing must contain at least one effect."));
	if (!RequiredCardTag.IsNone() && Trigger != EFantasyBlessingTrigger::PlayerCardResolved)
	{
		Errors.Add(TEXT("RequiredCardTag is only valid for PlayerCardResolved."));
	}
	return Errors;
}
