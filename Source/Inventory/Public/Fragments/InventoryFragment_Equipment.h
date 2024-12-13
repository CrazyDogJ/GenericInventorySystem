// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemDefinition.h"
#include "InventoryFragment_Equipment.generated.h"

class UAbilitySet;
/**
 * 
 */
UCLASS()
class INVENTORY_API UInventoryFragment_Equipment : public UInventoryItemFragment
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Equipment)
	TArray<TObjectPtr<const UAbilitySet>> AbilitySetsToGrant;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Equipment)
	TSubclassOf<AActor> WeaponActorToSpawn;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Equipment)
	TObjectPtr<UInventoryItemDefinition> AmmoItemUsage;
	
};
