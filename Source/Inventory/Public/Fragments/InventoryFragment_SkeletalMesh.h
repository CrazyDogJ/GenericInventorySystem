// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryFragment_Mesh.h"
#include "InventoryFragment_SkeletalMesh.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class INVENTORY_API UInventoryFragment_SkeletalMesh : public UInventoryFragment_Mesh
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	USkeletalMesh* PickupSkeletalMesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	USkeletalMesh* PickupSkeletalMesh_Multiple;
};
