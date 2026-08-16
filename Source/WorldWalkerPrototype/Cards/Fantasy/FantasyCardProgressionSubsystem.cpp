#include "Cards/Fantasy/FantasyCardProgressionSubsystem.h"

#include "Cards/CardDefinition.h"

void UFantasyCardProgressionSubsystem::EnsureRunStarted()
{
	if (bRunStarted)
	{
		return;
	}

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
		TEXT("W01_RUN_STARTED Chapter=1 Depth=0 Choices=%d"),
		RouteChoices.Num());
}

void UFantasyCardProgressionSubsystem::ResetRun()
{
	GrantedCardCopies.Reset();
	RemovedCardCopies.Reset();
	RouteChoices.Reset();
	ActiveNode = FFantasyRouteNodeChoice();
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
		TEXT("灰烬章节 %d/4 · 生命 %d/%d · 获得 %d 张 · 移除 %d 张"),
		FMath::Clamp(ChapterDepth + 1, 1, 4),
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
			TEXT("D2_DragonWhelp"),
			TEXT("飞龙幼崽 · 精英"),
			TEXT("最低确认牌名：法力、火焰冲击、元素波动。"),
			EFantasyRouteNodeType::EliteCombat,
			TEXT("DragonWhelp"));
		AddRouteChoice(
			TEXT("D2_HeadlessKnight"),
			TEXT("无头骑士 · 精英"),
			TEXT("最低确认牌名：法力、火焰冲击、忏悔。"),
			EFantasyRouteNodeType::EliteCombat,
			TEXT("HeadlessKnight"));
		AddRouteChoice(
			TEXT("D2_ExileCamp"),
			TEXT("流亡者营火"),
			TEXT("休整恢复生命，或带着临时护甲进入守关战。"),
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

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_ROUTE_CHOICES_READY Depth=%d Choices=%d"),
		ChapterDepth,
		RouteChoices.Num());
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
