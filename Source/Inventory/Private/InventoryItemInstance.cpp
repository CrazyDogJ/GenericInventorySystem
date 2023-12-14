// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemInstance.h"

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
	else
	{
		return nullptr;
	}
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

void UInventoryItemInstance::SetItemDef(TSubclassOf<UInventoryItemDefinition> InDef)
{
	ItemDef = InDef;
}

void UInventoryItemInstance::OnRep_Instigator()
{
}
