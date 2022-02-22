#pragma once

#include "FIDEClient.h"
#include "FUE4AssetData.h"

class FBlueprintParserUtils
{
public:
	static FUE4AssetData ParseUObject(const UObject* Object);

	static TArray<uint8> JsonToBytes(const FString& JSONPayload, bool FixBytes = true);
};
