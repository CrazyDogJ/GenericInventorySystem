// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemInstances/InventoryItemInstance.h"

#include "InventoryItemDefinition.h"
#include "Net/UnrealNetwork.h"

UInventoryItemInstance::UInventoryItemInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UWorld* UInventoryItemInstance::GetWorld() const
{
	if (const APawn* OwningPawn = GetPawn())
	{
		return OwningPawn->GetWorld();
	}
	return nullptr;
}

void UInventoryItemInstance::Tick(float DeltaTime)
{
	if (!IsUnreachable() && GetWorld())
	{
		//native tick here
		K2_Tick(DeltaTime);
	}
}

bool UInventoryItemInstance::IsTickable() const
{
	return bUseTick;
}

TStatId UInventoryItemInstance::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UInventoryItemInstance, STATGROUP_Tickables);
}

APawn* UInventoryItemInstance::GetPawn() const
{
	return Cast<APawn>(GetOuter());
}

void UInventoryItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, ItemDef);
}

const UInventoryItemFragment* UInventoryItemInstance::FindFragmentByClass(TSubclassOf<UInventoryItemFragment> FragmentClass) const
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		return GetDefault<UInventoryItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
	}

	return nullptr;
}

void UInventoryItemInstance::SetItemDef(const TSubclassOf<UInventoryItemDefinition>& InDef)
{
	ItemDef = InDef;
}

void UInventoryItemInstance::OnRep_Instigator()
{
	//TODO:Here is OnRep_Instigator
}
