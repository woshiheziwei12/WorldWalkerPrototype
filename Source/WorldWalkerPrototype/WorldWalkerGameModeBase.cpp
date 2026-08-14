#include "WorldWalkerGameModeBase.h"

#include "Characters/WorldWalkerCharacter.h"
#include "Characters/WorldWalkerEnemy.h"
#include "Cards/CardCombatComponent.h"
#include "Cards/CardDefinition.h"
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
		ExplorationMessage = TEXT("灰烬王国 · 黑棘隘口\n沿火炬小径调查营地 | E 交谈/挑战 | Q 切换形态 | 右后方返回主世界");
		ActivePlayer->ConfigureFantasyWorldForm(true, true);
		if (UCardCombatComponent* CardCombat = ActivePlayer->GetCardCombatComponent())
		{
			CardCombat->LoadStartingDeck();
		}
		LoadFantasyEnemyDefinition();
		SpawnFantasyWorldLayout();
		SpawnFantasyBattleArena();
		SpawnTestEnemy();
		const FVector GroundOrigin = ActivePlayer->GetActorLocation() - FVector(0.0f, 0.0f, 88.0f);
		const FVector PortalOffset = ActiveFantasyWorld
			? ActiveFantasyWorld->GetReturnPortalLocation() - GroundOrigin
			: ActivePlayer->GetActorRightVector().GetSafeNormal2D() * 650.0f;
		SpawnPortal(
			TravelSubsystem->GetWorldDefinition(UWorldTravelSubsystem::MainWorldId),
			PortalOffset);
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
		ActiveEnemy->ConfigureFantasyPresentation(EnemyName);
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

void AWorldWalkerGameModeBase::LoadFantasyEnemyDefinition()
{
	static const TCHAR* EnemyDefinitionPath =
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/Data/Enemies/DA_BlackthornOathKnight.DA_BlackthornOathKnight");
	ActiveFantasyEnemyDefinition = LoadObject<UFantasyEnemyDefinition>(nullptr, EnemyDefinitionPath);
	if (!ActiveFantasyEnemyDefinition)
	{
		UE_LOG(LogTemp, Error, TEXT("W01 enemy definition is missing: %s"), EnemyDefinitionPath);
	}
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
		LoadFantasyEnemyDefinition();
	}
	if (!ActiveCardCombat || !ActiveCardCombat->StartBattle())
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot start card combat: starter deck is unavailable."));
		return;
	}

	bCombatActive = true;
	bWaitingForEnemy = false;
	CurrentEnemyIntentIndex = 0;
	PlayerFantasyState = FFantasyCombatRuntimeState();
	EnemyFantasyState = FFantasyCombatRuntimeState();
	if (ActiveFantasyEnemyDefinition)
	{
		ActiveEnemy->GetCombatantComponent()->ConfigureMaxHealth(ActiveFantasyEnemyDefinition->MaxHealth);
	}

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
			EnemyCombatant->GetMaxHealth());
		RefreshCombatUI();
		Controller->SetCombatMessage(TEXT("你的回合：依次点亮钢铁、圣徽与奥术，可触发三印共鸣。"), true);
	}
}

void AWorldWalkerGameModeBase::HandlePlayCard(const int32 HandIndex)
{
	if (!bCombatActive || bWaitingForEnemy || !ActivePlayer || !ActiveEnemy || !ActiveCardCombat)
	{
		return;
	}

	UCardDefinition* Card = ActiveCardCombat->PlayCard(HandIndex);
	if (!Card)
	{
		if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
		{
			Controller->SetCombatMessage(TEXT("能量或英勇不足，暂时无法打出这张牌。"), true);
		}
		return;
	}

	ResolvePlayerCardEffects(Card);

	RefreshCombatUI();

	if (!ActiveEnemy->GetCombatantComponent()->IsAlive())
	{
		FinishCombat(true);
		return;
	}

	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		FString Message = FString::Printf(TEXT("打出：%s。"), *Card->DisplayName.ToString());
		if (ActiveCardCombat->LastCardTriggeredResonance())
		{
			Message += TEXT(" 三印共鸣：英勇 +1，抽 1 张牌。");
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
	if (!bCombatActive || bWaitingForEnemy || !ActiveCardCombat)
	{
		return;
	}

	bWaitingForEnemy = true;
	ActiveCardCombat->EndPlayerTurn();
	RefreshCombatUI();
	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		Controller->SetCombatMessage(TEXT("黑棘誓约骑士正在执行已公开的意图……"), false);
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

	if (!ActiveFantasyEnemyDefinition || ActiveFantasyEnemyDefinition->IntentCycle.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot execute W01 enemy turn: intent cycle is unavailable."));
		FinishCombat(false);
		return;
	}

	// A block generated by the enemy survives the player's turn and expires here.
	EnemyFantasyState.Block = 0;
	const FFantasyEnemyIntentStep& Intent = ActiveFantasyEnemyDefinition->IntentCycle[CurrentEnemyIntentIndex];
	ResolveEnemyIntent(Intent);
	CurrentEnemyIntentIndex = (CurrentEnemyIntentIndex + 1) % ActiveFantasyEnemyDefinition->IntentCycle.Num();
	RefreshCombatUI();

	if (!ActivePlayer->GetCombatantComponent()->IsAlive())
	{
		FinishCombat(false);
		return;
	}

	if (ActiveCardCombat)
	{
		ActiveCardCombat->StartPlayerTurn();
	}
	bWaitingForEnemy = false;
	RefreshCombatUI();
	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		Controller->SetCombatMessage(FString::Printf(
			TEXT("%s 已结算。轮到你行动，敌人的下一意图已经公开。"),
			*Intent.DisplayName.ToString()), true);
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
					bIsAttack && !bWeakConsumedForAttack);
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
			ActiveCardCombat->DrawCards(Effect.Magnitude);
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
		default:
			break;
		}
	}
}

