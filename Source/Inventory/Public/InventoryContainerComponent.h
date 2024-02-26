// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagStack.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InventoryContainerComponent.generated.h"

USTRUCT(BlueprintType)
struct FItemProbabilitySetting
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UInventoryItemDefinition> ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
	float ItemProbability;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int GenerateCountMin = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int GenerateCountMax = 2;
};

USTRUCT(BlueprintType)
struct FContainerSlot : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FContainerSlot()
	{}

public:
	UPROPERTY(BlueprintReadOnly, SaveGame)
	TSubclassOf<UInventoryItemDefinition> ItemID;

	UPROPERTY(BlueprintReadOnly, SaveGame)
	int StackCount = 0;

	UPROPERTY(BlueprintReadOnly, SaveGame)
	FGameplayTagStackContainer StackTagContainer;
};

USTRUCT(BlueprintType)
struct FContainerList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;

	FContainerList()
		: OwnerComponent(nullptr)
	{
	}

	FContainerList(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

public:
	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FContainerSlot, FContainerList>(Slots, DeltaParms, *this);
	}

	UPROPERTY(BlueprintReadOnly, SaveGame)
	TArray<FContainerSlot> Slots;

	void SetItem(TSubclassOf<UInventoryItemDefinition> ItemID, int Count, FGameplayTagStackContainer Tags, int SlotIndex);

	void InitializeList(int EmptySlotAmount);

	int FindEmpty();

	void FindStack(TSubclassOf<UInventoryItemDefinition> ItemDef, int& index, int& remainAmount);

	int AddItem(TSubclassOf<UInventoryItemDefinition> ItemDef, int Count, FGameplayTagStackContainer Tags, int SlotIndex = -1);

	void RemoveItem(int Count, int SlotIndex);
};

template<>
struct TStructOpsTypeTraits<FContainerList> : public TStructOpsTypeTraitsBase2<FContainerList>
{
	enum { WithNetDeltaSerializer = true };
};


UCLASS(BlueprintType, Blueprintable, ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class INVENTORY_API UInventoryContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UInventoryContainerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(Replicated, BlueprintReadOnly, SaveGame)
	FContainerList List;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FItemProbabilitySetting> GeneratedLootItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	bool bGenerateLoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	int EmptySlotAmount;

	UFUNCTION(BlueprintCallable)
	void SetItem(TSubclassOf<UInventoryItemDefinition> ItemID, int Count, FGameplayTagStackContainer Tags, int SlotIndex);

	UFUNCTION(BlueprintCallable)
	void Initialize();

	UFUNCTION(BlueprintCallable)
	int AddItem(TSubclassOf<UInventoryItemDefinition> ItemDef, int Count, FGameplayTagStackContainer Tags, int SlotIndex = -1);

	UFUNCTION(BlueprintCallable)
	void RemoveItem(int Count, int SlotIndex);

	UFUNCTION(BlueprintCallable)
	void GenerateLoot();

	virtual void BeginPlay() override;
};
