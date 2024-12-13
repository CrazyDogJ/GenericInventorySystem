// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryContainerComponent.h"

#include "InventoryItemDefinition.h"
#include "InventoryManagerComponent.h"
#include "ItemInstances/InventoryItemInstance_StatTags.h"
#include "Net/UnrealNetwork.h"

class FLifetimeProperty;
struct FReplicationFlags;

UInventoryContainerComponent::UInventoryContainerComponent(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UInventoryContainerComponent::SetItem(UInventoryItemDefinition* ItemID, int Count, FGameplayTagStackContainer Tags, int SlotIndex)
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

int UInventoryContainerComponent::AddItem(UInventoryItemDefinition* ItemDef, int Count, FGameplayTagStackContainer Tags, int SlotIndex /*= -1*/)
{
    return List.AddItem(ItemDef, Count, Tags, SlotIndex);
}

void UInventoryContainerComponent::RemoveItem(int Count, int SlotIndex)
{
    List.RemoveItem(Count, SlotIndex);
}

void UInventoryContainerComponent::GenerateLoot()
{
    if (!bGenerateLoot || GeneratedLootItems.Num() <= 0)
    {
        return;
    }

    for (FItemProbabilitySetting Setting : GeneratedLootItems)
    {
        if (Setting.ItemID != nullptr)
        {
            if (FMath::RandRange(0.f, 1.f) < Setting.ItemProbability)
            {
                const int AddCount = FMath::RandRange(Setting.GenerateCountMin, Setting.GenerateCountMax);
                AddItem(Setting.ItemID, AddCount, FGameplayTagStackContainer(), -1);
            }
        }
    }

    bGenerateLoot = false;
}

void UInventoryContainerComponent::DragDropItem(int DragIndex, int DropIndex)
{
    List.DragDropItem(DragIndex, DropIndex);
}

void UInventoryContainerComponent::DragItemToInventory(UInventoryManagerComponent* InventoryManager,
    int DragIndex, int DropIndex)
{
    //check valid ptr
    if (!InventoryManager)
    {
        return;
    }
    
    if (!List.Slots.IsValidIndex(DragIndex) || !InventoryManager->InventoryList.Slots.IsValidIndex(DropIndex))
    {
        return;
    }
	
    if (List.Slots[DragIndex].ItemID != nullptr
        && InventoryManager->InventoryList.Slots[DropIndex].GetItemDef() != nullptr
        && List.Slots[DragIndex].ItemID == InventoryManager->InventoryList.Slots[DropIndex].GetItemDef())
    {
        //Stack
        const int maxStackAmount = List.Slots[DragIndex].ItemID->MaxStackAmount;
        const int finalAmount = InventoryManager->InventoryList.Slots[DropIndex].StackCount + List.Slots[DragIndex].StackCount;
        const int calculateAmount = finalAmount - maxStackAmount;
        if (InventoryManager->InventoryList.Slots[DropIndex].StackCount != maxStackAmount)
        {
            if (calculateAmount > 0)
            {
                InventoryManager->InventoryList.Slots[DropIndex].StackCount = maxStackAmount;
                List.Slots[DragIndex].StackCount = calculateAmount;
            }
            else
            {
                InventoryManager->InventoryList.Slots[DropIndex].StackCount = finalAmount;
                RemoveItem(DragIndex, List.Slots[DragIndex].StackCount);
            }
            //Rep net
            InventoryManager->InventoryList.MarkItemDirty(InventoryManager->InventoryList.Slots[DropIndex]);
            List.MarkItemDirty(List.Slots[DragIndex]);
            InventoryManager->K2_InventoryListChanged();
            return;
        }
    }
	
    //Switch
    auto StackCount = InventoryManager->InventoryList.Slots[DragIndex].StackCount;
    auto Instance = Cast<UInventoryItemInstance_StatTags>(InventoryManager->InventoryList.Slots[DropIndex].Instance);
    FContainerSlot Slot = List.Slots[DragIndex];
    FGameplayTagStackContainer TagStacks;
    if (Instance)
    {
        TagStacks = Instance->GetStatTagsContainer();
    }
    SetItem(InventoryManager->InventoryList.Slots[DragIndex].GetItemDef(), StackCount, TagStacks, DragIndex);
    InventoryManager->RemoveItem(DropIndex, StackCount);
    InventoryManager->InventoryList.SetItemAt(Slot.ItemID, Slot.StackCount, Slot.StackTagContainer.GetTagStacks(), DropIndex);
}

void UInventoryContainerComponent::BeginPlay()
{
    if (this)
    {
        List.OwnerComponent = this;
    }

    if (GetOwnerRole() == ROLE_Authority)
    {
        List.Slots.SetNum(EmptySlotAmount);
        List.MarkArrayDirty();
    }
    
    Super::BeginPlay();
}

void UInventoryContainerComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, List);
}

