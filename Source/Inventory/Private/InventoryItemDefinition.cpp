// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemDefinition.h"

UInventoryItemDefinition::UInventoryItemDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

const UInventoryItemFragment* UInventoryItemDefinition::FindFragmentByClass(
	TSubclassOf<UInventoryItemFragment> FragmentClass) const
{
	if (FragmentClass != nullptr)
	{
		for (const UInventoryItemFragment* Fragment : Fragments)
		{
			if (Fragment && Fragment->IsA(FragmentClass))
			{
				return Fragment;
			}
		}
	}

	return nullptr;
}

const UInventoryItemFragment* UInventoryFunctionLibrary::FindItemDefinitionFragment(TSubclassOf<UInventoryItemDefinition> ItemDef, TSubclassOf<UInventoryItemFragment> FragmentClass)
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		return GetDefault<UInventoryItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
	}
	return nullptr;
}
