// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/InventoryInstancedStructObject.h"

#include "Net/UnrealNetwork.h"

bool UInventoryInstancedStructObject::IsSupportedForNetworking() const
{
	return true;
}

void UInventoryInstancedStructObject::GetLifetimeReplicatedProps(
	TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);

	UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass());
	if (BPClass != NULL)
	{
		BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}
	
	DOREPLIFETIME(ThisClass, RepInstancedStructContainer);
	DOREPLIFETIME(ThisClass, InventoryItemDef);
}

int32 UInventoryInstancedStructObject::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	if (const auto OuterManager = Cast<UActorComponent>(GetOuter()))
	{
		if (const auto OuterActor = OuterManager->GetOwner())
		{
			return OuterActor->GetFunctionCallspace(Function, Stack);
		}
	}
 
	return FunctionCallspace::Local;
}

bool UInventoryInstancedStructObject::CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms,
	FFrame* Stack)
{
	if (const auto OuterManager = Cast<UActorComponent>(GetOuter()))
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

bool UInventoryInstancedStructObject::ImplementsGetWorld() const
{
	return true;
}
