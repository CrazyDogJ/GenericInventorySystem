// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Core/InventoryItemFragmentBase.h"
#include "ItemFrag_SlotStackBase.generated.h"

class UInventoryManagerComp;
/**
 * 
 */
UCLASS()
class INVENTORYEXTRA_API UItemFrag_SlotStackBase : public UInventoryItemFragmentBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FGameplayTagContainer CategoryTags;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int MaxStackAmount = 10;
};
