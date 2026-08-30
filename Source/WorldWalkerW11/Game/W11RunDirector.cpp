#include "Game/W11RunDirector.h"

#include "Components/W11AttributeComponent.h"
#include "Components/W11RunEconomyComponent.h"
#include "Data/W11Definitions.h"
#include "Game/W11PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Game/W11Enemy.h"
#include "Game/W11EnemyProjectile.h"
#include "Game/W11GameMode.h"
#include "Game/W11GameState.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/Parse.h"
#include "WorldWalkerW11.h"

namespace
{
template <typename T>
void LoadSoftDefinitions(const TArray<TSoftObjectPtr<T>>& SoftDefinitions, TArray<TObjectPtr<T>>& OutDefinitions)
{
	OutDefinitions.Reset();
	for (const TSoftObjectPtr<T>& SoftDefinition : SoftDefinitions)
	{
		if (T* Definition = SoftDefinition.LoadSynchronous())
		{
			OutDefinitions.AddUnique(Definition);
		}
	}
}

int32 GetRarityPriceScale(const EW11Rarity Rarity)
{
	switch (Rarity)
	{
	case EW11Rarity::Yellow: return 100;
	case EW11Rarity::Mystic: return 180;
	case EW11Rarity::Earth: return 320;
	case EW11Rarity::Heaven: return 550;
	case EW11Rarity::Immortal: return 900;
	default: return 100;
	}
}
}

uint32 AW11RunDirector::GetStableNameHash(const FName Value)
{
	return FCrc::StrCrc32(*Value.ToString());
}

int32 AW11RunDirector::CalculateCurrentWaveBudget(
	const int32 RemainingBudget,
	const int32 RemainingWaves,
	const bool bLastWave)
{
	if (RemainingBudget <= 0 || RemainingWaves <= 0)
	{
		return 0;
	}
	return bLastWave
		? RemainingBudget
		: FMath::Max(1, FMath::CeilToInt(static_cast<float>(RemainingBudget) / RemainingWaves));
}

bool AW11RunDirector::ShouldRefillCurrentWave(
	const int32 ActiveEnemies,
	const int32 CurrentWaveBudgetRemaining,
	const int32 EncounterBudgetRemaining,
	const int32 ConcurrentCap)
{
	return ActiveEnemies >= 0
		&& ActiveEnemies < FMath::Max(1, ConcurrentCap)
		&& CurrentWaveBudgetRemaining > 0
		&& EncounterBudgetRemaining > 0;
}

AW11RunDirector::AW11RunDirector()
{
	bReplicates = false;
	PrimaryActorTick.bCanEverTick = false;
}

void AW11RunDirector::Initialize(UW11RunRuleSet* InRuleSet, const int32 InRunSeed)
{
	RuleSet = InRuleSet;
	RunSeed = InRunSeed;
	if (RuleSet)
	{
		LoadSoftDefinitions(RuleSet->StatChoices, StatChoices);
		LoadSoftDefinitions(RuleSet->Manuals, Manuals);
		LoadSoftDefinitions(RuleSet->Treasures, Treasures);
		LoadSoftDefinitions(RuleSet->Services, Services);
		LoadSoftDefinitions(RuleSet->ActiveAbilities, Abilities);
	}
}

TArray<FW11CultivationOffer> AW11RunDirector::GenerateCultivationOffers(const AW11PlayerState* PlayerState) const
{
	const int32 OfferCount = RuleSet ? FMath::Max(1, RuleSet->CultivationOfferCount) : 3;
	FRandomStream Stream = MakeStream(
		PlayerState,
		PlayerState ? PlayerState->GetPendingCultivationSelections() : 0,
		TEXT("Cultivation"));
	TArray<FW11CultivationOffer> Offers;
	TArray<TObjectPtr<UW11StatChoiceDefinition>> Candidates = StatChoices;
	const float Luck = PlayerState && PlayerState->GetAttributeComponent()
		? PlayerState->GetAttributeComponent()->GetStats().Luck
		: 0.0f;
	const float EnhancedChance = FMath::Min(0.25f, Luck / (Luck + 100.0f) * 0.25f);

	while (Offers.Num() < OfferCount && !Candidates.IsEmpty())
	{
		float TotalWeight = 0.0f;
		for (const UW11StatChoiceDefinition* Candidate : Candidates)
		{
			TotalWeight += Candidate ? FMath::Max(0.0f, Candidate->OfferWeight) : 0.0f;
		}
		const float Roll = Stream.FRandRange(0.0f, FMath::Max(TotalWeight, UE_SMALL_NUMBER));
		float Cursor = 0.0f;
		int32 SelectedIndex = 0;
		for (int32 Index = 0; Index < Candidates.Num(); ++Index)
		{
			Cursor += Candidates[Index] ? FMath::Max(0.0f, Candidates[Index]->OfferWeight) : 0.0f;
			if (Roll <= Cursor)
			{
				SelectedIndex = Index;
				break;
			}
		}

		const UW11StatChoiceDefinition* Selected = Candidates[SelectedIndex];
		Candidates.RemoveAtSwap(SelectedIndex);
		if (!Selected || Selected->Modifiers.IsEmpty())
		{
			continue;
		}
		const FW11StatModifier& Modifier = Selected->Modifiers[0];
		FW11CultivationOffer& Offer = Offers.AddDefaulted_GetRef();
		Offer.OfferId = Selected->DefinitionId;
		Offer.Stat = Modifier.Stat;
		Offer.Operation = Modifier.Operation;
		Offer.bEnhanced = Stream.FRand() < EnhancedChance;
		Offer.Magnitude = Modifier.Magnitude * (Offer.bEnhanced ? 1.5f : 1.0f);
	}

	if (Offers.Num() < OfferCount)
	{
		BuildFallbackCultivationOffers(Offers, Stream);
	}
	Offers.SetNum(FMath::Min(OfferCount, Offers.Num()));
	return Offers;
}

