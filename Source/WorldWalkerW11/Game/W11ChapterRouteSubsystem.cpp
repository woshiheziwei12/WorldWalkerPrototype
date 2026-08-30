#include "Game/W11ChapterRouteSubsystem.h"

#include "Components/W11AttributeComponent.h"
#include "Components/W11RunEconomyComponent.h"
#include "Data/W11Definitions.h"
#include "Game/W11GameState.h"
#include "Game/W11PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "WorldWalkerW11.h"

void UW11ChapterRouteSubsystem::ConfigureRoute(UW11ChapterRouteDefinition* InRoute)
{
	if (ActiveRoute != InRoute)
	{
		ActiveRoute = InRoute;
		ResetRouteCursor();
	}
}

void UW11ChapterRouteSubsystem::ResetRouteCursor()
{
	RouteCursor = FW11ChapterRouteCursor();
}

bool UW11ChapterRouteSubsystem::SetRouteCursor(const FW11ChapterRouteCursor& InCursor)
{
	if (!IsCursorValid(ActiveRoute, InCursor))
	{
		return false;
	}
	RouteCursor = InCursor;
	return true;
}

const UW11ChapterDefinition* UW11ChapterRouteSubsystem::GetCurrentChapter() const
{
	if (!ActiveRoute || RouteCursor.bFinalBoss || RouteCursor.bComplete
		|| !ActiveRoute->Chapters.IsValidIndex(RouteCursor.ChapterIndex))
	{
		return nullptr;
	}
	return ActiveRoute->Chapters[RouteCursor.ChapterIndex].LoadSynchronous();
}

const FW11ChapterRouteNode* UW11ChapterRouteSubsystem::GetCurrentNode() const
{
	const UW11ChapterDefinition* Chapter = GetCurrentChapter();
	return Chapter && Chapter->Nodes.IsValidIndex(RouteCursor.NodeIndex)
		? &Chapter->Nodes[RouteCursor.NodeIndex] : nullptr;
}

bool UW11ChapterRouteSubsystem::IsCursorValid(
	const UW11ChapterRouteDefinition* Route,
	const FW11ChapterRouteCursor& Cursor)
{
	if (!Route || Cursor.ChapterIndex < 0 || Cursor.NodeIndex < 0)
	{
		return false;
	}
	if (Cursor.bComplete)
	{
		return Cursor.bFinalBoss && Cursor.ChapterIndex == Route->Chapters.Num();
	}
	if (Cursor.bFinalBoss)
	{
		return Cursor.ChapterIndex == Route->Chapters.Num()
			&& Cursor.NodeIndex == 0 && !Route->FinalBossEncounter.IsNull();
	}
	if (!Route->Chapters.IsValidIndex(Cursor.ChapterIndex))
	{
		return false;
	}
	const UW11ChapterDefinition* Chapter = Route->Chapters[Cursor.ChapterIndex].LoadSynchronous();
	return Chapter && Chapter->Nodes.IsValidIndex(Cursor.NodeIndex);
}

bool UW11ChapterRouteSubsystem::AdvanceCursor(
	const UW11ChapterRouteDefinition* Route,
	FW11ChapterRouteCursor& InOutCursor)
{
	if (!IsCursorValid(Route, InOutCursor) || InOutCursor.bComplete)
	{
		return false;
	}
	if (InOutCursor.bFinalBoss)
	{
		InOutCursor.bComplete = true;
		return true;
	}
	const UW11ChapterDefinition* Chapter =
		Route->Chapters[InOutCursor.ChapterIndex].LoadSynchronous();
	if (Chapter && Chapter->Nodes.IsValidIndex(InOutCursor.NodeIndex + 1))
	{
		++InOutCursor.NodeIndex;
		return true;
	}
	if (Route->Chapters.IsValidIndex(InOutCursor.ChapterIndex + 1))
	{
		++InOutCursor.ChapterIndex;
		InOutCursor.NodeIndex = 0;
		return true;
	}
	InOutCursor.ChapterIndex = Route->Chapters.Num();
	InOutCursor.NodeIndex = 0;
	InOutCursor.bFinalBoss = true;
	return !Route->FinalBossEncounter.IsNull();
}

bool UW11ChapterRouteSubsystem::AdvanceRouteCursor()
{
	return AdvanceCursor(ActiveRoute, RouteCursor);
}

