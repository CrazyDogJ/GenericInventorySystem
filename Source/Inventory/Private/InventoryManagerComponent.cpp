// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagerComponent.h"

#include "Inventory.h"
#include "Fragments/InventoryFragment_SkeletalMesh.h"
#include "Logging/LogMacros.h"
#include "Fragments/InventoryFragment_StaticMesh.h"
#include "InventoryItemDefinition.h"
#include "ItemInstances/InventoryItemInstance_Equipment.h"
#include "InventorySettings.h"
#include "Components/SphereComponent.h"
#include "ItemActors/ItemActor_Common.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UInventoryManagerComponent::UInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EquippedInstance = nullptr,
	bForceUnequipped = false,
	SphereComp = nullptr;
}

void UInventoryManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, KnownRecipes);
	DOREPLIFETIME(ThisClass, bForceUnequipped);
}

void UInventoryManagerComponent::BeginPlay()
{
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

	TArray<AActor*> OverlappingActors;
	if (SphereComp)
	{
		SphereComp->GetOverlappingActors(OverlappingActors, AItemActor_Base::StaticClass());
	}
	const auto CachedOverlappedActors = OverlappedActors;
	OverlappedActors.Empty();

	if (OverlappingActors.Num() == 0)
	{
		PickupSelectedID = -1;
	}
	
	for (const auto Actor : OverlappingActors)
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
			OverlappedActors.Add(NotValidItem);
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
	if (OverlappedActors.Num() > 0)
	{
		PickupSelectedID = FMath::Clamp(PickupSelectedID + (bUpOrDown ? 1 : -1), MinIndex, OverlappedActors.Num()-1);
	}
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

void UInventoryManagerComponent::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (const auto Item = Cast<AItemActor_Base>(OtherActor))
	{
		if (OverlappedActors.Find(Item) != INDEX_NONE)
		{
			if (PickupSelectedID == OverlappedActors.Num()-1)
			{
				PickupSelectedID--;
			}
			OnItemEndOverlap(Item);
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
