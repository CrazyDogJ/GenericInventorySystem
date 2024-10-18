// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemInstances/InventoryItemInstance.h"
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

#define LOCTEXT_NAMESPACE "Inventory"
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	FString ItemID;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	FText DisplayName = LOCTEXT("", "");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	FText ItemDescription = LOCTEXT("", "");
#undef LOCTEXT_NAMESPACE
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	int MaxStackAmount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	FSlateBrush IconBrush;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Inventory" , Instanced)
	TArray<TObjectPtr<UInventoryItemFragment>> Fragments;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UInventoryItemInstance> Instance_BP;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	//Used in blueprint function library.
	const UInventoryItemFragment* FindFragmentByClass(const TSubclassOf<UInventoryItemFragment>& FragmentClass) const;

	//Used in c++ function.
	template <typename T>
	const T* FindFragmentByClass() const;
};
