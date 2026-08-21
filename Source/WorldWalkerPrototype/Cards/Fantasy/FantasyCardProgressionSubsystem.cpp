#include "Cards/Fantasy/FantasyCardProgressionSubsystem.h"

#include "Cards/CardDefinition.h"
#include "Cards/Fantasy/FantasyChapterDefinition.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

const FString UFantasyCardProgressionSubsystem::DefaultContentVersion(TEXT("W01-M1-v3"));

namespace
{
	uint32 HashStableUtf8(const FString& Value)
	{
		const FTCHARToUTF8 Utf8(*Value);
		uint32 Hash = 2166136261u;
		for (int32 Index = 0; Index < Utf8.Length(); ++Index)
		{
			Hash ^= static_cast<uint8>(Utf8.Get()[Index]);
			Hash *= 16777619u;
		}
		return Hash;
	}
}

bool UFantasyCardProgressionSubsystem::ConfigureRun(
	const int32 InRouteSeed,
	const FString& InContentVersion,
	const EFantasyRunDifficulty InDifficulty)
{
	const bool bValidDifficulty = InDifficulty == EFantasyRunDifficulty::Story
		|| InDifficulty == EFantasyRunDifficulty::Normal
		|| InDifficulty == EFantasyRunDifficulty::Hard;
	if (bRunStarted || InContentVersion.IsEmpty() || !bValidDifficulty)
	{
		return false;
	}
	RouteSeed = InRouteSeed;
	ContentVersion = InContentVersion;
	Difficulty = InDifficulty;
	bRunConfigurationResolved = true;
	return true;
}

FString UFantasyCardProgressionSubsystem::GetDifficultyName() const
{
	switch (Difficulty)
	{
	case EFantasyRunDifficulty::Story: return TEXT("Story");
	case EFantasyRunDifficulty::Hard: return TEXT("Hard");
	case EFantasyRunDifficulty::Normal:
	default: return TEXT("Normal");
	}
}

FString UFantasyCardProgressionSubsystem::GetEndReasonName(const EFantasyRunEndReason Reason)
{
	switch (Reason)
	{
	case EFantasyRunEndReason::Completed: return TEXT("Completed");
	case EFantasyRunEndReason::Defeat: return TEXT("Defeat");
	case EFantasyRunEndReason::Aborted: return TEXT("Aborted");
	case EFantasyRunEndReason::None:
	default: return TEXT("None");
	}
}

void UFantasyCardProgressionSubsystem::ResolveRunConfiguration()
{
	if (bRunConfigurationResolved)
	{
		return;
	}

	RouteSeed = DefaultRouteSeed;
	ContentVersion = DefaultContentVersion;
	Difficulty = EFantasyRunDifficulty::Normal;
	FParse::Value(FCommandLine::Get(), TEXT("W01RouteSeed="), RouteSeed);
	FString ParsedContentVersion;
	if (FParse::Value(
		FCommandLine::Get(),
		TEXT("W01ContentVersion="),
		ParsedContentVersion)
		&& !ParsedContentVersion.IsEmpty())
	{
		ContentVersion = ParsedContentVersion;
	}
	FString ParsedDifficulty;
	if (FParse::Value(FCommandLine::Get(), TEXT("W01Difficulty="), ParsedDifficulty))
	{
		if (ParsedDifficulty.Equals(TEXT("Story"), ESearchCase::IgnoreCase))
		{
			Difficulty = EFantasyRunDifficulty::Story;
		}
		else if (ParsedDifficulty.Equals(TEXT("Hard"), ESearchCase::IgnoreCase))
		{
			Difficulty = EFantasyRunDifficulty::Hard;
		}
		else if (!ParsedDifficulty.Equals(TEXT("Normal"), ESearchCase::IgnoreCase))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Unsupported W01 difficulty '%s'; using Normal."),
				*ParsedDifficulty);
		}
	}
	bRunConfigurationResolved = true;
}

void UFantasyCardProgressionSubsystem::ResolveRunIdentityFields()
{
	BuildVersion = FApp::GetBuildVersion();
	if (BuildVersion.IsEmpty())
	{
		BuildVersion = TEXT("LocalDevelopment");
	}
	RunSource = FParse::Param(FCommandLine::Get(), TEXT("W01M0RunAutomation"))
		? EFantasyRunSource::Automation
		: EFantasyRunSource::Manual;
}

