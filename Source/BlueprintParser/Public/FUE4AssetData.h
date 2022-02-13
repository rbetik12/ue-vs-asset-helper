#pragma once

#include "K2Node_AddDelegate.h"
#include "K2Node_CallDelegate.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ClearDelegate.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Containers/UnrealString.h"

#include "FUE4AssetData.generated.h"

#define BLUEPRINT_GENERATED_CLASS_NAME "BlueprintGeneratedClass"
#define WIDGET_BLUEPRINT_GENERATED_CLASS "WidgetBlueprintGeneratedClass"

USTRUCT()
struct FBlueprintClassObject
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	int Index;

	UPROPERTY()
	FString ObjectName;

	UPROPERTY()
	FString ClassName;

	UPROPERTY()
	FString SuperClassName;

	UPROPERTY()
	FString PackageName;

	FBlueprintClassObject(const int Index, const FString& ObjectName, const FString& ClassName,
	                      const FString& SuperClassName, const FString& PackageName):
		Index(Index),
		ObjectName(ObjectName),
		ClassName(ClassName),
		SuperClassName(SuperClassName),
		PackageName(PackageName)
	{
	}

	FBlueprintClassObject(): Index(0)
	{
	}
};

UENUM()
enum class EKind
{
	VariableGet,
	VariableSet,
	FunctionCall,
	AddDelegate,
	ClearDelegate,
	CallDelegate,
	Other
};

USTRUCT()
struct FK2GraphNodeObject
{
	GENERATED_USTRUCT_BODY()

	static EKind GetKindByClassName(const FString& ClassName)
	{
		if (ClassName == "K2Node_CallFunction")
		{
			return EKind::FunctionCall;
		}
		if (ClassName == "K2Node_VariableSet")
		{
			return EKind::VariableSet;
		}
		if (ClassName == "K2Node_VariableGet")
		{
			return EKind::VariableGet;
		}
		if (ClassName == "K2Node_AddDelegate")
		{
			return EKind::AddDelegate;
		}
		if (ClassName == "K2Node_ClearDelegate")
		{
			return EKind::ClearDelegate;
		}
		if (ClassName == "K2Node_CallDelegate")
		{
			return EKind::CallDelegate;
		}
		return EKind::Other;
	}

	static void GetMemberNameByClassName(const UK2Node* Node, FString& MemberName)
	{
		FString ClassName = Node->GetClass()->GetName();
		if (ClassName == "K2Node_CallFunction")
		{
			const UK2Node_CallFunction* CastedNode = Cast<UK2Node_CallFunction>(Node);
			MemberName = CastedNode->FunctionReference.GetMemberName().ToString();
		}
		if (ClassName == "K2Node_VariableSet")
		{
			const UK2Node_VariableSet* CastedNode = Cast<UK2Node_VariableSet>(Node);
			MemberName = CastedNode->VariableReference.GetMemberName().ToString();
		}
		if (ClassName == "K2Node_VariableGet")
		{
			const UK2Node_VariableGet* CastedNode = Cast<UK2Node_VariableGet>(Node);
			MemberName = CastedNode->VariableReference.GetMemberName().ToString();
		}
		if (ClassName == "K2Node_AddDelegate")
		{
			const UK2Node_AddDelegate* CastedNode = Cast<UK2Node_AddDelegate>(Node);
			MemberName = CastedNode->DelegateReference.GetMemberName().ToString();
		}
		if (ClassName == "K2Node_ClearDelegate")
		{
			const UK2Node_ClearDelegate* CastedNode = Cast<UK2Node_ClearDelegate>(Node);
			MemberName = CastedNode->DelegateReference.GetMemberName().ToString();
		}
		if (ClassName == "K2Node_CallDelegate")
		{
			const UK2Node_CallDelegate* CastedNode = Cast<UK2Node_CallDelegate>(Node);
			MemberName = CastedNode->DelegateReference.GetMemberName().ToString();
		}
	}

	UPROPERTY()
	int Index;

	UPROPERTY()
	EKind ObjectKind;

	UPROPERTY()
	FString MemberName;

	FK2GraphNodeObject(const int Index, const EKind ObjectKind, const FString MemberName):
		Index(Index),
		ObjectKind(ObjectKind),
		MemberName(MemberName)
	{
	}

	FK2GraphNodeObject(): Index(0), ObjectKind(EKind::Other)
	{
	}
};

USTRUCT()
struct FOtherAssetObject
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	int Index;

	UPROPERTY()
	FString ClassName;

	FOtherAssetObject(const int Index, const FString ClassName):
		Index(Index),
		ClassName(ClassName)
	{
	}

	FOtherAssetObject(): Index(0)
	{
	}
};

USTRUCT()
struct FUE4AssetData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FBlueprintClassObject> BlueprintClasses;

	UPROPERTY()
	TArray<FK2GraphNodeObject> K2VariableSets;

	UPROPERTY()
	TArray<FOtherAssetObject> OtherClasses;
};

struct FObjectExportSerialized
{
	int Index;
	FString ObjectName;
	FString ClassName;
	FString SuperClassName;

	bool IsBlueprintGeneratedClass() const
	{
		return ClassName == BLUEPRINT_GENERATED_CLASS_NAME
			|| ClassName == WIDGET_BLUEPRINT_GENERATED_CLASS;
	}
};
