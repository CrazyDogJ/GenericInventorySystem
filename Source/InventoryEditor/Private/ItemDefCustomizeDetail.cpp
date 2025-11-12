// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemDefCustomizeDetail.h"

#include "DetailLayoutBuilder.h"
#include "Core/InventoryInstancedStructObject.h"

TSharedRef<IDetailCustomization> FItemDefDetails::MakeInstance()
{
	return MakeShareable(new FItemDefDetails);
}

void FItemDefDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	const TSharedPtr<IPropertyHandle> Handle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UInventoryInstancedStructObject, RepInstancedStructContainer));
	Handle->MarkHiddenByCustomization();
}
