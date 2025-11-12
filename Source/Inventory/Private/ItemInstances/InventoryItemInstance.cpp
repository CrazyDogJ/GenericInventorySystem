// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemInstances/InventoryItemInstance.h"

#include "InventoryItemDefinition.h"
#include "InventoryManagerComponent.h"
#include "Net/UnrealNetwork.h"

UInventoryItemInstance::UInventoryItemInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UWorld* UInventoryItemInstance::GetWorld() const
{
	if (auto OwningActor = Cast<AActor>(GetOuter()))
	{
		return OwningActor->GetWorld();
	}
	return nullptr;
}

void UInventoryItemInstance::Tick(float DeltaTime)
{
	if (GetWorld())
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

int32 UInventoryItemInstance::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	if (AActor* OuterActor = Cast<AActor>(GetOuter()))
	{
		return OuterActor ->GetFunctionCallspace(Function, Stack);
	}
 
	return FunctionCallspace::Local;
}

bool UInventoryItemInstance::CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms,
	FFrame* Stack)
{
	if (AActor* OuterActor = Cast<AActor>(GetOuter()))
	{
		if (UNetDriver* NetDriver = OuterActor->GetNetDriver())
		{
			NetDriver->ProcessRemoteFunction(OuterActor, Function, Parms, OutParms, Stack, this);
			return true;
		}
	}
	return false;
}

APawn* UInventoryItemInstance::GetPawn() const
{
	return Cast<APawn>(GetOuter());
}

UInventoryManagerComponent* UInventoryItemInstance::GetInventoryManager() const
{
	return Cast<UInventoryManagerComponent>(GetPawn()->GetComponentByClass(UInventoryManagerComponent::StaticClass()));
}

void UInventoryItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass());
	if (BPClass != NULL)
	{
		BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}
	
	DOREPLIFETIME(ThisClass, ItemDef);
	DOREPLIFETIME(ThisClass, Instigator);
}

const UInventoryItemFragment* UInventoryItemInstance::FindFragmentByClass(TSubclassOf<UInventoryItemFragment> FragmentClass) const
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		return ItemDef->FindFragmentByClass(FragmentClass);
	}

	return nullptr;
}

void UInventoryItemInstance::SetItemDef(const UInventoryItemDefinition* InDef)
{
	ItemDef = const_cast<UInventoryItemDefinition*>(InDef);
}

void UInventoryItemInstance::OnRep_Instigator()
{
	//TODO:Here is OnRep_Instigator
}