bool UFantasyCardProgressionSubsystem::SelectProfession(
	const EFantasyPlayerProfession Profession)
{
	if (bRunStarted || Profession == EFantasyPlayerProfession::None)
	{
		return false;
	}

	SelectedProfession = Profession;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_PROFESSION_SELECTED Profession=%s"),
		*GetProfessionDisplayName());
	return true;
}

FString UFantasyCardProgressionSubsystem::GetProfessionDisplayName() const
{
	switch (SelectedProfession)
	{
	case EFantasyPlayerProfession::Mage: return TEXT("法师（小女巫）");
	case EFantasyPlayerProfession::Knight: return TEXT("女骑士");
	case EFantasyPlayerProfession::None:
	default: return TEXT("尚未选择");
	}
}

void UFantasyCardProgressionSubsystem::EnsureRunStarted()
{
	if (bRunStarted || !HasSelectedProfession())
	{
		return;
	}

	ResolveRunConfiguration();
	ResolveRunIdentityFields();
	bRunStarted = true;
	RunId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
	CurrentChapter = 1;
	ChapterDepth = 0;
	bHasActiveNode = false;
	bChapterComplete = false;
	EndReason = EFantasyRunEndReason::None;
	Gold = 0;
	DeletionCount = 0;
	LastEnemyId = NAME_None;
	BlessingIds.Reset();
	DefeatedEnemyIds.Reset();
	DecisionHistory.Reset();
	RunDeck.Reset();
	bDeckSnapshotInitialized = false;
	RunMaxHealth = 100;
	CurrentRunHealth = RunMaxHealth;
	RouteChoices.Reset();
	LoadChapterDefinition();
	LogStructuredEvent(TEXT("RunStarted"));
	RebuildRouteChoices();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_RUN_STARTED Chapter=1 Depth=0 Profession=%s Choices=%d RouteSeed=%d ContentVersion=%s Difficulty=%s BuildVersion=%s"),
		*GetProfessionDisplayName(),
		RouteChoices.Num(),
		RouteSeed,
		*ContentVersion,
		*GetDifficultyName(),
		*BuildVersion);
}

void UFantasyCardProgressionSubsystem::ResetRun()
{
	if (bRunStarted && EndReason == EFantasyRunEndReason::None)
	{
		EndRun(EFantasyRunEndReason::Aborted, LastEnemyId);
	}
	GrantedCardCopies.Reset();
	RemovedCardCopies.Reset();
	UpgradedFromCardCopies.Reset();
	UpgradedToCardCopies.Reset();
	RunDeck.Reset();
	BlessingIds.Reset();
	DefeatedEnemyIds.Reset();
	DecisionHistory.Reset();
	RouteChoices.Reset();
	ActiveNode = FFantasyRouteNodeChoice();
	SelectedProfession = EFantasyPlayerProfession::None;
	CurrentChapter = 1;
	ChapterDepth = 0;
	bRunStarted = false;
	bHasActiveNode = false;
	bChapterComplete = false;
	PendingBattleBlock = 0;
	PendingBattleValor = 0;
	RunMaxHealth = 100;
	CurrentRunHealth = RunMaxHealth;
	Gold = 0;
	DeletionCount = 0;
	BuildVersion.Reset();
	RunSource = EFantasyRunSource::Manual;
	EndReason = EFantasyRunEndReason::None;
	LastEnemyId = NAME_None;
	bDeckSnapshotInitialized = false;
	RandomStreamSequences.Reset();
	RunId.Reset();
	UE_LOG(LogTemp, Display, TEXT("W01_RUN_RESET"));
}

