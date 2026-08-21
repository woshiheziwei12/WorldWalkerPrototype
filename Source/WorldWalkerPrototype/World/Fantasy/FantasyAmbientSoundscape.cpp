#include "World/Fantasy/FantasyAmbientSoundscape.h"

#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

namespace
{
	const TCHAR* FireLoopObjectPath = TEXT(
		"/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/OpenGameArt/Audio/"
		"SW_W01_FireLoop.SW_W01_FireLoop");
	const TCHAR* AmbientAudioRoot = TEXT(
		"/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/OpenGameArt/Audio");

	constexpr float InitialAmbientDelayMin = 9.0f;
	constexpr float InitialAmbientDelayMax = 16.0f;
	constexpr float AmbientGapMin = 18.0f;
	constexpr float AmbientGapMax = 34.0f;
}

AFantasyAmbientSoundscape::AFantasyAmbientSoundscape()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// These positions coincide with the two village campfires and the two
	// Blackthorn courtyard braziers assembled by the W01 presentation actors.
	const FVector FireLocations[] = {
		FVector(860.0f, -210.0f, 82.0f),
		FVector(1420.0f, 220.0f, 82.0f),
		FVector(2390.0f, -455.0f, 95.0f),
		FVector(2390.0f, 455.0f, 95.0f)};

	for (int32 FireIndex = 0; FireIndex < UE_ARRAY_COUNT(FireLocations); ++FireIndex)
	{
		UAudioComponent* FireComponent = CreateDefaultSubobject<UAudioComponent>(
			FName(*FString::Printf(TEXT("FireLoop_%02d"), FireIndex)));
		FireComponent->SetupAttachment(SceneRoot);
		FireComponent->SetRelativeLocation(FireLocations[FireIndex]);
		FireComponent->SetMobility(EComponentMobility::Movable);
		FireComponent->bAutoActivate = false;
		FireComponent->bAllowSpatialization = true;
		FireComponent->bStopWhenOwnerDestroyed = true;
		FireComponent->bShouldRemainActiveIfDropped = true;
		ConfigureSpatialAudio(FireComponent, 85.0f, 760.0f);
		FireLoopComponents.Add(FireComponent);
	}

	AmbientCueComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AmbientCue"));
	AmbientCueComponent->SetupAttachment(SceneRoot);
	AmbientCueComponent->SetMobility(EComponentMobility::Movable);
	AmbientCueComponent->bAutoActivate = false;
	AmbientCueComponent->bAllowSpatialization = true;
	AmbientCueComponent->bStopWhenOwnerDestroyed = true;
	AmbientCueComponent->bShouldRemainActiveIfDropped = true;
	ConfigureSpatialAudio(AmbientCueComponent, 260.0f, 1850.0f);

	ExplorationMusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("ExplorationMusic"));
	ExplorationMusicComponent->SetupAttachment(SceneRoot);
	ExplorationMusicComponent->bAutoActivate = false;
	ExplorationMusicComponent->bAllowSpatialization = false;
	ExplorationMusicComponent->bStopWhenOwnerDestroyed = true;

	BattleMusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("BattleMusic"));
	BattleMusicComponent->SetupAttachment(SceneRoot);
	BattleMusicComponent->bAutoActivate = false;
	BattleMusicComponent->bAllowSpatialization = false;
	BattleMusicComponent->bStopWhenOwnerDestroyed = true;

	CardSfxComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("CardSfx"));
	CardSfxComponent->SetupAttachment(SceneRoot);
	CardSfxComponent->bAutoActivate = false;
	CardSfxComponent->bAllowSpatialization = false;
	CardSfxComponent->bStopWhenOwnerDestroyed = true;

	ActionSfxComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("ActionSfx"));
	ActionSfxComponent->SetupAttachment(SceneRoot);
	ActionSfxComponent->bAutoActivate = false;
	ActionSfxComponent->bAllowSpatialization = false;
	ActionSfxComponent->bStopWhenOwnerDestroyed = true;
}

