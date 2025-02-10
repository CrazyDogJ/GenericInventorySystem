#include "InventoryUtility.h"
#include "Inventory.h"
#include "InventoryContainerComponent.h"

#pragma region InventorySlot
bool FInventorySlot::IsSlotEmpty() const
{
	return StackedInstances.IsEmpty() && ItemDefinition == nullptr && StackAmount == 0;
}

FString FInventorySlot::GetDebugString() const
{
	FString DebugString = FString::Printf(TEXT("Slot Category : %s, Item : %s * %d"), *SlotCategoryTag.ToString(), *ItemDefinition.GetName(), GetItemStackCount());
	return DebugString;
}

void FInventorySlot::SwitchSlot(FInventorySlot& Slot)
{
	if (ItemDefinition != nullptr && Slot.ItemDefinition != nullptr)
	{
		if (ItemDefinition->GetMaxStackAmount(Slot.SlotCategoryTag) < StackAmount || Slot.ItemDefinition->GetMaxStackAmount(SlotCategoryTag) < Slot.StackAmount)
		{
			return;
		}
	}
	auto CachedStackedInstances = StackedInstances;
	auto CachedItemDefinition = ItemDefinition;
	auto CachedStackAmount = StackAmount;
	
	StackedInstances = Slot.StackedInstances;
	ItemDefinition = Slot.ItemDefinition;
	StackAmount = Slot.StackAmount;

	Slot.StackedInstances = CachedStackedInstances;
	Slot.ItemDefinition = CachedItemDefinition;
	Slot.StackAmount = CachedStackAmount;
}

int FInventorySlot::GetItemStackCount() const
{
	if (ItemDefinition != nullptr)
	{
		if (ItemDefinition->ItemInstanceType == IIT_Multiple)
		{
			return StackedInstances.Num();
		}
		return StackAmount;
	}
	return 0;
}
#pragma endregion 

void FInventoryList::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	NotifyComponentListChanged(RemovedIndices, ChangeType_Removed);
}

void FInventoryList::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	NotifyComponentListChanged(AddedIndices, ChangeType_Added);
}

void FInventoryList::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	NotifyComponentListChanged(ChangedIndices, ChangeType_Changed);
}

void FInventoryList::NotifyComponentListChanged(const FInventorySlot& Slot, const int Index,
                                                const TEnumAsByte<EArrayChangeType> ChangeType) const
{
	if (auto Comp = Cast<UInventoryContainerComponent>(OwnerComponent))
	{
		Comp->K2_InventoryListChanged(Slot, Index, ChangeType);
	}
}

void FInventoryList::NotifyComponentListChanged(const TArrayView<int32>& Indices,
	const TEnumAsByte<EArrayChangeType> ChangeType)
{
	if (auto Comp = Cast<UInventoryContainerComponent>(OwnerComponent))
	{
		for (auto Index : Indices)
		{
			Comp->K2_InventoryListChanged(Slots[Index], Index, ChangeType);
		}
	}
}

void FInventoryList::EmptySlotAt(int Index)
{
	// Validate first.
	if (!Slots.IsValidIndex(Index))
	{
		return;
	}
	auto& Slot = Slots[Index];
	if (Slot.IsSlotEmpty())
	{
		return;
	}

	// Destroy all instances and empty the slot.
	for (auto InstanceItr : Slot.StackedInstances)
	{
		if (InstanceItr)
		{
			InstanceItr->OnInstanceDestroyed();
		}
	}
	Slot.StackedInstances.Empty();
	Slot.ItemDefinition = nullptr;
	Slot.StackAmount = 0;

	// Mark dirty.
	MarkItemDirty(Slot);
	NotifyComponentListChanged(Slot, Index, ChangeType_Changed);
}