TArray<FW11ShopOffer> AW11RunDirector::GenerateShopOffers(
	const AW11PlayerState* PlayerState,
	const int32 StageIndex) const
{
	FRandomStream Stream = MakeStream(PlayerState, StageIndex, TEXT("ImmortalMarket"));
	TArray<FW11ShopOffer> Offers;
	TArray<TObjectPtr<UW11ManualDefinition>> ManualCandidates = Manuals;
	TArray<TObjectPtr<UW11TreasureDefinition>> TreasureCandidates = Treasures;
	TArray<TObjectPtr<UW11ShopServiceDefinition>> ServiceCandidates = Services;
	TArray<TObjectPtr<UW11AbilityDefinition>> AbilityCandidates = Abilities;

	const int32 ManualCount = RuleSet ? RuleSet->ManualShopSlots : 3;
	const int32 TreasureCount = RuleSet ? RuleSet->TreasureShopSlots : 2;
	const int32 ServiceCount = RuleSet ? RuleSet->ServiceShopSlots : 1;
	const int32 AbilityCount = RuleSet ? RuleSet->AbilityShopSlots : 1;
	const float StageScale = 1.0f + 0.1f * FMath::Max(0, StageIndex - 1);

	for (int32 Index = 0; Index < AbilityCount && !AbilityCandidates.IsEmpty(); ++Index)
	{
		const int32 SelectedIndex = Stream.RandRange(0, AbilityCandidates.Num() - 1);
		const UW11AbilityDefinition* Ability = AbilityCandidates[SelectedIndex];
		AbilityCandidates.RemoveAtSwap(SelectedIndex);
		if (!Ability)
		{
			continue;
		}
		FW11ShopOffer& Offer = Offers.AddDefaulted_GetRef();
		Offer.OfferId = FName(*FString::Printf(TEXT("Shop_S%d_A%d"), StageIndex, Index));
		Offer.Kind = EW11ShopOfferKind::Ability;
		Offer.DefinitionId = Ability->DefinitionId;
		Offer.Rarity = EW11Rarity::Mystic;
		Offer.Price = FMath::RoundToInt(180.0f * StageScale);
	}

	for (int32 Index = 0; Index < ManualCount && !ManualCandidates.IsEmpty(); ++Index)
	{
		const int32 SelectedIndex = Stream.RandRange(0, ManualCandidates.Num() - 1);
		const UW11ManualDefinition* Manual = ManualCandidates[SelectedIndex];
		ManualCandidates.RemoveAtSwap(SelectedIndex);
		if (!Manual)
		{
			continue;
		}
		FW11ShopOffer& Offer = Offers.AddDefaulted_GetRef();
		Offer.OfferId = FName(*FString::Printf(TEXT("Shop_S%d_M%d"), StageIndex, Index));
		Offer.Kind = EW11ShopOfferKind::Manual;
		Offer.DefinitionId = Manual->DefinitionId;
		Offer.Rarity = Manual->Rarity;
		Offer.Price = FMath::RoundToInt(FMath::Max(Manual->BasePrice, GetRarityPriceScale(Manual->Rarity)) * StageScale);
	}

	for (int32 Index = 0; Index < TreasureCount && !TreasureCandidates.IsEmpty(); ++Index)
	{
		const int32 SelectedIndex = Stream.RandRange(0, TreasureCandidates.Num() - 1);
		const UW11TreasureDefinition* Treasure = TreasureCandidates[SelectedIndex];
		TreasureCandidates.RemoveAtSwap(SelectedIndex);
		if (!Treasure || (PlayerState && PlayerState->GetEconomyComponent()->OwnsTreasure(Treasure->DefinitionId)))
		{
			--Index;
			continue;
		}
		FW11ShopOffer& Offer = Offers.AddDefaulted_GetRef();
		Offer.OfferId = FName(*FString::Printf(TEXT("Shop_S%d_T%d"), StageIndex, Index));
		Offer.Kind = EW11ShopOfferKind::Treasure;
		Offer.DefinitionId = Treasure->DefinitionId;
		Offer.Rarity = Treasure->Rarity;
		Offer.Price = FMath::RoundToInt(FMath::Max(Treasure->BasePrice, GetRarityPriceScale(Treasure->Rarity) + 50) * StageScale);
	}

	for (int32 Index = 0; Index < ServiceCount && !ServiceCandidates.IsEmpty(); ++Index)
	{
		const int32 SelectedIndex = Stream.RandRange(0, ServiceCandidates.Num() - 1);
		const UW11ShopServiceDefinition* Service = ServiceCandidates[SelectedIndex];
		ServiceCandidates.RemoveAtSwap(SelectedIndex);
		if (!Service)
		{
			continue;
		}
		FW11ShopOffer& Offer = Offers.AddDefaulted_GetRef();
		Offer.OfferId = FName(*FString::Printf(TEXT("Shop_S%d_S%d"), StageIndex, Index));
		Offer.Kind = EW11ShopOfferKind::Service;
		Offer.DefinitionId = Service->DefinitionId;
		Offer.Price = FMath::RoundToInt(Service->BasePrice * StageScale);
	}
	return Offers;
}