void AFantasyAmbientSoundscape::BeginPlay()
{
	Super::BeginPlay();

	FireLoopSound = LoadOptionalSound(FireLoopObjectPath, TEXT("fire_loop"));
	int32 PlayingFireLoops = 0;
	if (FireLoopSound)
	{
		const float FireDuration = FireLoopSound->GetDuration();
		for (int32 FireIndex = 0; FireIndex < FireLoopComponents.Num(); ++FireIndex)
		{
			UAudioComponent* FireComponent = FireLoopComponents[FireIndex];
			if (!FireComponent)
			{
				continue;
			}

			FireComponent->SetSound(FireLoopSound);
			// The CC0 fireplace recording has generous headroom (roughly -42 dB RMS).
			// Lift it here so the flame is audible at close range while attenuation
			// still keeps it well below dialogue away from the brazier.
			FireComponent->SetVolumeMultiplier(1.55f + 0.05f * static_cast<float>(FireIndex % 3));
			FireComponent->SetPitchMultiplier(0.965f + 0.022f * static_cast<float>(FireIndex));

			float StartOffset = 0.0f;
			if (FMath::IsFinite(FireDuration) && FireDuration > 1.0f)
			{
				StartOffset = FMath::Fmod(1.37f * static_cast<float>(FireIndex), FireDuration);
			}
			FireComponent->Play(StartOffset);
			++PlayingFireLoops;
		}
	}

	for (int32 AmbientIndex = 1; AmbientIndex <= 5; ++AmbientIndex)
	{
		const FString ObjectPath = FString::Printf(
			TEXT("%s/SW_W01_DarkAmbience_%02d.SW_W01_DarkAmbience_%02d"),
			AmbientAudioRoot,
			AmbientIndex,
			AmbientIndex);
		if (USoundBase* AmbientSound = LoadOptionalSound(ObjectPath, TEXT("dark_ambience")))
		{
			AmbientCueSounds.Add(AmbientSound);
		}
	}

	ExplorationMusic = LoadOptionalSound(
		FString::Printf(TEXT("%s/SW_W01_ExplorationMusic.SW_W01_ExplorationMusic"), AmbientAudioRoot),
		TEXT("exploration_music"));
	BattleMusic = LoadOptionalSound(
		FString::Printf(TEXT("%s/SW_W01_BattleMusic.SW_W01_BattleMusic"), AmbientAudioRoot),
		TEXT("battle_music"));
	CardPlaySound = LoadOptionalSound(
		FString::Printf(TEXT("%s/SW_W01_CardPlay.SW_W01_CardPlay"), AmbientAudioRoot),
		TEXT("card_play"));
	CardDrawSound = LoadOptionalSound(
		FString::Printf(TEXT("%s/SW_W01_CardDraw.SW_W01_CardDraw"), AmbientAudioRoot),
		TEXT("card_draw"));
	CardShuffleSound = LoadOptionalSound(
		FString::Printf(TEXT("%s/SW_W01_CardShuffle.SW_W01_CardShuffle"), AmbientAudioRoot),
		TEXT("card_shuffle"));
	TurnPassSound = LoadOptionalSound(
		FString::Printf(TEXT("%s/SW_W01_TurnPass.SW_W01_TurnPass"), AmbientAudioRoot),
		TEXT("turn_pass"));
	RouteSound = LoadOptionalSound(
		FString::Printf(TEXT("%s/SW_W01_RouteChoice.SW_W01_RouteChoice"), AmbientAudioRoot),
		TEXT("route_choice"));
	AttackSound = LoadOptionalSound(
		FString::Printf(TEXT("%s/SW_W01_Attack.SW_W01_Attack"), AmbientAudioRoot),
		TEXT("attack"));
	SpellSound = LoadOptionalSound(
		FString::Printf(TEXT("%s/SW_W01_Spell.SW_W01_Spell"), AmbientAudioRoot),
		TEXT("spell"));
	DefenseSound = LoadOptionalSound(
		FString::Printf(TEXT("%s/SW_W01_Defense.SW_W01_Defense"), AmbientAudioRoot),
		TEXT("defense"));

	if (ExplorationMusic && ExplorationMusicComponent)
	{
		ExplorationMusicComponent->SetSound(ExplorationMusic);
		ExplorationMusicComponent->SetVolumeMultiplier(0.0f);
		ExplorationMusicComponent->Play();
		ExplorationMusicComponent->FadeIn(1.6f, 0.24f, 0.0f);
	}
	if (BattleMusic && BattleMusicComponent)
	{
		BattleMusicComponent->SetSound(BattleMusic);
		BattleMusicComponent->SetVolumeMultiplier(0.0f);
	}

	// All cues remain outside the central movement lane. Their broad falloff
	// makes them audible while travelling without sounding attached to the
	// player or competing with NPC conversations.
	AmbientCueLocations = {
		FVector(-360.0f, -980.0f, 190.0f),
		FVector(260.0f, 1080.0f, 220.0f),
		FVector(820.0f, -1120.0f, 180.0f),
		FVector(1200.0f, 1030.0f, 235.0f),
		FVector(1580.0f, -1060.0f, 210.0f),
		FVector(1840.0f, 1010.0f, 190.0f),
		FVector(2160.0f, -1080.0f, 245.0f),
		FVector(2700.0f, 1040.0f, 225.0f)};

	if (!AmbientCueSounds.IsEmpty())
	{
		ScheduleNextAmbientCue(FMath::FRandRange(InitialAmbientDelayMin, InitialAmbientDelayMax));
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_AMBIENT_SOUNDSCAPE_READY FireAsset=%d/1 FireLoops=%d/4 Ambience=%d/5 CuePoints=%d Music=%d/2 CardSfx=%d/8"),
		FireLoopSound ? 1 : 0,
		PlayingFireLoops,
		AmbientCueSounds.Num(),
		AmbientCueLocations.Num(),
		(ExplorationMusic ? 1 : 0) + (BattleMusic ? 1 : 0),
		(CardPlaySound ? 1 : 0) + (CardDrawSound ? 1 : 0)
			+ (CardShuffleSound ? 1 : 0) + (TurnPassSound ? 1 : 0)
			+ (RouteSound ? 1 : 0) + (AttackSound ? 1 : 0)
			+ (SpellSound ? 1 : 0) + (DefenseSound ? 1 : 0));
}

