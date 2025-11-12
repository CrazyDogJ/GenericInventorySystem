#include "Core/InventoryInstancedStructComp.h"

#include "Core/InventoryInstancedStructObject.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"

UInventoryInstancedStructComp::UInventoryInstancedStructComp()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicated(true);
}

void UInventoryInstancedStructComp::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, Instances);
}

bool UInventoryInstancedStructComp::ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch,
	FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	
	for (const auto Itr : Instances.Instances)
	{
		if (Itr.InstanceObject)
		{
			WroteSomething |= Channel->ReplicateSubobject(Itr.InstanceObject, *Bunch, *RepFlags);
		}
	}
	
	return WroteSomething;
}

void UInventoryInstancedStructComp::ReadyForReplication()
{
	Super::ReadyForReplication();

	if (IsUsingRegisteredSubObjectList())
	{
		for (const auto Itr : Instances.Instances)
		{
			if (Itr.InstanceObject)
			{
				AddReplicatedSubObject(Itr.InstanceObject);
			}
		}
	}
}

void UInventoryInstancedStructComp::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (const auto Itr : Instances.Instances)
	{
		if (Itr.InstanceObject)
		{
			Itr.InstanceObject->Tick(DeltaTime);
		}
	}
}
