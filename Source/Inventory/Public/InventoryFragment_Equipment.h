// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemDefinition.h"
#include "InventoryFragment_Equipment.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORY_API UInventoryFragment_Equipment : public UInventoryItemFragment
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TArray<TObjectPtr<const UAbilitySet>> AbilitySetsToGrant;

	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TSubclassOf<AActor> WeaponActorToSpawn;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Equipment)
	TSubclassOf<UInventoryItemDefinition> AmmoItemUsage;
};
