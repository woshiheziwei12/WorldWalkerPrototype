#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "W11Types.generated.h"

UENUM(BlueprintType)
enum class EW11RunPhase : uint8
{
	Lobby,
	Combat,
	Cultivation,
	ImmortalMarket,
	ChapterTransition,
	Results
};

UENUM(BlueprintType)
enum class EW11EncounterType : uint8
{
	Clear,
	Waves,
	Survival,
	Elite,
	Boss
};

/** Data-driven chapter route node. Node counts remain content data, not GameMode rules. */
UENUM(BlueprintType)
enum class EW11ChapterNodeType : uint8
{
	Combat,
	Event,
	Rest,
	ImmortalMarket,
	SmallBoss,
	FinalBoss
};

USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11ChapterRouteCursor
{
	GENERATED_BODY()

	/** Zero-based index into the five authored chapter definitions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="W11|Chapter")
	int32 ChapterIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="W11|Chapter")
	int32 NodeIndex = 0;

	/** Set after the fifth small boss while the confirmed final encounter is active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="W11|Chapter")
	bool bFinalBoss = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="W11|Chapter")
	bool bComplete = false;
};

UENUM(BlueprintType)
enum class EW11EncounterCompletionRule : uint8
{
	EliminateAll,
	EliminateAllWaves,
	SurviveDuration,
	ExternalObjective
};

UENUM(BlueprintType)
enum class EW11HeroGender : uint8
{
	Male,
	Female
};

UENUM(BlueprintType)
enum class EW11FacingDirection : uint8
{
	Up,
	Down,
	Left,
	Right
};

UENUM(BlueprintType)
enum class EW11CharacterAction : uint8
{
	Idle,
	Move,
	Dodge,
	Cast,
	Hit
};

UENUM(BlueprintType)
enum class EW11AbilityArchetype : uint8
{
	DirectedSweep,
	RadialBurst,
	PiercingLine,
	SelfBarrier,
	SelfHeal,
	FormationBurst
};

/** Stable logical inputs. Physical bindings remain replaceable. */
UENUM(BlueprintType)
enum class EW11AbilitySlot : uint8
{
	BasicAttack,
	Active1,
	Active2,
	Active3,
	Active4
};

UENUM(BlueprintType)
enum class EW11AbilityTargetMode : uint8
{
	Self,
	Direction,
	TargetPoint,
	AutoTarget
};

UENUM(BlueprintType)
enum class EW11HitShape : uint8
{
	Point,
	Circle,
	Line,
	Cone,
	Projectile
};

UENUM(BlueprintType)
enum class EW11HitDelivery : uint8
{
	Immediate,
	Projectile,
	PersistentArea
};

UENUM(BlueprintType)
enum class EW11CooldownScaling : uint8
{
	None,
	AttackSpeed,
	AbilityHaste
};

UENUM(BlueprintType)
enum class EW11CriticalRollPolicy : uint8
{
	Never,
	PerExecution,
	PerTarget
};

UENUM(BlueprintType)
enum class EW11EffectResultType : uint8
{
	Damage,
	Heal,
	Barrier,
	Mana,
	Status
};

UENUM(BlueprintType)
enum class EW11StatusType : uint8
{
	None,
	Burn,
	Slow
};

UENUM(BlueprintType)
enum class EW11StatusStackingRule : uint8
{
	RefreshDuration,
	AddStackAndRefresh
};

UENUM(BlueprintType)
enum class EW11AbilityRejectionReason : uint8
{
	None,
	InvalidInput,
	StaleEncounter,
	DuplicateRequest,
	InvalidPhase,
	Defeated,
	Locked,
	Cooldown,
	InsufficientMana,
	UnslottedAbility,
	NoLegalTarget
};

UENUM(BlueprintType)
enum class EW11CombatOutcome : uint8
{
	Pending,
	Victory,
	Defeat
};

UENUM(BlueprintType)
enum class EW11CombatCueType : uint8
{
	AbilityStarted,
	Impact,
	Damage,
	Heal,
	Barrier,
	Immune,
	Defeated,
	Rejected
};

UENUM(BlueprintType)
enum class EW11EnemyCombatState : uint8
{
	AcquireTarget,
	Approach,
	Reposition,
	Windup,
	Active,
	Recovery,
	Stagger,
	Defeated
};

UENUM(BlueprintType)
enum class EW11TelegraphShape : uint8
{
	None,
	Line,
	Circle,
	ProjectilePath
};

UENUM(BlueprintType)
enum class EW11StatType : uint8
{
	MaxHealth,
	MaxMana,
	ManaRegen,
	Power,
	Armor,
	MoveSpeed,
	AttackSpeed,
	AbilityHaste,
	CritChance,
	CritMultiplier,
	Luck,
	AttackRange,
	Area,
	PickupRadius,
	Duration,
	HealingReceived
};

UENUM(BlueprintType)
enum class EW11ModifierOperation : uint8
{
	Add,
	Multiply,
	Override
};

UENUM(BlueprintType)
enum class EW11Rarity : uint8
{
	Yellow,
	Mystic,
	Earth,
	Heaven,
	Immortal
};

UENUM(BlueprintType)
enum class EW11ManualCategory : uint8
{
	BodyCultivation,
	Breathing,
	SwordAndMartial,
	Spell,
	Movement,
	DivineSense,
	Formation,
	Fortune,
	DivinePower
};

UENUM(BlueprintType)
enum class EW11ComprehensionRank : uint8
{
	None,
	Initiate,
	Adept,
	Mastered,
	Perfected
};

UENUM(BlueprintType)
enum class EW11TreasureCategory : uint8
{
	Artifact,
	ProtectiveTreasure,
	Curio,
	FormationDevice,
	Consumable
};

UENUM(BlueprintType)
enum class EW11ShopOfferKind : uint8
{
	Manual,
	Treasure,
	Service,
	Ability
};

USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11StatModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EW11StatType Stat = EW11StatType::Power;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EW11ModifierOperation Operation = EW11ModifierOperation::Add;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Magnitude = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SourceId = NAME_None;
};

USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11StatBlock
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxMana = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ManaRegenPerSecond = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Power = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Armor = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MoveSpeedScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackSpeedScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AbilityHaste = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CritChance = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CritMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Luck = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackRangeScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AreaScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PickupRadius = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DurationScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HealingReceivedScale = 1.0f;
};

USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11OwnedManual
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName ManualId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EW11ComprehensionRank Rank = EW11ComprehensionRank::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 InvestedSpiritStones = 0;
};

/** Unlocked active ability and its run-local upgrade level. */
USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11EquippedAbility
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName AbilityId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(ClampMin="1"))
	int32 Level = 1;
};

USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11CultivationOffer
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName OfferId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EW11StatType Stat = EW11StatType::Power;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EW11ModifierOperation Operation = EW11ModifierOperation::Add;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Magnitude = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bEnhanced = false;
};

USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11ShopOffer
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName OfferId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EW11ShopOfferKind Kind = EW11ShopOfferKind::Manual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName DefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EW11Rarity Rarity = EW11Rarity::Yellow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Price = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bPurchased = false;
};

USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11BattleReward
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SpiritStones = 120;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CultivationExperience = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bElite = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bBoss = false;
};

/** Stable hand-off payload for a future chapter/map-node router. */
USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11CombatNodeResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 EncounterInstanceId = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 MemoryRunSequence = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 StageIndex = 0;

	UPROPERTY(BlueprintReadOnly)
	FName EncounterId = NAME_None;

	UPROPERTY(BlueprintReadOnly)
	EW11EncounterType EncounterType = EW11EncounterType::Clear;

	UPROPERTY(BlueprintReadOnly)
	EW11CombatOutcome Outcome = EW11CombatOutcome::Pending;

	UPROPERTY(BlueprintReadOnly)
	FName Reason = NAME_None;

	UPROPERTY(BlueprintReadOnly)
	FW11BattleReward Reward;

	/** Stable PlayerIds that were unable to fight when authority finalized this node. */
	UPROPERTY(BlueprintReadOnly)
	TArray<int32> DefeatedPlayerIds;

	/** Stable PlayerIds that were still combat-capable when authority finalized this node. */
	UPROPERTY(BlueprintReadOnly)
	TArray<int32> SurvivingPlayerIds;
};

/** Synchronous boundary emitted before an in-memory run is discarded and reinitialized. */
USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11MemoryRunResetContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 CompletedMemoryRunSequence = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 NextMemoryRunSequence = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 CompletedRunSeed = 0;

	UPROPERTY(BlueprintReadOnly)
	FName RuleSetId = NAME_None;

	UPROPERTY(BlueprintReadOnly)
	FW11CombatNodeResult TerminalNode;
};

