#include "Game/W11ArenaBackdrop.h"

#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "UObject/ConstructorHelpers.h"

AW11ArenaBackdrop::AW11ArenaBackdrop()
{
	bReplicates = false;
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ArenaBackdropRoot"));
	SetRootComponent(SceneRoot);

	ArenaSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("ArenaSprite"));
	ArenaSprite->SetupAttachment(SceneRoot);
	ArenaSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArenaSprite->SetRelativeRotation(FRotator(0.0f, 90.0f, -90.0f));
	// The authored 8192 x 8192 square is scaled uniformly to 6400 x 6400 world
	// units. At the first chapter's 3000 uu camera width, the visible crop maps
	// one source pixel to roughly one native-4K screen pixel while retaining
	// four-way overscan around the 3200 x 3200 combat bounds.
	ArenaSprite->SetRelativeScale3D(FVector(0.78125f, 1.0f, 0.78125f));
	ArenaSprite->SetTranslucentSortPriority(-100);
	ArenaSprite->SetSpriteColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.34f));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TranslucentSpriteMaterial(
		TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	if (TranslucentSpriteMaterial.Succeeded())
	{
		ArenaSprite->SetMaterial(0, TranslucentSpriteMaterial.Object);
	}

	static ConstructorHelpers::FObjectFinder<UPaperSprite> BackdropSprite(
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Prototype/S_W11_FirstArena_Continuous.S_W11_FirstArena_Continuous"));
	if (BackdropSprite.Succeeded())
	{
		ArenaSprite->SetSprite(BackdropSprite.Object);
	}

	static ConstructorHelpers::FObjectFinder<UPaperSprite> JadeTileSprite(
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Art/Prototype/S_W11_Arena_JadeTile.S_W11_Arena_JadeTile"));
	constexpr int32 TileCountPerAxis = 6;
	constexpr float ArenaWorldSize = 6400.0f;
	constexpr float TileSourceSize = 1254.0f;
	constexpr float TileWorldSize = ArenaWorldSize / static_cast<float>(TileCountPerAxis);
	constexpr float TileScale = TileWorldSize / TileSourceSize;
	ArenaTiles.Reserve(TileCountPerAxis * TileCountPerAxis);
	for (int32 TileY = 0; TileY < TileCountPerAxis; ++TileY)
	{
		for (int32 TileX = 0; TileX < TileCountPerAxis; ++TileX)
		{
			UPaperSpriteComponent* Tile = CreateDefaultSubobject<UPaperSpriteComponent>(
				*FString::Printf(TEXT("ArenaJadeTile_%02d_%02d"), TileX, TileY));
			Tile->SetupAttachment(SceneRoot);
			Tile->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Tile->SetRelativeRotation(FRotator(0.0f, 90.0f, -90.0f));
			Tile->SetRelativeLocation(FVector(
				-ArenaWorldSize * 0.5f + (static_cast<float>(TileX) + 0.5f) * TileWorldSize,
				-ArenaWorldSize * 0.5f + (static_cast<float>(TileY) + 0.5f) * TileWorldSize,
				-1.0f));
			// Alternating mirrors make every shared edge meet the exact same source
			// edge, so the generated tile cannot expose a grid seam in motion.
			Tile->SetRelativeScale3D(FVector(
				(TileX % 2 == 0 ? 1.0f : -1.0f) * TileScale,
				1.0f,
				(TileY % 2 == 0 ? 1.0f : -1.0f) * TileScale));
			Tile->SetTranslucentSortPriority(-110);
			if (JadeTileSprite.Succeeded())
			{
				Tile->SetSprite(JadeTileSprite.Object);
			}
			ArenaTiles.Add(Tile);
		}
	}
}
