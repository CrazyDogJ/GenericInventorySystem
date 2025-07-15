// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"
#include "InventorySlotEntry.generated.h"

class UInventoryItemInstanceBase;
class UInventoryItemDefBase;

USTRUCT(BlueprintType)
struct FInventorySlotEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FInventorySlotEntry() {}

public:
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UInventoryItemDefBase> ItemDefinition;
	
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UInventoryItemInstanceBase> ItemInstance;
	
	UPROPERTY(BlueprintReadWrite)
	int StackCount = 0;

	bool IsSlotEmpty() const;
	void ClearSlot();
};
