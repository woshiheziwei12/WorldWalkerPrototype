#include "WorldWalkerGameModeBase.h"

#include "Characters/WorldWalkerCharacter.h"
#include "Characters/WorldWalkerEnemy.h"
#include "Cards/CardCombatComponent.h"
#include "Cards/CardDefinition.h"
#include "Cards/Fantasy/FantasyCardProgressionSubsystem.h"
#include "Cards/Fantasy/FantasyEnemyDeckRuntime.h"
#include "Cards/Fantasy/FantasyEnemyDefinition.h"
#include "Combat/CombatantComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "World/WorldDefinition.h"
#include "World/Fantasy/FantasyBattleArena.h"
#include "World/Fantasy/FantasyWorldLayout.h"
#include "World/WorldHubLayout.h"
#include "World/WorldPortal.h"
#include "World/WorldTravelSubsystem.h"
#include "WorldWalkerPlayerController.h"

namespace
{
	FLinearColor GetCardSchoolTint(const ECardSchool School)
	{
		switch (School)
		{
		case ECardSchool::Steel: return FLinearColor(0.42f, 0.49f, 0.58f, 1.0f);
		case ECardSchool::Faith: return FLinearColor(0.82f, 0.60f, 0.16f, 1.0f);
		case ECardSchool::Arcane: return FLinearColor(0.31f, 0.16f, 0.68f, 1.0f);
		case ECardSchool::None:
		default: return FLinearColor(0.55f, 0.14f, 0.10f, 1.0f);
		}
	}
}

AWorldWalkerGameModeBase::AWorldWalkerGameModeBase()
{
	DefaultPawnClass = AWorldWalkerCharacter::StaticClass();
	PlayerControllerClass = AWorldWalkerPlayerController::StaticClass();
}

void AWorldWalkerGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	ActivePlayer = Cast<AWorldWalkerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	InitializeWorldContent();

	UE_LOG(
		LogTemp,
		Display,
		TEXT("WorldWalker world ready. World=%s Player=%s Enemy=%s Portal=%s Hub=%s FantasyWorld=%s"),
		CurrentWorldDefinition ? *CurrentWorldDefinition->WorldId.ToString() : TEXT("Unregistered"),
		ActivePlayer ? TEXT("spawned") : TEXT("missing"),
		ActiveEnemy ? TEXT("spawned") : TEXT("none"),
		ActivePortal ? TEXT("spawned") : TEXT("none"),
		ActiveHub ? TEXT("spawned") : TEXT("none"),
		ActiveFantasyWorld ? TEXT("spawned") : TEXT("none"));
}

void AWorldWalkerGameModeBase::InitializeWorldContent()
{
	if (!ActivePlayer)
	{
		return;
	}

	UWorldTravelSubsystem* TravelSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UWorldTravelSubsystem>()
		: nullptr;
	CurrentWorldDefinition = TravelSubsystem
		? TravelSubsystem->ResolveCurrentWorldDefinition(this)
		: nullptr;

	if (!TravelSubsystem || !CurrentWorldDefinition)
	{
		ExplorationMessage = TEXT("UNREGISTERED TEST MAP\nApproach the red enemy and press E");
		SpawnTestEnemy();
		return;
	}

	if (CurrentWorldDefinition->WorldId == UWorldTravelSubsystem::MainWorldId)
	{
		ExplorationMessage = TEXT("MAIN WORLD - IMMORTAL HUB\nWalk into the vortex portal to travel");
		SpawnMainWorldHub(TravelSubsystem->GetWorldDefinition(UWorldTravelSubsystem::EasternHorrorWorldId));
		return;
	}

	if (CurrentWorldDefinition->WorldId == UWorldTravelSubsystem::EasternHorrorWorldId)
	{
		ExplorationMessage = TEXT("灰烬王国 · 月圆旅途\n选择路线→挑战/事件→战后三选一→章节守关者");
		ActivePlayer->ConfigureFantasyWorldForm(true, true);
		ActiveCardCombat = ActivePlayer->GetCardCombatComponent();
		if (ActiveCardCombat)
		{
			ActiveCardCombat->LoadStartingDeck();
		}
		SpawnFantasyWorldLayout();
		SpawnFantasyBattleArena();
		const FVector GroundOrigin = ActivePlayer->GetActorLocation() - FVector(0.0f, 0.0f, 88.0f);
		const FVector PortalOffset = ActiveFantasyWorld
			? ActiveFantasyWorld->GetReturnPortalLocation() - GroundOrigin
			: ActivePlayer->GetActorRightVector().GetSafeNormal2D() * 650.0f;
		SpawnPortal(
			TravelSubsystem->GetWorldDefinition(UWorldTravelSubsystem::MainWorldId),
			PortalOffset);
		InitializeFantasyRun();
	}
}

void AWorldWalkerGameModeBase::SpawnMainWorldHub(UWorldDefinition* DestinationWorld)
{
	if (!ActivePlayer || !DestinationWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot spawn main-world hub: player or destination WorldDefinition is missing."));
		return;
	}

	const FVector GroundOrigin = ActivePlayer->GetActorLocation() - FVector(0.0f, 0.0f, 88.0f);
	const FVector Forward = ActivePlayer->GetActorForwardVector().GetSafeNormal2D();
	const FVector HubLocation = GroundOrigin + Forward * 850.0f;
	const FRotator HubRotation(0.0f, Forward.Rotation().Yaw, 0.0f);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActiveHub = GetWorld()->SpawnActor<AWorldHubLayout>(
		AWorldHubLayout::StaticClass(),
		HubLocation,
		HubRotation,
		SpawnParameters);

	if (ActiveHub)
	{
		SpawnPortal(DestinationWorld, ActiveHub->GetActivePortalLocation() - GroundOrigin);
	}
}

void AWorldWalkerGameModeBase::SpawnTestEnemy()
{
	if (!ActivePlayer)
	{
		return;
	}

	const FVector Forward = ActiveFantasyWorld
		? ActiveFantasyWorld->GetActorForwardVector().GetSafeNormal2D()
		: ActivePlayer->GetActorForwardVector().GetSafeNormal2D();
	const FVector SpawnLocation = ActiveFantasyWorld
		? ActiveFantasyWorld->GetBattleAnchorLocation() + Forward * 145.0f + FVector(0.0f, 0.0f, 112.0f)
		: ActivePlayer->GetActorLocation() + Forward * 520.0f + FVector(0.0f, 0.0f, 24.0f);
	const FRotator SpawnRotation = (ActivePlayer->GetActorLocation() - SpawnLocation).Rotation();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ActiveEnemy = GetWorld()->SpawnActor<AWorldWalkerEnemy>(
		AWorldWalkerEnemy::StaticClass(),
		SpawnLocation,
		FRotator(0.0f, SpawnRotation.Yaw, 0.0f),
		SpawnParameters);

	if (ActiveEnemy && CurrentWorldDefinition
		&& CurrentWorldDefinition->WorldId == UWorldTravelSubsystem::EasternHorrorWorldId)
	{
		const FText EnemyName = ActiveFantasyEnemyDefinition
			? ActiveFantasyEnemyDefinition->DisplayName
			: FText::FromString(TEXT("黑棘誓约骑士"));
		ActiveEnemy->ConfigureFantasyPresentation(
			EnemyName,
			ActiveFantasyEnemyDefinition
				? ActiveFantasyEnemyDefinition->VisualProfile
				: EFantasyEnemyVisualProfile::Warrior);
		if (ActiveFantasyEnemyDefinition)
		{
			ActiveEnemy->GetCombatantComponent()->ConfigureMaxHealth(ActiveFantasyEnemyDefinition->MaxHealth);
		}
	}
}

