#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FantasyWorldChoiceActor.generated.h"

class AWorldWalkerGameModeBase;
class UBoxComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UWidgetComponent;

UENUM()
enum class EFantasyWorldChoiceKind : uint8
{
	Route,
	Event
};

/**
 * A physical W01 choice marker. Walking through its trigger selects a route or
 * event outcome; no full-screen card/choice page is involved.
 */
UCLASS()
class WORLDWALKERPROTOTYPE_API AFantasyWorldChoiceActor : public AActor
{
	GENERATED_BODY()

public:
	AFantasyWorldChoiceActor();

	void ConfigureChoice(
		AWorldWalkerGameModeBase* InGameMode,
		EFantasyWorldChoiceKind InKind,
		int32 InChoiceIndex,
		const FText& InTitle,
		const FText& InDescription,
		const FLinearColor& InColor);

	void SetChoiceEnabled(bool bEnabled);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UFUNCTION()
	void HandleTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void RefreshPresentation();

	UPROPERTY(VisibleAnywhere, Category="Choice")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category="Choice")
	TObjectPtr<UStaticMeshComponent> PedestalMesh;

	UPROPERTY(VisibleAnywhere, Category="Choice")
	TObjectPtr<UStaticMeshComponent> RuneMesh;

	UPROPERTY(VisibleAnywhere, Category="Choice")
	TObjectPtr<UStaticMeshComponent> HaloMesh;

	UPROPERTY(VisibleAnywhere, Category="Choice")
	TObjectPtr<UPointLightComponent> ChoiceLight;

	UPROPERTY(VisibleAnywhere, Category="Choice")
	TObjectPtr<UBoxComponent> ChoiceTrigger;

	UPROPERTY(VisibleAnywhere, Category="Choice")
	TObjectPtr<UWidgetComponent> ChoiceLabel;

	UPROPERTY(Transient)
	TObjectPtr<AWorldWalkerGameModeBase> OwningGameMode;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;

	FText ChoiceTitle;
	FText ChoiceDescription;
	FLinearColor ChoiceColor = FLinearColor(0.2f, 0.5f, 1.0f, 1.0f);
	EFantasyWorldChoiceKind ChoiceKind = EFantasyWorldChoiceKind::Route;
	int32 ChoiceIndex = INDEX_NONE;
	float ArmedAtTime = 0.0f;
	bool bChoiceEnabled = false;
};
