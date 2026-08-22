#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/Fantasy/FantasyCardProgressionSubsystem.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"

namespace
{
	const FString M5CardRoot(TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/Data/Cards"));

	TArray<UCardDefinition*> LoadM5Cards()
	{
		UAssetManager& Manager = UAssetManager::Get();
		Manager.ScanPathsForPrimaryAssets(UCardDefinition::PrimaryAssetType, {M5CardRoot},
			UCardDefinition::StaticClass(), false, false, true);
		TArray<FPrimaryAssetId> Ids;
		Manager.GetPrimaryAssetIdList(UCardDefinition::PrimaryAssetType, Ids);
		TArray<UCardDefinition*> Cards;
		for (const FPrimaryAssetId& Id : Ids)
		{
			const FSoftObjectPath Path = Manager.GetPrimaryAssetPath(Id);
			if (Path.ToString().StartsWith(M5CardRoot + TEXT("/")))
			{
				if (UCardDefinition* Card = Cast<UCardDefinition>(Path.TryLoad())) Cards.Add(Card);
			}
		}
		return Cards;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFantasyM5ProfessionContentTest,
	"WorldWalker.W01.M5.ProfessionContent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFantasyM5ProfessionContentTest::RunTest(const FString& Parameters)
{
	const TArray<UCardDefinition*> Cards = LoadM5Cards();
	for (const EFantasyPlayerProfession Profession : {
		EFantasyPlayerProfession::Ranger, EFantasyPlayerProfession::Nun})
	{
		int32 StarterCopies = 0;
		int32 RewardBases = 0;
		TSet<FName> Archetypes;
		for (const UCardDefinition* Card : Cards)
		{
			if (!Card || Card->Profession != Profession) continue;
			StarterCopies += Card->StartingDeckCopies;
			if (Card->bRewardEligible && Card->UpgradeLevel == 0)
			{
				++RewardBases;
				const UCardDefinition* Upgrade = Card->UpgradeCard.LoadSynchronous();
				TestNotNull(*FString::Printf(TEXT("Reward has upgrade: %s"), *Card->CardId.ToString()), Upgrade);
				for (const FName Tag : Card->BuildTags)
				{
					if (Tag.ToString().StartsWith(TEXT("Archetype."))) Archetypes.Add(Tag);
				}
			}
		}
		TestEqual(TEXT("Profession starter deck has ten copies"), StarterCopies, 10);
		TestEqual(TEXT("Profession has twenty-four base rewards"), RewardBases, 24);
		TestEqual(TEXT("Profession has three authored archetypes"), Archetypes.Num(), 3);
	}

	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UFantasyCardProgressionSubsystem* Progression = NewObject<UFantasyCardProgressionSubsystem>(GameInstance);
	TestTrue(TEXT("Ranger can be selected"), Progression->SelectProfession(EFantasyPlayerProfession::Ranger));
	TestEqual(TEXT("Ranger display name"), Progression->GetProfessionDisplayName(), FString(TEXT("游侠")));
	Progression->ResetRun();
	TestTrue(TEXT("Nun can be selected"), Progression->SelectProfession(EFantasyPlayerProfession::Nun));
	TestEqual(TEXT("Nun display name"), Progression->GetProfessionDisplayName(), FString(TEXT("修女")));
	return true;
}

#endif
