// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemInstance_StatTags.h"

#include "InventoryFragment_Stats.h"
#include "InventoryItemDefinition.h"
#include "InventorySettings.h"
#include "Net/UnrealNetwork.h"

UInventoryItemInstance_StatTags::UInventoryItemInstance_StatTags(const FObjectInitializer& ObjectInitializer)
{
}

void UInventoryItemInstance_StatTags::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, StatTags);
}

void UInventoryItemInstance_StatTags::AddStatTagStack(FGameplayTag Tag, float StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void UInventoryItemInstance_StatTags::RemoveStatTagStack(FGameplayTag Tag, float StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

float UInventoryItemInstance_StatTags::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool UInventoryItemInstance_StatTags::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}

void UInventoryItemInstance_StatTags::SetStatTagStacks(TArray<FGameplayTagStack> Tags)
{
	StatTags.SetStackTags(Tags);
}
