#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/W11Types.h"
#include "W11StatLibrary.generated.h"

UCLASS()
class WORLDWALKERW11_API UW11StatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="W11|Stats")
	static float GetEffectiveLuck(float Luck);

	UFUNCTION(BlueprintPure, Category="W11|Stats")
	static float GetArmorDamageReduction(float Armor);

	UFUNCTION(BlueprintPure, Category="W11|Stats")
	static float GetFinalAreaRadius(float BaseRadius, float AreaScale);

	UFUNCTION(BlueprintPure, Category="W11|Stats")
	static float GetFinalCooldown(float BaseCooldown, float AbilityHaste);

	static void ApplyModifier(FW11StatBlock& Stats, const FW11StatModifier& Modifier);
	static void Sanitize(FW11StatBlock& Stats);
};