void AFantasyAmbientSoundscape::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(AmbientCueTimer);
	}

	if (AmbientCueComponent)
	{
		AmbientCueComponent->Stop();
	}
	for (UAudioComponent* Component : {
		ExplorationMusicComponent.Get(),
		BattleMusicComponent.Get(),
		CardSfxComponent.Get(),
		ActionSfxComponent.Get()})
	{
		if (Component)
		{
			Component->Stop();
		}
	}
	for (UAudioComponent* FireComponent : FireLoopComponents)
	{
		if (FireComponent)
		{
			FireComponent->Stop();
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AFantasyAmbientSoundscape::SetBattleMusicActive(const bool bBattleActive)
{
	if (bBattleActive)
	{
		if (ExplorationMusicComponent && ExplorationMusicComponent->IsPlaying())
		{
			ExplorationMusicComponent->FadeOut(0.8f, 0.0f);
		}
		if (BattleMusicComponent && BattleMusic)
		{
			BattleMusicComponent->SetSound(BattleMusic);
			BattleMusicComponent->FadeIn(0.9f, 0.34f, 0.0f);
		}
	}
	else
	{
		if (BattleMusicComponent && BattleMusicComponent->IsPlaying())
		{
			BattleMusicComponent->FadeOut(0.9f, 0.0f);
		}
		if (ExplorationMusicComponent && ExplorationMusic)
		{
			ExplorationMusicComponent->SetSound(ExplorationMusic);
			ExplorationMusicComponent->FadeIn(1.2f, 0.24f, 0.0f);
		}
	}
	UE_LOG(LogTemp, Display, TEXT("W01_MUSIC_STATE Mode=%s"), bBattleActive ? TEXT("Battle") : TEXT("Exploration"));
}

void AFantasyAmbientSoundscape::PlayCardCue(
	const EFantasyAudioCue Cue,
	const bool bEnemy)
{
	if (CardSfxComponent && CardPlaySound)
	{
		CardSfxComponent->SetSound(CardPlaySound);
		CardSfxComponent->SetVolumeMultiplier(bEnemy ? 0.64f : 0.78f);
		CardSfxComponent->SetPitchMultiplier(bEnemy ? 0.91f : 1.02f);
		CardSfxComponent->Play();
	}

	USoundBase* ActionSound = nullptr;
	switch (Cue)
	{
	case EFantasyAudioCue::Attack: ActionSound = AttackSound; break;
	case EFantasyAudioCue::Spell: ActionSound = SpellSound; break;
	case EFantasyAudioCue::Defense:
	case EFantasyAudioCue::Equipment:
	case EFantasyAudioCue::Counter: ActionSound = DefenseSound; break;
	default: break;
	}
	if (ActionSfxComponent && ActionSound)
	{
		ActionSfxComponent->SetSound(ActionSound);
		ActionSfxComponent->SetVolumeMultiplier(bEnemy ? 0.56f : 0.68f);
		ActionSfxComponent->SetPitchMultiplier(bEnemy ? 0.90f : 1.0f);
		ActionSfxComponent->Play();
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_CARD_AUDIO_CUE Side=%s Cue=%d CardLayer=%d ActionLayer=%d"),
		bEnemy ? TEXT("Enemy") : TEXT("Player"),
		static_cast<int32>(Cue),
		CardPlaySound ? 1 : 0,
		ActionSound ? 1 : 0);
}

void AFantasyAmbientSoundscape::PlayInterfaceCue(const EFantasyAudioCue Cue)
{
	USoundBase* InterfaceSound = nullptr;
	switch (Cue)
	{
	case EFantasyAudioCue::Draw: InterfaceSound = CardDrawSound; break;
	case EFantasyAudioCue::TurnEnd: InterfaceSound = TurnPassSound; break;
	case EFantasyAudioCue::Reward: InterfaceSound = CardShuffleSound; break;
	case EFantasyAudioCue::Route: InterfaceSound = RouteSound; break;
	default: break;
	}
	if (CardSfxComponent && InterfaceSound)
	{
		CardSfxComponent->SetSound(InterfaceSound);
		CardSfxComponent->SetVolumeMultiplier(0.68f);
		CardSfxComponent->SetPitchMultiplier(1.0f);
		CardSfxComponent->Play();
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("W01_INTERFACE_AUDIO_CUE Cue=%d Loaded=%d"),
		static_cast<int32>(Cue),
		InterfaceSound ? 1 : 0);
}

void AFantasyAmbientSoundscape::ConfigureSpatialAudio(
	UAudioComponent* AudioComponent,
	const float InnerRadius,
	const float FalloffDistance) const
{
	if (!AudioComponent)
	{
		return;
	}

	FSoundAttenuationSettings Attenuation;
	Attenuation.bAttenuate = true;
	Attenuation.bSpatialize = true;
	Attenuation.bApplyNormalizationToStereoSounds = true;
	Attenuation.bEnableOcclusion = false;
	Attenuation.bEnableSendToAudioLink = false;
	Attenuation.DistanceAlgorithm = EAttenuationDistanceModel::Logarithmic;
	Attenuation.AttenuationShape = EAttenuationShape::Sphere;
	Attenuation.AttenuationShapeExtents = FVector(InnerRadius, 0.0f, 0.0f);
	Attenuation.FalloffDistance = FalloffDistance;
	Attenuation.SpatializationAlgorithm = ESoundSpatializationAlgorithm::SPATIALIZATION_Default;

	AudioComponent->SetOverrideAttenuation(true);
	AudioComponent->SetAttenuationOverrides(Attenuation);
}

USoundBase* AFantasyAmbientSoundscape::LoadOptionalSound(
	const FString& ObjectPath,
	const TCHAR* LayerName) const
{
	USoundBase* Sound = LoadObject<USoundBase>(nullptr, *ObjectPath);
	if (!Sound)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("W01_AMBIENT_AUDIO_OPTIONAL_ASSET_MISSING Layer=%s Asset=%s"),
			LayerName ? LayerName : TEXT("unknown"),
			*ObjectPath);
	}
	return Sound;
}

