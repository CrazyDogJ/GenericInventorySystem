// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagerComponent.h"

#include "Fragments/InventoryFragment_SkeletalMesh.h"
#include "Fragments/InventoryFragment_StaticMesh.h"
#include "InventoryItemDefinition.h"
#include "InventoryContainerComponent.h"
#include "ItemInstances/InventoryItemInstance_Equipment.h"
#include "ItemInstances/InventoryItemInstance_StatTags.h"
#include "InventorySettings.h"
#include "Components/SphereComponent.h"
#include "Engine/ActorChannel.h"
#include "ItemActors/ItemActor_Common.h"
#include "Net/UnrealNetwork.h"


FString FInventorySlot::GetDebugString() const
{
	TObjectPtr<UInventoryItemDefinition> ItemDef = GetItemDef();

	return FString::Printf(TEXT("%s (%d x %s)"), *GetNameSafe(Instance), StackCount, *GetNameSafe(ItemDef));
}

FContainerSlot FInventorySlot::ToStruct() const
{
	if (!GetItemDef())
	{
		return FContainerSlot();
	}

	FGameplayTagStackContainer Container;
	if (auto StatInstance = Cast<UInventoryItemInstance_StatTags>(Instance))
	{
		Container = StatInstance->GetStatTagsContainer();
	}
	return FContainerSlot(GetItemDef(), StackCount, Container);
}

TObjectPtr<UInventoryItemDefinition> FInventorySlot::GetItemDef() const
{
	if (Instance)
	{
		return Instance->GetItemDef();
	}
	return ItemDefinition;
}

void FInventoryList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	if (!OwnerComponent)
	{
		return;
	}
	Cast<UInventoryManagerComponent>(OwnerComponent)->OnInventoryListChanged.Broadcast();
}

void FInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	if (!OwnerComponent)
	{
		return;
	}
	Cast<UInventoryManagerComponent>(OwnerComponent)->OnInventoryListChanged.Broadcast();
}

void FInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	if (!OwnerComponent)
	{
		return;
	}
	Cast<UInventoryManagerComponent>(OwnerComponent)->OnInventoryListChanged.Broadcast();
}

void FInventoryList::AddEmptySlots(const int EmptySlotsAmount)
{
	Slots.SetNum(Slots.Num() + EmptySlotsAmount);
	MarkArrayDirty();
	Cast<UInventoryManagerComponent>(OwnerComponent)->K2_InventoryListChanged();
}

int FInventoryList::FindEmpty() const
{
	const int index = Slots.IndexOfByPredicate([](const FInventorySlot& InItem)
	{
		return InItem.GetItemDef() == nullptr;
	});
	return index;
}

void FInventoryList::FindStack(const UInventoryItemDefinition* ItemDef, int& Index, int& RemainAmount)
{
	Index = -1;
	RemainAmount = -1;

	if (ItemDef == nullptr)
	{
		return;
	}

	Index = Slots.IndexOfByPredicate([&ItemDef](const FInventorySlot& InItem)
	{
		if (InItem.GetItemDef() == nullptr)
		{
			return false;
		}
		return InItem.GetItemDef() == ItemDef && InItem.StackCount < ItemDef->MaxStackAmount;
	});

	if (Index < 0)
	{
		return;
	}
	
	RemainAmount = ItemDef->MaxStackAmount - Slots[Index].StackCount;
}

