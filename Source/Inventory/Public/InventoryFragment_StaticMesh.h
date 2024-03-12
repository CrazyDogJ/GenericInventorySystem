// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemDefinition.h"
#include "InventoryFragment_StaticMesh.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORY_API UInventoryFragment_StaticMesh : public UInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	UStaticMesh* PickupStaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	UStaticMesh* PickupStaticMesh_Multiple;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	bool bEnableCollisionWithPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	bool bEnablePhysics;
};
