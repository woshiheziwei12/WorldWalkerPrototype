#include "World/WorldTravelSubsystem.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "World/WorldDefinition.h"

const FName UWorldTravelSubsystem::MainWorldId(TEXT("W00_MainWorld"));
const FName UWorldTravelSubsystem::EasternHorrorWorldId(TEXT("W01_EasternHorror"));

void UWorldTravelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	WorldRegistry.Add(
		MainWorldId,
		TSoftObjectPtr<UWorldDefinition>(FSoftObjectPath(
			TEXT("/Game/WorldWalker/Worlds/W00_MainWorld/Data/DA_W00_MainWorld.DA_W00_MainWorld"))));
	WorldRegistry.Add(
		EasternHorrorWorldId,
		TSoftObjectPtr<UWorldDefinition>(FSoftObjectPath(
			TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/Data/DA_W01_EasternHorror.DA_W01_EasternHorror"))));
}

UWorldDefinition* UWorldTravelSubsystem::GetWorldDefinition(const FName WorldId) const
{
	const TSoftObjectPtr<UWorldDefinition>* Definition = WorldRegistry.Find(WorldId);
	return Definition ? Definition->LoadSynchronous() : nullptr;
}

UWorldDefinition* UWorldTravelSubsystem::ResolveCurrentWorldDefinition(const UObject* WorldContextObject)
{
	const UWorld* CurrentWorld = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!CurrentWorld)
	{
		return nullptr;
	}

	const FString CurrentPackageName = UWorld::RemovePIEPrefix(CurrentWorld->GetOutermost()->GetName());
	for (const TPair<FName, TSoftObjectPtr<UWorldDefinition>>& Pair : WorldRegistry)
	{
		UWorldDefinition* Definition = Pair.Value.LoadSynchronous();
		if (Definition && Definition->EntryMap.ToSoftObjectPath().GetLongPackageName() == CurrentPackageName)
		{
			CurrentWorldId = Pair.Key;
			return Definition;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("No WorldDefinition matches map '%s'."), *CurrentPackageName);
	return nullptr;
}

bool UWorldTravelSubsystem::TravelToWorld(const UWorldDefinition* DestinationWorld)
{
	if (!DestinationWorld || DestinationWorld->EntryMap.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("World travel rejected: destination or entry map is missing."));
		return false;
	}

	CurrentWorldId = DestinationWorld->WorldId;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Traveling to world '%s' via map '%s'."),
		*DestinationWorld->WorldId.ToString(),
		*DestinationWorld->EntryMap.ToSoftObjectPath().ToString());

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, DestinationWorld->EntryMap);
	return true;
}
