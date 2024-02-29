// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryItemInstance.h"
#include "GameFramework/Actor.h"
#include "ItemActor_Base.generated.h"

UCLASS()
class INVENTORY_API AItemActor_Base : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AItemActor_Base(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(ReplicatedUsing = OnRep_ItemID, EditAnywhere, BlueprintReadWrite, Category = "Inventory", SaveGame, Meta = (ExposeOnSpawn = true))
	TSubclassOf<UInventoryItemDefinition> ItemID;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = "1"), SaveGame, Meta = (ExposeOnSpawn = true))
	int Amount = 1;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, SaveGame, Meta = (ExposeOnSpawn = true))
	TArray<FGameplayTagStack> OverrideTagStack;

	UFUNCTION()
	virtual void OnRep_ItemID() {};
};
