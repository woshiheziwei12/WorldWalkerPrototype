#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Cards/Fantasy/FantasyBlessingDefinition.h"
#include "Cards/Fantasy/FantasyCampaignDefinition.h"
#include "Cards/Fantasy/FantasyChapterDefinition.h"
#include "Engine/AssetManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFantasyM3CampaignTest,
	"WorldWalker.W01.M3.EighteenDepthCampaign",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFantasyM3CampaignTest::RunTest(const FString& Parameters)
{
	const TArray<FString> ChapterAssets = {
		TEXT("DA_Chapter_W01_AshenKingdom"),
		TEXT("DA_Chapter_W01_BlackForest"),
		TEXT("DA_Chapter_W01_CursedCastle")};
	int32 TotalDepths = 0;
	for (int32 Index = 0; Index < ChapterAssets.Num(); ++Index)
	{
		const FString& AssetName = ChapterAssets[Index];
		const FString Path = FString::Printf(
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/Data/Encounters/%s.%s"),
			*AssetName, *AssetName);
		const UFantasyChapterDefinition* Chapter = LoadObject<UFantasyChapterDefinition>(nullptr, *Path);
		TestNotNull(*FString::Printf(TEXT("Chapter %d loads"), Index + 1), Chapter);
		if (!Chapter) continue;
		TestEqual(*FString::Printf(TEXT("Chapter %d number matches"), Index + 1),
			Chapter->ChapterNumber, Index + 1);
		TestEqual(*FString::Printf(TEXT("Chapter %d has six depths"), Index + 1),
			Chapter->TotalDepths, 6);
		TotalDepths += Chapter->TotalDepths;
		const TArray<FString> Errors = Chapter->ValidateDefinition();
		for (const FString& Error : Errors) AddError(FString::Printf(TEXT("Chapter %d: %s"), Index + 1, *Error));
		for (int32 Depth = 0; Depth < Chapter->TotalDepths; ++Depth)
		{
			TArray<FFantasyRouteNodeChoice> First;
			TArray<FFantasyRouteNodeChoice> Replay;
			FString Failure;
			TestTrue(*FString::Printf(TEXT("Chapter %d depth %d generates"), Index + 1, Depth),
				Chapter->GenerateRouteChoices(Depth, 20260822, TEXT("W01-M3-test"),
					EFantasyRunDifficulty::Normal, NAME_None, First, Failure));
			TestTrue(*FString::Printf(TEXT("Chapter %d depth %d replays"), Index + 1, Depth),
				Chapter->GenerateRouteChoices(Depth, 20260822, TEXT("W01-M3-test"),
					EFantasyRunDifficulty::Normal, NAME_None, Replay, Failure));
			TestEqual(TEXT("Replay keeps candidate count"), Replay.Num(), First.Num());
			for (int32 Choice = 0; Choice < First.Num() && Choice < Replay.Num(); ++Choice)
			{
				TestEqual(TEXT("Replay keeps candidate identity"), Replay[Choice].NodeId, First[Choice].NodeId);
			}
		}
	}
	TestEqual(TEXT("Campaign contains eighteen depths"), TotalDepths, 18);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFantasyM3EconomyContentTest,
	"WorldWalker.W01.M3.EconomyContent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFantasyM3EconomyContentTest::RunTest(const FString& Parameters)
{
	UAssetManager& AssetManager = UAssetManager::Get();
	const FString BlessingRoot(TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/Data/Blessings"));
	AssetManager.ScanPathsForPrimaryAssets(
		UFantasyBlessingDefinition::PrimaryAssetType, {BlessingRoot},
		UFantasyBlessingDefinition::StaticClass(), false, false, true);
	TArray<FPrimaryAssetId> BlessingIds;
	AssetManager.GetPrimaryAssetIdList(UFantasyBlessingDefinition::PrimaryAssetType, BlessingIds);
	int32 SharedCount = 0;
	int32 MageCount = 0;
	TSet<FName> StableIds;
	for (const FPrimaryAssetId& AssetId : BlessingIds)
	{
		UFantasyBlessingDefinition* Blessing = Cast<UFantasyBlessingDefinition>(
			AssetManager.GetPrimaryAssetPath(AssetId).TryLoad());
		if (!Blessing || !Blessing->GetPathName().StartsWith(BlessingRoot)) continue;
		TestFalse(TEXT("Blessing ID is unique"), StableIds.Contains(Blessing->BlessingId));
		StableIds.Add(Blessing->BlessingId);
		for (const FString& Error : Blessing->ValidateDefinition()) AddError(Error);
		if (Blessing->Profession == EFantasyPlayerProfession::Mage) ++MageCount;
		else if (Blessing->Profession == EFantasyPlayerProfession::None) ++SharedCount;
	}
	TestEqual(TEXT("Six shared blessings exist"), SharedCount, 6);
	TestEqual(TEXT("Six mage blessings exist"), MageCount, 6);

	const FString CardRoot(TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/Data/Cards"));
	AssetManager.ScanPathsForPrimaryAssets(
		UCardDefinition::PrimaryAssetType, {CardRoot}, UCardDefinition::StaticClass(), false, false, true);
	TArray<FPrimaryAssetId> CardIds;
	AssetManager.GetPrimaryAssetIdList(UCardDefinition::PrimaryAssetType, CardIds);
	int32 MageRewards = 0;
	for (const FPrimaryAssetId& AssetId : CardIds)
	{
		const UCardDefinition* Card = Cast<UCardDefinition>(AssetManager.GetPrimaryAssetPath(AssetId).TryLoad());
		if (Card && Card->GetPathName().StartsWith(CardRoot)
			&& Card->Profession == EFantasyPlayerProfession::Mage && Card->bRewardEligible)
		{
			++MageRewards;
		}
	}
	TestTrue(TEXT("Mage reward pool is within the 24-30 target"), MageRewards >= 24 && MageRewards <= 30);
	return true;
}

#endif
