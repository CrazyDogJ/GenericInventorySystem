// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InventoryItemDefinition.h"
#include "InventoryFragment_Quality.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORY_API UInventoryFragment_Quality : public UInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta=(Categories="InventoryQuality"))
	FGameplayTag QualityTag;
};
