// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagerComponent.h"

#include "InventoryFragment_Equipment.h"
#include "InventoryFragment_SkeletalMesh.h"
#include "InventoryFragment_StaticMesh.h"
#include "InventoryFragment_Stats.h"
#include "InventoryItemDefinition.h"
#include "InventoryItemInstance_Equipment.h"
#include "InventoryItemInstance_StatTags.h"
#include "ItemActor_Common.h"
#include "K2Node_SpawnActor.h"
#include "Components/SphereComponent.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"


FString FInventorySlot::GetDebugString() const
{
	TSubclassOf<UInventoryItemDefinition> ItemDef;
	if (Instance != nullptr)
	{
		ItemDef = Instance->GetItemDef();
	}

	return FString::Printf(TEXT("%s (%d x %s)"), *GetNameSafe(Instance), StackCount, *GetNameSafe(ItemDef));
}

TArray<UInventoryItemInstance*> FInventoryList::GetAllItems() const
{
	TArray<UInventoryItemInstance*> Results;
	Results.Reserve(Slots.Num());
	for (const FInventorySlot& Slot : Slots)
	{
		if (Slot.Instance != nullptr) //@TODO: Would prefer to not deal with this here and hide it further?
		{
			Results.Add(Slot.Instance);
		}
	}
	return Results;
}

void FInventoryList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
}

void FInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
}

void FInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
}

void FInventoryList::GiveEmptySlots(int EmptySlotsAmount)
{
	Slots.SetNum(EmptySlotsAmount);

	MarkArrayDirty();
}

int FInventoryList::FindEmpty() const
{
	const int index = Slots.IndexOfByPredicate([](const FInventorySlot& InItem)
	{
		return InItem.Instance == nullptr;
	});
	return index;
}

void FInventoryList::FindStack(const TSubclassOf<UInventoryItemDefinition> ItemDef, int& index, int& remainAmount)
{
	index = -1;
	remainAmount = -1;
	if (ItemDef != nullptr)
	{
		int id = 0;
		bool found = false;
		for (FInventorySlot Slot : Slots)
		{
			if (Slot.Instance != nullptr && found == false)
			{
				// check conditions, used to determine which slot can be stacked.
				if (Slot.Instance->GetItemDef() == ItemDef &&
					Slot.StackCount < ItemDef.GetDefaultObject()->MaxStackAmount)
				{
					found = true;
					index = id;
					remainAmount = ItemDef.GetDefaultObject()->MaxStackAmount - Slot.StackCount;
				}
			}
			id++;
		}
	}
	else
	{
		// no stack info;
		return;
	}
}

int FInventoryList::AddItem(TSubclassOf<UInventoryItemDefinition> ItemDef, int Count, const TArray<FGameplayTagStack> TagStackOverride, int SlotIndex)
{
	if (ItemDef != nullptr)
	{
		// First find stack.
		int FindStackRemainAmount;
		int FindStackIndex;
		FindStack(ItemDef, FindStackIndex, FindStackRemainAmount);

		// Loop find stack.
		if (SlotIndex < 0)
		{
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
		}
		// Finish find stack.
		if (Count <= 0)
		{
			return 0;
		}
		// Begin find empty.
		else
		{
			// Const max stack amount.
			const int maxStack = ItemDef.GetDefaultObject()->MaxStackAmount;
			// First find empty.
			int FindEmptyIndex = FindEmpty();
			
			// Loop find empty.
			while (FindEmptyIndex >= 0 && Count > 0)
			{
				FInventorySlot Slot = AddNewInstance(ItemDef, (Count >= maxStack) ? maxStack : Count);

				Slots[SlotIndex >= 0 ? SlotIndex : FindEmptyIndex] = Slot;
				MarkItemDirty(Slot);
				Count -= (Count >= maxStack) ? maxStack : Count;

				//如果有TagStack就设置，没有就原Def
				if (TagStackOverride.Num() > 0)
				{
					if (const auto instance = Cast<UInventoryItemInstance_StatTags>(Slot.Instance))
					{
						instance->SetStatTagStacks(TagStackOverride);
					}
				}

				FindEmptyIndex = FindEmpty();
			}
			return Count;
		}
	}
	//没找到最大值就返回-1
	else
	{
		return -1;
	}
}

