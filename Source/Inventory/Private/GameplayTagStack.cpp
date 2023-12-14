// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameplayTagStack.h"

#include "Logging/LogVerbosity.h"
#include "UObject/Stack.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameplayTagStack)

//////////////////////////////////////////////////////////////////////
// FGameplayTagStack

FString FGameplayTagStack::GetDebugString() const
{
	return FString::Printf(TEXT("%sx%f"), *Tag.ToString(), TagFloatValue);
}

//////////////////////////////////////////////////////////////////////
// FGameplayTagStackContainer

void FGameplayTagStackContainer::AddStack(FGameplayTag Tag, float StackCount)
{
	if (!Tag.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to AddStack"), ELogVerbosity::Warning);
		return;
	}

	if (StackCount > 0)
	{
		for (FGameplayTagStack& Stack : Stacks)
		{
			if (Stack.Tag == Tag)
			{
				const int32 NewCount = Stack.TagFloatValue + StackCount;
				Stack.TagFloatValue = NewCount;
				TagToCountMap[Tag] = NewCount;
				MarkItemDirty(Stack);
				return;
			}
		}

		FGameplayTagStack& NewStack = Stacks.Emplace_GetRef(Tag, StackCount);
		MarkItemDirty(NewStack);
		TagToCountMap.Add(Tag, StackCount);
	}
}

void FGameplayTagStackContainer::RemoveStack(FGameplayTag Tag, float StackCount)
{
	if (!Tag.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to RemoveStack"), ELogVerbosity::Warning);
		return;
	}

	//@TODO: Should we error if you try to remove a stack that doesn't exist or has a smaller count?
	if (StackCount > 0)
	{
		for (auto It = Stacks.CreateIterator(); It; ++It)
		{
			FGameplayTagStack& Stack = *It;
			if (Stack.Tag == Tag)
			{
				if (Stack.TagFloatValue <= StackCount)
				{
					It.RemoveCurrent();
					TagToCountMap.Remove(Tag);
					MarkArrayDirty();
				}
				else
				{
					const int32 NewCount = Stack.TagFloatValue - StackCount;
					Stack.TagFloatValue = NewCount;
					TagToCountMap[Tag] = NewCount;
					MarkItemDirty(Stack);
				}
				return;
			}
		}
	}
}

void FGameplayTagStackContainer::SetStackTags(const TArray<FGameplayTagStack>& TagsMap)
{
	Stacks = TagsMap;
	TagToCountMap.Empty();
	for (FGameplayTagStack& Stack : Stacks)
	{
		TagToCountMap.Add(Stack.Tag, Stack.TagFloatValue);
	}
	MarkArrayDirty();
}

void FGameplayTagStackContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (const int32 Index : RemovedIndices)
	{
		const FGameplayTag Tag = Stacks[Index].Tag;
		TagToCountMap.Remove(Tag);
	}
}

void FGameplayTagStackContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (const int32 Index : AddedIndices)
	{
		const FGameplayTagStack& Stack = Stacks[Index];
		TagToCountMap.Add(Stack.Tag, Stack.TagFloatValue);
	}
}

void FGameplayTagStackContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (const int32 Index : ChangedIndices)
	{
		const FGameplayTagStack& Stack = Stacks[Index];
		TagToCountMap[Stack.Tag] = Stack.TagFloatValue;
	}
}

