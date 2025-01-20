// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemInstances/InventoryItemInstance_Equipment.h"

#include "AbilitySet.h"
#include "AbilitySystemGlobals.h"
#include "Fragments/InventoryFragment_Equipment.h"

void UInventoryItemInstance_Equipment::OnEquipped()
{
	const auto ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Cast<AActor>(GetOuter()));
	ensure(ASC);
	for (auto abilitySet : AbilitySets)
	{
		abilitySet.Get()->GiveToAbilitySystem(ASC, &GrantedHandles, this);
	}
	K2_OnEquipped();
}

void UInventoryItemInstance_Equipment::OnUnequipped()
{
	const auto ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Cast<AActor>(GetOuter()));
	GrantedHandles.TakeFromAbilitySystem(ASC);
	K2_OnUnequipped();
}

void UInventoryItemInstance_Equipment::OnInstanceDestroyed()
{
	OnUnequipped();
	OnCategoryChanged(FGameplayTag::EmptyTag);
	Super::OnInstanceDestroyed();
}
