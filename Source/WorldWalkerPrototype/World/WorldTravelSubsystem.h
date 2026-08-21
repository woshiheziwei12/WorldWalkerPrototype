#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldTravelSubsystem.generated.h"

class UWorldDefinition;

UCLASS()
class WORLDWALKERPROTOTYPE_API UWorldTravelSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Stable ID kept only for the existing W01-specific gameplay implementation. */
	static const FName EasternHorrorWorldId;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * Rebuilds the registry from every WorldDefinition primary asset found by the
	 * Asset Registry. A new world registers itself by adding its own definition
	 * asset under a scanned content root; no hub or subsystem code changes are
	 * required.
	 */
	UFUNCTION(BlueprintCallable, Category="World Travel")
	void DiscoverWorldDefinitions();

	/** Allows a native/Blueprint world module to register a definition directly. */
	UFUNCTION(BlueprintCallable, Category="World Travel")
	bool RegisterWorldDefinition(UWorldDefinition* Definition);

	UFUNCTION(BlueprintCallable, Category="World Travel")
	UWorldDefinition* GetWorldDefinition(FName WorldId) const;

	UFUNCTION(BlueprintCallable, Category="World Travel")
	UWorldDefinition* GetMainWorldDefinition() const;

	/** Deterministic list of all valid, non-main worlds exposed by the hub. */
	UFUNCTION(BlueprintCallable, Category="World Travel")
	TArray<UWorldDefinition*> GetTravelDestinations() const;

	UFUNCTION(BlueprintCallable, Category="World Travel")
	TArray<UWorldDefinition*> GetRegisteredWorldDefinitions() const;

	UFUNCTION(BlueprintCallable, Category="World Travel")
	UWorldDefinition* ResolveCurrentWorldDefinition(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category="World Travel")
	bool TravelToWorld(const UWorldDefinition* DestinationWorld);

	UFUNCTION(BlueprintPure, Category="World Travel")
	FName GetCurrentWorldId() const { return CurrentWorldId; }

private:
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UWorldDefinition>> WorldRegistry;

	UPROPERTY(Transient)
	FName CurrentWorldId = NAME_None;
};
