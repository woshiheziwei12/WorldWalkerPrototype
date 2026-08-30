#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "W11ArenaBackdrop.generated.h"

class UPaperSpriteComponent;
class USceneComponent;

/** Local-only seamless painted arena surface covering the engine template grid. */
UCLASS(NotBlueprintable)
class WORLDWALKERW11_API AW11ArenaBackdrop : public AActor
{
	GENERATED_BODY()

public:
	AW11ArenaBackdrop();

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> ArenaSprite;

	/** 6x6 mirrored high-density tiles supply native detail beneath the arena-line overlay. */
	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UPaperSpriteComponent>> ArenaTiles;
};
