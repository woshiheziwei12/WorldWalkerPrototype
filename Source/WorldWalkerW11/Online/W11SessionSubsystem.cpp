#include "Online/W11SessionSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformProperties.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "WorldWalkerW11.h"

namespace W11Session
{
	static const FName RunTypeKey(TEXT("W11_RUN_TYPE"));
	static const FName SteamEvidenceTokenKey(TEXT("W11_STEAM_EVIDENCE_TOKEN"));
	static const FName SteamEvidencePackageHashKey(TEXT("W11_STEAM_PACKAGE_HASH"));
	static const FName SteamEvidenceScenarioKey(TEXT("W11_STEAM_EVIDENCE_SCENARIO"));
	static const FString RunTypeValue(TEXT("XIANXIA_ROGUELITE_PVE"));
	static const FString EntryMap(TEXT("/Game/WorldWalker/Worlds/W11_RogueSurvival/Maps/L_W11_RogueSurvival"));
}

void UW11SessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	FParse::Value(FCommandLine::Get(), TEXT("W11SteamEvidenceRole="), SteamEvidenceRole);
	FParse::Value(FCommandLine::Get(), TEXT("W11SteamEvidenceScenario="), SteamEvidenceScenario);
	FParse::Value(FCommandLine::Get(), TEXT("W11SteamEvidenceToken="), SteamEvidenceToken);
	FParse::Value(FCommandLine::Get(), TEXT("W11SteamPackageHash="), SteamEvidencePackageHash);
	SteamEvidenceRole.TrimStartAndEndInline();
	SteamEvidenceScenario.TrimStartAndEndInline();
	SteamEvidenceToken.TrimStartAndEndInline();
	SteamEvidencePackageHash.TrimStartAndEndInline();
	bSteamEvidenceRequested = !SteamEvidenceRole.IsEmpty()
		|| !SteamEvidenceScenario.IsEmpty()
		|| !SteamEvidenceToken.IsEmpty()
		|| !SteamEvidencePackageHash.IsEmpty();
	bSteamEvidenceContextValid = IsValidSteamEvidenceRole(SteamEvidenceRole)
		&& IsValidSteamEvidenceScenario(SteamEvidenceScenario)
		&& IsValidSteamEvidenceToken(SteamEvidenceToken)
		&& IsValidSteamEvidencePackageHash(SteamEvidencePackageHash);
	if (bSteamEvidenceRequested)
	{
		const FString LocalUserId = GetLocalUserId();
		LogSteamEvidence(TEXT("Environment"), bSteamEvidenceContextValid,
			FString::Printf(TEXT("ReleaseReady=%d"),
				IsReleaseSteamEvidenceEnvironment(
					FName(*GetOnlineSubsystemName()), GetOnlineAppId())
					&& IsValidSteamAccountId(LocalUserId) ? 1 : 0));
	}
	if (IOnlineSessionPtr Sessions = GetSessionInterface(); Sessions.IsValid())
	{
		InviteAcceptedHandle = Sessions->AddOnSessionUserInviteAcceptedDelegate_Handle(
			FOnSessionUserInviteAcceptedDelegate::CreateUObject(
				this, &UW11SessionSubsystem::HandleSessionUserInviteAccepted));
	}
	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(
			this, &UW11SessionSubsystem::HandleNetworkFailure);
	}
}

bool UW11SessionSubsystem::IsValidSteamEvidenceRole(const FString& Role)
{
	return Role.Equals(TEXT("Host"), ESearchCase::CaseSensitive)
		|| Role.Equals(TEXT("Client"), ESearchCase::CaseSensitive);
}

bool UW11SessionSubsystem::IsValidSteamEvidenceScenario(const FString& Scenario)
{
	return Scenario.Equals(TEXT("BasicJoin"), ESearchCase::CaseSensitive)
		|| Scenario.Equals(TEXT("FriendInvite"), ESearchCase::CaseSensitive)
		|| Scenario.Equals(TEXT("HostExit"), ESearchCase::CaseSensitive)
		|| Scenario.Equals(TEXT("ClientDisconnect"), ESearchCase::CaseSensitive)
		|| Scenario.Equals(TEXT("PlayerSync"), ESearchCase::CaseSensitive);
}