const UW11ManualDefinition* AW11RunDirector::FindManual(const FName DefinitionId) const
{
	const TObjectPtr<UW11ManualDefinition>* Found = Manuals.FindByPredicate([DefinitionId](const UW11ManualDefinition* Definition)
	{
		return Definition && Definition->DefinitionId == DefinitionId;
	});
	return Found ? Found->Get() : nullptr;
}

const UW11TreasureDefinition* AW11RunDirector::FindTreasure(const FName DefinitionId) const
{
	const TObjectPtr<UW11TreasureDefinition>* Found = Treasures.FindByPredicate([DefinitionId](const UW11TreasureDefinition* Definition)
	{
		return Definition && Definition->DefinitionId == DefinitionId;
	});
	return Found ? Found->Get() : nullptr;
}

const UW11ShopServiceDefinition* AW11RunDirector::FindService(const FName DefinitionId) const
{
	const TObjectPtr<UW11ShopServiceDefinition>* Found = Services.FindByPredicate([DefinitionId](const UW11ShopServiceDefinition* Definition)
	{
		return Definition && Definition->DefinitionId == DefinitionId;
	});
	return Found ? Found->Get() : nullptr;
}

const UW11AbilityDefinition* AW11RunDirector::FindAbility(const FName DefinitionId) const
{
	const TObjectPtr<UW11AbilityDefinition>* Found = Abilities.FindByPredicate([DefinitionId](const UW11AbilityDefinition* Definition)
	{
		return Definition && Definition->DefinitionId == DefinitionId;
	});
	return Found ? Found->Get() : nullptr;
}

