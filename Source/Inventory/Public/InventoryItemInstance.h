// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InventoryItemInstance.generated.h"

class UInventoryItemDefinition;
/**
 * Inventory Item Instance that can be blueprintable in order to make custom events.
 */
UCLASS(BlueprintType, Blueprintable)
class INVENTORY_API UInventoryItemInstance : public UObject
{
	GENERATED_BODY()

public:
	UInventoryItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UObject interface
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual UWorld* GetWorld() const override final;

	//~End of UObject interface

	UFUNCTION(BlueprintPure)
	UObject* GetInstigator() const { return Instigator; }

	void SetInstigator(UObject* InInstigator) { Instigator = InInstigator; }
	
	UFUNCTION(BlueprintPure)
	APawn* GetPawn() const;
	
	virtual void OnInstanceCreated()
	{
		// Default as outer object.
		SetInstigator(GetOuter());
		K2_OnInstanceCreated();
	}

	virtual void OnInstanceDestroyed()
	{
		K2_OnInstanceDestroyed();
	}
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName="On Instance Created")
	void K2_OnInstanceCreated();

	UFUNCTION(BlueprintImplementableEvent, DisplayName="On Instance Created")
	void K2_OnInstanceDestroyed();
	
	UFUNCTION(BlueprintPure)
	TSubclassOf<UInventoryItemDefinition> GetItemDef() const
	{
		return ItemDef;
	}
	
	UFUNCTION(BlueprintCallable, BlueprintPure=false, meta=(DeterminesOutputType=FragmentClass))
	const UInventoryItemFragment* FindFragmentByClass(TSubclassOf<UInventoryItemFragment> FragmentClass) const;

	template <typename ResultClass>
	const ResultClass* FindFragmentByClass() const
	{
		return (ResultClass*)FindFragmentByClass(ResultClass::StaticClass());
	}

	void SetItemDef(TSubclassOf<UInventoryItemDefinition> InDef);

private:
	UFUNCTION()
	void OnRep_Instigator();
	
	// The item definition
	UPROPERTY(Replicated)
	TSubclassOf<UInventoryItemDefinition> ItemDef;
	
	UPROPERTY(ReplicatedUsing=OnRep_Instigator)
	TObjectPtr<UObject> Instigator;
};
