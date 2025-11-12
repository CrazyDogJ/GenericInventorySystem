#pragma once

#include "CoreMinimal.h"
#include "InventoryInstancedStructArray.h"
#include "UObject/Object.h"
#include "InventoryInstancedStructObject.generated.h"

class UInventoryInstancedStructInterface;
class UInventoryInstancedStructDef;

UCLASS(Blueprintable, EditInlineNew)
class INVENTORY_API UInventoryInstancedStructObject : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, Category = "Inventory", Replicated)
	UInventoryInstancedStructDef* InventoryItemDef;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Inventory", Replicated)
	FInventoryInstancedStructContainer RepInstancedStructContainer;
	
	// UObject interface
	// Network
	virtual bool IsSupportedForNetworking() const override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;
	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms, FFrame* Stack) override;
	virtual bool ImplementsGetWorld() const override;
	// End of UObject interface
	
	// Tick by component.
	UFUNCTION(BlueprintImplementableEvent)
	void Tick(float DeltaTime);
};