void AW11RunDirector::StartStage(
	const int32 StageIndex,
	const int32 EncounterInstanceId,
	UW11EncounterDefinition* OverrideEncounter)
{
	if (!HasAuthority())
	{
		return;
	}
	ActiveStageIndex = FMath::Max(1, StageIndex);
	ActiveEncounterInstanceId = EncounterInstanceId;
	ActiveEnemyCount = 0;
	ActiveReward = FW11BattleReward();
	ActiveEncounter = nullptr;
	ActiveWaveIndex = 0;
	ActiveWaveBudgetRemaining = 0;
	ActiveWaveSpawnOrdinal = 0;
	NextStableSpawnId = 1;
	RemainingEncounterBudget = 0;
	ActiveArenaPawnZ = 96.0f;
	ActiveWaveStream.Initialize(0);
	CandidateSignature = 0;
	SpawnSignature = 0;
	bObjectiveCompleted = false;
	GetWorldTimerManager().ClearTimer(WaveTimerHandle);
	GetWorldTimerManager().ClearTimer(SurvivalTimerHandle);
	GetWorldTimerManager().ClearTimer(AutomationWaveResolveTimerHandle);
	if (!OverrideEncounter && (!RuleSet || RuleSet->Encounters.IsEmpty()))
	{
		return;
	}
	if (OverrideEncounter)
	{
		ActiveEncounter = OverrideEncounter;
	}
	else for (const TSoftObjectPtr<UW11EncounterDefinition>& SoftEncounter : RuleSet->Encounters)
	{
		UW11EncounterDefinition* Candidate = SoftEncounter.LoadSynchronous();
		if (Candidate && Candidate->StageIndex == ActiveStageIndex)
		{
			ActiveEncounter = Candidate;
			break;
		}
	}
	if (!ActiveEncounter && RuleSet && !RuleSet->Encounters.IsEmpty())
	{
		ActiveEncounter = RuleSet->Encounters[(ActiveStageIndex - 1) % RuleSet->Encounters.Num()].LoadSynchronous();
	}
	if (!ActiveEncounter || ActiveEncounter->EnemyPool.IsEmpty())
	{
		return;
	}
	ActiveReward = ActiveEncounter->Reward;
	for (const FW11WeightedEnemyEntry& Candidate : ActiveEncounter->EnemyPool)
	{
		const UW11EnemyDefinition* CandidateDefinition = Candidate.Enemy.LoadSynchronous();
		CandidateSignature = HashCombineFast(CandidateSignature,
			HashCombineFast(GetStableNameHash(CandidateDefinition ? CandidateDefinition->DefinitionId : NAME_None),
				HashCombineFast(GetTypeHash(Candidate.SpawnCost), GetTypeHash(Candidate.Weight))));
	}
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	const int32 PlayerCount = GameState ? FMath::Max(1, GameState->PlayerArray.Num()) : 1;
	RemainingEncounterBudget = FMath::Max(1, FMath::RoundToInt(ActiveEncounter->SpawnBudget
		* (1.0f + ActiveEncounter->BudgetScalePerAdditionalPlayer * (PlayerCount - 1))));
	const float ObjectiveEnd = ActiveEncounter->CompletionRule == EW11EncounterCompletionRule::SurviveDuration
		? GetWorld()->GetTimeSeconds() + FMath::Max(1.0f, ActiveEncounter->DurationSeconds) : 0.0f;
	if (AW11GameState* W11State = GetWorld()->GetGameState<AW11GameState>())
	{
		W11State->SetEncounterObjective(
			ActiveEncounter->DefinitionId, ActiveEncounter->EncounterType,
			0, FMath::Max(1, ActiveEncounter->WaveCount), ObjectiveEnd);
	}
	SpawnNextWave();
	if (ActiveEncounter->CompletionRule == EW11EncounterCompletionRule::SurviveDuration)
	{
		GetWorldTimerManager().SetTimer(
			SurvivalTimerHandle, FTimerDelegate::CreateUObject(
				this, &AW11RunDirector::CompleteActiveObjective, FName(TEXT("SurvivalDurationElapsed"))),
			FMath::Max(1.0f, ActiveEncounter->DurationSeconds), false);
	}
	UE_LOG(
		LogWorldWalkerW11,
		Display,
		TEXT("W11_ENCOUNTER_STARTED Schema=%d Stage=%d EncounterInstance=%d Encounter=%s Type=%d Rule=%d Enemies=%d Budget=%d RemainingBudget=%d Waves=%d CandidateSignature=%08X SpawnSignature=%08X"),
		RuleSet ? RuleSet->CombatLogSchemaVersion : 1,
		ActiveStageIndex,
		ActiveEncounterInstanceId,
		*ActiveEncounter->DefinitionId.ToString(),
		static_cast<int32>(ActiveEncounter->EncounterType),
		static_cast<int32>(ActiveEncounter->CompletionRule),
		ActiveEnemyCount,
		ActiveEncounter->SpawnBudget,
		RemainingEncounterBudget,
		ActiveEncounter->WaveCount,
		CandidateSignature,
		SpawnSignature);
}

void AW11RunDirector::SpawnNextWave()
{
	if (!HasAuthority() || !ActiveEncounter || bObjectiveCompleted || RemainingEncounterBudget <= 0)
	{
		return;
	}
	const int32 TotalWaves = FMath::Max(1, ActiveEncounter->WaveCount);
	if (ActiveWaveIndex >= TotalWaves)
	{
		return;
	}
	++ActiveWaveIndex;
	const int32 RemainingWaves = TotalWaves - ActiveWaveIndex + 1;
	ActiveWaveBudgetRemaining = CalculateCurrentWaveBudget(
		RemainingEncounterBudget, RemainingWaves, ActiveWaveIndex == TotalWaves);
	ActiveWaveSpawnOrdinal = 0;
	const uint32 StreamSeed = ActiveEncounter->EncounterType == EW11EncounterType::Clear && ActiveWaveIndex == 1
		? HashCombineFast(GetTypeHash(RunSeed), GetTypeHash(ActiveStageIndex))
		: HashCombineFast(
			HashCombineFast(GetTypeHash(RunSeed), GetTypeHash(ActiveStageIndex)),
			HashCombineFast(GetStableNameHash(ActiveEncounter->DefinitionId), GetTypeHash(ActiveWaveIndex)));
	ActiveWaveStream.Initialize(static_cast<int32>(StreamSeed));
	ActiveArenaPawnZ = 96.0f;
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	if (GameState && !GameState->PlayerArray.IsEmpty())
	{
		if (const APawn* Pawn = GameState->PlayerArray[0] ? GameState->PlayerArray[0]->GetPawn() : nullptr)
		{
			ActiveArenaPawnZ = Pawn->GetActorLocation().Z;
		}
	}
	SpawnCurrentWaveEnemies(false);
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_ENCOUNTER_WAVE_STARTED EncounterInstance=%d Wave=%d/%d Active=%d WaveBudgetRemaining=%d RemainingBudget=%d SpawnSignature=%08X"),
		ActiveEncounterInstanceId, ActiveWaveIndex, TotalWaves, ActiveEnemyCount,
		ActiveWaveBudgetRemaining, RemainingEncounterBudget, SpawnSignature);
}

