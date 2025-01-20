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
	MT_None = 0			UMETA(DisplayName = "No Item Mesh"),
	MT_StaticMesh = 1	UMETA(DisplayName = "Static Mesh"),
	MT_SkeletalMesh = 2 UMETA(DisplayName = "Skeletal Mesh"),
};

UENUM(BlueprintType)
enum EItemInstanceType : uint8
{
	IIT_None = 0		UMETA(DisplayName = "No Item Instance"),
	IIT_OnlyOne = 1		UMETA(DisplayName = "One Instance Per Inventory"),
	IIT_Multiple = 2	UMETA(DisplayName = "Multiple Instances Per Slot/Inventory"),
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

#pragma region Properties
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
	FString ValidateMessage;
	
	/** Be used to identify localization text key. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Basic Info")
	FString ItemID;

#define LOCTEXT_NAMESPACE "Inventory"
	/** Item's display name. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Basic Info")
	FText DisplayName = LOCTEXT("", "");

	/** Item's display description. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Basic Info")
	FText ItemDescription = LOCTEXT("", "");
#undef LOCTEXT_NAMESPACE

	/** You can set max stack amount for each category */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Basic Info", meta=(Categories="Inventory.Category"))
	TMap<FGameplayTag, int> MaxStackAmountPerCategory;
	
	/** Item's icon shown in UI*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Basic Info")
	FSlateBrush IconBrush;

	/** Be used in item actor to show specific mesh type */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
	TEnumAsByte<EMeshType> MeshType = MT_None;

	/** Mesh settings fragment */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", Instanced)
	TObjectPtr<UInventoryFragment_Mesh> MeshSettings;

	/** Item instance type */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Instance")
	TEnumAsByte<EItemInstanceType> ItemInstanceType = IIT_None;

	/** You can make your own blueprint to modify it for default */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Instance", Instanced, meta=(EditCondition="ItemInstanceType != IIT_None", EditConditionHides))
	TObjectPtr<UInventoryItemInstance> DefaultItemInstance;
	
	/** Additional item descriptions */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category= "Inventory", Instanced)
	TArray<TObjectPtr<UInventoryItemFragment>> Fragments;
#pragma endregion

#pragma region Functions
	/** We return found max stack amount. If category id is empty, we return default max stack amount*/
	int GetMaxStackAmount(const FGameplayTag CategoryTag) const;

	/** Get maximum max stack amount */
	int GetMaxMaxStackAmount() const;
#pragma endregion
	
	//UInventoryItemInstance* GetItemInstance() const;
#if WITH_EDITOR
	void SetValidateMessage();
	virtual void PostLoad() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	//Used in blueprint function library.
	const UInventoryItemFragment* FindFragmentByClass(const TSubclassOf<UInventoryItemFragment>& FragmentClass) const;

	//Used in c++ function.
	template <typename T>
	const T* FindFragmentByClass() const;
};
