// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryInstancedStructArray.h"
#include "Engine/DataAsset.h"
#include "StructUtils/InstancedStruct.h"
#include "InventoryInstancedStructDef.generated.h"

class UInventoryInstancedStructInterface;
class UInventoryInstancedStructObject;
/**
 * 
 */
UCLASS(Blueprintable, Const)
class INVENTORY_API UInventoryInstancedStructDef : public UDataAsset
{
	GENERATED_BODY()

public:
	virtual void PostLoad() override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FInstancedStruct> DefaultProperties;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TSubclassOf<UInventoryInstancedStructObject> InventoryItemObject;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	FInventoryInstancedStructContainer ObjectDefaultContainer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FInstancedStruct> OverrideInstancedStructs;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int FindDefaultPropertyByStructType(const UScriptStruct* ScriptStruct);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UInventoryInstancedStructObject* NewInstancedStructObject(UObject* Outer);
};
