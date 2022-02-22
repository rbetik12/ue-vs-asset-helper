#pragma once

#include "Containers/EnumAsByte.h"
#include "Containers/UnrealString.h"
#include "Sockets.h"
#include "FIDEClient.generated.h"

UENUM()
enum class EResponseStatus: uint8
{
	OK,
	ERROR
};

USTRUCT()
struct FIDEResponseHeader
{
	GENERATED_BODY()

	UPROPERTY()
	TEnumAsByte<EResponseStatus> Status;
};

USTRUCT()
struct FIDEResponse
{
	GENERATED_BODY()

	UPROPERTY()
	FIDEResponseHeader Header;

	UPROPERTY()
	FString AnswerString;

	FString ToJSON() const;
};

struct FIDEClient
{
	FSocket* Socket;
	FString Address;

	void SendResponse(const FIDEResponse& Response) const;
};
