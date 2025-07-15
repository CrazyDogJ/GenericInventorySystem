// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/InventoryItemDefBase.h"

#include "Core/InventoryItemFragmentBase.h"
#include "Core/InventoryItemInstanceBase.h"

UInventoryItemDefBase::UInventoryItemDefBase()
{
}

const UInventoryItemFragmentBase* UInventoryItemDefBase::FindFragmentByClass(
	TSubclassOf<UInventoryItemFragmentBase> FragmentClass) const
{
	if (!FragmentClass || Fragments.IsEmpty())
	{
		return nullptr;
	}
	
	for (auto Itr : Fragments)
	{
		if (Itr && Itr.IsA(FragmentClass))
		{
			return Itr;
		}
	}

	return nullptr;
}

TObjectPtr<UInventoryItemInstanceBase> UInventoryItemDefBase::NewInstanceObject(UObject* OuterObject)
{
	if (!OuterObject) return nullptr;

	const auto Instance = NewObject<UInventoryItemInstanceBase>(OuterObject);
	
	Instance->InventoryItemDef = this;
	
	for (auto Itr : Fragments)
	{
		if (Itr->bRuntime)
		{
			auto NewFrag = NewObject<UInventoryItemFragmentBase>(Instance, Itr.GetClass(), NAME_None, RF_NoFlags, Itr);
			Instance->InstanceFragments.Add(NewFrag);
			NewFrag->OnInstanceCreated(Instance);
		}
	}
	
	return Instance;
}
