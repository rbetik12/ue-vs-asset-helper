#pragma once

#include "CoreMinimal.h"
#include "FIDEClient.h"
#include "FUE4AssetData.h"

class FBlueprintParserModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

	void ServeIDEClientConnection();
	void ServeIDEClientData();
	void ServeIDERequest(FIDERequest Request);
	void CreateIDESocket();

	TMap<FString, FBlueprintClassObject> BlueprintClassObjectCache;
	FSocket* ListenSocket = nullptr;
	TSharedPtr<FIDEClient> IDEClient = nullptr;
	TFuture<void> IDEFuture;
	bool bShouldListen = true;
};
