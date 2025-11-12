#pragma once

#include "CoreMinimal.h"
#include "InventoryInstancedStructArray.h"
#include "Components/ActorComponent.h"
#include "InventoryInstancedStructComp.generated.h"

class UInventoryInstancedStructObject;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORY_API UInventoryInstancedStructComp : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryInstancedStructComp();
	
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite)
	FInventoryInstanceObjectContainer Instances;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	//~UObject interface
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	virtual void ReadyForReplication() override;
	//~End of UObject interface

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