FW11ActiveRunSnapshot UW11ChapterRouteSubsystem::BuildSnapshot(
	const AW11GameState* State,
	const TArray<AW11PlayerState*>& Players,
	const FString& ContentVersion,
	const FString& RuleVersion) const
{
	FW11ActiveRunSnapshot Snapshot;
	Snapshot.SchemaVersion = SnapshotSchemaVersion;
	Snapshot.ContentVersion = ContentVersion;
	Snapshot.RuleVersion = RuleVersion;
	Snapshot.RouteId = ActiveRoute ? ActiveRoute->DefinitionId : NAME_None;
	Snapshot.RunSeed = State ? State->GetRunSeed() : 0;
	Snapshot.MemoryRunSequence = State ? State->GetMemoryRunSequence() : 0;
	Snapshot.StageIndex = State ? State->GetStageIndex() : 0;
	Snapshot.Cursor = RouteCursor;
	if (const UW11ChapterDefinition* Chapter = GetCurrentChapter())
	{
		Snapshot.ChapterId = Chapter->DefinitionId;
	}
	if (const FW11ChapterRouteNode* Node = GetCurrentNode())
	{
		Snapshot.NodeId = Node->NodeId;
	}
	for (const AW11PlayerState* PlayerState : Players)
	{
		if (!PlayerState || !PlayerState->GetAttributeComponent()
			|| !PlayerState->GetEconomyComponent())
		{
			continue;
		}
		const UW11AttributeComponent* Attributes = PlayerState->GetAttributeComponent();
		const UW11RunEconomyComponent* Economy = PlayerState->GetEconomyComponent();
		FW11PlayerRunSnapshot& Player = Snapshot.Players.AddDefaulted_GetRef();
		Player.PlayerId = PlayerState->GetPlayerId();
		Player.HeroId = PlayerState->GetSelectedHeroId();
		Player.SectId = PlayerState->GetSelectedSectId();
		Player.InitialAbilityId = PlayerState->GetInitialAbilityId();
		Player.Stats = Attributes->GetStats();
		Player.Health = Attributes->GetHealth();
		Player.Mana = Attributes->GetMana();
		Player.Barrier = Attributes->GetBarrier();
		Player.ActiveAbilitySlots = PlayerState->GetActiveAbilitySlots();
		Player.UnlockedAbilities = PlayerState->GetUnlockedAbilities();
		Player.SpiritStones = Economy->GetSpiritStones();
		Player.CultivationLevel = Economy->GetCultivationLevel();
		Player.CultivationExperience = Economy->GetCultivationExperience();
		Player.ExperienceToNextLevel = Economy->GetExperienceToNextLevel();
		Player.OwnedManuals = Economy->GetOwnedManuals();
		Player.GrantedBehaviors = Economy->GetGrantedBehaviors();
		Player.OwnedTreasures = Economy->GetOwnedTreasures();
		Player.ManualSlots = Economy->GetManualSlots();
		Player.TreasureSlots = Economy->GetTreasureSlots();
	}
	Snapshot.Players.Sort([](const FW11PlayerRunSnapshot& Left, const FW11PlayerRunSnapshot& Right)
	{
		return Left.PlayerId < Right.PlayerId;
	});
	return Snapshot;
}

bool UW11ChapterRouteSubsystem::SaveAtCurrentSafeNode(
	const AW11GameState* State,
	const TArray<AW11PlayerState*>& Players,
	const FString& ContentVersion,
	const FString& RuleVersion)
{
	const FW11ChapterRouteNode* Node = GetCurrentNode();
	if (!State || !Node || !Node->bSafeNode || Players.IsEmpty())
	{
		return false;
	}
	UW11ActiveRunSaveGame* SaveGame = Cast<UW11ActiveRunSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UW11ActiveRunSaveGame::StaticClass()));
	if (!SaveGame)
	{
		return false;
	}
	SaveGame->Snapshot = BuildSnapshot(State, Players, ContentVersion, RuleVersion);
	const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGame, GetSnapshotSlotName(), 0);
	if (bSaved)
	{
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_SAFE_NODE_SNAPSHOT_SAVED Success=1 Route=%s Chapter=%s Node=%s Players=%d Seed=%d"),
			*SaveGame->Snapshot.RouteId.ToString(), *SaveGame->Snapshot.ChapterId.ToString(),
			*SaveGame->Snapshot.NodeId.ToString(), SaveGame->Snapshot.Players.Num(),
			SaveGame->Snapshot.RunSeed);
	}
	else
	{
		UE_LOG(LogWorldWalkerW11, Error,
			TEXT("W11_SAFE_NODE_SNAPSHOT_SAVED Success=0 Route=%s Chapter=%s Node=%s Players=%d Seed=%d"),
			*SaveGame->Snapshot.RouteId.ToString(), *SaveGame->Snapshot.ChapterId.ToString(),
			*SaveGame->Snapshot.NodeId.ToString(), SaveGame->Snapshot.Players.Num(),
			SaveGame->Snapshot.RunSeed);
	}
	return bSaved;
}

bool UW11ChapterRouteSubsystem::LoadSafeNodeSnapshot(
	const FString& ExpectedContentVersion,
	const FString& ExpectedRuleVersion,
	FW11ActiveRunSnapshot& OutSnapshot)
{
	UW11ActiveRunSaveGame* SaveGame = Cast<UW11ActiveRunSaveGame>(
		UGameplayStatics::LoadGameFromSlot(GetSnapshotSlotName(), 0));
	if (!SaveGame || SaveGame->Snapshot.SchemaVersion != SnapshotSchemaVersion
		|| SaveGame->Snapshot.ContentVersion != ExpectedContentVersion
		|| SaveGame->Snapshot.RuleVersion != ExpectedRuleVersion
		|| !ActiveRoute || SaveGame->Snapshot.RouteId != ActiveRoute->DefinitionId
		|| !IsCursorValid(ActiveRoute, SaveGame->Snapshot.Cursor)
		|| SaveGame->Snapshot.Players.IsEmpty())
	{
		return false;
	}
	OutSnapshot = SaveGame->Snapshot;
	return true;
}

bool UW11ChapterRouteSubsystem::DeleteActiveRunSnapshot()
{
	const bool bExisted = DoesActiveRunSnapshotExist();
	const bool bDeleted = !bExisted || UGameplayStatics::DeleteGameInSlot(GetSnapshotSlotName(), 0);
	if (bDeleted)
	{
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_ACTIVE_RUN_SNAPSHOT_DELETED Existed=%d Success=1"), bExisted ? 1 : 0);
	}
	else
	{
		UE_LOG(LogWorldWalkerW11, Error,
			TEXT("W11_ACTIVE_RUN_SNAPSHOT_DELETED Existed=%d Success=0"), bExisted ? 1 : 0);
	}
	return bDeleted;
}

bool UW11ChapterRouteSubsystem::DoesActiveRunSnapshotExist() const
{
	return UGameplayStatics::DoesSaveGameExist(GetSnapshotSlotName(), 0);
}
