#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "W11CombatCueActor.generated.h"

class UPaperFlipbook;
class UPaperFlipbookComponent;
class UTextRenderComponent;

/** Local-only short-lived impact and combat-number presentation. */
UCLASS(NotPlaceable, Transient)
class WORLDWALKERW11_API AW11CombatCueActor : public AActor
{
	GENERATED_BODY()

public:
	AW11CombatCueActor();
	void Configure(UPaperFlipbook* ImpactFlipbook, const FText& Text, const FLinearColor& Color, float Scale);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperFlipbookComponent> ImpactComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> NumberComponent;
};
