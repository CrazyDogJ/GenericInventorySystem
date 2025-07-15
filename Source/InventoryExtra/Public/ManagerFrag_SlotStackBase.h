// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InventorySlotArray.h"
#include "Core/InventoryManagerFragment.h"
#include "ManagerFrag_SlotStackBase.generated.h"

class UItemFrag_SlotStackBase;
class UInventoryItemInstanceBase;

UCLASS(Blueprintable)
class INVENTORYEXTRA_API UManagerFrag_SlotStackBase : public UInventoryManagerFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere)
	FGameplayTag CategoryTag;

	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere)
	int SlotsCount = 1;

	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere)
	FInventorySlotArray InventorySlotArray;

	virtual void K2_BeginPlay_Implementation() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual TArray<TObjectPtr<UObject>> CollectReplicatedSubobjects() override;
	
	UFUNCTION(BlueprintCallable)
	void MarkItemDirty(int Index);

	UFUNCTION(BlueprintCallable)
	void MarkArrayDirty();

#pragma region Main Functions
	UFUNCTION(BlueprintCallable)
	int FindEmpty() const;

	UFUNCTION(BlueprintCallable)
	int FindStack(UInventoryItemDefBase* ItemDef);

	UFUNCTION(BlueprintCallable)
	const UItemFrag_SlotStackBase* GetItemFragFromItemDef(UInventoryItemDefBase* ItemDef);

	UFUNCTION(BlueprintCallable)
	int GetItemMaxStackAmount(UInventoryItemDefBase* ItemDef);
	
	UFUNCTION(BlueprintCallable)
	int AddItemDef(UInventoryItemDefBase* ItemDef, int Count);

	UFUNCTION(BlueprintCallable)
	int AddItemInstance(UInventoryItemInstanceBase* ItemInstance, int Count);
#pragma endregion
};