void UFantasyCardProgressionSubsystem::LogStructuredEvent(
	const FString& EventType,
	const TMap<FString, FString>& StringFields,
	const TMap<FString, int64>& NumberFields) const
{
	if (RunId.IsEmpty())
	{
		return;
	}
	TSharedRef<FJsonObject> Event = MakeShared<FJsonObject>();
	Event->SetNumberField(TEXT("schemaVersion"), StructuredEventSchemaVersion);
	Event->SetStringField(TEXT("event"), EventType);
	Event->SetStringField(TEXT("runId"), RunId);
	Event->SetStringField(TEXT("contentVersion"), ContentVersion);
	Event->SetStringField(TEXT("buildVersion"), BuildVersion);
	Event->SetNumberField(TEXT("routeSeed"), RouteSeed);
	Event->SetStringField(TEXT("difficulty"), GetDifficultyName());
	Event->SetStringField(TEXT("profession"), GetProfessionDisplayName());
	Event->SetNumberField(TEXT("chapter"), CurrentChapter);
	Event->SetNumberField(TEXT("depth"), ChapterDepth);
	Event->SetNumberField(TEXT("currentHealth"), CurrentRunHealth);
	Event->SetNumberField(TEXT("maxHealth"), RunMaxHealth);
	Event->SetNumberField(TEXT("gold"), Gold);
	Event->SetNumberField(TEXT("deletionCount"), DeletionCount);
	Event->SetNumberField(TEXT("blessingCount"), BlessingIds.Num());
	Event->SetStringField(
		TEXT("source"),
		RunSource == EFantasyRunSource::Automation
			? TEXT("automation")
			: TEXT("manual"));
	for (const TPair<FString, FString>& Field : StringFields)
	{
		Event->SetStringField(Field.Key, Field.Value);
	}
	for (const TPair<FString, int64>& Field : NumberFields)
	{
		Event->SetNumberField(Field.Key, Field.Value);
	}

	FString JsonLine;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonLine);
	FJsonSerializer::Serialize(Event, Writer);
	UE_LOG(LogTemp, Display, TEXT("W01_RUN_EVENT_JSON %s"), *JsonLine);
}

void UFantasyCardProgressionSubsystem::EndRun(
	const EFantasyRunEndReason Reason,
	const FName EnemyId)
{
	if (!bRunStarted || Reason == EFantasyRunEndReason::None
		|| EndReason != EFantasyRunEndReason::None)
	{
		return;
	}
	EndReason = Reason;
	if (!EnemyId.IsNone())
	{
		LastEnemyId = EnemyId;
	}
	TMap<FString, FString> Fields = {{TEXT("reason"), GetEndReasonName(Reason)}};
	if (!LastEnemyId.IsNone())
	{
		Fields.Add(TEXT("enemyId"), LastEnemyId.ToString());
	}
	LogStructuredEvent(TEXT("RunEnded"), Fields);
}

int32 UFantasyCardProgressionSubsystem::BuildDeterministicSeed(
	const FName StreamName,
	const int32 Sequence) const
{
	const uint32 SeedHash = HashCombineFast(
		GetTypeHash(RouteSeed),
		HashCombineFast(
			HashStableUtf8(ContentVersion),
			HashCombineFast(
				GetTypeHash(static_cast<uint8>(Difficulty)),
				HashCombineFast(HashStableUtf8(StreamName.ToString()), GetTypeHash(Sequence)))));
	return static_cast<int32>(SeedHash);
}

int32 UFantasyCardProgressionSubsystem::ConsumeDeterministicSeed(const FName StreamName)
{
	ResolveRunConfiguration();
	int32& Sequence = RandomStreamSequences.FindOrAdd(StreamName);
	const int32 DerivedSeed = BuildDeterministicSeed(StreamName, Sequence);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_RANDOM_STREAM Stream=%s Sequence=%d Seed=%d RouteSeed=%d ContentVersion=%s Difficulty=%s"),
		*StreamName.ToString(),
		Sequence,
		DerivedSeed,
		RouteSeed,
		*ContentVersion,
		*GetDifficultyName());
	++Sequence;
	return DerivedSeed;
}

FString UFantasyCardProgressionSubsystem::BuildRouteChoiceSignature() const
{
	TArray<FString> NodeIds;
	NodeIds.Reserve(RouteChoices.Num());
	for (const FFantasyRouteNodeChoice& Choice : RouteChoices)
	{
		NodeIds.Add(Choice.NodeId.ToString());
	}
	return FString::Join(NodeIds, TEXT("|"));
}

FName UFantasyCardProgressionSubsystem::MakeCardStateKey(
	const FName CardId,
	const int32 UpgradeLevel)
{
	return FName(*FString::Printf(
		TEXT("%s@%d"),
		*CardId.ToString(),
		FMath::Clamp(UpgradeLevel, 0, 1)));
}

