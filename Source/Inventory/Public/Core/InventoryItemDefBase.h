// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InventoryItemDefBase.generated.h"

class UInventoryItemInstanceBase;
class UInventoryItemFragmentBase;

/**
 * 
 */
UCLASS(Blueprintable, Const)
class INVENTORY_API UInventoryItemDefBase : public UDataAsset
{
	GENERATED_BODY()

public:
	UInventoryItemDefBase();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Inventory", Instanced)
	TArray<TObjectPtr<UInventoryItemFragmentBase>> Fragments;

	UFUNCTION(BlueprintCallable, BlueprintPure=false, meta=(DeterminesOutputType=FragmentClass))
	const UInventoryItemFragmentBase* FindFragmentByClass(TSubclassOf<UInventoryItemFragmentBase> FragmentClass) const;

	TObjectPtr<UInventoryItemInstanceBase> NewInstanceObject(UObject* OuterObject);
};
