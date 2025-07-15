// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InventoryItemInstanceBase.generated.h"

class UInventoryItemDefBase;
class UInventoryItemFragmentBase;

/**
 * 
 */
UCLASS(BlueprintType)
class INVENTORY_API UInventoryItemInstanceBase : public UObject
{
	GENERATED_BODY()

	UInventoryItemInstanceBase();
	
public:
	
	// UObject interface
	// Network
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	// End of UObject interface
	
	UFUNCTION(BlueprintCallable, BlueprintPure=false, meta=(DeterminesOutputType=FragmentClass))
	const UInventoryItemFragmentBase* FindFragmentByClass(TSubclassOf<UInventoryItemFragmentBase> FragmentClass) const;
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	UInventoryItemDefBase* InventoryItemDef;
	
	UPROPERTY(Replicated)
	TArray<TObjectPtr<UInventoryItemFragmentBase>> InstanceFragments;
};