bool UFantasyCardProgressionSubsystem::GrantCard(const UCardDefinition* Card)
{
	if (!bRunStarted || !Card || Card->CardId.IsNone()
		|| Card->CardSetId != FName(TEXT("W01_EasternHorror"))
		|| (Card->Profession != EFantasyPlayerProfession::None
			&& Card->Profession != SelectedProfession))
	{
		return false;
	}

	const int32 UpgradeLevel = FMath::Clamp(Card->UpgradeLevel, 0, 1);
	int32& Copies = GrantedCardCopies.FindOrAdd(
		MakeCardStateKey(Card->CardId, UpgradeLevel));
	++Copies;
	if (bDeckSnapshotInitialized)
	{
		AddDeckCount(Card->CardId, UpgradeLevel, 1);
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01 card grant persisted for this run. Card=%s Upgrade=%d Copies=%d TotalGranted=%d"),
		*Card->CardId.ToString(),
		UpgradeLevel,
		Copies,
		GetTotalGrantedCopies());
	LogStructuredEvent(
		TEXT("CardAdded"),
		{{TEXT("cardId"), Card->CardId.ToString()}},
		{{TEXT("upgradeLevel"), UpgradeLevel}, {TEXT("count"), 1}});
	return true;
}

bool UFantasyCardProgressionSubsystem::RemoveCardCopy(
	const FName CardId,
	const int32 UpgradeLevel)
{
	const int32 SafeUpgradeLevel = FMath::Clamp(UpgradeLevel, 0, 1);
	if (!bRunStarted || CardId.IsNone()
		|| (bDeckSnapshotInitialized && GetRunDeckCopies(CardId, SafeUpgradeLevel) <= 0))
	{
		return false;
	}

	int32& Copies = RemovedCardCopies.FindOrAdd(
		MakeCardStateKey(CardId, SafeUpgradeLevel));
	++Copies;
	++DeletionCount;
	if (bDeckSnapshotInitialized)
	{
		AddDeckCount(CardId, SafeUpgradeLevel, -1);
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01 run deck removed one copy. Card=%s Upgrade=%d RemovedCopies=%d TotalRemoved=%d"),
		*CardId.ToString(),
		SafeUpgradeLevel,
		Copies,
		GetTotalRemovedCopies());
	LogStructuredEvent(
		TEXT("CardRemoved"),
		{{TEXT("cardId"), CardId.ToString()}},
		{{TEXT("upgradeLevel"), SafeUpgradeLevel},
		 {TEXT("count"), 1},
		 {TEXT("deletionCount"), DeletionCount}});
	return true;
}

bool UFantasyCardProgressionSubsystem::UpgradeCardCopy(const UCardDefinition* BaseCard)
{
	if (!bRunStarted || !BaseCard || BaseCard->CardId.IsNone()
		|| BaseCard->CardSetId != FName(TEXT("W01_EasternHorror"))
		|| (BaseCard->Profession != EFantasyPlayerProfession::None
			&& BaseCard->Profession != SelectedProfession)
		|| BaseCard->UpgradeLevel != 0
		|| BaseCard->UpgradeCard.IsNull() || !bDeckSnapshotInitialized
		|| GetRunDeckCopies(BaseCard->CardId, 0) <= 0)
	{
		return false;
	}
	const UCardDefinition* UpgradeCard = BaseCard->UpgradeCard.Get();
	if (!UpgradeCard)
	{
		UpgradeCard = BaseCard->UpgradeCard.LoadSynchronous();
	}
	if (!UpgradeCard || UpgradeCard == BaseCard || UpgradeCard->UpgradeLevel != 1
		|| UpgradeCard->CardId.IsNone()
		|| UpgradeCard->CardSetId != BaseCard->CardSetId
		|| UpgradeCard->Profession != BaseCard->Profession)
	{
		return false;
	}

	++UpgradedFromCardCopies.FindOrAdd(MakeCardStateKey(BaseCard->CardId, 0));
	++UpgradedToCardCopies.FindOrAdd(MakeCardStateKey(UpgradeCard->CardId, 1));
	AddDeckCount(BaseCard->CardId, 0, -1);
	AddDeckCount(UpgradeCard->CardId, 1, 1);
	LogStructuredEvent(
		TEXT("CardUpgraded"),
		{{TEXT("fromCardId"), BaseCard->CardId.ToString()},
		 {TEXT("toCardId"), UpgradeCard->CardId.ToString()}},
		{{TEXT("fromLevel"), 0}, {TEXT("toLevel"), 1}});
	return true;
}

int32 UFantasyCardProgressionSubsystem::GetGrantedCopies(
	const FName CardId,
	const int32 UpgradeLevel) const
{
	return GrantedCardCopies.FindRef(MakeCardStateKey(CardId, UpgradeLevel));
}

