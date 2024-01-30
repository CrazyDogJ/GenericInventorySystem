// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemInstance_StatTags.h"
#include "InventoryItemInstance_Equipment.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class INVENTORY_API UInventoryItemInstance_Equipment : public UInventoryItemInstance_StatTags
{
	GENERATED_BODY()

public:
	virtual void OnEquipped();
	virtual void OnUnequipped();
	
protected:

	UFUNCTION(BlueprintImplementableEvent, Category=Equipment, meta=(DisplayName="On Equipped"))
	void K2_OnEquipped();

	UFUNCTION(BlueprintImplementableEvent, Category=Equipment, meta=(DisplayName="On Unequipped"))
	void K2_OnUnequipped();
};
