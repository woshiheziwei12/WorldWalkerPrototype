#include "WorldWalkerGameModeBase.h"

#include "Characters/WorldWalkerCharacter.h"
#include "Characters/WorldWalkerEnemy.h"
#include "Cards/CardCombatComponent.h"
#include "Cards/CardDefinition.h"
#include "Cards/Fantasy/FantasyBlessingDefinition.h"
#include "Cards/Fantasy/FantasyCardProgressionSubsystem.h"
#include "Cards/Fantasy/FantasyEnemyDeckRuntime.h"
#include "Cards/Fantasy/FantasyEnemyDefinition.h"
#include "CollisionQueryParams.h"
#include "Combat/CombatantComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "World/WorldDefinition.h"
#include "World/Fantasy/FantasyBattleArena.h"
#include "World/Fantasy/FantasyAmbientSoundscape.h"
#include "World/Fantasy/FantasyWorldLayout.h"
#include "World/Platforming/SpiralTowerWorldLayout.h"
#include "World/Fantasy/Exploration/FantasyWorldChoiceActor.h"
#include "World/WorldHubLayout.h"
#include "World/WorldPortal.h"
#include "World/WorldTravelSubsystem.h"
#include "WorldWalkerPlayerController.h"

namespace
{
	bool GM0AutomationInjectedDefeat = false;
	bool GM0AutomationCompletedRun = false;

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

	EFantasyAudioCue GetCardAudioCue(const UCardDefinition* Card)
	{
		if (!Card)
		{
			return EFantasyAudioCue::Defense;
		}
		switch (Card->CardType)
		{
		case ECardType::Attack: return EFantasyAudioCue::Attack;
		case ECardType::Spell: return EFantasyAudioCue::Spell;
		case ECardType::Equipment: return EFantasyAudioCue::Equipment;
		case ECardType::Counter: return EFantasyAudioCue::Counter;
		case ECardType::Action:
		default: return EFantasyAudioCue::Defense;
		}
	}

	int32 GetCardShopPrice(const UCardDefinition* Card)
	{
		if (!Card) return 0;
		switch (Card->Rarity)
		{
		case EFantasyCardRarity::Rare: return 85;
		case EFantasyCardRarity::Uncommon: return 65;
		case EFantasyCardRarity::Common:
		default: return 45;
		}
	}
}

AWorldWalkerGameModeBase::AWorldWalkerGameModeBase()
{
	DefaultPawnClass = AWorldWalkerCharacter::StaticClass();
	PlayerControllerClass = AWorldWalkerPlayerController::StaticClass();
	bM0RunAutomationEnabled = FParse::Param(
		FCommandLine::Get(),
		TEXT("W01M0RunAutomation"));
	FString AutomationProfession;
	if (FParse::Value(
		FCommandLine::Get(),
		TEXT("W01M0Profession="),
		AutomationProfession)
		&& AutomationProfession.Equals(TEXT("Knight"), ESearchCase::IgnoreCase))
	{
		M0AutomationProfession = EFantasyPlayerProfession::Knight;
	}
	CombatTiming = FFantasyCombatTiming::ForAutomationMode(
		FParse::Param(FCommandLine::Get(), TEXT("W01FastCombat")));
}

void AWorldWalkerGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	ActivePlayer = Cast<AWorldWalkerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	InitializeWorldContent();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_COMBAT_TIMING_MODE Fast=%d TurnStartDelay=%.2f CardDelay=%.2f"),
		CombatTiming.bFastCombat ? 1 : 0,
		CombatTiming.EnemyTurnStartDelay,
		CombatTiming.EnemyCardPresentationDelay);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("WorldWalker world ready. World=%s Player=%s Enemy=%s Portal=%s Hub=%s FantasyWorld=%s SpiralTower=%s"),
		CurrentWorldDefinition ? *CurrentWorldDefinition->WorldId.ToString() : TEXT("Unregistered"),
		ActivePlayer ? TEXT("spawned") : TEXT("missing"),
		ActiveEnemy ? TEXT("spawned") : TEXT("none"),
		ActivePortal ? TEXT("spawned") : TEXT("none"),
		ActiveHub ? TEXT("spawned") : TEXT("none"),
		ActiveFantasyWorld ? TEXT("spawned") : TEXT("none"),
		ActiveSpiralTowerWorld ? TEXT("spawned") : TEXT("none"));

	if (bM0RunAutomationEnabled)
	{
		GetWorldTimerManager().SetTimerForNextTick(
			this,
			&AWorldWalkerGameModeBase::InitializeM0RunAutomation);
	}
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
		ActiveCardCombat = ActivePlayer->GetCardCombatComponent();
		const bool bDeckReady = ActiveCardCombat && ActiveCardCombat->LoadStartingDeck();
		const bool bEnemyReady = LoadFantasyEnemyDefinition(TEXT("DrowsyBat"));
		ExplorationMessage = bDeckReady && bEnemyReady
			? TEXT("UNREGISTERED TEST MAP\nApproach the red enemy and press E")
			: TEXT("UNREGISTERED TEST MAP\nTest combat data is unavailable; run W01 content setup");
		SpawnTestEnemy();
		UE_LOG(
			LogTemp,
			Display,
			TEXT("W01_TEST_COMBAT_READY Deck=%d EnemyDefinition=%d"),
			bDeckReady ? 1 : 0,
			bEnemyReady ? 1 : 0);
		return;
	}

	SpawnRegisteredWorldRoot();

	if (CurrentWorldDefinition->bIsMainWorld)
	{
		ExplorationMessage = TEXT("MAIN WORLD - IMMORTAL HUB\nWalk into the vortex portal or approach another world gate");
		SpawnMainWorldHub(TravelSubsystem->GetTravelDestinations());
		return;
	}

	if (CurrentWorldDefinition->WorldId == UWorldTravelSubsystem::EasternHorrorWorldId)
	{
		ExplorationMessage = TEXT("灰烬王国 · 月圆旅途\n选择路线→挑战/事件→战后三选一→章节守关者");
		ActivePlayer->ConfigureFantasyWorldForm(true, true);
		ActiveCardCombat = ActivePlayer->GetCardCombatComponent();
		SpawnFantasyWorldLayout();
		SpawnFantasyBattleArena();
		const FVector GroundOrigin = ActivePlayer->GetActorLocation() - FVector(0.0f, 0.0f, 88.0f);
		const FVector PortalOffset = ActiveFantasyWorld
			? ActiveFantasyWorld->GetReturnPortalLocation() - GroundOrigin
			: ActivePlayer->GetActorRightVector().GetSafeNormal2D() * 650.0f;
		SpawnPortal(
			TravelSubsystem->GetMainWorldDefinition(),
			PortalOffset);
		GetWorldTimerManager().SetTimerForNextTick(
			this,
			&AWorldWalkerGameModeBase::InitializeFantasyRun);
		return;
	}

	if (CurrentWorldDefinition->WorldId == UWorldTravelSubsystem::SpiralTowerWorldId)
	{
		ExplorationMessage = TEXT("巴比伦阶梯神塔 · 外墙攀登\nWASD 移动 | Shift 冲刺 | Space 跳跃 | 拾取攀岩手甲后：空中朝向边缘按住 Space 翻上 | 登顶选择归途");
		if (!ActivePlayer->ConfigureSpiralTowerAnimeForm())
		{
			ActivePlayer->ConfigureMainWorldAnimeForm();
		}
		SpawnSpiralTowerWorldLayout();
		return;
	}
}

void AWorldWalkerGameModeBase::SpawnRegisteredWorldRoot()
{
	if (!CurrentWorldDefinition || CurrentWorldDefinition->WorldRootActorClass.IsNull() || !GetWorld())
	{
		return;
	}

	UClass* RootClass = CurrentWorldDefinition->WorldRootActorClass.LoadSynchronous();
	if (!RootClass || !RootClass->IsChildOf(AActor::StaticClass()))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Registered world '%s' has an invalid WorldRootActorClass."),
			*CurrentWorldDefinition->WorldId.ToString());
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActiveWorldRoot = GetWorld()->SpawnActor<AActor>(RootClass, FTransform::Identity, SpawnParameters);
	if (ActiveWorldRoot)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("Registered world root spawned. World=%s Class=%s"),
			*CurrentWorldDefinition->WorldId.ToString(),
			*GetNameSafe(RootClass));
	}
	else
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Registered world root failed to spawn. World=%s Class=%s"),
			*CurrentWorldDefinition->WorldId.ToString(),
			*GetNameSafe(RootClass));
	}
}

void AWorldWalkerGameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(EnemyTurnTimer);
	GetWorldTimerManager().ClearTimer(M0RunAutomationTimer);
	DestroyWorldChoices();
	Super::EndPlay(EndPlayReason);
}

void AWorldWalkerGameModeBase::InitializeM0RunAutomation()
{
	if (!bM0RunAutomationEnabled || !CurrentWorldDefinition)
	{
		FailM0RunAutomation(TEXT("current world definition is unavailable"));
		return;
	}

	if (CurrentWorldDefinition->bIsMainWorld)
	{
		if (!GM0AutomationCompletedRun)
		{
			FailM0RunAutomation(TEXT("entered W00 before completing the W01 chapter"));
			return;
		}
		UE_LOG(
			LogTemp,
			Display,
			TEXT("W01_M0_AUTOMATION_WORLD_RETURNED World=W00_MainWorld Result=Success"));
		FPlatformMisc::RequestExitWithStatus(false, 0);
		return;
	}

	if (CurrentWorldDefinition->WorldId != UWorldTravelSubsystem::EasternHorrorWorldId)
	{
		FailM0RunAutomation(FString::Printf(
			TEXT("unsupported world %s"),
			*CurrentWorldDefinition->WorldId.ToString()));
		return;
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_M0_AUTOMATION_STARTED Profession=%s RetryPhase=%d"),
		M0AutomationProfession == EFantasyPlayerProfession::Knight ? TEXT("Knight") : TEXT("Mage"),
		GM0AutomationInjectedDefeat ? 1 : 0);
	GetWorldTimerManager().SetTimer(
		M0RunAutomationTimer,
		this,
		&AWorldWalkerGameModeBase::HandleM0RunAutomationStep,
		0.01f,
		true);
}