int32 UFantasyCardProgressionSubsystem::GetRemovedCopies(
	const FName CardId,
	const int32 UpgradeLevel) const
{
	return RemovedCardCopies.FindRef(MakeCardStateKey(CardId, UpgradeLevel));
}

int32 UFantasyCardProgressionSubsystem::GetUpgradedFromCopies(
	const FName CardId,
	const int32 UpgradeLevel) const
{
	return UpgradedFromCardCopies.FindRef(MakeCardStateKey(CardId, UpgradeLevel));
}

int32 UFantasyCardProgressionSubsystem::GetUpgradedToCopies(
	const FName CardId,
	const int32 UpgradeLevel) const
{
	return UpgradedToCardCopies.FindRef(MakeCardStateKey(CardId, UpgradeLevel));
}

int32 UFantasyCardProgressionSubsystem::GetTotalGrantedCopies() const
{
	int32 Total = 0;
	for (const TPair<FName, int32>& Pair : GrantedCardCopies)
	{
		Total += FMath::Max(0, Pair.Value);
	}
	return Total;
}

int32 UFantasyCardProgressionSubsystem::GetTotalRemovedCopies() const
{
	int32 Total = 0;
	for (const TPair<FName, int32>& Pair : RemovedCardCopies)
	{
		Total += FMath::Max(0, Pair.Value);
	}
	return Total;
}

void UFantasyCardProgressionSubsystem::ReplaceDeckSnapshot(
	const TArray<UCardDefinition*>& Cards)
{
	RunDeck.Reset();
	bDeckSnapshotInitialized = true;
	for (const UCardDefinition* Card : Cards)
	{
		if (Card && !Card->CardId.IsNone())
		{
			AddDeckCount(Card->CardId, Card->UpgradeLevel, 1);
		}
	}

	TArray<FString> Entries;
	Entries.Reserve(RunDeck.Num());
	for (const FFantasyRunCardCount& Entry : RunDeck)
	{
		Entries.Add(FString::Printf(
			TEXT("%s@%d:%d"),
			*Entry.CardId.ToString(),
			Entry.UpgradeLevel,
			Entry.Count));
	}
	Entries.Sort();
	LogStructuredEvent(
		TEXT("DeckSnapshot"),
		{{TEXT("signature"), FString::Join(Entries, TEXT("|"))}},
		{{TEXT("uniqueCardVersions"), RunDeck.Num()},
		 {TEXT("totalCards"), Cards.Num()}});
}

void UFantasyCardProgressionSubsystem::AddDeckCount(
	const FName CardId,
	const int32 UpgradeLevel,
	const int32 Delta)
{
	const int32 SafeLevel = FMath::Clamp(UpgradeLevel, 0, 1);
	const int32 ExistingIndex = RunDeck.IndexOfByPredicate(
		[CardId, SafeLevel](const FFantasyRunCardCount& Entry)
		{
			return Entry.CardId == CardId && Entry.UpgradeLevel == SafeLevel;
		});
	if (ExistingIndex == INDEX_NONE)
	{
		if (Delta > 0)
		{
			FFantasyRunCardCount& Entry = RunDeck.AddDefaulted_GetRef();
			Entry.CardId = CardId;
			Entry.UpgradeLevel = SafeLevel;
			Entry.Count = Delta;
		}
		return;
	}
	RunDeck[ExistingIndex].Count = FMath::Max(0, RunDeck[ExistingIndex].Count + Delta);
	if (RunDeck[ExistingIndex].Count == 0)
	{
		RunDeck.RemoveAt(ExistingIndex);
	}
}

int32 UFantasyCardProgressionSubsystem::GetRunDeckCopies(
	const FName CardId,
	const int32 UpgradeLevel) const
{
	const FFantasyRunCardCount* Entry = RunDeck.FindByPredicate(
		[CardId, UpgradeLevel](const FFantasyRunCardCount& Candidate)
		{
			return Candidate.CardId == CardId
				&& Candidate.UpgradeLevel == FMath::Clamp(UpgradeLevel, 0, 1);
		});
	return Entry ? Entry->Count : 0;
}