bool UW11SessionSubsystem::IsValidSteamEvidenceToken(const FString& Token)
{
	if (Token.Len() < 8 || Token.Len() > 64)
	{
		return false;
	}
	for (const TCHAR Character : Token)
	{
		if (Character > 127
			|| (!FChar::IsAlnum(Character) && Character != TEXT('_') && Character != TEXT('-')))
		{
			return false;
		}
	}
	return true;
}

bool UW11SessionSubsystem::IsValidSteamEvidencePackageHash(const FString& PackageHash)
{
	if (PackageHash == TEXT("EDITOR"))
	{
		return true;
	}
	if (PackageHash.Len() != 64)
	{
		return false;
	}
	for (const TCHAR Character : PackageHash)
	{
		if (!FChar::IsHexDigit(Character))
		{
			return false;
		}
	}
	return true;
}

bool UW11SessionSubsystem::IsValidSteamAccountId(const FString& AccountId)
{
	if (AccountId.Len() < 15 || AccountId.Len() > 20 || !AccountId.IsNumeric())
	{
		return false;
	}
	for (const TCHAR Character : AccountId)
	{
		if (Character != TEXT('0'))
		{
			return true;
		}
	}
	return false;
}

bool UW11SessionSubsystem::IsReleaseSteamEvidenceEnvironment(
	const FName SubsystemName,
	const FString& AppId)
{
	return SubsystemName == FName(TEXT("STEAM"))
		&& AppId.IsNumeric()
		&& AppId != TEXT("0")
		&& AppId != TEXT("480");
}

void UW11SessionSubsystem::Deinitialize()
{
	if (GEngine && NetworkFailureHandle.IsValid())
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
	}
	NetworkFailureHandle.Reset();
	ClearDelegates();
	SessionSearch.Reset();
	Super::Deinitialize();
}

IOnlineSessionPtr UW11SessionSubsystem::GetSessionInterface() const
{
	const IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	return Subsystem ? Subsystem->GetSessionInterface() : nullptr;
}

FString UW11SessionSubsystem::GetOnlineSubsystemName() const
{
	const IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	return Subsystem ? Subsystem->GetSubsystemName().ToString() : TEXT("NONE");
}

FString UW11SessionSubsystem::GetOnlineAppId() const
{
	const IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	const FString AppId = Subsystem ? Subsystem->GetAppId() : FString();
	return AppId.IsEmpty() ? TEXT("NONE") : AppId;
}

FString UW11SessionSubsystem::GetLocalUserId() const
{
	const IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem || Subsystem->GetSubsystemName() != FName(TEXT("STEAM")))
	{
		return TEXT("NONE");
	}
	const IOnlineIdentityPtr Identity = Subsystem ? Subsystem->GetIdentityInterface() : nullptr;
	const FUniqueNetIdPtr UserId = Identity.IsValid() ? Identity->GetUniquePlayerId(0) : nullptr;
	return UserId.IsValid() ? UserId->ToString() : TEXT("NONE");
}

FString UW11SessionSubsystem::GetRuntimeKind() const
{
	return FPlatformProperties::RequiresCookedData()
		? TEXT("CookedGame") : TEXT("EditorStandalone");
}

FString UW11SessionSubsystem::GetNamedSessionId(const FName SessionName) const
{
	const IOnlineSessionPtr Sessions = GetSessionInterface();
	const FNamedOnlineSession* NamedSession = Sessions.IsValid()
		? Sessions->GetNamedSession(SessionName) : nullptr;
	return NamedSession && NamedSession->SessionInfo.IsValid()
		? NamedSession->SessionInfo->GetSessionId().ToString() : TEXT("NONE");
}

void UW11SessionSubsystem::LogSteamEvidence(
	const TCHAR* Event,
	const bool bSuccess,
	const FString& Facts) const
{
	if (!bSteamEvidenceRequested)
	{
		return;
	}
	UE_LOG(LogWorldWalkerW11, Display,
		TEXT("W11_STEAM_EVIDENCE Event=%s Role=%s Scenario=%s Token=%s PackageHash=%s OSS=%s AppId=%s LocalUserId=%s Runtime=%s Success=%d %s"),
		Event,
		SteamEvidenceRole.IsEmpty() ? TEXT("NONE") : *SteamEvidenceRole,
		SteamEvidenceScenario.IsEmpty() ? TEXT("NONE") : *SteamEvidenceScenario,
		SteamEvidenceToken.IsEmpty() ? TEXT("NONE") : *SteamEvidenceToken,
		SteamEvidencePackageHash.IsEmpty() ? TEXT("NONE") : *SteamEvidencePackageHash,
		*GetOnlineSubsystemName(),
		*GetOnlineAppId(),
		*GetLocalUserId(),
		*GetRuntimeKind(),
		bSuccess ? 1 : 0,
		*Facts);
}

