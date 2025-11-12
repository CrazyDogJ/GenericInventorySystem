// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/InventoryInstancedStructDef.h"

#include "Core/InventoryInstancedStructObject.h"

void UInventoryInstancedStructDef::PostLoad()
{
	Super::PostLoad();

	if (InventoryItemObject.GetDefaultObject())
	{
		ObjectDefaultContainer = InventoryItemObject.GetDefaultObject()->RepInstancedStructContainer;
	}
}

void UInventoryInstancedStructDef::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == "InventoryItemObject" && InventoryItemObject.GetDefaultObject())
	{
		ObjectDefaultContainer = InventoryItemObject.GetDefaultObject()->RepInstancedStructContainer;
	}
}

int UInventoryInstancedStructDef::FindDefaultPropertyByStructType(const UScriptStruct* ScriptStruct)
{
	for (int i = 0; i < DefaultProperties.Num(); ++i)
	{
		if (DefaultProperties[i].GetScriptStruct() == ScriptStruct)
		{
			return i;
		}
	}

	return INDEX_NONE;
}

UInventoryInstancedStructObject* UInventoryInstancedStructDef::NewInstancedStructObject(UObject* Outer)
{
	const auto NewInstance = NewObject<UInventoryInstancedStructObject>(Outer, InventoryItemObject->GetClass(), NAME_None, RF_NoFlags, InventoryItemObject);
	NewInstance->InventoryItemDef = this;
	for (auto& Itr : NewInstance->RepInstancedStructContainer.Properties)
	{
		auto ItrStructType = Itr.Data.GetScriptStruct();
		const auto Found = OverrideInstancedStructs.FindByPredicate([ItrStructType](const FInstancedStruct& Struct)
		{
			return Struct.GetScriptStruct() == ItrStructType;
		});

		if (Found)
		{
			Itr.Data = *Found;
			NewInstance->RepInstancedStructContainer.MarkItemDirty(Itr);
		}
	}
	NewInstance->RepInstancedStructContainer.MarkArrayDirty();
	return NewInstance;
}
