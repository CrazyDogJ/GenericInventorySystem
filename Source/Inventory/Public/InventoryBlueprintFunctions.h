// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventorySettings.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InventoryBlueprintFunctions.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORY_API UInventoryBlueprintFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintPure, Category = Inventory)
	static TArray<FQualitySetting> GetQualitySettings();

	UFUNCTION(BlueprintPure, Category = Inventory)
	static FLinearColor GetQualityColorByGameplayTag(const FGameplayTag Tag);

	UFUNCTION(BlueprintPure, Category = Inventory)
	static FText GetQualityNameByGameplayTag(const FGameplayTag Tag);

	UFUNCTION(BlueprintPure, Category = Inventory)
	static bool IsTextNumeric(const FText& inputText);

	/*
	 * Return custom depth stencil that used for selected item actor's outline.
	**/
	UFUNCTION(BlueprintPure, Category = Inventory)
	static int32 GetInventoryCustomDepthStencil();

	UFUNCTION(BlueprintCallable, Category = Inventory)
	static void BeginBuff(UInventoryItemInstance_StatTags* Instance);

	UFUNCTION(BlueprintCallable, Category = Inventory)
	static void EndBuff(UInventoryItemInstance_StatTags* Instance);

	UFUNCTION(BlueprintCallable, Category = Inventory)
	static FText GetDescriptionFromBuffObject(FGameplayTagStack Tag);
};
