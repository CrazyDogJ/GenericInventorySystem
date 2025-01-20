// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagStack.h"
#include "ItemInstances/InventoryItemInstance.h"
#include "Crafting/InventoryItemRecipe.h"
#include "ItemActors/ItemActor_Base.h"
#include "Components/ActorComponent.h"
#include "Components/SphereComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "InventoryManagerComponent.generated.h"

UENUM(BlueprintType)
enum EArrayChangeType : uint8
{
	ChangeType_Added	UMETA(DisplayName = "Added"),
	ChangeType_Removed	UMETA(DisplayName = "Removed"),
	ChangeType_Changed	UMETA(DisplayName = "Changed")
};

struct FItemInstanceArchive : public FObjectAndNameAsStringProxyArchive
{
	FItemInstanceArchive(FArchive& InInnerArchive) : FObjectAndNameAsStringProxyArchive(InInnerArchive, true)
	{
		ArIsSaveGame = true;
	}
};

USTRUCT(BlueprintType)
struct FItemInstanceSaveData
{
	GENERATED_BODY()

	FItemInstanceSaveData()
	{}
	
	FItemInstanceSaveData(const TArray<uint8>& InInstanceData)
		: InstanceData(InInstanceData)
	{}
	
	UPROPERTY(VisibleAnywhere, SaveGame)
	TArray<uint8> InstanceData;
};

USTRUCT(BlueprintType)
struct FItemSlotSaveData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, SaveGame)
	TArray<FItemInstanceSaveData> InstancesData;

	UPROPERTY(BlueprintReadOnly, SaveGame, VisibleAnywhere)
	TObjectPtr<UInventoryItemDefinition> ItemDefinition;

	UPROPERTY(BlueprintReadOnly, SaveGame, VisibleAnywhere)
	int StackCount;
};

USTRUCT(BlueprintType)
struct FInventorySaveData
{
	GENERATED_BODY()
	
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TMap<FGameplayTag, int> SlotsAmount;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	int SelectedQuickBarIndex;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	TMap<int, FItemSlotSaveData> SlotDataMap;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	bool bIsValid = false;
	
	bool IsValid() const
	{
		return bIsValid;
	}
};

USTRUCT(BlueprintType)
struct FInventorySlot : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FInventorySlot()
	{}

	explicit FInventorySlot(const FGameplayTag CategoryID)
		: SlotCategoryTag(CategoryID)
	{}

private:
	friend UInventoryManagerComponent;
	
public:
	// Slot runtime variables
	UPROPERTY(BlueprintReadOnly)
	TArray<UInventoryItemInstance*> StackedInstances;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UInventoryItemDefinition> ItemDefinition = nullptr;

	UPROPERTY()
	int StackAmount = 0;

	// It will return instances num if item allow multiple item instances.
	int GetItemStackCount() const;
	
	// Slot category id setting
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag SlotCategoryTag;

	bool IsSlotEmpty() const;
	
	FString GetDebugString() const;

	/** Just set slot variables */
	void SwitchSlot(FInventorySlot& Slot);
};

USTRUCT(BlueprintType)
struct FInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()
	
	FInventoryList()
		: OwnerComponent(nullptr)
	{
	}

	FInventoryList(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

private:
	friend UInventoryManagerComponent;

	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;

public:
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FInventorySlot, FInventoryList>(Slots, DeltaParms, *this);
	}
	
