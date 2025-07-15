// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/InventoryItemInstanceBase.h"
#include "Core/InventoryItemFragmentBase.h"
#include "Net/UnrealNetwork.h"

UInventoryItemInstanceBase::UInventoryItemInstanceBase()
{
}

void UInventoryItemInstanceBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InstanceFragments);
	DOREPLIFETIME(ThisClass, InventoryItemDef);
}

const UInventoryItemFragmentBase* UInventoryItemInstanceBase::FindFragmentByClass(
	TSubclassOf<UInventoryItemFragmentBase> FragmentClass) const
{
	if (!FragmentClass || InstanceFragments.IsEmpty())
	{
		return nullptr;
	}
	
	for (auto Itr : InstanceFragments)
	{
		if (Itr && Itr.IsA(FragmentClass))
		{
			return Itr;
		}
	}

	return nullptr;
}
