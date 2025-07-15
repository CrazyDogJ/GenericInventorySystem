// Fill out your copyright notice in the Description page of Project Settings.


#include "InventorySlotEntry.h"

bool FInventorySlotEntry::IsSlotEmpty() const
{
	return ItemDefinition == nullptr && ItemInstance == nullptr && StackCount == 0;
}

void FInventorySlotEntry::ClearSlot()
{
	ItemDefinition = nullptr;
	// TODO : Destroy instance here!
	ItemInstance = nullptr;
	StackCount = 0;
}
