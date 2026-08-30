#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Core/W11Types.h"
#include "W11Definitions.generated.h"

class UPaperFlipbook;
class UTexture2D;
class UWorld;

USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11CharacterActionFlipbooks
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
	TSoftObjectPtr<UPaperFlipbook> Idle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
	TSoftObjectPtr<UPaperFlipbook> Move;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
	TSoftObjectPtr<UPaperFlipbook> Dodge;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
	TSoftObjectPtr<UPaperFlipbook> Cast;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
	TSoftObjectPtr<UPaperFlipbook> Hit;
};

USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11ManualRankEffects
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EW11ComprehensionRank Rank = EW11ComprehensionRank::Initiate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FW11StatModifier> StatModifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer GrantedTags;

	/** Stable behavior ids such as ManualBehavior.ActiveBurn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> GrantedBehaviors;
};

USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11WeightedEnemyEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<class UW11EnemyDefinition> Enemy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1"))
	int32 SpawnCost = 1;
};

UCLASS(Abstract, BlueprintType)
class WORLDWALKERW11_API UW11DefinitionBase : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AssetRegistrySearchable, Category="Identity")
	FName DefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
	FString ContentVersion = TEXT("W11-M0-v1");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TSoftObjectPtr<UTexture2D> Icon;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType)
class WORLDWALKERW11_API UW11StatChoiceDefinition : public UW11DefinitionBase
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cultivation")
	TArray<FW11StatModifier> Modifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cultivation", meta=(ClampMin="0.0"))
	float OfferWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cultivation", meta=(ClampMin="0"))
	int32 MaxSelectionsPerRun = 0;
};

UCLASS(BlueprintType)
class WORLDWALKERW11_API UW11ManualDefinition : public UW11DefinitionBase
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Manual")
	EW11ManualCategory Category = EW11ManualCategory::BodyCultivation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Manual")
	EW11Rarity Rarity = EW11Rarity::Yellow;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Manual", meta=(ClampMin="0"))
	int32 BasePrice = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Manual")
	TArray<FW11ManualRankEffects> RankEffects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Manual")
	FGameplayTagContainer ManualTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Manual")
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Manual")
	FGameplayTagContainer BlockedTags;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType)
class WORLDWALKERW11_API UW11TreasureDefinition : public UW11DefinitionBase
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Treasure")
	EW11TreasureCategory Category = EW11TreasureCategory::Curio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Treasure")
	EW11Rarity Rarity = EW11Rarity::Yellow;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Treasure", meta=(ClampMin="0"))
	int32 BasePrice = 150;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Treasure")
	bool bUnique = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Treasure")
	bool bConsumable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Treasure")
	TArray<FW11StatModifier> StatModifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Treasure")
	FGameplayTagContainer TreasureTags;
};

UCLASS(BlueprintType)
class WORLDWALKERW11_API UW11ShopServiceDefinition : public UW11DefinitionBase
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Service", meta=(ClampMin="0"))
	int32 BasePrice = 60;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Service")
	TArray<FW11StatModifier> PermanentModifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Service", meta=(ClampMin="0.0"))
	float HealFraction = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Service", meta=(ClampMin="0.0"))
	float RestoreManaFraction = 0.0f;
};

UCLASS(BlueprintType)
class WORLDWALKERW11_API UW11AbilityDefinition : public UW11DefinitionBase
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability|Contract")
	EW11AbilitySlot LogicalSlot = EW11AbilitySlot::Active1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability|Contract")
	EW11AbilityTargetMode TargetMode = EW11AbilityTargetMode::Direction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability|Contract")
	EW11HitShape HitShape = EW11HitShape::Line;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability|Contract")
	EW11HitDelivery Delivery = EW11HitDelivery::Immediate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability|Contract")
	EW11CooldownScaling CooldownScaling = EW11CooldownScaling::AbilityHaste;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability|Contract")
	EW11CriticalRollPolicy CriticalRollPolicy = EW11CriticalRollPolicy::PerExecution;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
	EW11AbilityArchetype Archetype = EW11AbilityArchetype::DirectedSweep;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability", meta=(ClampMin="0.0"))
	float BaseDamage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability", meta=(ClampMin="0.0"))
	float PowerCoefficient = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability", meta=(ClampMin="0.0"))
	float ManaCost = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability", meta=(ClampMin="0.05"))
	float Cooldown = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability", meta=(ClampMin="0.0"))
	float BaseRange = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability", meta=(ClampMin="0.0"))
	float BaseRadius = 48.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability", meta=(ClampMin="0.0", ClampMax="360.0"))
	float ConeAngleDegrees = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability", meta=(ClampMin="0.0"))
	float ProjectileSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability", meta=(ClampMin="1"))
	int32 MaxTargets = 16;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
	bool bAllowRepeatHits = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
	bool bPiercesTargets = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability|Timing", meta=(ClampMin="0.0"))
	float WindupSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability|Timing", meta=(ClampMin="0.0"))
	float ActiveSeconds = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability|Timing", meta=(ClampMin="0.0"))
	float RecoverySeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability|Enemy")
	EW11TelegraphShape TelegraphShape = EW11TelegraphShape::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability|Enemy", meta=(ClampMin="0.0"))
	float PreferredRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability|Enemy", meta=(ClampMin="0.05"))
	float ProjectileLifetime = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability", meta=(ClampMin="0.0"))
	float BarrierAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability", meta=(ClampMin="0.0"))
	float HealAmount = 0.0f;

	/** Optional reliable owner result; may accompany any archetype. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability", meta=(ClampMin="0.0"))
	float ManaRestoreAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
	FGameplayTagContainer AbilityTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability|Status")
	TArray<FW11StatusEffectSpec> StatusEffects;

	// Presentation is intentionally a separate data asset. Combat ranges, damage and
	// authority never depend on the flipbook dimensions or playback timing.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TSoftObjectPtr<class UW11SkillVisualDefinition> Visuals;
};

UCLASS(BlueprintType)
class WORLDWALKERW11_API UW11CharacterAnimationSet : public UW11DefinitionBase
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation|Facing")
	FW11CharacterActionFlipbooks Right;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation|Facing")
	FW11CharacterActionFlipbooks Left;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation|Facing")
	FW11CharacterActionFlipbooks Up;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation|Facing")
	FW11CharacterActionFlipbooks Down;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation|Fallback")
	bool bKeepLastHorizontalFacingForVerticalMovement = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation", meta=(ClampMin="0.01"))
	float WorldScale = 0.72f;
};

UCLASS(BlueprintType)
class WORLDWALKERW11_API UW11SkillVisualDefinition : public UW11DefinitionBase
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TSoftObjectPtr<UPaperFlipbook> EffectFlipbook;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TSoftObjectPtr<UPaperFlipbook> ImpactFlipbook;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio")
	TSoftObjectPtr<class USoundBase> CastSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio")
	TSoftObjectPtr<class USoundBase> ImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio")
	TSoftObjectPtr<class USoundBase> CriticalSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio")
	TSoftObjectPtr<class USoundBase> DefeatSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	bool bDirectional = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	EW11FacingDirection AuthoredDirection = EW11FacingDirection::Right;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	bool bAttachToCaster = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	FVector2D LocalOffset = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual", meta=(ClampMin="0.01"))
	float WorldScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	int32 TranslucentSortPriority = 40;

	/** Local presentation-only Flipbook hold. Never changes world or authority time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Impact", meta=(ClampMin="0.0", ClampMax="0.12"))
	float HitStopSeconds = 0.025f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Impact", meta=(ClampMin="0.0", ClampMax="0.12"))
	float CriticalHitStopSeconds = 0.045f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Impact", meta=(ClampMin="0.0", ClampMax="0.12"))
	float DefeatHitStopSeconds = 0.065f;

	/** Maximum local camera displacement in Unreal units for each impact tier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Impact", meta=(ClampMin="0.0", ClampMax="8.0"))
	float CameraImpactStrength = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Impact", meta=(ClampMin="0.0", ClampMax="8.0"))
	float CriticalCameraImpactStrength = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Impact", meta=(ClampMin="0.0", ClampMax="8.0"))
	float DefeatCameraImpactStrength = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation|Impact", meta=(ClampMin="0.01", ClampMax="0.30"))
	float CameraImpactDuration = 0.12f;

	float ResolveHitStopSeconds(EW11CombatCueType CueType, bool bCritical) const;
	float ResolveCameraImpactStrength(EW11CombatCueType CueType, bool bCritical) const;
};

UCLASS(BlueprintType)
class WORLDWALKERW11_API UW11HeroDefinition : public UW11DefinitionBase
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hero")
	EW11HeroGender Gender = EW11HeroGender::Male;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hero")
	FText Epithet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hero", meta=(MultiLine="true"))
	FText Biography;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hero")
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hero|Presentation")
	TSoftObjectPtr<UTexture2D> Avatar;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hero|Presentation")
	TSoftObjectPtr<UW11CharacterAnimationSet> CombatAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hero")
	TArray<FW11StatModifier> StartingModifiers;
};

UCLASS(BlueprintType)
class WORLDWALKERW11_API UW11SectDefinition : public UW11DefinitionBase
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sect", meta=(MultiLine="true"))
	FText Description;

	/** Short cultivation identity shown below the selection artwork. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sect")
	FText StyleTagline;

	/** Full-bleed 16:9 artwork used by the fullscreen sect-selection cards. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sect|Presentation")
	TSoftObjectPtr<UTexture2D> SelectionArtwork;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sect")
	TSoftObjectPtr<UW11AbilityDefinition> InitialAbility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sect")
	FLinearColor AccentColor = FLinearColor(0.2f, 0.8f, 0.7f, 1.0f);
};

USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11BossPhaseDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Phase")
	FName PhaseId = NAME_None;

	/** Switch into this phase at or below this remaining-health fraction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Phase", meta=(ClampMin="0.01", ClampMax="0.99"))
	float EnterAtHealthFraction = 0.66f;

	/** Stable, auditable mechanic identity; phases may never be health-only multipliers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Phase")
	FName MechanicId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss Phase")
	TArray<TSoftObjectPtr<UW11AbilityDefinition>> Abilities;
};

UCLASS(BlueprintType)
class WORLDWALKERW11_API UW11EnemyDefinition : public UW11DefinitionBase
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy")
	FW11StatBlock BaseStats;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy", meta=(ClampMin="1"))
	int32 SpawnCost = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy")
	bool bElite = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy")
	bool bBoss = false;

	/** Formal chapter/final bosses must supply auditable mechanic-bearing phases. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Boss")
	bool bFormalBoss = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Boss")
	FName InitialBossPhaseId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Boss")
	FName InitialBossMechanicId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Boss")
	TArray<FW11BossPhaseDefinition> BossPhases;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy")
	TArray<TSoftObjectPtr<UW11AbilityDefinition>> Abilities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TSoftObjectPtr<class UPaperFlipbook> IdleFlipbook;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TSoftObjectPtr<class UPaperSprite> IdleSprite;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TSoftObjectPtr<UPaperFlipbook> MoveFlipbook;

	/** Optional left-facing movement cycle. Null keeps the right-facing cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TSoftObjectPtr<UPaperFlipbook> MoveFlipbookLeft;
};

UCLASS(BlueprintType)
class WORLDWALKERW11_API UW11EncounterDefinition : public UW11DefinitionBase
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
	EW11EncounterType EncounterType = EW11EncounterType::Clear;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
	EW11EncounterCompletionRule CompletionRule = EW11EncounterCompletionRule::EliminateAll;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter", meta=(ClampMin="1"))
	int32 StageIndex = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter", meta=(ClampMin="1"))
	int32 SpawnBudget = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter", meta=(ClampMin="1.0"))
	float DurationSeconds = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter|Waves", meta=(ClampMin="1"))
	int32 WaveCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter|Waves", meta=(ClampMin="0.0"))
	float InterWaveDelaySeconds = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter|Spawning", meta=(ClampMin="1"))
	int32 MaxConcurrentEnemies = 24;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter|Spawning", meta=(ClampMin="0.0"))
	float SpawnSafeRadius = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter|Spawning", meta=(ClampMin="1.0"))
	float SpawnOuterRadius = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter|Scaling", meta=(ClampMin="0.0"))
	float BudgetScalePerAdditionalPlayer = 0.35f;

	/** Per-encounter baseline enemy durability, kept separate from co-op scaling. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter|Scaling", meta=(ClampMin="0.01"))
	float EnemyBaseHealthScale = 1.0f;

	/** Per-encounter baseline outgoing power, allowing pacing and lethality to be tuned independently. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter|Scaling", meta=(ClampMin="0.0"))
	float EnemyBasePowerScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter|Scaling", meta=(ClampMin="0.0"))
	float HealthScalePerAdditionalPlayer = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
	TArray<FW11WeightedEnemyEntry> EnemyPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Encounter")
	FW11BattleReward Reward;
};

USTRUCT(BlueprintType)
struct WORLDWALKERW11_API FW11ChapterRouteNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chapter")
	FName NodeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chapter")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chapter")
	EW11ChapterNodeType NodeType = EW11ChapterNodeType::Combat;

	/** Required for Combat, SmallBoss and FinalBoss nodes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chapter")
	TSoftObjectPtr<UW11EncounterDefinition> Encounter;

	/** Rest, market and explicitly authored route boundaries may persist an activity snapshot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chapter")
	bool bSafeNode = false;

	/** Optional branch identity; empty means the node belongs to the mandatory route. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chapter")
	FName BranchId = NAME_None;
};

UCLASS(BlueprintType)
class WORLDWALKERW11_API UW11ChapterDefinition : public UW11DefinitionBase
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chapter", meta=(ClampMin="1", ClampMax="5"))
	int32 ChapterIndex = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chapter")
	TSoftObjectPtr<UWorld> ChapterMap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chapter")
	TArray<FW11ChapterRouteNode> Nodes;
};

UCLASS(BlueprintType)
class WORLDWALKERW11_API UW11ChapterRouteDefinition : public UW11DefinitionBase
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chapter")
	TArray<TSoftObjectPtr<UW11ChapterDefinition>> Chapters;

	/** The confirmed three-phase Gu Changyuan encounter after all five chapter gates. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Chapter")
	TSoftObjectPtr<UW11EncounterDefinition> FinalBossEncounter;
};

UCLASS(BlueprintType)
class WORLDWALKERW11_API UW11RunRuleSet : public UW11DefinitionBase
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run|Versioning")
	FString RuleVersion = TEXT("W11-Combat-v1");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run|Versioning", meta=(ClampMin="1"))
	int32 CombatLogSchemaVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run", meta=(ClampMin="1", ClampMax="4"))
	int32 MaxPlayers = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run", meta=(ClampMin="1"))
	int32 ManualSlots = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run", meta=(ClampMin="1"))
	int32 TreasureSlots = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run", meta=(ClampMin="1"))
	int32 CultivationOfferCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run", meta=(ClampMin="1"))
	int32 ManualShopSlots = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run", meta=(ClampMin="0"))
	int32 TreasureShopSlots = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run", meta=(ClampMin="0"))
	int32 ServiceShopSlots = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run", meta=(ClampMin="0"))
	int32 AbilityShopSlots = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run")
	FW11StatBlock DefaultPlayerStats;

	/** Shared, mandatory basic attack. It never occupies an active ability slot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run|Combat")
	TSoftObjectPtr<UW11AbilityDefinition> BasicAttackAbility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run|Combat|Dodge", meta=(ClampMin="1.0"))
	float DodgeDistance = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run|Combat|Dodge", meta=(ClampMin="0.01"))
	float DodgeDuration = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run|Combat|Dodge", meta=(ClampMin="0.01"))
	float DodgeCooldown = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run|Combat|Dodge", meta=(ClampMin="0.0"))
	float DodgeInvulnerabilityDuration = 0.20f;

	/** Character-independent active ability pool available for run unlocks/upgrades. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run|Combat")
	TArray<TSoftObjectPtr<UW11AbilityDefinition>> ActiveAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run")
	TArray<TSoftObjectPtr<UW11StatChoiceDefinition>> StatChoices;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run")
	TArray<TSoftObjectPtr<UW11ManualDefinition>> Manuals;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run")
	TArray<TSoftObjectPtr<UW11TreasureDefinition>> Treasures;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run")
	TArray<TSoftObjectPtr<UW11ShopServiceDefinition>> Services;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run")
	TArray<TSoftObjectPtr<UW11EncounterDefinition>> Encounters;

	/** Optional formal chapter route. Null preserves the arena compatibility flow. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run|Chapter")
	TSoftObjectPtr<UW11ChapterRouteDefinition> ChapterRoute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run|Character Creation")
	TArray<TSoftObjectPtr<UW11HeroDefinition>> Heroes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Run|Character Creation")
	TArray<TSoftObjectPtr<UW11SectDefinition>> Sects;
};
