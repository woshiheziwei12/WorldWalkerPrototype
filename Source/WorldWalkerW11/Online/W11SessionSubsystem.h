#pragma once

#include "CoreMinimal.h"
#include "Net/Core/Connection/NetEnums.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "W11SessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FW11SessionOperationEvent, bool, bSuccess, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FW11SessionSearchEvent, int32, ResultCount);

/**
 * Thin Online Subsystem boundary for W11. Gameplay never depends on Steam types:
 * Steam, EOS or Null may back the same host/find/join flow.
 */
UCLASS()
class WORLDWALKERW11_API UW11SessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static bool IsValidSteamEvidenceRole(const FString& Role);
	static bool IsValidSteamEvidenceScenario(const FString& Scenario);
	static bool IsValidSteamEvidenceToken(const FString& Token);
	static bool IsValidSteamEvidencePackageHash(const FString& PackageHash);
	static bool IsValidSteamAccountId(const FString& AccountId);
	static bool IsReleaseSteamEvidenceEnvironment(FName SubsystemName, const FString& AppId);

	UFUNCTION(BlueprintCallable, Category="W11|Online")
	void HostRunSession(bool bIsLANMatch = false, int32 PublicConnections = 4);

	UFUNCTION(BlueprintCallable, Category="W11|Online")
	void FindRunSessions(bool bIsLANQuery = false, int32 MaxResults = 50);

	UFUNCTION(BlueprintCallable, Category="W11|Online")
	void JoinRunSession(int32 ResultIndex);

	UFUNCTION(BlueprintCallable, Category="W11|Online")
	void LeaveRunSession();

	/** Server-authored population facts used by real multi-machine acceptance. */
	void RecordParticipantJoined(int32 PlayerId, int32 PlayerCount);
	void RecordParticipantLeft(int32 PlayerId, int32 PlayerCount);

	UFUNCTION(BlueprintPure, Category="W11|Online")
	TArray<FString> GetSearchResultLabels() const;

	UPROPERTY(BlueprintAssignable, Category="W11|Online")
	FW11SessionOperationEvent OnHostCompleted;

	UPROPERTY(BlueprintAssignable, Category="W11|Online")
	FW11SessionSearchEvent OnSearchCompleted;

	UPROPERTY(BlueprintAssignable, Category="W11|Online")
	FW11SessionOperationEvent OnJoinCompleted;

	UPROPERTY(BlueprintAssignable, Category="W11|Online")
	FW11SessionOperationEvent OnLeaveCompleted;

private:
	IOnlineSessionPtr GetSessionInterface() const;
	void LogSteamEvidence(const TCHAR* Event, bool bSuccess, const FString& Facts = FString()) const;
	bool ValidateSteamEvidenceOperation(const TCHAR* Operation, const TCHAR* RequiredRole);
	FString GetOnlineSubsystemName() const;
	FString GetOnlineAppId() const;
	FString GetLocalUserId() const;
	FString GetRuntimeKind() const;
	FString GetNamedSessionId(FName SessionName) const;
	void HandleCreateSessionComplete(FName SessionName, bool bSuccess);
	void HandleFindSessionsComplete(bool bSuccess);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bSuccess);
	void HandleSessionUserInviteAccepted(
		bool bWasSuccessful,
		int32 ControllerId,
		FUniqueNetIdPtr UserId,
		const FOnlineSessionSearchResult& InviteResult);
	void HandleNetworkFailure(
		UWorld* World,
		class UNetDriver* NetDriver,
		ENetworkFailure::Type FailureType,
		const FString& ErrorString);
	void ClearDelegates();

	TSharedPtr<class FOnlineSessionSearch> SessionSearch;
	FDelegateHandle CreateHandle;
	FDelegateHandle FindHandle;
	FDelegateHandle JoinHandle;
	FDelegateHandle DestroyHandle;
	FDelegateHandle InviteAcceptedHandle;
	FDelegateHandle NetworkFailureHandle;
	FString SteamEvidenceRole;
	FString SteamEvidenceScenario;
	FString SteamEvidenceToken;
	FString SteamEvidencePackageHash;
	FString PendingJoinSource = TEXT("None");
	bool bSteamEvidenceRequested = false;
	bool bSteamEvidenceContextValid = false;
};
