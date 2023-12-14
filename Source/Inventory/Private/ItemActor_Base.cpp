// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemActor_Base.h"

#include "Net/UnrealNetwork.h"

AItemActor_Base::AItemActor_Base(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetReplicates(true);
	SetReplicatingMovement(true);
}

void AItemActor_Base::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AItemActor_Base, ItemID);
	DOREPLIFETIME(AItemActor_Base, Amount);
	DOREPLIFETIME(AItemActor_Base, OverrideTagStack);
}