int32 UFantasyCardProgressionSubsystem::AddGold(const int32 Amount)
{
	if (!bRunStarted || Amount <= 0)
	{
		return 0;
	}
	const int32 PreviousGold = Gold;
	Gold = static_cast<int32>(FMath::Min<int64>(
		MAX_int32,
		static_cast<int64>(Gold) + Amount));
	const int32 Applied = Gold - PreviousGold;
	LogStructuredEvent(
		TEXT("GoldChanged"),
		{{TEXT("reason"), TEXT("Granted")}},
		{{TEXT("delta"), Applied}, {TEXT("gold"), Gold}});
	return Applied;
}

bool UFantasyCardProgressionSubsystem::SpendGold(const int32 Amount)
{
	if (!bRunStarted || Amount <= 0 || Amount > Gold)
	{
		return false;
	}
	Gold -= Amount;
	LogStructuredEvent(
		TEXT("GoldChanged"),
		{{TEXT("reason"), TEXT("Spent")}},
		{{TEXT("delta"), -Amount}, {TEXT("gold"), Gold}});
	return true;
}

bool UFantasyCardProgressionSubsystem::GrantBlessing(const FName BlessingId)
{
	if (!bRunStarted || BlessingId.IsNone() || BlessingIds.Contains(BlessingId))
	{
		return false;
	}
	BlessingIds.Add(BlessingId);
	LogStructuredEvent(
		TEXT("BlessingGranted"),
		{{TEXT("blessingId"), BlessingId.ToString()}},
		{{TEXT("blessingCount"), BlessingIds.Num()}});
	return true;
}

FString UFantasyCardProgressionSubsystem::BuildRunSummary() const
{
	return FString::Printf(
		TEXT("经典第一章 · %s · %s · 路程 %d/6 · 生命 %d/%d · 金币 %d · 获得 %d 张 · 移除 %d 张 · 祝福 %d"),
		*GetProfessionDisplayName(),
		*GetDifficultyName(),
		FMath::Clamp(ChapterDepth + 1, 1, GetTotalRouteDepths()),
		CurrentRunHealth,
		RunMaxHealth,
		Gold,
		GetTotalGrantedCopies(),
		GetTotalRemovedCopies(),
		BlessingIds.Num());
}

void UFantasyCardProgressionSubsystem::AddDecisionRecord(
	const FName EventType,
	const FString& CandidateSignature,
	const FName SelectionId,
	const int32 SelectionIndex)
{
	FFantasyRunDecisionRecord& Record = DecisionHistory.AddDefaulted_GetRef();
	Record.EventType = EventType;
	Record.Chapter = CurrentChapter;
	Record.Depth = ChapterDepth;
	Record.CandidateSignature = CandidateSignature;
	Record.SelectionId = SelectionId;
	Record.SelectionIndex = SelectionIndex;
}

void UFantasyCardProgressionSubsystem::RecordRewardOffer(
	const FString& CandidateSignature)
{
	AddDecisionRecord(TEXT("RewardOffered"), CandidateSignature, NAME_None);
	LogStructuredEvent(
		TEXT("RewardOffered"),
		{{TEXT("signature"), CandidateSignature}});
}

void UFantasyCardProgressionSubsystem::RecordRewardSelection(
	const FName CardId,
	const bool bSkipped,
	const int32 ChoiceIndex)
{
	const int32 RecordedIndex = bSkipped ? INDEX_NONE : ChoiceIndex;
	AddDecisionRecord(
		TEXT("RewardSelected"),
		FString(),
		bSkipped ? NAME_None : CardId,
		RecordedIndex);
	LogStructuredEvent(
		TEXT("RewardSelected"),
		{{TEXT("cardId"), bSkipped ? TEXT("None") : CardId.ToString()},
		 {TEXT("decision"), bSkipped ? TEXT("Skipped") : TEXT("Claimed")}},
		{{TEXT("choiceIndex"), RecordedIndex}});
}

void UFantasyCardProgressionSubsystem::RecordEventOffer(
	const FName EventId,
	const FString& CandidateSignature)
{
	AddDecisionRecord(TEXT("EventOffered"), CandidateSignature, EventId);
	LogStructuredEvent(
		TEXT("EventOffered"),
		{{TEXT("eventId"), EventId.ToString()},
		 {TEXT("signature"), CandidateSignature}});
}

void UFantasyCardProgressionSubsystem::RecordEventSelection(
	const FName EventId,
	const int32 ChoiceIndex)
{
	AddDecisionRecord(TEXT("EventSelected"), FString(), EventId, ChoiceIndex);
	LogStructuredEvent(
		TEXT("EventSelected"),
		{{TEXT("eventId"), EventId.ToString()}},
		{{TEXT("choiceIndex"), ChoiceIndex}});
}