void AW11RunDirector::SpawnCurrentWaveEnemies(const bool bReinforcement)
{
	if (!HasAuthority() || !ActiveEncounter || bObjectiveCompleted
		|| ActiveWaveBudgetRemaining <= 0 || RemainingEncounterBudget <= 0)
	{
		return;
	}
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	const int32 PlayerCount = GameState ? FMath::Max(1, GameState->PlayerArray.Num()) : 1;
	const int32 ConcurrentCap = FMath::Max(1, ActiveEncounter->MaxConcurrentEnemies);
	const int32 TotalWaves = FMath::Max(1, ActiveEncounter->WaveCount);
	const int32 SpawnCountBefore = ActiveEnemyCount;
	while (ActiveWaveBudgetRemaining > 0 && RemainingEncounterBudget > 0
		&& ActiveEnemyCount < ConcurrentCap && ActiveWaveSpawnOrdinal < 256)
	{
		TArray<int32, TInlineAllocator<8>> AffordableIndices;
		for (int32 Index = 0; Index < ActiveEncounter->EnemyPool.Num(); ++Index)
		{
			const int32 Cost = FMath::Max(1, ActiveEncounter->EnemyPool[Index].SpawnCost);
			if (Cost <= ActiveWaveBudgetRemaining && Cost <= RemainingEncounterBudget)
			{
				AffordableIndices.Add(Index);
			}
		}
		if (AffordableIndices.IsEmpty())
		{
			// No definition fits the exact budget remainder; consume the unusable tail
			// so the wave can complete instead of stalling forever.
			RemainingEncounterBudget = FMath::Max(
				0, RemainingEncounterBudget - ActiveWaveBudgetRemaining);
			ActiveWaveBudgetRemaining = 0;
			break;
		}

		int32 EntryIndex = AffordableIndices[0];
		if (ActiveWaveSpawnOrdinal < ActiveEncounter->EnemyPool.Num()
			&& AffordableIndices.Contains(ActiveWaveSpawnOrdinal))
		{
			EntryIndex = ActiveWaveSpawnOrdinal;
		}
		else
		{
			float TotalWeight = 0.0f;
			for (const int32 AffordableIndex : AffordableIndices)
			{
				TotalWeight += FMath::Max(0.0f, ActiveEncounter->EnemyPool[AffordableIndex].Weight);
			}
			const float Roll = ActiveWaveStream.FRandRange(0.0f, FMath::Max(TotalWeight, UE_SMALL_NUMBER));
			float Cursor = 0.0f;
			for (const int32 AffordableIndex : AffordableIndices)
			{
				Cursor += FMath::Max(0.0f, ActiveEncounter->EnemyPool[AffordableIndex].Weight);
				if (Roll <= Cursor)
				{
					EntryIndex = AffordableIndex;
					break;
				}
			}
		}
		const FW11WeightedEnemyEntry& Entry = ActiveEncounter->EnemyPool[EntryIndex];
		UW11EnemyDefinition* Definition = Entry.Enemy.LoadSynchronous();
		const int32 Cost = FMath::Max(1, Entry.SpawnCost);
		if (!Definition)
		{
			++ActiveWaveSpawnOrdinal;
			continue;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		if (AW11Enemy* Enemy = GetWorld()->SpawnActor<AW11Enemy>(
			AW11Enemy::StaticClass(), ChooseSafeSpawnLocation(ActiveWaveStream, ActiveArenaPawnZ), FRotator::ZeroRotator, Params))
		{
			const int32 SpawnId = NextStableSpawnId++;
			Enemy->InitializeFromDefinition(Definition,
				FMath::Max(0.01f, ActiveEncounter->EnemyBaseHealthScale)
					* (1.0f + ActiveEncounter->HealthScalePerAdditionalPlayer * (PlayerCount - 1)),
				FMath::Max(0.0f, ActiveEncounter->EnemyBasePowerScale),
				ActiveEncounterInstanceId, SpawnId);
			if (AW11GameMode* GameMode = GetWorld()->GetAuthGameMode<AW11GameMode>())
			{
				GameMode->RecordEnemySpawn(Definition->DefinitionId);
			}
			++ActiveEnemyCount;
			SpawnSignature = HashCombineFast(SpawnSignature,
				HashCombineFast(GetStableNameHash(Definition->DefinitionId), GetTypeHash(SpawnId)));
			UE_LOG(LogWorldWalkerW11, Display,
				TEXT("W11_WAVE_ENEMY_SPAWNED EncounterInstance=%d Wave=%d SpawnId=%d Enemy=%s Elite=%d Boss=%d"),
				ActiveEncounterInstanceId, ActiveWaveIndex, SpawnId, *Definition->DefinitionId.ToString(),
				Definition->bElite ? 1 : 0, Definition->bBoss ? 1 : 0);
		}
		ActiveWaveBudgetRemaining = FMath::Max(0, ActiveWaveBudgetRemaining - Cost);
		RemainingEncounterBudget = FMath::Max(0, RemainingEncounterBudget - Cost);
		++ActiveWaveSpawnOrdinal;
	}
	if (AW11GameState* State = GetWorld()->GetGameState<AW11GameState>())
	{
		State->SetRemainingEnemyCount(ActiveEnemyCount);
		State->SetEncounterObjective(ActiveEncounter->DefinitionId, ActiveEncounter->EncounterType,
			ActiveWaveIndex, TotalWaves,
			ActiveEncounter->CompletionRule == EW11EncounterCompletionRule::SurviveDuration
				? GetWorld()->GetTimeSeconds() + (GetWorldTimerManager().IsTimerActive(SurvivalTimerHandle)
					? GetWorldTimerManager().GetTimerRemaining(SurvivalTimerHandle)
					: FMath::Max(1.0f, ActiveEncounter->DurationSeconds)) : 0.0f);
	}
	const int32 SpawnedNow = ActiveEnemyCount - SpawnCountBefore;
	if (bReinforcement && SpawnedNow > 0)
	{
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_WAVE_REINFORCEMENT_SPAWNED EncounterInstance=%d Wave=%d Spawned=%d Active=%d WaveBudgetRemaining=%d RemainingBudget=%d"),
			ActiveEncounterInstanceId, ActiveWaveIndex, SpawnedNow, ActiveEnemyCount,
			ActiveWaveBudgetRemaining, RemainingEncounterBudget);
	}
	if (SpawnedNow > 0 && FParse::Param(FCommandLine::Get(), TEXT("W11AutoResolveWaves")))
	{
		GetWorldTimerManager().SetTimer(AutomationWaveResolveTimerHandle, this,
			&AW11RunDirector::ResolveWaveForAutomation, 0.75f, false);
	}
}