void AWorldWalkerGameModeBase::HandleM0RunAutomationStep()
{
	if (++M0AutomationStepCount > 30000)
	{
		FailM0RunAutomation(TEXT("step limit exceeded"));
		return;
	}

	UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	if (!Progression || !ActivePlayer)
	{
		return;
	}

	switch (FantasyRunFlowState)
	{
	case EFantasyRunFlowState::ProfessionChoice:
		HandleProfessionSelection(
			M0AutomationProfession == EFantasyPlayerProfession::Mage ? 0 : 1);
		return;

	case EFantasyRunFlowState::RouteChoice:
		if (M0AutomationDeckViewPhase == 0)
		{
			HandleDeckViewToggle();
			M0AutomationDeckViewPhase = 1;
			return;
		}
		if (M0AutomationDeckViewPhase == 1)
		{
			HandleDeckViewToggle();
			M0AutomationDeckViewPhase = 2;
			return;
		}
		// Honor the M2 chapter quota: fight at depths 0 and 2, take safe non-combat
		// branches at 1/3/4, then fight the boss at depth 5.
		for (int32 ChoiceIndex = 0; ChoiceIndex < Progression->GetRouteChoices().Num(); ++ChoiceIndex)
		{
			const bool bNeedsCombat = Progression->GetChapterDepth() == 0
				|| Progression->GetChapterDepth() == 2
				|| Progression->GetChapterDepth() >= 5;
			if (Progression->GetRouteChoices()[ChoiceIndex].IsCombat() == bNeedsCombat)
			{
				HandleRouteSelection(ChoiceIndex);
				return;
			}
		}
		FailM0RunAutomation(TEXT("no route choice matched the automation policy"));
		return;

	case EFantasyRunFlowState::EventChoice:
		if (CurrentEventId == TEXT("WanderingMerchant"))
		{
			for (int32 Index = 0; Index < PendingShopCards.Num(); ++Index)
			{
				if (PendingShopCards[Index]
					&& Progression->GetGold() >= GetCardShopPrice(PendingShopCards[Index]))
				{
					HandleEventSelection(Index);
					return;
				}
			}
			HandleEventSelection(PendingEventChoiceCount - 1);
		}
		else
		{
			// Choice 0 restores health, takes treasure gold, or is the safest branch.
			HandleEventSelection(0);
		}
		return;

	case EFantasyRunFlowState::Exploration:
		if (Progression->HasActiveNode() && Progression->GetActiveNode().IsCombat()
			&& ActiveEnemy && !bCombatActive)
		{
			StartCombat(ActivePlayer, ActiveEnemy);
		}
		return;

	case EFantasyRunFlowState::PlayerTurn:
		if (!GM0AutomationInjectedDefeat)
		{
			GM0AutomationInjectedDefeat = true;
			bM0AutomationRestartPending = true;
			ActivePlayer->GetCombatantComponent()->ReceiveDamage(
				ActivePlayer->GetCombatantComponent()->GetMaxHealth() + 1000);
			UE_LOG(
				LogTemp,
				Display,
				TEXT("W01_M0_AUTOMATION_DEFEAT_INJECTED Result=Defeat"));
			FinishCombat(false);
			return;
		}
		if (!ActiveCardCombat)
		{
			FailM0RunAutomation(TEXT("player card component disappeared during combat"));
			return;
		}
		for (int32 HandIndex = 0; HandIndex < ActiveCardCombat->GetHand().Num(); ++HandIndex)
		{
			if (ActiveCardCombat->IsCardPlayable(HandIndex))
			{
				HandlePlayCard(HandIndex);
				return;
			}
		}
		HandleEndPlayerTurn();
		return;

	case EFantasyRunFlowState::EnemyTurn:
		return;

	case EFantasyRunFlowState::RewardChoice:
		if (M0AutomationRewardCount++ == 0)
		{
			HandleRewardSelection(0);
		}
		else
		{
			HandleRewardSkip();
		}
		return;

	case EFantasyRunFlowState::RewardConfirmed:
		HandleReturnToExploration();
		return;

	case EFantasyRunFlowState::Resolution:
		HandleNodeResolutionContinue();
		return;

	case EFantasyRunFlowState::Defeat:
		if (!bM0AutomationRestartPending)
		{
			FailM0RunAutomation(TEXT("automatic player lost after the injected retry"));
			return;
		}
		bM0AutomationRestartPending = false;
		UE_LOG(LogTemp, Display, TEXT("W01_M0_AUTOMATION_RETRY_REQUESTED"));
		RestartDemo();
		return;

	case EFantasyRunFlowState::ChapterComplete:
		GM0AutomationCompletedRun = true;
		GetWorldTimerManager().ClearTimer(M0RunAutomationTimer);
		UE_LOG(
			LogTemp,
			Display,
			TEXT("W01_M0_AUTOMATION_CHAPTER_COMPLETE Profession=%s Depth=%d Rewards=%d Removed=%d"),
			*Progression->GetProfessionDisplayName(),
			Progression->GetChapterDepth(),
			Progression->GetTotalGrantedCopies(),
			Progression->GetTotalRemovedCopies());
		HandleNodeResolutionContinue();
		if (UWorldTravelSubsystem* TravelSubsystem = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UWorldTravelSubsystem>()
			: nullptr)
		{
			if (TravelSubsystem->TravelToWorld(TravelSubsystem->GetMainWorldDefinition()))
			{
				return;
			}
		}
		FailM0RunAutomation(TEXT("return travel to W00 was rejected"));
		return;
	}
}

void AWorldWalkerGameModeBase::FailM0RunAutomation(const FString& Reason)
{
	GetWorldTimerManager().ClearTimer(M0RunAutomationTimer);
	UE_LOG(
		LogTemp,
		Error,
		TEXT("W01_M0_AUTOMATION_FAILED Reason=%s Flow=%d Steps=%d"),
		*Reason,
		static_cast<int32>(FantasyRunFlowState),
		M0AutomationStepCount);
	FPlatformMisc::RequestExitWithStatus(true, 1);
}

void AWorldWalkerGameModeBase::SpawnMainWorldHub(const TArray<UWorldDefinition*>& DestinationWorlds)
{
	if (!ActivePlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot spawn main-world hub: player is missing."));
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
		ActiveHub->ConfigureDestination(DestinationWorlds.IsEmpty() ? nullptr : DestinationWorlds[0]);

		// The authored vortex features the first registered destination. Additional
		// worlds receive ordinary data-driven portals arranged beside it. Adding a
		// definition asset is therefore sufficient to expose a new world in W00.
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal2D();
		for (int32 DestinationIndex = 1; DestinationIndex < DestinationWorlds.Num(); ++DestinationIndex)
		{
			const int32 SideIndex = (DestinationIndex + 1) / 2;
			const float Side = DestinationIndex % 2 == 1 ? 1.0f : -1.0f;
			const UWorldDefinition* Destination = DestinationWorlds[DestinationIndex];
			const FVector PortalLocation = Destination
				&& Destination->WorldId == UWorldTravelSubsystem::SpiralTowerWorldId
				? ActiveHub->GetSpiralTowerPortalLocation()
				: ActiveHub->GetActivePortalLocation() + Right * Side * SideIndex * 430.0f;
			SpawnPortal(DestinationWorlds[DestinationIndex], PortalLocation - GroundOrigin);
		}
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

void AWorldWalkerGameModeBase::SpawnSpiralTowerWorldLayout()
{
	if (!ActivePlayer || !GetWorld())
	{
		return;
	}

	if (ASpiralTowerWorldLayout* RegisteredTower = Cast<ASpiralTowerWorldLayout>(ActiveWorldRoot))
	{
		ActiveSpiralTowerWorld = RegisteredTower;
		return;
	}

	const FVector Forward = ActivePlayer->GetActorForwardVector().GetSafeNormal2D();
	const FVector GroundOrigin = ActivePlayer->GetActorLocation() - FVector(0.0f, 0.0f, 88.0f);
	const FRotator WorldRotation(0.0f, Forward.Rotation().Yaw, 0.0f);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActiveSpiralTowerWorld = GetWorld()->SpawnActor<ASpiralTowerWorldLayout>(
		ASpiralTowerWorldLayout::StaticClass(),
		GroundOrigin,
		WorldRotation,
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

	UFantasyCardProgressionSubsystem* Progression =
		GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>();
	if (!Progression)
	{
		return;
	}
	if (!Progression->HasSelectedProfession())
	{
		PresentProfessionChoices();
		return;
	}
	BeginSelectedProfessionRun();
}

void AWorldWalkerGameModeBase::PresentProfessionChoices()
{
	FantasyRunFlowState = EFantasyRunFlowState::ProfessionChoice;
	if (ActivePlayer)
	{
		ActivePlayer->SetCombatLocked(true);
	}
	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		Controller->SetDeckAccessEnabled(false);
		Controller->ShowProfessionSelection({
			TEXT("法师（小女巫）\n法力牌积累资源，以火焰、寒冰和毒系咒术终结战斗"),
			TEXT("女骑士\n免费攻击、行动牌与装备构成稳定攻防")});
	}
	UE_LOG(LogTemp, Display, TEXT("W01_PROFESSION_CHOICES_READY Count=2"));
}

void AWorldWalkerGameModeBase::HandleProfessionSelection(const int32 ChoiceIndex)
{
	if (FantasyRunFlowState != EFantasyRunFlowState::ProfessionChoice || !GetGameInstance())
	{
		return;
	}
	const EFantasyPlayerProfession Profession = ChoiceIndex == 0
		? EFantasyPlayerProfession::Mage
		: ChoiceIndex == 1
			? EFantasyPlayerProfession::Knight
			: EFantasyPlayerProfession::None;
	UFantasyCardProgressionSubsystem* Progression =
		GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>();
	if (!Progression || !Progression->SelectProfession(Profession))
	{
		PresentProfessionChoices();
		return;
	}
	if (ActivePlayer)
	{
		ActivePlayer->ConfigureFantasyProfession(Profession);
	}
	BeginSelectedProfessionRun();
}

void AWorldWalkerGameModeBase::BeginSelectedProfessionRun()
{
	UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	if (!Progression || !Progression->HasSelectedProfession() || !ActiveCardCombat)
	{
		return;
	}

	Progression->EnsureRunStarted();
	ActivePlayer->ConfigureFantasyProfession(Progression->GetSelectedProfession());
	if (!ActiveCardCombat->LoadStartingDeck())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("W01 profession starter deck unavailable. Profession=%s"),
			*Progression->GetProfessionDisplayName());
		Progression->ResetRun();
		PresentProfessionChoices();
		return;
	}
	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		Controller->SetProfessionLabel(Progression->GetProfessionDisplayName());
		Controller->SetDeckAccessEnabled(true);
	}
	RestoreRunHealthToPlayer();
	GetWorldTimerManager().SetTimerForNextTick(
		this,
		&AWorldWalkerGameModeBase::ResumeOrPresentFantasyRun);
}

