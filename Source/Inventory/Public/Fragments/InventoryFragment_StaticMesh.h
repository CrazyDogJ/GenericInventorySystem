// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryFragment_Mesh.h"
#include "InventoryFragment_StaticMesh.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORY_API UInventoryFragment_StaticMesh : public UInventoryFragment_Mesh
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	UStaticMesh* PickupStaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	UStaticMesh* PickupStaticMesh_Multiple;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh", meta=(Units = "kg"))
	float Mass;
};
