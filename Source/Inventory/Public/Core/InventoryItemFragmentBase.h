// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InventoryItemFragmentBase.generated.h"

class UInventoryItemInstanceBase;

/** Represents a fragment of an item definition */
UCLASS(DefaultToInstanced, EditInlineNew, Abstract, Blueprintable)
class INVENTORY_API UInventoryItemFragmentBase : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(UInventoryItemInstanceBase* Instance) {OnFragmentCreated();}

	// UObject interface
	// World
	virtual UWorld* GetWorld() const override final;
	virtual bool ImplementsGetWorld() const override { return true; }
	// Tick
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	// Network
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;
	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms, FFrame* Stack) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	// End of UObject interface

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRuntime = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bEnableTick = false;

	UFUNCTION(BlueprintImplementableEvent)
	void OnFragmentCreated();
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName=Tick)
	void K2_Tick(float DeltaTime);
};
