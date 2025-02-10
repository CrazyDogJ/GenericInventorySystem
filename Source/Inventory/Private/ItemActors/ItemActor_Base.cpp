// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemActors/ItemActor_Base.h"

#include "Inventory.h"
#include "InventoryItemDefinition.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"

AItemActor_Base::AItemActor_Base(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	SetReplicatingMovement(true);
}

bool AItemActor_Base::ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch,
	FReplicationFlags* RepFlags)
{
	if (bUseDefaultInstance)
	{
		return Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	}
	
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (auto Itr : ItemInstances)
	{
		if (Itr && IsValid(Itr))
		{
			WroteSomething |= Channel->ReplicateSubobject(Itr, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

void AItemActor_Base::BeginReplication()
{
	if (bUseDefaultInstance)
	{
		Super::BeginReplication();
		return;
	}

	for (auto Itr : ItemInstances)
	{
		if (Itr)
		{
			AddReplicatedSubObject(Itr);
		}
	}
}

void AItemActor_Base::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	//SetHidden(false);
	//if (ItemID == nullptr || Amount == 0)
	//{
	//	SetHidden(true);
	//	return;
	//}
	//
	//if (!bUseDefaultInstance)
	//{
	//	bool bInstancesValid = true;
	//	for (auto Itr : ItemInstances)
	//	{
	//		if (Itr != nullptr && Itr->GetClass() != ItemID->DefaultItemInstance.GetClass())
	//		{
	//			bInstancesValid = false;
	//			break;
	//		}
	//	}
	//	if (!bInstancesValid)
	//	{
	//		SetHidden(true);
	//	}
	//}
}

#if WITH_EDITOR
void AItemActor_Base::RefreshItemInstance()
{
	if (bUseDefaultInstance)
	{
		Modify();
		ItemInstances.Empty();
		// ReSharper disable once CppExpressionWithoutSideEffects
		MarkPackageDirty();
		return;
	}
	
	if (ItemID)
	{
		Modify();
		ItemInstances.Empty();
		int Times = 0;
		switch (ItemID->ItemInstanceType)
		{
		case IIT_None: Times = 0;
			break;
		case IIT_OnlyOne: Times = 1;
			break;
		case IIT_Multiple: Times = Amount;
			break;
		default: ;
		}
		for (int Idx = 0; Idx < Times; Idx++)
		{
			if (ItemID->DefaultItemInstance == nullptr)
			{
				UE_LOG(LogInventory, Error, TEXT("Has no default item instance, please check item definition : %s"), *ItemID->GetName())
				return;
			}
			ItemInstances.Emplace(NewObject<UInventoryItemInstance>(this, ItemID->DefaultItemInstance->GetClass(),
																NAME_None, RF_NoFlags,
																ItemID->DefaultItemInstance));
		}
		// ReSharper disable once CppExpressionWithoutSideEffects
		MarkPackageDirty();
	}
}
#endif

void AItemActor_Base::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ItemID);
	DOREPLIFETIME(ThisClass, Amount);
	DOREPLIFETIME(ThisClass, ItemInstances);
	DOREPLIFETIME(ThisClass, bUseDefaultInstance)
}