void UFantasyCardProgressionSubsystem::RecordBattleStarted(
	const FName EnemyId,
	const int32 PlayerDeckCount)
{
	LastEnemyId = EnemyId;
	LogStructuredEvent(
		TEXT("BattleStarted"),
		{{TEXT("enemyId"), EnemyId.IsNone() ? TEXT("Unknown") : EnemyId.ToString()}},
		{{TEXT("playerDeckCount"), PlayerDeckCount}});
}

void UFantasyCardProgressionSubsystem::RecordEnemyDefeated(const FName EnemyId)
{
	if (EnemyId.IsNone())
	{
		return;
	}
	LastEnemyId = EnemyId;
	DefeatedEnemyIds.Add(EnemyId);
	LogStructuredEvent(
		TEXT("EnemyDefeated"),
		{{TEXT("enemyId"), EnemyId.ToString()}},
		{{TEXT("defeatedCount"), DefeatedEnemyIds.Num()}});
}

const TArray<FFantasyRouteNodeChoice>& UFantasyCardProgressionSubsystem::GetRouteChoices()
{
	EnsureRunStarted();
	if (RouteChoices.IsEmpty() && !bHasActiveNode && !bChapterComplete)
	{
		RebuildRouteChoices();
	}
	return RouteChoices;
}

bool UFantasyCardProgressionSubsystem::SelectRouteChoice(
	const int32 ChoiceIndex,
	FFantasyRouteNodeChoice& OutChoice)
{
	EnsureRunStarted();
	if (bHasActiveNode || bChapterComplete || !RouteChoices.IsValidIndex(ChoiceIndex))
	{
		return false;
	}

	ActiveNode = RouteChoices[ChoiceIndex];
	OutChoice = ActiveNode;
	bHasActiveNode = true;
	RouteChoices.Reset();
	AddDecisionRecord(
		TEXT("RouteSelected"),
		FString(),
		ActiveNode.NodeId,
		ChoiceIndex);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_ROUTE_NODE_SELECTED Depth=%d Node=%s Type=%d Payload=%s"),
		ChapterDepth,
		*ActiveNode.NodeId.ToString(),
		static_cast<int32>(ActiveNode.NodeType),
		*ActiveNode.PayloadId.ToString());
	LogStructuredEvent(
		TEXT("RouteSelected"),
		{{TEXT("nodeId"), ActiveNode.NodeId.ToString()},
		 {TEXT("payloadId"), ActiveNode.PayloadId.ToString()}},
		{{TEXT("nodeType"), static_cast<int64>(ActiveNode.NodeType)},
		 {TEXT("choiceIndex"), ChoiceIndex}});
	return true;
}

bool UFantasyCardProgressionSubsystem::CompleteActiveNode()
{
	if (!bHasActiveNode)
	{
		return false;
	}

	const FName CompletedNodeId = ActiveNode.NodeId;
	const bool bCompletedBoss = ActiveNode.NodeType == EFantasyRouteNodeType::Boss;
	ActiveNode = FFantasyRouteNodeChoice();
	bHasActiveNode = false;
	if (bCompletedBoss)
	{
		bChapterComplete = true;
		RouteChoices.Reset();
	}
	else
	{
		++ChapterDepth;
		RebuildRouteChoices();
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_ROUTE_NODE_COMPLETED Node=%s Depth=%d ChapterComplete=%d"),
		*CompletedNodeId.ToString(),
		ChapterDepth,
		bChapterComplete ? 1 : 0);
	LogStructuredEvent(
		TEXT("NodeCompleted"),
		{{TEXT("nodeId"), CompletedNodeId.ToString()}},
		{{TEXT("chapterComplete"), bChapterComplete ? 1 : 0}});
	if (bChapterComplete)
	{
		EndRun(EFantasyRunEndReason::Completed, LastEnemyId);
	}
	return true;
}

void UFantasyCardProgressionSubsystem::AddPendingBattleBoon(
	const int32 Block,
	const int32 Valor)
{
	PendingBattleBlock += FMath::Max(0, Block);
	PendingBattleValor += FMath::Max(0, Valor);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01 pending battle boon updated. Block=%d Valor=%d"),
		PendingBattleBlock,
		PendingBattleValor);
}

