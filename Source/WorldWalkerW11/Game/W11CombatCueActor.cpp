#include "Game/W11CombatCueActor.h"

#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInterface.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "UObject/ConstructorHelpers.h"

AW11CombatCueActor::AW11CombatCueActor()
{
	bReplicates = false;
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("W11CueRoot"));
	SetRootComponent(SceneRoot);
	ImpactComponent = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("W11Impact"));
	ImpactComponent->SetupAttachment(SceneRoot);
	ImpactComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ImpactComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, -90.0f));
	ImpactComponent->SetLooping(false);
	ImpactComponent->SetTranslucentSortPriority(80);
	ImpactComponent->SetCastShadow(false);
	ImpactComponent->SetReceivesDecals(false);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TranslucentSpriteMaterial(
		TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	if (TranslucentSpriteMaterial.Succeeded())
	{
		ImpactComponent->SetMaterial(0, TranslucentSpriteMaterial.Object);
	}
	NumberComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("W11CombatNumber"));
	NumberComponent->SetupAttachment(SceneRoot);
	NumberComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NumberComponent->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	NumberComponent->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	NumberComponent->SetWorldSize(30.0f);
	NumberComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 42.0f));
	NumberComponent->SetRelativeRotation(FRotator(90.0f, 0.0f, 90.0f));
	NumberComponent->SetTranslucentSortPriority(90);
	SetLifeSpan(0.9f);
}

void AW11CombatCueActor::Configure(
	UPaperFlipbook* ImpactFlipbook,
	const FText& Text,
	const FLinearColor& Color,
	const float Scale)
{
	ImpactComponent->SetFlipbook(ImpactFlipbook);
	ImpactComponent->SetRelativeScale3D(FVector(FMath::Max(0.01f, Scale)));
	ImpactComponent->SetVisibility(ImpactFlipbook != nullptr);
	if (ImpactFlipbook)
	{
		ImpactComponent->PlayFromStart();
	}
	NumberComponent->SetText(Text);
	NumberComponent->SetTextRenderColor(Color.ToFColor(true));
	NumberComponent->SetVisibility(!Text.IsEmpty());
}
