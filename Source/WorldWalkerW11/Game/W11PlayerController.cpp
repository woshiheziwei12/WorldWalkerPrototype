#include "Game/W11PlayerController.h"

#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Game/W11GameMode.h"
#include "Game/W11GameState.h"
#include "Game/W11PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "Online/W11SessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "Sound/SoundBase.h"
#include "UI/W11EntryMenuWidget.h"
#include "World/WorldTravelSubsystem.h"
#include "World/WorldDefinition.h"
#include "WorldWalkerW11.h"

namespace W11EntrySettings
{
	static const TCHAR* ConfigSection = TEXT("/Script/WorldWalkerW11.W11EntryMenuSettings");
	static const TCHAR* MusicVolumeKey = TEXT("MusicVolume");
	static const TCHAR* TitleMusicPath =
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Audio/Music/S_W11_TaixuMoonGate_MenuTheme.S_W11_TaixuMoonGate_MenuTheme");
	static const TCHAR* HeroSelectionMusicPath =
		TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Audio/Music/S_W11_MirrorOfLives_HeroSelectionTheme.S_W11_MirrorOfLives_HeroSelectionTheme");
}

AW11PlayerController::AW11PlayerController()
{
	bShowMouseCursor = false;
}

void AW11PlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AW11PlayerController::InitializeEntryFlow);
	}
}

void AW11PlayerController::InitializeEntryFlow()
{
	const AW11GameState* State = GetWorld() ? GetWorld()->GetGameState<AW11GameState>() : nullptr;
	if (!State || State->GetRunPhase() == EW11RunPhase::Lobby)
	{
		ShowEntryMenu();
	}
}

void AW11PlayerController::ShowEntryMenu()
{
	if (!IsLocalController())
	{
		return;
	}
	if (!EntryMenuWidget)
	{
		EntryMenuWidget = CreateWidget<UW11EntryMenuWidget>(this, UW11EntryMenuWidget::StaticClass());
		if (EntryMenuWidget)
		{
			EntryMenuWidget->Configure(this);
		}
	}
	if (EntryMenuWidget && !EntryMenuWidget->IsInViewport())
	{
		EntryMenuWidget->AddToViewport(200);
	}

	bShowMouseCursor = true;
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (EntryMenuWidget)
	{
		InputMode.SetWidgetToFocus(EntryMenuWidget->TakeWidget());
	}
	SetInputMode(InputMode);
	if (APawn* CurrentPawn = GetPawn())
	{
		CurrentPawn->SetActorHiddenInGame(true);
	}
	StartEntryMusic();
	UE_LOG(LogWorldWalkerW11, Display, TEXT("W11_ENTRY_MENU_SHOWN Pages=7 MainButtons=8 Heroes=10 Sects=6"));
}

void AW11PlayerController::HideEntryMenu()
{
	if (!IsLocalController())
	{
		return;
	}
	if (EntryMenuWidget)
	{
		EntryMenuWidget->RemoveFromParent();
		EntryMenuWidget = nullptr;
	}
	// Combat keeps a visible, free mouse cursor while keyboard input continues
	// to reach the possessed character. Reset clears any repeated menu locks.
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	if (APawn* CurrentPawn = GetPawn())
	{
		CurrentPawn->SetActorHiddenInGame(false);
	}
	StopEntryMusic();
	UE_LOG(LogWorldWalkerW11, Display, TEXT("W11_ENTRY_MENU_HIDDEN"));
}

void AW11PlayerController::StartSoloRun(const FName HeroId, const FName SectId)
{
	UE_LOG(LogWorldWalkerW11, Display, TEXT("W11_ENTRY_SOLO_REQUESTED Hero=%s Sect=%s"),
		*HeroId.ToString(), *SectId.ToString());
	ServerStartRun(HeroId, SectId);
}

void AW11PlayerController::ResumeSoloRun()
{
	ServerResumeRun();
}

bool AW11PlayerController::CanResumeSoloRun() const
{
	const AW11GameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AW11GameMode>() : nullptr;
	return GameMode && GameMode->CanResumeActiveRun();
}

void AW11PlayerController::HostCoopRun()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UW11SessionSubsystem* Sessions = GameInstance->GetSubsystem<UW11SessionSubsystem>())
		{
			const IOnlineSubsystem* Online = IOnlineSubsystem::Get();
			const bool bUseLAN = Online && Online->GetSubsystemName() == FName(TEXT("NULL"));
			Sessions->HostRunSession(bUseLAN, 4);
		}
	}
}

