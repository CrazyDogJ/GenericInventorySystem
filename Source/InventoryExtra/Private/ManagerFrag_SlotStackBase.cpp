// Fill out your copyright notice in the Description page of Project Settings.


#include "ManagerFrag_SlotStackBase.h"

#include "InventorySlotEntry.h"
#include "ItemFrag_SlotStackBase.h"
#include "Core/InventoryItemDefBase.h"
#include "Core/InventoryItemInstanceBase.h"
#include "Net/UnrealNetwork.h"

void UManagerFrag_SlotStackBase::K2_BeginPlay_Implementation()
{
	Super::K2_BeginPlay_Implementation();

	InventorySlotArray.Entries.SetNum(SlotsCount);
	InventorySlotArray.MarkArrayDirty();
}

void UManagerFrag_SlotStackBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, CategoryTag);
	DOREPLIFETIME(ThisClass, SlotsCount);
	DOREPLIFETIME(ThisClass, InventorySlotArray);
}

TArray<TObjectPtr<UObject>> UManagerFrag_SlotStackBase::CollectReplicatedSubobjects()
{
	return TArray<TObjectPtr<UObject>>();
	//TArray<TObjectPtr<UObject>> Result;
	//for (auto Itr : InventorySlotArray.Entries)
	//{
	//	Result.Add(Itr.ItemInstance);
	//}
	//return Result;
}

void UManagerFrag_SlotStackBase::MarkItemDirty(int Index)
{
	if (InventorySlotArray.Entries.IsValidIndex(Index))
	{
		InventorySlotArray.MarkItemDirty(InventorySlotArray.Entries[Index]);
	}
}

void UManagerFrag_SlotStackBase::MarkArrayDirty()
{
	InventorySlotArray.MarkArrayDirty();
}

int UManagerFrag_SlotStackBase::FindEmpty() const
{
	return InventorySlotArray.Entries.IndexOfByPredicate([](const FInventorySlotEntry& Entry)
	{
		return Entry.IsSlotEmpty();
	});
}

int UManagerFrag_SlotStackBase::FindStack(UInventoryItemDefBase* ItemDef)
{
	if (ItemDef)
	{
		if (const int MaxStackAmount = GetItemMaxStackAmount(ItemDef); MaxStackAmount > 0)
		{
			return InventorySlotArray.Entries.IndexOfByPredicate([ItemDef, MaxStackAmount](const FInventorySlotEntry& Entry)
			{
				return Entry.ItemDefinition == ItemDef && MaxStackAmount > Entry.StackCount;
			});
		}
	}

	return INDEX_NONE;
}

const UItemFrag_SlotStackBase* UManagerFrag_SlotStackBase::GetItemFragFromItemDef(UInventoryItemDefBase* ItemDef)
{
	if (!ItemDef)
	{
		return nullptr;
	}

	const auto Frag = ItemDef->FindFragmentByClass(UItemFrag_SlotStackBase::StaticClass());
	return Cast<UItemFrag_SlotStackBase>(Frag);
}

int UManagerFrag_SlotStackBase::GetItemMaxStackAmount(UInventoryItemDefBase* ItemDef)
{
	const auto StackFrag = GetItemFragFromItemDef(ItemDef);
	if (!StackFrag)
	{
		return INDEX_NONE;
	}

	// TODO : Override max stack count for some game need.
	return StackFrag->CategoryTags.HasTag(CategoryTag) ? StackFrag->MaxStackAmount : INDEX_NONE;
}

int UManagerFrag_SlotStackBase::AddItemDef(UInventoryItemDefBase* InItemDef, int Count)
{
	// 1.Validate
	if (InItemDef == nullptr)
	{
		return Count;
	}
	const int MaxStackAmount = GetItemMaxStackAmount(InItemDef);
	if (MaxStackAmount < 0)
	{
		return Count;
	}
	
	// 2.First find stack.
	int FindStackIndex = FindStack(InItemDef);
	// Loop find stack.
	while (FindStackIndex >= 0 && Count > 0)
	{
		// Remain count calc.
		const int FindStackRemainAmount = MaxStackAmount - InventorySlotArray.Entries[FindStackIndex].StackCount;
		// Cache values
		const auto Bool = FindStackRemainAmount >= Count;
		const auto Amount = Bool ? Count : FindStackRemainAmount;
		// Set values
		InventorySlotArray.Entries[FindStackIndex].StackCount += Amount;
		Count = Count - Amount;
		InventorySlotArray.MarkItemDirty(InventorySlotArray.Entries[FindStackIndex]);
		// End Add Function
		if (Bool)
		{
			return 0;
		}
		// While find stack
		FindStackIndex = FindStack(InItemDef);
	}

	// 3.Begin find empty.
	int FindEmptyIndex = FindEmpty();
	// Loop find empty
	while (FindEmptyIndex >= 0 && Count > 0)
	{
		const auto Bool = MaxStackAmount >= Count;
		const auto Amount = Bool ? Count : MaxStackAmount;
		
		InventorySlotArray.Entries[FindEmptyIndex].ItemDefinition = InItemDef;
		InventorySlotArray.Entries[FindEmptyIndex].StackCount += Amount;
		Count = Count - Amount;
		InventorySlotArray.MarkItemDirty(InventorySlotArray.Entries[FindEmptyIndex]);
		// End Add Function
		if (Bool)
		{
			return 0;
		}
		// While find stack
		FindEmptyIndex = FindEmpty();
	}
	
	return Count;
}

int UManagerFrag_SlotStackBase::AddItemInstance(UInventoryItemInstanceBase* ItemInstance, int Count)
{
	if (ItemInstance)
	{
		return AddItemDef(ItemInstance->InventoryItemDef, Count);
	}
	
	return Count;
}
