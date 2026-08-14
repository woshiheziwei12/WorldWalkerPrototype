#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WorldWalkerInteractable.generated.h"

class AWorldWalkerCharacter;

UINTERFACE(MinimalAPI, Blueprintable)
class UWorldWalkerInteractable : public UInterface
{
	GENERATED_BODY()
};

class WORLDWALKERPROTOTYPE_API IWorldWalkerInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	bool CanInteract(const AWorldWalkerCharacter* InteractingCharacter) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	void Interact(AWorldWalkerCharacter* InteractingCharacter);
};
