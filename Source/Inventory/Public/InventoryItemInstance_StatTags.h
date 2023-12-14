// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagStack.h"
#include "InventoryItemInstance.h"
#include "InventoryItemInstance_StatTags.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class INVENTORY_API UInventoryItemInstance_StatTags : public UInventoryItemInstance
{
	GENERATED_BODY()

public:
	UInventoryItemInstance_StatTags(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	// Adds a specified number of stacks to the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void AddStatTagStack(FGameplayTag Tag, float StackCount);

	// Removes a specified number of stacks from the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category= Inventory)
	void RemoveStatTagStack(FGameplayTag Tag, float StackCount);

	// Returns the stack count of the specified tag (or 0 if the tag is not present)
	UFUNCTION(BlueprintCallable, Category=Inventory)
	float GetStatTagStackCount(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category=Inventory)
	TArray<FGameplayTagStack> GetStatTags() const {return StatTags.GetTagStacks();};
	
	// Returns true if there is at least one stack of the specified tag
	UFUNCTION(BlueprintCallable, Category=Inventory)
	bool HasStatTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category= Inventory)
	void SetStatTagStacks(TArray<FGameplayTagStack> Tags);

	UFUNCTION(BlueprintPure, Category=Inventory)
	FGameplayTagStackContainer GetStatTagsContainer() {return  StatTags;};
	
private:
	UPROPERTY(Replicated)
	FGameplayTagStackContainer StatTags;
};