FInventorySlot FInventoryList::AddNewInstance(TSubclassOf<UInventoryItemDefinition> ItemDef, int StackAmount) const
{
	FInventorySlot Slot;
	Slot.StackCount = StackAmount;

	//check instance bp type is valid
	TSubclassOf<UInventoryItemInstance> InstanceType = ItemDef.GetDefaultObject()->Instance_BP;
	if (InstanceType != nullptr)
	{
		Slot.Instance = NewObject<UInventoryItemInstance>(OwnerComponent->GetOwner(), InstanceType);
	}
	else
	{
		InstanceType = UInventoryItemInstance::StaticClass();
		Slot.Instance = NewObject<UInventoryItemInstance>(OwnerComponent->GetOwner(), InstanceType);
	}
	
	// Trigger instance created event, can override by child class.
	Slot.Instance->SetItemDef(ItemDef);
	
	for (const UInventoryItemFragment* Fragment : GetDefault<UInventoryItemDefinition>(ItemDef)->Fragments)
	{
		if (Fragment != nullptr)
		{
			Fragment->OnInstanceCreated(Slot.Instance);
		}
	}
	
	Slot.Instance->OnInstanceCreated();
	
	return Slot;
}

void FInventoryList::RemoveItem(const int index, const int amount)
{
	if (Slots.Num() > index)
	{
		const int count = FMath::Max(Slots[index].StackCount - amount, 0);
		if (count > 0)
		{
			Slots[index].StackCount = count;
		}
		else
		{
			if (bIsACopyList == false)
			{
				if (UInventoryManagerComponent* IMC = Cast<UInventoryManagerComponent>(OwnerComponent))
				{
					if (index == IMC->SelectedQuickBarIndex)
					{
						IMC->UnequipInstance();
					}
				}
				if (Slots[index].Instance)
				{
					Slots[index].Instance->OnInstanceDestroyed();
					Slots[index].Instance->ConditionalBeginDestroy();
				}
			}
			const FInventorySlot EmptySlot;
			Slots[index] = EmptySlot;
		}
		MarkItemDirty(Slots[index]);
	}
}

bool FInventoryList::ItemDefUsed(TSubclassOf<UInventoryItemDefinition> ItemDef, int Amount)
{
	if (ItemDef != nullptr && Amount > 0 && GetTotalItemAmount(ItemDef) >= Amount)
	{
		int id = 0;
		for (const auto slot : Slots)
		{
			if (slot.Instance)
			{
				if (slot.Instance->GetItemDef() == ItemDef && Amount != 0)
				{
					if (Amount > slot.StackCount)
					{
						RemoveItem(id, slot.StackCount);
						Amount -= slot.StackCount;
					}
					else
					{
						RemoveItem(id,Amount);
						return true;
					}
				}
			}
			id++;
		}
	}
	return false;
}

void FInventoryList::DragDropItem(int DragIndex, int DropIndex)
{
	//check valid ptr
	check(Slots.IsValidIndex(DragIndex) && Slots.IsValidIndex(DropIndex))
	
	if (Slots[DragIndex].Instance != nullptr
		&& Slots[DropIndex].Instance != nullptr
		&& Slots[DragIndex].Instance->GetItemDef() == Slots[DropIndex].Instance->GetItemDef())
	{
		//Stack
		const int maxStackAmount = Slots[DragIndex].Instance->GetItemDef().GetDefaultObject()->MaxStackAmount;
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
	const FInventorySlot Slot = Slots[DropIndex];
	Slots[DropIndex] = Slots[DragIndex];
	Slots[DragIndex] = Slot;
	
	//Rep net
	MarkItemDirty(Slots[DropIndex]);
	MarkItemDirty(Slots[DragIndex]);
}

int FInventoryList::GetTotalItemAmount(TSubclassOf<UInventoryItemDefinition> ItemDef)
{
	if (ItemDef != nullptr)
	{
		int LocalTotalAmount = 0;
		for (FInventorySlot Slot : Slots)
		{
			if (Slot.Instance != nullptr)
			{
				if (ItemDef == Slot.Instance->GetItemDef())
				{
					LocalTotalAmount += Slot.StackCount;
				}
			}
		}
		return LocalTotalAmount;
	}
	else return -1;
}

// Sets default values for this component's properties
UInventoryManagerComponent::UInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UInventoryManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InventoryList);
	DOREPLIFETIME(ThisClass, SelectedQuickBarIndex);
	DOREPLIFETIME(ThisClass, KnownRecipes);
}

void UInventoryManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (this)
	{
		InventoryList.OwnerComponent = this;
	}
	InventoryList.GiveEmptySlots(InventorySlotAmount);

	//加入检测球体碰撞
	SphereComp = Cast<USphereComponent>(GetOwner()->AddComponentByClass(USphereComponent::StaticClass(), true, GetOwner()->GetTransform(), false));
	SphereComp->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
	SphereComp->bHiddenInGame = !bDebugDraw;
	SphereComp->SetSphereRadius(CanPickUpItemRadius);
	SphereComp->OnComponentBeginOverlap.AddDynamic(this, &UInventoryManagerComponent::OnOverlapBegin);
	SphereComp->OnComponentEndOverlap.AddDynamic(this, &UInventoryManagerComponent::OnOverlapEnd);
}

void UInventoryManagerComponent::OnRep_SelectedQuickBarIndex()
{
	UnequipInstance();
	EquipInstance(SelectedQuickBarIndex);
}

void UInventoryManagerComponent::OnRep_List()
{
	if (InventoryList.Slots.IsValidIndex(SelectedQuickBarIndex))
	{
		if (EquippedInstance != InventoryList.Slots[SelectedQuickBarIndex].Instance)
		{
			OnRep_SelectedQuickBarIndex();
		}
	}
}

int UInventoryManagerComponent::AddItem(TArray<FGameplayTagStack> TagStackOverride,TSubclassOf<UInventoryItemDefinition> ItemDef, int Count, int SlotIndex)
{
	if (ItemDef != nullptr)
	{
		const TArray<FGameplayTagStack> EmptyTagStack;
		const int Return = InventoryList.AddItem(ItemDef, Count, (TagStackOverride.Num() >= 0) ? TagStackOverride : EmptyTagStack, SlotIndex);
		OnRep_List();
		return Return;
	}
	return -1;
}

bool UInventoryManagerComponent::ItemDefUsed(TSubclassOf<UInventoryItemDefinition> ItemDef, int Amount)
{
	return InventoryList.ItemDefUsed(ItemDef, Amount);
}