void AWorldWalkerGameModeBase::SpawnFantasyWorldLayout()
{
	if (!ActivePlayer || !GetWorld())
	{
		return;
	}

	const FVector Forward = ActivePlayer->GetActorForwardVector().GetSafeNormal2D();
	const FVector GroundOrigin = ActivePlayer->GetActorLocation() - FVector(0.0f, 0.0f, 88.0f);
	const FRotator WorldRotation(0.0f, Forward.Rotation().Yaw, 0.0f);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActiveFantasyWorld = GetWorld()->SpawnActor<AFantasyWorldLayout>(
		AFantasyWorldLayout::StaticClass(),
		GroundOrigin,
		WorldRotation,
		SpawnParameters);
}

void AWorldWalkerGameModeBase::SpawnFantasyBattleArena()
{
	if (!ActivePlayer || !GetWorld())
	{
		return;
	}

	const FVector Forward = ActiveFantasyWorld
		? ActiveFantasyWorld->GetActorForwardVector().GetSafeNormal2D()
		: ActivePlayer->GetActorForwardVector().GetSafeNormal2D();
	const FVector GroundOrigin = ActivePlayer->GetActorLocation() - FVector(0.0f, 0.0f, 88.0f);
	const FVector ArenaLocation = ActiveFantasyWorld
		? ActiveFantasyWorld->GetBattleAnchorLocation()
		: GroundOrigin + Forward * 260.0f;
	const FRotator ArenaRotation(0.0f, Forward.Rotation().Yaw, 0.0f);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActiveFantasyArena = GetWorld()->SpawnActor<AFantasyBattleArena>(
		AFantasyBattleArena::StaticClass(),
		ArenaLocation,
		ArenaRotation,
		SpawnParameters);
}

bool AWorldWalkerGameModeBase::LoadFantasyEnemyDefinition(const FName EnemyId)
{
	if (EnemyId.IsNone())
	{
		return false;
	}

	const FString AssetName = FString::Printf(TEXT("DA_Enemy_%s"), *EnemyId.ToString());
	const FString EnemyDefinitionPath = FString::Printf(
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/Data/Enemies/%s.%s"),
		*AssetName,
		*AssetName);
	ActiveFantasyEnemyDefinition = LoadObject<UFantasyEnemyDefinition>(
		nullptr,
		*EnemyDefinitionPath);
	if (!ActiveFantasyEnemyDefinition)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("W01 enemy definition is missing. EnemyId=%s Path=%s"),
			*EnemyId.ToString(),
			*EnemyDefinitionPath);
		return false;
	}

	return true;
}

void AWorldWalkerGameModeBase::InitializeFantasyRun()
{
	if (!GetGameInstance() || !ActivePlayer)
	{
		return;
	}

	if (UFantasyCardProgressionSubsystem* Progression =
		GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>())
	{
		Progression->EnsureRunStarted();
	}
	RestoreRunHealthToPlayer();
	GetWorldTimerManager().SetTimerForNextTick(
		this,
		&AWorldWalkerGameModeBase::ResumeOrPresentFantasyRun);
}

void AWorldWalkerGameModeBase::ResumeOrPresentFantasyRun()
{
	UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	if (!Progression || !ActivePlayer)
	{
		return;
	}

	if (Progression->IsChapterComplete())
	{
		FantasyRunFlowState = EFantasyRunFlowState::ChapterComplete;
		ActivePlayer->SetCombatLocked(true);
		if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
		{
			Controller->ShowNodeResolution(
				TEXT("你已完成灰烬章节。可继续探索世界，或从返回门回到主世界。"),
				true);
		}
		return;
	}

	if (Progression->HasActiveNode())
	{
		ActivateRouteNode(Progression->GetActiveNode());
		return;
	}

	PresentRouteChoices();
}

void AWorldWalkerGameModeBase::PresentRouteChoices()
{
	UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	AWorldWalkerPlayerController* Controller = GetWorldWalkerController();
	if (!Progression || !Controller || !ActivePlayer || Progression->IsChapterComplete())
	{
		return;
	}

	TArray<FString> ChoiceLabels;
	for (const FFantasyRouteNodeChoice& Choice : Progression->GetRouteChoices())
	{
		const TCHAR* TypeLabel = TEXT("战斗");
		switch (Choice.NodeType)
		{
		case EFantasyRouteNodeType::EliteCombat: TypeLabel = TEXT("精英战"); break;
		case EFantasyRouteNodeType::Event: TypeLabel = TEXT("事件"); break;
		case EFantasyRouteNodeType::Rest: TypeLabel = TEXT("休整"); break;
		case EFantasyRouteNodeType::Boss: TypeLabel = TEXT("守关战"); break;
		case EFantasyRouteNodeType::Combat:
		default: break;
		}
		ChoiceLabels.Add(FString::Printf(
			TEXT("【%s】%s\n%s"),
			TypeLabel,
			*Choice.DisplayName.ToString(),
			*Choice.Description.ToString()));
	}

	FantasyRunFlowState = EFantasyRunFlowState::RouteChoice;
	ActivePlayer->SetCombatLocked(true);
	Controller->ShowRouteSelection(Progression->BuildRunSummary(), ChoiceLabels);
}

void AWorldWalkerGameModeBase::HandleRouteSelection(const int32 ChoiceIndex)
{
	if (FantasyRunFlowState != EFantasyRunFlowState::RouteChoice || !GetGameInstance())
	{
		return;
	}

	UFantasyCardProgressionSubsystem* Progression =
		GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>();
	FFantasyRouteNodeChoice SelectedNode;
	if (!Progression || !Progression->SelectRouteChoice(ChoiceIndex, SelectedNode))
	{
		PresentRouteChoices();
		return;
	}
	ActivateRouteNode(SelectedNode);
}

void AWorldWalkerGameModeBase::ActivateRouteNode(const FFantasyRouteNodeChoice& Node)
{
	if (Node.IsCombat())
	{
		PrepareCombatNode(Node.PayloadId);
		return;
	}

	PresentEventChoices(Node.PayloadId);
}

void AWorldWalkerGameModeBase::PrepareCombatNode(const FName EnemyId)
{
	if (!ActivePlayer)
	{
		return;
	}

	ClearActiveEnemy();
	if (!LoadFantasyEnemyDefinition(EnemyId))
	{
		CompleteActiveNode(FString::Printf(
			TEXT("对手资料【%s】缺失，已跳过该节点以保护旅途进度。"),
			*EnemyId.ToString()));
		return;
	}

	SpawnTestEnemy();
	if (!ActiveEnemy)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("W01 combat actor failed to spawn. Enemy=%s"),
			*EnemyId.ToString());
		ClearActiveEnemy();
		CompleteActiveNode(FString::Printf(
			TEXT("对手【%s】未能生成，已安全跳过该节点。"),
			*EnemyId.ToString()));
		return;
	}
	FantasyRunFlowState = EFantasyRunFlowState::Exploration;
	ActivePlayer->SetCombatLocked(false);
	ExplorationMessage = FString::Printf(
		TEXT("当前节点：%s\n前往道路尽头，靠近对手并按 E 挑战。"),
		ActiveFantasyEnemyDefinition
			? *ActiveFantasyEnemyDefinition->DisplayName.ToString()
			: TEXT("未知对手"));
	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		Controller->ExitCombatToExploration(ExplorationMessage);
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_COMBAT_NODE_READY Enemy=%s Spawned=%d"),
		*EnemyId.ToString(),
		ActiveEnemy ? 1 : 0);
}

void AWorldWalkerGameModeBase::ClearActiveEnemy()
{
	if (ActiveEnemy)
	{
		ActiveEnemy->Destroy();
		ActiveEnemy = nullptr;
	}
	ActiveEnemyDeck = nullptr;
	ActiveFantasyEnemyDefinition = nullptr;
}

