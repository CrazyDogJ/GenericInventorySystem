// Fill out your copyright notice in the Description page of Project Settings.


#include "InventorySlotArray.h"

bool FInventorySlotArray::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
	return FastArrayDeltaSerialize<FInventorySlotEntry, FInventorySlotArray>(Entries, DeltaParms, *this);
}
