// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemActor_Base.h"
#include "ItemActor_Common.generated.h"

UCLASS()
class INVENTORY_API AItemActor_Common : public AItemActor_Base
{
	GENERATED_BODY()

public:
	AItemActor_Common();

	void SetupActor(const FInventorySlot& Slot);
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Inventory", AdvancedDisplay)
	UMeshComponent* MeshComponent;

	UPROPERTY()
	FVector InitVelocity;

	UPROPERTY()
	FVector InitAngularVelocity;
	
	virtual void OnConstruction(const FTransform& Transform) override;

	//The same as common
	virtual void NativeOnItemPickedUp() override;
	virtual void OnRep_ItemID() override;

	void InitComps(UMeshComponent*& InMeshComponent, const FTransform& Transform);
};
