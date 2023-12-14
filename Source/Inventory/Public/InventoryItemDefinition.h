// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemInstance.h"
#include "UObject/Object.h"
#include "InventoryItemDefinition.generated.h"

//////////////////////////////////////////////////////////////////////

// Represents a fragment of an item definition
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class INVENTORY_API UInventoryItemFragment : public UObject
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(UInventoryItemInstance* Instance) const {}
};

//////////////////////////////////////////////////////////////////////

/**
 * 
 */
UCLASS(Blueprintable, Const, Abstract)
class INVENTORY_API UInventoryItemDefinition : public UObject
{
	GENERATED_BODY()

public:
	UInventoryItemDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	FText ItemDescription = FText::GetEmpty();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	int MaxStackAmount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	FSlateBrush IconBrush;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Inventory" , Instanced)
	TArray<TObjectPtr<UInventoryItemFragment>> Fragments;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UInventoryItemInstance> Instance_BP;
	
	//Used in blueprint function library below.
	const UInventoryItemFragment* FindFragmentByClass(TSubclassOf<UInventoryItemFragment> FragmentClass) const;
};

UCLASS()
class UInventoryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Get item definition fragment by class from default item definition.
	 * @param ItemDef 
	 * @param FragmentClass 
	 * @return FragmentObject
	 */
	UFUNCTION(BlueprintCallable, meta=(DeterminesOutputType=FragmentClass))
	static const UInventoryItemFragment* FindItemDefinitionFragment(TSubclassOf<UInventoryItemDefinition> ItemDef, TSubclassOf<UInventoryItemFragment> FragmentClass);
};