bool UInventoryManagerComponent::CheckRecipeNeedItems(TSubclassOf<UInventoryItemRecipe> Recipe)
{
	if (IsValid(Recipe))
	{
		const auto needItems = Recipe.GetDefaultObject()->NeedItems;
		for (const TPair<TSubclassOf<UInventoryItemDefinition>, int>& pair : needItems)
		{
			const bool bMoreThan = ItemTotalAmount(pair.Key) >= pair.Value;
			if (!bMoreThan)
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

void UInventoryManagerComponent::CraftItem_Implementation(TSubclassOf<UInventoryItemRecipe> Recipe)
{
	const TMap<TSubclassOf<UInventoryItemDefinition>, int> Need = Recipe.GetDefaultObject()->NeedItems;
    const TMap<TSubclassOf<UInventoryItemDefinition>, int> Out = Recipe.GetDefaultObject()->OutItems;
    if (CheckRecipeNeedItems(Recipe)
    	&& CheckInventoryExchange(Need, Out))
    {
    	for (auto pair : Need.Array())
    	{
    		ItemDefUsed(pair.Key, pair.Value);
    	}
    	for (auto pair : Out.Array())
    	{
    		const TArray<FGameplayTagStack> TagStackOverride;
    		InventoryList.AddItem(pair.Key, pair.Value, TagStackOverride);
    	}
    }
}

int UInventoryManagerComponent::ItemTotalAmount(TSubclassOf<UInventoryItemDefinition> ItemDef)
{
	return InventoryList.GetTotalItemAmount(ItemDef);
}

int UInventoryManagerComponent::FindEmpty()
{
	return InventoryList.FindEmpty();
}

bool UInventoryManagerComponent::CheckInventoryExchange(TMap<TSubclassOf<UInventoryItemDefinition>, int> OutItems,
	TMap<TSubclassOf<UInventoryItemDefinition>, int> InItems)
{
	FInventoryList List = InventoryList;
	List.bIsACopyList = true;

	for (const TPair<TSubclassOf<UInventoryItemDefinition>, int>& pair : OutItems)
	{
		if (!List.ItemDefUsed(pair.Key, pair.Value))
		{
			return false;
		}
	}

	for (const TPair<TSubclassOf<UInventoryItemDefinition>, int>& pair : InItems)
	{
		const TArray<FGameplayTagStack> TagStackOverride;
		if (List.AddItem(pair.Key, pair.Value, TagStackOverride) != 0)
		{
			return false;
		}
	}
	
	return true;
}

void UInventoryManagerComponent::SplitItem_Implementation(int index, int amount)
{
	check(InventoryList.Slots.IsValidIndex(index))

	const int emptyIndex = InventoryList.FindEmpty();
	if (emptyIndex >= 0)
	{
		TArray<FGameplayTagStack> stackTags;
		if (Cast<UInventoryItemInstance_StatTags>(InventoryList.Slots[index].Instance))
		{
			stackTags = Cast<UInventoryItemInstance_StatTags>(InventoryList.Slots[index].Instance)->GetStatTags();
		}
		InventoryList.AddItem(InventoryList.Slots[index].Instance->GetItemDef(), amount, stackTags, emptyIndex);
		InventoryList.RemoveItem(index, amount);
	}
}

bool UInventoryManagerComponent::DropItemCheck(const TObjectPtr<UInventoryItemInstance> instancePtr, FVector& DropLocation) const
{
	check(instancePtr)
	float checkLength = 50;
	if (const auto ptr1 = instancePtr->FindFragmentByClass<UInventoryFragment_StaticMesh>())
	{
		checkLength = ptr1->PickupStaticMesh->GetBounds().SphereRadius * 2;
	}
	else if (const auto ptr2 = instancePtr->FindFragmentByClass<UInventoryFragment_SkeletalMesh>())
	{
		checkLength = ptr2->PickupSkeletalMesh->GetBounds().SphereRadius * 2;
	}
	
	const FVector ActorLocation = GetOwner()->GetActorLocation();
	DropLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * checkLength;
	DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), DropLocation, FColor::Red, false, 3);
	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, ActorLocation, DropLocation, ECC_Visibility);
	return !HitResult.IsValidBlockingHit();
}

void UInventoryManagerComponent::DropItem_Implementation(int index, int amount)
{
	if (InventoryList.Slots.IsValidIndex(index))
	{
		FVector DropLocation;
		if (DropItemCheck(InventoryList.Slots[index].Instance,DropLocation))
		{
			const auto instance = InventoryList.Slots[index].Instance;
			FGameplayTagStackContainer tagContainer;
			if (const auto statTagsInstance = Cast<UInventoryItemInstance_StatTags>(instance))
			{
				tagContainer = statTagsInstance->GetStatTagsContainer();
			}
			CreateItemActorInFront(instance->GetItemDef(), amount, tagContainer, DropLocation);
			RemoveItem(index, amount);
		}
		else
		{
			K2_UnableToDropItem();
		}
	}
}

void UInventoryManagerComponent::PickUpItem_Implementation(AItemActor_Base* ItemActor)
{
	if (ItemActor != nullptr)
	{
		const int RemainItemAmount = InventoryList.AddItem(ItemActor->ItemID, ItemActor->Amount, ItemActor->OverrideTagStack);
		if (RemainItemAmount == 0)
		{
			ItemActor->Destroy(true);
		}
		else if (RemainItemAmount > 0)
		{
			ItemActor->Amount = RemainItemAmount;
		}
		OnRep_List();
	}
}

