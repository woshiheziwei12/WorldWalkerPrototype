#include "World/WorldTravelSubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Modules/ModuleManager.h"
#include "World/WorldDefinition.h"

const FName UWorldTravelSubsystem::EasternHorrorWorldId(TEXT("W01_EasternHorror"));
const FName UWorldTravelSubsystem::SpiralTowerWorldId(TEXT("W02_SpiralTower"));

namespace
{
	FString GetDefinitionWorldRoot(const UWorldDefinition* Definition)
	{
		static const FString WorldsRoot(TEXT("/Game/WorldWalker/Worlds/"));
		const FString PackageName = Definition ? Definition->GetOutermost()->GetName() : FString();
		if (!PackageName.StartsWith(WorldsRoot))
		{
			return FString();
		}

		const FString RelativePackageName = PackageName.RightChop(WorldsRoot.Len());
		int32 SeparatorIndex = INDEX_NONE;
		if (!RelativePackageName.FindChar(TEXT('/'), SeparatorIndex))
		{
			return FString();
		}
		return WorldsRoot + RelativePackageName.Left(SeparatorIndex);
	}

	bool IsAssetInsideWorldRoot(const FSoftObjectPath& AssetPath, const FString& WorldRoot)
	{
		const FString PackageName = AssetPath.GetLongPackageName();
		return AssetPath.IsNull()
			|| WorldRoot.IsEmpty()
			|| PackageName.StartsWith(TEXT("/Script/"))
			|| PackageName.StartsWith(WorldRoot + TEXT("/"));
	}
}

void UWorldTravelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	DiscoverWorldDefinitions();
}

void UWorldTravelSubsystem::DiscoverWorldDefinitions()
{
	WorldRegistry.Reset();

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		TEXT("AssetRegistry")).Get();
	const FName WorldsPackagePath(TEXT("/Game/WorldWalker/Worlds"));
	AssetRegistry.ScanPathsSynchronous({ WorldsPackagePath.ToString() });

	FARFilter DefinitionFilter;
	DefinitionFilter.PackagePaths.Add(WorldsPackagePath);
	DefinitionFilter.ClassPaths.Add(UWorldDefinition::StaticClass()->GetClassPathName());
	DefinitionFilter.bRecursivePaths = true;
	DefinitionFilter.bRecursiveClasses = true;

	TArray<FAssetData> DefinitionAssets;
	AssetRegistry.GetAssets(DefinitionFilter, DefinitionAssets);
	DefinitionAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.PackageName.LexicalLess(Right.PackageName);
	});

	for (const FAssetData& DefinitionAsset : DefinitionAssets)
	{
		UWorldDefinition* Definition = Cast<UWorldDefinition>(DefinitionAsset.GetAsset());
		if (!Definition)
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("World registration failed: definition at '%s' could not be loaded."),
				*DefinitionAsset.GetObjectPathString());
			continue;
		}
		RegisterWorldDefinition(Definition);
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("World registry discovered %d definition(s) without hard-coded world paths."),
		WorldRegistry.Num());
}

bool UWorldTravelSubsystem::RegisterWorldDefinition(UWorldDefinition* Definition)
{
	if (!Definition || Definition->WorldId.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("World registration rejected: definition or WorldId is missing."));
		return false;
	}

	if (const TObjectPtr<UWorldDefinition>* Existing = WorldRegistry.Find(Definition->WorldId))
	{
		if (*Existing == Definition)
		{
			return true;
		}
		UE_LOG(
			LogTemp,
			Error,
			TEXT("World registration rejected: duplicate WorldId '%s' (%s and %s)."),
			*Definition->WorldId.ToString(),
			*GetPathNameSafe(*Existing),
			*GetPathNameSafe(Definition));
		return false;
	}

	const FString WorldRoot = GetDefinitionWorldRoot(Definition);
	if (!IsAssetInsideWorldRoot(Definition->EntryMap.ToSoftObjectPath(), WorldRoot)
		|| !IsAssetInsideWorldRoot(Definition->WorldRootActorClass.ToSoftObjectPath(), WorldRoot))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("World registration rejected: '%s' references an entry map or root actor outside '%s'."),
			*Definition->WorldId.ToString(),
			*WorldRoot);
		return false;
	}

	if (Definition->bIsMainWorld && GetMainWorldDefinition())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("World registration rejected: '%s' attempts to register a second main world."),
			*Definition->WorldId.ToString());
		return false;
	}

	WorldRegistry.Add(Definition->WorldId, Definition);
	return true;
}

UWorldDefinition* UWorldTravelSubsystem::GetWorldDefinition(const FName WorldId) const
{
	const TObjectPtr<UWorldDefinition>* Definition = WorldRegistry.Find(WorldId);
	return Definition ? Definition->Get() : nullptr;
}

UWorldDefinition* UWorldTravelSubsystem::GetMainWorldDefinition() const
{
	UWorldDefinition* MainWorld = nullptr;
	for (const TPair<FName, TObjectPtr<UWorldDefinition>>& Pair : WorldRegistry)
	{
		if (!Pair.Value || !Pair.Value->bIsMainWorld)
		{
			continue;
		}
		if (!MainWorld || Pair.Key.LexicalLess(MainWorld->WorldId))
		{
			MainWorld = Pair.Value;
		}
	}
	return MainWorld;
}

TArray<UWorldDefinition*> UWorldTravelSubsystem::GetRegisteredWorldDefinitions() const
{
	TArray<UWorldDefinition*> Definitions;
	Definitions.Reserve(WorldRegistry.Num());
	for (const TPair<FName, TObjectPtr<UWorldDefinition>>& Pair : WorldRegistry)
	{
		if (Pair.Value)
		{
			Definitions.Add(Pair.Value);
		}
	}
	Definitions.Sort([](const UWorldDefinition& Left, const UWorldDefinition& Right)
	{
		if (Left.PortalOrder != Right.PortalOrder)
		{
			return Left.PortalOrder < Right.PortalOrder;
		}
		return Left.WorldId.LexicalLess(Right.WorldId);
	});
	return Definitions;
}

TArray<UWorldDefinition*> UWorldTravelSubsystem::GetTravelDestinations() const
{
	TArray<UWorldDefinition*> Destinations = GetRegisteredWorldDefinitions();
	Destinations.RemoveAll([](const UWorldDefinition* Definition)
	{
		return !Definition
			|| Definition->bIsMainWorld
			|| !Definition->bExposeInMainWorld
			|| Definition->EntryMap.IsNull();
	});
	return Destinations;
}

UWorldDefinition* UWorldTravelSubsystem::ResolveCurrentWorldDefinition(const UObject* WorldContextObject)
{
	const UWorld* CurrentWorld = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!CurrentWorld)
	{
		return nullptr;
	}

	const FString CurrentPackageName = UWorld::RemovePIEPrefix(CurrentWorld->GetOutermost()->GetName());
	for (const TPair<FName, TObjectPtr<UWorldDefinition>>& Pair : WorldRegistry)
	{
		UWorldDefinition* Definition = Pair.Value;
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
