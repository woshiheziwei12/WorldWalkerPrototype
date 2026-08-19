#include "Cards/Fantasy/FantasyCardProgressionSubsystem.h"

#include "Cards/CardDefinition.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

const FString UFantasyCardProgressionSubsystem::DefaultContentVersion(TEXT("W01-M1-v1"));

bool UFantasyCardProgressionSubsystem::ConfigureRun(
	const int32 InRouteSeed,
	const FString& InContentVersion)
{
	if (bRunStarted || InContentVersion.IsEmpty())
	{
		return false;
	}
	RouteSeed = InRouteSeed;
	ContentVersion = InContentVersion;
	bRunConfigurationResolved = true;
	return true;
}

void UFantasyCardProgressionSubsystem::ResolveRunConfiguration()
{
	if (bRunConfigurationResolved)
	{
		return;
	}

	RouteSeed = DefaultRouteSeed;
	ContentVersion = DefaultContentVersion;
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
	bRunConfigurationResolved = true;
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
	bRunStarted = true;
	ChapterDepth = 0;
	bHasActiveNode = false;
	bChapterComplete = false;
	RunMaxHealth = 100;
	CurrentRunHealth = RunMaxHealth;
	RouteChoices.Reset();
	RebuildRouteChoices();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_RUN_STARTED Chapter=1 Depth=0 Profession=%s Choices=%d RouteSeed=%d ContentVersion=%s"),
		*GetProfessionDisplayName(),
		RouteChoices.Num(),
		RouteSeed,
		*ContentVersion);
}

void UFantasyCardProgressionSubsystem::ResetRun()
{
	GrantedCardCopies.Reset();
	RemovedCardCopies.Reset();
	RouteChoices.Reset();
	ActiveNode = FFantasyRouteNodeChoice();
	SelectedProfession = EFantasyPlayerProfession::None;
	ChapterDepth = 0;
	bRunStarted = false;
	bHasActiveNode = false;
	bChapterComplete = false;
	PendingBattleBlock = 0;
	PendingBattleValor = 0;
	RunMaxHealth = 100;
	CurrentRunHealth = RunMaxHealth;
	UE_LOG(LogTemp, Display, TEXT("W01_RUN_RESET"));
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

bool UFantasyCardProgressionSubsystem::GrantCard(const UCardDefinition* Card)
{
	if (!Card || Card->CardId.IsNone()
		|| Card->CardSetId != FName(TEXT("W01_EasternHorror")))
	{
		return false;
	}

	int32& Copies = GrantedCardCopies.FindOrAdd(Card->CardId);
	++Copies;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01 card grant persisted for this run. Card=%s Copies=%d TotalGranted=%d"),
		*Card->CardId.ToString(),
		Copies,
		GetTotalGrantedCopies());
	return true;
}

bool UFantasyCardProgressionSubsystem::RemoveCardCopy(const FName CardId)
{
	if (CardId.IsNone())
	{
		return false;
	}

	int32& Copies = RemovedCardCopies.FindOrAdd(CardId);
	++Copies;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01 run deck removed one copy. Card=%s RemovedCopies=%d TotalRemoved=%d"),
		*CardId.ToString(),
		Copies,
		GetTotalRemovedCopies());
	return true;
}

int32 UFantasyCardProgressionSubsystem::GetGrantedCopies(const FName CardId) const
{
	return GrantedCardCopies.FindRef(CardId);
}

