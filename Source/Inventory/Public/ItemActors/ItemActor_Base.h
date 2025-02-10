// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemInstances/InventoryItemInstance.h"
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
	UInventoryItemDefinition* ItemID;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = "1"), SaveGame, Meta = (ExposeOnSpawn = true))
	int Amount = 1;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	bool bUseDefaultInstance = true;
	
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Inventory", Meta = (ExposeOnSpawn = true, EditCondition = "bUseDefaultInstance == false", EditConditionHides), Instanced)
	TArray<UInventoryItemInstance*> ItemInstances;
	
	// When client get this actor's item definition, set up actor with infos in ID.
	UFUNCTION()
	virtual void OnRep_ItemID() {};
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnItemPickedUp();

	UFUNCTION(BlueprintImplementableEvent)
	void OnMeshReady(UPrimitiveComponent* MeshComp);
	
	// Need to implement for individual usage.
	virtual void NativeOnItemPickedUp() {OnItemPickedUp();}
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	virtual void BeginReplication() override;
	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	// Useful to reset item instance to default.
	UFUNCTION(CallInEditor, Category = "Inventory|Editor Events")
	void RefreshItemInstance();
#endif
	
};