#pragma region CalculationFunction

	void NotifyComponentListChanged(const FInventorySlot& Slot, const int Index, const TEnumAsByte<EArrayChangeType> ChangeType) const;
	void NotifyComponentListChanged(const TArrayView<int32>& Indices, const TEnumAsByte<EArrayChangeType> ChangeType);
	
	/** Empty the slot at given index. */
	void EmptySlotAt(int Index);

	/** Set item stack count at given index.
	 * If input == 0, it will empty this slot.
	 * If input > 0, it will create new default instance or simply add stack count.
	 * If input < 0, it will destroy the last instance or simply remove stack count.
	 */
	void SetItemStackCountAt(int Index, int InCount);

	/** Create a new instance.
	 * It will initialize the instance, so WE DON'T NEED TO INIT IT after calling this function! */
	UInventoryItemInstance* AddNewItemInstance(const UInventoryItemDefinition* ItemDef) const;

	/** Helper function to find last category index. */
	int FindCategoryLastItemIndex(const FGameplayTag SlotCategoryTag) const;
	
	/**
	 * Used to add empty slots to inventory list.
	 * @param EmptySlotsAmount The count that need to add slots.
	 * @param SlotCategoryTag The slot you want to be.
	 */
	void AddEmptySlots(const int& EmptySlotsAmount, const FGameplayTag& SlotCategoryTag);

	void AddEmptySlots(const TMap<FGameplayTag, int>& InitMap);

	TArray<int32> GetSlotsByCategory(const FGameplayTag& CategoryTag, const bool& MatchAll = false) const;
	
	/**
	 * Find first empty slot index, if not found will return -1.
	 * @return First empty slot index
	 * @param SlotCategoryTag The empty slot category id you want to find.
	 */
	int FindEmpty(const TArray<FGameplayTag>& SlotCategoryTag) const;

	/**
	 * Helper function. Find empty slot by item def.
	 * @return First empty slot index
	 * @param ItemDefinition The item def you want to find.
	 * @param MaxStackInThisSlot Max stack in this slot
	 */
	int FindEmptyForItemDef(const UInventoryItemDefinition* ItemDefinition, int& MaxStackInThisSlot) const;
	
	/**
	 * Find the first stackable slot to stack the input item def, if not found, index will return -1.
	 * @param ItemDef Def that need to find.
	 * @param Index Slot index that can stack.
	 * @param RemainAmount Amount that found slot can stack.
	 */
	void FindStack(const UInventoryItemDefinition* ItemDef, int& Index, int& RemainAmount) const;

	/** Stack instances to specific slot by index. */
	void StackInstances(const UInventoryItemDefinition* ItemDef, TArray<UInventoryItemInstance*>& InArray, const int SlotIndex, const int SplitAmount);
	
// FUCK!!! Take good care of these two AddItem functions. THEY ARE SO COMPLICATED!!!
	
	/**
	 * Automatically add item to inventory by item def and count.
	 * @param ItemDef Def that need to add.
	 * @param Count Item amount to add.
	 * @return Can't add item stack count.
	 */
	int AddItem(const UInventoryItemDefinition* ItemDef, int Count);

	/**
	 * Automatically add item to inventory by instance.
	 * @param ItemDef Item def the instance belong.
	 * @param Instances Item instance with parameters.
	 * @return Can't add item instances index(return 3 means {1, 2, 3, 4, 5} -> {3, 4, 5}).
	 */
	int AddItem(const UInventoryItemDefinition* ItemDef, TArray<UInventoryItemInstance*> Instances);

	// Helper function to get category array
	static TArray<FGameplayTag> GetItemDefCategoryArray(const UInventoryItemDefinition* ItemDef);
	
	// Set item at index.
	bool SetItemAt(UInventoryItemDefinition* ItemDef, int Count, const int& Index, bool bForceSet = false);
	
	// Deprecated : Create a new slot with instance.
	void AddNewInstance(FInventorySlot& Slot, const UInventoryItemDefinition* ItemDef, int StackAmount) const;

	bool ItemDefUsed(const UInventoryItemDefinition* ItemDef, int Amount = 1); 

// FUCK!!! This function is also complicated!!!!
	void DragDropItem(int DragIndex, int DropIndex);
	
	int GetTotalItemAmount(const UInventoryItemDefinition* ItemDef) const;
	
#pragma endregion CalculationFunction
	// Array content
	UPROPERTY(BlueprintReadOnly)
	TArray<FInventorySlot> Slots;
};

