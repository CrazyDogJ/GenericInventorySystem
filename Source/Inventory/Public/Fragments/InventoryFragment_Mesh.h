// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemDefinition.h"
#include "InventoryFragment_Mesh.generated.h"

/**
 * 
 */
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class INVENTORY_API UInventoryFragment_Mesh : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	bool bEnableCollisionWithPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	bool bEnablePhysics;
};
