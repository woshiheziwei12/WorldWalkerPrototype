#pragma once

#include "CoreMinimal.h"
#include "Dialogue/FantasyDialogueTypes.h"
#include "GameFramework/Character.h"
#include "Interaction/WorldWalkerInteractable.h"
#include "FantasyNPC.generated.h"

class APlayerController;
class AWorldWalkerCharacter;
class UAnimSequence;
class USkeletalMesh;
class UStaticMeshComponent;
class UWidgetComponent;
class UWorldWalkerDialogueWidget;
class UWorldWalkerNPCPromptWidget;

/** W01 ambient cast member with native Chinese conversation UI and optional Quaternius presentation. */
UCLASS()
class WORLDWALKERPROTOTYPE_API AFantasyNPC
	: public ACharacter
	, public IWorldWalkerInteractable
{
	GENERATED_BODY()

public:
	AFantasyNPC();

	/** Safe to call immediately after SpawnActor; location and rotation remain owned by the arena. */
	void ConfigureNPC(
		EFantasyNPCArchetype Archetype,
		const FText& NameOverride = FText::GetEmpty());

	const FText& GetNPCName() const { return ActiveDialogue.SpeakerName; }
	EFantasyNPCArchetype GetNPCArchetype() const { return NPCArchetype; }
	bool IsDialogueActive() const { return bDialogueActive; }

	virtual bool CanInteract_Implementation(const AWorldWalkerCharacter* InteractingCharacter) const override;
	virtual void Interact_Implementation(AWorldWalkerCharacter* InteractingCharacter) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ApplyArchetypePresentation();
	void RefreshPrompt();
	void BeginDialogue(AWorldWalkerCharacter* InteractingCharacter);
	void FinishDialogue();
	void HandleChoiceSelected(int32 ChoiceIndex);
	void HandleAmbientGesture();
	void PlayGesture(bool bAmbient);
	void ResumeIdle();
	void ScheduleAmbientGesture();
	void SetPromptVisible(bool bVisible);
	USkeletalMesh* LoadFirstMesh(const TArray<FString>& Candidates) const;
	UAnimSequence* LoadFirstAnimation(const TArray<FString>& Candidates) const;

	UPROPERTY(VisibleAnywhere, Category="Visual")
	TObjectPtr<UStaticMeshComponent> PrototypeBody;

	UPROPERTY(VisibleAnywhere, Category="Interaction")
	TObjectPtr<UWidgetComponent> PromptWidgetComponent;

	UPROPERTY(Transient)
	TObjectPtr<UWorldWalkerDialogueWidget> DialogueWidget;

	UPROPERTY(Transient)
	TObjectPtr<AWorldWalkerCharacter> ActiveInteractingPlayer;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> IdleAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> GestureAnimation;

	UPROPERTY(EditAnywhere, Category="NPC")
	EFantasyNPCArchetype NPCArchetype = EFantasyNPCArchetype::GateVeteran;

	FFantasyDialogueScript ActiveDialogue;
	FText ConfiguredNameOverride;
	FRotator RestingRotation = FRotator::ZeroRotator;
	FTimerHandle AmbientGestureTimer;
	FTimerHandle ReturnToIdleTimer;
	bool bDialogueActive = false;
	bool bEndingPlay = false;
};
