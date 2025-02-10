// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryContainerComponent.h"
#include "InventorySettings.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"

UInventoryContainerComponent::UInventoryContainerComponent(const FObjectInitializer& ObjectInitializer)
{
	bAllowAnyoneToDestroyMe = true;
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	if (auto InventorySetting = GetMutableDefault<UInventorySettings>())
	{
		InventorySlotAmount = {TPair<FGameplayTag, int>(InventorySetting->DefaultCategoryTag, 10)};
	}
}

void UInventoryContainerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InventoryList);
}

void UInventoryContainerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (this)
	{
		InventoryList.OwnerComponent = this;
	}

	if (GetOwner()->HasAuthority())
	{
		InventoryList.AddEmptySlots(InventorySlotAmount);
	}
}

void UInventoryContainerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	for (auto Slot : InventoryList.Slots)
	{
		for (auto Instance : Slot.StackedInstances)
		{
			if (Instance)
			{
				Instance->OnInstanceDestroyed();
			}
		}
	}
	
	InventoryList.Slots.Empty();
	InventoryList.MarkArrayDirty();
}

bool UInventoryContainerComponent::ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch,
	FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (FInventorySlot& Slot : InventoryList.Slots)
	{
		for (auto Itr : Slot.StackedInstances)
		{
			if (Itr && IsValid(Itr))
			{
				WroteSomething |= Channel->ReplicateSubobject(Itr, *Bunch, *RepFlags);
			}
		}
	}

	return WroteSomething;
}

void UInventoryContainerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	if (IsUsingRegisteredSubObjectList())
	{
		for (const FInventorySlot& Slot : InventoryList.Slots)
		{
			for (auto Itr : Slot.StackedInstances)
			{
				if (Itr && IsValid(Itr))
				{
					AddReplicatedSubObject(Itr);
				}
			}
		}
	}
}

void UInventoryContainerComponent::AddEmptySlots(const FGameplayTag& SlotCategoryTag, int Count)
{
	InventoryList.AddEmptySlots(Count, SlotCategoryTag);
}

int UInventoryContainerComponent::AddItem(UInventoryItemDefinition* ItemDef, const int Count)
{
	if (ItemDef != nullptr && Count > 0)
	{
		const int Return = InventoryList.AddItem(ItemDef, Count);
		return Return;
	}
	return -1;
}

bool UInventoryContainerComponent::ItemDefUsed(const UInventoryItemDefinition* ItemDef, const int Amount)
{
	return InventoryList.ItemDefUsed(ItemDef, Amount);
}

int UInventoryContainerComponent::ItemTotalAmount(const UInventoryItemDefinition* ItemDef)
{
	return InventoryList.GetTotalItemAmount(ItemDef);
}

int UInventoryContainerComponent::FindEmpty(const TArray<FGameplayTag>& CategoryTags) const
{
	return InventoryList.FindEmpty(CategoryTags);
}

bool UInventoryContainerComponent::CheckInventoryExchange(TMap<UInventoryItemDefinition*, int> OutItems,
	TMap<UInventoryItemDefinition*, int> InItems, const int OutTimes, const int InTimes)
{
	FInventoryList List = FInventoryList();
	List = InventoryList;

	for (const TPair<UInventoryItemDefinition*, int>& Pair : OutItems)
	{
		if (!List.ItemDefUsed(Pair.Key, Pair.Value * OutTimes))
		{
			return false;
		}
	}

	for (const TPair<UInventoryItemDefinition*, int>& Pair : InItems)
	{
		if (List.AddItem(Pair.Key, Pair.Value * InTimes) != 0)
		{
			return false;
		}
	}
	
	return true;
}


void UInventoryContainerComponent::SplitItem(const int Index, const int Amount)
{
	check(InventoryList.Slots.IsValidIndex(Index))
	if (InventoryList.Slots[Index].ItemDefinition == nullptr)
	{
		return;
	}
	
	int MaxStack;
	if (const int EmptyIndex = InventoryList.FindEmptyForItemDef(InventoryList.Slots[Index].ItemDefinition, MaxStack); EmptyIndex >= 0)
	{
		// Only not multiple right now, TODO : Do it later
		if (InventoryList.Slots[Index].ItemDefinition->ItemInstanceType != IIT_Multiple)
		{
			if (InventoryList.SetItemAt(InventoryList.Slots[Index].ItemDefinition, Amount, EmptyIndex))
			{
				InventoryList.SetItemStackCountAt(Index, InventoryList.Slots[Index].GetItemStackCount() - Amount);
			}
		}
	}
}