void AW11PlayerController::FindCoopRuns()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UW11SessionSubsystem* Sessions = GameInstance->GetSubsystem<UW11SessionSubsystem>())
		{
			const IOnlineSubsystem* Online = IOnlineSubsystem::Get();
			const bool bUseLAN = Online && Online->GetSubsystemName() == FName(TEXT("NULL"));
			Sessions->FindRunSessions(bUseLAN, 50);
		}
	}
}

void AW11PlayerController::JoinCoopRun(const int32 ResultIndex)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UW11SessionSubsystem* Sessions = GameInstance->GetSubsystem<UW11SessionSubsystem>())
		{
			Sessions->JoinRunSession(ResultIndex);
		}
	}
}

TArray<FString> AW11PlayerController::GetSessionResultLabels() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UW11SessionSubsystem* Sessions = GameInstance->GetSubsystem<UW11SessionSubsystem>())
		{
			return Sessions->GetSearchResultLabels();
		}
	}
	return {};
}

void AW11PlayerController::ReturnToMainWorld()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UWorldTravelSubsystem* Travel = GameInstance->GetSubsystem<UWorldTravelSubsystem>())
		{
			Travel->DiscoverWorldDefinitions();
			if (const UWorldDefinition* MainWorld = Travel->GetMainWorldDefinition())
			{
				if (Travel->TravelToWorld(MainWorld))
				{
					StopEntryMusic();
				}
			}
		}
	}
}

void AW11PlayerController::QuitGameFromMenu()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

float AW11PlayerController::GetMenuMusicVolume() const
{
	float Volume = 0.55f;
	if (GConfig)
	{
		GConfig->GetFloat(W11EntrySettings::ConfigSection, W11EntrySettings::MusicVolumeKey, Volume, GGameUserSettingsIni);
	}
	return FMath::Clamp(Volume, 0.0f, 1.0f);
}