void AWorldWalkerGameModeBase::PresentEventChoices(const FName EventId)
{
	if (!ActivePlayer)
	{
		return;
	}

	FString Title;
	FString Lore;
	TArray<FString> Choices;
	if (EventId == TEXT("MoonlitWell"))
	{
		Title = TEXT("月下古井");
		Lore = TEXT("井水映出了三个不同的你。每一道倒影都会永久改变本次旅途。");
		Choices = {
			TEXT("饮下银色井水：恢复 25 点生命"),
			TEXT("触碰血色倒影：失去至多 8 生命（保留 1），获得【迅捷攻击】"),
			TEXT("沉下沉重倒影：从旅途牌组移除 1 张【普通攻击】")};
	}
	else if (EventId == TEXT("AshenSmith"))
	{
		Title = TEXT("灰烬铁匠");
		Lore = TEXT("失明的铁匠只凭声音敲打剑身。他要求你用伤痕、旧剑或一个承诺付账。");
		Choices = {
			TEXT("以血淬火：失去至多 10 生命（保留 1），获得【绝对防御】"),
			TEXT("熔掉旧剑：移除 1 张【普通攻击】"),
			TEXT("记下盾阵：下一场战斗开始时获得 12 格挡")};
	}
	else
	{
		Title = TEXT("流亡者营火");
		Lore = TEXT("无名者们为你留下火种、旧盾和一句祝词。黑棘庭院已在前方。");
		Choices = {
			TEXT("靠近营火休息：恢复 35 点生命"),
			TEXT("接过旧盾：下一战获得 8 格挡"),
			TEXT("保持警惕：不获得任何效果，继续旅途")};
	}

	CurrentEventId = EventId;
	FantasyRunFlowState = EFantasyRunFlowState::EventChoice;
	ActivePlayer->SetCombatLocked(true);
	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		Controller->ShowEventSelection(Title, Lore, Choices);
	}
}

void AWorldWalkerGameModeBase::HandleEventSelection(const int32 ChoiceIndex)
{
	if (FantasyRunFlowState != EFantasyRunFlowState::EventChoice || !ActivePlayer
		|| ChoiceIndex < 0 || ChoiceIndex > 2)
	{
		return;
	}

	UCombatantComponent* PlayerCombatant = ActivePlayer->GetCombatantComponent();
	UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	FString Result;
	auto ApplyNonLethalHealthLoss = [PlayerCombatant](const int32 RequestedLoss) -> int32
	{
		if (!PlayerCombatant || !PlayerCombatant->IsAlive())
		{
			return 0;
		}
		const int32 AppliedLoss = FMath::Min(
			FMath::Max(0, RequestedLoss),
			FMath::Max(0, PlayerCombatant->GetCurrentHealth() - 1));
		PlayerCombatant->ReceiveDamage(AppliedLoss);
		return AppliedLoss;
	};
	auto GrantCardById = [this](const FName CardId) -> bool
	{
		return ActiveCardCombat
			&& ActiveCardCombat->GrantRunCard(ActiveCardCombat->FindCardDefinition(CardId));
	};

	if (CurrentEventId == TEXT("MoonlitWell"))
	{
		if (ChoiceIndex == 0)
		{
			PlayerCombatant->RestoreHealth(25);
			Result = TEXT("冰凉井水洗去了疲惫。恢复 25 点生命。");
		}
		else if (ChoiceIndex == 1)
		{
			const int32 HealthLost = ApplyNonLethalHealthLoss(8);
			const bool bGranted = GrantCardById(TEXT("Knight_SwiftAttack"));
			Result = bGranted
				? FString::Printf(TEXT("倒影夺走 %d 点生命，却将【迅捷攻击】留在你手中。"), HealthLost)
				: FString::Printf(TEXT("你失去 %d 点生命，但卡牌资料尚未生成。"), HealthLost);
		}
		else
		{
			const bool bRemoved = ActiveCardCombat
				&& ActiveCardCombat->RemoveCardFromRun(TEXT("Knight_NormalAttack"));
			Result = bRemoved
				? TEXT("一张【普通攻击】沉入井底，旅途牌组更精简了。")
				: TEXT("没有可移除的【普通攻击】，倒影随水波散去。");
		}
	}
	else if (CurrentEventId == TEXT("AshenSmith"))
	{
		if (ChoiceIndex == 0)
		{
			const int32 HealthLost = ApplyNonLethalHealthLoss(10);
			const bool bGranted = GrantCardById(TEXT("Knight_AbsoluteDefense"));
			Result = bGranted
				? FString::Printf(TEXT("铁锤落下，你失去 %d 点生命并获得【绝对防御】。"), HealthLost)
				: FString::Printf(TEXT("你失去 %d 点生命，但卡牌资料尚未生成。"), HealthLost);
		}
		else if (ChoiceIndex == 1)
		{
			const bool bRemoved = ActiveCardCombat
				&& ActiveCardCombat->RemoveCardFromRun(TEXT("Knight_NormalAttack"));
			Result = bRemoved
				? TEXT("旧剑融化，一张【普通攻击】已从牌组移除。")
				: TEXT("牌组里已没有可熔的【普通攻击】。");
		}
		else
		{
			if (Progression)
			{
				Progression->AddPendingBattleBoon(12, 0);
			}
			Result = TEXT("你记住了盾阵的每个节奏。下一战初始格挡 +12。");
		}
	}
	else
	{
		if (ChoiceIndex == 0)
		{
			PlayerCombatant->RestoreHealth(35);
			Result = TEXT("营火让你恢复了 35 点生命。");
		}
		else if (ChoiceIndex == 1)
		{
			if (Progression)
			{
				Progression->AddPendingBattleBoon(8, 0);
			}
			Result = TEXT("你将旧盾挂在背后。下一战初始格挡 +8。");
		}
		else
		{
			Result = TEXT("你没有停留，只把火光记在了心里。");
		}
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_EVENT_RESOLVED Event=%s Choice=%d"),
		*CurrentEventId.ToString(),
		ChoiceIndex);
	SyncRunHealthFromPlayer();
	CompleteActiveNode(Result);
}

void AWorldWalkerGameModeBase::CompleteActiveNode(const FString& ResolutionMessage)
{
	UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	if (!Progression || !Progression->CompleteActiveNode())
	{
		return;
	}

	CurrentEventId = NAME_None;
	const bool bChapterComplete = Progression->IsChapterComplete();
	FantasyRunFlowState = bChapterComplete
		? EFantasyRunFlowState::ChapterComplete
		: EFantasyRunFlowState::Resolution;
	if (ActivePlayer)
	{
		ActivePlayer->SetCombatLocked(true);
	}
	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		Controller->ShowNodeResolution(ResolutionMessage, bChapterComplete);
	}
	if (bChapterComplete)
	{
		UE_LOG(LogTemp, Display, TEXT("W01_CHAPTER_COMPLETE Chapter=1"));
	}
}

void AWorldWalkerGameModeBase::HandleNodeResolutionContinue()
{
	if (FantasyRunFlowState != EFantasyRunFlowState::Resolution
		&& FantasyRunFlowState != EFantasyRunFlowState::ChapterComplete)
	{
		return;
	}

	if (FantasyRunFlowState == EFantasyRunFlowState::ChapterComplete)
	{
		if (ActivePlayer)
		{
			ActivePlayer->SetCombatLocked(false);
		}
		if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
		{
			Controller->ExitCombatToExploration(
				TEXT("灰烬章节已完成。你可以与营地居民交谈，或从右后方的门返回主世界。"));
		}
		return;
	}

	PresentRouteChoices();
}

void AWorldWalkerGameModeBase::SpawnPortal(
	UWorldDefinition* DestinationWorld,
	const FVector& OffsetFromPlayer)
{
	if (!ActivePlayer || !DestinationWorld)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot spawn portal: player or destination WorldDefinition is missing."));
		return;
	}

	const FVector GroundOrigin = ActivePlayer->GetActorLocation() - FVector(0.0f, 0.0f, 88.0f);
	const FVector SpawnLocation = GroundOrigin + OffsetFromPlayer;
	const FRotator FacingRotation = (GroundOrigin - SpawnLocation).Rotation();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActivePortal = GetWorld()->SpawnActor<AWorldPortal>(
		AWorldPortal::StaticClass(),
		SpawnLocation,
		FRotator(0.0f, FacingRotation.Yaw, 0.0f),
		SpawnParameters);
	if (ActivePortal)
	{
		ActivePortal->ConfigurePortal(DestinationWorld);
	}
}