void FInventoryList::SetItemStackCountAt(int Index, int InCount)
{
	// Validate first.
	if (!Slots.IsValidIndex(Index))
	{
		return;
	}
	auto& Slot = Slots[Index];
	if (Slot.ItemDefinition == nullptr || Slot.StackAmount == InCount)
	{
		return;
	}
	
	// Will empty the slot.
	if (InCount == 0)
	{
		EmptySlotAt(Index);
		return;
	}
	
	// Check max stack amount.
	auto FoundMaxStack = Slot.ItemDefinition->GetMaxStackAmount(Slot.SlotCategoryTag);
	if (FoundMaxStack <= 0)
	{
		UE_LOG(LogInventory, Warning, TEXT("Test max stack finding here!"))
		return;
	}

	// Calculation!!!
	int Delta = InCount - Slot.StackAmount;
	switch (Slot.ItemDefinition->ItemInstanceType)
	{
	case IIT_None:
		Slot.StackAmount = FMath::Clamp(InCount, 1, FoundMaxStack);
		break;
	case IIT_OnlyOne:
		// Deal with instances.
		if (Slot.ItemDefinition->DefaultItemInstance == nullptr)
		{
			UE_LOG(LogInventory, Error, TEXT("Has no default item instance, please check item definition : %s"), *Slot.ItemDefinition->GetName())
		}
		else if (Delta > 0 && Slot.StackAmount == 0)
		{
			// Only one instance created
			Slot.StackedInstances.Emplace(AddNewItemInstance(Slot.ItemDefinition));
		}
		Slot.StackAmount = FMath::Clamp(InCount, 1, FoundMaxStack);
		break;
	case IIT_Multiple:
		// Deal with instances.
		if (Slot.ItemDefinition->DefaultItemInstance == nullptr)
		{
			UE_LOG(LogInventory, Error, TEXT("Has no default item instance, please check item definition : %s"), *Slot.ItemDefinition->GetName())
		}
		else
		{
			if (Delta > 0)
			{
				// Add new instance
				for (int Idx = 0; Idx < Delta; ++Idx)
				{
					Slot.StackedInstances.Emplace(AddNewItemInstance(Slot.ItemDefinition));
				}
			}
			else
			{
				// Remove last instance
				for (int Idx = 0; Idx > Delta; --Idx)
				{
					Slot.StackedInstances.Last()->OnInstanceDestroyed();
					Slot.StackedInstances.RemoveAt(Slot.StackedInstances.Num() - 1);
				}
			}
			Slot.StackAmount = FMath::Clamp(InCount, 1, FoundMaxStack);
		}
		break;
	default: ;
	}
	
	MarkItemDirty(Slot);
	NotifyComponentListChanged(Slot, Index, ChangeType_Changed);
}

UInventoryItemInstance* FInventoryList::AddNewItemInstance(const UInventoryItemDefinition* ItemDef) const
{
	// Validate!
	if (!ItemDef)
	{
		return nullptr;
	}
	if (ItemDef->ItemInstanceType == IIT_None || !ItemDef->DefaultItemInstance)
	{
		//Skip
		//UE_LOG(LogInventory, Error, TEXT("Item Definition has no item instance class, please check this item def : %s"), *ItemDef->GetName());
		return nullptr;
	}
	// If only one instance, we use exist item instance.
	if (ItemDef->ItemInstanceType == IIT_OnlyOne)
	{
		for (auto Slot : Slots)
		{
			for (auto Instance : Slot.StackedInstances)
			{
				if (Instance && Instance->GetItemDef() == ItemDef)
				{
					return Instance;
				}
			}
		}
	}
	// Add new instance and initialize and return.
	auto NewItemInstance = NewObject<UInventoryItemInstance>(OwnerComponent->GetOwner(),
		ItemDef->DefaultItemInstance.GetClass(),
		NAME_None, RF_NoFlags, ItemDef->DefaultItemInstance);
	NewItemInstance->SetItemDef(ItemDef);
	NewItemInstance->OnInstanceCreated();
	return NewItemInstance;
}

int FInventoryList::FindCategoryLastItemIndex(const FGameplayTag SlotCategoryTag) const
{
	// Reverse for loop find last category last item
	for (int32 i = Slots.Num() - 1; i >= 0; --i)
	{
		if (Slots[i].SlotCategoryTag.MatchesTag(SlotCategoryTag))
		{
			return i;
		}
	}
	return -1;
}

