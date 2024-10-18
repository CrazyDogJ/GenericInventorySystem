// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InventoryItemInstance.generated.h"

class UInventoryItemDefinition;
/**
 * Inventory Item Instance that can be blueprintable in order to make custom events.
 */
UCLASS(BlueprintType, Blueprintable)
class INVENTORY_API UInventoryItemInstance : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UInventoryItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UObject interface
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual UWorld* GetWorld() const override final;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	//~End of UObject interface

	UFUNCTION(BlueprintPure)
	UObject* GetInstigator() const { return Instigator; }

	void SetInstigator(UObject* InInstigator) { Instigator = InInstigator; }
	
	UFUNCTION(BlueprintPure)
	APawn* GetPawn() const;
	
	virtual void OnInstanceCreated()
	{
		// Default as outer object.
		SetInstigator(GetOuter());
		K2_OnInstanceCreated();
	}
	
	virtual void OnInstanceDestroyed()
	{
		bUseTick = false;
		K2_OnInstanceDestroyed();
	}
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName="On Instance Created")
	void K2_OnInstanceCreated();

	UFUNCTION(BlueprintImplementableEvent, DisplayName="On Instance Destroyed")
	void K2_OnInstanceDestroyed();

	UFUNCTION(BlueprintImplementableEvent, DisplayName="Tick")
	void K2_Tick(float deltaTime);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bUseTick = false;
	
	UFUNCTION(BlueprintPure)
	TSubclassOf<UInventoryItemDefinition> GetItemDef() const
	{
		return ItemDef;
	}
	
	UFUNCTION(BlueprintCallable, BlueprintPure=false, meta=(DeterminesOutputType=FragmentClass))
	const UInventoryItemFragment* FindFragmentByClass(TSubclassOf<UInventoryItemFragment> FragmentClass) const;

	void SetItemDef(const TSubclassOf<UInventoryItemDefinition>& InDef);

private:
	UFUNCTION()
	void OnRep_Instigator();
	
	// The item definition
	UPROPERTY(Replicated)
	TSubclassOf<UInventoryItemDefinition> ItemDef;
	
	UPROPERTY(ReplicatedUsing=OnRep_Instigator)
	TObjectPtr<UObject> Instigator;
};