void UFantasyCardProgressionSubsystem::ConsumePendingBattleBoon(
	int32& OutBlock,
	int32& OutValor)
{
	OutBlock = PendingBattleBlock;
	OutValor = PendingBattleValor;
	PendingBattleBlock = 0;
	PendingBattleValor = 0;
}

void UFantasyCardProgressionSubsystem::RecordRunHealth(
	const int32 CurrentHealth,
	const int32 MaxHealth)
{
	RunMaxHealth = FMath::Max(1, MaxHealth);
	CurrentRunHealth = FMath::Clamp(CurrentHealth, 0, RunMaxHealth);
}

int32 UFantasyCardProgressionSubsystem::GetTotalRouteDepths() const
{
	return ChapterDefinition ? FMath::Max(1, ChapterDefinition->TotalDepths) : 6;
}

bool UFantasyCardProgressionSubsystem::LoadChapterDefinition()
{
	if (ChapterDefinition)
	{
		return true;
	}
	ChapterDefinition = LoadObject<UFantasyChapterDefinition>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/Data/Encounters/DA_Chapter_W01_AshenKingdom.DA_Chapter_W01_AshenKingdom"));
	if (!ChapterDefinition)
	{
		UE_LOG(LogTemp, Error, TEXT("W01 chapter definition failed to load."));
		return false;
	}
	const TArray<FString> ValidationErrors = ChapterDefinition->ValidateDefinition();
	if (!ValidationErrors.IsEmpty())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("W01 chapter definition is invalid. Errors=%s"),
			*FString::Join(ValidationErrors, TEXT(" | ")));
		ChapterDefinition = nullptr;
		return false;
	}
	return true;
}

void UFantasyCardProgressionSubsystem::RebuildRouteChoices()
{
	RouteChoices.Reset();
	if (!bRunStarted || bHasActiveNode || bChapterComplete)
	{
		return;
	}

	bool bEmergencyFallback = false;
	FString FailureReason;
	if (!LoadChapterDefinition()
		|| !ChapterDefinition->GenerateRouteChoices(
			ChapterDepth,
			RouteSeed,
			ContentVersion,
			Difficulty,
			LastEnemyId,
			RouteChoices,
			FailureReason))
	{
		bEmergencyFallback = true;
		UE_LOG(
			LogTemp,
			Error,
			TEXT("W01 route generation failed. Depth=%d Reason=%s"),
			ChapterDepth,
			FailureReason.IsEmpty() ? TEXT("Chapter definition unavailable") : *FailureReason);
		BuildEmergencyRouteFallback();
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_ROUTE_CHOICES_READY Depth=%d Choices=%d RouteSeed=%d ContentVersion=%s Difficulty=%s Signature=%s"),
		ChapterDepth,
		RouteChoices.Num(),
		RouteSeed,
		*ContentVersion,
		*GetDifficultyName(),
		*BuildRouteChoiceSignature());
	AddDecisionRecord(
		TEXT("RouteOffered"),
		BuildRouteChoiceSignature(),
		NAME_None);
	LogStructuredEvent(
		TEXT("RouteOffered"),
		{{TEXT("signature"), BuildRouteChoiceSignature()},
		 {TEXT("generator"), bEmergencyFallback ? TEXT("EmergencyFallback") : TEXT("ChapterDefinition")}},
		{{TEXT("choiceCount"), RouteChoices.Num()}});
}

void UFantasyCardProgressionSubsystem::BuildEmergencyRouteFallback()
{
	RouteChoices.Reset();
	FFantasyRouteNodeChoice& Choice = RouteChoices.AddDefaulted_GetRef();
	const bool bFinalDepth = ChapterDepth >= GetTotalRouteDepths() - 1;
	Choice.NodeId = FName(*FString::Printf(TEXT("D%d_EmergencyFallback"), ChapterDepth));
	Choice.DisplayName = FText::FromString(
		bFinalDepth ? TEXT("守关路线资料恢复") : TEXT("安全休整路线"));
	Choice.Description = FText::FromString(
		TEXT("章节定义不可用；使用可继续流程的紧急候选，并记录错误供内容修复。"));
	Choice.NodeType = bFinalDepth
		? EFantasyRouteNodeType::Boss
		: EFantasyRouteNodeType::Rest;
	Choice.PayloadId = bFinalDepth ? FName(TEXT("HeadlessKnightBoss")) : FName(TEXT("ExileCamp"));
}
