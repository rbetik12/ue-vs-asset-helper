#include "BlueprintParser.h"
#include "FUE4AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/ObjectLibrary.h"
#include "FBlueprintParserUtils.h"
#include "Common/TcpSocketBuilder.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "FIDEClient.h"
#include "JsonObjectConverter.h"

#define LOCTEXT_NAMESPACE "FBlueprintParserModule"
DECLARE_LOG_CATEGORY_CLASS(LogBlueprintParser, Log, All);

void FBlueprintParserModule::StartupModule()
{
	TArray<FAssetData> AssetData;
	const auto ObjLib = UObjectLibrary::CreateLibrary(UObject::StaticClass(), true, true);
	ObjLib->bRecursivePaths = true;
	ObjLib->LoadAssetDataFromPath(TEXT("/Game/"));
	ObjLib->GetAssetDataList(AssetData);
	for (const auto& Asset: AssetData)
	{
		const auto Object = Asset.GetAsset();
		FUE4AssetData UEAssetData = FBlueprintParserUtils::ParseUObject(Object);
		for (const auto& BlueprintClassObj: UEAssetData.BlueprintClasses)
		{
			BlueprintClassObjectCache.Add(BlueprintClassObj.SuperClassName, BlueprintClassObj);
		}
	}

	CreateIDESocket();
}

void FBlueprintParserModule::ShutdownModule()
{
	if (ListenSocket)
	{
		ListenSocket->Close();
	}
}

void FBlueprintParserModule::CreateIDESocket()
{
	FIPv4Address IpAddress;
	FIPv4Address::Parse(FString("127.0.0.1"), IpAddress);
	FIPv4Endpoint Endpoint(IpAddress, 8080);

	ListenSocket = FTcpSocketBuilder(TEXT("TcpSocket")).AsReusable();
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	ListenSocket->Bind(*SocketSubsystem->CreateInternetAddr(Endpoint.Address.Value, Endpoint.Port));
	ListenSocket->Listen(1);
	
	IDEFuture = Async(EAsyncExecution::Thread, [&]()
	{
		ServeIDEClientConnection();
	});
}
// TODO: Handle IDE disconnect
void FBlueprintParserModule::ServeIDEClientConnection()
{
	while (bShouldListen)
	{
		bool bHasPendingConnection;
		ListenSocket->HasPendingConnection(bHasPendingConnection);
		if (bHasPendingConnection && !IDEClient)
		{
			TSharedPtr<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
			FSocket* Client = ListenSocket->Accept(*Addr,TEXT("tcp-client"));

			const FString AddressString = Addr->ToString(true);

			IDEClient = MakeShareable(new FIDEClient());
			IDEClient->Address = AddressString;
			IDEClient->Socket = Client;
		}
		else if (bHasPendingConnection && IDEClient)
		{
			UE_LOG(LogBlueprintParser, Warning, TEXT("Unexpected pending client connection!"));
		}

		uint32 BufferSize = 0;
		if (IDEClient && IDEClient->Socket->HasPendingData(BufferSize))
		{
			ServeIDEClientData(BufferSize);
		}
	}
}

void FBlueprintParserModule::ServeIDEClientData(const uint32 BufferSize)
{
	if (IDEClient->Socket->GetConnectionState() != ESocketConnectionState::SCS_Connected)
	{
		UE_LOG(LogBlueprintParser, Log, TEXT("IDE client disconnected!"));
		return;
	}

	TArray<uint8> RecvBuffer;
	RecvBuffer.SetNumZeroed(BufferSize);
	int32 Read = 0;

	IDEClient->Socket->Recv(RecvBuffer.GetData(), RecvBuffer.Num(), Read);
	if (Read != BufferSize)
	{
		UE_LOG(LogBlueprintParser, Warning,
			TEXT("Didn't receive expected bytes amount! Expected: %d Actual: %d"), BufferSize, Read);
		return;
	}

	// Fix string by subtracting 1 from character value
	for (auto& StrByte: RecvBuffer)
	{
		StrByte -= 1;
	}
	
	const FString ClassName = BytesToString(RecvBuffer.GetData(), RecvBuffer.Num());
	const FString ClassNameWithoutPrefix = ClassName.RightChop(1);

	FBlueprintClassObject* Obj = BlueprintClassObjectCache.Find(ClassName);
	if (!Obj)
	{
		Obj = BlueprintClassObjectCache.Find(ClassNameWithoutPrefix);
	}

	if (!Obj)
	{
		FIDEResponse Response;
		Response.Header.Status = EResponseStatus::ERROR;
		Response.AnswerString = "Can't find blueprint class object with name " + ClassName;
		FString JSONPayload;
		TArray<uint8> BytesArray;
		FJsonObjectConverter::UStructToJsonObjectString(Response, JSONPayload, 0, 0);
		BytesArray.SetNumZeroed(JSONPayload.Len());
		StringToBytes(JSONPayload, BytesArray.GetData(), BytesArray.Num());
		int32 Sent = 0;
		for (auto& Byte: BytesArray)
		{
			Byte += 1;
		}
		const int32 size = BytesArray.Num();
		IDEClient->Socket->Send((uint8*)&size, sizeof(int32), Sent);
		IDEClient->Socket->Send(BytesArray.GetData(), BytesArray.Num(), Sent);
		if (Sent != BytesArray.Num())
		{
			UE_LOG(LogBlueprintParser, Warning,
				TEXT("Didn't send expected bytes amount! Expected: %d Actual: %d"), BytesArray.Num(), Sent);
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FBlueprintParserModule, BlueprintParser)