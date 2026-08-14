#include "Cards/Fantasy/FantasyEnemyDefinition.h"

const FPrimaryAssetType UFantasyEnemyDefinition::PrimaryAssetType(TEXT("FantasyEnemyDefinition"));

FPrimaryAssetId UFantasyEnemyDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, EnemyId.IsNone() ? GetFName() : EnemyId);
}

FString FFantasyEnemyIntentStep::BuildPreviewText(const int32 CurrentStrength) const
{
	TArray<FString> Fragments;
	for (const FFantasyCombatEffectSpec& Effect : Effects)
	{
		if (Effect.EffectType == EFantasyCombatEffectType::Damage)
		{
			Fragments.Add(FString::Printf(TEXT("造成 %d 点伤害"), Effect.Magnitude + CurrentStrength));
		}
		else
		{
			Fragments.Add(Effect.BuildRulesFragment());
		}
	}

	return FString::Printf(TEXT("%s：%s"), *DisplayName.ToString(), *FString::Join(Fragments, TEXT("；")));
}
