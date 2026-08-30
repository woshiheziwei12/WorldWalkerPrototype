#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/W11Types.h"
#include "W11ChapterRouteSubsystem.generated.h"

class AW11GameState;
class AW11PlayerState;
class UW11ChapterDefinition;
class UW11ChapterRouteDefinition;
struct FW11ChapterRouteNode;

USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11PlayerRunSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 PlayerId = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName HeroId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName SectId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName InitialAbilityId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FW11StatBlock Stats;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Health = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Mana = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Barrier = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FName> ActiveAbilitySlots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FW11EquippedAbility> UnlockedAbilities;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 SpiritStones = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CultivationLevel = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CultivationExperience = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 ExperienceToNextLevel = 100;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FW11OwnedManual> OwnedManuals;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FName> GrantedBehaviors;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FName> OwnedTreasures;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 ManualSlots = 6;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 TreasureSlots = 4;
};

USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11ActiveRunSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 SchemaVersion = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString ContentVersion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString RuleVersion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName RouteId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 RunSeed = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 MemoryRunSequence = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 StageIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FW11ChapterRouteCursor Cursor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName ChapterId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName NodeId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FW11PlayerRunSnapshot> Players;
};

UCLASS()
class WORLDWALKERW11_API UW11ActiveRunSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere)
	FW11ActiveRunSnapshot Snapshot;
};

/**
 * Persists the one allowed activity snapshot at authored safe nodes. It survives
 * map travel through GameInstance but never treats death as resumable progress.
 */
UCLASS()
class WORLDWALKERW11_API UW11ChapterRouteSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 SnapshotSchemaVersion = 1;

	void ConfigureRoute(UW11ChapterRouteDefinition* InRoute);
	void ResetRouteCursor();
	bool SetRouteCursor(const FW11ChapterRouteCursor& InCursor);
	bool AdvanceRouteCursor();
	bool IsRouteConfigured() const { return ActiveRoute != nullptr; }
	bool IsRouteComplete() const { return RouteCursor.bComplete; }
	const FW11ChapterRouteCursor& GetRouteCursor() const { return RouteCursor; }
	const UW11ChapterDefinition* GetCurrentChapter() const;
	const FW11ChapterRouteNode* GetCurrentNode() const;
	UW11ChapterRouteDefinition* GetActiveRoute() const { return ActiveRoute; }

	static bool IsCursorValid(
		const UW11ChapterRouteDefinition* Route,
		const FW11ChapterRouteCursor& Cursor);
	static bool AdvanceCursor(
		const UW11ChapterRouteDefinition* Route,
		FW11ChapterRouteCursor& InOutCursor);

	FW11ActiveRunSnapshot BuildSnapshot(
		const AW11GameState* State,
		const TArray<AW11PlayerState*>& Players,
		const FString& ContentVersion,
		const FString& RuleVersion) const;
	bool SaveAtCurrentSafeNode(
		const AW11GameState* State,
		const TArray<AW11PlayerState*>& Players,
		const FString& ContentVersion,
		const FString& RuleVersion);
	bool LoadSafeNodeSnapshot(
		const FString& ExpectedContentVersion,
		const FString& ExpectedRuleVersion,
		FW11ActiveRunSnapshot& OutSnapshot);
	bool DeleteActiveRunSnapshot();
	bool DoesActiveRunSnapshotExist() const;

	static FString GetSnapshotSlotName() { return TEXT("W11_ActiveRun"); }

private:
	UPROPERTY(Transient)
	TObjectPtr<UW11ChapterRouteDefinition> ActiveRoute;

	FW11ChapterRouteCursor RouteCursor;
};
