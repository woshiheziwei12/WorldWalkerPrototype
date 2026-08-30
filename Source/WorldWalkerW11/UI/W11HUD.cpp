#include "UI/W11HUD.h"

#include "UI/W11CombatHUDWidget.h"
#include "WorldWalkerW11.h"

void AW11HUD::BeginPlay()
{
	Super::BeginPlay();
	if (APlayerController* Controller = GetOwningPlayerController(); Controller && Controller->IsLocalController())
	{
		CombatHUDWidget = CreateWidget<UW11CombatHUDWidget>(Controller, UW11CombatHUDWidget::StaticClass());
		if (CombatHUDWidget)
		{
			CombatHUDWidget->AddToViewport(20);
			UE_LOG(LogWorldWalkerW11, Display, TEXT("W11_COMBAT_HUD_READY Controller=%s Widget=%s"),
				*GetNameSafe(Controller), *GetNameSafe(CombatHUDWidget));
		}
		else
		{
			UE_LOG(LogWorldWalkerW11, Error, TEXT("W11_COMBAT_HUD_CREATE_FAILED Controller=%s"), *GetNameSafe(Controller));
		}
	}
}

void AW11HUD::DrawHUD()
{
	Super::DrawHUD();
}
