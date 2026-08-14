#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/WorldWalkerInteractable.h"
#include "WorldPortal.generated.h"

class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UWorldDefinition;

UCLASS()
class WORLDWALKERPROTOTYPE_API AWorldPortal
	: public AActor
	, public IWorldWalkerInteractable
{
	GENERATED_BODY()

public:
	AWorldPortal();

	void ConfigurePortal(UWorldDefinition* InDestinationWorld);

	virtual bool CanInteract_Implementation(const AWorldWalkerCharacter* InteractingCharacter) const override;
	virtual void Interact_Implementation(AWorldWalkerCharacter* InteractingCharacter) override;

protected:
	virtual void BeginPlay() override;

private:
	void RefreshPresentation();

	UPROPERTY(VisibleAnywhere, Category="Portal")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category="Portal")
	TObjectPtr<UStaticMeshComponent> LeftPillar;

	UPROPERTY(VisibleAnywhere, Category="Portal")
	TObjectPtr<UStaticMeshComponent> RightPillar;

	UPROPERTY(VisibleAnywhere, Category="Portal")
	TObjectPtr<UStaticMeshComponent> TopBeam;

	UPROPERTY(VisibleAnywhere, Category="Portal")
	TObjectPtr<UStaticMeshComponent> PortalSurface;

	UPROPERTY(VisibleAnywhere, Category="Portal")
	TObjectPtr<UTextRenderComponent> PortalLabel;

	UPROPERTY(Transient)
	TObjectPtr<UWorldDefinition> DestinationWorld;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PortalMaterial;
};
