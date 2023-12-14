// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "Containers/Map.h"
#include "Containers/Set.h"
#include "Containers/SparseArray.h"
#include "Containers/UnrealString.h"
#include "GameplayTagContainer.h"
#include "HAL/Platform.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Templates/UnrealTemplate.h"
#include "UObject/Class.h"

#include "GameplayTagStack.generated.h"

struct FGameplayTagStackContainer;
struct FNetDeltaSerializeInfo;

/**
 * Represents one stack of a gameplay tag (tag + count)
 */
USTRUCT(BlueprintType)
struct FGameplayTagStack : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FGameplayTagStack()
	{}

	FGameplayTagStack(FGameplayTag InTag, float InStackCount)
		: Tag(InTag)
		, TagFloatValue(InStackCount)
	{
	}

	FString GetDebugString() const;

private:
	friend FGameplayTagStackContainer;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	float TagFloatValue = 0;
};

/** Container of gameplay tag stacks */
USTRUCT(BlueprintType)
struct FGameplayTagStackContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	FGameplayTagStackContainer()
	//	: Owner(nullptr)
	{
	}

public:
	// Adds a specified number of stacks to the tag (does nothing if StackCount is below 1)
	void AddStack(FGameplayTag Tag, float StackCount);

	// Removes a specified number of stacks from the tag (does nothing if StackCount is below 1)
	void RemoveStack(FGameplayTag Tag, float StackCount);

	// Returns the stack count of the specified tag (or 0 if the tag is not present)
	float GetStackCount(FGameplayTag Tag) const
	{
		return TagToCountMap.FindRef(Tag);
	}

	TArray<FGameplayTagStack> GetTagStacks() const
	{
		return Stacks;
	}

	void SetStackTags(const TArray<FGameplayTagStack>& TagsMap);

	// Returns true if there is at least one stack of the specified tag
	bool ContainsTag(FGameplayTag Tag) const
	{
		return TagToCountMap.Contains(Tag);
	}

	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FGameplayTagStack, FGameplayTagStackContainer>(Stacks, DeltaParms, *this);
	}

private:
	// Replicated list of gameplay tag stacks
	UPROPERTY(SaveGame)
	TArray<FGameplayTagStack> Stacks;

	// Accelerated list of tag stacks for queries
	TMap<FGameplayTag, float> TagToCountMap;
};

template<>
struct TStructOpsTypeTraits<FGameplayTagStackContainer> : public TStructOpsTypeTraitsBase2<FGameplayTagStackContainer>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
