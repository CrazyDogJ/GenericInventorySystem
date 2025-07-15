// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/InventoryItemFragmentBase.h"

#include "Core/InventoryItemInstanceBase.h"
#include "Core/InventoryManagerComp.h"

UWorld* UInventoryItemFragmentBase::GetWorld() const
{
	return UObject::GetWorld();
}

void UInventoryItemFragmentBase::Tick(float DeltaTime)
{
	K2_Tick(DeltaTime);
}

bool UInventoryItemFragmentBase::IsTickable() const
{
	return bEnableTick;
}

TStatId UInventoryItemFragmentBase::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UInventoryItemFragmentBase, STATGROUP_Tickables);
}

int32 UInventoryItemFragmentBase::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	if (const auto OuterInstance = Cast<UInventoryItemInstanceBase>(GetOuter()))
	{
		if (const auto OuterManager = Cast<UInventoryManagerComp>(OuterInstance->GetOuter()))
		{
			if (const auto OuterActor = OuterManager->GetOwner())
			{
				return OuterActor->GetFunctionCallspace(Function, Stack);
			}
		}
	}
 
	return FunctionCallspace::Local;
}

bool UInventoryItemFragmentBase::CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms,
	FFrame* Stack)
{
	if (const auto OuterInstance = Cast<UInventoryItemInstanceBase>(GetOuter()))
	{
		if (const auto OuterManager = Cast<UInventoryManagerComp>(OuterInstance->GetOuter()))
		{
			if (const auto OuterActor = OuterManager->GetOwner())
			{
				if (UNetDriver* NetDriver = OuterActor->GetNetDriver())
				{
					NetDriver->ProcessRemoteFunction(OuterActor, Function, Parms, OutParms, Stack, this);
					return true;
				}
			}
		}
	}
	
	return false;
}

void UInventoryItemFragmentBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass());
	if (BPClass != NULL)
	{
		BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}
}