void AWorldWalkerGameModeBase::StartCombat(
	AWorldWalkerCharacter* PlayerCharacter,
	AWorldWalkerEnemy* EnemyCharacter)
{
	if (bCombatActive || !PlayerCharacter || !EnemyCharacter)
	{
		return;
	}

	ActivePlayer = PlayerCharacter;
	ActiveEnemy = EnemyCharacter;
	ActiveCardCombat = ActivePlayer->GetCardCombatComponent();
	if (!ActiveFantasyEnemyDefinition)
	{
		const UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
			: nullptr;
		if (!Progression || !Progression->HasActiveNode()
			|| !LoadFantasyEnemyDefinition(Progression->GetActiveNode().PayloadId))
		{
			UE_LOG(LogTemp, Error, TEXT("Cannot start W01 combat: active enemy definition is unavailable."));
			return;
		}
	}
	if (!ActiveCardCombat || !ActiveCardCombat->StartBattle())
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot start card combat: starter deck is unavailable."));
		return;
	}

	ActiveEnemyDeck = NewObject<UFantasyEnemyDeckRuntime>(this);
	const bool bUsingRealEnemyDeck = ActiveEnemyDeck
		&& ActiveEnemyDeck->Initialize(ActiveFantasyEnemyDefinition);
	if (bUsingRealEnemyDeck)
	{
		// Draw the publicly previewed hand before the player's first turn. After
		// every enemy action we prepare the following hand in the same way.
		ActiveEnemyDeck->StartTurn();
	}
	else
	{
		ActiveEnemyDeck = nullptr;
	}

	bCombatActive = true;
	bWaitingForEnemy = false;
	bAwaitingRewardSelection = false;
	bRewardReadyToLeave = false;
	PendingRewardChoices.Reset();
	CurrentEnemyIntentIndex = 0;
	CurrentEnemyTurnNumber = 0;
	bEnemyDefeatPassiveConsumed = false;
	bEnemyReactivePassiveTriggered = false;
	PlayerFantasyState = FFantasyCombatRuntimeState();
	EnemyFantasyState = FFantasyCombatRuntimeState();
	if (ActiveFantasyEnemyDefinition)
	{
		ActiveEnemy->GetCombatantComponent()->ConfigureMaxHealth(ActiveFantasyEnemyDefinition->MaxHealth);
		if (ActiveFantasyEnemyDefinition->PassiveId == TEXT("GuardPost"))
		{
			EnemyFantasyState.Block = 6;
		}
		else if (ActiveFantasyEnemyDefinition->PassiveId == TEXT("DragonScale"))
		{
			EnemyFantasyState.Block = 8;
		}
	}
	if (UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr)
	{
		int32 InitialBlock = 0;
		int32 InitialValor = 0;
		Progression->ConsumePendingBattleBoon(InitialBlock, InitialValor);
		ActiveCardCombat->AddBlock(InitialBlock);
		ActiveCardCombat->AddValor(InitialValor);
	}

	FantasyRunFlowState = EFantasyRunFlowState::PlayerTurn;
	ActivePlayer->SetCombatLocked(true);
	ActiveEnemy->SetInCombat(true);

	const FVector FacingDirection = ActiveEnemy->GetActorLocation() - ActivePlayer->GetActorLocation();
	ActivePlayer->SetActorRotation(FRotator(0.0f, FacingDirection.Rotation().Yaw, 0.0f));

	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		const UCombatantComponent* PlayerCombatant = ActivePlayer->GetCombatantComponent();
		const UCombatantComponent* EnemyCombatant = ActiveEnemy->GetCombatantComponent();
		Controller->EnterCombat(
			PlayerCombatant->GetCurrentHealth(),
			PlayerCombatant->GetMaxHealth(),
			EnemyCombatant->GetCurrentHealth(),
			EnemyCombatant->GetMaxHealth(),
			ActiveFantasyEnemyDefinition
				? ActiveFantasyEnemyDefinition->DisplayName.ToString()
				: TEXT("未知对手"));
		RefreshCombatUI();
		Controller->SetCombatMessage(
			TEXT("你的回合：攻击牌无需资源；行动牌消耗每回合刷新的行动力，咒术消耗战斗内积累的法力。"),
			true);
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_COMBAT_STARTED Enemy=%s RealDeck=%d PlayerDeck=%d"),
		ActiveFantasyEnemyDefinition ? *ActiveFantasyEnemyDefinition->EnemyId.ToString() : TEXT("Unknown"),
		bUsingRealEnemyDeck ? 1 : 0,
		ActiveCardCombat->GetStartingDeckCount());
}

void AWorldWalkerGameModeBase::HandlePlayCard(const int32 HandIndex)
{
	if (!bCombatActive || bWaitingForEnemy
		|| FantasyRunFlowState != EFantasyRunFlowState::PlayerTurn
		|| !ActivePlayer || !ActiveEnemy || !ActiveCardCombat)
	{
		return;
	}

	UCardDefinition* Card = ActiveCardCombat->PlayCard(HandIndex);
	if (!Card)
	{
		if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
		{
			Controller->SetCombatMessage(TEXT("行动力或法力不足，暂时无法打出这张牌。"), true);
		}
		return;
	}

	const int32 EnemyHealthBeforeCard = ActiveEnemy->GetCombatantComponent()->GetCurrentHealth();
	FString TriggeredCounterName;
	if (ActiveEnemyDeck)
	{
		if (UCardDefinition* CounterCard = ActiveEnemyDeck->ConsumeNextCounter())
		{
			TriggeredCounterName = CounterCard->DisplayName.ToString();
			ResolveEnemyCardEffects(CounterCard);
			ActiveEnemyDeck->FinalizeTriggeredCounter(CounterCard);
			if (!ActivePlayer->GetCombatantComponent()->IsAlive())
			{
				ActiveCardCombat->FinalizePlayedCard(Card);
				SyncRunHealthFromPlayer();
				FinishCombat(false);
				return;
			}
		}
	}
	ResolvePlayerCardEffects(Card);
	ActiveCardCombat->FinalizePlayedCard(Card);
	bool bTriggeredReactivePassive = false;
	if (!bEnemyReactivePassiveTriggered && ActiveFantasyEnemyDefinition
		&& ActiveFantasyEnemyDefinition->PassiveId == TEXT("Drowsy")
		&& ActiveEnemy->GetCombatantComponent()->IsAlive()
		&& ActiveEnemy->GetCombatantComponent()->GetCurrentHealth() < EnemyHealthBeforeCard)
	{
		bEnemyReactivePassiveTriggered = true;
		bTriggeredReactivePassive = true;
		EnemyFantasyState.Strength += 1;
		ActiveEnemy->PlayIntentAnimation(EWorldWalkerEnemyAnimationCue::Empower);
		UE_LOG(LogTemp, Display, TEXT("W01_ENEMY_PASSIVE_TRIGGERED Enemy=DrowsyBat Passive=Drowsy"));
	}

	SyncRunHealthFromPlayer();
	RefreshCombatUI();

	if (!ActiveEnemy->GetCombatantComponent()->IsAlive())
	{
		if (!TryTriggerEnemyDefeatPassive())
		{
			FinishCombat(true);
		}
		return;
	}

	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		FString Message = FString::Printf(TEXT("打出：%s。"), *Card->DisplayName.ToString());
		if (!TriggeredCounterName.IsEmpty())
		{
			Message += FString::Printf(TEXT(" 敌人的反制【%s】已触发。"), *TriggeredCounterName);
		}
		if (bTriggeredReactivePassive)
		{
			Message += TEXT(" 贪睡蝙蝠被惊醒：力量 +1。");
		}
		if (!ActiveCardCombat->HasAnyPlayableCard())
		{
			Message += TEXT(" 当前已无可用牌，可以结束回合。");
		}
		Controller->SetCombatMessage(Message, true);
	}
}