void AWorldWalkerGameModeBase::HandleDeckViewToggle()
{
	AWorldWalkerPlayerController* Controller = GetWorldWalkerController();
	if (!Controller || !ActiveCardCombat || ActiveCardCombat->GetStartingDeckCount() <= 0)
	{
		return;
	}

	if (bDeckViewerOpen)
	{
		bDeckViewerOpen = false;
		if (ActivePlayer && bRestoreMovementAfterDeckViewer)
		{
			ActivePlayer->SetCombatLocked(false);
		}
		bRestoreMovementAfterDeckViewer = false;
		const bool bUiOnly = FantasyRunFlowState == EFantasyRunFlowState::ProfessionChoice
			|| FantasyRunFlowState == EFantasyRunFlowState::Resolution
			|| FantasyRunFlowState == EFantasyRunFlowState::ChapterComplete;
		const bool bCombatInput = bCombatActive
			|| FantasyRunFlowState == EFantasyRunFlowState::RewardChoice
			|| FantasyRunFlowState == EFantasyRunFlowState::RewardConfirmed
			|| FantasyRunFlowState == EFantasyRunFlowState::Defeat;
		Controller->HideDeckViewer(bCombatInput, bUiOnly);
		UE_LOG(LogTemp, Display, TEXT("W01_DECK_VIEW_CLOSED"));
		return;
	}

	UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	bDeckViewerOpen = true;
	bRestoreMovementAfterDeckViewer = ActivePlayer
		&& !bCombatActive
		&& FantasyRunFlowState != EFantasyRunFlowState::Resolution
		&& FantasyRunFlowState != EFantasyRunFlowState::ChapterComplete;
	if (ActivePlayer && bRestoreMovementAfterDeckViewer)
	{
		ActivePlayer->SetCombatLocked(true);
	}
	Controller->ShowDeckViewer(
		FString::Printf(
			TEXT("当前牌组 · %s · 共 %d 张"),
			Progression ? *Progression->GetProfessionDisplayName() : TEXT("旅人"),
			ActiveCardCombat->GetStartingDeckCount()),
		ActiveCardCombat->BuildCurrentDeckSummary());
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_DECK_VIEW_OPENED Cards=%d"),
		ActiveCardCombat->GetStartingDeckCount());
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
				TEXT("你已完成三章十八层旅途。可继续探索世界，或从返回门回到主世界。"),
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

	TArray<FText> ChoiceTitles;
	TArray<FText> ChoiceDescriptions;
	TArray<FLinearColor> ChoiceColors;
	for (const FFantasyRouteNodeChoice& Choice : Progression->GetRouteChoices())
	{
		const TCHAR* TypeLabel = TEXT("战斗");
		FLinearColor ChoiceColor(0.72f, 0.12f, 0.08f, 1.0f);
		switch (Choice.NodeType)
		{
		case EFantasyRouteNodeType::EliteCombat:
			TypeLabel = TEXT("精英战");
			ChoiceColor = FLinearColor(0.58f, 0.12f, 0.78f, 1.0f);
			break;
		case EFantasyRouteNodeType::Event:
			TypeLabel = TEXT("命运事件");
			ChoiceColor = FLinearColor(0.08f, 0.55f, 0.92f, 1.0f);
			break;
		case EFantasyRouteNodeType::Rest:
			TypeLabel = TEXT("休整");
			ChoiceColor = FLinearColor(0.12f, 0.72f, 0.36f, 1.0f);
			break;
		case EFantasyRouteNodeType::Shop:
			TypeLabel = TEXT("商店");
			ChoiceColor = FLinearColor(0.86f, 0.57f, 0.09f, 1.0f);
			break;
		case EFantasyRouteNodeType::Treasure:
			TypeLabel = TEXT("宝箱");
			ChoiceColor = FLinearColor(0.10f, 0.72f, 0.78f, 1.0f);
			break;
		case EFantasyRouteNodeType::Boss:
			TypeLabel = TEXT("守关战");
			ChoiceColor = FLinearColor(1.0f, 0.32f, 0.035f, 1.0f);
			break;
		case EFantasyRouteNodeType::Combat:
		default: break;
		}
		ChoiceTitles.Add(FText::FromString(FString::Printf(
			TEXT("【%s】%s"), TypeLabel, *Choice.DisplayName.ToString())));
		ChoiceDescriptions.Add(FText::FromString(FString::Printf(
			TEXT("%s\n走入光柱选择"), *Choice.Description.ToString())));
		ChoiceColors.Add(ChoiceColor);
	}

	FantasyRunFlowState = EFantasyRunFlowState::RouteChoice;
	ActivePlayer->SetCombatLocked(false);
	ExplorationMessage = FString::Printf(
		TEXT("%s\n前方出现三座命运标记：走入光柱选择下一条路线。"),
		*Progression->BuildRunSummary());
	Controller->ExitCombatToExploration(ExplorationMessage);
	SpawnWorldChoices(false, ChoiceTitles, ChoiceDescriptions, ChoiceColors);
	if (AFantasyAmbientSoundscape* Soundscape = GetFantasySoundscape())
	{
		Soundscape->PlayInterfaceCue(EFantasyAudioCue::Route);
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_3D_ROUTE_CHOICES_READY Count=%d"),
		ActiveWorldChoices.Num());
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
	DestroyWorldChoices();
	ActivateRouteNode(SelectedNode);
}

void AWorldWalkerGameModeBase::SpawnWorldChoices(
	const bool bEventChoices,
	const TArray<FText>& Titles,
	const TArray<FText>& Descriptions,
	const TArray<FLinearColor>& Colors)
{
	DestroyWorldChoices();
	if (!GetWorld() || !ActivePlayer)
	{
		return;
	}

	const FVector Forward = ActiveFantasyWorld
		? ActiveFantasyWorld->GetForwardFacingRotation().Vector().GetSafeNormal2D()
		: ActivePlayer->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
	const FVector PlayerFeet = ActivePlayer->GetActorLocation() - FVector(0.0f, 0.0f, 88.0f);
	const float LateralSpacing = Titles.Num() > 2 ? 285.0f : 240.0f;
	const float CenterOffset = 0.5f * static_cast<float>(Titles.Num() - 1);
	const FVector DesiredChoiceCenter = PlayerFeet
		+ Forward * (bEventChoices ? 520.0f : 620.0f);
	const FVector ChoiceCenter = ActiveFantasyWorld
		? ActiveFantasyWorld->ConstrainChoiceCenterToPlayableArea(DesiredChoiceCenter)
		: DesiredChoiceCenter;

	for (int32 ChoiceIndex = 0; ChoiceIndex < Titles.Num(); ++ChoiceIndex)
	{
		FVector SpawnLocation = ChoiceCenter
			+ Right * ((static_cast<float>(ChoiceIndex) - CenterOffset) * LateralSpacing);
		FHitResult GroundHit;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(W01WorldChoiceGround), false, ActivePlayer);
		if (GetWorld()->LineTraceSingleByChannel(
			GroundHit,
			SpawnLocation + FVector(0.0f, 0.0f, 650.0f),
			SpawnLocation - FVector(0.0f, 0.0f, 950.0f),
			ECC_Visibility,
			QueryParams))
		{
			SpawnLocation.Z = GroundHit.ImpactPoint.Z;
		}
		else if (ActiveFantasyWorld)
		{
			SpawnLocation.Z = ActiveFantasyWorld->GetActorLocation().Z;
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("W01 choice ground trace missed; using authored platform height. Index=%d Location=%s"),
				ChoiceIndex,
				*SpawnLocation.ToCompactString());
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AFantasyWorldChoiceActor* ChoiceActor = GetWorld()->SpawnActor<AFantasyWorldChoiceActor>(
			AFantasyWorldChoiceActor::StaticClass(),
			SpawnLocation,
			(ActivePlayer->GetActorLocation() - SpawnLocation).Rotation(),
			SpawnParameters);
		if (!ChoiceActor)
		{
			continue;
		}
		ChoiceActor->ConfigureChoice(
			this,
			bEventChoices ? EFantasyWorldChoiceKind::Event : EFantasyWorldChoiceKind::Route,
			ChoiceIndex,
			Titles[ChoiceIndex],
			Descriptions.IsValidIndex(ChoiceIndex) ? Descriptions[ChoiceIndex] : FText::GetEmpty(),
			Colors.IsValidIndex(ChoiceIndex) ? Colors[ChoiceIndex] : FLinearColor::Blue);
		ActiveWorldChoices.Add(ChoiceActor);
	}
}

