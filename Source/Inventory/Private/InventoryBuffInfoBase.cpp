// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryBuffInfoBase.h"

#include <string>

FText UInventoryBuffInfoBase::DescriptionOverride_Implementation(const float Value)
{
	return Description;
}
