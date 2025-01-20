// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagerComponent.h"

#include "BlueprintTypePromotion.h"
#include "Inventory.h"
#include "Fragments/InventoryFragment_SkeletalMesh.h"
#include "Logging/LogMacros.h"
#include "Fragments/InventoryFragment_StaticMesh.h"
#include "InventoryItemDefinition.h"
#include "ItemInstances/InventoryItemInstance_Equipment.h"
#include "InventorySettings.h"
#include "Components/SphereComponent.h"
#include "Engine/ActorChannel.h"
#include "ItemActors/ItemActor_Common.h"
#include "Net/UnrealNetwork.h"

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
	if (auto InventoryManagerComp = Cast<UInventoryManagerComponent>(OwnerComponent))
	{
		InventoryManagerComp->K2_InventoryListChanged(Slot, Index, ChangeType);
	}
}

void FInventoryList::NotifyComponentListChanged(const TArrayView<int32>& Indices,
	const TEnumAsByte<EArrayChangeType> ChangeType)
{
	if (auto InventoryManagerComp = Cast<UInventoryManagerComponent>(OwnerComponent))
	{
		for (auto Index : Indices)
		{
			InventoryManagerComp->K2_InventoryListChanged(Slots[Index], Index, ChangeType);
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
		InstanceItr->OnInstanceDestroyed();
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

void NotifyCategoryChanged(const FInventorySlot& Slot)
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
			NotifyCategoryChanged(Slots[FindEmptyIndex]);
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
		auto IdArray = GetItemDefCategoryArray(ItemDef);
		if (!IdArray.Find(Slots[Index].SlotCategoryTag))
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

void FInventoryList::DragDropItem(const int DragIndex, const int DropIndex)
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

	// Stack on drop slot
	if (Slots[DragIndex].ItemDefinition == Slots[DropIndex].ItemDefinition)
	{
		const int MaxStackAmount = Slots[DropIndex].ItemDefinition->GetMaxStackAmount(Slots[DropIndex].SlotCategoryTag);
		const int FinalAmount = Slots[DropIndex].GetItemStackCount() + Slots[DragIndex].GetItemStackCount();
		const int CalculateAmount = FinalAmount - MaxStackAmount;
		// If not max stack
		if (Slots[DropIndex].GetItemStackCount() != MaxStackAmount)
		{
			if (Slots[DropIndex].ItemDefinition->ItemInstanceType == IIT_Multiple)
			{
				StackInstances(Slots[DropIndex].ItemDefinition, Slots[DragIndex].StackedInstances, DropIndex,
							   CalculateAmount > 0
								   ? MaxStackAmount - Slots[DropIndex].GetItemStackCount()
								   : Slots[DragIndex].GetItemStackCount());
			}
			else
			{
				SetItemStackCountAt(DropIndex, CalculateAmount > 0 ? MaxStackAmount : FinalAmount);
				SetItemStackCountAt(DragIndex, CalculateAmount > 0 ? CalculateAmount : 0);
			}
			
			MarkItemDirty(Slots[DropIndex]);
			MarkItemDirty(Slots[DragIndex]);
			NotifyComponentListChanged(Slots[DropIndex], DropIndex, ChangeType_Changed);
			NotifyComponentListChanged(Slots[DragIndex], DragIndex, ChangeType_Changed);
			return;
		}
	}
	else
	{
		auto MaxStack = Slots[DragIndex].ItemDefinition->GetMaxStackAmount(Slots[DropIndex].SlotCategoryTag);
		// Drag to empty with little part start
		if (MaxStack < Slots[DragIndex].GetItemStackCount())
		{
			Slots[DropIndex].ItemDefinition = Slots[DragIndex].ItemDefinition;
			Slots[DragIndex].StackAmount -= MaxStack;
			switch (Slots[DragIndex].ItemDefinition->ItemInstanceType)
			{
			case IIT_None:
				Slots[DropIndex].StackAmount = MaxStack;
				break;
			case IIT_OnlyOne:
				SetItemStackCountAt(DropIndex, MaxStack);
				break;
			case IIT_Multiple:
				StackInstances(Slots[DragIndex].ItemDefinition, Slots[DragIndex].StackedInstances, DropIndex, MaxStack);
				break;
			default: ;
			}
			MarkItemDirty(Slots[DropIndex]);
			MarkItemDirty(Slots[DragIndex]);
			NotifyComponentListChanged(Slots[DropIndex], DropIndex, ChangeType_Changed);
			NotifyComponentListChanged(Slots[DragIndex], DragIndex, ChangeType_Changed);
			NotifyCategoryChanged(Slots[DropIndex]);
			NotifyCategoryChanged(Slots[DragIndex]);
			return;
			//UE_LOG(LogInventory, Warning, TEXT("Can't fit this slot, can only drop part of items to this slot"))
		}
		else if (Slots[DropIndex].ItemDefinition == nullptr ||
				Slots[DropIndex].ItemDefinition->GetMaxStackAmount(Slots[DragIndex].SlotCategoryTag) >= Slots[DropIndex].GetItemStackCount())
		{
			{
				// Switch start
				Slots[DropIndex].SwitchSlot(Slots[DragIndex]);
				//Rep net
				MarkItemDirty(Slots[DropIndex]);
				MarkItemDirty(Slots[DragIndex]);
				NotifyComponentListChanged(Slots[DropIndex], DropIndex, ChangeType_Changed);
				NotifyComponentListChanged(Slots[DragIndex], DragIndex, ChangeType_Changed);
				NotifyCategoryChanged(Slots[DropIndex]);
				NotifyCategoryChanged(Slots[DragIndex]);
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

// Sets default values for this component's properties
UInventoryManagerComponent::UInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAllowAnyoneToDestroyMe = true;
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	if (auto InventorySetting = GetMutableDefault<UInventorySettings>())
	{
		InventorySlotAmount = {TPair<FGameplayTag, int>(InventorySetting->DefaultCategoryTag, 10)};
	}
}

void UInventoryManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InventoryList);
	DOREPLIFETIME(ThisClass, KnownRecipes);
	DOREPLIFETIME(ThisClass, bForceUnequipped);
}

void UInventoryManagerComponent::BeginPlay()
{
	if (this)
	{
		InventoryList.OwnerComponent = this;
	}

	if (GetOwner()->HasAuthority())
	{
		InventoryList.AddEmptySlots(InventorySlotAmount);
	}
	
	if (bUseSphereDetection)
	{
		// All sphere collision to detect item actors.
		SphereComp = Cast<USphereComponent>(GetOwner()->AddComponentByClass(USphereComponent::StaticClass(), true, GetOwner()->GetTransform(), false));
		SphereComp->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
		SphereComp->bHiddenInGame = !bDebugDraw;
		SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		SphereComp->SetCollisionObjectType(SphereCollisionObjectType);
		SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
		for (const auto Response : SphereCollisionResponses)
		{
			SphereComp->SetCollisionResponseToChannel(Response.Key, Response.Value);
		}
		SphereComp->SetSphereRadius(CanPickUpItemRadius);
		SphereComp->OnComponentEndOverlap.AddDynamic(this, &UInventoryManagerComponent::OnOverlapEnd);
	}

	Super::BeginPlay();
}

void UInventoryManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TArray<AActor*> NotValidatedItemPtrs;
	if (SphereComp)
	{
		SphereComp->GetOverlappingActors(NotValidatedItemPtrs, AItemActor_Base::StaticClass());
	}
	const auto CachedOverlappedActors = OverlappedActorsPtrs;
	OverlappedActorsPtrs.Empty();

	if (NotValidatedItemPtrs.Num() == 0)
	{
		PickupSelectedID = -1;
	}
	
	for (const auto Actor : NotValidatedItemPtrs)
	{
		//Cast
		auto NotValidItem = Cast<AItemActor_Base>(Actor);
		if (!NotValidItem)
		{
			continue;
		}
		
		//LineTrace
		FCollisionQueryParams CollisionParameters;
		CollisionParameters.AddIgnoredActor(GetOwner());
		
		FHitResult HitResult;
		FVector EndLocation = NotValidItem->GetActorLocation();

		//Not hit then add.
		if (const bool bHit = GetWorld()->LineTraceSingleByChannel(
			HitResult,
			GetOwner()->GetActorLocation(),
			EndLocation,
			ECC_Visibility,
			CollisionParameters
		); !bHit || HitResult.GetActor() == NotValidItem)
		{
			OverlappedActorsPtrs.Add(NotValidItem);
			if (CachedOverlappedActors.Find(NotValidItem) == INDEX_NONE)
			{
				if (CachedOverlappedActors.Num() == 0)
				{
					if (bInteractValid)
					{
						PickupSelectedID = -1;
					}
					else
					{
						PickupSelectedID = 0;
					}
				}
				else if (CachedOverlappedActors.IsValidIndex(PickupSelectedID))
				{
					const auto cacheActorPtr = CachedOverlappedActors[PickupSelectedID];
					PickupSelectedID = CachedOverlappedActors.Find(cacheActorPtr);
				}
				OnItemBeginOverlap(NotValidItem);
			}
		}
		else
		{
			if (CachedOverlappedActors.Find(NotValidItem) != INDEX_NONE)
			{
				if (PickupSelectedID == CachedOverlappedActors.Num()-1)
				{
					PickupSelectedID--;
				}
				OnItemEndOverlap(NotValidItem);
			}
		}
	}
}

void UInventoryManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

void UInventoryManagerComponent::AddEmptySlots(const FGameplayTag& SlotCategoryTag, int Count)
{
	InventoryList.AddEmptySlots(Count, SlotCategoryTag);
}

int UInventoryManagerComponent::AddItem(UInventoryItemDefinition* ItemDef, const int Count)
{
	if (ItemDef != nullptr && Count > 0)
	{
		const int Return = InventoryList.AddItem(ItemDef, Count);
		return Return;
	}
	return -1;
}

bool UInventoryManagerComponent::ItemDefUsed(const UInventoryItemDefinition* ItemDef, const int Amount)
{
	return InventoryList.ItemDefUsed(ItemDef, Amount);
}

int UInventoryManagerComponent::RecipeCraftTimes(UInventoryItemRecipe* Recipe)
{
	if (Recipe)
	{
		int MinTime = INT_MAX;
		for (const auto NeedItems = Recipe->NeedItems;
			const TPair<UInventoryItemDefinition*, int>& Pair : NeedItems)
		{
			MinTime = FMath::Min(ItemTotalAmount(Pair.Key) / Pair.Value, MinTime);
		}
		return MinTime;
	}
	return -1;
}

bool UInventoryManagerComponent::CheckRecipeNeedItems(const UInventoryItemRecipe* Recipe)
{
	if (IsValid(Recipe))
	{
		for (const auto NeedItems = Recipe->NeedItems; const TPair<
			     UInventoryItemDefinition*, int>& Pair : NeedItems)
		{
			if (ItemTotalAmount(Pair.Key) < Pair.Value)
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

void UInventoryManagerComponent::CraftItem_Implementation(const UInventoryItemRecipe* Recipe, const int Times)
{
	const TMap<UInventoryItemDefinition*, int> Need = Recipe->NeedItems;
    const TMap<UInventoryItemDefinition*, int> Out = Recipe->OutItems;
    if (CheckRecipeNeedItems(Recipe)
    	&& CheckInventoryExchange(Need, Out, Times, Times))
    {
    	for (const auto Pair : Need.Array())
    	{
    		ItemDefUsed(Pair.Key, Pair.Value * Times);
    	}
    	for (auto Pair : Out.Array())
    	{
    		InventoryList.AddItem(Pair.Key, Pair.Value * Times);
    	}
    }
}

int UInventoryManagerComponent::ItemTotalAmount(const UInventoryItemDefinition* ItemDef)
{
	return InventoryList.GetTotalItemAmount(ItemDef);
}

int UInventoryManagerComponent::FindEmpty(const TArray<FGameplayTag>& CategoryTags) const
{
	return InventoryList.FindEmpty(CategoryTags);
}

bool UInventoryManagerComponent::CheckInventoryExchange(TMap<UInventoryItemDefinition*, int> OutItems,
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

void UInventoryManagerComponent::SplitItem_Implementation(const int Index, const int Amount)
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
			InventoryList.SetItemAt(InventoryList.Slots[Index].ItemDefinition, Amount, EmptyIndex);
			InventoryList.SetItemStackCountAt(Index, InventoryList.Slots[Index].GetItemStackCount() - Amount);
		}
	}
}

bool UInventoryManagerComponent::DropItemCheck(const UInventoryItemDefinition* ItemDef, FVector& DropLocation) const
{
	check(ItemDef)
	float CheckLength = 0.0f;
	switch (ItemDef->MeshType)
	{
	case MT_None: return false;
		break;
	case MT_StaticMesh:
		if (const auto Ptr1 = Cast<UInventoryFragment_StaticMesh>(ItemDef->MeshSettings))
		{
			CheckLength = Ptr1->PickupStaticMesh->GetBounds().SphereRadius * 2;
		}
		break;
	case MT_SkeletalMesh:
		if (const auto Ptr2 = Cast<UInventoryFragment_SkeletalMesh>(ItemDef->MeshSettings))
		{
			CheckLength = Ptr2->PickupSkeletalMesh->GetBounds().SphereRadius * 2;
		}
		break;
	default: return false;
	}
	
	const FVector ActorLocation = GetOwner()->GetActorLocation();
	DropLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * CheckLength;
	DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), DropLocation, FColor::Red, false, 3);
	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, ActorLocation, DropLocation, ECC_Visibility);
	return !HitResult.IsValidBlockingHit();
}