bool UW11SessionSubsystem::ValidateSteamEvidenceOperation(
	const TCHAR* Operation,
	const TCHAR* RequiredRole)
{
	if (!bSteamEvidenceRequested)
	{
		return true;
	}
	if (!bSteamEvidenceContextValid)
	{
		LogSteamEvidence(Operation, false, TEXT("Reason=InvalidEvidenceContext"));
		return false;
	}
	if (!SteamEvidenceRole.Equals(RequiredRole, ESearchCase::CaseSensitive))
	{
		LogSteamEvidence(Operation, false, TEXT("Reason=RoleMismatch"));
		return false;
	}
	const FString AcceptedFacts = FCString::Strcmp(Operation, TEXT("JoinRequest")) == 0
		? TEXT("Reason=Accepted Source=Browser") : TEXT("Reason=Accepted");
	LogSteamEvidence(Operation, true, AcceptedFacts);
	return true;
}

void UW11SessionSubsystem::HostRunSession(const bool bIsLANMatch, const int32 PublicConnections)
{
	if (!ValidateSteamEvidenceOperation(TEXT("HostRequest"), TEXT("Host")))
	{
		OnHostCompleted.Broadcast(false, TEXT("Steam evidence role cannot host"));
		return;
	}
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		LogSteamEvidence(TEXT("HostComplete"), false, TEXT("Reason=SessionInterfaceUnavailable SessionId=NONE"));
		OnHostCompleted.Broadcast(false, TEXT("Online Subsystem unavailable"));
		return;
	}
	if (Sessions->GetNamedSession(NAME_GameSession))
	{
		LogSteamEvidence(TEXT("HostComplete"), false, TEXT("Reason=SessionAlreadyActive SessionId=NONE"));
		OnHostCompleted.Broadcast(false, TEXT("A session is already active"));
		return;
	}

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = bIsLANMatch;
	Settings.NumPublicConnections = FMath::Clamp(PublicConnections, 1, 4);
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = false;
	Settings.bAllowJoinViaPresence = true;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.Set(W11Session::RunTypeKey, W11Session::RunTypeValue, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	if (bSteamEvidenceRequested)
	{
		Settings.Set(W11Session::SteamEvidenceTokenKey, SteamEvidenceToken,
			EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		Settings.Set(W11Session::SteamEvidencePackageHashKey, SteamEvidencePackageHash,
			EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
		Settings.Set(W11Session::SteamEvidenceScenarioKey, SteamEvidenceScenario,
			EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	}

	CreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UW11SessionSubsystem::HandleCreateSessionComplete));
	if (!Sessions->CreateSession(0, NAME_GameSession, Settings))
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		CreateHandle.Reset();
		LogSteamEvidence(TEXT("HostComplete"), false, TEXT("Reason=CreateRequestRejected SessionId=NONE"));
		OnHostCompleted.Broadcast(false, TEXT("CreateSession request was rejected"));
	}
}

void UW11SessionSubsystem::FindRunSessions(const bool bIsLANQuery, const int32 MaxResults)
{
	if (!ValidateSteamEvidenceOperation(TEXT("FindRequest"), TEXT("Client")))
	{
		OnSearchCompleted.Broadcast(0);
		return;
	}
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		LogSteamEvidence(TEXT("FindComplete"), false, TEXT("Reason=SessionInterfaceUnavailable Results=0 FirstSessionId=NONE"));
		OnSearchCompleted.Broadcast(0);
		return;
	}
	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->bIsLanQuery = bIsLANQuery;
	SessionSearch->MaxSearchResults = FMath::Clamp(MaxResults, 1, 200);
	SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	SessionSearch->QuerySettings.Set(W11Session::RunTypeKey, W11Session::RunTypeValue, EOnlineComparisonOp::Equals);
	if (bSteamEvidenceRequested)
	{
		SessionSearch->QuerySettings.Set(
			W11Session::SteamEvidenceTokenKey, SteamEvidenceToken, EOnlineComparisonOp::Equals);
		SessionSearch->QuerySettings.Set(
			W11Session::SteamEvidencePackageHashKey, SteamEvidencePackageHash, EOnlineComparisonOp::Equals);
		SessionSearch->QuerySettings.Set(
			W11Session::SteamEvidenceScenarioKey, SteamEvidenceScenario, EOnlineComparisonOp::Equals);
	}

	FindHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UW11SessionSubsystem::HandleFindSessionsComplete));
	if (!Sessions->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		FindHandle.Reset();
		SessionSearch.Reset();
		LogSteamEvidence(TEXT("FindComplete"), false, TEXT("Reason=FindRequestRejected Results=0 FirstSessionId=NONE"));
		OnSearchCompleted.Broadcast(0);
	}
}