int32 AWorldWalkerGameModeBase::ResolveDamageAgainstEnemy(
	const int32 BaseDamage,
	const bool bIsAttack,
	const bool bConsumeWeak)
{
	int32 Damage = FMath::Max(0, BaseDamage);
	if (bIsAttack)
	{
		Damage += PlayerFantasyState.Strength;
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
	return EnemyFantasyState.AbsorbDamage(Damage);
}

int32 AWorldWalkerGameModeBase::ResolveDamageAgainstPlayer(
	const int32 BaseDamage,
	const bool bConsumeWeak,
	bool& bPerfectBlock)
{
	int32 Damage = FMath::Max(0, BaseDamage) + EnemyFantasyState.Strength;
	if (bConsumeWeak && EnemyFantasyState.Weak > 0)
	{
		Damage = FMath::FloorToInt(static_cast<float>(Damage) * 0.75f);
		--EnemyFantasyState.Weak;
	}
	if (Damage > 0 && PlayerFantasyState.Exposed > 0)
	{
		Damage = FMath::FloorToInt(static_cast<float>(Damage) * 1.5f);
		--PlayerFantasyState.Exposed;
	}

	const int32 HealthDamage = ActiveCardCombat->AbsorbIncomingDamage(Damage);
	bPerfectBlock = Damage > 0 && HealthDamage == 0;
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
				bool bPerfectBlock = false;
				const int32 HealthDamage = ResolveDamageAgainstPlayer(
					Effect.Magnitude,
					!bWeakConsumed,
					bPerfectBlock);
				ActivePlayer->GetCombatantComponent()->ReceiveDamage(HealthDamage);
				if (HealthDamage > 0)
				{
					ActivePlayer->PlayFantasyHitReactionAnimation();
				}
				bWeakConsumed = true;
				if (bPerfectBlock)
				{
					ActiveCardCombat->AddValor(1);
				}
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
		default:
			break;
		}
	}
}

FString AWorldWalkerGameModeBase::BuildNextIntentText() const
{
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
	GetWorldTimerManager().ClearTimer(EnemyTurnTimer);
	RefreshCombatUI();

	if (AWorldWalkerPlayerController* Controller = GetWorldWalkerController())
	{
		Controller->ShowCombatResult(bPlayerWon);
	}

	if (bPlayerWon && ActiveEnemy)
	{
		ActiveEnemy->PlayIntentAnimation(EWorldWalkerEnemyAnimationCue::Death);
		ActiveEnemy->SetActorEnableCollision(false);
	}
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

			switch (Card->School)
			{
			case ECardSchool::Steel:
				CardSchoolTints.Add(FLinearColor(0.42f, 0.49f, 0.58f, 1.0f));
				break;
			case ECardSchool::Faith:
				CardSchoolTints.Add(FLinearColor(0.82f, 0.60f, 0.16f, 1.0f));
				break;
			case ECardSchool::Arcane:
				CardSchoolTints.Add(FLinearColor(0.31f, 0.16f, 0.68f, 1.0f));
				break;
			case ECardSchool::None:
			default:
				CardSchoolTints.Add(FLinearColor(0.55f, 0.14f, 0.10f, 1.0f));
				break;
			}
		}

		Controller->RefreshCombat(
			PlayerCombatant->GetCurrentHealth(),
			PlayerCombatant->GetMaxHealth(),
			EnemyCombatant->GetCurrentHealth(),
			EnemyCombatant->GetMaxHealth(),
			ActiveCardCombat->GetCurrentEnergy(),
			ActiveCardCombat->GetMaxEnergy(),
			ActiveCardCombat->GetCurrentBlock(),
			ActiveCardCombat->GetCurrentValor(),
			ActiveCardCombat->GetMaxValor(),
			ActiveCardCombat->GetSchoolSummary(),
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
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	UGameplayStatics::OpenLevel(this, FName(*LevelName));
}
