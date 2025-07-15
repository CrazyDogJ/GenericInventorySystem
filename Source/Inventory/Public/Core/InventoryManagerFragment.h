// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InventoryManagerFragment.generated.h"

class UInventoryManagerComp;
/**
 * 
 */
UCLASS(DefaultToInstanced, EditInlineNew, Abstract, Blueprintable)
class INVENTORY_API UInventoryManagerFragment : public UObject
{
	GENERATED_BODY()

public:
	
	// UObject interface
	// World
	virtual bool ImplementsGetWorld() const override { return true; }
	// Network
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;
	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms, FFrame* Stack) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	// Subobjects
	virtual TArray<TObjectPtr<UObject>> CollectReplicatedSubobjects() {return TArray<TObjectPtr<UObject>>();}
	// End of UObject interface

	UFUNCTION(BlueprintPure)
	UInventoryManagerComp* GetInventoryManagerComp() const;
	
	UFUNCTION(BlueprintNativeEvent, DisplayName="Begin Play")
	void K2_BeginPlay();
	
	UFUNCTION(BlueprintNativeEvent, DisplayName="Tick")
	void K2_Tick(float DeltaSeconds);

	UFUNCTION(BlueprintNativeEvent, DisplayName="End Play")
	void K2_EndPlay(const EEndPlayReason::Type EndPlayReason);
};