void FContainerList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
    // ReSharper disable once CppExpressionWithoutSideEffects
    Cast<UInventoryContainerComponent>(OwnerComponent)->OnContainerListChanged.Broadcast();
}

void FContainerList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
    // ReSharper disable once CppExpressionWithoutSideEffects
    Cast<UInventoryContainerComponent>(OwnerComponent)->OnContainerListChanged.Broadcast();
}

void FContainerList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
    // ReSharper disable once CppExpressionWithoutSideEffects
    Cast<UInventoryContainerComponent>(OwnerComponent)->OnContainerListChanged.Broadcast();
}

void FContainerList::SetItem(const TObjectPtr<UInventoryItemDefinition> ItemID, int Count, const FGameplayTagStackContainer& Tags, int SlotIndex)
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

int FContainerList::FindEmpty() const
{
    const int index = Slots.IndexOfByPredicate([](const FContainerSlot& InItem)
    {
        return InItem.ItemID == nullptr;
    });
    return index;
}

void FContainerList::FindStack(const TObjectPtr<UInventoryItemDefinition> ItemDef, int& index, int& remainAmount)
{
    index = -1;
    remainAmount = -1;

    if (ItemDef == nullptr)
    {
        return;
    }

    index = Slots.IndexOfByPredicate([&ItemDef](const FContainerSlot& InItem)
    {
        return InItem.ItemID == ItemDef && InItem.StackCount < ItemDef->MaxStackAmount;
    });

    if (index < 0)
    {
        return;
    }
    
    remainAmount = ItemDef->MaxStackAmount - Slots[index].StackCount;
}

int FContainerList::AddItem(const TObjectPtr<UInventoryItemDefinition> ItemDef, int Count, const FGameplayTagStackContainer& Tags, int SlotIndex /*= -1*/)
{
    if (ItemDef == nullptr || Count <= 0)
    {
        return -1;
    }
    
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
    int FindEmptyIndex = FindEmpty();
    //循环，直到空位被填完
    while (FindEmptyIndex >= 0 && Count > 0)
    {
        FContainerSlot Slot;
        Slot.ItemID = ItemDef;
        Slot.StackCount = (Count >= ItemDef->MaxStackAmount) ? ItemDef->MaxStackAmount : Count;
        Count -= (Count >= ItemDef->MaxStackAmount) ? ItemDef->MaxStackAmount : Count;

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

void FContainerList::RemoveItem(int Count, int SlotIndex)
{
    if (Slots[SlotIndex].StackCount >= 1)
    {
        Slots[SlotIndex].StackCount -= Count;
    }
    if (Slots[SlotIndex].StackCount <= 0)
    {
        FContainerSlot EmptySlot;
        Slots[SlotIndex] = EmptySlot;
    }
    MarkItemDirty(Slots[SlotIndex]);
}

void FContainerList::DragDropItem(int DragIndex, int DropIndex)
{
    //check valid ptr
    if (DragIndex == DropIndex)
    {
        return;
    }
	
    if (!Slots.IsValidIndex(DragIndex) || !Slots.IsValidIndex(DropIndex))
    {
        return;
    }

    //Stack
    if (Slots[DragIndex].ItemID != nullptr
        && Slots[DropIndex].ItemID != nullptr
        && Slots[DragIndex].ItemID == Slots[DropIndex].ItemID)
    {
        //Stack
        const int maxStackAmount = Slots[DragIndex].ItemID->MaxStackAmount;
        const int finalAmount = Slots[DropIndex].StackCount + Slots[DragIndex].StackCount;
        const int calculateAmount = finalAmount - maxStackAmount;
        if (Slots[DropIndex].StackCount != maxStackAmount)
        {
            if (calculateAmount > 0)
            {
                Slots[DropIndex].StackCount = maxStackAmount;
                Slots[DragIndex].StackCount = calculateAmount;
            }
            else
            {
                Slots[DropIndex].StackCount = finalAmount;
                RemoveItem(DragIndex, Slots[DragIndex].StackCount);
            }
            //Rep net
            MarkItemDirty(Slots[DropIndex]);
            MarkItemDirty(Slots[DragIndex]);

            return;
        }
    }
	
    //Switch
    const FContainerSlot Slot = Slots[DropIndex];
    Slots[DropIndex] = Slots[DragIndex];
    Slots[DragIndex] = Slot;
	
    //Rep net
    MarkItemDirty(Slots[DropIndex]);
    MarkItemDirty(Slots[DragIndex]);
}
