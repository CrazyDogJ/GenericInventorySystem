// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/InventoryManagerFragment.h"

#include "Core/InventoryManagerComp.h"

int32 UInventoryManagerFragment::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	if (const auto OuterManager = Cast<UInventoryManagerComp>(GetOuter()))
	{
		if (const auto OuterActor = OuterManager->GetOwner())
		{
			return OuterActor->GetFunctionCallspace(Function, Stack);
		}
	}
 
	return FunctionCallspace::Local;
}

bool UInventoryManagerFragment::CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms,
	FFrame* Stack)
{
	if (const auto OuterManager = Cast<UInventoryManagerComp>(GetOuter()))
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
	
	return false;
}

void UInventoryManagerFragment::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);

	UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass());
	if (BPClass != NULL)
	{
		BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}
}

UInventoryManagerComp* UInventoryManagerFragment::GetInventoryManagerComp() const
{
	return Cast<UInventoryManagerComp>(GetOuter());
}

void UInventoryManagerFragment::K2_EndPlay_Implementation(const EEndPlayReason::Type EndPlayReason)
{
}

void UInventoryManagerFragment::K2_Tick_Implementation(float DeltaSeconds)
{
}

void UInventoryManagerFragment::K2_BeginPlay_Implementation()
{
}
