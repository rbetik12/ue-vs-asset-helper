#include "FIDEClient.h"
#include "FBlueprintParserUtils.h"
#include "JsonObjectConverter.h"

DECLARE_LOG_CATEGORY_CLASS(LogIDEClient, Log, All);

void FIDEClient::SendResponse(const FIDEResponse& Response) const
{
	TArray<uint8> BytesArray = FBlueprintParserUtils::JsonToBytes(Response.ToJSON());
		
	int32 Sent = 0;
	const int32 size = BytesArray.Num();
	Socket->Send((uint8*)&size, sizeof(int32), Sent);
	Socket->Send(BytesArray.GetData(), BytesArray.Num(), Sent);
	if (Sent != BytesArray.Num())
	{
		UE_LOG(LogIDEClient, Warning,
			   TEXT("Didn't send expected bytes amount! Expected: %d Actual: %d"), BytesArray.Num(), Sent);
	}
}

FString FIDEResponse::ToJSON() const
{
	FString JSONPayload;
	FJsonObjectConverter::UStructToJsonObjectString(*this, JSONPayload, 0, 0);
	return std::move(JSONPayload);
}