void AWorldWalkerGameModeBase::DestroyWorldChoices()
{
	for (AFantasyWorldChoiceActor* ChoiceActor : ActiveWorldChoices)
	{
		if (IsValid(ChoiceActor))
		{
			ChoiceActor->SetChoiceEnabled(false);
			ChoiceActor->Destroy();
		}
	}
	ActiveWorldChoices.Reset();
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
	UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	const bool bMage = Progression
		&& Progression->GetSelectedProfession() == EFantasyPlayerProfession::Mage;
	const FString GiftCardName = bMage ? TEXT("风之石") : TEXT("迅捷攻击");
	const FString SmithCardName = bMage ? TEXT("冰盾") : TEXT("绝对防御");
	const bool bNewEvent = CurrentEventId != EventId;
	if (bNewEvent)
	{
		PendingShopCards.Reset();
		PendingTreasureCard = nullptr;
		PendingEventBlessing = nullptr;
		PendingEventFeedback.Reset();
	}
	if (EventId == TEXT("WanderingMerchant"))
	{
		Title = TEXT("夜路商队");
		Lore = FString::Printf(TEXT("车灯下摆着卡牌、药剂与封蜡圣徽。当前金币：%d。"),
			Progression ? Progression->GetGold() : 0);
		if (PendingShopCards.IsEmpty() && ActiveCardCombat)
		{
			for (UCardDefinition* Card : ActiveCardCombat->BuildRewardChoices(5))
			{
				PendingShopCards.Add(Card);
			}
			while (PendingShopCards.Num() < 5) PendingShopCards.Add(nullptr);
		}
		if (!PendingEventBlessing) PendingEventBlessing = SelectAvailableBlessing(TEXT("ShopBlessing"));
		for (const UCardDefinition* Card : PendingShopCards)
		{
			Choices.Add(Card
				? FString::Printf(TEXT("购买【%s】（%d 金币）"), *Card->DisplayName.ToString(), GetCardShopPrice(Card))
				: TEXT("该卡牌已售出"));
		}
		const int32 DeletePrice = 60 + (Progression ? Progression->GetDeletionCount() * 20 : 0);
		Choices.Add(FString::Printf(TEXT("删除 1 张基础攻击（%d 金币）"), DeletePrice));
		Choices.Add(TEXT("恢复 25 点生命（35 金币）"));
		Choices.Add(PendingEventBlessing
			? FString::Printf(TEXT("购买祝福【%s】（%d 金币）"),
				*PendingEventBlessing->DisplayName.ToString(), PendingEventBlessing->ShopPrice)
			: TEXT("祝福已售罄"));
		Choices.Add(TEXT("离开商店（不消费）"));
	}
	else if (EventId == TEXT("AncientChest"))
	{
		Title = TEXT("封印宝箱");
		Lore = TEXT("三枚锁扣只能开启一枚：金币、祝福或一张旅途卡牌。");
		if (bNewEvent && ActiveCardCombat)
		{
			const TArray<UCardDefinition*> Cards = ActiveCardCombat->BuildRewardChoices(1);
			PendingTreasureCard = Cards.IsEmpty() ? nullptr : Cards[0];
			PendingEventBlessing = SelectAvailableBlessing(TEXT("TreasureBlessing"));
		}
		Choices = {
			TEXT("开启金币锁：获得 45 金币"),
			PendingEventBlessing
				? FString::Printf(TEXT("开启圣徽锁：获得【%s】"), *PendingEventBlessing->DisplayName.ToString())
				: TEXT("开启圣徽锁：转化为 30 金币"),
			PendingTreasureCard
				? FString::Printf(TEXT("开启卡牌锁：获得【%s】"), *PendingTreasureCard->DisplayName.ToString())
				: TEXT("开启卡牌锁：转化为 30 金币")};
	}
	else if (EventId == TEXT("MoonlitWell"))
	{
		Title = TEXT("月下古井");
		Lore = TEXT("井水映出了三个不同的你。每一道倒影都会永久改变本次旅途。");
		Choices = {
			TEXT("饮下银色井水：恢复 25 点生命"),
			FString::Printf(TEXT("触碰血色倒影：失去至多 8 生命（保留 1），获得【%s】"), *GiftCardName),
			TEXT("沉下沉重倒影：从旅途牌组移除 1 张【普通攻击】")};
	}
	else if (EventId == TEXT("AshenSmith"))
	{
		Title = TEXT("灰烬铁匠");
		Lore = TEXT("失明的铁匠只凭声音敲打剑身。他要求你用伤痕、旧剑或一个承诺付账。");
		Choices = {
			FString::Printf(TEXT("以血淬火：失去至多 10 生命（保留 1），获得【%s】"), *SmithCardName),
			TEXT("熔掉旧剑：移除 1 张【普通攻击】"),
			TEXT("记下盾阵：下一场战斗开始时获得 12 格挡")};
	}
	else
	{
		Title = TEXT("流亡者营火");
		Lore = TEXT("无名者们为你留下火种、旧盾和一句祝词。黑棘庭院已在前方。");
		Choices = {
			TEXT("靠近营火休息：恢复 35 点生命"),
			TEXT("借火淬炼：升级牌组中第一张可升级卡牌"),
			TEXT("接过旧盾：下一战获得 8 格挡")};
	}

	CurrentEventId = EventId;
	PendingEventChoiceCount = Choices.Num();
	FantasyRunFlowState = EFantasyRunFlowState::EventChoice;
	ActivePlayer->SetCombatLocked(false);
	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		Controller->ExitCombatToExploration(FString::Printf(
			TEXT("%s\n%s%s\n走入一枚符文，直接刻下选择。"),
			*Title,
			*Lore,
			PendingEventFeedback.IsEmpty()
				? TEXT("")
				: *FString::Printf(TEXT("\n上次交易：%s"), *PendingEventFeedback)));
	}
	TArray<FText> ChoiceTitles;
	TArray<FText> ChoiceDescriptions;
	const TArray<FLinearColor> Palette = {
		FLinearColor(0.78f, 0.18f, 0.08f, 1.0f),
		FLinearColor(0.86f, 0.57f, 0.09f, 1.0f),
		FLinearColor(0.16f, 0.48f, 0.92f, 1.0f)};
	TArray<FLinearColor> ChoiceColors;
	for (int32 ChoiceIndex = 0; ChoiceIndex < Choices.Num(); ++ChoiceIndex)
	{
		ChoiceTitles.Add(FText::FromString(FString::Printf(
			TEXT("选择 %d"), ChoiceIndex + 1)));
		ChoiceDescriptions.Add(FText::FromString(Choices[ChoiceIndex]));
		ChoiceColors.Add(Palette[ChoiceIndex % Palette.Num()]);
	}
	SpawnWorldChoices(true, ChoiceTitles, ChoiceDescriptions, ChoiceColors);
	if (Progression)
	{
		Progression->RecordEventOffer(EventId, FString::Join(Choices, TEXT("|")));
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_3D_EVENT_CHOICES_READY Event=%s Count=%d"),
		*EventId.ToString(),
		ActiveWorldChoices.Num());
}

