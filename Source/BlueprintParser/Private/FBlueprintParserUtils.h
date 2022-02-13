#pragma once

#include "FUE4AssetData.h"

class FBlueprintParserUtils
{
public:
	static FUE4AssetData ParseUObject(const UObject* Object);
};