void AWorldWalkerGameModeBase::HandleEndPlayerTurn()
{
	if (!bCombatActive || bWaitingForEnemy
		|| FantasyRunFlowState != EFantasyRunFlowState::PlayerTurn
		|| !ActiveCardCombat)
	{
		return;
	}

	bWaitingForEnemy = true;
	FantasyRunFlowState = EFantasyRunFlowState::EnemyTurn;
	ActiveCardCombat->EndPlayerTurn();
	ResolveEndOfTurnPoison(true);
	SyncRunHealthFromPlayer();
	RefreshCombatUI();
	if (!ActivePlayer || !ActivePlayer->GetCombatantComponent()->IsAlive())
	{
		FinishCombat(false);
		return;
	}
	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		Controller->SetCombatMessage(FString::Printf(
			TEXT("%s 正在打出已公开的下一张牌……"),
			ActiveFantasyEnemyDefinition
				? *ActiveFantasyEnemyDefinition->DisplayName.ToString()
				: TEXT("对手")), false);
	}

	GetWorldTimerManager().SetTimer(
		EnemyTurnTimer,
		this,
		&AWorldWalkerGameModeBase::HandleEnemyTurn,
		0.7f,
		false);
}

void AWorldWalkerGameModeBase::HandleEnemyTurn()
{
	if (!bCombatActive || !ActivePlayer || !ActiveEnemy || !ActiveCardCombat)
	{
		return;
	}

	// A block generated by the enemy survives the player's turn and expires here.
	EnemyFantasyState.Block = 0;
	ApplyEnemyTurnStartEquipment();
	++CurrentEnemyTurnNumber;
	FString ResolvedSummary;
	if (ActiveEnemyDeck && ActiveEnemyDeck->IsInitialized())
	{
		TArray<FString> PlayedNames;
		while (UCardDefinition* EnemyCard = ActiveEnemyDeck->PlayNextCard())
		{
			PlayedNames.Add(EnemyCard->DisplayName.ToString());
			if (EnemyCard->CardType == ECardType::Counter)
			{
				// Counter effects are armed now and resolved exactly once, before
				// the player's next card. FinalizeCard moves them to CounterZone.
				ActiveEnemy->PlayIntentAnimation(EWorldWalkerEnemyAnimationCue::Empower);
			}
			else
			{
				ResolveEnemyCardEffects(EnemyCard);
			}
			ActiveEnemyDeck->FinalizeCard(EnemyCard);
			if (!ActivePlayer->GetCombatantComponent()->IsAlive())
			{
				break;
			}
		}
		ResolvedSummary = PlayedNames.IsEmpty()
			? TEXT("对手没有可支付的牌，跳过了行动")
			: FString::Printf(TEXT("对手打出：%s"), *FString::Join(PlayedNames, TEXT("、")));
	}
	else if (ActiveFantasyEnemyDefinition
		&& !ActiveFantasyEnemyDefinition->IntentCycle.IsEmpty())
	{
		const FFantasyEnemyIntentStep& Intent =
			ActiveFantasyEnemyDefinition->IntentCycle[CurrentEnemyIntentIndex];
		ResolveEnemyIntent(Intent);
		CurrentEnemyIntentIndex =
			(CurrentEnemyIntentIndex + 1) % ActiveFantasyEnemyDefinition->IntentCycle.Num();
		ResolvedSummary = FString::Printf(TEXT("%s 已结算"), *Intent.DisplayName.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot execute W01 enemy turn: deck and intent fallback are unavailable."));
		FinishCombat(false);
		return;
	}
	SyncRunHealthFromPlayer();
	RefreshCombatUI();

	if (!ActivePlayer->GetCombatantComponent()->IsAlive())
	{
		FinishCombat(false);
		return;
	}

	ResolveEndOfTurnPoison(false);
	SyncRunHealthFromPlayer();
	if (!ActiveEnemy->GetCombatantComponent()->IsAlive())
	{
		if (!TryTriggerEnemyDefeatPassive())
		{
			FinishCombat(true);
			return;
		}
	}

	if (ActiveEnemyDeck && ActiveEnemyDeck->IsInitialized())
	{
		ActiveEnemyDeck->StartTurn();
	}

	ActiveCardCombat->StartPlayerTurn();
	bWaitingForEnemy = false;
	FantasyRunFlowState = EFantasyRunFlowState::PlayerTurn;
	RefreshCombatUI();
	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		Controller->SetCombatMessage(FString::Printf(
			TEXT("%s。轮到你行动，敌人的下一张牌已经公开。"),
			*ResolvedSummary), true);
	}
}

