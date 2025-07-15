// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/InventoryManagerComp.h"

#include "Core/InventoryItemDefBase.h"
#include "Core/InventoryManagerFragment.h"
#include "Core/InventoryItemInstanceBase.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"

UInventoryManagerComp::UInventoryManagerComp()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInventoryManagerComp::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ManagerFragments);
}

bool UInventoryManagerComp::ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch,
	FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	
	// Manager frags
	for (const auto Frag : ManagerFragments)
	{
		if (Frag)
		{
			WroteSomething |= Channel->ReplicateSubobject(Frag, *Bunch, *RepFlags);
			for (auto Itr : Frag->CollectReplicatedSubobjects())
			{
				WroteSomething |= Channel->ReplicateSubobject(Itr, *Bunch, *RepFlags);
			}
		}
	}

	for (const auto Itr : Instances)
	{
		WroteSomething |= Channel->ReplicateSubobject(Itr, *Bunch, *RepFlags);
	}
	
	return WroteSomething;
}

void UInventoryManagerComp::ReadyForReplication()
{
	Super::ReadyForReplication();

	if (IsUsingRegisteredSubObjectList())
	{
		// Manager frags
		for (const auto Frag : ManagerFragments)
		{
			if (Frag)
			{
				AddReplicatedSubObject(Frag);
				for (auto Itr : Frag->CollectReplicatedSubobjects())
				{
					AddReplicatedSubObject(Itr);
				}
			}
		}

		for (const auto Itr : Instances)
		{
			AddReplicatedSubObject(Itr);
		}
	}
}

void UInventoryManagerComp::BeginPlay()
{
	Super::BeginPlay();

	for (auto Frag : ManagerFragments)
	{
		Frag->K2_BeginPlay();
	}
}

void UInventoryManagerComp::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                          FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (auto Frag : ManagerFragments)
	{
		Frag->K2_Tick(DeltaTime);
	}
}

void UInventoryManagerComp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	for (auto Frag : ManagerFragments)
	{
		Frag->K2_EndPlay(EndPlayReason);
	}
}

UInventoryItemInstanceBase* UInventoryManagerComp::NewInstanceByItemDef(UInventoryItemDefBase* ItemDef)
{
	return ItemDef->NewInstanceObject(this);
}

void UInventoryManagerComp::RemoveInstance(UInventoryItemInstanceBase* Instance)
{
	Instances.Remove(Instance);
}

void UInventoryManagerComp::AddInstance(UInventoryItemInstanceBase* Instance)
{
	Instances.Add(Instance);
}

const UInventoryManagerFragment* UInventoryManagerComp::FindFragmentByClass(
	TSubclassOf<UInventoryManagerFragment> FragmentClass) const
{
	if (!FragmentClass || ManagerFragments.IsEmpty())
	{
		return nullptr;
	}

	const auto Result = ManagerFragments.FindByPredicate([FragmentClass](const TObjectPtr<UInventoryManagerFragment>& Check)
	{
		if (Check && Check->IsA(FragmentClass))
		{
			return true;
		}
		return false;
	});

	if (Result)
	{
		return *Result;
	}
	
	return nullptr;
}

TArray<UInventoryManagerFragment*> UInventoryManagerComp::FindFragmentsByClass(
	TSubclassOf<UInventoryManagerFragment> FragmentClass) const
{
	TArray<UInventoryManagerFragment*> Result = ManagerFragments.FilterByPredicate([FragmentClass](const TObjectPtr<UInventoryManagerFragment>& Frag)
	{
		if (Frag && Frag->IsA(FragmentClass))
		{
			return true;
		}
		return false;
	});
	
	return Result;
}
