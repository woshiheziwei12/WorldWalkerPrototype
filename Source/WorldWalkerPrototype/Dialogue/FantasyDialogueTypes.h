#pragma once

#include "CoreMinimal.h"
#include "FantasyDialogueTypes.generated.h"

UENUM(BlueprintType)
enum class EFantasyNPCArchetype : uint8
{
	GateVeteran UMETA(DisplayName="守门老兵"),
	ExiledSister UMETA(DisplayName="流亡修女"),
	ArcaneScholar UMETA(DisplayName="奥术学者"),
	AshenRanger UMETA(DisplayName="灰烬游侠")
};

USTRUCT(BlueprintType)
struct WORLDWALKERPROTOTYPE_API FFantasyDialogueChoice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue")
	FText ChoiceText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue", meta=(MultiLine=true))
	FText ReplyText;
};

USTRUCT(BlueprintType)
struct WORLDWALKERPROTOTYPE_API FFantasyDialogueScript
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue")
	FText SpeakerName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue", meta=(MultiLine=true))
	FText OpeningText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue")
	TArray<FFantasyDialogueChoice> Choices;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue", meta=(MultiLine=true))
	FText FarewellText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue")
	FLinearColor AccentColor = FLinearColor(0.66f, 0.46f, 0.18f, 1.0f);
};
