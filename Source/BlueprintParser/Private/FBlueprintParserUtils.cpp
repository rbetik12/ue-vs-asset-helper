#include "FBlueprintParserUtils.h"

static FObjectExportSerialized ReadObjectExport(const FLinkerLoad* Linker, const FObjectExport& ObjExport, const int Index)
{
	FObjectExportSerialized ObjectExportSerialized;
	ObjectExportSerialized.Index = Index;
	ObjectExportSerialized.ObjectName = ObjExport.ObjectName.ToString();
	ObjectExportSerialized.ClassName = Linker->ImpExp(ObjExport.ClassIndex).ObjectName.ToString();
	
	if (ObjExport.SuperIndex.IsNull())
	{
		ObjectExportSerialized.SuperClassName = FString();
	}
	else
	{
		ObjectExportSerialized.SuperClassName = Linker->ImpExp(ObjExport.SuperIndex).ObjectName.ToString();
	}

	return ObjectExportSerialized;
}

FUE4AssetData FBlueprintParserUtils::ParseUObject(const UObject* Object)
{
	const auto PackageName = Object->GetPackage()->GetFName();
	FPackagePath PackagePath;
	FPackagePath::TryFromPackageName(PackageName, PackagePath);
	
	const auto Linker = GetPackageLinker(nullptr, PackagePath, 0x0, nullptr, nullptr, nullptr, nullptr);

	TArray<FBlueprintClassObject> BlueprintClassObjects;
	TArray<FK2GraphNodeObject> K2GraphNodeObjects;
	TArray<FOtherAssetObject> OtherAssetObjects;
	for (int Index = 0; Index < Linker->ExportMap.Num(); Index++)
	{
		auto& ObjectExp = Linker->ExportMap[Index];
		FObjectExportSerialized ObjectExportSerialized = ReadObjectExport(Linker, ObjectExp, Index);
	
		if (ObjectExportSerialized.IsBlueprintGeneratedClass() && !ObjectExportSerialized.SuperClassName.IsEmpty())
		{
			BlueprintClassObjects.Add(FBlueprintClassObject(Index, ObjectExportSerialized.ObjectName,
															ObjectExportSerialized.ClassName,
															ObjectExportSerialized.SuperClassName,
															PackageName.ToString()));
		}
		else
		{
			const EKind Kind = FK2GraphNodeObject::GetKindByClassName(ObjectExportSerialized.ClassName);
			if (Kind != EKind::Other)
			{
				if (ObjectExp.Object)
				{
					const UK2Node* Node = Cast<UK2Node>(ObjectExp.Object);
					FString MemberName;
					FK2GraphNodeObject::GetMemberNameByClassName(Node, MemberName);
					
					K2GraphNodeObjects.Add(FK2GraphNodeObject(Index, Kind, MemberName));
				}
			}
			else
			{
				OtherAssetObjects.Add(FOtherAssetObject(Index, ObjectExportSerialized.ClassName));
			}
		}
	}
	
	FUE4AssetData AssetData;
	AssetData.BlueprintClasses = std::move(BlueprintClassObjects);
	AssetData.OtherClasses = std::move(OtherAssetObjects);
	AssetData.K2VariableSets = std::move(K2GraphNodeObjects);
	
	return AssetData;
}