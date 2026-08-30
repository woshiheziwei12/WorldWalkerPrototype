#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "W11HUD.generated.h"

class UW11CombatHUDWidget;

UCLASS()
class WORLDWALKERW11_API AW11HUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UW11CombatHUDWidget> CombatHUDWidget;
};
