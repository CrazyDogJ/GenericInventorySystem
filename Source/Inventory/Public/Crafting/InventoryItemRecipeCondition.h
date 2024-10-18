// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryManagerComponent.h"
#include "UObject/Object.h"
#include "InventoryItemRecipeCondition.generated.h"

/**
 * 
 */
UCLASS(DefaultToInstanced, EditInlineNew, Abstract, Blueprintable)
class INVENTORY_API UInventoryItemRecipeCondition : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	FText GetDisplayText(UInventoryManagerComponent* IMC) const;
	virtual FText GetDisplayText_Implementation(UInventoryManagerComponent* IMC) const { return FText::GetEmpty(); }

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool CraftCondition(UInventoryManagerComponent* IMC) const;
	virtual bool CraftCondition_Implementation(UInventoryManagerComponent* IMC) const { return false; }
	
};