void AW11RunDirector::ResolveWaveForAutomation()
{
	if (!HasAuthority() || bObjectiveCompleted || ActiveEncounterInstanceId <= 0)
	{
		return;
	}
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_AUTOMATION_WAVE_RESOLVE EncounterInstance=%d Wave=%d Active=%d"),
		ActiveEncounterInstanceId, ActiveWaveIndex, ActiveEnemyCount);
	TArray<AW11Enemy*> Targets;
	for (TActorIterator<AW11Enemy> It(GetWorld()); It; ++It)
	{
		if (It->GetEncounterInstanceId() == ActiveEncounterInstanceId
			&& It->GetAttributeComponent() && !It->GetAttributeComponent()->IsDefeated())
		{
			Targets.Add(*It);
		}
	}
	for (AW11Enemy* Enemy : Targets)
	{
		if (Enemy && Enemy->GetAttributeComponent())
		{
			if (Enemy->IsBoss())
			{
				const float MaximumHealth = Enemy->GetAttributeComponent()->GetStats().MaxHealth;
				for (int32 Attempt = 0;
					Attempt < 12
					&& !Enemy->GetAttributeComponent()->IsDefeated()
					&& Enemy->GetBossPhaseIndex() < Enemy->GetAuthoredBossTransitionCount();
					++Attempt)
				{
					Enemy->GetAttributeComponent()->ApplyRawDamage(MaximumHealth * 0.20f);
				}
				Enemy->GetAttributeComponent()->ApplyRawDamage(MaximumHealth * 10.0f);
			}
			else
			{
				Enemy->GetAttributeComponent()->ApplyRawDamage(1000000.0f);
			}
		}
	}
}