void UInventoryManagerComponent::RemoveItem_Implementation(int index, int amount)
{
	InventoryList.RemoveItem(index, amount);
}

void UInventoryManagerComponent::CreateItemActorInFront_Implementation(TSubclassOf<UInventoryItemDefinition> ItemDef,
	int count, FGameplayTagStackContainer TagStackContainer, FVector DropLocation)
{
	FActorSpawnParameters spawnInfo;
	spawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FTransform actorTransform = FTransform(GetOwner()->GetActorForwardVector().ToOrientationRotator(), DropLocation);
	if (const auto actor = GetWorld()->SpawnActorDeferred<AItemActor_Common>(AItemActor_Common::StaticClass(), actorTransform))
	{
		actor->ItemID = ItemDef;
		actor->Amount = count;
		actor->OverrideTagStack = TagStackContainer.GetTagStacks();
		actor->FinishSpawning(actorTransform);
	}
}

void UInventoryManagerComponent::PickupSelectedIdChange(bool bUpOrDown)
{
	if (OverlapedActorsPtrs.Num() > 0)
	{
		PickupSelectedID = FMath::Clamp(PickupSelectedID + (bUpOrDown ? 1 : -1), 0, OverlapedActorsPtrs.Num()-1);
	}
	else
	{
		PickupSelectedID = -1;
	}
}

void UInventoryManagerComponent::DragDropItem_Implementation(int DragIndex, int DropIndex)
{
	InventoryList.DragDropItem(DragIndex, DropIndex);

	OnRep_List();
}

void UInventoryManagerComponent::ChangeQuickBarIndex_Server_Implementation(int index)
{
	if (SelectedQuickBarIndex != index)
	{
		SelectedQuickBarIndex = index;
		OnRep_SelectedQuickBarIndex();
	}
}

void UInventoryManagerComponent::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (const auto ItemActor = Cast<AItemActor_Base>(OtherActor))
	{
		if (OverlapedActorsPtrs.Num() == 0)
		{
			OverlapedActorsPtrs.Add(ItemActor);
			PickupSelectedID = 0;
		}
		else
		{
			const auto cacheActorPtr = OverlapedActorsPtrs[PickupSelectedID];
			OverlapedActorsPtrs.Add(ItemActor);
			PickupSelectedID = OverlapedActorsPtrs.Find(cacheActorPtr);
		}
		OnItemBeginOverlap(ItemActor);
	}
}

void UInventoryManagerComponent::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (const auto ItemActor = Cast<AItemActor_Base>(OtherActor))
	{
		if (PickupSelectedID == OverlapedActorsPtrs.Num()-1)
		{
			PickupSelectedID--;
		}
		OverlapedActorsPtrs.Remove(ItemActor);
		OnItemEndOverlap(ItemActor);
	}
}

bool UInventoryManagerComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (FInventorySlot& Slot : InventoryList.Slots)
	{
		UInventoryItemInstance* Instance = Slot.Instance;

		if (Instance && IsValid(Instance))
		{
			WroteSomething |= Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags);
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
			UInventoryItemInstance* Instance = Slot.Instance;

			if (IsValid(Instance))
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
}

void UInventoryManagerComponent::UnequipInstance()
{
	if (EquippedInstance != nullptr)
	{
		if (const auto instance = Cast<UInventoryItemInstance_Equipment>(EquippedInstance))
		{
			instance->OnUnequipped();
			instance->SetInstigator(nullptr);
		}
	}
	EquippedInstance = nullptr;
}

void UInventoryManagerComponent::EquipInstance(int SlotIndex)
{
	if (InventoryList.Slots.IsValidIndex(SlotIndex))
	{
		EquippedInstance = InventoryList.Slots[SlotIndex].Instance;
		if (const auto instance = Cast<UInventoryItemInstance_Equipment>(EquippedInstance))
		{
			instance->OnEquipped();
			instance->SetInstigator(GetOwner());
		}
	}
}