void AWorldWalkerGameModeBase::HandleEventSelection(const int32 ChoiceIndex)
{
	if (FantasyRunFlowState != EFantasyRunFlowState::EventChoice || !ActivePlayer
		|| ChoiceIndex < 0 || ChoiceIndex >= PendingEventChoiceCount)
	{
		return;
	}
	DestroyWorldChoices();

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
	const bool bMage = Progression
		&& Progression->GetSelectedProfession() == EFantasyPlayerProfession::Mage;
	const FName BasicAttackId = bMage ? FName(TEXT("Mage_NormalAttack")) : FName(TEXT("Knight_NormalAttack"));
	const FName GiftCardId = bMage ? FName(TEXT("Mage_WindStone")) : FName(TEXT("Knight_SwiftAttack"));
	const FName SmithCardId = bMage ? FName(TEXT("Mage_IceShield")) : FName(TEXT("Knight_AbsoluteDefense"));
	const TCHAR* GiftCardName = bMage ? TEXT("风之石") : TEXT("迅捷攻击");
	const TCHAR* SmithCardName = bMage ? TEXT("冰盾") : TEXT("绝对防御");

	if (CurrentEventId == TEXT("WanderingMerchant"))
	{
		bool bLeaveShop = ChoiceIndex == PendingEventChoiceCount - 1;
		if (PendingShopCards.IsValidIndex(ChoiceIndex))
		{
			UCardDefinition* Card = PendingShopCards[ChoiceIndex];
			const int32 Price = GetCardShopPrice(Card);
			if (!Card)
			{
				Result = TEXT("这个货位已经空了。");
			}
			else if (!Progression || Progression->GetGold() < Price)
			{
				Result = TEXT("金币不足，商人把卡牌收回了绒布下。");
			}
			else if (ActiveCardCombat && ActiveCardCombat->GrantRunCard(Card)
				&& Progression->SpendGold(Price))
			{
				PendingShopCards[ChoiceIndex] = nullptr;
				Result = FString::Printf(TEXT("花费 %d 金币购买【%s】。"), Price, *Card->DisplayName.ToString());
			}
			else
			{
				Result = TEXT("交易未能完成，金币没有扣除。");
			}
		}
		else if (ChoiceIndex == 5)
		{
			const int32 Price = 60 + (Progression ? Progression->GetDeletionCount() * 20 : 0);
			if (!Progression || Progression->GetGold() < Price)
			{
				Result = TEXT("金币不足，无法支付删牌费用。");
			}
			else if (ActiveCardCombat && ActiveCardCombat->RemoveCardFromRun(BasicAttackId)
				&& Progression->SpendGold(Price))
			{
				Result = FString::Printf(TEXT("花费 %d 金币移除一张基础攻击。"), Price);
			}
			else
			{
				Result = TEXT("没有可删除的基础攻击，金币没有扣除。");
			}
		}
		else if (ChoiceIndex == 6)
		{
			if (!Progression || Progression->GetGold() < 35)
			{
				Result = TEXT("金币不足，无法购买恢复。");
			}
			else if (PlayerCombatant->GetCurrentHealth() >= PlayerCombatant->GetMaxHealth())
			{
				Result = TEXT("生命已满，无需购买恢复。");
			}
			else if (Progression->SpendGold(35))
			{
				PlayerCombatant->RestoreHealth(25);
				Result = TEXT("花费 35 金币，恢复 25 点生命。");
			}
		}
		else if (ChoiceIndex == 7)
		{
			if (!PendingEventBlessing)
			{
				Result = TEXT("本次旅途可获得的祝福已经售罄。");
			}
			else if (!Progression || Progression->GetGold() < PendingEventBlessing->ShopPrice)
			{
				Result = TEXT("金币不足，无法购买这枚祝福。");
			}
			else if (Progression->GrantBlessing(PendingEventBlessing->BlessingId)
				&& Progression->SpendGold(PendingEventBlessing->ShopPrice))
			{
				Result = FString::Printf(TEXT("获得祝福【%s】。"), *PendingEventBlessing->DisplayName.ToString());
				PendingEventBlessing = SelectAvailableBlessing(TEXT("ShopBlessing"));
			}
		}
		else
		{
			Result = TEXT("你向商队点头告别，没有被强制消费。");
			bLeaveShop = true;
		}

		if (!bLeaveShop)
		{
			if (Progression) Progression->RecordEventSelection(CurrentEventId, ChoiceIndex);
			SyncRunHealthFromPlayer();
			PendingEventFeedback = Result;
			PresentEventChoices(CurrentEventId);
			return;
		}
	}
	else if (CurrentEventId == TEXT("AncientChest"))
	{
		if (ChoiceIndex == 0)
		{
			if (Progression) Progression->AddGold(45);
			Result = TEXT("宝箱吐出 45 枚仍带余温的金币。");
		}
		else if (ChoiceIndex == 1)
		{
			if (Progression && PendingEventBlessing
				&& Progression->GrantBlessing(PendingEventBlessing->BlessingId))
			{
				Result = FString::Printf(TEXT("获得祝福【%s】。"), *PendingEventBlessing->DisplayName.ToString());
			}
			else
			{
				if (Progression) Progression->AddGold(30);
				Result = TEXT("圣徽锁已经空了，封印转化为 30 金币。");
			}
		}
		else
		{
			if (PendingTreasureCard && ActiveCardCombat
				&& ActiveCardCombat->GrantRunCard(PendingTreasureCard))
			{
				Result = FString::Printf(TEXT("获得卡牌【%s】。"), *PendingTreasureCard->DisplayName.ToString());
			}
			else
			{
				if (Progression) Progression->AddGold(30);
				Result = TEXT("卡牌锁已经空了，封印转化为 30 金币。");
			}
		}
	}
	else if (CurrentEventId == TEXT("MoonlitWell"))
	{
		if (ChoiceIndex == 0)
		{
			PlayerCombatant->RestoreHealth(25);
			Result = TEXT("冰凉井水洗去了疲惫。恢复 25 点生命。");
		}
		else if (ChoiceIndex == 1)
		{
			const int32 HealthLost = ApplyNonLethalHealthLoss(8);
			const bool bGranted = GrantCardById(GiftCardId);
			Result = bGranted
				? FString::Printf(TEXT("倒影夺走 %d 点生命，却将【%s】留在你手中。"), HealthLost, GiftCardName)
				: FString::Printf(TEXT("你失去 %d 点生命，但卡牌资料尚未生成。"), HealthLost);
		}
		else
		{
			const bool bRemoved = ActiveCardCombat
				&& ActiveCardCombat->RemoveCardFromRun(BasicAttackId);
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
			const bool bGranted = GrantCardById(SmithCardId);
			Result = bGranted
				? FString::Printf(TEXT("铁锤落下，你失去 %d 点生命并获得【%s】。"), HealthLost, SmithCardName)
				: FString::Printf(TEXT("你失去 %d 点生命，但卡牌资料尚未生成。"), HealthLost);
		}
		else if (ChoiceIndex == 1)
		{
			const bool bRemoved = ActiveCardCombat
				&& ActiveCardCombat->RemoveCardFromRun(BasicAttackId);
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
			const bool bUpgraded = ActiveCardCombat && ActiveCardCombat->UpgradeFirstEligibleRunCard();
			Result = bUpgraded
				? TEXT("火星沿着牌面游走，一张卡牌完成了升级。")
				: TEXT("当前牌组没有可升级的卡牌，你只在火边稍作休整。");
		}
		else
		{
			if (Progression)
			{
				Progression->AddPendingBattleBoon(8, 0);
			}
			Result = TEXT("你将旧盾挂在背后。下一战初始格挡 +8。");
		}
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_EVENT_RESOLVED Event=%s Choice=%d"),
		*CurrentEventId.ToString(),
		ChoiceIndex);
	if (Progression)
	{
		Progression->RecordEventSelection(CurrentEventId, ChoiceIndex);
	}
	SyncRunHealthFromPlayer();
	CompleteActiveNode(Result);
}