void FInventoryList::AddEmptySlots(const int& EmptySlotsAmount, const FGameplayTag& SlotCategoryTag)
{
	// If find category, will add and return
	auto CategoryFindLast = FindCategoryLastItemIndex(SlotCategoryTag);
	if (CategoryFindLast >= 0)
	{
		for (int Idx = 0; Idx < EmptySlotsAmount; ++Idx)
		{
			Slots.Insert(FInventorySlot(SlotCategoryTag), CategoryFindLast + 1 + Idx);
			MarkItemDirty(Slots[CategoryFindLast + 1 + Idx]);
			NotifyComponentListChanged(Slots[CategoryFindLast + 1 + Idx], CategoryFindLast + 1 + Idx, ChangeType_Added);
		}
		return;
	}
	
	for (int Idx = 0; Idx < EmptySlotsAmount; ++Idx)
	{
		auto NewSlot = Slots.Add(FInventorySlot(SlotCategoryTag));
		MarkItemDirty(Slots[NewSlot]);
		NotifyComponentListChanged(Slots[NewSlot], NewSlot, ChangeType_Added);
	}
}

void FInventoryList::AddEmptySlots(const TMap<FGameplayTag, int>& InitMap)
{
	for (auto Pair : InitMap)
	{
		for (int32 Idx = 0; Idx < Pair.Value; ++Idx)
		{
			auto NewSlot = Slots.Add(FInventorySlot(Pair.Key));
			MarkItemDirty(Slots[NewSlot]);
			NotifyComponentListChanged(Slots[NewSlot], NewSlot, ChangeType_Added);
		}
	}
}

TArray<int32> FInventoryList::GetSlotsByCategory(const FGameplayTag& CategoryTag, const bool& MatchAll) const
{
	TArray<int32> Result;
	for (int32 Idx = 0; Idx < Slots.Num(); ++Idx)
	{
		auto Slot = Slots[Idx];
		if (MatchAll)
		{
			if (Slot.SlotCategoryTag.MatchesTagExact(CategoryTag))
			{
				Result.Add(Idx);
			}
		}
		else
		{
			if (Slot.SlotCategoryTag.MatchesTag(CategoryTag))
			{
				Result.Add(Idx);
			}
		}
	}
	return Result;
}

int FInventoryList::FindEmpty(const TArray<FGameplayTag>& SlotCategoryTag) const
{
	for (auto Tag : SlotCategoryTag)
	{
		auto SlotArray = GetSlotsByCategory(Tag);
		for (auto Index : SlotArray)
		{
			if (Slots[Index].IsSlotEmpty())
			{
				return Index;
			}
		}
	}
	return -1;
}

int FInventoryList::FindEmptyForItemDef(const UInventoryItemDefinition* ItemDefinition, int& MaxStackInThisSlot) const
{
	auto Result = FindEmpty(GetItemDefCategoryArray(ItemDefinition));
	if (Result >= 0)
	{
		MaxStackInThisSlot = ItemDefinition->GetMaxStackAmount(Slots[Result].SlotCategoryTag);
	}
	return Result;
}

void FInventoryList::FindStack(const UInventoryItemDefinition* ItemDef, int& Index, int& RemainAmount) const
{
	Index = -1;
	RemainAmount = -1;

	if (ItemDef == nullptr)
	{
		return;
	}

	int MaxStackAmount = -1;
	Index = Slots.IndexOfByPredicate([&ItemDef, &MaxStackAmount](const FInventorySlot& InItem)
	{
		if (!InItem.IsSlotEmpty())
		{
			MaxStackAmount = ItemDef->GetMaxStackAmount(InItem.SlotCategoryTag);
			return InItem.ItemDefinition == ItemDef && InItem.GetItemStackCount() < MaxStackAmount;
		}
		return false;
	});

	if (Index < 0 || MaxStackAmount < 0)
	{
		return;
	}
	
	RemainAmount = MaxStackAmount - Slots[Index].GetItemStackCount();
}

void FInventoryList::StackInstances(const UInventoryItemDefinition* ItemDef, TArray<UInventoryItemInstance*>& InArray, const int SlotIndex, const int SplitAmount)
{
	for (int Idx = 0; Idx < SplitAmount; ++Idx)
	{
		auto NewInstance = InArray[Idx];
		if (NewInstance == nullptr)
		{
			NewInstance = AddNewItemInstance(ItemDef);
		}
		else if (NewInstance->GetOuter() != OwnerComponent->GetOwner())
		{
			// Fix outer problem
			auto NewItemInstance = NewObject<UInventoryItemInstance>(OwnerComponent->GetOwner(),
				ItemDef->DefaultItemInstance.GetClass(),
				NAME_None, RF_NoFlags, NewInstance);
			NewItemInstance->SetItemDef(ItemDef);
			NewItemInstance->OnInstanceCreated();

			NewInstance->OnInstanceDestroyed();
			
			NewInstance = NewItemInstance;
		}
		Slots[SlotIndex].StackedInstances.Emplace(NewInstance);
		InArray[Idx]->OnCategoryChanged(Slots[SlotIndex].SlotCategoryTag);
		InArray.RemoveAt(Idx, EAllowShrinking::No);
	}
	InArray.Shrink();
}

