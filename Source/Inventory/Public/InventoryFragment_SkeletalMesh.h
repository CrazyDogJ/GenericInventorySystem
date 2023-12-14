// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemDefinition.h"
#include "InventoryFragment_SkeletalMesh.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORY_API UInventoryFragment_SkeletalMesh : public UInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	USkeletalMesh* PickupSkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	bool bEnableCollisionWithPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	bool bEnablePhysics;
};
