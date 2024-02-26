// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemInstance_StatTags.h"
#include "UObject/Object.h"
#include "InventoryBuffInfoBase.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class INVENTORY_API UInventoryBuffInfoBase : public UObject
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Inventory)
	FText Name;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Inventory)
	FText Description;

	UFUNCTION(BlueprintNativeEvent, Category=Inventory)
	FText DescriptionOverride(const float Value);

	UFUNCTION(BlueprintImplementableEvent, Category=Inventory)
	void OnBuffBegin(const UInventoryItemInstance_StatTags* Instance, const float Value);

	UFUNCTION(BlueprintImplementableEvent, Category=Inventory)
	void OnBuffEnd(const UInventoryItemInstance_StatTags* Instance, const float Value);

	/**
	 * Call when buff ended.
	 */
	UFUNCTION(BlueprintCallable, Category=Inventory)
	void CleanUp();
};
