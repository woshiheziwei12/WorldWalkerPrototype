#include "World/WorldDefinition.h"

FPrimaryAssetId UWorldDefinition::GetPrimaryAssetId() const
{
	return WorldId.IsNone()
		? Super::GetPrimaryAssetId()
		: FPrimaryAssetId(TEXT("WorldDefinition"), WorldId);
}
