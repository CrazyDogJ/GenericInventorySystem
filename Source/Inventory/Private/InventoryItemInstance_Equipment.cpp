// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemInstance_Equipment.h"

#include "AbilitySet.h"
#include "AbilitySystemGlobals.h"
#include "InventoryFragment_Equipment.h"

void UInventoryItemInstance_Equipment::OnEquipped()
{
	auto AbilitySets = Cast<UInventoryFragment_Equipment>(FindFragmentByClass(UInventoryFragment_Equipment::StaticClass()))->AbilitySetsToGrant;
	const auto ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Cast<AActor>(GetOuter()));
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