void AWorldWalkerGameModeBase::CompleteActiveNode(const FString& ResolutionMessage)
{
	UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	const int32 ChapterBefore = Progression ? Progression->GetCurrentChapter() : 0;
	if (!Progression || !Progression->CompleteActiveNode())
	{
		return;
	}

	CurrentEventId = NAME_None;
	PendingEventChoiceCount = 0;
	PendingEventFeedback.Reset();
	PendingShopCards.Reset();
	PendingTreasureCard = nullptr;
	PendingEventBlessing = nullptr;
	const bool bChapterComplete = Progression->IsChapterComplete();
	FString FinalMessage = ResolutionMessage;
	if (!bChapterComplete && Progression->GetCurrentChapter() > ChapterBefore)
	{
		FinalMessage += FString::Printf(
			TEXT("\n章节过渡完成：进入第 %d 章，生命、牌组、金币与祝福全部保留。"),
			Progression->GetCurrentChapter());
	}
	FantasyRunFlowState = bChapterComplete
		? EFantasyRunFlowState::ChapterComplete
		: EFantasyRunFlowState::Resolution;
	if (ActivePlayer)
	{
		ActivePlayer->SetCombatLocked(true);
	}
	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		Controller->ShowNodeResolution(FinalMessage, bChapterComplete);
	}
	if (bChapterComplete)
	{
		UE_LOG(LogTemp, Display, TEXT("W01_CAMPAIGN_COMPLETE Chapters=3 Depths=18"));
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
				TEXT("三章十八层旅途已完成。你可以与营地居民交谈，或从右后方的门返回主世界。"));
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
		UE_LOG(
			LogTemp,
			Display,
			TEXT("World portal ready. Destination=%s Location=%s"),
			*DestinationWorld->WorldId.ToString(),
			*SpawnLocation.ToCompactString());
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
	UFantasyCardProgressionSubsystem* RandomProgression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	const bool bUsingRealEnemyDeck = ActiveEnemyDeck
		&& ActiveEnemyDeck->Initialize(
			ActiveFantasyEnemyDefinition,
			RandomProgression
				? RandomProgression->ConsumeDeterministicSeed(TEXT("EnemyBattle"))
				: INDEX_NONE);
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
	MechanicTurnTriggerCounts.Reset();
	MechanicBattleTriggerCounts.Reset();
	MechanicSuppressedTurns.Reset();
	BlessingBattleTriggerCounts.Reset();
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
	DispatchEnemyMechanics(EFantasyMechanicTrigger::BattleStarted);
	DispatchPlayerBlessings(EFantasyBlessingTrigger::BattleStarted);
	DispatchPlayerBlessings(EFantasyBlessingTrigger::PlayerTurnStarted);

	FantasyRunFlowState = EFantasyRunFlowState::PlayerTurn;
	ActivePlayer->SetCombatLocked(true);
	ActiveEnemy->SetInCombat(true);
	if (AFantasyAmbientSoundscape* Soundscape = GetFantasySoundscape())
	{
		Soundscape->SetBattleMusicActive(true);
		Soundscape->PlayInterfaceCue(EFantasyAudioCue::Draw);
	}

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
				: TEXT("未知对手"),
			GetEnemyPortraitTexture());
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
	if (RandomProgression)
	{
		RandomProgression->RecordBattleStarted(
			ActiveFantasyEnemyDefinition
				? ActiveFantasyEnemyDefinition->EnemyId
				: NAME_None,
			ActiveCardCombat->GetStartingDeckCount());
	}
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
	if (UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr)
	{
		Progression->LogStructuredEvent(
			TEXT("CardPlayed"),
			{{TEXT("actor"), TEXT("Player")},
			 {TEXT("cardId"), Card->CardId.ToString()}});
	}
	if (AFantasyAmbientSoundscape* Soundscape = GetFantasySoundscape())
	{
		Soundscape->PlayCardCue(GetCardAudioCue(Card), false);
	}

	const int32 EnemyHealthBeforeCard = ActiveEnemy->GetCombatantComponent()->GetCurrentHealth();
	FString TriggeredCounterName;
	if (ActiveEnemyDeck)
	{
		if (UCardDefinition* CounterCard = ActiveEnemyDeck->ConsumeNextCounter())
		{
			TriggeredCounterName = CounterCard->DisplayName.ToString();
			if (AFantasyAmbientSoundscape* Soundscape = GetFantasySoundscape())
			{
				Soundscape->PlayCardCue(EFantasyAudioCue::Counter, true);
			}
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
	DispatchPlayerBlessings(EFantasyBlessingTrigger::PlayerCardResolved, Card);
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
	if (AFantasyAmbientSoundscape* Soundscape = GetFantasySoundscape())
	{
		Soundscape->PlayInterfaceCue(EFantasyAudioCue::TurnEnd);
	}
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
		CombatTiming.EnemyTurnStartDelay,
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
	MechanicTurnTriggerCounts.Reset();
	DispatchEnemyMechanics(EFantasyMechanicTrigger::EnemyTurnStarted);
	DispatchPlayerBlessings(EFantasyBlessingTrigger::EnemyTurnStarted);
	for (auto It = MechanicSuppressedTurns.CreateIterator(); It; ++It)
	{
		It.Value() = FMath::Max(0, It.Value() - 1);
		if (It.Value() == 0)
		{
			It.RemoveCurrent();
		}
	}
	CurrentEnemyTurnCardNames.Reset();
	if (ActiveEnemyDeck && ActiveEnemyDeck->IsInitialized())
	{
		HandleEnemyTurnStep();
		return;
	}
	if (ActiveFantasyEnemyDefinition
		&& !ActiveFantasyEnemyDefinition->IntentCycle.IsEmpty())
	{
		const FFantasyEnemyIntentStep& Intent =
			ActiveFantasyEnemyDefinition->IntentCycle[CurrentEnemyIntentIndex];
		ResolveEnemyIntent(Intent);
		CurrentEnemyTurnCardNames.Add(Intent.DisplayName.ToString());
		CurrentEnemyIntentIndex =
			(CurrentEnemyIntentIndex + 1) % ActiveFantasyEnemyDefinition->IntentCycle.Num();
		SyncRunHealthFromPlayer();
		RefreshCombatUI();
		GetWorldTimerManager().SetTimer(
			EnemyTurnTimer,
			this,
			&AWorldWalkerGameModeBase::FinishEnemyTurnSequence,
			CombatTiming.EnemyCardPresentationDelay,
			false);
		return;
	}

	UE_LOG(LogTemp, Error, TEXT("Cannot execute W01 enemy turn: deck and intent fallback are unavailable."));
	FinishCombat(false);
}

void AWorldWalkerGameModeBase::HandleEnemyTurnStep()
{
	if (!bCombatActive || FantasyRunFlowState != EFantasyRunFlowState::EnemyTurn
		|| !ActivePlayer || !ActiveEnemy || !ActiveCardCombat || !ActiveEnemyDeck)
	{
		return;
	}

	UCardDefinition* EnemyCard = ActiveEnemyDeck->PlayNextCard();
	if (!EnemyCard)
	{
		FinishEnemyTurnSequence();
		return;
	}

	CurrentEnemyTurnCardNames.Add(EnemyCard->DisplayName.ToString());
	if (UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr)
	{
		Progression->LogStructuredEvent(
			TEXT("CardPlayed"),
			{{TEXT("actor"), TEXT("Enemy")},
			 {TEXT("cardId"), EnemyCard->CardId.ToString()}},
			{{TEXT("turn"), CurrentEnemyTurnNumber}});
	}
	if (AFantasyAmbientSoundscape* Soundscape = GetFantasySoundscape())
	{
		Soundscape->PlayCardCue(GetCardAudioCue(EnemyCard), true);
	}
	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		Controller->SetCombatMessage(FString::Printf(
			TEXT("%s 打出【%s】……"),
			ActiveFantasyEnemyDefinition
				? *ActiveFantasyEnemyDefinition->DisplayName.ToString()
				: TEXT("对手"),
			*EnemyCard->DisplayName.ToString()), false);
	}

	if (EnemyCard->CardType == ECardType::Counter)
	{
		// The counter is armed now and resolves exactly once before the next
		// player card. It still receives its own presentation beat here.
		ActiveEnemy->PlayIntentAnimation(EWorldWalkerEnemyAnimationCue::Empower);
	}
	else
	{
		ResolveEnemyCardEffects(EnemyCard);
	}
	ActiveEnemyDeck->FinalizeCard(EnemyCard);
	SyncRunHealthFromPlayer();
	RefreshCombatUI();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_ENEMY_CARD_PRESENTED Card=%s Turn=%d Delay=%.2f"),
		*EnemyCard->CardId.ToString(),
		CurrentEnemyTurnNumber,
		CombatTiming.EnemyCardPresentationDelay);

	if (!ActivePlayer->GetCombatantComponent()->IsAlive())
	{
		FinishCombat(false);
		return;
	}
	if (!ActiveEnemy->GetCombatantComponent()->IsAlive())
	{
		if (!TryTriggerEnemyDefeatPassive())
		{
			FinishCombat(true);
		}
		return;
	}

	GetWorldTimerManager().SetTimer(
		EnemyTurnTimer,
		this,
		&AWorldWalkerGameModeBase::HandleEnemyTurnStep,
		CombatTiming.EnemyCardPresentationDelay,
		false);
}

void AWorldWalkerGameModeBase::FinishEnemyTurnSequence()
{
	if (!bCombatActive || !ActivePlayer || !ActiveEnemy || !ActiveCardCombat)
	{
		return;
	}

	ResolveEndOfTurnPoison(false);
	EnemyFantasyState.RemoveStatus(EFantasyCombatStatus::Chill, 1);
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
	DispatchPlayerBlessings(EFantasyBlessingTrigger::PlayerTurnStarted);
	bWaitingForEnemy = false;
	FantasyRunFlowState = EFantasyRunFlowState::PlayerTurn;
	RefreshCombatUI();
	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		const FString ResolvedSummary = CurrentEnemyTurnCardNames.IsEmpty()
			? TEXT("对手没有可支付的牌，跳过了行动")
			: FString::Printf(
				TEXT("对手打出：%s"),
				*FString::Join(CurrentEnemyTurnCardNames, TEXT("、")));
		Controller->SetCombatMessage(FString::Printf(
			TEXT("%s。轮到你行动，敌人的下一张牌已经公开。"),
			*ResolvedSummary), true);
	}
	if (AFantasyAmbientSoundscape* Soundscape = GetFantasySoundscape())
	{
		Soundscape->PlayInterfaceCue(EFantasyAudioCue::Draw);
	}
}

