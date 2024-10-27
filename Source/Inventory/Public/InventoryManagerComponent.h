// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagStack.h"
#include "InventoryContainerComponent.h"
#include "ItemInstances/InventoryItemInstance.h"
#include "Crafting/InventoryItemRecipe.h"
#include "ItemActors/ItemActor_Base.h"
#include "Components/ActorComponent.h"
#include "Components/SphereComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InventoryManagerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryListChanged);

USTRUCT(BlueprintType)
struct FInventorySaveData
{
	GENERATED_BODY()
	
	UPROPERTY(SaveGame, BlueprintReadOnly)
	int SlotsAmount;
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<TSubclassOf<UInventoryItemDefinition>> ItemID;
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<int> StackCount;
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<FGameplayTagStackContainer> StackTags;
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<int> SlotIndex;
	UPROPERTY(SaveGame, BlueprintReadOnly)
	int SelectedQuickBarIndex;

	bool IsValid() const
	{
		return SlotIndex.Num() > 0;
	}
};

USTRUCT(BlueprintType)
struct FInventorySlot : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FInventorySlot()
	{}

	FString GetDebugString() const;

private:
	friend UInventoryManagerComponent;

public:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UInventoryItemInstance> Instance = nullptr;

	UPROPERTY(BlueprintReadOnly)
	int StackCount = 0;

	FContainerSlot ToStruct() const;
};

USTRUCT(BlueprintType)
struct FInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

#pragma region CommonInterface
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
	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FInventorySlot, FInventoryList>(Slots, DeltaParms, *this);
	}
#pragma endregion
	
