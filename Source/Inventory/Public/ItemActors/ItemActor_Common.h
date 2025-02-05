// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemActor_Base.h"
#include "ItemActor_Common.generated.h"

UCLASS()
class INVENTORY_API AItemActor_Common : public AItemActor_Base
{
	GENERATED_BODY()

public:
	AItemActor_Common();

	void SetupActor(const FInventorySlot& Slot);

	void InitComps(const FTransform& Transform);

	UFUNCTION(BlueprintPure)
	bool IsRuntimeActor() const;

#if WITH_EDITORONLY_DATA
	bool bIsSimulatingPhysicsInEditor = false;
#endif

#if WITH_EDITOR
	// Useful to build a level with item actors.
	UFUNCTION(CallInEditor, Category = "Inventory|Editor Events")
	void SimulatePhysics();

	// Useful to refresh mesh when changing the item definition's mesh description.
	UFUNCTION(CallInEditor, Category = "Inventory|Editor Events")
	void RefreshMesh();
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnMeshReady(UPrimitiveComponent* MeshComp);
#endif
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Inventory", AdvancedDisplay)
	UStaticMeshComponent* ItemStaticMeshComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Inventory", AdvancedDisplay)
	USkeletalMeshComponent* ItemSkeletalMeshComponent = nullptr;
	
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NativeOnItemPickedUp() override;
	virtual void OnRep_ItemID() override;
};
