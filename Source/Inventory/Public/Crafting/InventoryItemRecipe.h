// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemDefinition.h"
#include "UObject/Object.h"
#include "InventoryItemRecipe.generated.h"

class UInventoryItemRecipeCondition;

/**
 * 
 */
UCLASS(Blueprintable, Const, Abstract)
class INVENTORY_API UInventoryItemRecipe : public UObject
{
	GENERATED_BODY()

public:
	UInventoryItemRecipe(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting")
	FText DisplayDescText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting")
	TMap<TSubclassOf<UInventoryItemDefinition>, int> NeedItems;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting")
	TMap<TSubclassOf<UInventoryItemDefinition>, int> OutItems;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Inventory" , Instanced)
	TArray<TObjectPtr<UInventoryItemRecipeCondition>> Conditions;
};

UCLASS()
class UCraftingFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static TArray<UInventoryItemRecipeCondition*> GetConditions(TSubclassOf<UInventoryItemRecipe> Recipe);
};