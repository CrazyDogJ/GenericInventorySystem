// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemDefinition.h"
#include "ItemActor_Base.h"
#include "ItemActor_Common.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "InventorySubsystem.generated.h"

USTRUCT(BlueprintType)
struct FInventoryItemInfo
{
	GENERATED_BODY()

	FInventoryItemInfo()
	{
	};

	FInventoryItemInfo(FTransform InTransform, TSubclassOf<UInventoryItemDefinition> InItemID, int InAmount, TArray<FGameplayTagStack> InOverrideTagStack)
	{
		Transform = InTransform;
		ItemID = InItemID;
		Amount = InAmount;
		OverrideTagStack = InOverrideTagStack;
	};
	
public:
	UPROPERTY(BlueprintReadOnly, SaveGame)
	FTransform Transform;
	
	UPROPERTY(BlueprintReadOnly, SaveGame)
	TSubclassOf<UInventoryItemDefinition> ItemID;

	UPROPERTY(BlueprintReadOnly, SaveGame)
	int Amount;

	UPROPERTY(BlueprintReadOnly, SaveGame)
	TArray<FGameplayTagStack> OverrideTagStack;
};

/**
 * 
 */
UCLASS()
class INVENTORY_API UInventorySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void RegisterItemActorToUnloadedPool(AItemActor_Common* ItemActor);

	UFUNCTION(BlueprintCallable)
	void RegisterItemActorToLoadedPool(AItemActor_Common* ItemActor);

	UFUNCTION(BlueprintCallable)
	void ItemActorDestroyed(AItemActor_Common* ItemActor);

	UFUNCTION(BlueprintCallable)
	void LoadData(TArray<FInventoryItemInfo> Data);

	/**
	 * Used to refresh items out of loaded area.
	 */
	UFUNCTION(BlueprintCallable)
	void ClearUnloadedItemActors();
	
	UFUNCTION(BlueprintPure)
	TArray<FInventoryItemInfo> GetUnloadedItemActors()
	{
		return NotLoadedItemActors;
	};
	
	bool IsCurrentCellLoaded(const FVector& InLocation) const;
	
	virtual TStatId GetStatId() const override
	{
		return GetStatID();
	}

	virtual void Tick(float DeltaTime) override;
	
protected:
	TArray<FInventoryItemInfo> NotLoadedItemActors;

	TArray<TObjectPtr<AItemActor_Common>> LoadedItemActors;

	AItemActor_Common* SpawnItemFromSaveData(const FInventoryItemInfo& Info);
};