void UW11SessionSubsystem::JoinRunSession(const int32 ResultIndex)
{
	if (!ValidateSteamEvidenceOperation(TEXT("JoinRequest"), TEXT("Client")))
	{
		OnJoinCompleted.Broadcast(false, TEXT("Steam evidence role cannot join"));
		return;
	}
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || !SessionSearch.IsValid() || !SessionSearch->SearchResults.IsValidIndex(ResultIndex))
	{
		LogSteamEvidence(TEXT("JoinComplete"), false, TEXT("Reason=InvalidSessionResult SessionId=NONE"));
		OnJoinCompleted.Broadcast(false, TEXT("Invalid session result"));
		return;
	}
	if (bSteamEvidenceRequested)
	{
		FString ResultToken;
		FString ResultPackageHash;
		FString ResultScenario;
		SessionSearch->SearchResults[ResultIndex].Session.SessionSettings.Get(
			W11Session::SteamEvidenceTokenKey, ResultToken);
		SessionSearch->SearchResults[ResultIndex].Session.SessionSettings.Get(
			W11Session::SteamEvidencePackageHashKey, ResultPackageHash);
		SessionSearch->SearchResults[ResultIndex].Session.SessionSettings.Get(
			W11Session::SteamEvidenceScenarioKey, ResultScenario);
		if (ResultToken != SteamEvidenceToken)
		{
			LogSteamEvidence(TEXT("JoinRequest"), false, TEXT("Reason=TokenMismatch"));
			OnJoinCompleted.Broadcast(false, TEXT("Session evidence token mismatch"));
			return;
		}
		if (ResultPackageHash != SteamEvidencePackageHash)
		{
			LogSteamEvidence(TEXT("JoinRequest"), false, TEXT("Reason=PackageHashMismatch"));
			OnJoinCompleted.Broadcast(false, TEXT("Session package hash mismatch"));
			return;
		}
		if (ResultScenario != SteamEvidenceScenario)
		{
			LogSteamEvidence(TEXT("JoinRequest"), false, TEXT("Reason=ScenarioMismatch Source=Browser"));
			OnJoinCompleted.Broadcast(false, TEXT("Session evidence scenario mismatch"));
			return;
		}
	}
	PendingJoinSource = TEXT("Browser");
	JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UW11SessionSubsystem::HandleJoinSessionComplete));
	if (!Sessions->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[ResultIndex]))
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		JoinHandle.Reset();
		LogSteamEvidence(TEXT("JoinComplete"), false, TEXT("Reason=JoinRequestRejected SessionId=NONE"));
		OnJoinCompleted.Broadcast(false, TEXT("JoinSession request was rejected"));
	}
}

void UW11SessionSubsystem::LeaveRunSession()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || !Sessions->GetNamedSession(NAME_GameSession))
	{
		LogSteamEvidence(TEXT("LeaveComplete"), true, TEXT("Reason=NoActiveSession"));
		OnLeaveCompleted.Broadcast(true, TEXT("No active session"));
		return;
	}
	DestroyHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UW11SessionSubsystem::HandleDestroySessionComplete));
	if (!Sessions->DestroySession(NAME_GameSession))
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
		DestroyHandle.Reset();
		OnLeaveCompleted.Broadcast(false, TEXT("DestroySession request was rejected"));
	}
}