int FInventoryList::AddItem(const UInventoryItemDefinition* ItemDef, int Count)
{
	TArray<UInventoryItemInstance*> InArray;
	InArray.SetNum(Count);
	return AddItem(ItemDef, InArray);
}

int FInventoryList::AddItem(const UInventoryItemDefinition* InItemDef, TArray<UInventoryItemInstance*> Instances)
{
	if (!Instances.IsValidIndex(0) || InItemDef == nullptr)
	{
		return Instances.Num();
	}
	
	// First find stack.
	int FindStackRemainAmount;
	int FindStackIndex;
	FindStack(InItemDef, FindStackIndex, FindStackRemainAmount);

	// Loop find stack.
	while (FindStackIndex >= 0 && Instances.Num() > 0)
	{
		auto Bool = FindStackRemainAmount >= Instances.Num();
		auto Amount = Bool ? Instances.Num() : FindStackRemainAmount;
		if (InItemDef->ItemInstanceType == IIT_Multiple)
		{
			StackInstances(InItemDef, Instances, FindStackIndex, Amount);
		}
		else
		{
			SetItemStackCountAt(FindStackIndex, Slots[FindStackIndex].GetItemStackCount() + Amount);
			Instances.SetNum(Instances.Num() - Amount);
		}
		MarkItemDirty(Slots[FindStackIndex]);
		if (Bool)
		{
			NotifyComponentListChanged(Slots[FindStackIndex], FindStackIndex, ChangeType_Changed);
			return 0;
		}
		FindStack(InItemDef, FindStackIndex, FindStackRemainAmount);
	}

	// Begin find empty.
	int FindEmptySlotMaxStackAmount = -1;
	int FindEmptyIndex = FindEmptyForItemDef(InItemDef, FindEmptySlotMaxStackAmount);
	// Loop find empty.
	while (FindEmptyIndex >= 0 && Instances.Num() > 0)
	{
		auto Bool = FindEmptySlotMaxStackAmount >= Instances.Num();
		auto Amount = Bool ? Instances.Num() : FindEmptySlotMaxStackAmount;
		Slots[FindEmptyIndex].ItemDefinition = const_cast<UInventoryItemDefinition*>(InItemDef);
		Slots[FindEmptyIndex].StackAmount = Amount;
		if (InItemDef->ItemInstanceType == IIT_Multiple)
		{
			// If pick up item actor, these instances will be nullptr!!!
			// So we should fix it through these codes.
			for (int32 Idx = 0; Idx < Instances.Num(); ++Idx)
			{
				if (Instances[Idx] == nullptr)
				{
					Instances[Idx] = AddNewItemInstance(InItemDef);
				}
			}
			StackInstances(InItemDef, Instances, FindEmptyIndex, Amount);
		}
		else
		{
			Slots[FindEmptyIndex].StackedInstances.Emplace(AddNewItemInstance(InItemDef));
			Instances.SetNum(Instances.Num() - Amount);
			UInventoryContainerComponent::NotifyCategoryChanged(Slots[FindEmptyIndex]);
		}
		MarkItemDirty(Slots[FindEmptyIndex]);
		if (Bool)
		{
			NotifyComponentListChanged(Slots[FindEmptyIndex], FindEmptyIndex, ChangeType_Changed);
			return 0;
		}
		FindEmptyIndex = FindEmptyForItemDef(InItemDef, FindEmptySlotMaxStackAmount);
	}
	
	return Instances.Num();
}

TArray<FGameplayTag> FInventoryList::GetItemDefCategoryArray(const UInventoryItemDefinition* ItemDef)
{
	if (ItemDef)
	{
		TArray<FGameplayTag> Result;
		ItemDef->MaxStackAmountPerCategory.GetKeys(Result);
		return Result;
	}
	return {FGameplayTag::EmptyTag};
}

