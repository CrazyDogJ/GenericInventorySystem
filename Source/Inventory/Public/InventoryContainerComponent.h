// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryUtility.h"
#include "Components/ActorComponent.h"
#include "StructUtils/InstancedStruct.h"
#include "InventoryContainerComponent.generated.h"

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class INVENTORY_API UInventoryContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UInventoryContainerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
#pragma region Properties
	
public:
	/** Core of inventory container */
	UPROPERTY(Replicated, BlueprintReadOnly)
	FInventoryList InventoryList;
	
	/** Will add slots by category string. If value = 0, the keys will still be the priority to add slots.
	 * The priority will affect auto add items function!
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Inventory)
	TMap<FGameplayTag, int> InventorySlotAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,	Category = Inventory, meta = (GetAllowedClasses = "GetAllowedInventoryInstancedStruct", DisallowedClasses = "GetDisallowedInventoryInstancedStruct", ExcludeBaseStruct, ShowTreeView))
	FInstancedStruct TestInstancedStruct;
	
#pragma endregion

#pragma region Functions
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//~UObject interface
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	virtual void ReadyForReplication() override;
	//~End of UObject interface
	
public:

	UFUNCTION()
	TArray<TSoftObjectPtr<UScriptStruct>> GetAllowedInventoryInstancedStruct() const;

	UFUNCTION()
	TArray<TSoftObjectPtr<UScriptStruct>> GetDisallowedInventoryInstancedStruct() const;
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Inventory List Changed")
	void K2_InventoryListChanged(const FInventorySlot& SlotPtr, int Index, EArrayChangeType ChangeType);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddEmptySlots(const FGameplayTag& SlotCategoryTag, int Count = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	int AddItem(UInventoryItemDefinition* ItemDef, int Count = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool ItemDefUsed(const UInventoryItemDefinition* ItemDef, int Amount = 1);

	UFUNCTION(BlueprintCallable, Category = Inventory)
	int ItemTotalAmount(const UInventoryItemDefinition* ItemDef);

	UFUNCTION(BlueprintCallable, Category = Inventory)
	int FindEmpty(const TArray<FGameplayTag>& CategoryTags) const;

	UFUNCTION(BlueprintCallable, Category = Inventory)
	bool CheckInventoryExchange(TMap<UInventoryItemDefinition*, int> OutItems, TMap<UInventoryItemDefinition*, int> InItems,
								int OutTimes = 1, int InTimes = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void SplitItem(int Index, int Amount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Inventory)
	void RemoveItem(int Index, int Amount);

	UFUNCTION(BlueprintPure, Category = Inventory)
	TArray<int32> GetSlotsByCategory(const FGameplayTag CategoryTag, const bool MatchAll) const;
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Inventory)
	void DragDropItem(UInventoryContainerComponent* Comp, int DragIndex, int DropIndex);

	UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly)
	void ClearItems();
	
	//Translate to an object that save properties(used to save and load)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	FInventorySaveData GetSaveData();
	
	static void NotifyCategoryChanged(const FInventorySlot& Slot);

	//Load save date(used to save and load)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool LoadSaveData(FInventorySaveData SaveData);
#pragma endregion 
};
