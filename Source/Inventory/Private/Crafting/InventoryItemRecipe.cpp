// Fill out your copyright notice in the Description page of Project Settings.


#include "Crafting/InventoryItemRecipe.h"

UInventoryItemRecipe::UInventoryItemRecipe(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TArray<UInventoryItemRecipeCondition*> UCraftingFunctionLibrary::GetConditions(
	UInventoryItemRecipe* Recipe)
{
	if (Recipe)
	{
		const auto result = Recipe->Conditions;
		return result;
	}
	TArray<UInventoryItemRecipeCondition*> empty;
	return empty;
}
