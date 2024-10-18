// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemDefinition.h"

UInventoryItemDefinition::UInventoryItemDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
// Localization
void UInventoryItemDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);

#define LOCTEXT_NAMESPACE "Inventory"
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UInventoryItemDefinition, ItemID))
	{
		// 当 ItemID 发生变化时，更新 ItemName 和 ItemDescription
		FTextKey Name = FString::Printf(TEXT("IDN_%s"), *ItemID);
		FTextKey Desc = FString::Printf(TEXT("IDD_%s"), *ItemID);
		DisplayName = DisplayName.ChangeKey(TEXT("Inventory"), Name, DisplayName);
		ItemDescription = ItemDescription.ChangeKey(TEXT("Inventory"), Desc, ItemDescription);
	}
#undef LOCTEXT_NAMESPACE
}
#endif

const UInventoryItemFragment* UInventoryItemDefinition::FindFragmentByClass(
	const TSubclassOf<UInventoryItemFragment>& FragmentClass) const
{
	if (FragmentClass != nullptr)
	{
		for (const UInventoryItemFragment* Fragment : Fragments)
		{
			if (Fragment && Fragment->IsA(FragmentClass))
			{
				return Fragment;
			}
		}
	}

	return nullptr;
}

template <typename T>
const T* UInventoryItemDefinition::FindFragmentByClass() const
{
	static_assert(TIsDerivedFrom<T, UInventoryItemFragment>::IsDerived, "T must be derived from UMyClass");
	
	for (const UInventoryItemFragment* Fragment : Fragments)
	{
		if (auto Out = Cast<T>(Fragment))
		{
			return Out;
		}
	}

	return nullptr;
}