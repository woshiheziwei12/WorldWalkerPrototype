#include "Game/W11GameState.h"

#include "Game/W11ArenaBackdrop.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"
#include "WorldWalkerW11.h"

AW11GameState::AW11GameState()
{
	bReplicates = true;
}

void AW11GameState::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() != NM_DedicatedServer && GetWorld())
	{
		GetWorld()->SpawnActor<AW11ArenaBackdrop>(
			AW11ArenaBackdrop::StaticClass(),
			FVector(0.0f, 0.0f, 10.0f),
			FRotator::ZeroRotator);
	}
}

void AW11GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AW11GameState, RunPhase);
	DOREPLIFETIME(AW11GameState, RunSeed);
	DOREPLIFETIME(AW11GameState, MemoryRunSequence);
	DOREPLIFETIME(AW11GameState, StageIndex);
	DOREPLIFETIME(AW11GameState, ChapterIndex);
	DOREPLIFETIME(AW11GameState, ChapterNodeIndex);
	DOREPLIFETIME(AW11GameState, ChapterNodeType);
	DOREPLIFETIME(AW11GameState, ChapterId);
	DOREPLIFETIME(AW11GameState, ChapterNodeId);
	DOREPLIFETIME(AW11GameState, PhaseEndServerTime);
	DOREPLIFETIME(AW11GameState, RuleSetId);
	DOREPLIFETIME(AW11GameState, EncounterInstanceId);
	DOREPLIFETIME(AW11GameState, CombatOutcome);
	DOREPLIFETIME(AW11GameState, OutcomeReason);
	DOREPLIFETIME(AW11GameState, OutcomeSequence);
	DOREPLIFETIME(AW11GameState, RemainingEnemyCount);
	DOREPLIFETIME(AW11GameState, ActiveEncounterId);
	DOREPLIFETIME(AW11GameState, EncounterType);
	DOREPLIFETIME(AW11GameState, CurrentWave);
	DOREPLIFETIME(AW11GameState, WaveCount);
	DOREPLIFETIME(AW11GameState, ObjectiveEndServerTime);
}

void AW11GameState::InitializeRun(const int32 InRunSeed, const FName InRuleSetId)
{
	if (!HasAuthority())
	{
		return;
	}
	MemoryRunSequence = AdvanceMemoryRunSequence(
		MemoryRunSequence, EncounterInstanceId, OutcomeSequence);
	RunSeed = InRunSeed;
	RuleSetId = InRuleSetId;
	StageIndex = 0;
	ChapterIndex = 0;
	ChapterNodeIndex = 0;
	ChapterNodeType = EW11ChapterNodeType::Combat;
	ChapterId = NAME_None;
	ChapterNodeId = NAME_None;
	CombatOutcome = EW11CombatOutcome::Pending;
	OutcomeReason = NAME_None;
	RemainingEnemyCount = 0;
	ActiveEncounterId = NAME_None;
	EncounterType = EW11EncounterType::Clear;
	CurrentWave = 0;
	WaveCount = 0;
	ObjectiveEndServerTime = 0.0f;
	SetRunPhase(EW11RunPhase::Lobby);
}

void AW11GameState::SetEncounterObjective(
	const FName EncounterId,
	const EW11EncounterType Type,
	const int32 InCurrentWave,
	const int32 InWaveCount,
	const float InObjectiveEndServerTime)
{
	if (!HasAuthority())
	{
		return;
	}
	ActiveEncounterId = EncounterId;
	EncounterType = Type;
	CurrentWave = FMath::Max(0, InCurrentWave);
	WaveCount = FMath::Max(0, InWaveCount);
	ObjectiveEndServerTime = FMath::Max(0.0f, InObjectiveEndServerTime);
	BroadcastRuntimeStateChanged();
	ForceNetUpdate();
}

void AW11GameState::SetRemainingEnemyCount(const int32 NewCount)
{
	if (HasAuthority())
	{
		RemainingEnemyCount = FMath::Max(0, NewCount);
		BroadcastRuntimeStateChanged();
		ForceNetUpdate();
	}
}

int32 AW11GameState::BeginEncounter()
{
	if (!HasAuthority())
	{
		return EncounterInstanceId;
	}
	// Zero is reserved for "no encounter". This also gives wraparound a defined reset.
	EncounterInstanceId = AdvanceAuthoritySequence(EncounterInstanceId);
	CombatOutcome = EW11CombatOutcome::Pending;
	OutcomeReason = NAME_None;
	BroadcastRuntimeStateChanged();
	ForceNetUpdate();
	return EncounterInstanceId;
}