bool FInventoryList::SetItemAt(UInventoryItemDefinition* ItemDef, int Count, const int& Index, bool bForceSet)
{
	if (!Slots.IsValidIndex(Index) || ItemDef == nullptr)
	{
		return false;
	}

	if (!bForceSet)
	{
		if (GetItemDefCategoryArray(ItemDef).Find(Slots[Index].SlotCategoryTag) < 0)
		{
			return false;
		}
	}

	// Empty slot and set item
	for (auto InstanceItr : Slots[Index].StackedInstances)
	{
		InstanceItr->OnInstanceDestroyed();
	}
	Slots[Index].StackedInstances.Empty();
	Slots[Index].ItemDefinition = nullptr;
	Slots[Index].StackAmount = 0;
	
	Slots[Index].ItemDefinition = ItemDef;
	SetItemStackCountAt(Index, Count);
	return true;
}

// Deprecated
void FInventoryList::AddNewInstance(FInventorySlot& Slot, const UInventoryItemDefinition* ItemDef, int StackAmount) const
{
	if (ItemDef == nullptr || StackAmount <= 0)
	{
		return;
	}
	if (!Slot.IsSlotEmpty())
	{
		// Destroy all instances and empty the slot.
		for (auto InstanceItr : Slot.StackedInstances)
		{
			InstanceItr->OnInstanceDestroyed();
		}
		Slot.StackedInstances.Empty();
		Slot.ItemDefinition = nullptr;
		Slot.StackAmount = 0;
	}
	
	int MaxStackAmount = ItemDef->GetMaxStackAmount(Slot.SlotCategoryTag);
	StackAmount = FMath::Clamp(StackAmount, 1, MaxStackAmount);
	Slot.ItemDefinition = const_cast<UInventoryItemDefinition*>(ItemDef);
	Slot.StackAmount = StackAmount;

	switch (ItemDef->ItemInstanceType)
	{
	case IIT_None:
		break;
	case IIT_OnlyOne:
		Slot.StackedInstances.Emplace(AddNewItemInstance(ItemDef));
		break;
	case IIT_Multiple:
		for (int Idx = 0; Idx < StackAmount; ++Idx)
		{
			Slot.StackedInstances.Emplace(AddNewItemInstance(ItemDef));
		}
		break;
	default: ;
	}
}

bool FInventoryList::ItemDefUsed(const UInventoryItemDefinition* ItemDef, int Amount)
{
	if (ItemDef == nullptr || Amount <= 0 || GetTotalItemAmount(ItemDef) < Amount)
	{
		return false;
	}
	
	for (int ID = 0; ID < Slots.Num(); ID++)
	{
		auto& Slot = Slots[ID];
		if (!Slot.ItemDefinition)
		{
			continue;
		}
		
		if (Slot.ItemDefinition == ItemDef && Amount != 0)
		{
			if (Amount > Slot.GetItemStackCount())
			{
				EmptySlotAt(ID);
				Amount -= Slot.GetItemStackCount();
			}
			else
			{
				SetItemStackCountAt(ID, Slot.GetItemStackCount() - Amount);
				return true;
			}
		}
	}
	return false;
}

