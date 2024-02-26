// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemRecipe.h"

UInventoryItemRecipe::UInventoryItemRecipe(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TArray<UInventoryItemRecipeCondition*> UCraftingFunctionLibrary::GetConditions(
	TSubclassOf<UInventoryItemRecipe> Recipe)
{
	if (Recipe)
	{
		const auto result = Recipe.GetDefaultObject()->Conditions;
		return result;
	}
	TArray<UInventoryItemRecipeCondition*> empty;
	return empty;
}