bool AW11GameState::TrySetCombatOutcome(
	const int32 ExpectedEncounterInstanceId,
	const EW11CombatOutcome NewOutcome,
	const FName Reason)
{
	if (!HasAuthority() || !CanTransitionCombatOutcome(
		EncounterInstanceId, ExpectedEncounterInstanceId, CombatOutcome, NewOutcome))
	{
		return false;
	}
	CombatOutcome = NewOutcome;
	OutcomeReason = Reason;
	OutcomeSequence = AdvanceAuthoritySequence(OutcomeSequence);
	BroadcastRuntimeStateChanged();
	ForceNetUpdate();
	return true;
}

bool AW11GameState::CanTransitionCombatOutcome(
	const int32 CurrentEncounterInstanceId,
	const int32 ExpectedEncounterInstanceId,
	const EW11CombatOutcome CurrentOutcome,
	const EW11CombatOutcome NewOutcome)
{
	return ExpectedEncounterInstanceId > 0
		&& ExpectedEncounterInstanceId == CurrentEncounterInstanceId
		&& CurrentOutcome == EW11CombatOutcome::Pending
		&& NewOutcome != EW11CombatOutcome::Pending;
}

bool AW11GameState::IsPendingCombatEncounter(
	const EW11RunPhase RunPhase,
	const EW11CombatOutcome CurrentOutcome,
	const int32 CurrentEncounterInstanceId,
	const int32 ExpectedEncounterInstanceId)
{
	return RunPhase == EW11RunPhase::Combat
		&& CurrentOutcome == EW11CombatOutcome::Pending
		&& ExpectedEncounterInstanceId > 0
		&& CurrentEncounterInstanceId == ExpectedEncounterInstanceId;
}

int32 AW11GameState::AdvanceAuthoritySequence(const int32 CurrentSequence)
{
	return CurrentSequence <= 0 || CurrentSequence == MAX_int32
		? 1 : CurrentSequence + 1;
}

int32 AW11GameState::AdvanceMemoryRunSequence(
	const int32 CurrentMemoryRunSequence,
	int32& InOutEncounterSequence,
	int32& InOutOutcomeSequence)
{
	if (CurrentMemoryRunSequence <= 0)
	{
		InOutEncounterSequence = 0;
		InOutOutcomeSequence = 0;
	}
	return AdvanceAuthoritySequence(CurrentMemoryRunSequence);
}

void AW11GameState::SetRunPhase(const EW11RunPhase NewPhase, const float DurationSeconds)
{
	if (!HasAuthority())
	{
		return;
	}
	RunPhase = NewPhase;
	PhaseEndServerTime = DurationSeconds > 0.0f
		? GetServerWorldTimeSeconds() + DurationSeconds
		: 0.0f;
	BroadcastRuntimeStateChanged();
	ForceNetUpdate();
}

void AW11GameState::BroadcastRuntimeStateChanged()
{
	OnRuntimeStateChanged.Broadcast();
}

void AW11GameState::OnRep_RuntimeState()
{
	BroadcastRuntimeStateChanged();
}

void AW11GameState::OnRep_CombatOutcomeSnapshot()
{
	BroadcastRuntimeStateChanged();
	if (FParse::Param(FCommandLine::Get(), TEXT("W11NetworkSmoke")))
	{
		UE_LOG(LogWorldWalkerW11, Display,
			TEXT("W11_NETWORK_OUTCOME_SNAPSHOT EncounterInstance=%d Sequence=%d Outcome=%d Reason=%s Phase=%d"),
			EncounterInstanceId, OutcomeSequence, static_cast<int32>(CombatOutcome),
			*OutcomeReason.ToString(), static_cast<int32>(RunPhase));
	}
}

void AW11GameState::SetStageIndex(const int32 NewStageIndex)
{
	if (HasAuthority())
	{
		StageIndex = FMath::Max(0, NewStageIndex);
		BroadcastRuntimeStateChanged();
		ForceNetUpdate();
	}
}

void AW11GameState::SetChapterRouteState(
	const int32 InChapterIndex,
	const int32 InNodeIndex,
	const EW11ChapterNodeType InNodeType,
	const FName InChapterId,
	const FName InNodeId)
{
	if (!HasAuthority())
	{
		return;
	}
	ChapterIndex = FMath::Max(0, InChapterIndex);
	ChapterNodeIndex = FMath::Max(0, InNodeIndex);
	ChapterNodeType = InNodeType;
	ChapterId = InChapterId;
	ChapterNodeId = InNodeId;
	BroadcastRuntimeStateChanged();
	ForceNetUpdate();
}

float AW11GameState::GetPhaseTimeRemaining() const
{
	return PhaseEndServerTime > 0.0f
		? FMath::Max(0.0f, PhaseEndServerTime - GetServerWorldTimeSeconds())
		: 0.0f;
}

float AW11GameState::GetObjectiveTimeRemaining() const
{
	return ObjectiveEndServerTime > 0.0f
		? FMath::Max(0.0f, ObjectiveEndServerTime - GetServerWorldTimeSeconds())
		: 0.0f;
}