void FInventoryList::DragDropItem(FInventoryList& DropInventoryList, const int DragIndex, const int DropIndex)
{
	//valid slot
	if (!Slots.IsValidIndex(DragIndex) || !DropInventoryList.Slots.IsValidIndex(DropIndex))
	{
		return;
	}

	// Stack on drop slot
	if (Slots[DragIndex].ItemDefinition == DropInventoryList.Slots[DropIndex].ItemDefinition)
	{
		const int MaxStackAmount = DropInventoryList.Slots[DropIndex].ItemDefinition->GetMaxStackAmount(DropInventoryList.Slots[DropIndex].SlotCategoryTag);
		const int FinalAmount = DropInventoryList.Slots[DropIndex].GetItemStackCount() + Slots[DragIndex].GetItemStackCount();
		const int CalculateAmount = FinalAmount - MaxStackAmount;
		// If not max stack
		if (DropInventoryList.Slots[DropIndex].GetItemStackCount() != MaxStackAmount)
		{
			if (DropInventoryList.Slots[DropIndex].ItemDefinition->ItemInstanceType == IIT_Multiple)
			{
				DropInventoryList.StackInstances(Slots[DropIndex].ItemDefinition, Slots[DragIndex].StackedInstances, DropIndex,
							   CalculateAmount > 0
								   ? MaxStackAmount - Slots[DropIndex].GetItemStackCount()
								   : Slots[DragIndex].GetItemStackCount());
			}
			else
			{
				DropInventoryList.SetItemStackCountAt(DropIndex, CalculateAmount > 0 ? MaxStackAmount : FinalAmount);
				SetItemStackCountAt(DragIndex, CalculateAmount > 0 ? CalculateAmount : 0);
			}
			
			DropInventoryList.MarkItemDirty(DropInventoryList.Slots[DropIndex]);
			MarkItemDirty(Slots[DragIndex]);
			NotifyComponentListChanged(DropInventoryList.Slots[DropIndex], DropIndex, ChangeType_Changed);
			NotifyComponentListChanged(Slots[DragIndex], DragIndex, ChangeType_Changed);
			return;
		}
	}
	else
	{
		auto MaxStack = Slots[DragIndex].ItemDefinition->GetMaxStackAmount(DropInventoryList.Slots[DropIndex].SlotCategoryTag);
		// Drag to empty with little part start
		if (MaxStack < Slots[DragIndex].GetItemStackCount())
		{
			DropInventoryList.Slots[DropIndex].ItemDefinition = Slots[DragIndex].ItemDefinition;
			Slots[DragIndex].StackAmount -= MaxStack;
			switch (Slots[DragIndex].ItemDefinition->ItemInstanceType)
			{
			case IIT_None:
				DropInventoryList.Slots[DropIndex].StackAmount = MaxStack;
				break;
			case IIT_OnlyOne:
				DropInventoryList.SetItemStackCountAt(DropIndex, MaxStack);
				break;
			case IIT_Multiple:
				DropInventoryList.StackInstances(Slots[DragIndex].ItemDefinition, Slots[DragIndex].StackedInstances, DropIndex, MaxStack);
				break;
			default: ;
			}
			DropInventoryList.MarkItemDirty(DropInventoryList.Slots[DropIndex]);
			MarkItemDirty(Slots[DragIndex]);
			DropInventoryList.NotifyComponentListChanged(Slots[DropIndex], DropIndex, ChangeType_Changed);
			NotifyComponentListChanged(Slots[DragIndex], DragIndex, ChangeType_Changed);
			UInventoryContainerComponent::NotifyCategoryChanged(DropInventoryList.Slots[DropIndex]);
			UInventoryContainerComponent::NotifyCategoryChanged(Slots[DragIndex]);
			return;
			//UE_LOG(LogInventory, Warning, TEXT("Can't fit this slot, can only drop part of items to this slot"))
		}
		else if (DropInventoryList.Slots[DropIndex].ItemDefinition == nullptr ||
				DropInventoryList.Slots[DropIndex].ItemDefinition->GetMaxStackAmount(Slots[DragIndex].SlotCategoryTag) >= DropInventoryList.Slots[DropIndex].GetItemStackCount())
		{
			{
				// Switch start
				DropInventoryList.Slots[DropIndex].SwitchSlot(Slots[DragIndex]);
				//Rep net
				DropInventoryList.MarkItemDirty(DropInventoryList.Slots[DropIndex]);
				MarkItemDirty(Slots[DragIndex]);
				DropInventoryList.NotifyComponentListChanged(Slots[DropIndex], DropIndex, ChangeType_Changed);
				NotifyComponentListChanged(Slots[DragIndex], DragIndex, ChangeType_Changed);
				UInventoryContainerComponent::NotifyCategoryChanged(DropInventoryList.Slots[DropIndex]);
				UInventoryContainerComponent::NotifyCategoryChanged(Slots[DragIndex]);
				return;
				// Switch end
			}
		}
	}
}

int FInventoryList::GetTotalItemAmount(const UInventoryItemDefinition* ItemDef) const
{
	if (ItemDef != nullptr)
	{
		int LocalTotalAmount = 0;
		for (FInventorySlot Slot : Slots)
		{
			if (ItemDef == Slot.ItemDefinition)
			{
				LocalTotalAmount += Slot.GetItemStackCount();
			}
		}
		return LocalTotalAmount;
	}
	return -1;
}