void AW11RunDirector::NotifyEnemyDefeated(const AW11Enemy* Enemy)
{
	if (!HasAuthority() || !Enemy || ActiveEnemyCount <= 0
		|| Enemy->GetEncounterInstanceId() != ActiveEncounterInstanceId)
	{
		return;
	}
	--ActiveEnemyCount;
	if (ActiveEncounter && ShouldRefillCurrentWave(
		ActiveEnemyCount, ActiveWaveBudgetRemaining, RemainingEncounterBudget,
		ActiveEncounter->MaxConcurrentEnemies))
	{
		SpawnCurrentWaveEnemies(true);
	}
	if (AW11GameState* W11State = GetWorld()->GetGameState<AW11GameState>())
	{
		W11State->SetRemainingEnemyCount(ActiveEnemyCount);
	}
	if (ActiveEnemyCount == 0)
	{
		if (!ActiveEncounter || bObjectiveCompleted)
		{
			return;
		}
		const bool bHasMoreWaves = ActiveWaveIndex < FMath::Max(1, ActiveEncounter->WaveCount)
			&& RemainingEncounterBudget > 0;
		if (bHasMoreWaves)
		{
			GetWorldTimerManager().SetTimer(WaveTimerHandle, this,
				&AW11RunDirector::SpawnNextWave,
				FMath::Max(0.0f, ActiveEncounter->InterWaveDelaySeconds), false);
		}
		else if (ActiveEncounter->CompletionRule == EW11EncounterCompletionRule::EliminateAll
			|| ActiveEncounter->CompletionRule == EW11EncounterCompletionRule::EliminateAllWaves)
		{
			CompleteActiveObjective(TEXT("EncounterObjectiveCompleted"));
		}
	}
}

void AW11RunDirector::CompleteActiveObjective(const FName Reason)
{
	if (!HasAuthority() || !ActiveEncounter || bObjectiveCompleted || ActiveEncounterInstanceId <= 0)
	{
		return;
	}
	bObjectiveCompleted = true;
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_ENCOUNTER_OBJECTIVE_COMPLETED EncounterInstance=%d Encounter=%s Type=%d Rule=%d Reason=%s ActiveEnemies=%d Wave=%d"),
		ActiveEncounterInstanceId, *ActiveEncounter->DefinitionId.ToString(),
		static_cast<int32>(ActiveEncounter->EncounterType),
		static_cast<int32>(ActiveEncounter->CompletionRule), *Reason.ToString(),
		ActiveEnemyCount, ActiveWaveIndex);
	if (AW11GameMode* GameMode = GetWorld()->GetAuthGameMode<AW11GameMode>())
	{
		GameMode->CompleteCombatStage(ActiveReward, Reason);
	}
}

void AW11RunDirector::SignalExternalObjectiveCompleted(const FName ObjectiveId)
{
	if (ActiveEncounter && ActiveEncounter->CompletionRule == EW11EncounterCompletionRule::ExternalObjective)
	{
		CompleteActiveObjective(ObjectiveId.IsNone() ? FName(TEXT("ExternalObjective")) : ObjectiveId);
	}
}

FVector AW11RunDirector::ChooseSafeSpawnLocation(FRandomStream& Stream, const float ArenaPawnZ) const
{
	const float Inner = ActiveEncounter ? FMath::Max(0.0f, ActiveEncounter->SpawnSafeRadius) : 450.0f;
	const float Outer = ActiveEncounter ? FMath::Max(Inner + 1.0f, ActiveEncounter->SpawnOuterRadius) : 900.0f;
	TArray<FVector> PlayerLocations;
	if (const AGameStateBase* State = GetWorld()->GetGameState())
	{
		for (const APlayerState* PlayerState : State->PlayerArray)
		{
			if (const APawn* Pawn = PlayerState ? PlayerState->GetPawn() : nullptr)
			{
				PlayerLocations.Add(Pawn->GetActorLocation());
			}
		}
	}
	FVector Best(Outer, 0.0f, ArenaPawnZ);
	float BestNearestDistanceSq = -1.0f;
	for (int32 Attempt = 0; Attempt < 12; ++Attempt)
	{
		const float Angle = Stream.FRandRange(0.0f, 2.0f * PI);
		const float Radius = Stream.FRandRange(Inner, Outer);
		const FVector Candidate(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, ArenaPawnZ);
		float NearestDistanceSq = TNumericLimits<float>::Max();
		for (const FVector& PlayerLocation : PlayerLocations)
		{
			NearestDistanceSq = FMath::Min(NearestDistanceSq, FVector::DistSquared2D(Candidate, PlayerLocation));
		}
		if (PlayerLocations.IsEmpty() || NearestDistanceSq >= FMath::Square(Inner))
		{
			return Candidate;
		}
		if (NearestDistanceSq > BestNearestDistanceSq)
		{
			Best = Candidate;
			BestNearestDistanceSq = NearestDistanceSq;
		}
	}
	return Best;
}

FName AW11RunDirector::GetActiveEncounterId() const
{
	return ActiveEncounter ? ActiveEncounter->DefinitionId : NAME_None;
}

EW11EncounterType AW11RunDirector::GetActiveEncounterType() const
{
	return ActiveEncounter ? ActiveEncounter->EncounterType : EW11EncounterType::Clear;
}