/** Client intent only. It deliberately contains no caster, ability id, damage or cost. */
USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11AbilityRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 EncounterInstanceId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EW11AbilitySlot Slot = EW11AbilitySlot::BasicAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D InputDirection = FVector2D(0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector_NetQuantize10 TargetPoint = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ClientRequestSequence = 0;
};

/** Server-only damage description. Never accepted from a client RPC. */
USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11DamageSpec
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<AActor> Source = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName AbilityId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float RawDamage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bCritical = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGameplayTagContainer DamageTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector_NetQuantize ImpactLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector_NetQuantizeNormal ImpactDirection = FVector::ForwardVector;
};

USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11DamageBreakdown
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float RawDamage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ArmorMitigated = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float BarrierAbsorbed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float HealthDamage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bImmune = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bDefeated = false;
};

/** Data-driven status payload owned by an ability or injected by a manual behavior. */
USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11StatusEffectSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName StatusId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EW11StatusType Type = EW11StatusType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.05"))
	float DurationSeconds = 2.0f;

	/** Burn damage per tick or Slow movement multiplier (0.65 = 35% slow). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0"))
	float Magnitude = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.05"))
	float TickIntervalSeconds = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1"))
	int32 MaxStacks = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EW11StatusStackingRule StackingRule = EW11StatusStackingRule::RefreshDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDispellable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
	float BossDurationScale = 0.5f;
};

/** Replicated status fact. Clients derive remaining time from EndServerTime. */
USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11ActiveStatus
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName StatusId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EW11StatusType Type = EW11StatusType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<AActor> Source = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName SourceAbilityId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Stacks = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Magnitude = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float TickIntervalSeconds = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float StartServerTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float EndServerTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float NextTickServerTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 MaxStacks = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EW11StatusStackingRule StackingRule = EW11StatusStackingRule::RefreshDuration;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bDispellable = true;
};

USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11EffectResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EW11EffectResultType Type = EW11EffectResultType::Damage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<AActor> Source = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName AbilityId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 StableTargetId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float RequestedAmount = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ActualAmount = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FW11DamageBreakdown Damage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bCritical = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName StatusId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EW11StatusType StatusType = EW11StatusType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 StatusStacks = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float StatusEndServerTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bStatusRefreshed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector_NetQuantize ImpactLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector_NetQuantizeNormal ImpactDirection = FVector::ForwardVector;
};

USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11AbilityExecutionResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 EncounterInstanceId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 ClientRequestSequence = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 AuthorityExecutionSequence = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bAccepted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EW11AbilityRejectionReason RejectionReason = EW11AbilityRejectionReason::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ServerCooldownEndTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<AActor> Source = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName AbilityId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EW11AbilitySlot Slot = EW11AbilitySlot::BasicAttack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FW11EffectResult> Effects;
};

USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11CombatCue
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 EncounterInstanceId = 0;

	/** Stable player or enemy id; actor references are not reliable identity keys across RPC consumers. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 SourceStableId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EW11CombatCueType Type = EW11CombatCueType::Impact;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 AuthorityExecutionSequence = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 EffectSequence = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<AActor> Source = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName AbilityId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector_NetQuantize Location = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector_NetQuantizeNormal Direction = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float PrimaryAmount = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float SecondaryAmount = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bCritical = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGameplayTag PresentationTag;
};

/** Bounded per-consumer filter for transient presentation RPCs. */
struct WORLDWALKERW11_API FW11CombatCueDeduplicator
{
	bool Accept(const FW11CombatCue& Cue)
	{
		if (Cue.EncounterInstanceId <= 0
			|| Cue.SourceStableId <= 0
			|| Cue.AuthorityExecutionSequence <= 0 || Cue.EffectSequence <= 0)
		{
			return false;
		}
		if (ObservedEncounterInstanceId != Cue.EncounterInstanceId)
		{
			Reset(Cue.EncounterInstanceId);
		}
		const FObservedKey Key{
			Cue.SourceStableId,
			Cue.AuthorityExecutionSequence,
			Cue.EffectSequence};
		if (ObservedKeys.Contains(Key))
		{
			return false;
		}
		ObservedKeys.Add(Key);
		ObservedOrder.Add(Key);
		if (ObservedOrder.Num() > MaxBufferedKeys)
		{
			ObservedKeys.Remove(ObservedOrder[0]);
			ObservedOrder.RemoveAt(0, 1, EAllowShrinking::No);
		}
		return true;
	}

	void Reset(const int32 EncounterInstanceId = 0)
	{
		ObservedEncounterInstanceId = EncounterInstanceId;
		ObservedKeys.Reset();
		ObservedOrder.Reset();
	}

