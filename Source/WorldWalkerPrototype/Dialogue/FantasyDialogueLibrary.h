#pragma once

#include "CoreMinimal.h"
#include "Dialogue/FantasyDialogueTypes.h"

/** Central, presentation-only source for the small W01 ambient dialogue cast. */
struct WORLDWALKERPROTOTYPE_API FFantasyDialogueLibrary
{
	static FFantasyDialogueScript BuildScript(EFantasyNPCArchetype Archetype);
	static TArray<FString> GetMeshAssetCandidates(EFantasyNPCArchetype Archetype);
	static TArray<FString> GetIdleAnimationCandidates(EFantasyNPCArchetype Archetype);
	static TArray<FString> GetGestureAnimationCandidates(EFantasyNPCArchetype Archetype);
	static FLinearColor GetFallbackBodyColor(EFantasyNPCArchetype Archetype);

private:
	static FString GetCharacterAssetFolder(EFantasyNPCArchetype Archetype);
	static FString GetCharacterAssetStem(EFantasyNPCArchetype Archetype);
};
