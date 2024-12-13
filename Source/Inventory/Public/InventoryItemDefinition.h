// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemInstances/InventoryItemInstance.h"
#include "UObject/Object.h"
#include "InventoryItemDefinition.generated.h"

class UInventoryFragment_Mesh;

UENUM(BlueprintType)
enum EMeshType : uint8
{
	MT_None = 0,
	MT_StaticMesh = 1,
	MT_SkeletalMesh = 2
};

/** Represents a fragment of an item definition */
UCLASS(DefaultToInstanced, EditInlineNew, Abstract, Blueprintable)
class INVENTORY_API UInventoryItemFragment : public UObject
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(UInventoryItemInstance* Instance) const {}
};

/**
 * Item definition define item's properties
 */
UCLASS(Blueprintable, Const)
class INVENTORY_API UInventoryItemDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UInventoryItemDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Basic Info")
	FString ItemID;

#define LOCTEXT_NAMESPACE "Inventory"
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Basic Info")
	FText DisplayName = LOCTEXT("", "");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Basic Info")
	FText ItemDescription = LOCTEXT("", "");
#undef LOCTEXT_NAMESPACE
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Basic Info")
	int MaxStackAmount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Basic Info")
	FSlateBrush IconBrush;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
	TEnumAsByte<EMeshType> MeshType = MT_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", Instanced)
	TObjectPtr<UInventoryFragment_Mesh> MeshSettings;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Inventory", Instanced)
	TArray<TObjectPtr<UInventoryItemFragment>> Fragments;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", Instanced)
	TObjectPtr<UInventoryItemInstance> ItemInstance;
	
	//UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	//TSubclassOf<UInventoryItemInstance> Instance_BP;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	//Used in blueprint function library.
	const UInventoryItemFragment* FindFragmentByClass(const TSubclassOf<UInventoryItemFragment>& FragmentClass) const;

	//Used in c++ function.
	template <typename T>
	const T* FindFragmentByClass() const;
};
