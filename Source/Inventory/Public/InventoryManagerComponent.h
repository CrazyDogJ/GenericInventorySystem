// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagStack.h"
#include "InventoryItemInstance.h"
#include "ItemActor_Base.h"
#include "Components/ActorComponent.h"
#include "Components/SphereComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InventoryManagerComponent.generated.h"

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

	TArray<UInventoryItemInstance*> GetAllItems() const;

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

	void GiveEmptySlots(int EmptySlotsAmount);

#pragma region CalculationFunction
	
	/**
	 * Find first empty slot index, if not found will return -1.
	 * @return First empty slot index
	 */
	int FindEmpty() const;

	/**
	 * Find the first stackable slot to stack the input item def, if not found, index will return -1.
	 * @param ItemDef Def that need to find.
	 * @param index Slot index that can stack.
	 * @param remainAmount Amount that found slot can stack.
	 */
	void FindStack(const TSubclassOf<UInventoryItemDefinition> ItemDef, int& index, int& remainAmount);

	/**
	 * Add item to inventory.
	 * @param ItemDef Def that need to add.
	 * @param Count Item amount to add.
	 * @param TagStackOverride If item instance is class of stat tags, override stat tags of the item.
	 * @param SlotIndex Can add in specific slot if not -1.
	 */
	int AddItem(TSubclassOf<UInventoryItemDefinition> ItemDef, int Count, const TArray<FGameplayTagStack> TagStackOverride, int SlotIndex = -1);

	FInventorySlot AddNewInstance(TSubclassOf<UInventoryItemDefinition> ItemDef, int StackAmount = 1) const;
	
	void RemoveItem(const int index, const int amount);

	bool ItemDefUsed(TSubclassOf<UInventoryItemDefinition> ItemDef, int Amount = 1); 

	void DragDropItem(int DragIndex, int DropIndex);
	
	int GetTotalItemAmount(TSubclassOf<UInventoryItemDefinition> ItemDef);
	
#pragma endregion CalculationFunction
	
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

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	int InventorySlotAmount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	int QuickBarAmount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pickup)
	int PickupSelectedID = -1;

	UPROPERTY(BlueprintReadOnly, Category = Pickup)
	TArray<TObjectPtr<AItemActor_Base>> OverlapedActorsPtrs;
	
	UPROPERTY(ReplicatedUsing = OnRep_SelectedQuickBarIndex, BlueprintReadWrite, Category = Inventory)
	int SelectedQuickBarIndex = 0;

	UPROPERTY(EditAnywhere, Category = Inventory)
	bool bDebugDraw = false;
	
	UPROPERTY(BlueprintReadOnly)
	UInventoryItemInstance* EquippedInstance;
	
	UFUNCTION()
	void OnRep_SelectedQuickBarIndex();

	UFUNCTION()
	void OnRep_List();
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	int AddItem(TArray<FGameplayTagStack> TagStackOverride, TSubclassOf<UInventoryItemDefinition> ItemDef, int Count = 1, int SlotIndex = -1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool ItemDefUsed(TSubclassOf<UInventoryItemDefinition> ItemDef, int Amount = 1);
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = Pickup)
	void PickUpItem(AItemActor_Base* ItemActor);

	UFUNCTION(BlueprintCallable, Category = Inventory)
	int ItemTotalAmount(TSubclassOf<UInventoryItemDefinition> ItemDef);
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = Inventory)
	void DropItem(int index, int amount);

	UFUNCTION(BlueprintCallable, Category = Inventory)
	int FindEmpty();

	UFUNCTION(BlueprintCallable, Server, Reliable, Category=Inventory)
	void SplitItem(int index, int amount);
	
	bool DropItemCheck(const TObjectPtr<UInventoryItemInstance> instancePtr, FVector& DropLocation) const;
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = Inventory)
	void RemoveItem(int index, int amount);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = Inventory)
	void CreateItemActorInFront(TSubclassOf<UInventoryItemDefinition> ItemDef, int count, FGameplayTagStackContainer TagStackContainer, FVector DropLocation);
	
	UFUNCTION(BlueprintCallable, Category = Pickup)
	void PickupSelectedIdChange(bool bUpOrDown);

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = Inventory)
	void DragDropItem(int DragIndex, int DropIndex);
	
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = Inventory)
	void ChangeQuickBarIndex_Server(int index);

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Unable To Drop Item")
	void K2_UnableToDropItem();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USphereComponent* SphereComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CanPickUpItemRadius = 80;

	// declare overlap begin function
	UFUNCTION()
	void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

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