void UW11SessionSubsystem::RecordParticipantJoined(const int32 PlayerId, const int32 PlayerCount)
{
	LogSteamEvidence(TEXT("ParticipantJoined"), PlayerCount >= 1 && PlayerCount <= 4,
		FString::Printf(TEXT("PlayerId=%d Players=%d SessionId=%s"),
			PlayerId, PlayerCount, *GetNamedSessionId(NAME_GameSession)));
}

void UW11SessionSubsystem::RecordParticipantLeft(const int32 PlayerId, const int32 PlayerCount)
{
	LogSteamEvidence(TEXT("ParticipantLeft"), PlayerCount >= 0 && PlayerCount < 4,
		FString::Printf(TEXT("PlayerId=%d Players=%d SessionId=%s"),
			PlayerId, PlayerCount, *GetNamedSessionId(NAME_GameSession)));
}

TArray<FString> UW11SessionSubsystem::GetSearchResultLabels() const
{
	TArray<FString> Labels;
	if (!SessionSearch.IsValid())
	{
		return Labels;
	}
	for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults)
	{
		const int32 OpenSlots = Result.Session.NumOpenPublicConnections;
		const int32 MaxSlots = Result.Session.SessionSettings.NumPublicConnections;
		Labels.Add(FString::Printf(TEXT("W11 PvE  %d/%d  Ping %d ms"), MaxSlots - OpenSlots, MaxSlots, Result.PingInMs));
	}
	return Labels;
}

void UW11SessionSubsystem::HandleCreateSessionComplete(const FName SessionName, const bool bSuccess)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	}
	CreateHandle.Reset();
	LogSteamEvidence(TEXT("HostComplete"), bSuccess,
		FString::Printf(TEXT("SessionId=%s"), *GetNamedSessionId(SessionName)));
	if (bSuccess && GetWorld())
	{
		GetWorld()->ServerTravel(W11Session::EntryMap + TEXT("?listen?W11Start=1"));
	}
	OnHostCompleted.Broadcast(bSuccess, bSuccess ? TEXT("Hosting W11 run") : TEXT("Failed to create session"));
}

void UW11SessionSubsystem::HandleFindSessionsComplete(const bool bSuccess)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	}
	FindHandle.Reset();
	if (bSuccess && bSteamEvidenceRequested && SessionSearch.IsValid())
	{
		SessionSearch->SearchResults.RemoveAll([this](const FOnlineSessionSearchResult& Result)
		{
			FString ResultToken;
			FString ResultPackageHash;
			FString ResultScenario;
			Result.Session.SessionSettings.Get(W11Session::SteamEvidenceTokenKey, ResultToken);
			Result.Session.SessionSettings.Get(
				W11Session::SteamEvidencePackageHashKey, ResultPackageHash);
			Result.Session.SessionSettings.Get(
				W11Session::SteamEvidenceScenarioKey, ResultScenario);
			return ResultToken != SteamEvidenceToken
				|| ResultPackageHash != SteamEvidencePackageHash
				|| ResultScenario != SteamEvidenceScenario;
		});
	}
	const int32 ResultCount = bSuccess && SessionSearch.IsValid()
		? SessionSearch->SearchResults.Num() : 0;
	FString FirstSessionId(TEXT("NONE"));
	if (ResultCount > 0 && SessionSearch->SearchResults[0].Session.SessionInfo.IsValid())
	{
		FirstSessionId = SessionSearch->SearchResults[0].GetSessionIdStr();
	}
	LogSteamEvidence(TEXT("FindComplete"), bSuccess && ResultCount > 0,
		FString::Printf(TEXT("Results=%d FirstSessionId=%s"), ResultCount, *FirstSessionId));
	OnSearchCompleted.Broadcast(ResultCount);
}

void UW11SessionSubsystem::HandleJoinSessionComplete(const FName SessionName, const EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	}
	JoinHandle.Reset();
	FString ConnectString;
	const bool bSuccess = Result == EOnJoinSessionCompleteResult::Success
		&& Sessions.IsValid()
		&& Sessions->GetResolvedConnectString(SessionName, ConnectString);
	LogSteamEvidence(TEXT("JoinComplete"), bSuccess,
		FString::Printf(TEXT("Result=%d SessionId=%s Source=%s"),
			static_cast<int32>(Result), *GetNamedSessionId(SessionName), *PendingJoinSource));
	const bool bInviteJoin = PendingJoinSource == TEXT("Invite");
	PendingJoinSource = TEXT("None");
	if (bSuccess && !bInviteJoin && GetGameInstance())
	{
		if (APlayerController* Controller = GetGameInstance()->GetFirstLocalPlayerController())
		{
			Controller->ClientTravel(ConnectString, TRAVEL_Absolute);
		}
	}
	OnJoinCompleted.Broadcast(bSuccess, bSuccess ? TEXT("Joining W11 run") : TEXT("Failed to resolve session address"));
}