void AW11RunDirector::CancelActiveEncounter(const int32 EncounterInstanceId)
{
	if (!HasAuthority() || EncounterInstanceId <= 0)
	{
		return;
	}
	for (TActorIterator<AW11Enemy> It(GetWorld()); It; ++It)
	{
		if (It->GetEncounterInstanceId() == EncounterInstanceId)
		{
			It->CancelForCombatEnd(EncounterInstanceId);
		}
	}
	for (TActorIterator<AW11EnemyProjectile> It(GetWorld()); It; ++It)
	{
		if (It->GetEncounterInstanceId() == EncounterInstanceId)
		{
			if (AW11GameMode* GameMode = GetWorld()->GetAuthGameMode<AW11GameMode>())
			{
				GameMode->RecordCancelledThreat();
			}
			It->Destroy();
		}
	}
	if (ActiveEncounterInstanceId == EncounterInstanceId)
	{
		GetWorldTimerManager().ClearTimer(WaveTimerHandle);
		GetWorldTimerManager().ClearTimer(SurvivalTimerHandle);
		GetWorldTimerManager().ClearTimer(AutomationWaveResolveTimerHandle);
		bObjectiveCompleted = true;
		ActiveEnemyCount = 0;
		ActiveEncounterInstanceId = 0;
		ActiveWaveBudgetRemaining = 0;
		ActiveWaveSpawnOrdinal = 0;
	}
	if (AW11GameState* W11State = GetWorld()->GetGameState<AW11GameState>())
	{
		W11State->SetRemainingEnemyCount(0);
	}
}

FRandomStream AW11RunDirector::MakeStream(
	const AW11PlayerState* PlayerState,
	const int32 StageIndex,
	const FName Purpose) const
{
	const int32 PlayerId = PlayerState ? PlayerState->GetPlayerId() : 0;
	const uint32 PurposeHash = GetStableNameHash(Purpose);
	const uint32 Seed = HashCombineFast(
		HashCombineFast(GetTypeHash(RunSeed), GetTypeHash(StageIndex)),
		HashCombineFast(GetTypeHash(PlayerId), PurposeHash));
	return FRandomStream(static_cast<int32>(Seed));
}

void AW11RunDirector::BuildFallbackCultivationOffers(
	TArray<FW11CultivationOffer>& OutOffers,
	FRandomStream& Stream) const
{
	struct FFallbackChoice
	{
		const TCHAR* Id;
		EW11StatType Stat;
		EW11ModifierOperation Operation;
		float Magnitude;
	};
	static const FFallbackChoice Choices[] = {
		{ TEXT("Vitality"), EW11StatType::MaxHealth, EW11ModifierOperation::Multiply, 1.12f },
		{ TEXT("Essence"), EW11StatType::MaxMana, EW11ModifierOperation::Multiply, 1.15f },
		{ TEXT("Circulation"), EW11StatType::ManaRegen, EW11ModifierOperation::Add, 1.5f },
		{ TEXT("Offense"), EW11StatType::Power, EW11ModifierOperation::Multiply, 1.10f },
		{ TEXT("BodyGuard"), EW11StatType::Armor, EW11ModifierOperation::Add, 12.0f },
		{ TEXT("Movement"), EW11StatType::MoveSpeed, EW11ModifierOperation::Multiply, 1.08f },
		{ TEXT("AttackSpeed"), EW11StatType::AttackSpeed, EW11ModifierOperation::Multiply, 1.10f },
		{ TEXT("Haste"), EW11StatType::AbilityHaste, EW11ModifierOperation::Add, 0.12f },
		{ TEXT("Critical"), EW11StatType::CritChance, EW11ModifierOperation::Add, 0.05f },
		{ TEXT("Fortune"), EW11StatType::Luck, EW11ModifierOperation::Add, 20.0f },
		{ TEXT("Sense"), EW11StatType::AttackRange, EW11ModifierOperation::Multiply, 1.12f },
		{ TEXT("Domain"), EW11StatType::Area, EW11ModifierOperation::Multiply, 1.15f },
		{ TEXT("Awareness"), EW11StatType::PickupRadius, EW11ModifierOperation::Multiply, 1.25f }
	};

	TArray<int32> Indices;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Choices); ++Index)
	{
		Indices.Add(Index);
	}
	while (OutOffers.Num() < 3 && !Indices.IsEmpty())
	{
		const int32 Pick = Stream.RandRange(0, Indices.Num() - 1);
		const FFallbackChoice& Choice = Choices[Indices[Pick]];
		Indices.RemoveAtSwap(Pick);
		FW11CultivationOffer& Offer = OutOffers.AddDefaulted_GetRef();
		Offer.OfferId = Choice.Id;
		Offer.Stat = Choice.Stat;
		Offer.Operation = Choice.Operation;
		Offer.Magnitude = Choice.Magnitude;
	}
}