void AWorldWalkerGameModeBase::ResolvePlayerCardEffects(UCardDefinition* Card)
{
	if (!Card || !ActivePlayer || !ActiveEnemy || !ActiveCardCombat)
	{
		return;
	}

	const bool bHasOpponentDamage = Card->Effects.ContainsByPredicate([](const FFantasyCombatEffectSpec& Effect)
	{
		return Effect.EffectType == EFantasyCombatEffectType::Damage
			&& Effect.Target == EFantasyCombatTarget::Opponent;
	});
	if (bHasOpponentDamage)
	{
		ActivePlayer->PlayFantasyCardAttackAnimation();
	}

	bool bWeakConsumedForAttack = false;
	for (const FFantasyCombatEffectSpec& Effect : Card->Effects)
	{
		switch (Effect.EffectType)
		{
		case EFantasyCombatEffectType::Damage:
			if (Effect.Target == EFantasyCombatTarget::Opponent)
			{
				const bool bIsAttack = Card->CardType == ECardType::Attack;
				const int32 HealthDamage = ResolveDamageAgainstEnemy(
					Effect.Magnitude,
					bIsAttack,
					bIsAttack && !bWeakConsumedForAttack,
					Effect.bPiercing);
				ActiveEnemy->GetCombatantComponent()->ReceiveDamage(HealthDamage);
				if (HealthDamage > 0)
				{
					ActiveEnemy->PlayIntentAnimation(EWorldWalkerEnemyAnimationCue::HitReact);
				}
				bWeakConsumedForAttack |= bIsAttack;
			}
			break;
		case EFantasyCombatEffectType::Block:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				ActiveCardCombat->AddBlock(Effect.Magnitude);
			}
			else
			{
				EnemyFantasyState.Block += FMath::Max(0, Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::Heal:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				ActivePlayer->GetCombatantComponent()->RestoreHealth(Effect.Magnitude);
			}
			else
			{
				ActiveEnemy->GetCombatantComponent()->RestoreHealth(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::Draw:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				ActiveCardCombat->DrawCards(Effect.Magnitude);
			}
			else if (ActiveEnemyDeck)
			{
				ActiveEnemyDeck->DrawCards(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::ApplyStatus:
			(Effect.Target == EFantasyCombatTarget::Self ? PlayerFantasyState : EnemyFantasyState)
				.AddStatus(Effect.Status, Effect.Magnitude);
			break;
		case EFantasyCombatEffectType::RemoveStatus:
			(Effect.Target == EFantasyCombatTarget::Self ? PlayerFantasyState : EnemyFantasyState)
				.RemoveStatus(Effect.Status, Effect.Magnitude);
			break;
		case EFantasyCombatEffectType::GainValor:
			ActiveCardCombat->AddValor(Effect.Magnitude);
			break;
		case EFantasyCombatEffectType::GainAction:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				ActiveCardCombat->AddActionPoints(Effect.Magnitude);
			}
			else if (ActiveEnemyDeck)
			{
				ActiveEnemyDeck->AddAction(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::GainMana:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				ActiveCardCombat->AddMana(Effect.Magnitude);
			}
			else if (ActiveEnemyDeck)
			{
				ActiveEnemyDeck->AddMana(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::DiscardRandom:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				ActiveCardCombat->DiscardRandomCards(Effect.Magnitude);
			}
			else if (ActiveEnemyDeck)
			{
				ActiveEnemyDeck->DiscardRandom(Effect.Magnitude);
			}
			break;
		default:
			break;
		}
	}
}

int32 AWorldWalkerGameModeBase::ResolveDamageAgainstEnemy(
	const int32 BaseDamage,
	const bool bIsAttack,
	const bool bConsumeWeak,
	const bool bPiercing)
{
	int32 Damage = FMath::Max(0, BaseDamage);
	if (bIsAttack)
	{
		Damage += PlayerFantasyState.Strength;
		Damage += ActiveCardCombat ? ActiveCardCombat->GetEquipmentAttackBonus() : 0;
		if (bConsumeWeak && PlayerFantasyState.Weak > 0)
		{
			Damage = FMath::FloorToInt(static_cast<float>(Damage) * 0.75f);
			--PlayerFantasyState.Weak;
		}
	}
	if (Damage > 0 && EnemyFantasyState.Exposed > 0)
	{
		Damage = FMath::FloorToInt(static_cast<float>(Damage) * 1.5f);
		--EnemyFantasyState.Exposed;
	}
	return bPiercing ? Damage : EnemyFantasyState.AbsorbDamage(Damage);
}

int32 AWorldWalkerGameModeBase::ResolveDamageAgainstPlayer(
	const int32 BaseDamage,
	const bool bIsAttack,
	const bool bConsumeWeak,
	const bool bPiercing)
{
	int32 Damage = FMath::Max(0, BaseDamage);
	if (bIsAttack)
	{
		Damage += EnemyFantasyState.Strength;
		Damage += ActiveEnemyDeck ? ActiveEnemyDeck->GetAttackBonus() : 0;
		if (bConsumeWeak && EnemyFantasyState.Weak > 0)
		{
			Damage = FMath::FloorToInt(static_cast<float>(Damage) * 0.75f);
			--EnemyFantasyState.Weak;
		}
	}
	if (Damage > 0 && PlayerFantasyState.Exposed > 0)
	{
		Damage = FMath::FloorToInt(static_cast<float>(Damage) * 1.5f);
		--PlayerFantasyState.Exposed;
	}

	const int32 HealthDamage = bPiercing
		? Damage
		: ActiveCardCombat->AbsorbIncomingDamage(Damage);
	return HealthDamage;
}

void AWorldWalkerGameModeBase::ResolveEnemyIntent(const FFantasyEnemyIntentStep& Intent)
{
	const bool bHasAttack = Intent.Effects.ContainsByPredicate([](const FFantasyCombatEffectSpec& Effect)
	{
		return Effect.EffectType == EFantasyCombatEffectType::Damage
			&& Effect.Target == EFantasyCombatTarget::Opponent;
	});
	ActiveEnemy->PlayIntentAnimation(
		bHasAttack ? EWorldWalkerEnemyAnimationCue::Attack : EWorldWalkerEnemyAnimationCue::Empower);

	bool bWeakConsumed = false;
	for (const FFantasyCombatEffectSpec& Effect : Intent.Effects)
	{
		switch (Effect.EffectType)
		{
		case EFantasyCombatEffectType::Damage:
			if (Effect.Target == EFantasyCombatTarget::Opponent)
			{
				const int32 HealthDamage = ResolveDamageAgainstPlayer(
					Effect.Magnitude,
					true,
					!bWeakConsumed,
					Effect.bPiercing);
				ActivePlayer->GetCombatantComponent()->ReceiveDamage(HealthDamage);
				if (HealthDamage > 0)
				{
					ActivePlayer->PlayFantasyHitReactionAnimation();
				}
				bWeakConsumed = true;
			}
			break;
		case EFantasyCombatEffectType::Block:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				EnemyFantasyState.Block += FMath::Max(0, Effect.Magnitude);
			}
			else
			{
				ActiveCardCombat->AddBlock(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::Heal:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				ActiveEnemy->GetCombatantComponent()->RestoreHealth(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::Draw:
			if (ActiveEnemyDeck)
			{
				ActiveEnemyDeck->DrawCards(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::ApplyStatus:
			(Effect.Target == EFantasyCombatTarget::Self ? EnemyFantasyState : PlayerFantasyState)
				.AddStatus(Effect.Status, Effect.Magnitude);
			break;
		case EFantasyCombatEffectType::RemoveStatus:
			(Effect.Target == EFantasyCombatTarget::Self ? EnemyFantasyState : PlayerFantasyState)
				.RemoveStatus(Effect.Status, Effect.Magnitude);
			break;
		case EFantasyCombatEffectType::GainValor:
			if (Effect.Target == EFantasyCombatTarget::Opponent)
			{
				ActiveCardCombat->AddValor(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::GainAction:
			if (Effect.Target == EFantasyCombatTarget::Self && ActiveEnemyDeck)
			{
				ActiveEnemyDeck->AddAction(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::GainMana:
			if (Effect.Target == EFantasyCombatTarget::Self && ActiveEnemyDeck)
			{
				ActiveEnemyDeck->AddMana(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::DiscardRandom:
			if (Effect.Target == EFantasyCombatTarget::Self && ActiveEnemyDeck)
			{
				ActiveEnemyDeck->DiscardRandom(Effect.Magnitude);
			}
			else
			{
				ActiveCardCombat->DiscardRandomCards(Effect.Magnitude);
			}
			break;
		default:
			break;
		}
	}
}

void AWorldWalkerGameModeBase::ResolveEnemyCardEffects(UCardDefinition* Card)
{
	if (!Card || !ActivePlayer || !ActiveEnemy || !ActiveCardCombat || !ActiveEnemyDeck)
	{
		return;
	}

	const bool bIsAttackCard = Card->CardType == ECardType::Attack;
	const bool bHasOpponentDamage = Card->Effects.ContainsByPredicate([](const FFantasyCombatEffectSpec& Effect)
	{
		return Effect.EffectType == EFantasyCombatEffectType::Damage
			&& Effect.Target == EFantasyCombatTarget::Opponent;
	});
	ActiveEnemy->PlayIntentAnimation(
		bHasOpponentDamage ? EWorldWalkerEnemyAnimationCue::Attack : EWorldWalkerEnemyAnimationCue::Empower);

	bool bWeakConsumed = false;
	for (const FFantasyCombatEffectSpec& Effect : Card->Effects)
	{
		switch (Effect.EffectType)
		{
		case EFantasyCombatEffectType::Damage:
			if (Effect.Target == EFantasyCombatTarget::Opponent)
			{
				const int32 HealthDamage = ResolveDamageAgainstPlayer(
					Effect.Magnitude,
					bIsAttackCard,
					bIsAttackCard && !bWeakConsumed,
					Effect.bPiercing);
				ActivePlayer->GetCombatantComponent()->ReceiveDamage(HealthDamage);
				if (HealthDamage > 0)
				{
					ActivePlayer->PlayFantasyHitReactionAnimation();
				}
				bWeakConsumed |= bIsAttackCard;
			}
			else
			{
				const int32 SelfDamage = EnemyFantasyState.AbsorbDamage(Effect.Magnitude);
				ActiveEnemy->GetCombatantComponent()->ReceiveDamage(SelfDamage);
			}
			break;
		case EFantasyCombatEffectType::Block:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				EnemyFantasyState.Block += FMath::Max(0, Effect.Magnitude);
			}
			else
			{
				ActiveCardCombat->AddBlock(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::Heal:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				ActiveEnemy->GetCombatantComponent()->RestoreHealth(Effect.Magnitude);
			}
			else
			{
				ActivePlayer->GetCombatantComponent()->RestoreHealth(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::Draw:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				ActiveEnemyDeck->DrawCards(Effect.Magnitude);
			}
			else
			{
				ActiveCardCombat->DrawCards(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::ApplyStatus:
			(Effect.Target == EFantasyCombatTarget::Self ? EnemyFantasyState : PlayerFantasyState)
				.AddStatus(Effect.Status, Effect.Magnitude);
			break;
		case EFantasyCombatEffectType::RemoveStatus:
			(Effect.Target == EFantasyCombatTarget::Self ? EnemyFantasyState : PlayerFantasyState)
				.RemoveStatus(Effect.Status, Effect.Magnitude);
			break;
		case EFantasyCombatEffectType::GainValor:
			if (Effect.Target == EFantasyCombatTarget::Opponent)
			{
				ActiveCardCombat->AddValor(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::GainAction:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				ActiveEnemyDeck->AddAction(Effect.Magnitude);
			}
			else
			{
				ActiveCardCombat->AddActionPoints(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::GainMana:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				ActiveEnemyDeck->AddMana(Effect.Magnitude);
			}
			else
			{
				ActiveCardCombat->AddMana(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::DiscardRandom:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				ActiveEnemyDeck->DiscardRandom(Effect.Magnitude);
			}
			else
			{
				ActiveCardCombat->DiscardRandomCards(Effect.Magnitude);
			}
			break;
		default:
			break;
		}
	}
}

void AWorldWalkerGameModeBase::ResolveEndOfTurnPoison(const bool bPlayerTurnEnded)
{
	FFantasyCombatRuntimeState& State = bPlayerTurnEnded
		? PlayerFantasyState
		: EnemyFantasyState;
	if (State.Poison <= 0)
	{
		return;
	}

	const int32 PoisonDamage = State.Poison;
	State.Poison = FMath::Max(0, State.Poison - 1);
	if (bPlayerTurnEnded && ActivePlayer)
	{
		ActivePlayer->GetCombatantComponent()->ReceiveDamage(PoisonDamage);
		ActivePlayer->PlayFantasyHitReactionAnimation();
	}
	else if (ActiveEnemy)
	{
		ActiveEnemy->GetCombatantComponent()->ReceiveDamage(PoisonDamage);
		ActiveEnemy->PlayIntentAnimation(EWorldWalkerEnemyAnimationCue::HitReact);
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_POISON_TICK Target=%s Damage=%d Remaining=%d"),
		bPlayerTurnEnded ? TEXT("Player") : TEXT("Enemy"),
		PoisonDamage,
		State.Poison);
}

void AWorldWalkerGameModeBase::ApplyEnemyTurnStartEquipment()
{
	if (!ActiveEnemyDeck)
	{
		return;
	}
	for (const UCardDefinition* Equipment : ActiveEnemyDeck->GetEquipmentZone())
	{
		if (Equipment)
		{
			EnemyFantasyState.Block += FMath::Max(0, Equipment->EquipmentTurnStartBlock);
		}
	}
}

bool AWorldWalkerGameModeBase::TryTriggerEnemyDefeatPassive()
{
	if (!ActiveEnemy || !ActiveFantasyEnemyDefinition || bEnemyDefeatPassiveConsumed
		|| (ActiveFantasyEnemyDefinition->PassiveId != TEXT("RevivalOath")
			&& ActiveFantasyEnemyDefinition->PassiveId != TEXT("AshenRevival")))
	{
		return false;
	}

	bEnemyDefeatPassiveConsumed = true;
	UCombatantComponent* EnemyCombatant = ActiveEnemy->GetCombatantComponent();
	EnemyCombatant->ResetHealth();
	EnemyCombatant->ReceiveDamage(EnemyCombatant->GetMaxHealth() / 2);
	EnemyFantasyState.Block += 10;
	EnemyFantasyState.Strength += 2;
	ActiveEnemy->PlayIntentAnimation(EWorldWalkerEnemyAnimationCue::Empower);
	RefreshCombatUI();
	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		Controller->SetCombatMessage(
			TEXT("无头骑士的【执念】触发：以半数生命复生，获得 10 格挡与 2 力量。"),
			FantasyRunFlowState == EFantasyRunFlowState::PlayerTurn);
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_ENEMY_PASSIVE_TRIGGERED Enemy=%s Passive=%s"),
		*ActiveFantasyEnemyDefinition->EnemyId.ToString(),
		*ActiveFantasyEnemyDefinition->PassiveId.ToString());
	return true;
}

void AWorldWalkerGameModeBase::RestoreRunHealthToPlayer()
{
	UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	if (!Progression || !ActivePlayer)
	{
		return;
	}

	UCombatantComponent* PlayerCombatant = ActivePlayer->GetCombatantComponent();
	PlayerCombatant->ConfigureMaxHealth(Progression->GetRunMaxHealth());
	PlayerCombatant->ReceiveDamage(
		Progression->GetRunMaxHealth() - Progression->GetCurrentRunHealth());
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_RUN_HEALTH_RESTORED Health=%d/%d"),
		PlayerCombatant->GetCurrentHealth(),
		PlayerCombatant->GetMaxHealth());
}

void AWorldWalkerGameModeBase::SyncRunHealthFromPlayer()
{
	UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	if (!Progression || !ActivePlayer)
	{
		return;
	}

	const UCombatantComponent* PlayerCombatant = ActivePlayer->GetCombatantComponent();
	Progression->RecordRunHealth(
		PlayerCombatant->GetCurrentHealth(),
		PlayerCombatant->GetMaxHealth());
}

FString AWorldWalkerGameModeBase::BuildNextIntentText() const
{
	if (ActiveEnemyDeck && ActiveEnemyDeck->IsInitialized())
	{
		const FString PassiveLine = ActiveFantasyEnemyDefinition
			&& !ActiveFantasyEnemyDefinition->PassiveName.IsEmpty()
			? FString::Printf(
				TEXT("被动【%s】：%s\n"),
				*ActiveFantasyEnemyDefinition->PassiveName.ToString(),
				*ActiveFantasyEnemyDefinition->PassiveDescription.ToString())
			: FString();
		return FString::Printf(
			TEXT("%s敌方牌区：抽牌 %d｜手牌 %d｜弃牌 %d｜装备 %d｜反制 %d｜行动力 %d｜法力 %d\n%s"),
			*PassiveLine,
			ActiveEnemyDeck->GetDrawPileCount(),
			ActiveEnemyDeck->GetHandCount(),
			ActiveEnemyDeck->GetDiscardPileCount(),
			ActiveEnemyDeck->GetEquipmentCount(),
			ActiveEnemyDeck->GetCounterCount(),
			ActiveEnemyDeck->GetCurrentActionPoints(),
			ActiveEnemyDeck->GetCurrentMana(),
			*ActiveEnemyDeck->BuildPreview(EnemyFantasyState.Strength));
	}
	if (!ActiveFantasyEnemyDefinition
		|| !ActiveFantasyEnemyDefinition->IntentCycle.IsValidIndex(CurrentEnemyIntentIndex))
	{
		return TEXT("敌人意图数据不可用");
	}

	return ActiveFantasyEnemyDefinition->IntentCycle[CurrentEnemyIntentIndex]
		.BuildPreviewText(EnemyFantasyState.Strength);
}

void AWorldWalkerGameModeBase::FinishCombat(const bool bPlayerWon)
{
	bCombatActive = false;
	bWaitingForEnemy = false;
	FantasyRunFlowState = bPlayerWon
		? EFantasyRunFlowState::RewardChoice
		: EFantasyRunFlowState::Defeat;
	GetWorldTimerManager().ClearTimer(EnemyTurnTimer);
	SyncRunHealthFromPlayer();
	RefreshCombatUI();

	if (bPlayerWon && ActiveEnemy)
	{
		ActiveEnemy->PlayIntentAnimation(EWorldWalkerEnemyAnimationCue::Death);
		ActiveEnemy->SetActorEnableCollision(false);
	}

	if (bPlayerWon)
	{
		BeginVictoryReward();
	}
	else if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		Controller->ShowCombatResult(false);
	}
}

void AWorldWalkerGameModeBase::BeginVictoryReward()
{
	PendingRewardChoices.Reset();
	if (ActiveCardCombat)
	{
		for (UCardDefinition* Card : ActiveCardCombat->BuildRewardChoices(3))
		{
			PendingRewardChoices.Add(Card);
		}
	}
	bAwaitingRewardSelection = PendingRewardChoices.Num() == 3;
	bRewardReadyToLeave = false;
	FantasyRunFlowState = EFantasyRunFlowState::RewardChoice;

	AWorldWalkerPlayerController* Controller = GetWorldWalkerController();
	if (!Controller || !bAwaitingRewardSelection)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("W01 victory reward unavailable. Expected=3 Actual=%d"),
			PendingRewardChoices.Num());
		bRewardReadyToLeave = Controller != nullptr;
		if (Controller)
		{
			FantasyRunFlowState = EFantasyRunFlowState::RewardConfirmed;
			Controller->ShowRewardConfirmation(
				TEXT("战利品资料尚未生成，本次不发放卡牌。你仍可安全返回探索。"));
		}
		return;
	}

	TArray<FString> RewardLabels;
	TArray<UTexture2D*> RewardArtworks;
	TArray<FLinearColor> RewardSchoolTints;
	for (const UCardDefinition* Card : PendingRewardChoices)
	{
		RewardLabels.Add(Card ? Card->BuildRulesText() : TEXT("缺失的战利品"));
		RewardArtworks.Add(Card ? Card->Artwork.LoadSynchronous() : nullptr);
		RewardSchoolTints.Add(Card
			? GetCardSchoolTint(Card->School)
			: FLinearColor(0.20f, 0.16f, 0.12f, 1.0f));
	}
	Controller->ShowRewardSelection(RewardLabels, RewardArtworks, RewardSchoolTints);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_VICTORY_REWARD_READY Choices=%s|%s|%s"),
		*PendingRewardChoices[0]->CardId.ToString(),
		*PendingRewardChoices[1]->CardId.ToString(),
		*PendingRewardChoices[2]->CardId.ToString());
}

void AWorldWalkerGameModeBase::HandleRewardSelection(const int32 RewardIndex)
{
	if (FantasyRunFlowState != EFantasyRunFlowState::RewardChoice
		|| !bAwaitingRewardSelection || !PendingRewardChoices.IsValidIndex(RewardIndex)
		|| !ActiveCardCombat)
	{
		return;
	}

	UCardDefinition* RewardCard = PendingRewardChoices[RewardIndex];
	if (!ActiveCardCombat->GrantRewardCard(RewardCard))
	{
		if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
		{
			Controller->SetCombatMessage(TEXT("战利品未能写入牌组，请重新选择。"), true);
		}
		return;
	}

	bAwaitingRewardSelection = false;
	bRewardReadyToLeave = true;
	FantasyRunFlowState = EFantasyRunFlowState::RewardConfirmed;
	PendingRewardChoices.Reset();
	const FString Confirmation = FString::Printf(
		TEXT("已收下【%s】并加入本次旅途牌组。当前牌组 %d 张，其中战利品 %d 张。"),
		*RewardCard->DisplayName.ToString(),
		ActiveCardCombat->GetStartingDeckCount(),
		ActiveCardCombat->GetRunRewardCount());
	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		Controller->ShowRewardConfirmation(Confirmation);
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_VICTORY_REWARD_CLAIMED Card=%s Deck=%d RunRewards=%d"),
		*RewardCard->CardId.ToString(),
		ActiveCardCombat->GetStartingDeckCount(),
		ActiveCardCombat->GetRunRewardCount());
}

void AWorldWalkerGameModeBase::HandleReturnToExploration()
{
	if (!bRewardReadyToLeave || FantasyRunFlowState != EFantasyRunFlowState::RewardConfirmed
		|| !ActivePlayer)
	{
		return;
	}

	const FString DefeatedEnemyName = ActiveFantasyEnemyDefinition
		? ActiveFantasyEnemyDefinition->DisplayName.ToString()
		: TEXT("对手");
	const int32 RewardCount = ActiveCardCombat ? ActiveCardCombat->GetRunRewardCount() : 0;
	bRewardReadyToLeave = false;
	ClearActiveEnemy();
	CompleteActiveNode(FString::Printf(
		TEXT("你击败了【%s】，战利品已加入牌组。旅途中累计获得 %d 张卡。"),
		*DefeatedEnemyName,
		RewardCount));
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_REWARD_RETURNED_TO_EXPLORATION Enemy=%s Rewards=%d"),
		*DefeatedEnemyName,
		RewardCount);
}

void AWorldWalkerGameModeBase::RefreshCombatUI() const
{
	if (!ActivePlayer || !ActiveEnemy || !ActiveCardCombat)
	{
		return;
	}

	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		const UCombatantComponent* PlayerCombatant = ActivePlayer->GetCombatantComponent();
		const UCombatantComponent* EnemyCombatant = ActiveEnemy->GetCombatantComponent();
		TArray<FString> CardLabels;
		TArray<bool> CardPlayable;
		TArray<UTexture2D*> CardArtworks;
		TArray<FLinearColor> CardSchoolTints;
		for (const UCardDefinition* Card : ActiveCardCombat->GetHand())
		{
			CardLabels.Add(Card ? Card->BuildRulesText() : TEXT("缺失卡牌"));
			CardPlayable.Add(Card && ActiveCardCombat->IsCardPlayable(Card));
			CardArtworks.Add(Card ? Card->Artwork.LoadSynchronous() : nullptr);
			if (!Card)
			{
				CardSchoolTints.Add(FLinearColor(0.20f, 0.16f, 0.12f, 1.0f));
				continue;
			}

			CardSchoolTints.Add(GetCardSchoolTint(Card->School));
		}

		Controller->RefreshCombat(
			PlayerCombatant->GetCurrentHealth(),
			PlayerCombatant->GetMaxHealth(),
			EnemyCombatant->GetCurrentHealth(),
			EnemyCombatant->GetMaxHealth(),
			ActiveFantasyEnemyDefinition
				? ActiveFantasyEnemyDefinition->DisplayName.ToString()
				: TEXT("未知对手"),
			ActiveCardCombat->GetCurrentActionPoints(),
			ActiveCardCombat->GetMaxActionPoints(),
			ActiveCardCombat->GetCurrentMana(),
			ActiveCardCombat->GetEquipmentCount(),
			ActiveCardCombat->GetCurrentBlock(),
			ActiveCardCombat->GetDrawPileCount(),
			ActiveCardCombat->GetDiscardPileCount(),
			ActiveCardCombat->GetExhaustPileCount(),
			FString::Printf(
				TEXT("旅途牌组 %d 张｜本次获得 %d 张｜移除 %d 张"),
				ActiveCardCombat->GetStartingDeckCount(),
				ActiveCardCombat->GetRunRewardCount(),
				ActiveCardCombat->GetRunRemovedCount()),
			PlayerFantasyState.BuildSummary(),
			EnemyFantasyState.BuildSummary(),
			BuildNextIntentText(),
			CardLabels,
			CardPlayable,
			CardArtworks,
			CardSchoolTints);
	}
}

AWorldWalkerPlayerController* AWorldWalkerGameModeBase::GetWorldWalkerController() const
{
	return Cast<AWorldWalkerPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
}

void AWorldWalkerGameModeBase::RestartDemo()
{
	if (UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr)
	{
		Progression->ResetRun();
	}
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	UGameplayStatics::OpenLevel(this, FName(*LevelName));
}