void UInventoryManagerComponent::DropItem_Implementation(const int Index, const int Amount)
{
	if (InventoryList.Slots.IsValidIndex(Index))
	{
		if (FVector DropLocation; DropItemCheck(InventoryList.Slots[Index].ItemDefinition,DropLocation))
		{
			CreateItemActorInFront(InventoryList.Slots[Index], DropLocation);
			RemoveItem(Index, Amount);
		}
		else
		{
			K2_UnableToDropItem();
		}
	}
}

void UInventoryManagerComponent::PickUpItem_Implementation(AItemActor_Base* ItemActor)
{
	// Validate
	if (ItemActor == nullptr)
	{
		return;
	}
	if (ItemActor->ItemID == nullptr || ItemActor->Amount <= 0)
	{
		UE_LOG(LogInventory, Error, TEXT("It should not happen! Please check the item actor you picked up! The item actor is %s"), *ItemActor->GetName())
		return;
	}
	
	// Add items
	bool bChanged = false;
	if (ItemActor->ItemID->ItemInstanceType == IIT_Multiple && !ItemActor->bUseDefaultInstance)
	{
		int RemainItemAmount = InventoryList.AddItem(ItemActor->ItemID, ItemActor->ItemInstances);
		ItemActor->ItemInstances.RemoveAt(0, ItemActor->ItemInstances.Num() - RemainItemAmount);
		if (ItemActor->Amount != ItemActor->ItemInstances.Num())
		{
			ItemActor->Amount = ItemActor->ItemInstances.Num();
			bChanged = true;
		}
	}
	else
	{
		int RemainItemAmount = InventoryList.AddItem(ItemActor->ItemID, ItemActor->Amount);
		if (ItemActor->Amount != RemainItemAmount)
		{
			ItemActor->Amount = RemainItemAmount;
			bChanged = true;
		}
	}
	// If changed then update something.
	if (bChanged)
	{
		ItemActor->NativeOnItemPickedUp();
	}
}