int FInventoryList::AddItem(UInventoryItemDefinition* ItemDef, int Count, const TArray<FGameplayTagStack>& TagStackOverride)
{
	if (ItemDef == nullptr)
	{
		// Error.
		return -1;
	}

	auto OldCount = Count;
	
	// First find stack.
	int FindStackRemainAmount;
	int FindStackIndex;
	FindStack(ItemDef, FindStackIndex, FindStackRemainAmount);

	// Loop find stack.
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
	}
	
	// Finish find stack.
	if (Count <= 0)
	{
		Cast<UInventoryManagerComponent>(OwnerComponent)->K2_InventoryListChanged();
		return 0;
	}
	
	// Begin find empty.
	// Const max stack amount.
	const int maxStack = ItemDef->MaxStackAmount;
	// First find empty.
	int FindEmptyIndex = FindEmpty();
			
	// Loop find empty.
	while (FindEmptyIndex >= 0 && Count > 0)
	{
		FInventorySlot Slot = AddNewInstance(ItemDef, (Count >= maxStack) ? maxStack : Count);

		Slots[FindEmptyIndex] = Slot;
		MarkItemDirty(Slot);
		Count -= (Count >= maxStack) ? maxStack : Count;
		
		if (TagStackOverride.Num() > 0)
		{
			if (const auto instance = Cast<UInventoryItemInstance_StatTags>(Slot.Instance))
			{
				instance->SetStatTagStacks(TagStackOverride);
			}
		}

		FindEmptyIndex = FindEmpty();
	}
	
	if (Count != OldCount)
	{
		Cast<UInventoryManagerComponent>(OwnerComponent)->K2_InventoryListChanged();
	}
	return Count;
}

void FInventoryList::SetItemAt(UInventoryItemDefinition* ItemDef, const int Count,
	const TArray<FGameplayTagStack>& TagStackOverride, const int& Index)
{
	if (!Slots.IsValidIndex(Index))
	{
		return;
	}
	
	RemoveItemAt(Index, Slots[Index].StackCount);
	const FInventorySlot Slot = AddNewInstance(ItemDef, Count);
	if (TagStackOverride.Num() > 0)
	{
		if (const auto instance = Cast<UInventoryItemInstance_StatTags>(Slot.Instance))
		{
			instance->SetStatTagStacks(TagStackOverride);
		}
	}
	Slots[Index] = Slot;
	MarkItemDirty(Slots[Index]);
	Cast<UInventoryManagerComponent>(OwnerComponent)->K2_InventoryListChanged();
}

FInventorySlot FInventoryList::AddNewInstance(UInventoryItemDefinition* ItemDef, const int StackAmount) const
{
	FInventorySlot Slot;
	Slot.StackCount = StackAmount;

	//check instance bp type is valid
	if (auto InstanceInDef = DuplicateObject(ItemDef->ItemInstance, OwnerComponent->GetOwner()))
	{
		Slot.Instance = InstanceInDef;
		// Trigger instance created event, can override by child class.
		Slot.ItemDefinition = ItemDef;
		Slot.Instance->SetItemDef(ItemDef);
		
		for (const UInventoryItemFragment* Fragment : ItemDef->Fragments)
		{
			if (Fragment != nullptr)
			{
				Fragment->OnInstanceCreated(Slot.Instance);
			}
		}
		Slot.Instance->OnInstanceCreated();
	}
	else
	{
		Slot.ItemDefinition = ItemDef;
	}
	
	return Slot;
}

void FInventoryList::RemoveItemAt(const int Index, const int Amount)
{
	if (Slots.Num() <= Index || Amount == 0)
	{
		return;
	}

	if (const int Count = FMath::Max(Slots[Index].StackCount - Amount, 0); Count > 0)
	{
		Slots[Index].StackCount = Count;
	}
	else
	{
		if (OwnerComponent)
		{
			if (UInventoryManagerComponent* IMC = Cast<UInventoryManagerComponent>(OwnerComponent))
			{
				if (Index == IMC->SelectedQuickBarIndex)
				{
					IMC->UnequipInstance();
				}
			}
		}
		
		if (Slots[Index].Instance)
		{
			Slots[Index].Instance->OnInstanceDestroyed();
			Slots[Index].Instance->ConditionalBeginDestroy();
		}
		
		const FInventorySlot EmptySlot;
		Slots[Index] = EmptySlot;
	}
	MarkItemDirty(Slots[Index]);
	Cast<UInventoryManagerComponent>(OwnerComponent)->K2_InventoryListChanged();
}

