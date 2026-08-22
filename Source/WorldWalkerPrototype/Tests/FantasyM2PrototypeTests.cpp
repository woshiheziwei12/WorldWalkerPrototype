#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/Fantasy/FantasyEnemyDeckRuntime.h"
#include "Cards/Fantasy/FantasyEnemyDefinition.h"
#include "Engine/AssetManager.h"

namespace
{
	const FString M2EnemyRoot(TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/Data/Enemies"));
	const FString M2CardRoot(TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/Data/Cards"));
	const TArray<FString> M2EnemyIds = {
		TEXT("ForestWolf"), TEXT("PoisonSpider"), TEXT("Treant"),
		TEXT("TavernDrunk"), TEXT("RangerHunter"), TEXT("WitchAcolyte")};

	UFantasyEnemyDefinition* LoadM2Enemy(const FString& EnemyId)
	{
		const FString AssetName = FString::Printf(TEXT("DA_Enemy_%s"), *EnemyId);
		return LoadObject<UFantasyEnemyDefinition>(nullptr, *FString::Printf(
			TEXT("%s/%s.%s"), *M2EnemyRoot, *AssetName, *AssetName));
	}

	const UCardDefinition* FindDeckCard(
		const UFantasyEnemyDefinition* Enemy,
		const FName CardId)
	{
		if (!Enemy) return nullptr;
		for (const FFantasyEnemyDeckEntry& Entry : Enemy->Deck)
		{
			if (Entry.Card && Entry.Card->CardId == CardId) return Entry.Card;
		}
		return nullptr;
	}

	int32 SimulateEnemyDeck(UFantasyEnemyDefinition* Enemy, const int32 Seed)
	{
		UFantasyEnemyDeckRuntime* Runtime = NewObject<UFantasyEnemyDeckRuntime>();
		if (!Runtime || !Runtime->Initialize(Enemy, Seed)) return 0;
		int32 Played = 0;
		for (int32 Turn = 0; Turn < 12; ++Turn)
		{
			Runtime->StartTurn();
			while (UCardDefinition* Card = Runtime->PlayNextCard())
			{
				++Played;
				for (const FFantasyCombatEffectSpec& Effect : Card->Effects)
				{
					if (Effect.EffectType == EFantasyCombatEffectType::GainMana
						&& Effect.Target == EFantasyCombatTarget::Self)
					{
						Runtime->AddMana(Effect.Magnitude);
					}
				}
				Runtime->FinalizeCard(Card);
			}
		}
		return Played;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFantasyM2MechanicMatrixTest,
	"WorldWalker.W01.M2.MechanicMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFantasyM2MechanicMatrixTest::RunTest(const FString& Parameters)
{
	TMap<FName, UFantasyEnemyDefinition*> Enemies;
	for (const FString& EnemyId : M2EnemyIds)
	{
		UFantasyEnemyDefinition* Enemy = LoadM2Enemy(EnemyId);
		TestNotNull(*FString::Printf(TEXT("M2 prototype loads: %s"), *EnemyId), Enemy);
		if (Enemy) Enemies.Add(FName(*EnemyId), Enemy);
	}

	const UFantasyEnemyDefinition* Wolf = Enemies.FindRef(TEXT("ForestWolf"));
	TestTrue(TEXT("Wolf has generic pursuit rule"), Wolf && Wolf->Mechanics.ContainsByPredicate(
		[](const FFantasyCombatMechanicRule& Rule)
		{
			return Rule.MechanicId == TEXT("ForestWolf.Pursuit")
				&& Rule.Trigger == EFantasyMechanicTrigger::EnemyAttackResolved
				&& Rule.MinActualDamage == 1 && Rule.MaxTargetBlock == 2
				&& Rule.Limit == EFantasyMechanicLimit::OncePerTurn;
		}));

	const UFantasyEnemyDefinition* Spider = Enemies.FindRef(TEXT("PoisonSpider"));
	const bool bSpiderPoisons = Spider && Spider->Deck.ContainsByPredicate(
		[](const FFantasyEnemyDeckEntry& Entry)
		{
			return Entry.Card && Entry.Card->Effects.ContainsByPredicate(
				[](const FFantasyCombatEffectSpec& Effect)
				{
					return Effect.EffectType == EFantasyCombatEffectType::ApplyStatus
						&& Effect.Status == EFantasyCombatStatus::Poison;
				});
		});
	TestTrue(TEXT("Spider poison is authored on cards"), bSpiderPoisons);

	const UFantasyEnemyDefinition* Treant = Enemies.FindRef(TEXT("Treant"));
	TestTrue(TEXT("Treant growth is fire-suppressible"), Treant && Treant->Mechanics.ContainsByPredicate(
		[](const FFantasyCombatMechanicRule& Rule)
		{
			return Rule.RequiredSourceCardTag == TEXT("Element.Fire")
				&& Rule.SuppressTargetMechanicId == TEXT("Treant.Growth")
				&& Rule.SuppressTurns == 1;
		}));

	const UFantasyEnemyDefinition* Drunk = Enemies.FindRef(TEXT("TavernDrunk"));
	const UCardDefinition* DrunkLow = FindDeckCard(Drunk, TEXT("Enemy_DrunkLow"));
	const UCardDefinition* DrunkHigh = FindDeckCard(Drunk, TEXT("Enemy_DrunkHigh"));
	const UCardDefinition* DrunkSpill = FindDeckCard(Drunk, TEXT("Enemy_DrunkSpill"));
	TestTrue(TEXT("Drunk has meaningful high/low variance"), DrunkLow && DrunkHigh
		&& DrunkLow->Effects.Num() > 0 && DrunkHigh->Effects.Num() > 0
		&& DrunkLow->Effects[0].Magnitude == 3 && DrunkHigh->Effects[0].Magnitude == 9);
	TestTrue(TEXT("Drunk spill discards from both sides"), DrunkSpill && DrunkSpill->Effects.Num() == 2
		&& DrunkSpill->Effects[0].Target != DrunkSpill->Effects[1].Target);

	const UFantasyEnemyDefinition* Hunter = Enemies.FindRef(TEXT("RangerHunter"));
	const UCardDefinition* Longbow = FindDeckCard(Hunter, TEXT("Enemy_HunterLongbow"));
	TestTrue(TEXT("Hunter longbow is persistent +2 attack equipment"), Longbow
		&& Longbow->CardType == ECardType::Equipment && Longbow->EquipmentAttackBonus == 2);

	const UFantasyEnemyDefinition* Witch = Enemies.FindRef(TEXT("WitchAcolyte"));
	const UCardDefinition* Hex = FindDeckCard(Witch, TEXT("Enemy_WitchHex"));
	const UCardDefinition* Drain = FindDeckCard(Witch, TEXT("Enemy_WitchDrain"));
	TestTrue(TEXT("Witch curse has a per-battle cap of three"), Hex && Hex->Effects.Num() == 1
		&& Hex->Effects[0].EffectType == EFantasyCombatEffectType::AddTemporaryCard
		&& Hex->Effects[0].PayloadId == TEXT("Mage_HexCurse") && Hex->Effects[0].Limit == 3);
	TestTrue(TEXT("Witch drains two mana"), Drain && Drain->Effects.Num() == 1
		&& Drain->Effects[0].EffectType == EFantasyCombatEffectType::LoseMana
		&& Drain->Effects[0].Magnitude == 2);

	FFantasyCombatRuntimeState State;
	State.AddStatus(EFantasyCombatStatus::Burning, 4);
	State.AddStatus(EFantasyCombatStatus::Chill, 3);
	TestEqual(TEXT("Burning stacks in runtime state"), State.GetStatus(EFantasyCombatStatus::Burning), 4);
	TestEqual(TEXT("Chill stacks in runtime state"), State.GetStatus(EFantasyCombatStatus::Chill), 3);
	State.RemoveStatus(EFantasyCombatStatus::Burning, 99);
	TestEqual(TEXT("Status removal clamps at zero"), State.Burning, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFantasyM2PrototypeSimulationTest,
	"WorldWalker.W01.M2.PrototypeSimulation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFantasyM2PrototypeSimulationTest::RunTest(const FString& Parameters)
{
	for (const FString& EnemyId : M2EnemyIds)
	{
		UFantasyEnemyDefinition* Enemy = LoadM2Enemy(EnemyId);
		const int32 FirstRun = SimulateEnemyDeck(Enemy, 20260822);
		const int32 Replay = SimulateEnemyDeck(Enemy, 20260822);
		TestTrue(*FString::Printf(TEXT("Prototype plays cards over 12 turns: %s"), *EnemyId), FirstRun > 0);
		TestEqual(*FString::Printf(TEXT("Prototype simulation replays deterministically: %s"), *EnemyId),
			Replay, FirstRun);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFantasyM2MageArchetypeTest,
	"WorldWalker.W01.M2.MageArchetypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFantasyM2MageArchetypeTest::RunTest(const FString& Parameters)
{
	UAssetManager& AssetManager = UAssetManager::Get();
	AssetManager.ScanPathsForPrimaryAssets(
		UCardDefinition::PrimaryAssetType, {M2CardRoot}, UCardDefinition::StaticClass(), false, false, true);
	TArray<FPrimaryAssetId> AssetIds;
	AssetManager.GetPrimaryAssetIdList(UCardDefinition::PrimaryAssetType, AssetIds);
	const TArray<FName> Archetypes = {TEXT("Archetype.Fire"), TEXT("Archetype.Frost"), TEXT("Archetype.Arcane")};
	for (const FName Archetype : Archetypes)
	{
		int32 BaseCount = 0;
		for (const FPrimaryAssetId& AssetId : AssetIds)
		{
			UCardDefinition* Card = Cast<UCardDefinition>(AssetManager.GetPrimaryAssetPath(AssetId).TryLoad());
			if (!Card || !Card->GetPathName().StartsWith(M2CardRoot)
				|| Card->Profession != EFantasyPlayerProfession::Mage
				|| Card->UpgradeLevel != 0 || !Card->BuildTags.Contains(Archetype))
			{
				continue;
			}
			++BaseCount;
			TestTrue(*FString::Printf(TEXT("Archetype base is reward eligible: %s"), *Card->CardId.ToString()),
				Card->bRewardEligible);
			const UCardDefinition* Upgrade = Card->UpgradeCard.LoadSynchronous();
			TestNotNull(*FString::Printf(TEXT("Archetype base has upgrade: %s"), *Card->CardId.ToString()), Upgrade);
			if (Upgrade)
			{
				TestNotEqual(*FString::Printf(TEXT("Upgrade changes rules: %s"), *Card->CardId.ToString()),
					Upgrade->BuildRulesText(), Card->BuildRulesText());
			}
		}
		TestEqual(*FString::Printf(TEXT("Exactly six base cards for %s"), *Archetype.ToString()), BaseCount, 6);
	}
	return true;
}

#endif
