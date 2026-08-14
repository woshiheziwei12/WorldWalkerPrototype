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
	static const FName MainWorldId;
	static const FName EasternHorrorWorldId;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category="World Travel")
	UWorldDefinition* GetWorldDefinition(FName WorldId) const;

	UFUNCTION(BlueprintCallable, Category="World Travel")
	UWorldDefinition* ResolveCurrentWorldDefinition(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category="World Travel")
	bool TravelToWorld(const UWorldDefinition* DestinationWorld);

	UFUNCTION(BlueprintPure, Category="World Travel")
	FName GetCurrentWorldId() const { return CurrentWorldId; }

private:
	UPROPERTY(Transient)
	TMap<FName, TSoftObjectPtr<UWorldDefinition>> WorldRegistry;

	UPROPERTY(Transient)
	FName CurrentWorldId = NAME_None;
};
