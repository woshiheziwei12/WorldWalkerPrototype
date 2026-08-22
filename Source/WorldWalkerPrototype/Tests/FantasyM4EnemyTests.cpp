#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/Fantasy/FantasyEnemyDeckRuntime.h"
#include "Cards/Fantasy/FantasyEnemyDefinition.h"

namespace
{
	const FString M4EnemyRoot(TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/Data/Enemies"));
	const TArray<FString> M4NormalIds = {
		TEXT("ChurchPenitent"), TEXT("WanderingGhost"), TEXT("Gargoyle"),
		TEXT("MagicMirror"), TEXT("AlchemicalConstruct"), TEXT("FallenCleric")};
	const TArray<FString> M4EliteIds = {
		TEXT("GiantSpiderMatriarch"), TEXT("BlackForestHunter"),
		TEXT("BlackthornCrossbowman"), TEXT("GraveyardGuard")};
	const TArray<FString> M4AllNormalIds = {
		TEXT("DrowsyBat"), TEXT("MagicApprentice"), TEXT("VillageGuard"), TEXT("Hypnotist"),
		TEXT("Scarecrow"), TEXT("FortuneTeller"), TEXT("DragonWhelp"), TEXT("HeadlessKnight"),
		TEXT("ForestWolf"), TEXT("PoisonSpider"), TEXT("Treant"), TEXT("TavernDrunk"),
		TEXT("RangerHunter"), TEXT("WitchAcolyte"), TEXT("ChurchPenitent"), TEXT("WanderingGhost"),
		TEXT("Gargoyle"), TEXT("MagicMirror"), TEXT("AlchemicalConstruct"), TEXT("FallenCleric")};
	const TArray<FString> M4AllEliteIds = {
		TEXT("ScarecrowElite"), TEXT("FortuneTellerElite"), TEXT("GiantSpiderMatriarch"),
		TEXT("BlackForestHunter"), TEXT("BlackthornCrossbowman"), TEXT("GraveyardGuard")};
	const TArray<FString> M4AllBossIds = {
		TEXT("HeadlessKnightBoss"), TEXT("WolfKing"), TEXT("BlackForestWitch"),
		TEXT("MagicMirrorGuardian")};

	UFantasyEnemyDefinition* LoadM4Enemy(const FString& EnemyId)
	{
		const FString AssetName = FString::Printf(TEXT("DA_Enemy_%s"), *EnemyId);
		return LoadObject<UFantasyEnemyDefinition>(nullptr, *FString::Printf(
			TEXT("%s/%s.%s"), *M4EnemyRoot, *AssetName, *AssetName));
	}

	int32 SimulateM4Deck(UFantasyEnemyDefinition* Enemy, const int32 Seed)
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
	FFantasyM4EnemyMechanismTest,
	"WorldWalker.W01.M4.EnemyMechanisms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFantasyM4EnemyMechanismTest::RunTest(const FString& Parameters)
{
	TSet<FName> PassiveIds;
	for (const FString& EnemyId : M4NormalIds)
	{
		UFantasyEnemyDefinition* Enemy = LoadM4Enemy(EnemyId);
		TestNotNull(*FString::Printf(TEXT("M4 normal loads: %s"), *EnemyId), Enemy);
		if (!Enemy) continue;
		TestEqual(*FString::Printf(TEXT("M4 normal tier: %s"), *EnemyId),
			Enemy->EncounterTier, EFantasyEncounterTier::Normal);
		TestTrue(*FString::Printf(TEXT("M4 normal has generic mechanics: %s"), *EnemyId),
			!Enemy->Mechanics.IsEmpty());
		TestFalse(*FString::Printf(TEXT("M4 passive is distinct: %s"), *EnemyId),
			PassiveIds.Contains(Enemy->PassiveId));
		PassiveIds.Add(Enemy->PassiveId);
	}
	for (const FString& EnemyId : M4EliteIds)
	{
		UFantasyEnemyDefinition* Enemy = LoadM4Enemy(EnemyId);
		TestNotNull(*FString::Printf(TEXT("M4 elite loads: %s"), *EnemyId), Enemy);
		if (!Enemy) continue;
		TestEqual(*FString::Printf(TEXT("M4 elite tier: %s"), *EnemyId),
			Enemy->EncounterTier, EFantasyEncounterTier::Elite);
		TestTrue(*FString::Printf(TEXT("M4 elite has generic mechanics: %s"), *EnemyId),
			!Enemy->Mechanics.IsEmpty());
		TestFalse(*FString::Printf(TEXT("M4 passive is distinct: %s"), *EnemyId),
			PassiveIds.Contains(Enemy->PassiveId));
		PassiveIds.Add(Enemy->PassiveId);
	}
	TestEqual(TEXT("All ten M4 encounters have distinct passive contracts"), PassiveIds.Num(), 10);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFantasyM4EnemySimulationTest,
	"WorldWalker.W01.M4.EnemySimulation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFantasyM4EnemySimulationTest::RunTest(const FString& Parameters)
{
	for (const FString& EnemyId : M4AllNormalIds)
	{
		UFantasyEnemyDefinition* Enemy = LoadM4Enemy(EnemyId);
		for (int32 Sample = 0; Sample < 3; ++Sample)
		{
			const int32 Seed = 20260823 + Sample;
			const int32 First = SimulateM4Deck(Enemy, Seed);
			TestTrue(*FString::Printf(TEXT("Normal simulation plays cards: %s/%d"), *EnemyId, Sample), First > 0);
			TestEqual(*FString::Printf(TEXT("Normal simulation deterministic: %s/%d"), *EnemyId, Sample),
				SimulateM4Deck(Enemy, Seed), First);
		}
	}
	for (const FString& EnemyId : M4AllEliteIds)
	{
		UFantasyEnemyDefinition* Enemy = LoadM4Enemy(EnemyId);
		for (int32 Sample = 0; Sample < 8; ++Sample)
		{
			const int32 Seed = 20260901 + Sample;
			const int32 First = SimulateM4Deck(Enemy, Seed);
			TestTrue(*FString::Printf(TEXT("Elite simulation plays cards: %s/%d"), *EnemyId, Sample), First > 0);
			TestEqual(*FString::Printf(TEXT("Elite simulation deterministic: %s/%d"), *EnemyId, Sample),
				SimulateM4Deck(Enemy, Seed), First);
		}
	}
	for (const FString& EnemyId : M4AllBossIds)
	{
		UFantasyEnemyDefinition* Enemy = LoadM4Enemy(EnemyId);
		TestNotNull(*FString::Printf(TEXT("M4 boss loads: %s"), *EnemyId), Enemy);
		if (!Enemy) continue;
		TestFalse(*FString::Printf(TEXT("Boss passive contract is set: %s"), *EnemyId), Enemy->PassiveId.IsNone());
		for (int32 Sample = 0; Sample < 8; ++Sample)
		{
			const int32 Seed = 20261001 + Sample;
			const int32 First = SimulateM4Deck(Enemy, Seed);
			TestTrue(*FString::Printf(TEXT("Boss simulation plays cards: %s/%d"), *EnemyId, Sample), First > 0);
			TestEqual(*FString::Printf(TEXT("Boss simulation deterministic: %s/%d"), *EnemyId, Sample),
				SimulateM4Deck(Enemy, Seed), First);
		}
	}
	return true;
}

#endif