void AWorldWalkerGameModeBase::ResolvePlayerCardEffects(UCardDefinition* Card)
{
	if (!Card || !ActivePlayer || !ActiveEnemy || !ActiveCardCombat)
	{
		return;
	}

	switch (Card->CardType)
	{
	case ECardType::Attack:
		ActivePlayer->PlayFantasyCardAttackAnimation();
		break;
	case ECardType::Spell:
		ActivePlayer->PlayFantasyCardSpellAnimation();
		break;
	case ECardType::Action:
	case ECardType::Equipment:
	case ECardType::Counter:
	default:
		ActivePlayer->PlayFantasyCardUtilityAnimation();
		break;
	}

	bool bWeakConsumedForAttack = false;
	int32 TotalHealthDamage = 0;
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
				TotalHealthDamage += HealthDamage;
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
		case EFantasyCombatEffectType::LoseMana:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				ActiveCardCombat->RemoveMana(Effect.Magnitude);
			}
			else if (ActiveEnemyDeck)
			{
				ActiveEnemyDeck->RemoveMana(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::AddTemporaryCard:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				ActiveCardCombat->AddTemporaryCardToDiscard(
					ActiveCardCombat->FindCardDefinition(Effect.PayloadId), Effect.Magnitude, Effect.Limit);
			}
			break;
		case EFantasyCombatEffectType::ConsumeStatusForDamage:
			if (Effect.Target == EFantasyCombatTarget::Opponent)
			{
				const int32 StatusLayers = EnemyFantasyState.GetStatus(Effect.Status);
				EnemyFantasyState.RemoveStatus(Effect.Status, StatusLayers);
				const int32 HealthDamage = ResolveDamageAgainstEnemy(
					StatusLayers * FMath::Max(0, Effect.Multiplier), false, false, Effect.bPiercing);
				ActiveEnemy->GetCombatantComponent()->ReceiveDamage(HealthDamage);
				TotalHealthDamage += HealthDamage;
			}
			break;
		case EFantasyCombatEffectType::ConsumeStatusForBlock:
			{
				const int32 StatusLayers = EnemyFantasyState.GetStatus(Effect.Status);
				EnemyFantasyState.RemoveStatus(Effect.Status, StatusLayers);
				ActiveCardCombat->AddBlock(StatusLayers * FMath::Max(0, Effect.Multiplier));
			}
			break;
		case EFantasyCombatEffectType::DamagePerMana:
			if (Effect.Target == EFantasyCombatTarget::Opponent)
			{
				const int32 ScaledDamage = Effect.Magnitude
					+ ActiveCardCombat->GetCurrentMana() * FMath::Max(0, Effect.Multiplier);
				const int32 HealthDamage = ResolveDamageAgainstEnemy(
					ScaledDamage, false, false, Effect.bPiercing);
				ActiveEnemy->GetCombatantComponent()->ReceiveDamage(HealthDamage);
				TotalHealthDamage += HealthDamage;
			}
			break;
		default:
			break;
		}
	}
	DispatchEnemyMechanics(EFantasyMechanicTrigger::DamageResolved, Card, TotalHealthDamage);
	DispatchEnemyMechanics(EFantasyMechanicTrigger::PlayerCardResolved, Card, TotalHealthDamage);
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
		Damage = FMath::Max(0, Damage - EnemyFantasyState.Chill);
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
	if (HealthDamage > 0)
	{
		DispatchPlayerBlessings(EFantasyBlessingTrigger::PlayerDamaged, nullptr, HealthDamage);
	}
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
	const int32 PlayerBlockBeforeCard = ActiveCardCombat->GetCurrentBlock();
	int32 TotalHealthDamage = 0;
	for (const FFantasyCombatEffectSpec& Effect : Card->Effects)
	{
		switch (Effect.EffectType)
		{
		case EFantasyCombatEffectType::Damage:
			if (Effect.Target == EFantasyCombatTarget::Opponent)
			{
				const int32 StrengthScaledMagnitude = Effect.Magnitude
					+ (!bIsAttackCard && Effect.bScalesWithStrength
						? FMath::Max(0, EnemyFantasyState.Strength)
						: 0);
				const int32 HealthDamage = ResolveDamageAgainstPlayer(
					StrengthScaledMagnitude,
					bIsAttackCard,
					bIsAttackCard && !bWeakConsumed,
					Effect.bPiercing);
				ActivePlayer->GetCombatantComponent()->ReceiveDamage(HealthDamage);
				TotalHealthDamage += HealthDamage;
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
				if (Card->CardType == ECardType::Counter)
				{
					const int32 Drawn = ActiveEnemyDeck->DrawCardsIgnoringHandLimit(Effect.Magnitude);
					UE_LOG(
						LogTemp,
						Display,
						TEXT("W01_ENEMY_COUNTER_DRAW Card=%s Requested=%d Drawn=%d Hand=%d"),
						*Card->CardId.ToString(),
						Effect.Magnitude,
						Drawn,
						ActiveEnemyDeck->GetHandCount());
				}
				else
				{
					ActiveEnemyDeck->DrawCards(Effect.Magnitude);
				}
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
		case EFantasyCombatEffectType::LoseMana:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				ActiveEnemyDeck->RemoveMana(Effect.Magnitude);
			}
			else
			{
				ActiveCardCombat->RemoveMana(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::AddTemporaryCard:
			if (Effect.Target == EFantasyCombatTarget::Opponent)
			{
				ActiveCardCombat->AddTemporaryCardToDiscard(
					ActiveCardCombat->FindCardDefinition(Effect.PayloadId), Effect.Magnitude, Effect.Limit);
			}
			break;
		default:
			break;
		}
	}
	if (bHasOpponentDamage)
	{
		DispatchEnemyMechanics(
			EFantasyMechanicTrigger::EnemyAttackResolved,
			Card,
			TotalHealthDamage,
			PlayerBlockBeforeCard);
	}
}

void AWorldWalkerGameModeBase::DispatchEnemyMechanics(
	const EFantasyMechanicTrigger Trigger,
	const UCardDefinition* SourceCard,
	const int32 ActualDamage,
	const int32 TargetBlockBefore)
{
	if (!ActiveFantasyEnemyDefinition)
	{
		return;
	}
	for (const FFantasyCombatMechanicRule& Rule : ActiveFantasyEnemyDefinition->Mechanics)
	{
		if (Rule.MechanicId.IsNone() || Rule.Trigger != Trigger
			|| MechanicSuppressedTurns.FindRef(Rule.MechanicId) > 0
			|| ActualDamage < Rule.MinActualDamage
			|| (Rule.MaxTargetBlock >= 0 && TargetBlockBefore > Rule.MaxTargetBlock)
			|| (!Rule.RequiredSourceCardTag.IsNone()
				&& (!SourceCard || !SourceCard->BuildTags.Contains(Rule.RequiredSourceCardTag)))
			|| (Rule.Limit == EFantasyMechanicLimit::OncePerTurn
				&& MechanicTurnTriggerCounts.FindRef(Rule.MechanicId) > 0)
			|| (Rule.Limit == EFantasyMechanicLimit::OncePerBattle
				&& MechanicBattleTriggerCounts.FindRef(Rule.MechanicId) > 0))
		{
			continue;
		}

		++MechanicTurnTriggerCounts.FindOrAdd(Rule.MechanicId);
		++MechanicBattleTriggerCounts.FindOrAdd(Rule.MechanicId);
		ExecuteEnemyMechanicEffects(Rule.Effects);
		if (!Rule.SuppressTargetMechanicId.IsNone() && Rule.SuppressTurns > 0)
		{
			int32& Remaining = MechanicSuppressedTurns.FindOrAdd(Rule.SuppressTargetMechanicId);
			Remaining = FMath::Max(Remaining, Rule.SuppressTurns);
		}
		if (UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
			: nullptr)
		{
			Progression->LogStructuredEvent(
				TEXT("MechanicTriggered"),
				{{TEXT("enemyId"), ActiveFantasyEnemyDefinition->EnemyId.ToString()},
				 {TEXT("mechanicId"), Rule.MechanicId.ToString()}},
				{{TEXT("turn"), CurrentEnemyTurnNumber}, {TEXT("actualDamage"), ActualDamage}});
		}
		UE_LOG(LogTemp, Display, TEXT("W01_MECHANIC_TRIGGERED Enemy=%s Mechanic=%s Turn=%d"),
			*ActiveFantasyEnemyDefinition->EnemyId.ToString(), *Rule.MechanicId.ToString(), CurrentEnemyTurnNumber);
	}
}

void AWorldWalkerGameModeBase::ExecuteEnemyMechanicEffects(
	const TArray<FFantasyCombatEffectSpec>& Effects)
{
	if (!ActivePlayer || !ActiveEnemy || !ActiveCardCombat)
	{
		return;
	}
	for (const FFantasyCombatEffectSpec& Effect : Effects)
	{
		switch (Effect.EffectType)
		{
		case EFantasyCombatEffectType::Damage:
			if (Effect.Target == EFantasyCombatTarget::Opponent)
			{
				const int32 HealthDamage = ResolveDamageAgainstPlayer(
					Effect.Magnitude, false, false, Effect.bPiercing);
				ActivePlayer->GetCombatantComponent()->ReceiveDamage(HealthDamage);
				if (HealthDamage > 0)
				{
					ActivePlayer->PlayFantasyHitReactionAnimation();
				}
			}
			else
			{
				ActiveEnemy->GetCombatantComponent()->ReceiveDamage(
					EnemyFantasyState.AbsorbDamage(Effect.Magnitude));
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
		case EFantasyCombatEffectType::ApplyStatus:
			(Effect.Target == EFantasyCombatTarget::Self ? EnemyFantasyState : PlayerFantasyState)
				.AddStatus(Effect.Status, Effect.Magnitude);
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
		case EFantasyCombatEffectType::LoseMana:
			if (Effect.Target == EFantasyCombatTarget::Self && ActiveEnemyDeck)
			{
				ActiveEnemyDeck->RemoveMana(Effect.Magnitude);
			}
			else
			{
				ActiveCardCombat->RemoveMana(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::AddTemporaryCard:
			if (Effect.Target == EFantasyCombatTarget::Opponent)
			{
				ActiveCardCombat->AddTemporaryCardToDiscard(
					ActiveCardCombat->FindCardDefinition(Effect.PayloadId), Effect.Magnitude, Effect.Limit);
			}
			break;
		default:
			break;
		}
	}
}

void AWorldWalkerGameModeBase::LoadBlessingDefinitions()
{
	if (!BlessingDefinitions.IsEmpty())
	{
		return;
	}
	UAssetManager& AssetManager = UAssetManager::Get();
	const FString Root(TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/Data/Blessings"));
	AssetManager.ScanPathsForPrimaryAssets(
		UFantasyBlessingDefinition::PrimaryAssetType,
		{Root},
		UFantasyBlessingDefinition::StaticClass(),
		false,
		false,
		true);
	TArray<FPrimaryAssetId> AssetIds;
	AssetManager.GetPrimaryAssetIdList(UFantasyBlessingDefinition::PrimaryAssetType, AssetIds);
	for (const FPrimaryAssetId& AssetId : AssetIds)
	{
		UFantasyBlessingDefinition* Definition = Cast<UFantasyBlessingDefinition>(
			AssetManager.GetPrimaryAssetPath(AssetId).TryLoad());
		if (Definition && Definition->GetPathName().StartsWith(Root)
			&& Definition->ValidateDefinition().IsEmpty())
		{
			BlessingDefinitions.Add(Definition->BlessingId, Definition);
		}
	}
}

UFantasyBlessingDefinition* AWorldWalkerGameModeBase::SelectAvailableBlessing(
	const FName StreamName)
{
	UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	if (!Progression)
	{
		return nullptr;
	}
	LoadBlessingDefinitions();
	TArray<UFantasyBlessingDefinition*> Candidates;
	for (const TPair<FName, TObjectPtr<UFantasyBlessingDefinition>>& Pair : BlessingDefinitions)
	{
		UFantasyBlessingDefinition* Definition = Pair.Value;
		if (Definition && !Progression->GetBlessings().Contains(Definition->BlessingId)
			&& (Definition->Profession == EFantasyPlayerProfession::None
				|| Definition->Profession == Progression->GetSelectedProfession()))
		{
			Candidates.Add(Definition);
		}
	}
	Candidates.Sort([](const UFantasyBlessingDefinition& Left, const UFantasyBlessingDefinition& Right)
	{
		return Left.BlessingId.LexicalLess(Right.BlessingId);
	});
	if (Candidates.IsEmpty())
	{
		return nullptr;
	}
	const int32 Seed = Progression->ConsumeDeterministicSeed(StreamName);
	return Candidates[FMath::Abs(Seed) % Candidates.Num()];
}

void AWorldWalkerGameModeBase::DispatchPlayerBlessings(
	const EFantasyBlessingTrigger Trigger,
	const UCardDefinition* SourceCard,
	const int32 ActualDamage)
{
	UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	if (!Progression
		|| (!bCombatActive && Trigger != EFantasyBlessingTrigger::RewardSkipped))
	{
		return;
	}
	LoadBlessingDefinitions();
	for (const FName BlessingId : Progression->GetBlessings())
	{
		const TObjectPtr<UFantasyBlessingDefinition>* Found = BlessingDefinitions.Find(BlessingId);
		const UFantasyBlessingDefinition* Definition = Found ? Found->Get() : nullptr;
		if (!Definition || Definition->Trigger != Trigger
			|| (Definition->Profession != EFantasyPlayerProfession::None
				&& Definition->Profession != Progression->GetSelectedProfession())
			|| (!Definition->RequiredCardTag.IsNone()
				&& (!SourceCard || !SourceCard->BuildTags.Contains(Definition->RequiredCardTag)))
			|| (Definition->MaxTriggersPerBattle > 0
				&& BlessingBattleTriggerCounts.FindRef(BlessingId) >= Definition->MaxTriggersPerBattle))
		{
			continue;
		}

		++BlessingBattleTriggerCounts.FindOrAdd(BlessingId);
		ExecutePlayerBlessingEffects(Definition->Effects);
		Progression->LogStructuredEvent(
			TEXT("BlessingTriggered"),
			{{TEXT("blessingId"), BlessingId.ToString()}},
			{{TEXT("turn"), CurrentEnemyTurnNumber}, {TEXT("actualDamage"), ActualDamage}});
	}
}

void AWorldWalkerGameModeBase::ExecutePlayerBlessingEffects(
	const TArray<FFantasyCombatEffectSpec>& Effects)
{
	if (!ActivePlayer || !ActiveCardCombat)
	{
		return;
	}
	for (const FFantasyCombatEffectSpec& Effect : Effects)
	{
		switch (Effect.EffectType)
		{
		case EFantasyCombatEffectType::Damage:
			if (Effect.Target == EFantasyCombatTarget::Opponent && ActiveEnemy)
			{
				ActiveEnemy->GetCombatantComponent()->ReceiveDamage(
					ResolveDamageAgainstEnemy(Effect.Magnitude, false, false, Effect.bPiercing));
			}
			break;
		case EFantasyCombatEffectType::Block:
			if (Effect.Target == EFantasyCombatTarget::Self) ActiveCardCombat->AddBlock(Effect.Magnitude);
			break;
		case EFantasyCombatEffectType::Heal:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				ActivePlayer->GetCombatantComponent()->RestoreHealth(Effect.Magnitude);
			}
			break;
		case EFantasyCombatEffectType::Draw:
			if (Effect.Target == EFantasyCombatTarget::Self) ActiveCardCombat->DrawCards(Effect.Magnitude);
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
			if (Effect.Target == EFantasyCombatTarget::Self) ActiveCardCombat->AddValor(Effect.Magnitude);
			break;
		case EFantasyCombatEffectType::GainAction:
			if (Effect.Target == EFantasyCombatTarget::Self) ActiveCardCombat->AddActionPoints(Effect.Magnitude);
			break;
		case EFantasyCombatEffectType::GainMana:
			if (Effect.Target == EFantasyCombatTarget::Self) ActiveCardCombat->AddMana(Effect.Magnitude);
			break;
		case EFantasyCombatEffectType::AddTemporaryCard:
			if (Effect.Target == EFantasyCombatTarget::Self)
			{
				ActiveCardCombat->AddTemporaryCardToDiscard(
					ActiveCardCombat->FindCardDefinition(Effect.PayloadId), Effect.Magnitude, Effect.Limit);
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
	CurrentEnemyTurnCardNames.Reset();
	SyncRunHealthFromPlayer();
	RefreshCombatUI();
	UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr;
	const FName EnemyId = ActiveFantasyEnemyDefinition
		? ActiveFantasyEnemyDefinition->EnemyId
		: NAME_None;
	if (!bPlayerWon)
	{
		if (Progression)
		{
			Progression->EndRun(EFantasyRunEndReason::Defeat, EnemyId);
		}
	}
	else if (Progression)
	{
		Progression->RecordEnemyDefeated(EnemyId);
		const EFantasyEncounterTier Tier = ActiveFantasyEnemyDefinition
			? ActiveFantasyEnemyDefinition->EncounterTier
			: EFantasyEncounterTier::Normal;
		const int32 GoldReward = Tier == EFantasyEncounterTier::Boss
			? 45
			: Tier == EFantasyEncounterTier::Elite ? 30 : 18;
		Progression->AddGold(GoldReward);
		if (Tier == EFantasyEncounterTier::Boss)
		{
			if (UFantasyBlessingDefinition* Blessing = SelectAvailableBlessing(TEXT("BossBlessing")))
			{
				Progression->GrantBlessing(Blessing->BlessingId);
			}
		}
	}

	if (bPlayerWon && ActiveEnemy)
	{
		ActiveEnemy->PlayIntentAnimation(EWorldWalkerEnemyAnimationCue::Death);
		ActiveEnemy->SetActorEnableCollision(false);
	}

	if (bPlayerWon)
	{
		if (AFantasyAmbientSoundscape* Soundscape = GetFantasySoundscape())
		{
			Soundscape->PlayInterfaceCue(EFantasyAudioCue::Reward);
		}
		BeginVictoryReward();
	}
	else if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		if (AFantasyAmbientSoundscape* Soundscape = GetFantasySoundscape())
		{
			Soundscape->SetBattleMusicActive(false);
		}
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
	bRewardWasSkipped = false;
	FantasyRunFlowState = EFantasyRunFlowState::RewardChoice;
	TArray<FString> RewardCandidateIds;
	RewardCandidateIds.Reserve(PendingRewardChoices.Num());
	for (const UCardDefinition* Card : PendingRewardChoices)
	{
		RewardCandidateIds.Add(Card ? Card->CardId.ToString() : TEXT("None"));
	}
	if (UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr)
	{
		Progression->RecordRewardOffer(
			FString::Join(RewardCandidateIds, TEXT("|")));
	}

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
	bRewardWasSkipped = false;
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
	if (UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr)
	{
		Progression->RecordRewardSelection(RewardCard->CardId, false, RewardIndex);
	}
}

void AWorldWalkerGameModeBase::HandleRewardSkip()
{
	if (FantasyRunFlowState != EFantasyRunFlowState::RewardChoice
		|| !bAwaitingRewardSelection)
	{
		return;
	}

	bAwaitingRewardSelection = false;
	bRewardReadyToLeave = true;
	bRewardWasSkipped = true;
	FantasyRunFlowState = EFantasyRunFlowState::RewardConfirmed;
	PendingRewardChoices.Reset();
	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		Controller->ShowRewardConfirmation(
			TEXT("你没有拿走任何卡牌。牌组保持不变，可以继续路线。"));
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_VICTORY_REWARD_SKIPPED Deck=%d"),
		ActiveCardCombat ? ActiveCardCombat->GetStartingDeckCount() : 0);
	if (UFantasyCardProgressionSubsystem* Progression = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFantasyCardProgressionSubsystem>()
		: nullptr)
	{
		Progression->RecordRewardSelection(NAME_None, true, INDEX_NONE);
		Progression->AddGold(8);
	}
	DispatchPlayerBlessings(EFantasyBlessingTrigger::RewardSkipped);
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
	if (AFantasyAmbientSoundscape* Soundscape = GetFantasySoundscape())
	{
		Soundscape->SetBattleMusicActive(false);
	}
	ClearActiveEnemy();
	const FString ResolutionMessage = bRewardWasSkipped
		? FString::Printf(
			TEXT("你击败了【%s】，并选择不拿卡牌。旅途中累计获得 %d 张卡。"),
			*DefeatedEnemyName,
			RewardCount)
		: FString::Printf(
			TEXT("你击败了【%s】，战利品已加入牌组。旅途中累计获得 %d 张卡。"),
			*DefeatedEnemyName,
			RewardCount);
	CompleteActiveNode(ResolutionMessage);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_REWARD_RETURNED_TO_EXPLORATION Enemy=%s Rewards=%d Skipped=%d"),
		*DefeatedEnemyName,
		RewardCount,
		bRewardWasSkipped ? 1 : 0);
	bRewardWasSkipped = false;
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
			GetEnemyPortraitTexture(),
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

AFantasyAmbientSoundscape* AWorldWalkerGameModeBase::GetFantasySoundscape() const
{
	return ActiveFantasyWorld ? ActiveFantasyWorld->GetAmbientSoundscape() : nullptr;
}

UTexture2D* AWorldWalkerGameModeBase::GetEnemyPortraitTexture() const
{
	if (!ActiveFantasyEnemyDefinition)
	{
		return nullptr;
	}
	for (const FFantasyEnemyDeckEntry& Entry : ActiveFantasyEnemyDefinition->Deck)
	{
		if (Entry.Card && !Entry.Card->Artwork.IsNull())
		{
			if (UTexture2D* Artwork = Entry.Card->Artwork.LoadSynchronous())
			{
				return Artwork;
			}
		}
	}
	return nullptr;
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
