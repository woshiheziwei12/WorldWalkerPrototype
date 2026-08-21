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

	UGameInstance* SeedTestGameInstance = NewObject<UGameInstance>();
	UFantasyCardProgressionSubsystem* FirstStreams =
		NewObject<UFantasyCardProgressionSubsystem>(SeedTestGameInstance);
	UFantasyCardProgressionSubsystem* ReplayStreams =
		NewObject<UFantasyCardProgressionSubsystem>(SeedTestGameInstance);
	UFantasyCardProgressionSubsystem* HardStreams =
		NewObject<UFantasyCardProgressionSubsystem>(SeedTestGameInstance);
	FirstStreams->ConfigureRun(314159, TEXT("W01-M1-test"));
	ReplayStreams->ConfigureRun(314159, TEXT("W01-M1-test"));
	HardStreams->ConfigureRun(
		314159,
		TEXT("W01-M1-test"),
		EFantasyRunDifficulty::Hard);
	const int32 FirstPlayerBattle = FirstStreams->ConsumeDeterministicSeed(TEXT("PlayerBattle"));
	const int32 SecondPlayerBattle = FirstStreams->ConsumeDeterministicSeed(TEXT("PlayerBattle"));
	TestEqual(
		TEXT("Named stream sequence zero replays"),
		ReplayStreams->ConsumeDeterministicSeed(TEXT("PlayerBattle")),
		FirstPlayerBattle);
	TestEqual(
		TEXT("Named stream sequence one replays"),
		ReplayStreams->ConsumeDeterministicSeed(TEXT("PlayerBattle")),
		SecondPlayerBattle);
	TestNotEqual(
		TEXT("Isolated stream names derive different random seeds"),
		FirstStreams->ConsumeDeterministicSeed(TEXT("RewardOffer")),
		FirstPlayerBattle);
	TestNotEqual(
		TEXT("Difficulty participates in deterministic random seeds"),
		HardStreams->ConsumeDeterministicSeed(TEXT("PlayerBattle")),
		FirstPlayerBattle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFantasyRunStateContractTest,
	"WorldWalker.W01.Run.StateContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFantasyRunStateContractTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UFantasyCardProgressionSubsystem* Progression =
		NewObject<UFantasyCardProgressionSubsystem>(GameInstance);
	TestFalse(
		TEXT("Run configuration rejects an invalid difficulty enum"),
		Progression->ConfigureRun(
			777,
			TEXT("W01-M1-state-test"),
			static_cast<EFantasyRunDifficulty>(255)));
	TestTrue(
		TEXT("Run configuration accepts explicit difficulty"),
		Progression->ConfigureRun(
			777,
			TEXT("W01-M1-state-test"),
			EFantasyRunDifficulty::Hard));
	TestTrue(
		TEXT("Profession can be selected before the run starts"),
		Progression->SelectProfession(EFantasyPlayerProfession::Mage));
	Progression->EnsureRunStarted();
	TestEqual(TEXT("Difficulty is retained"), Progression->GetDifficulty(), EFantasyRunDifficulty::Hard);
	TestFalse(TEXT("Run ID is assigned"), Progression->GetRunId().IsEmpty());
	TestFalse(TEXT("Build version is assigned"), Progression->GetBuildVersion().IsEmpty());
	TestEqual(TEXT("Run begins without an end reason"), Progression->GetEndReason(), EFantasyRunEndReason::None);

	UCardDefinition* BaseCard = NewObject<UCardDefinition>();
	BaseCard->CardId = TEXT("Test_State_Base");
	BaseCard->CardSetId = TEXT("W01_EasternHorror");
	BaseCard->Profession = EFantasyPlayerProfession::Mage;
	BaseCard->UpgradeLevel = 0;
	UCardDefinition* UpgradeCard = NewObject<UCardDefinition>();
	UpgradeCard->CardId = TEXT("Test_State_Upgrade");
	UpgradeCard->CardSetId = BaseCard->CardSetId;
	UpgradeCard->Profession = BaseCard->Profession;
	UpgradeCard->UpgradeLevel = 1;
	BaseCard->UpgradeCard = UpgradeCard;
	UCardDefinition* ForeignProfessionCard = NewObject<UCardDefinition>();
	ForeignProfessionCard->CardId = TEXT("Test_State_Foreign");
	ForeignProfessionCard->CardSetId = BaseCard->CardSetId;
	ForeignProfessionCard->Profession = EFantasyPlayerProfession::Knight;
	TestFalse(
		TEXT("A card from another profession is rejected"),
		Progression->GrantCard(ForeignProfessionCard));

	TArray<UCardDefinition*> InitialDeck = {BaseCard};
	Progression->ReplaceDeckSnapshot(InitialDeck);
	TestTrue(TEXT("A valid reward card is persisted"), Progression->GrantCard(BaseCard));
	TestEqual(TEXT("Reward updates the base deck tuple"), Progression->GetRunDeckCopies(BaseCard->CardId, 0), 2);
	TestTrue(TEXT("One exact base copy can be removed"), Progression->RemoveCardCopy(BaseCard->CardId, 0));
	TestEqual(TEXT("Deletion count is retained"), Progression->GetDeletionCount(), 1);
	TestTrue(TEXT("One exact base copy can be upgraded"), Progression->UpgradeCardCopy(BaseCard));
	TestEqual(TEXT("Base tuple is consumed by upgrade"), Progression->GetRunDeckCopies(BaseCard->CardId, 0), 0);
	TestEqual(TEXT("Level-one tuple is added by upgrade"), Progression->GetRunDeckCopies(UpgradeCard->CardId, 1), 1);
	TestFalse(TEXT("A missing base copy cannot be upgraded twice"), Progression->UpgradeCardCopy(BaseCard));

	TestEqual(TEXT("Gold grant applies"), Progression->AddGold(120), 120);
	TestTrue(TEXT("Affordable gold spend succeeds"), Progression->SpendGold(45));
	TestFalse(TEXT("Unaffordable gold spend is rejected"), Progression->SpendGold(100));
	TestEqual(TEXT("Gold balance is retained"), Progression->GetGold(), 75);
	TestTrue(TEXT("A blessing is retained"), Progression->GrantBlessing(TEXT("Test_Blessing")));
	TestFalse(TEXT("A duplicate blessing is rejected"), Progression->GrantBlessing(TEXT("Test_Blessing")));

	Progression->RecordRewardOffer(TEXT("CardA|CardB|CardC"));
	Progression->RecordRewardSelection(TEXT("CardB"), false, 1);
	Progression->RecordEventOffer(TEXT("Test_Event"), TEXT("Left|Right"));
	Progression->RecordEventSelection(TEXT("Test_Event"), 0);
	Progression->RecordBattleStarted(TEXT("Test_Enemy"), 1);
	Progression->RecordEnemyDefeated(TEXT("Test_Enemy"));
	TestTrue(
		TEXT("Decision history retains exact reward selection index"),
		Progression->GetDecisionHistory().ContainsByPredicate(
			[](const FFantasyRunDecisionRecord& Record)
			{
				return Record.EventType == TEXT("RewardSelected")
					&& Record.SelectionId == TEXT("CardB")
					&& Record.SelectionIndex == 1;
			}));
	TestEqual(TEXT("Defeated enemy history is retained"), Progression->GetDefeatedEnemyIds().Num(), 1);
	TestEqual(TEXT("Last enemy is retained"), Progression->GetLastEnemyId(), FName(TEXT("Test_Enemy")));

	Progression->EndRun(EFantasyRunEndReason::Defeat, TEXT("Test_Enemy"));
	Progression->EndRun(EFantasyRunEndReason::Completed);
	TestEqual(TEXT("The first terminal reason is immutable"), Progression->GetEndReason(), EFantasyRunEndReason::Defeat);
	Progression->ResetRun();
	TestEqual(TEXT("Reset clears gold"), Progression->GetGold(), 0);
	TestTrue(TEXT("Reset clears the deck snapshot"), Progression->GetRunDeck().IsEmpty());
	TestTrue(TEXT("Reset clears blessings"), Progression->GetBlessings().IsEmpty());
	TestTrue(TEXT("Reset clears decision history"), Progression->GetDecisionHistory().IsEmpty());
	TestTrue(TEXT("Reset clears defeated enemies"), Progression->GetDefeatedEnemyIds().IsEmpty());
	TestEqual(TEXT("Reset clears the terminal reason"), Progression->GetEndReason(), EFantasyRunEndReason::None);
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
