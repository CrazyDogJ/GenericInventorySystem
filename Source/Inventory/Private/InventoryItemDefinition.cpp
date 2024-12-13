// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemDefinition.h"

#include "Fragments/InventoryFragment_SkeletalMesh.h"
#include "Fragments/InventoryFragment_StaticMesh.h"
#include "ItemInstances/InventoryItemInstance_StatTags.h"

UInventoryItemDefinition::UInventoryItemDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
// Localization
void UInventoryItemDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);
	
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	// Clamp max stack amount to 1 if this item is stat tags class or equipment class.
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UInventoryItemDefinition, ItemInstance) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UInventoryItemDefinition, MaxStackAmount))
	{
		if (Cast<UInventoryItemInstance_StatTags>(ItemInstance))
		{
			Modify();
			MaxStackAmount = 1;
		}
	}
	// Auto set mesh fragment
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UInventoryItemDefinition, MeshType))
	{
		Modify();
		switch (MeshType)
		{
		case MT_None:
			if (MeshSettings)
			{
				MeshSettings = nullptr;
			}
			break;
		case MT_StaticMesh:
			if (!Cast<UInventoryFragment_StaticMesh>(MeshSettings))
			{
				MeshSettings = NewObject<UInventoryFragment_StaticMesh>(this, UInventoryFragment_StaticMesh::StaticClass(), NAME_None, RF_Public | RF_Transactional);
			}
			break;
		case MT_SkeletalMesh:
			if (!Cast<UInventoryFragment_SkeletalMesh>(MeshSettings))
			{
				MeshSettings = NewObject<UInventoryFragment_SkeletalMesh>(this, UInventoryFragment_SkeletalMesh::StaticClass(), NAME_None, RF_Public | RF_Transactional);
			}
			break;
		default:;
		}
	}
	// Auto set item name and description.
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UInventoryItemDefinition, ItemID))
	{
		// 当 ItemID 发生变化时，更新 ItemName 和 ItemDescription
		FTextKey Name = FString::Printf(TEXT("IDN_%s"), *ItemID);
		FTextKey Desc = FString::Printf(TEXT("IDD_%s"), *ItemID);
#define LOCTEXT_NAMESPACE "Inventory"	
		DisplayName = DisplayName.ChangeKey(TEXT("Inventory"), Name, DisplayName);
		ItemDescription = ItemDescription.ChangeKey(TEXT("Inventory"), Desc, ItemDescription);
#undef LOCTEXT_NAMESPACE
	}
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