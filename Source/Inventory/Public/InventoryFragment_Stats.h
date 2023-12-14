// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemDefinition.h"
#include "InventoryFragment_Stats.generated.h"

struct FGameplayTag;
class UInventoryItemInstance;
class UObject;

/**
 * 
 */
UCLASS()
class INVENTORY_API UInventoryFragment_Stats : public UInventoryItemFragment
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category=Stats)
	TMap<FGameplayTag, float> InitialItemStats;

public:
	virtual void OnInstanceCreated(UInventoryItemInstance* Instance) const override;

	UFUNCTION(BlueprintCallable)
	float GetItemStatByTag(FGameplayTag Tag) const;
};