#pragma region CalculationFunction

	/**
	 * Used to add empty slots to inventory list.
	 * @param EmptySlotsAmount The count that need to add slots.
	 */
	void AddEmptySlots(int EmptySlotsAmount);
	
	/**
	 * Find first empty slot index, if not found will return -1.
	 * @return First empty slot index
	 */
	int FindEmpty() const;

	/**
	 * Find the first stackable slot to stack the input item def, if not found, index will return -1.
	 * @param ItemDef Def that need to find.
	 * @param Index Slot index that can stack.
	 * @param RemainAmount Amount that found slot can stack.
	 */
	void FindStack(const TSubclassOf<UInventoryItemDefinition>& ItemDef, int& Index, int& RemainAmount);

	/**
	 * Automatilly add item to inventory.
	 * @param ItemDef Def that need to add.
	 * @param Count Item amount to add.
	 * @param TagStackOverride If item instance is class of stat tags, override stat tags of the item.
	 */
	int AddItem(const TSubclassOf<UInventoryItemDefinition>& ItemDef, int Count, const TArray<FGameplayTagStack>& TagStackOverride);

	// Set item at index.
	void SetItemAt(const TSubclassOf<UInventoryItemDefinition>& ItemDef, int Count, const TArray<FGameplayTagStack>& TagStackOverride, const int& Index);
	
	// Create a new slot with instance.
	FInventorySlot AddNewInstance(const TSubclassOf<UInventoryItemDefinition>& ItemDef, int StackAmount = 1) const;

	// Remove item by amount at specific index.
	void RemoveItemAt(const int Index, const int Amount);

	bool ItemDefUsed(const TSubclassOf<UInventoryItemDefinition>& ItemDef, int Amount = 1); 

	void DragDropItem(int DragIndex, int DropIndex);
	
	int GetTotalItemAmount(const TSubclassOf<UInventoryItemDefinition>& ItemDef);
	
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Inventory)
	int InventorySlotAmount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Inventory)
	int QuickBarAmount = 3;

	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	int PickupSelectedID = -1;

	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	bool bInteractValid = false;
	
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TArray<TObjectPtr<AItemActor_Base>> OverlappedActorsPtrs;
	
	UPROPERTY(ReplicatedUsing = OnRep_SelectedQuickBarIndex, BlueprintReadOnly, Category = Inventory)
	int SelectedQuickBarIndex = 0;

	UPROPERTY(EditAnywhere, Category = Inventory)
	bool bDebugDraw = false;
	
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	UInventoryItemInstance* EquippedInstance;

	UPROPERTY(ReplicatedUsing = OnRep_KnownRecipes, BlueprintReadWrite, EditAnywhere, Category = Crafting)
	TArray<TSoftClassPtr<UInventoryItemRecipe>> KnownRecipes;
	
	UFUNCTION()
	void OnRep_SelectedQuickBarIndex();

	UFUNCTION(BlueprintImplementableEvent)
	void OnRep_KnownRecipes();

	UFUNCTION()
	void OnRep_List();

	UPROPERTY(BlueprintAssignable)
	FOnInventoryListChanged OnInventoryListChanged;
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	int AddItem(TArray<FGameplayTagStack> TagStackOverride, TSubclassOf<UInventoryItemDefinition> ItemDef, int Count = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool ItemDefUsed(TSubclassOf<UInventoryItemDefinition> ItemDef, int Amount = 1);

	UFUNCTION(BlueprintCallable, Category = Crafting)
	int RecipeCraftTimes(TSubclassOf<UInventoryItemRecipe> Recipe);
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = Inventory)
	void PickUpItem(AItemActor_Base* ItemActor);

	UFUNCTION(BlueprintCallable, Category = Crafting)
	bool CheckRecipeNeedItems(TSubclassOf<UInventoryItemRecipe> Recipe);
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = Crafting)
	void CraftItem(TSubclassOf<UInventoryItemRecipe> Recipe, int Times = 1);

	UFUNCTION(BlueprintCallable, Category = Inventory)
	int ItemTotalAmount(TSubclassOf<UInventoryItemDefinition> ItemDef);
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = Inventory)
	void DropItem(int Index, int Amount);

	UFUNCTION(BlueprintCallable, Category = Inventory)
	int FindEmpty();

	UFUNCTION(BlueprintCallable, Category = Inventory)
	bool CheckInventoryExchange(TMap<TSubclassOf<UInventoryItemDefinition>, int> OutItems, TMap<TSubclassOf<UInventoryItemDefinition>, int> InItems, int
	                            OutTimes = 1, int InTimes = 1);

	UFUNCTION(BlueprintCallable, Server, Reliable, Category=Inventory)
	void SplitItem(int Index, int Amount);
	
	bool DropItemCheck(const TObjectPtr<UInventoryItemInstance>& InstancePtr, FVector& DropLocation) const;
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = Inventory)
	void RemoveItem(int Index, int Amount);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = Inventory)
	void CreateItemActorInFront(TSubclassOf<UInventoryItemDefinition> ItemDef, int Count, FGameplayTagStackContainer TagStackContainer, FVector DropLocation);
	
	UFUNCTION(BlueprintCallable, Category = Inventory)
	void PickupSelectedIdChange(bool bUpOrDown);

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = Inventory)
	void DragDropItem(int DragIndex, int DropIndex);

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = Inventory)
	void DragItemToContainer(UInventoryContainerComponent* Container, int DragIndex, int DropIndex);
	
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = Inventory)
	void ChangeQuickBarIndex_Server(int Index);

	UFUNCTION(BlueprintCallable, NetMulticast, Reliable, Category = Inventory)
	void ForceUnequipItem();

	UPROPERTY(BlueprintReadOnly, Replicated, Category = Inventory)
	bool bForceUnequipped;
	
	UFUNCTION(BlueprintCallable, NetMulticast, Reliable, Category = Inventory)
	void CancelForceUnequipItem();

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Unable To Drop Item")
	void K2_UnableToDropItem();

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Inventory List Changed")
	void K2_InventoryListChanged();
	
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
	void EquipInstance(int SlotIndex);
	
	UPROPERTY(ReplicatedUsing = OnRep_List, BlueprintReadOnly, SaveGame)
	FInventoryList InventoryList;
};