bool FInventoryList::ItemDefUsed(const UInventoryItemDefinition* ItemDef, int Amount)
{
	if (ItemDef == nullptr || Amount <= 0 || GetTotalItemAmount(ItemDef) < Amount)
	{
		return false;
	}
	
	for (int ID = 0; ID < Slots.Num(); ID++)
	{
		auto Slot = Slots[ID];
		if (!Slot.GetItemDef())
		{
			continue;
		}
		
		if (Slot.GetItemDef() == ItemDef && Amount != 0)
		{
			if (Amount > Slot.StackCount)
			{
				RemoveItemAt(ID, Slot.StackCount);
				Amount -= Slot.StackCount;
			}
			else
			{
				RemoveItemAt(ID,Amount);
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
	
	if (Slots[DragIndex].GetItemDef() != nullptr
		&& Slots[DropIndex].GetItemDef() != nullptr
		&& Slots[DragIndex].GetItemDef() == Slots[DropIndex].GetItemDef())
	{
		//Stack
		const int maxStackAmount = Slots[DragIndex].GetItemDef()->MaxStackAmount;
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
				RemoveItemAt(DragIndex, Slots[DragIndex].StackCount);
			}
			//Rep net
			MarkItemDirty(Slots[DropIndex]);
			MarkItemDirty(Slots[DragIndex]);
			Cast<UInventoryManagerComponent>(OwnerComponent)->K2_InventoryListChanged();
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
	Cast<UInventoryManagerComponent>(OwnerComponent)->K2_InventoryListChanged();
}

int FInventoryList::GetTotalItemAmount(const UInventoryItemDefinition* ItemDef)
{
	if (ItemDef != nullptr)
	{
		int LocalTotalAmount = 0;
		for (FInventorySlot Slot : Slots)
		{
			if (ItemDef == Slot.GetItemDef())
			{
				LocalTotalAmount += Slot.StackCount;
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
}

void UInventoryManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InventoryList);
	DOREPLIFETIME(ThisClass, SelectedQuickBarIndex);
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
		InventoryList.Slots.Empty();
		InventoryList.AddEmptySlots(InventorySlotAmount);
	}
	
	if (bUseSphereDetection)
	{
		//加入检测球体碰撞
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

	if (EquippedInstance)
	{
		EquippedInstance->OnInstanceDestroyed();
		EquippedInstance->ConditionalBeginDestroy();
	}
	
	InventoryList.Slots.Empty();
	InventoryList.MarkArrayDirty();
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

int UInventoryManagerComponent::AddItem(const TArray<FGameplayTagStack> TagStackOverride, UInventoryItemDefinition* ItemDef, const int Count)
{
	if (ItemDef != nullptr)
	{
		const TArray<FGameplayTagStack> EmptyTagStack;
		const int Return = InventoryList.AddItem(ItemDef, Count, (TagStackOverride.Num() >= 0) ? TagStackOverride : EmptyTagStack);
		OnRep_List();
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
    		const TArray<FGameplayTagStack> TagStackOverride;
    		InventoryList.AddItem(Pair.Key, Pair.Value * Times, TagStackOverride);
    	}
    }
}

int UInventoryManagerComponent::ItemTotalAmount(const UInventoryItemDefinition* ItemDef)
{
	return InventoryList.GetTotalItemAmount(ItemDef);
}

int UInventoryManagerComponent::FindEmpty()
{
	return InventoryList.FindEmpty();
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
		const TArray<FGameplayTagStack> TagStackOverride;
		if (List.AddItem(Pair.Key, Pair.Value * InTimes, TagStackOverride) != 0)
		{
			return false;
		}
	}
	
	return true;
}

void UInventoryManagerComponent::SplitItem_Implementation(const int Index, const int Amount)
{
	check(InventoryList.Slots.IsValidIndex(Index))

	if (const int EmptyIndex = InventoryList.FindEmpty(); EmptyIndex >= 0)
	{
		TArray<FGameplayTagStack> StackTags;
		//if (Cast<UInventoryItemInstance_StatTags>(InventoryList.Slots[Index].Instance))
		//{
		//	StackTags = Cast<UInventoryItemInstance_StatTags>(InventoryList.Slots[Index].Instance)->GetStatTags();
		//}
		InventoryList.SetItemAt(InventoryList.Slots[Index].GetItemDef(), Amount, StackTags, EmptyIndex);
		InventoryList.RemoveItemAt(Index, Amount);
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
		if (FVector DropLocation; DropItemCheck(InventoryList.Slots[Index].GetItemDef(),DropLocation))
		{
			const auto Instance = InventoryList.Slots[Index].Instance;
			FGameplayTagStackContainer TagContainer;
			if (const auto StatTagsInstance = Cast<UInventoryItemInstance_StatTags>(Instance))
			{
				TagContainer = StatTagsInstance->GetStatTagsContainer();
			}
			CreateItemActorInFront(InventoryList.Slots[Index].GetItemDef(), Amount, TagContainer, DropLocation);
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
	if (ItemActor != nullptr)
	{
		const int RemainItemAmount = InventoryList.AddItem(ItemActor->ItemID, ItemActor->Amount, ItemActor->OverrideTagStack);
		ItemActor->Amount = RemainItemAmount;
		ItemActor->NativeOnItemPickedUp();
		OnRep_List();
	}
}

void UInventoryManagerComponent::RemoveItem_Implementation(const int Index, const int Amount)
{
	InventoryList.RemoveItemAt(Index, Amount);
}

void UInventoryManagerComponent::CreateItemActorInFront_Implementation(const UInventoryItemDefinition* ItemDef, int Count, FGameplayTagStackContainer TagStackContainer, FVector DropLocation)
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
		NewActor->ItemID = const_cast<UInventoryItemDefinition*>(ItemDef);
		NewActor->Amount = Count;
		NewActor->OverrideTagStack = TagStackContainer.GetTagStacks();
		
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

void UInventoryManagerComponent::DragItemToContainer_Implementation(UInventoryContainerComponent* Container,
                                                                    const int DragIndex, const int DropIndex)
{
	if (!Container)
	{
		return;
	}
	
	//check valid ptr
	if (!InventoryList.Slots.IsValidIndex(DragIndex) || !Container->List.Slots.IsValidIndex(DropIndex))
	{
		return;
	}
	
	if (InventoryList.Slots[DragIndex].GetItemDef() != nullptr
		&& Container->List.Slots[DropIndex].ItemID != nullptr
		&& InventoryList.Slots[DragIndex].GetItemDef() == Container->List.Slots[DropIndex].ItemID)
	{
		//Stack
		const int MaxStackAmount = InventoryList.Slots[DragIndex].GetItemDef()->MaxStackAmount;
		const int FinalAmount = Container->List.Slots[DropIndex].StackCount + InventoryList.Slots[DragIndex].StackCount;
		const int CalculateAmount = FinalAmount - MaxStackAmount;
		if (Container->List.Slots[DropIndex].StackCount != MaxStackAmount)
		{
			if (CalculateAmount > 0)
			{
				Container->List.Slots[DropIndex].StackCount = MaxStackAmount;
				InventoryList.Slots[DragIndex].StackCount = CalculateAmount;
			}
			else
			{
				Container->List.Slots[DropIndex].StackCount = FinalAmount;
				RemoveItem(DragIndex, InventoryList.Slots[DragIndex].StackCount);
			}
			//Rep net
			Container->List.MarkItemDirty(Container->List.Slots[DropIndex]);
			InventoryList.MarkItemDirty(InventoryList.Slots[DragIndex]);
			K2_InventoryListChanged();
			return;
		}
	}
	
	//Switch
	const auto DragItemDef = InventoryList.Slots[DragIndex].GetItemDef();
	const auto StackCount = InventoryList.Slots[DragIndex].StackCount;
	const auto Instance = Cast<UInventoryItemInstance_StatTags>(InventoryList.Slots[DragIndex].Instance);
	const FContainerSlot Slot = Container->List.Slots[DropIndex];
	FGameplayTagStackContainer TagStacks;
	if (Instance)
	{
		TagStacks = Instance->GetStatTagsContainer();
	}
	Container->SetItem(DragItemDef, StackCount, TagStacks, DropIndex);
	RemoveItem(DragIndex, StackCount);
	InventoryList.SetItemAt(Slot.ItemID, Slot.StackCount, Slot.StackTagContainer.GetTagStacks(), DragIndex);
}

void UInventoryManagerComponent::DragDropItem_Implementation(const int DragIndex, const int DropIndex)
{
	InventoryList.DragDropItem(DragIndex, DropIndex);

	OnRep_List();
}

void UInventoryManagerComponent::ChangeQuickBarIndex_Server_Implementation(const int Index)
{
	if (SelectedQuickBarIndex != Index)
	{
		SelectedQuickBarIndex = Index;
		OnRep_SelectedQuickBarIndex();
	}
}

void UInventoryManagerComponent::ForceUnequipItem_Implementation()
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

void UInventoryManagerComponent::CancelForceUnequipItem_Implementation()
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

void UInventoryManagerComponent::ClearItems()
{
	for (int a = 0; a <= InventoryList.Slots.Num() - 1; a = a + 1)
	{
		InventoryList.RemoveItemAt(a, InventoryList.Slots[a].StackCount);
	}
}

FInventorySaveData UInventoryManagerComponent::GetSaveData()
{
	FInventorySaveData InventorySaveData;
	InventorySaveData.SlotsAmount = InventorySlotAmount;
	InventorySaveData.SelectedQuickBarIndex = SelectedQuickBarIndex;
	InventorySaveData.bIsValid = true;
	for (int Idx = 0; Idx < InventoryList.Slots.Num(); Idx++)
	{
		if (!InventoryList.Slots[Idx].GetItemDef())
		{
			continue;
		}

		TArray<uint8> InstanceData;
		if (auto InstancePtr = InventoryList.Slots[Idx].Instance)
		{
			// Save to binary
			InstancePtr->K2_OnPreSaveGame();
			FMemoryWriter MemoryWriter(InstanceData, true);
			FItemInstanceArchive Ar (MemoryWriter);
			InstancePtr->Serialize(Ar);
		}
		
		auto NewSlotData = FItemSlotSaveData(InstanceData, InventoryList.Slots[Idx].GetItemDef(), InventoryList.Slots[Idx].StackCount);
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
	InventoryList.AddEmptySlots(InventorySlotAmount);

	for (auto DataPair : SaveData.SlotDataMap)
	{
		InventoryList.Slots[DataPair.Key].ItemDefinition = DataPair.Value.ItemDefinition;
		InventoryList.Slots[DataPair.Key].StackCount = DataPair.Value.StackCount;
		if (auto InstanceInDef = DuplicateObject(DataPair.Value.ItemDefinition->ItemInstance, GetOwner()))
		{
			InventoryList.Slots[DataPair.Key].Instance = InstanceInDef;
			InstanceInDef->SetItemDef(DataPair.Value.ItemDefinition);
			// Load instance
			FMemoryReader MemoryReader(DataPair.Value.Data, true);
			FItemInstanceArchive Ar(MemoryReader);
			InstanceInDef->Serialize(Ar);
			InstanceInDef->K2_OnPostLoadGame();
		}
		InventoryList.MarkItemDirty(InventoryList.Slots[DataPair.Key]);
	}
	
	SelectedQuickBarIndex = SaveData.SelectedQuickBarIndex;
	OnRep_SelectedQuickBarIndex();
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
		if (UInventoryItemInstance* Instance = Slot.Instance; Instance && IsValid(Instance))
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
			if (UInventoryItemInstance* Instance = Slot.Instance; IsValid(Instance))
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
		if (const auto Instance = Cast<UInventoryItemInstance_Equipment>(EquippedInstance))
		{
			Instance->OnUnequipped();
			Instance->SetInstigator(nullptr);
		}
	}
	EquippedInstance = nullptr;
}

void UInventoryManagerComponent::EquipInstance(int SlotIndex)
{
	if (InventoryList.Slots.IsValidIndex(SlotIndex))
	{
		EquippedInstance = InventoryList.Slots[SlotIndex].Instance;
		if (const auto Instance = Cast<UInventoryItemInstance_Equipment>(EquippedInstance))
		{
			Instance->OnEquipped();
			Instance->SetInstigator(GetOwner());
		}
	}
}
