#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "World/WorldDefinition.h"
#include "World/WorldTravelSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldRegistrationContractTest,
	"WorldWalker.WorldRegistry.RegistrationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldRegistrationContractTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UWorldTravelSubsystem* Registry = NewObject<UWorldTravelSubsystem>(GameInstance);

	UWorldDefinition* MainWorld = NewObject<UWorldDefinition>(Registry);
	MainWorld->WorldId = TEXT("Test_Home");
	MainWorld->bIsMainWorld = true;
	MainWorld->EntryMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Test/Home.Home")));

	UWorldDefinition* LaterWorld = NewObject<UWorldDefinition>(Registry);
	LaterWorld->WorldId = TEXT("Test_Later");
	LaterWorld->PortalOrder = 20;
	LaterWorld->EntryMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Test/Later.Later")));

	UWorldDefinition* EarlierWorld = NewObject<UWorldDefinition>(Registry);
	EarlierWorld->WorldId = TEXT("Test_Earlier");
	EarlierWorld->PortalOrder = 10;
	EarlierWorld->EntryMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Test/Earlier.Earlier")));

	TestTrue(TEXT("main world registers"), Registry->RegisterWorldDefinition(MainWorld));
	TestTrue(TEXT("later destination registers"), Registry->RegisterWorldDefinition(LaterWorld));
	TestTrue(TEXT("earlier destination registers"), Registry->RegisterWorldDefinition(EarlierWorld));
	TestTrue(TEXT("registering the same definition is idempotent"), Registry->RegisterWorldDefinition(EarlierWorld));
	TestEqual(TEXT("main world is selected by metadata"), Registry->GetMainWorldDefinition(), MainWorld);

	const TArray<UWorldDefinition*> Destinations = Registry->GetTravelDestinations();
	TestEqual(TEXT("main world is excluded from destinations"), Destinations.Num(), 2);
	if (Destinations.Num() == 2)
	{
		TestEqual(TEXT("portal order is data-driven"), Destinations[0], EarlierWorld);
		TestEqual(TEXT("second destination follows registry order"), Destinations[1], LaterWorld);
	}

	LaterWorld->bExposeInMainWorld = false;
	const TArray<UWorldDefinition*> VisibleDestinations = Registry->GetTravelDestinations();
	TestEqual(TEXT("a world can hide itself without changing hub code"), VisibleDestinations.Num(), 1);
	return true;
}

#endif
