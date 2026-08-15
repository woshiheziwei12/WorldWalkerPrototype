#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FantasyCardProgressionSubsystem.generated.h"

class UCardDefinition;

/**
 * Keeps W01 deck rewards alive while the player travels between maps in the
 * current run.  Card definitions remain the source of truth; this subsystem
 * stores only stable CardIds and awarded copy counts.
 */
UCLASS()
class WORLDWALKERPROTOTYPE_API UFantasyCardProgressionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	bool GrantCard(const UCardDefinition* Card);
	int32 GetGrantedCopies(FName CardId) const;
	int32 GetTotalGrantedCopies() const;
	FString BuildRunSummary() const;

private:
	UPROPERTY(Transient)
	TMap<FName, int32> GrantedCardCopies;
};
