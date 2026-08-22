#include "Cards/Fantasy/FantasyCampaignDefinition.h"

#include "Cards/Fantasy/FantasyChapterDefinition.h"

const FPrimaryAssetType UFantasyCampaignDefinition::PrimaryAssetType(TEXT("FantasyCampaignDefinition"));

FPrimaryAssetId UFantasyCampaignDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, CampaignId.IsNone() ? GetFName() : CampaignId);
}

TArray<FString> UFantasyCampaignDefinition::ValidateDefinition() const
{
	TArray<FString> Errors;
	if (CampaignId.IsNone()) Errors.Add(TEXT("CampaignId is empty."));
	if (PlannedChapterCount < 3) Errors.Add(TEXT("Campaign must reserve at least three chapters."));
	if (Chapters.Num() != PlannedChapterCount) Errors.Add(TEXT("Chapter slot count must match PlannedChapterCount."));
	int32 OpenCount = 0;
	TSet<int32> Numbers;
	for (const FFantasyCampaignChapterSlot& Slot : Chapters)
	{
		if (Slot.ChapterId.IsNone()) Errors.Add(TEXT("A chapter slot has no ChapterId."));
		if (Slot.ChapterNumber < 1 || Numbers.Contains(Slot.ChapterNumber))
		{
			Errors.Add(FString::Printf(TEXT("Invalid or duplicate chapter number: %d."), Slot.ChapterNumber));
		}
		Numbers.Add(Slot.ChapterNumber);
		if (Slot.bOpen)
		{
			++OpenCount;
			if (Slot.Definition.IsNull()) Errors.Add(TEXT("An open chapter has no definition."));
		}
	}
	if (OpenCount != OpenChapterCount) Errors.Add(TEXT("OpenChapterCount does not match open slots."));
	return Errors;
}

const FFantasyCampaignChapterSlot* UFantasyCampaignDefinition::FindChapter(const int32 ChapterNumber) const
{
	return Chapters.FindByPredicate([ChapterNumber](const FFantasyCampaignChapterSlot& Slot)
	{
		return Slot.ChapterNumber == ChapterNumber;
	});
}