void UInventoryManagerComponent::RemoveItem_Implementation(const int Index, const int Amount)
{
	if (InventoryList.Slots.IsValidIndex(Index))
	{
		InventoryList.SetItemStackCountAt(Index, InventoryList.Slots[Index].GetItemStackCount() - Amount);
	}
}

void UInventoryManagerComponent::CreateItemActorInFront_Implementation(const FInventorySlot SlotToDrop, const FVector DropLocation)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FTransform ActorTransform = FTransform(GetOwner()->GetActorForwardVector().ToOrientationRotator(), DropLocation);

	//use project settings cpp class or bp class
	UClass* Class = AItemActor_Common::StaticClass();
	if (const UInventorySettings* Settings = GetMutableDefault<UInventorySettings>())
	{
		if (Settings->GetDynamicItemActorClass())
		{
			Class = Settings->GetDynamicItemActorClass();
		}
	}
	
	//Spawn dynamic actor;
	if (AItemActor_Common* NewActor = GetWorld()->SpawnActorDeferred<AItemActor_Common>(Class, ActorTransform))
	{
		NewActor->SetupActor(SlotToDrop);
		NewActor->FinishSpawning(ActorTransform);
	}
}

void UInventoryManagerComponent::PickupSelectedIdChange(const bool bUpOrDown)
{
	const int MinIndex = bInteractValid ? -1 : 0;
	if (OverlappedActorsPtrs.Num() > 0)
	{
		PickupSelectedID = FMath::Clamp(PickupSelectedID + (bUpOrDown ? 1 : -1), MinIndex, OverlappedActorsPtrs.Num()-1);
	}
}

