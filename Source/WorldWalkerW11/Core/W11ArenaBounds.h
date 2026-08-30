#pragma once

#include "CoreMinimal.h"

class ACharacter;

/** Shared deterministic arena boundary policy for players and server AI. */
class WORLDWALKERW11_API FW11ArenaBounds
{
public:
	static float ResolveHalfExtent(int32 ChapterIndex, int32 StageIndex);
	static FVector ClampLocation(const FVector& Location, float CapsuleRadius, float HalfExtent);
	static bool ConstrainCharacter(ACharacter& Character, float HalfExtent);
};