void AW11PlayerController::SetMenuMusicVolume(const float Volume)
{
	const float Clamped = FMath::Clamp(Volume, 0.0f, 1.0f);
	if (EntryMusicComponent)
	{
		EntryMusicComponent->SetVolumeMultiplier(Clamped);
	}
	if (GConfig)
	{
		GConfig->SetFloat(W11EntrySettings::ConfigSection, W11EntrySettings::MusicVolumeKey, Clamped, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void AW11PlayerController::SetHeroSelectionMusic(const bool bEnabled)
{
	SwitchEntryMusic(bEnabled
		? W11EntrySettings::HeroSelectionMusicPath
		: W11EntrySettings::TitleMusicPath,
		bEnabled ? 1.4f : 1.0f);
}

void AW11PlayerController::StartEntryMusic()
{
	if (EntryMusicComponent && EntryMusicComponent->IsPlaying())
	{
		return;
	}
	SwitchEntryMusic(W11EntrySettings::TitleMusicPath, 1.5f);
}

void AW11PlayerController::SwitchEntryMusic(const TCHAR* MusicAssetPath, const float FadeInDuration)
{
	if (!MusicAssetPath || ActiveEntryMusicPath == MusicAssetPath && EntryMusicComponent && EntryMusicComponent->IsPlaying())
	{
		return;
	}
	USoundBase* Music = LoadObject<USoundBase>(nullptr, MusicAssetPath);
	if (!Music)
	{
		UE_LOG(LogTemp, Warning, TEXT("W11_ENTRY_MUSIC_MISSING Asset=%s"), MusicAssetPath);
		return;
	}
	if (EntryMusicComponent)
	{
		EntryMusicComponent->FadeOut(0.65f, 0.0f);
	}
	EntryMusicComponent = UGameplayStatics::CreateSound2D(this, Music, GetMenuMusicVolume(), 1.0f, 0.0f, nullptr, false, true);
	if (EntryMusicComponent)
	{
		ActiveEntryMusicPath = MusicAssetPath;
		EntryMusicComponent->FadeIn(FadeInDuration, GetMenuMusicVolume());
		UE_LOG(LogTemp, Display, TEXT("W11_ENTRY_MUSIC_STARTED Volume=%.2f Asset=%s"),
			GetMenuMusicVolume(), MusicAssetPath);
	}
}

void AW11PlayerController::StopEntryMusic()
{
	if (EntryMusicComponent)
	{
		EntryMusicComponent->FadeOut(0.8f, 0.0f);
		EntryMusicComponent = nullptr;
		ActiveEntryMusicPath.Reset();
		UE_LOG(LogTemp, Display, TEXT("W11_ENTRY_MUSIC_STOPPED FadeOut=0.8"));
	}
}

void AW11PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	check(InputComponent);
	InputComponent->BindAction(TEXT("W11Choice1"), IE_Pressed, this, &AW11PlayerController::HandleChoice1);
	InputComponent->BindAction(TEXT("W11Choice2"), IE_Pressed, this, &AW11PlayerController::HandleChoice2);
	InputComponent->BindAction(TEXT("W11Choice3"), IE_Pressed, this, &AW11PlayerController::HandleChoice3);
	InputComponent->BindAction(TEXT("W11Choice4"), IE_Pressed, this, &AW11PlayerController::HandleChoice4);
	InputComponent->BindAction(TEXT("W11Choice5"), IE_Pressed, this, &AW11PlayerController::HandleChoice5);
	InputComponent->BindAction(TEXT("W11Choice6"), IE_Pressed, this, &AW11PlayerController::HandleChoice6);
	InputComponent->BindAction(TEXT("W11Ready"), IE_Pressed, this, &AW11PlayerController::HandleReady);
}

void AW11PlayerController::HandleChoice(const int32 OfferIndex)
{
	const AW11GameState* State = GetWorld() ? GetWorld()->GetGameState<AW11GameState>() : nullptr;
	if (!State)
	{
		return;
	}
	if (State->GetRunPhase() == EW11RunPhase::Cultivation)
	{
		SelectCultivationOffer(OfferIndex);
	}
	else if (State->GetRunPhase() == EW11RunPhase::ImmortalMarket)
	{
		PurchaseShopOffer(OfferIndex);
	}
}

void AW11PlayerController::HandleChoice1() { HandleChoice(0); }
void AW11PlayerController::HandleChoice2() { HandleChoice(1); }
void AW11PlayerController::HandleChoice3() { HandleChoice(2); }
void AW11PlayerController::HandleChoice4() { HandleChoice(3); }
void AW11PlayerController::HandleChoice5() { HandleChoice(4); }
void AW11PlayerController::HandleChoice6() { HandleChoice(5); }

void AW11PlayerController::HandleReady()
{
	const AW11PlayerState* State = GetPlayerState<AW11PlayerState>();
	SetReadyForNextStage(State ? !State->IsReadyForNextStage() : true);
}

void AW11PlayerController::SelectCultivationOffer(const int32 OfferIndex)
{
	ServerSelectCultivationOffer(OfferIndex);
}

void AW11PlayerController::PurchaseShopOffer(const int32 OfferIndex)
{
	ServerPurchaseShopOffer(OfferIndex);
}

void AW11PlayerController::SetReadyForNextStage(const bool bReady)
{
	ServerSetReadyForNextStage(bReady);
}

void AW11PlayerController::RequestNewRun()
{
	ServerRequestNewRun();
}

void AW11PlayerController::EquipAbility(const EW11AbilitySlot Slot, const FName AbilityId)
{
	ServerEquipAbility(Slot, AbilityId);
}

void AW11PlayerController::ClientReturnToEntryMenu_Implementation()
{
	ShowEntryMenu();
}

void AW11PlayerController::ServerSelectCultivationOffer_Implementation(const int32 OfferIndex)
{
	if (AW11GameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AW11GameMode>() : nullptr)
	{
		GameMode->HandleCultivationSelection(this, OfferIndex);
	}
}

void AW11PlayerController::ServerPurchaseShopOffer_Implementation(const int32 OfferIndex)
{
	if (AW11GameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AW11GameMode>() : nullptr)
	{
		GameMode->HandleShopPurchase(this, OfferIndex);
	}
}

void AW11PlayerController::ServerEquipAbility_Implementation(
	const EW11AbilitySlot Slot,
	const FName AbilityId)
{
	if (AW11GameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AW11GameMode>() : nullptr)
	{
		GameMode->HandleEquipAbility(this, Slot, AbilityId);
	}
}

void AW11PlayerController::ServerSetReadyForNextStage_Implementation(const bool bReady)
{
	if (AW11GameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AW11GameMode>() : nullptr)
	{
		GameMode->HandleReadyForNextStage(this, bReady);
	}
}

void AW11PlayerController::ServerStartRun_Implementation(const FName HeroId, const FName SectId)
{
	if (AW11GameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AW11GameMode>() : nullptr)
	{
		if (GameMode->StartSoloRun(this, HeroId, SectId))
		{
			HideEntryMenu();
		}
	}
}

void AW11PlayerController::ServerResumeRun_Implementation()
{
	if (AW11GameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AW11GameMode>() : nullptr)
	{
		if (GameMode->ResumeSoloRun(this))
		{
			HideEntryMenu();
		}
	}
}

void AW11PlayerController::ServerRequestNewRun_Implementation()
{
	if (AW11GameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AW11GameMode>() : nullptr)
	{
		GameMode->RestartRunToLobby(this);
	}
}
