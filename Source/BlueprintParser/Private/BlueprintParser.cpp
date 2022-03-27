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
#include "Async/Async.h"

#define LOCTEXT_NAMESPACE "FBlueprintParserModule"
DECLARE_LOG_CATEGORY_CLASS(LogBlueprintParser, Log, All);

void FBlueprintParserModule::StartupModule()
{
	TArray<FAssetData> AssetData;
	const auto ObjLib = UObjectLibrary::CreateLibrary(UObject::StaticClass(), true, true);
	ObjLib->bRecursivePaths = true;
	ObjLib->LoadAssetDataFromPath(TEXT("/Game/"));
	ObjLib->GetAssetDataList(AssetData);
	for (const auto& Asset : AssetData)
	{
		const auto Object = Asset.GetAsset();
		FUE4AssetData UEAssetData = FBlueprintParserUtils::ParseUObject(Object);
		for (const auto& BlueprintClassObj : UEAssetData.BlueprintClasses)
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
		//ListenSocket->HasPendingConnection(bHasPendingConnection);
		ListenSocket->WaitForPendingConnection(bHasPendingConnection, 1000000);
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
			TSharedPtr<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
			FSocket* Client = ListenSocket->Accept(*Addr,TEXT("tcp-client"));
			const FString AddressString = Addr->ToString(true);

			IDEClient->Address = AddressString;
			IDEClient->Socket->Wait(ESocketWaitConditions::WaitForWrite, 10000000);
			IDEClient->Socket->Close();
			IDEClient->Socket = Client;
			// UE_LOG(LogBlueprintParser, Warning, TEXT("Unexpected pending client connection!"));
		}

		uint32 BufferSize = 0;
		if (IDEClient && IDEClient->Socket->HasPendingData(BufferSize) && BufferSize > 4)
		{
			ServeIDEClientData();
		}
	}
}

void FBlueprintParserModule::ServeIDEClientData()
{
	if (IDEClient->Socket->GetConnectionState() != ESocketConnectionState::SCS_Connected)
	{
		UE_LOG(LogBlueprintParser, Log, TEXT("IDE client disconnected!"));
		return;
	}

	int32 Read = 0;
	int32 RequestSize = 0;
	IDEClient->Socket->Recv((uint8*)&RequestSize, sizeof(int32), Read);
	if (Read != 4)
	{
		UE_LOG(LogBlueprintParser, Warning,
		       TEXT("Didn't receive expected bytes amount! Expected: %d Actual: %d"), 4, Read);
		return;
	}

	TArray<uint8> RecvBuffer;
	RecvBuffer.SetNumZeroed(RequestSize);
	IDEClient->Socket->Recv(RecvBuffer.GetData(), RecvBuffer.Num(), Read);
	if (Read != RequestSize)
	{
		UE_LOG(LogBlueprintParser, Warning,
		       TEXT("Didn't receive expected bytes amount! Expected: %d Actual: %d"), RequestSize, Read);
		return;
	}

	// Fix string by subtracting 1 from character value
	for (auto& StrByte : RecvBuffer)
	{
		StrByte -= 1;
	}

	const FString JSONStrRequest = BytesToString(RecvBuffer.GetData(), RecvBuffer.Num());
	TSharedPtr<FJsonObject> JSONRequest = MakeShareable(new FJsonObject);
	const auto Reader = TJsonReaderFactory<>::Create(JSONStrRequest);
	if (!FJsonSerializer::Deserialize(Reader, JSONRequest))
	{
		// TODO: Handle json parse error
	}
	FIDERequest request;
	FJsonObjectConverter::JsonObjectStringToUStruct(JSONStrRequest, &request);
	ServeIDERequest(request);
}

void FBlueprintParserModule::ServeIDERequest(FIDERequest Request)
{
	const FString ClassName = Request.Data;
	const FString ClassNameWithoutPrefix = ClassName.RightChop(1);

	FBlueprintClassObject* Obj = BlueprintClassObjectCache.Find(ClassName);
	if (!Obj)
	{
		Obj = BlueprintClassObjectCache.Find(ClassNameWithoutPrefix);
	}

	if (!Obj)
	{
		FIDEResponse Response;
		Response.Status = EResponseStatus::ERROR;
		Response.AnswerString = "Can't find blueprint class object with name " + ClassName;

		IDEClient->SendResponse(Response);
	}
	else
	{
		switch (Request.Type)
		{
		case ERequestType::OPEN:
			{
				FBlueprintParserUtils::OpenBlueprint(Obj, [IDEClient = IDEClient, ClassName = std::move(ClassName)]
				{
					FIDEResponse Response;
					Response.Status = EResponseStatus::OK;
					Response.AnswerString = "Successfully opened blueprint object " + ClassName;

					IDEClient->SendResponse(Response);
				});
			}
			break;
		case ERequestType::GET_INFO:
			{
				const auto UEObject = LoadObject<UObject>(nullptr, *Obj->PackageName);
				if (UEObject)
				{
					Obj->Properties = FBlueprintParserUtils::GetUObjectBlueprintProperties(UEObject);
				}
				else
				{
					UE_LOG(LogBlueprintParser, Warning, TEXT("Can't load object with package name: %s"), *Obj->PackageName);
				}
				
				FString JSONPayload;
				FJsonObjectConverter::UStructToJsonObjectString(*Obj, JSONPayload, 0, 0);
				
				FIDEResponse Response;
				Response.Status = EResponseStatus::OK;
				Response.AnswerString = JSONPayload;

				IDEClient->SendResponse(Response);
			}
			break;
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBlueprintParserModule, BlueprintParser)