void UW11SessionSubsystem::HandleSessionUserInviteAccepted(
	const bool bWasSuccessful,
	const int32 ControllerId,
	FUniqueNetIdPtr UserId,
	const FOnlineSessionSearchResult& InviteResult)
{
	if (!bSteamEvidenceRequested)
	{
		return;
	}
	FString ResultToken;
	FString ResultPackageHash;
	FString ResultScenario;
	if (InviteResult.IsValid())
	{
		InviteResult.Session.SessionSettings.Get(W11Session::SteamEvidenceTokenKey, ResultToken);
		InviteResult.Session.SessionSettings.Get(
			W11Session::SteamEvidencePackageHashKey, ResultPackageHash);
		InviteResult.Session.SessionSettings.Get(W11Session::SteamEvidenceScenarioKey, ResultScenario);
	}
	const FString InviteSessionId = InviteResult.IsValid() && InviteResult.Session.SessionInfo.IsValid()
		? InviteResult.GetSessionIdStr() : TEXT("NONE");
	const bool bAccepted = bWasSuccessful
		&& InviteResult.IsValid()
		&& bSteamEvidenceContextValid
		&& SteamEvidenceRole == TEXT("Client")
		&& SteamEvidenceScenario == TEXT("FriendInvite")
		&& ResultToken == SteamEvidenceToken
		&& ResultPackageHash == SteamEvidencePackageHash
		&& ResultScenario == SteamEvidenceScenario;
	LogSteamEvidence(TEXT("InviteAccepted"), bAccepted,
		FString::Printf(TEXT("Controller=%d SessionId=%s MetadataMatch=%d"),
			ControllerId, *InviteSessionId, bAccepted ? 1 : 0));
	if (!bAccepted)
	{
		return;
	}
	LogSteamEvidence(TEXT("JoinRequest"), true,
		FString::Printf(TEXT("Reason=Accepted Source=Invite SessionId=%s"), *InviteSessionId));
	if (IOnlineSessionPtr Sessions = GetSessionInterface(); Sessions.IsValid())
	{
		PendingJoinSource = TEXT("Invite");
		JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
			FOnJoinSessionCompleteDelegate::CreateUObject(
				this, &UW11SessionSubsystem::HandleJoinSessionComplete));
	}
}

void UW11SessionSubsystem::HandleNetworkFailure(
	UWorld* World,
	UNetDriver* NetDriver,
	const ENetworkFailure::Type FailureType,
	const FString& ErrorString)
{
	if (!bSteamEvidenceRequested || !World || World != GetWorld())
	{
		return;
	}
	LogSteamEvidence(TEXT("NetworkFailure"), true,
		FString::Printf(TEXT("FailureType=%s SessionId=%s"),
			ENetworkFailure::ToString(FailureType), *GetNamedSessionId(NAME_GameSession)));
}

void UW11SessionSubsystem::HandleDestroySessionComplete(const FName SessionName, const bool bSuccess)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	}
	DestroyHandle.Reset();
	LogSteamEvidence(TEXT("LeaveComplete"), bSuccess,
		FString::Printf(TEXT("Session=%s"), *SessionName.ToString()));
	OnLeaveCompleted.Broadcast(bSuccess, bSuccess ? TEXT("Session closed") : TEXT("Failed to close session"));
}

void UW11SessionSubsystem::ClearDelegates()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		return;
	}
	if (CreateHandle.IsValid()) Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	if (FindHandle.IsValid()) Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	if (JoinHandle.IsValid()) Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	if (DestroyHandle.IsValid()) Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	if (InviteAcceptedHandle.IsValid()) Sessions->ClearOnSessionUserInviteAcceptedDelegate_Handle(InviteAcceptedHandle);
	CreateHandle.Reset();
	FindHandle.Reset();
	JoinHandle.Reset();
	DestroyHandle.Reset();
	InviteAcceptedHandle.Reset();
}
