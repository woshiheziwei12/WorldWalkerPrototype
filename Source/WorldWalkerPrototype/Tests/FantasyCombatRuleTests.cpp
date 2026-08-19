#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/Fantasy/FantasyEnemyDeckRuntime.h"
#include "Cards/Fantasy/FantasyEnemyDefinition.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFantasyStrengthScaledPreviewTest,
	"WorldWalker.W01.Combat.StrengthScaledDamagePreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFantasyStrengthScaledPreviewTest::RunTest(const FString& Parameters)
{
	UCardDefinition* Repentance = NewObject<UCardDefinition>();
	Repentance->CardId = TEXT("Test_Repentance");
	Repentance->DisplayName = FText::FromString(TEXT("忏悔"));
	Repentance->CardType = ECardType::Spell;
	Repentance->bUseClassicResources = true;

	FFantasyCombatEffectSpec Damage;
	Damage.EffectType = EFantasyCombatEffectType::Damage;
	Damage.Target = EFantasyCombatTarget::Opponent;
	Damage.Magnitude = 4;
	Damage.bPiercing = true;
	Damage.bScalesWithStrength = true;
	Repentance->Effects.Add(Damage);

	UFantasyEnemyDefinition* Enemy = NewObject<UFantasyEnemyDefinition>();
	Enemy->EnemyId = TEXT("Test_HeadlessKnight");
	Enemy->MaxHandSize = 1;
	Enemy->StartingMana = 2;
	Enemy->CardsPerTurn = 1;
	FFantasyEnemyDeckEntry Entry;
	Entry.Card = Repentance;
	Entry.Copies = 1;
	Enemy->Deck.Add(Entry);

	UFantasyEnemyDeckRuntime* Runtime = NewObject<UFantasyEnemyDeckRuntime>();
	TestTrue(TEXT("Enemy deck initializes"), Runtime->Initialize(Enemy));
	Runtime->StartTurn();
	const FString Preview = Runtime->BuildPreview(2);
	TestTrue(TEXT("Preview includes revival Strength on Spell damage"), Preview.Contains(TEXT("6 点穿刺伤害")));

	const UCardDefinition* RepentanceAsset = LoadObject<UCardDefinition>(
		nullptr,
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/Data/Cards/DA_EnemyCard_Repentance.DA_EnemyCard_Repentance"));
	TestNotNull(TEXT("Generated Repentance asset loads"), RepentanceAsset);
	if (RepentanceAsset)
	{
		TestTrue(TEXT("Generated Repentance has a damage effect"), !RepentanceAsset->Effects.IsEmpty());
		if (!RepentanceAsset->Effects.IsEmpty())
		{
			TestTrue(
				TEXT("Generated Repentance damage scales with Strength"),
				RepentanceAsset->Effects[0].bScalesWithStrength);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFantasyCounterDrawCapacityTest,
	"WorldWalker.W01.Combat.CounterDrawIgnoresTurnHandLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFantasyCounterDrawCapacityTest::RunTest(const FString& Parameters)
{
	UCardDefinition* Filler = NewObject<UCardDefinition>();
	Filler->CardId = TEXT("Test_Filler");
	Filler->CardType = ECardType::Attack;
	Filler->bUseClassicResources = true;

	UFantasyEnemyDefinition* Enemy = NewObject<UFantasyEnemyDefinition>();
	Enemy->EnemyId = TEXT("Test_CounterDraw");
	Enemy->MaxHandSize = 3;
	Enemy->CardsPerTurn = 1;
	FFantasyEnemyDeckEntry Entry;
	Entry.Card = Filler;
	Entry.Copies = 4;
	Enemy->Deck.Add(Entry);

	UFantasyEnemyDeckRuntime* Runtime = NewObject<UFantasyEnemyDeckRuntime>();
	TestTrue(TEXT("Enemy deck initializes"), Runtime->Initialize(Enemy));
	Runtime->StartTurn();
	TestEqual(TEXT("Normal turn draw fills the hand cap"), Runtime->GetHandCount(), 3);
	TestEqual(TEXT("Delayed Counter draws one card"), Runtime->DrawCardsIgnoringHandLimit(1), 1);
	TestEqual(TEXT("Counter draw may exceed the turn hand cap"), Runtime->GetHandCount(), 4);
	return true;
}

#endif
