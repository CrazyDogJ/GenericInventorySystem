// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryContainerComponent.h"

#include "InventoryItemDefinition.h"
#include "Net/UnrealNetwork.h"

class FLifetimeProperty;
struct FReplicationFlags;

UInventoryContainerComponent::UInventoryContainerComponent(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UInventoryContainerComponent::SetItem(TSubclassOf<UInventoryItemDefinition> ItemID, int Count, FGameplayTagStackContainer Tags, int SlotIndex)
{
	List.SetItem(ItemID, Count, Tags, SlotIndex);
}

/*
 *Will auto call when begin play.
 **/
void UInventoryContainerComponent::Initialize()
{
	List.InitializeList(EmptySlotAmount);
}

int UInventoryContainerComponent::AddItem(TSubclassOf<UInventoryItemDefinition> ItemDef, int Count, FGameplayTagStackContainer Tags, int SlotIndex /*= -1*/)
{
    return List.AddItem(ItemDef, Count, Tags, SlotIndex);
}

void UInventoryContainerComponent::RemoveItem(int Count, int SlotIndex)
{
    List.RemoveItem(Count, SlotIndex);
}

void UInventoryContainerComponent::GenerateLoot()
{
    if (bGenerateLoot)
    {
        if (GeneratedLootItems.Num() > 0)
        {
            for (FItemProbabilitySetting Setting : GeneratedLootItems)
            {
                if (Setting.ItemID != nullptr)
                {
                    if (FMath::RandRange(0.f, 1.f) < Setting.ItemProbability)
                    {
                        const int AddCount = FMath::RandRange(Setting.GenerateCountMin, Setting.GenerateCountMax);
                        FGameplayTagStackContainer EmptyContainer;
                        AddItem(Setting.ItemID, AddCount, EmptyContainer, -1);
                    }
                }
            }
        }
    }

    bGenerateLoot = false;
}

void UInventoryContainerComponent::BeginPlay()
{
    Super::BeginPlay();

    Initialize();
}

void UInventoryContainerComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, List);
}

void FContainerList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{

}

void FContainerList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{

}

void FContainerList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{

}

void FContainerList::SetItem(TSubclassOf<UInventoryItemDefinition> ItemID, int Count, FGameplayTagStackContainer Tags, int SlotIndex)
{
	FContainerSlot Slot;
	Slot.ItemID = ItemID;
	Slot.StackCount = Count;
	Slot.StackTagContainer = Tags;
	Slots[SlotIndex] = Slot;
	MarkItemDirty(Slots[SlotIndex]);
}

void FContainerList::InitializeList(int EmptySlotAmount)
{
	Slots.SetNum(EmptySlotAmount);
	MarkArrayDirty();
}

int FContainerList::FindEmpty()
{
    const int index = Slots.IndexOfByPredicate([](const FContainerSlot& InItem)
    {
        return InItem.ItemID == nullptr;
    });
    return index;
}

void FContainerList::FindStack(TSubclassOf<UInventoryItemDefinition> ItemDef, int& index, int& remainAmount)
{
    int id = 0;

    index = -1;
    remainAmount = -1;
    if (ItemDef != nullptr)
    {
        bool found = false;
        for (FContainerSlot Slot : Slots)
        {
            if (Slot.ItemID != nullptr)
            {
                if (Slot.ItemID == ItemDef && Slot.StackCount < ItemDef.GetDefaultObject()->MaxStackAmount && found == false)
                {
                    found = true;
                    index = id;
                    remainAmount = ItemDef.GetDefaultObject()->MaxStackAmount - Slot.StackCount;
                }
                else
                {
                    id++;
                }
            }
        }
    }
    else
    {
        // no stack info;
        return;
    }
}

int FContainerList::AddItem(TSubclassOf<UInventoryItemDefinition> ItemDef, int Count, FGameplayTagStackContainer Tags, int SlotIndex /*= -1*/)
{
    if (ItemDef != nullptr)
    {
        //寻找堆叠项目
        int FindStackRemainAmount;
        int FindStackIndex;
        FindStack(ItemDef, FindStackIndex, FindStackRemainAmount);

        while (FindStackIndex >= 0 && Count > 0)
        {
            FindStack(ItemDef, FindStackIndex, FindStackRemainAmount);
            if (FindStackRemainAmount >= Count)
            {
                Slots[FindStackIndex].StackCount += Count;
                MarkItemDirty(Slots[FindStackIndex]);
                Count = 0;
            }
            else if (FindStackIndex >= 0)
            {
                Slots[FindStackIndex].StackCount += FindStackRemainAmount;
                MarkItemDirty(Slots[FindStackIndex]);
                Count -= FindStackRemainAmount;
            }
        };
        //完成寻找堆叠项目
        if (Count <= 0)
        {
            return 0;
        }
        //开始寻找空位逻辑
        else
        {
            int FindEmptyIndex;
            FindEmptyIndex = FindEmpty();
            //循环，直到空位被填完
            while (FindEmptyIndex >= 0 && Count > 0)
            {
                FContainerSlot Slot;
                Slot.ItemID = ItemDef;
                Slot.StackCount = (Count >= ItemDef.GetDefaultObject()->MaxStackAmount) ? ItemDef.GetDefaultObject()->MaxStackAmount : Count;
                Count -= (Count >= ItemDef.GetDefaultObject()->MaxStackAmount) ? ItemDef.GetDefaultObject()->MaxStackAmount : Count;

                //如果有TagStack就设置，没有就原Def
                if (Tags.GetTagStacks().Num() > 0)
                {
                    Slot.StackTagContainer = Tags;
                }
                Slots[(SlotIndex >= 0) ? SlotIndex : FindEmptyIndex] = Slot;
                MarkItemDirty(Slots[(SlotIndex >= 0) ? SlotIndex : FindEmptyIndex]);
                FindEmptyIndex = FindEmpty();
            }
            return Count;
        }
    }
    else
    {
        return -1;
    }
}

void FContainerList::RemoveItem(int Count, int SlotIndex)
{
    if (Slots[SlotIndex].StackCount >= 1)
    {
        Slots[SlotIndex].StackCount -= Count;
        MarkItemDirty(Slots[SlotIndex]);
    }
    if (Slots[SlotIndex].StackCount <= 0)
    {
        FContainerSlot EmptySlot;
        Slots[SlotIndex] = EmptySlot;
    }
}