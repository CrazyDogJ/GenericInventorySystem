// Fill out your copyright notice in the Description page of Project Settings.


#include "InventorySubsystem.h"

#include "InventorySettings.h"
#include "WorldPartition/WorldPartitionLevelStreamingPolicy.h"
#include "WorldPartition/WorldPartitionSubsystem.h"

struct FWorldPartitionStreamingQuerySource;
class UWorldPartitionSubsystem;

void UInventorySubsystem::RegisterItemActorToUnloadedPool(AItemActor_Common* ItemActor)
{
	if (ItemActor)
	{
		const auto ItemActorInfo = FInventoryItemInfo(ItemActor->GetTransform(), ItemActor->ItemID, ItemActor->Amount, ItemActor->OverrideTagStack);
		NotLoadedItemActors.Add(ItemActorInfo);
	}
}

void UInventorySubsystem::RegisterItemActorToLoadedPool(AItemActor_Common* ItemActor)
{
	if (ItemActor)
	{
		LoadedItemActors.AddUnique(ItemActor);
	}
}

void UInventorySubsystem::ItemActorDestroyed(AItemActor_Common* ItemActor)
{
	if (ItemActor)
	{
		LoadedItemActors.Remove(ItemActor);
	}
}

void UInventorySubsystem::LoadData(TArray<FInventoryItemInfo> Data)
{
	NotLoadedItemActors = Data;
}

void UInventorySubsystem::ClearUnloadedItemActors()
{
	NotLoadedItemActors.Empty();
}

bool UInventorySubsystem::IsCurrentCellLoaded(const FVector& InLocation) const
{
	const auto WorldPartitionSubsystem = GetWorld()->GetSubsystem<UWorldPartitionSubsystem>();
	if (!WorldPartitionSubsystem)
	{
		// Assuming that all collisions are loaded if not using WorldPartition.
		return true;
	}

	TArray<FWorldPartitionStreamingQuerySource> QuerySources;
	FWorldPartitionStreamingQuerySource& QuerySource = QuerySources.Emplace_GetRef();
	QuerySource.bSpatialQuery = true;
	QuerySource.Location = InLocation;
	QuerySource.bUseGridLoadingRange = false;
	QuerySource.Radius = 1.f; // 1cm should be enough to know if grid is loaded at specific area
	QuerySource.bDataLayersOnly = false;

	// Execute query
	return WorldPartitionSubsystem->IsStreamingCompleted(EWorldPartitionRuntimeCellState::Activated, QuerySources, /*bExactState*/ false);
}

void UInventorySubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TArray<AItemActor_Common*> RemoveItemActors;
	
	for (auto Actor : LoadedItemActors)
	{
		if (!IsCurrentCellLoaded(Actor->GetActorLocation()))
		{
			RegisterItemActorToUnloadedPool(Actor);
			RemoveItemActors.Add(Actor);
		}
	}
	for (auto Ptr : RemoveItemActors)
	{
		Ptr->Destroy();
	}

	TArray<int32> IndexToRemove;
	//check which should spawn
	for (int32 Index = 0; Index < NotLoadedItemActors.Num(); ++Index)
	{
		auto Actor = NotLoadedItemActors[Index];
		if (IsCurrentCellLoaded(Actor.Transform.GetLocation()))
		{
			IndexToRemove.Add(Index);
		}
	}
	//begin spawn
	for (const int32 RemoveIndex : IndexToRemove)
	{
		if (NotLoadedItemActors.IsValidIndex(RemoveIndex))
		{
			auto Actor = NotLoadedItemActors[RemoveIndex];
			RegisterItemActorToLoadedPool(SpawnItemFromSaveData(Actor));
			NotLoadedItemActors.RemoveAtSwap(RemoveIndex, 1, false);
		}
	}
	NotLoadedItemActors.Shrink();
}

AItemActor_Common* UInventorySubsystem::SpawnItemFromSaveData(const FInventoryItemInfo& Info)
{
	FActorSpawnParameters spawnInfo;
	spawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined;
	UClass* Class = AItemActor_Common::StaticClass();
	if (const UInventorySettings* Settings = GetMutableDefault<UInventorySettings>())
	{
		if (Settings->ItemActorBP_Class)
		{
			Class = Settings->ItemActorBP_Class;
		}
	}
	
	if (auto ItemActor = GetWorld()->SpawnActorDeferred<AItemActor_Common>(Class, Info.Transform))
	{
		ItemActor->ItemID = Info.ItemID;
		ItemActor->Amount = Info.Amount;
		ItemActor->OverrideTagStack = Info.OverrideTagStack;
		ItemActor->FinishSpawning(Info.Transform);
		return ItemActor;
	}

	return nullptr;
}
