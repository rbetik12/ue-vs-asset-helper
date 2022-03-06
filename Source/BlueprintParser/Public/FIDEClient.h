#pragma once

#include "Containers/EnumAsByte.h"
#include "Containers/UnrealString.h"
#include "Sockets.h"
#include "FIDEClient.generated.h"

UENUM()
enum class ERequestType : uint8
{
	GET_INFO,
	OPEN
};

UENUM()
enum class EResponseStatus: uint8
{
	OK,
	ERROR
};

USTRUCT()
struct FIDEResponse
{
	GENERATED_BODY()

	UPROPERTY()
	TEnumAsByte<EResponseStatus> Status;

	UPROPERTY()
	FString AnswerString;

	FString ToJSON() const;
};

USTRUCT()
struct FIDERequest
{
	GENERATED_BODY()

	UPROPERTY()
	TEnumAsByte<ERequestType> Type;

	UPROPERTY()
	FString Data;
};

struct FIDEClient
{
	FSocket* Socket;
	FString Address;

	void SendResponse(const FIDEResponse& Response) const;
};
