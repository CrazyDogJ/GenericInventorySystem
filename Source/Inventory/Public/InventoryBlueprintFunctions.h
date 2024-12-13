// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryManagerComponent.h"
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

public:
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

	UFUNCTION(BlueprintCallable, Category = Inventory)
	static TArray<FGameplayTagStack> GetStatTags(FGameplayTagStackContainer InContainer) {return InContainer.GetTagStacks();}

	/**
	 * Get item definition fragment by class from default item definition.
	 * @param ItemDef 
	 * @param FragmentClass 
	 * @return FragmentObject
	 */
	UFUNCTION(BlueprintCallable, meta=(DeterminesOutputType=FragmentClass))
	static const UInventoryItemFragment* FindItemDefinitionFragment(UInventoryItemDefinition* ItemDef, TSubclassOf<UInventoryItemFragment> FragmentClass);

	// Local press ga input
	UFUNCTION(BlueprintCallable)
	static void PressInputByTag(UAbilitySystemComponent* ASC, const FGameplayTag& InTag);

	// Local release ga input
	UFUNCTION(BlueprintCallable)
	static void ReleaseInputByTag(UAbilitySystemComponent* ASC, const FGameplayTag& InTag);

	UFUNCTION(BlueprintPure)
	static UInventoryItemDefinition* GetItemDefinition(const FInventorySlot& InSlot);
private:
	static TObjectPtr<UInventorySettings> GetInventoryProjectSettings();
};
