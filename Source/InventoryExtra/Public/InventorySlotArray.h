// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventorySlotEntry.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"
#include "InventorySlotArray.generated.h"

USTRUCT(BlueprintType)
struct FInventorySlotArray : public FFastArraySerializer
{
	GENERATED_BODY()
	
	FInventorySlotArray()
		: OwnerComponent(nullptr)
	{
	}

	FInventorySlotArray(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

public:
	// Owner component
	UPROPERTY(NotReplicated, BlueprintReadOnly)
	TObjectPtr<UActorComponent> OwnerComponent;
	
	// Array content
	UPROPERTY(BlueprintReadWrite)
	TArray<FInventorySlotEntry> Entries;
	
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize) {}
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize) {}
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize) {}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);
};

template<>
struct TStructOpsTypeTraits<FInventorySlotArray> : public TStructOpsTypeTraitsBase2<FInventorySlotArray>
{
	enum { WithNetDeltaSerializer = true };
};