template<>
struct TStructOpsTypeTraits<FInventoryList> : public TStructOpsTypeTraitsBase2<FInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryListChanged);

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class INVENTORY_API UInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	// Sets default values for this component's properties
	UInventoryManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** Will add slots by category string. If value = 0, the keys will still be the priority to add slots.
	 * The priority will affect auto add items function!
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Inventory)
	TMap<FGameplayTag, int> InventorySlotAmount;

	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	int PickupSelectedID = -1;

	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	bool bInteractValid = false;
	
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TArray<TObjectPtr<AItemActor_Base>> OverlappedActorsPtrs;

	UPROPERTY(EditAnywhere, Category = Inventory)
	bool bDebugDraw = false;
	
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	UInventoryItemInstance* EquippedInstance;

	UPROPERTY(ReplicatedUsing = OnRep_KnownRecipes, BlueprintReadWrite, EditAnywhere, Category = Crafting)
	TArray<TSoftObjectPtr<UInventoryItemRecipe>> KnownRecipes;

	UFUNCTION(BlueprintImplementableEvent)
	void OnRep_KnownRecipes();
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddEmptySlots(const FGameplayTag& SlotCategoryTag, int Count = 1);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	int AddItem(UInventoryItemDefinition* ItemDef, int Count = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool ItemDefUsed(const UInventoryItemDefinition* ItemDef, int Amount = 1);

	UFUNCTION(BlueprintCallable, Category = Crafting)
	int RecipeCraftTimes(UInventoryItemRecipe* Recipe);
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = Inventory)
	void PickUpItem(AItemActor_Base* ItemActor);

	UFUNCTION(BlueprintCallable, Category = Crafting)
	bool CheckRecipeNeedItems(const UInventoryItemRecipe* Recipe);
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = Crafting)
	void CraftItem(const UInventoryItemRecipe* Recipe, int Times = 1);

	UFUNCTION(BlueprintCallable, Category = Inventory)
	int ItemTotalAmount(const UInventoryItemDefinition* ItemDef);
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = Inventory)
	void DropItem(int Index, int Amount);

	UFUNCTION(BlueprintCallable, Category = Inventory)
	int FindEmpty(const TArray<FGameplayTag>& CategoryTags) const;

	UFUNCTION(BlueprintCallable, Category = Inventory)
	bool CheckInventoryExchange(TMap<UInventoryItemDefinition*, int> OutItems, TMap<UInventoryItemDefinition*, int> InItems, int
	                            OutTimes = 1, int InTimes = 1);

	UFUNCTION(BlueprintCallable, Server, Reliable, Category=Inventory)
	void SplitItem(int Index, int Amount);
	
	bool DropItemCheck(const UInventoryItemDefinition* ItemDef, FVector& DropLocation) const;
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = Inventory)
	void RemoveItem(int Index, int Amount);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = Inventory)
	void CreateItemActorInFront(const FInventorySlot SlotToDrop, const FVector DropLocation);
	
	UFUNCTION(BlueprintCallable, Category = Inventory)
	void PickupSelectedIdChange(bool bUpOrDown);

	UFUNCTION(BlueprintPure, Category = Inventory)
	TArray<int32> GetSlotsByCategory(const FGameplayTag CategoryTag, const bool MatchAll) const;
	
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = Inventory)
	void DragDropItem(int DragIndex, int DropIndex);

	//UFUNCTION(BlueprintCallable, Server, Reliable, Category = Inventory)
	//void DragItemToContainer(UInventoryContainerComponent* Container, int DragIndex, int DropIndex);

#pragma region QuickBar
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Inventory)
	void ForceUnequipItem();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Inventory)
	void CancelForceUnequipItem();
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Inventory)
	bool bForceUnequipped;

	UFUNCTION(BlueprintCallable)
	void ChangeQuickBarIndex(const int& Index);

	UFUNCTION(Server, Reliable)
	void ChangeQuickBarIndex_Server(const int& Index);
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Inventory)
	int SelectedQuickBarIndex = 0;

	UFUNCTION(BlueprintCallable)
	void ChangeEquipmentItem(const UInventoryItemInstance* Instance);

	UFUNCTION(Server, Reliable)
	void ChangeEquipmentItem_Server(const UInventoryItemInstance* Instance);

	void ChangeEquipmentItemImplementation(const UInventoryItemInstance* Instance);
#pragma endregion QuickBar
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Unable To Drop Item")
	void K2_UnableToDropItem();

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Inventory List Changed")
	void K2_InventoryListChanged(const FInventorySlot& SlotPtr, int Index, EArrayChangeType ChangeType);
	
	UFUNCTION(BlueprintCallable,BlueprintAuthorityOnly)
	void ClearItems();
	
	//Translate to an object that save properties(used to save and load)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	FInventorySaveData GetSaveData();

	//Load save date(used to save and load)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool LoadSaveData(FInventorySaveData SaveData);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sphere Collision")
	USphereComponent* SphereComp;

	UPROPERTY(EditDefaultsOnly, Category = "Sphere Collision")
	TEnumAsByte<ECollisionChannel> SphereCollisionObjectType = ECC_WorldDynamic;
	
	UPROPERTY(EditDefaultsOnly, Category = "Sphere Collision")
	TMap<TEnumAsByte<ECollisionChannel>, TEnumAsByte<ECollisionResponse>> SphereCollisionResponses = {{ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap}};
	
	UPROPERTY(EditDefaultsOnly, Category = "Sphere Collision")
	float CanPickUpItemRadius = 80;

	UPROPERTY(EditDefaultsOnly, Category = "Sphere Collision")
	bool bUseSphereDetection = false;
	
	// declare overlap end function
	UFUNCTION()
	void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Blueprint Events
	UFUNCTION(BlueprintImplementableEvent)
	void OnItemBeginOverlap(AItemActor_Base* ItemActor);

	UFUNCTION(BlueprintImplementableEvent)
	void OnItemEndOverlap(AItemActor_Base* ItemActor);
	
	//~UObject interface
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	virtual void ReadyForReplication() override;
	//~End of UObject interface

	//Helper functions
public:
	
	void UnequipInstance();
	void EquipInstance(UInventoryItemInstance* ItemInstance);
	
	UPROPERTY(Replicated, BlueprintReadOnly, SaveGame)
	FInventoryList InventoryList;
};