	int32 NumBuffered() const { return ObservedKeys.Num(); }

private:
	struct FObservedKey
	{
		int32 SourceStableId = 0;
		int32 AuthorityExecutionSequence = 0;
		int32 EffectSequence = 0;

		bool operator==(const FObservedKey& Other) const
		{
			return SourceStableId == Other.SourceStableId
				&& AuthorityExecutionSequence == Other.AuthorityExecutionSequence
				&& EffectSequence == Other.EffectSequence;
		}

		friend uint32 GetTypeHash(const FObservedKey& Key)
		{
			uint32 Hash = HashCombineFast(
				GetTypeHash(Key.SourceStableId), GetTypeHash(Key.AuthorityExecutionSequence));
			return HashCombineFast(Hash, GetTypeHash(Key.EffectSequence));
		}
	};

	static constexpr int32 MaxBufferedKeys = 256;
	int32 ObservedEncounterInstanceId = 0;
	TSet<FObservedKey> ObservedKeys;
	TArray<FObservedKey> ObservedOrder;
};

/** Sound layers are collapsed per source execution so area hits do not multiply the same sound by target count. */
enum class EW11CombatAudioLayer : uint8
{
	Primary = 1 << 0,
	Critical = 1 << 1,
	Defeat = 1 << 2
};

/** Bounded per-consumer gate that permits at most one sound from each layer for an execution. */
struct WORLDWALKERW11_API FW11CombatAudioGate
{
	bool Accept(const FW11CombatCue& Cue, const EW11CombatAudioLayer Layer)
	{
		const uint8 LayerBit = static_cast<uint8>(Layer);
		if (Cue.EncounterInstanceId <= 0 || Cue.SourceStableId <= 0
			|| Cue.AuthorityExecutionSequence <= 0 || LayerBit == 0)
		{
			return false;
		}
		if (ObservedEncounterInstanceId != Cue.EncounterInstanceId)
		{
			Reset(Cue.EncounterInstanceId);
		}

		const FExecutionKey Key{Cue.SourceStableId, Cue.AuthorityExecutionSequence};
		if (uint8* ObservedLayerMask = ObservedLayers.Find(Key))
		{
			if ((*ObservedLayerMask & LayerBit) != 0)
			{
				return false;
			}
			*ObservedLayerMask |= LayerBit;
			return true;
		}

		ObservedLayers.Add(Key, LayerBit);
		ObservedOrder.Add(Key);
		if (ObservedOrder.Num() > MaxBufferedExecutions)
		{
			ObservedLayers.Remove(ObservedOrder[0]);
			ObservedOrder.RemoveAt(0, 1, EAllowShrinking::No);
		}
		return true;
	}

	void Reset(const int32 EncounterInstanceId = 0)
	{
		ObservedEncounterInstanceId = EncounterInstanceId;
		ObservedLayers.Reset();
		ObservedOrder.Reset();
	}

	int32 NumBuffered() const { return ObservedLayers.Num(); }

private:
	struct FExecutionKey
	{
		int32 SourceStableId = 0;
		int32 AuthorityExecutionSequence = 0;

		bool operator==(const FExecutionKey& Other) const
		{
			return SourceStableId == Other.SourceStableId
				&& AuthorityExecutionSequence == Other.AuthorityExecutionSequence;
		}

		friend uint32 GetTypeHash(const FExecutionKey& Key)
		{
			return HashCombineFast(
				GetTypeHash(Key.SourceStableId), GetTypeHash(Key.AuthorityExecutionSequence));
		}
	};

	static constexpr int32 MaxBufferedExecutions = 256;
	int32 ObservedEncounterInstanceId = 0;
	TMap<FExecutionKey, uint8> ObservedLayers;
	TArray<FExecutionKey> ObservedOrder;
};

/** Recoverable, replicated enemy attack state; clients derive visuals from server time. */
USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11ReplicatedAttackState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 EncounterInstanceId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 AttackInstanceId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName AttackDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EW11EnemyCombatState State = EW11EnemyCombatState::AcquireTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 LockedTargetStableId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector_NetQuantizeNormal LockedDirection = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector_NetQuantize10 LockedTargetPoint = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EW11TelegraphShape TelegraphShape = EW11TelegraphShape::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float TelegraphRange = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float TelegraphRadius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float WindupStartServerTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ActiveStartServerTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float RecoveryEndServerTime = 0.0f;
};
