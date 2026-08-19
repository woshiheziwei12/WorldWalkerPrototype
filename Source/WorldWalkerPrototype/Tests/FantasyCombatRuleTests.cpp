#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/Fantasy/FantasyEnemyDeckRuntime.h"
#include "Cards/Fantasy/FantasyEnemyDefinition.h"
#include "Cards/Fantasy/FantasyCardProgressionSubsystem.h"
#include "Cards/Fantasy/FantasyRunTypes.h"
#include "Engine/GameInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFantasyDeterministicRouteTest,
	"WorldWalker.W01.Run.DeterministicRoutes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFantasyDeterministicRouteTest::RunTest(const FString& Parameters)
{
	auto BuildSignatures = [](const int32 RouteSeed)
	{
		TArray<FString> Signatures;
		UGameInstance* GameInstance = NewObject<UGameInstance>();
		UFantasyCardProgressionSubsystem* Progression =
			NewObject<UFantasyCardProgressionSubsystem>(GameInstance);
		Progression->ConfigureRun(RouteSeed, TEXT("W01-M1-test"));
		Progression->SelectProfession(EFantasyPlayerProfession::Mage);
		Progression->EnsureRunStarted();
		for (int32 Depth = 0; Depth < Progression->GetTotalRouteDepths(); ++Depth)
		{
			Signatures.Add(Progression->BuildRouteChoiceSignature());
			FFantasyRouteNodeChoice Selected;
			if (!Progression->SelectRouteChoice(0, Selected)
				|| !Progression->CompleteActiveNode())
			{
				Signatures.Add(TEXT("INVALID"));
				break;
			}
		}
		return Signatures;
	};

	const TArray<FString> FirstRun = BuildSignatures(314159);
	const TArray<FString> Replay = BuildSignatures(314159);
	const TArray<FString> DifferentSeed = BuildSignatures(271828);
	TestEqual(TEXT("A complete chapter exposes six route layers"), FirstRun.Num(), 6);
	TestTrue(TEXT("Same Seed and content version reproduce all route layers"), Replay == FirstRun);
	TestTrue(TEXT("A different Seed changes the route ordering"), DifferentSeed != FirstRun);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFantasyFastCombatTimingTest,
	"WorldWalker.W01.Combat.FastTiming",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFantasyFastCombatTimingTest::RunTest(const FString& Parameters)
{
	const FFantasyCombatTiming NormalTiming = FFantasyCombatTiming::ForAutomationMode(false);
	TestFalse(TEXT("Normal timing is not marked fast"), NormalTiming.bFastCombat);
	TestEqual(TEXT("Normal enemy turn start delay is unchanged"), NormalTiming.EnemyTurnStartDelay, 0.7f);
	TestEqual(
		TEXT("Normal enemy card presentation delay is unchanged"),
		NormalTiming.EnemyCardPresentationDelay,
		1.05f);

	const FFantasyCombatTiming FastTiming = FFantasyCombatTiming::ForAutomationMode(true);
	TestTrue(TEXT("Fast timing is marked fast"), FastTiming.bFastCombat);
	TestEqual(TEXT("Fast enemy turn start delay"), FastTiming.EnemyTurnStartDelay, 0.01f);
	TestEqual(TEXT("Fast enemy card presentation delay"), FastTiming.EnemyCardPresentationDelay, 0.01f);
	TestTrue(
		TEXT("Fast timer delays stay positive"),
		FastTiming.EnemyTurnStartDelay > 0.0f
			&& FastTiming.EnemyCardPresentationDelay > 0.0f);
	return true;
}

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
