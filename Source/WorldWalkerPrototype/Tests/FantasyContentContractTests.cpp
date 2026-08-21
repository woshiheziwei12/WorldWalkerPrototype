#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/Fantasy/FantasyEnemyDefinition.h"
#include "Engine/AssetManager.h"

namespace
{
	const FString W01CardPath(TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/Data/Cards"));
	const FString W01EnemyPath(TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/Data/Enemies"));

	TArray<UCardDefinition*> LoadW01Cards()
	{
		UAssetManager& AssetManager = UAssetManager::Get();
		AssetManager.ScanPathsForPrimaryAssets(
			UCardDefinition::PrimaryAssetType,
			{W01CardPath},
			UCardDefinition::StaticClass(),
			false,
			false,
			true);

		TArray<FPrimaryAssetId> AssetIds;
		AssetManager.GetPrimaryAssetIdList(UCardDefinition::PrimaryAssetType, AssetIds);
		TArray<UCardDefinition*> Cards;
		for (const FPrimaryAssetId& AssetId : AssetIds)
		{
			const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(AssetId);
			if (AssetPath.ToString().StartsWith(W01CardPath + TEXT("/")))
			{
				if (UCardDefinition* Card = Cast<UCardDefinition>(AssetPath.TryLoad()))
				{
					Cards.Add(Card);
				}
			}
		}
		return Cards;
	}

	TArray<UFantasyEnemyDefinition*> LoadActiveW01Enemies()
	{
		const TArray<FString> EnemyIds = {
			TEXT("DrowsyBat"), TEXT("MagicApprentice"),
			TEXT("VillageGuard"), TEXT("Hypnotist"),
			TEXT("Scarecrow"), TEXT("FortuneTeller"),
			TEXT("DragonWhelp"), TEXT("HeadlessKnight"),
			TEXT("ScarecrowElite"), TEXT("FortuneTellerElite"),
			TEXT("HeadlessKnightBoss")};

		TArray<UFantasyEnemyDefinition*> Enemies;
		for (const FString& EnemyId : EnemyIds)
		{
			const FString AssetName = FString::Printf(TEXT("DA_Enemy_%s"), *EnemyId);
			const FString ObjectPath = FString::Printf(
				TEXT("%s/%s.%s"), *W01EnemyPath, *AssetName, *AssetName);
			if (UFantasyEnemyDefinition* Enemy = LoadObject<UFantasyEnemyDefinition>(nullptr, *ObjectPath))
			{
				Enemies.Add(Enemy);
			}
		}
		return Enemies;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFantasyCardContentContractsTest,
	"WorldWalker.W01.Content.CardContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFantasyCardContentContractsTest::RunTest(const FString& Parameters)
{
	const TArray<UCardDefinition*> Cards = LoadW01Cards();
	TestEqual(TEXT("W01 contains the generated 52 card definitions"), Cards.Num(), 52);

	TSet<FName> CardIds;
	TSet<FSoftObjectPath> ClaimedUpgradeTargets;
	for (const UCardDefinition* Card : Cards)
	{
		const FString Context = Card ? Card->GetPathName() : TEXT("Null card");
		TestNotNull(*FString::Printf(TEXT("Card asset loads: %s"), *Context), Card);
		if (!Card)
		{
			continue;
		}

		TestFalse(*FString::Printf(TEXT("CardId is set: %s"), *Context), Card->CardId.IsNone());
		TestFalse(*FString::Printf(TEXT("CardId is unique: %s"), *Context), CardIds.Contains(Card->CardId));
		CardIds.Add(Card->CardId);
		TestFalse(*FString::Printf(TEXT("DisplayName is set: %s"), *Context), Card->DisplayName.IsEmpty());
		TestFalse(*FString::Printf(TEXT("Description is set: %s"), *Context), Card->Description.IsEmpty());
		TestTrue(*FString::Printf(TEXT("BuildTags are set: %s"), *Context), !Card->BuildTags.IsEmpty());
		TestTrue(*FString::Printf(TEXT("Starting copies are non-negative: %s"), *Context), Card->StartingDeckCopies >= 0);
		TestTrue(*FString::Printf(TEXT("Upgrade level is supported: %s"), *Context), Card->UpgradeLevel == 0 || Card->UpgradeLevel == 1);

		const bool bEnemyCard = Card->CardSetId == TEXT("W01_Enemy");
		TestEqual(
			*FString::Printf(TEXT("Enemy rarity matches card set: %s"), *Context),
			Card->Rarity == EFantasyCardRarity::Enemy,
			bEnemyCard);
		if (!bEnemyCard)
		{
			TestTrue(*FString::Printf(TEXT("Player card has a profession: %s"), *Context), Card->Profession != EFantasyPlayerProfession::None);
			if (Card->StartingDeckCopies > 0)
			{
				TestEqual(*FString::Printf(TEXT("Starter copy uses starter rarity: %s"), *Context), Card->Rarity, EFantasyCardRarity::Starter);
			}
			if (Card->bRewardEligible)
			{
				TestTrue(
					*FString::Printf(TEXT("Reward rarity is progression-safe: %s"), *Context),
					Card->Rarity == EFantasyCardRarity::Common
						|| Card->Rarity == EFantasyCardRarity::Uncommon
						|| Card->Rarity == EFantasyCardRarity::Rare);
			}
		}

		if (!Card->UpgradeCard.IsNull())
		{
			const FSoftObjectPath UpgradePath = Card->UpgradeCard.ToSoftObjectPath();
			TestFalse(*FString::Printf(TEXT("Upgrade target is unique: %s"), *Context), ClaimedUpgradeTargets.Contains(UpgradePath));
			ClaimedUpgradeTargets.Add(UpgradePath);
			const UCardDefinition* Upgrade = Card->UpgradeCard.LoadSynchronous();
			TestNotNull(*FString::Printf(TEXT("Upgrade target loads: %s"), *Context), Upgrade);
			if (Upgrade)
			{
				TestNotEqual(*FString::Printf(TEXT("Upgrade is not self: %s"), *Context), Upgrade, Card);
				TestEqual(*FString::Printf(TEXT("Upgrade keeps card set: %s"), *Context), Upgrade->CardSetId, Card->CardSetId);
				TestEqual(*FString::Printf(TEXT("Upgrade keeps profession: %s"), *Context), Upgrade->Profession, Card->Profession);
				TestEqual(*FString::Printf(TEXT("Upgrade advances one level: %s"), *Context), Upgrade->UpgradeLevel, Card->UpgradeLevel + 1);
				TestTrue(*FString::Printf(TEXT("Upgrade chain stops at level one: %s"), *Context), Upgrade->UpgradeCard.IsNull());
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFantasyEnemyContentContractsTest,
	"WorldWalker.W01.Content.EnemyContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFantasyEnemyContentContractsTest::RunTest(const FString& Parameters)
{
	const TArray<UFantasyEnemyDefinition*> Enemies = LoadActiveW01Enemies();
	TestEqual(TEXT("All 11 active W01 enemy definitions load"), Enemies.Num(), 11);

	TSet<FName> EnemyIds;
	int32 NormalCount = 0;
	int32 EliteCount = 0;
	int32 BossCount = 0;
	for (const UFantasyEnemyDefinition* Enemy : Enemies)
	{
		const FString Context = Enemy ? Enemy->GetPathName() : TEXT("Null enemy");
		TestNotNull(*FString::Printf(TEXT("Enemy asset loads: %s"), *Context), Enemy);
		if (!Enemy)
		{
			continue;
		}

		TestFalse(*FString::Printf(TEXT("EnemyId is set: %s"), *Context), Enemy->EnemyId.IsNone());
		TestFalse(*FString::Printf(TEXT("EnemyId is unique: %s"), *Context), EnemyIds.Contains(Enemy->EnemyId));
		EnemyIds.Add(Enemy->EnemyId);
		TestFalse(*FString::Printf(TEXT("Family is set: %s"), *Context), Enemy->Family.IsNone());
		TestFalse(*FString::Printf(TEXT("Unlock condition is set: %s"), *Context), Enemy->UnlockCondition.IsNone());
		TestEqual(*FString::Printf(TEXT("Enemy belongs to chapter one: %s"), *Context), Enemy->Chapter, 1);
		TestTrue(*FString::Printf(TEXT("Danger is positive: %s"), *Context), Enemy->DangerRating > 0);
		TestTrue(*FString::Printf(TEXT("Depth range is ordered: %s"), *Context), Enemy->MinDepth >= 0 && Enemy->MaxDepth >= Enemy->MinDepth);
		TestTrue(*FString::Printf(TEXT("Reward weight is positive: %s"), *Context), Enemy->RewardWeight > 0.0f);
		TestTrue(*FString::Printf(TEXT("Enemy has a presentation profile: %s"), *Context), StaticEnum<EFantasyEnemyVisualProfile>()->IsValidEnumValue(static_cast<int64>(Enemy->VisualProfile)));
		TestTrue(*FString::Printf(TEXT("Enemy has a safe fallback intent: %s"), *Context), !Enemy->IntentCycle.IsEmpty());
		TestTrue(*FString::Printf(TEXT("Enemy has a real deck: %s"), *Context), !Enemy->Deck.IsEmpty());

		bool bCanGenerateMana = false;
		bool bHasInitiallyPlayableCard = false;
		for (const FFantasyEnemyDeckEntry& Entry : Enemy->Deck)
		{
			TestNotNull(*FString::Printf(TEXT("Deck card reference is valid: %s"), *Context), Entry.Card.Get());
			TestTrue(*FString::Printf(TEXT("Deck copies are positive: %s"), *Context), Entry.Copies > 0);
			if (!Entry.Card)
			{
				continue;
			}
			TestEqual(*FString::Printf(TEXT("Enemy deck uses enemy cards: %s"), *Context), Entry.Card->CardSetId, FName(TEXT("W01_Enemy")));
			TestTrue(*FString::Printf(TEXT("Action cost is payable each turn: %s"), *Context), Entry.Card->ActionCost <= Enemy->MaxActionPoints);
			bHasInitiallyPlayableCard |= Entry.Card->ActionCost <= Enemy->MaxActionPoints
				&& Entry.Card->ManaCost <= Enemy->StartingMana;
			for (const FFantasyCombatEffectSpec& Effect : Entry.Card->Effects)
			{
				bCanGenerateMana |= Effect.EffectType == EFantasyCombatEffectType::GainMana
					&& Entry.Card->ActionCost <= Enemy->MaxActionPoints
					&& Entry.Card->ManaCost <= Enemy->StartingMana;
			}
		}
		TestTrue(*FString::Printf(TEXT("Deck has an initially playable card: %s"), *Context), bHasInitiallyPlayableCard);
		for (const FFantasyEnemyDeckEntry& Entry : Enemy->Deck)
		{
			if (Entry.Card)
			{
				TestTrue(
					*FString::Printf(TEXT("Mana card cannot remain permanently dead: %s/%s"), *Context, *Entry.Card->CardId.ToString()),
					Entry.Card->ManaCost <= Enemy->StartingMana || bCanGenerateMana);
			}
		}

		switch (Enemy->EncounterTier)
		{
		case EFantasyEncounterTier::Normal:
			++NormalCount;
			TestTrue(*FString::Printf(TEXT("Normal enemy is in depths 0-3: %s"), *Context), Enemy->MaxDepth <= 3);
			break;
		case EFantasyEncounterTier::Elite:
			++EliteCount;
			TestEqual(*FString::Printf(TEXT("Elite is in depth 4: %s"), *Context), Enemy->MinDepth, 4);
			break;
		case EFantasyEncounterTier::Boss:
			++BossCount;
			TestEqual(*FString::Printf(TEXT("Boss is in depth 5: %s"), *Context), Enemy->MinDepth, 5);
			break;
		default:
			AddError(FString::Printf(TEXT("Invalid encounter tier: %s"), *Context));
			break;
		}
		TestEqual(
			*FString::Printf(TEXT("Boss compatibility flag matches tier: %s"), *Context),
			Enemy->bBoss,
			Enemy->EncounterTier == EFantasyEncounterTier::Boss);
	}

	TestEqual(TEXT("Chapter one has eight normal encounters"), NormalCount, 8);
	TestEqual(TEXT("Chapter one has two elite encounters"), EliteCount, 2);
	TestEqual(TEXT("Chapter one has one boss encounter"), BossCount, 1);
	return true;
}

#endif
