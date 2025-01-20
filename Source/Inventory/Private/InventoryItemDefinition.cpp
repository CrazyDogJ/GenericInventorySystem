// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemDefinition.h"

#include "InventorySettings.h"
#include "Fragments/InventoryFragment_SkeletalMesh.h"
#include "Fragments/InventoryFragment_StaticMesh.h"

UInventoryItemDefinition::UInventoryItemDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (auto GlobalSettings = GetMutableDefault<UInventorySettings>())
	{
		MaxStackAmountPerCategory.Add(GlobalSettings->DefaultCategoryTag, 1);
	}
}

int UInventoryItemDefinition::GetMaxStackAmount(const FGameplayTag CategoryTag) const
{
	auto Tag = CategoryTag;
	if (!Tag.IsValid())
	{
		if (auto Settings = GetMutableDefault<UInventorySettings>())
		{
			Tag = Settings->DefaultCategoryTag;
		}
	}

	for (auto Pair : MaxStackAmountPerCategory)
	{
		if (Tag.MatchesTag(Pair.Key))
		{
			return Pair.Value;
		}
	}
	
	return -1;
}

int UInventoryItemDefinition::GetMaxMaxStackAmount() const
{
	TArray<int> MaxStackAmountArray;
	MaxStackAmountPerCategory.GenerateValueArray(MaxStackAmountArray);
	int MaxNum = 0;
	for (auto Itr : MaxStackAmountArray)
	{
		MaxNum = FMath::Max(MaxNum, Itr);
	}
	return MaxNum;
}

void UInventoryItemDefinition::SetValidateMessage()
{
	FString Result;

	if (ItemID.IsEmpty())
	{
		Result += TEXT("x Item ID is empty, it will cause localization problem! \n");
	}

	if (DisplayName.IsEmpty())
	{
		Result += TEXT("x Item display name is empty! \n");
	}

	if (ItemDescription.IsEmpty())
	{
		Result += TEXT("x Item display description text is empty! \n");
	}

	if (MaxStackAmountPerCategory.IsEmpty())
	{
		Result += TEXT("x Item max stack settings is not valid! \n");
	}

	if (MeshType == MT_None)
	{
		Result += TEXT("x Item mesh is not valid! \n");
	}

	if ((MeshType == MT_StaticMesh && !Cast<UInventoryFragment_StaticMesh>(MeshSettings)) ||
		(MeshType == MT_SkeletalMesh && !Cast<UInventoryFragment_SkeletalMesh>(MeshSettings)) )
	{
		Result += TEXT("x Item mesh is not match, you can change mesh type to fix it! \n");
	}

	if (Result.IsEmpty())
	{
		Result = "* Item is perfect and valid!";
	}
	
	ValidateMessage = Result;
}

#if WITH_EDITOR

void UInventoryItemDefinition::PostLoad()
{
	Super::PostLoad();
	SetValidateMessage();
	if (ItemInstanceType == IIT_None)
	{
		DefaultItemInstance = nullptr;
	}
}

// Localization
void UInventoryItemDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);
	SetValidateMessage();
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	Modify();
	// Auto set mesh fragment
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UInventoryItemDefinition, MeshType))
	{
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
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UInventoryItemDefinition, ItemInstanceType))
	{
		if (ItemInstanceType == IIT_None)
		{
			DefaultItemInstance = nullptr;
		}
	}
	MarkPackageDirty();
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