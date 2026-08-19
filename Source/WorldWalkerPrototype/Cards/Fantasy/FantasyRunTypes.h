#pragma once

#include "CoreMinimal.h"
#include "FantasyRunTypes.generated.h"

/** Presentation timing for enemy turns. Fast mode is intended for unattended regression runs. */
struct WORLDWALKERPROTOTYPE_API FFantasyCombatTiming
{
	bool bFastCombat = false;
	float EnemyTurnStartDelay = 0.7f;
	float EnemyCardPresentationDelay = 1.05f;

	static FFantasyCombatTiming ForAutomationMode(const bool bFastCombat)
	{
		FFantasyCombatTiming Timing;
		if (bFastCombat)
		{
			Timing.bFastCombat = true;
			// Keep a one-frame-safe positive delay so timer callbacks never recurse inline.
			Timing.EnemyTurnStartDelay = 0.01f;
			Timing.EnemyCardPresentationDelay = 0.01f;
		}
		return Timing;
	}
};

/** Player professions share the same classic route, but own separate starter/reward pools. */
UENUM(BlueprintType)
enum class EFantasyPlayerProfession : uint8
{
	None,
	Knight,
	Mage
};

/** High-level pages used by the W01 classic deck-building chapter. */
UENUM(BlueprintType)
enum class EFantasyRouteNodeType : uint8
{
	Combat,
	EliteCombat,
	Event,
	Rest,
	Boss
};
/** A visible route choice. PayloadId resolves to an enemy or event definition. */
USTRUCT(BlueprintType)
struct WORLDWALKERPROTOTYPE_API FFantasyRouteNodeChoice
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Route")
	FName NodeId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Route")
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Route")
	FText Description;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Route")
	EFantasyRouteNodeType NodeType = EFantasyRouteNodeType::Combat;

	/** EnemyId for combat nodes or EventId for non-combat nodes. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Route")
	FName PayloadId;

	bool IsCombat() const
	{
		return NodeType == EFantasyRouteNodeType::Combat
			|| NodeType == EFantasyRouteNodeType::EliteCombat
			|| NodeType == EFantasyRouteNodeType::Boss;
	}
};
