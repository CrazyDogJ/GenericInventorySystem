// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryFragment_Stats.h"

#include "GameplayTagContainer.h"
#include "InventoryItemInstance_StatTags.h"

void UInventoryFragment_Stats::OnInstanceCreated(UInventoryItemInstance* Instance) const
{
	for (const auto& KVP : InitialItemStats)
	{
		Cast<UInventoryItemInstance_StatTags>(Instance)->AddStatTagStack(KVP.Key, KVP.Value);
	}
}

float UInventoryFragment_Stats::GetItemStatByTag(FGameplayTag Tag) const
{
	if (const float* StatPtr = InitialItemStats.Find(Tag))
	{
		return *StatPtr;
	}

	return 0;
}