TArray<int32> UInventoryManagerComponent::GetSlotsByCategory(const FGameplayTag CategoryTag, const bool MatchAll) const
{
	return InventoryList.GetSlotsByCategory(CategoryTag, MatchAll);
}

void UInventoryManagerComponent::DragDropItem_Implementation(const int DragIndex, const int DropIndex)
{
	InventoryList.DragDropItem(DragIndex, DropIndex);
}

void UInventoryManagerComponent::ForceUnequipItem()
{
	if (bForceUnequipped == false)
	{
		if (EquippedInstance)
		{
			if (auto Instance = Cast<UInventoryItemInstance_Equipment>(EquippedInstance))
			{
				Instance->OnUnequipped();
			}
		}
		bForceUnequipped = true;
	}
}

void UInventoryManagerComponent::CancelForceUnequipItem()
{
	if (bForceUnequipped == true)
	{
		if (EquippedInstance)
		{
			if (auto Instance = Cast<UInventoryItemInstance_Equipment>(EquippedInstance))
			{
				Instance->OnEquipped();
			}
		}
		bForceUnequipped = false;
	}
}

void UInventoryManagerComponent::ChangeQuickBarIndex(const int& Index)
{
	SelectedQuickBarIndex = Index;
	if (!GetOwner()->HasAuthority())
	{
		ChangeQuickBarIndex_Server(Index);
	}
}