int32 UFantasyCardProgressionSubsystem::GetRemovedCopies(const FName CardId) const
{
	return RemovedCardCopies.FindRef(CardId);
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

FString UFantasyCardProgressionSubsystem::BuildRunSummary() const
{
	return FString::Printf(
		TEXT("经典第一章 · %s · 路程 %d/6 · 生命 %d/%d · 获得 %d 张 · 移除 %d 张"),
		*GetProfessionDisplayName(),
		FMath::Clamp(ChapterDepth + 1, 1, GetTotalRouteDepths()),
		CurrentRunHealth,
		RunMaxHealth,
		GetTotalGrantedCopies(),
		GetTotalRemovedCopies());
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
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_ROUTE_NODE_SELECTED Depth=%d Node=%s Type=%d Payload=%s"),
		ChapterDepth,
		*ActiveNode.NodeId.ToString(),
		static_cast<int32>(ActiveNode.NodeType),
		*ActiveNode.PayloadId.ToString());
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

void UFantasyCardProgressionSubsystem::RebuildRouteChoices()
{
	RouteChoices.Reset();
	if (!bRunStarted || bHasActiveNode || bChapterComplete)
	{
		return;
	}

	switch (ChapterDepth)
	{
	case 0:
		AddRouteChoice(
			TEXT("D0_DrowsyBat"),
			TEXT("贪睡蝙蝠"),
			TEXT("最低确认牌名：爪击、猛击、吸血。战后可选一张牌。"),
			EFantasyRouteNodeType::Combat,
			TEXT("DrowsyBat"));
		AddRouteChoice(
			TEXT("D0_MagicApprentice"),
			TEXT("魔法学徒"),
			TEXT("最低确认牌名：法力、智慧、元素波动、火焰冲击。"),
			EFantasyRouteNodeType::Combat,
			TEXT("MagicApprentice"));
		AddRouteChoice(
			TEXT("D0_MoonlitWell"),
			TEXT("月下古井"),
			TEXT("恢复、牺牲换牌或洗掉一张基础攻击。"),
			EFantasyRouteNodeType::Event,
			TEXT("MoonlitWell"));
		break;

	case 1:
		AddRouteChoice(
			TEXT("D1_VillageGuard"),
			TEXT("村庄守卫"),
			TEXT("最低确认牌名：急行、禁止通行！、短剑、迅捷攻击。"),
			EFantasyRouteNodeType::Combat,
			TEXT("VillageGuard"));
		AddRouteChoice(
			TEXT("D1_Hypnotist"),
			TEXT("催眠师"),
			TEXT("最低确认牌名：退缩、催眠、酸性喷雾。"),
			EFantasyRouteNodeType::Combat,
			TEXT("Hypnotist"));
		AddRouteChoice(
			TEXT("D1_AshenSmith"),
			TEXT("灰烬铁匠"),
			TEXT("负伤换取一张骑士牌，或移除一张普通攻击。"),
			EFantasyRouteNodeType::Event,
			TEXT("AshenSmith"));
		break;

	case 2:
		AddRouteChoice(
			TEXT("D2_Scarecrow"),
			TEXT("稻草人"),
			TEXT("最低确认牌名：法力、火焰冲击、元素波动、火苗、法力图腾。"),
			EFantasyRouteNodeType::Combat,
			TEXT("Scarecrow"));
		AddRouteChoice(
			TEXT("D2_FortuneTeller"),
			TEXT("女占卜师"),
			TEXT("最低确认牌名：智慧、元素波动、治愈、风之石、水晶球。"),
			EFantasyRouteNodeType::Combat,
			TEXT("FortuneTeller"));
		AddRouteChoice(
			TEXT("D2_ExileCamp"),
			TEXT("流亡者营火"),
			TEXT("休整恢复生命，或带着临时护甲进入下一战。"),
			EFantasyRouteNodeType::Rest,
			TEXT("ExileCamp"));
		break;

	case 3:
		AddRouteChoice(
			TEXT("D3_DragonWhelp"),
			TEXT("飞龙幼崽 · 精英"),
			TEXT("最低确认牌名：法力、火焰冲击、元素波动。"),
			EFantasyRouteNodeType::EliteCombat,
			TEXT("DragonWhelp"));
		AddRouteChoice(
			TEXT("D3_HeadlessKnight"),
			TEXT("无头骑士 · 精英"),
			TEXT("最低确认牌名：法力、火焰冲击、忏悔。"),
			EFantasyRouteNodeType::EliteCombat,
			TEXT("HeadlessKnight"));
		AddRouteChoice(
			TEXT("D3_MoonlitWell"),
			TEXT("月下古井"),
			TEXT("恢复、牺牲换取职业牌或移除一张基础攻击。"),
			EFantasyRouteNodeType::Event,
			TEXT("MoonlitWell"));
		break;

	case 4:
		AddRouteChoice(
			TEXT("D4_ScarecrowElite"),
			TEXT("稻草人 · 精英"),
			TEXT("同一最低确认牌名集合，使用更高生命的章节后段版本。"),
			EFantasyRouteNodeType::EliteCombat,
			TEXT("ScarecrowElite"));
		AddRouteChoice(
			TEXT("D4_FortuneTellerElite"),
			TEXT("女占卜师 · 精英"),
			TEXT("同一最低确认牌名集合，使用更高生命的章节后段版本。"),
			EFantasyRouteNodeType::EliteCombat,
			TEXT("FortuneTellerElite"));
		AddRouteChoice(
			TEXT("D4_ExileCamp"),
			TEXT("流亡者营火"),
			TEXT("守关战前最后一次休整。"),
			EFantasyRouteNodeType::Rest,
			TEXT("ExileCamp"));
		break;

	default:
		AddRouteChoice(
			TEXT("D3_ChapterBoss"),
			TEXT("无头骑士 · 章节守关者"),
			TEXT("增强版法力/火焰冲击/忏悔牌组，击败后完成灰烬章节。"),
			EFantasyRouteNodeType::Boss,
			TEXT("HeadlessKnightBoss"));
		break;
	}
	ApplyDeterministicRouteOrder();

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_ROUTE_CHOICES_READY Depth=%d Choices=%d RouteSeed=%d ContentVersion=%s Signature=%s"),
		ChapterDepth,
		RouteChoices.Num(),
		RouteSeed,
		*ContentVersion,
		*BuildRouteChoiceSignature());
}

void UFantasyCardProgressionSubsystem::ApplyDeterministicRouteOrder()
{
	if (RouteChoices.Num() < 2)
	{
		return;
	}

	const uint32 SeedHash = HashCombineFast(
		GetTypeHash(RouteSeed),
		HashCombineFast(GetTypeHash(ContentVersion), GetTypeHash(ChapterDepth)));
	FRandomStream Stream(static_cast<int32>(SeedHash));
	for (int32 Index = RouteChoices.Num() - 1; Index > 0; --Index)
	{
		RouteChoices.Swap(Index, Stream.RandRange(0, Index));
	}
}

void UFantasyCardProgressionSubsystem::AddRouteChoice(
	const FName NodeId,
	const TCHAR* DisplayName,
	const TCHAR* Description,
	const EFantasyRouteNodeType NodeType,
	const FName PayloadId)
{
	FFantasyRouteNodeChoice& Choice = RouteChoices.AddDefaulted_GetRef();
	Choice.NodeId = NodeId;
	Choice.DisplayName = FText::FromString(DisplayName);
	Choice.Description = FText::FromString(Description);
	Choice.NodeType = NodeType;
	Choice.PayloadId = PayloadId;
}
