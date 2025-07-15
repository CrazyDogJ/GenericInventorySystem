// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryManagerComp.generated.h"

class UInventoryItemDefBase;
class UInventoryItemInstanceBase;
class UInventoryManagerFragment;

UCLASS(meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORY_API UInventoryManagerComp : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UInventoryManagerComp();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	//~UObject interface
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	virtual void ReadyForReplication() override;
	//~End of UObject interface

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	TArray<TObjectPtr<UInventoryItemInstanceBase>> Instances;
	
	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Instanced)
	TArray<TObjectPtr<UInventoryManagerFragment>> ManagerFragments;

	UFUNCTION(BlueprintCallable)
	UInventoryItemInstanceBase* NewInstanceByItemDef(UInventoryItemDefBase* ItemDef);

	UFUNCTION(BlueprintCallable)
	void AddInstance(UInventoryItemInstanceBase* Instance);

	UFUNCTION(BlueprintCallable)
	void RemoveInstance(UInventoryItemInstanceBase* Instance);

	UFUNCTION(BlueprintCallable, BlueprintPure=false, meta=(DeterminesOutputType=FragmentClass))
	const UInventoryManagerFragment* FindFragmentByClass(TSubclassOf<UInventoryManagerFragment> FragmentClass) const;

	UFUNCTION(BlueprintCallable, BlueprintPure=false, meta=(DeterminesOutputType=FragmentClass))
	TArray<UInventoryManagerFragment*> FindFragmentsByClass(TSubclassOf<UInventoryManagerFragment> FragmentClass) const;
};