void UInventoryManagerComponent::ChangeQuickBarIndex_Server_Implementation(const int& Index)
{
	SelectedQuickBarIndex = Index;
}

void UInventoryManagerComponent::ChangeEquipmentItem(const UInventoryItemInstance* Instance)
{
	if (EquippedInstance == Instance)
	{
		return;
	}
	
	if (GetOwner()->HasAuthority())
	{
		ChangeEquipmentItemImplementation(Instance);
	}
	else
	{
		ChangeEquipmentItemImplementation(Instance);
		ChangeEquipmentItem_Server(Instance);
	}
}

void UInventoryManagerComponent::ChangeEquipmentItem_Server_Implementation(const UInventoryItemInstance* Instance)
{
	ChangeEquipmentItemImplementation(Instance);
}

void UInventoryManagerComponent::ChangeEquipmentItemImplementation(const UInventoryItemInstance* Instance)
{
	UnequipInstance();
	if (Instance)
	{
		EquipInstance(const_cast<UInventoryItemInstance*>(Instance));
	}
}

void UInventoryManagerComponent::ClearItems()
{
	for (int a = 0; a <= InventoryList.Slots.Num() - 1; a = a + 1)
	{
		InventoryList.EmptySlotAt(a);
	}
}

FInventorySaveData UInventoryManagerComponent::GetSaveData()
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
	InventorySaveData.SelectedQuickBarIndex = SelectedQuickBarIndex;
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

bool UInventoryManagerComponent::LoadSaveData(FInventorySaveData SaveData)
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
	
	SelectedQuickBarIndex = SaveData.SelectedQuickBarIndex;
	return true;
}

void UInventoryManagerComponent::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (const auto Item = Cast<AItemActor_Base>(OtherActor))
	{
		if (OverlappedActorsPtrs.Find(Item) != INDEX_NONE)
		{
			if (PickupSelectedID == OverlappedActorsPtrs.Num()-1)
			{
				PickupSelectedID--;
			}
			OnItemEndOverlap(Item);
		}
	}
}

bool UInventoryManagerComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
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

void UInventoryManagerComponent::ReadyForReplication()
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

void UInventoryManagerComponent::UnequipInstance()
{
	if (EquippedInstance != nullptr)
	{
		if (const auto Instance = Cast<UInventoryItemInstance_Equipment>(EquippedInstance))
		{
			Instance->OnUnequipped();
			Instance->SetInstigator(nullptr);
		}
		EquippedInstance = nullptr;
	}
}

void UInventoryManagerComponent::EquipInstance(UInventoryItemInstance* ItemInstance)
{
	if (ItemInstance)
	{
		if (const auto Instance = Cast<UInventoryItemInstance_Equipment>(ItemInstance))
		{
			Instance->OnEquipped();
			Instance->SetInstigator(GetOwner());
		}
	}
	EquippedInstance = ItemInstance;
}
