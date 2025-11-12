// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/InventoryInstancedStructArray.h"

#include "Inventory.h"

const UScriptStruct* FInventoryInstancedStructEntry::GetDataStructType() const
{
	return Data.GetScriptStruct();
}

bool FInventoryInstancedStructContainer::IsPropertiesValid() const
{
	TArray<const UScriptStruct*> Types;
	
	for (auto Entry : Properties)
	{
		auto EntryType = Entry.GetDataStructType();
		if (Types.Find(EntryType) != INDEX_NONE)
		{
			UE_LOG(LogInventory, Error, TEXT("Struct type '%s' has already exist"), *EntryType->GetName())
			return false;
		}

		Types.Add(EntryType);
	}

	return true;
}

FInstancedStruct* FInventoryInstancedStructContainer::GetDataStructByType(const UScriptStruct* InType)
{
	for (int i = 0; i < Properties.Num(); ++i)
	{
		if (Properties[i].GetDataStructType() == InType)
		{
			return &Properties[i].Data;
		}
	}

	return nullptr;
}

void UInventoryInstancedStructLibrary::MarkInvContainerItemDirty(FInventoryInstancedStructContainer& InContainer, const int Index)
{
	if (InContainer.Properties.IsValidIndex(Index))
	{
		InContainer.MarkItemDirty(InContainer.Properties[Index]);
	}
}

void UInventoryInstancedStructLibrary::MarkInvContainerArrayDirty(FInventoryInstancedStructContainer& InContainer)
{
	InContainer.MarkArrayDirty();
}

void UInventoryInstancedStructLibrary::MarkInstanceContainerItemDirty(FInventoryInstanceObjectContainer& InContainer,
	const int Index)
{
	if (InContainer.Instances.IsValidIndex(Index))
	{
		InContainer.MarkItemDirty(InContainer.Instances[Index]);
	}
}

void UInventoryInstancedStructLibrary::MarkInstanceContainerArrayDirty(FInventoryInstanceObjectContainer& InContainer)
{
	InContainer.MarkArrayDirty();
}

bool UInventoryInstancedStructLibrary::IsStructTypeEqual(const FInstancedStruct& InstancedStruct,
                                                         const UScriptStruct* InStructType)
{
	return InstancedStruct.GetScriptStruct() == InStructType;
}

int UInventoryInstancedStructLibrary::FindInstancedStruct(const FInventoryInstancedStructContainer& Inventory,
	UScriptStruct* StructType, const bool bMatchChildOf)
{
	if (!StructType)
	{
		return INDEX_NONE;
	}

	for (int i = 0; i < Inventory.Properties.Num(); ++i)
	{
		auto Entry = Inventory.Properties[i];
		if (const UScriptStruct* S = Entry.Data.GetScriptStruct())
		{
			if (bMatchChildOf ? S->IsChildOf(StructType) : (S == StructType))
			{
				return i;
			}
		}
	}

	return INDEX_NONE;
}