void UInventoryContainerComponent::RemoveItem(const int Index, const int Amount)
{
	if (InventoryList.Slots.IsValidIndex(Index))
	{
		InventoryList.SetItemStackCountAt(Index, InventoryList.Slots[Index].GetItemStackCount() - Amount);
	}
}

TArray<int32> UInventoryContainerComponent::GetSlotsByCategory(const FGameplayTag CategoryTag, const bool MatchAll) const
{
	return InventoryList.GetSlotsByCategory(CategoryTag, MatchAll);
}

void UInventoryContainerComponent::DragDropItem(UInventoryContainerComponent* Comp, const int DragIndex, const int DropIndex)
{
	// Invalid pointer will default to self
	if (!Comp)
	{
		Comp = this;
	}
	
	if (Comp == this && DragIndex == DropIndex)
	{
		return;
	}
	
	InventoryList.DragDropItem(Comp->InventoryList, DragIndex, DropIndex);
}

void UInventoryContainerComponent::ClearItems()
{
	for (int a = 0; a <= InventoryList.Slots.Num() - 1; a = a + 1)
	{
		InventoryList.EmptySlotAt(a);
	}
}

FInventorySaveData UInventoryContainerComponent::GetSaveData()
{
	FInventorySaveData InventorySaveData;
	for (auto Slot : InventoryList.Slots)
	{
		if (auto Value = InventorySaveData.SlotsAmount.Find(Slot.SlotCategoryTag))
		{
			InventorySaveData.SlotsAmount.Add(Slot.SlotCategoryTag, *Value + 1);
		}
		else
		{
			InventorySaveData.SlotsAmount.Add(Slot.SlotCategoryTag, 1);
		}
	}
	InventorySaveData.bIsValid = true;
	for (int Idx = 0; Idx < InventoryList.Slots.Num(); Idx++)
	{
		if (!InventoryList.Slots[Idx].ItemDefinition)
		{
			continue;
		}

		TArray<FItemInstanceSaveData> InstancesSaveData;
		for (auto Itr : InventoryList.Slots[Idx].StackedInstances)
		{
			if (Itr)
			{
				TArray<uint8> InstanceData;
				Itr->K2_OnPreSaveGame();
				FMemoryWriter MemoryWriter(InstanceData, true);
				FItemInstanceArchive Ar(MemoryWriter);
				Itr->Serialize(Ar);
				auto NewInstanceSaveData = FItemInstanceSaveData(InstanceData);
				InstancesSaveData.Emplace(NewInstanceSaveData);
			}
		}
		
		auto NewSlotData = FItemSlotSaveData(InstancesSaveData, InventoryList.Slots[Idx].ItemDefinition, InventoryList.Slots[Idx].GetItemStackCount());
		InventorySaveData.SlotDataMap.Add(Idx, NewSlotData);
	}
	
	return InventorySaveData;
}

void UInventoryContainerComponent::NotifyCategoryChanged(const FInventorySlot& Slot)
{
	for (const auto Itr : Slot.StackedInstances)
	{
		if (Itr == nullptr)
		{
			continue;
		}
		Itr->OnCategoryChanged(Slot.SlotCategoryTag);
	}
}

bool UInventoryContainerComponent::LoadSaveData(FInventorySaveData SaveData)
{
	if (!SaveData.IsValid())
	{
		return false;
	}
	
	ClearItems();
	// Set empty slot
	InventorySlotAmount = SaveData.SlotsAmount;
	InventoryList.Slots.Empty();
	for (auto Itr : SaveData.SlotsAmount)
	{
		InventoryList.AddEmptySlots(Itr.Value, Itr.Key);
	}

	for (auto DataPair : SaveData.SlotDataMap)
	{
		InventoryList.Slots[DataPair.Key].ItemDefinition = DataPair.Value.ItemDefinition;
		InventoryList.Slots[DataPair.Key].StackAmount = DataPair.Value.StackCount;
		TArray<UInventoryItemInstance*> Instances;
		for (auto ItrData : DataPair.Value.InstancesData)
		{
			if (auto InstanceInDef = InventoryList.AddNewItemInstance(DataPair.Value.ItemDefinition))
			{
				FMemoryReader MemoryReader(ItrData.InstanceData, true);
				FItemInstanceArchive Ar(MemoryReader);
				InstanceInDef->Serialize(Ar);
				InstanceInDef->K2_OnPostLoadGame();
				Instances.Emplace(InstanceInDef);
			}
		}
		InventoryList.Slots[DataPair.Key].StackedInstances = Instances;
		InventoryList.MarkItemDirty(InventoryList.Slots[DataPair.Key]);
		NotifyCategoryChanged(InventoryList.Slots[DataPair.Key]);
	}
	
	return true;
}