void AFantasyAmbientSoundscape::ScheduleNextAmbientCue(const float DelaySeconds)
{
	if (!GetWorld() || AmbientCueSounds.IsEmpty() || AmbientCueLocations.IsEmpty())
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		AmbientCueTimer,
		this,
		&AFantasyAmbientSoundscape::PlayAmbientCue,
		FMath::Max(0.1f, DelaySeconds),
		false);
}

void AFantasyAmbientSoundscape::PlayAmbientCue()
{
	if (!AmbientCueComponent || AmbientCueSounds.IsEmpty() || AmbientCueLocations.IsEmpty())
	{
		return;
	}

	int32 CueIndex = FMath::RandRange(0, AmbientCueSounds.Num() - 1);
	if (AmbientCueSounds.Num() > 1 && CueIndex == LastAmbientCueIndex)
	{
		CueIndex = (CueIndex + FMath::RandRange(1, AmbientCueSounds.Num() - 1))
			% AmbientCueSounds.Num();
	}
	LastAmbientCueIndex = CueIndex;

	const int32 LocationIndex = FMath::RandRange(0, AmbientCueLocations.Num() - 1);
	USoundBase* CueSound = AmbientCueSounds[CueIndex];
	if (!CueSound)
	{
		ScheduleNextAmbientCue(AmbientGapMin);
		return;
	}

	AmbientCueComponent->SetRelativeLocation(AmbientCueLocations[LocationIndex]);
	AmbientCueComponent->SetSound(CueSound);
	AmbientCueComponent->SetVolumeMultiplier(FMath::FRandRange(0.16f, 0.22f));
	AmbientCueComponent->SetPitchMultiplier(FMath::FRandRange(0.95f, 1.035f));
	AmbientCueComponent->Play();

	const float RawDuration = CueSound->GetDuration();
	const float CueDuration = FMath::IsFinite(RawDuration) && RawDuration > 0.0f
		? FMath::Clamp(RawDuration, 3.0f, 45.0f)
		: 12.0f;
	const float NextDelay = CueDuration + FMath::FRandRange(AmbientGapMin, AmbientGapMax);
	ScheduleNextAmbientCue(NextDelay);

	UE_LOG(
		LogTemp,
		Verbose,
		TEXT("W01_AMBIENT_AUDIO_CUE Sound=%s Point=%d NextDelay=%.1f"),
		*GetNameSafe(CueSound),
		LocationIndex,
		NextDelay);
}
