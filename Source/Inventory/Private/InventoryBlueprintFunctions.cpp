// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryBlueprintFunctions.h"

#include "InventoryBuffInfoBase.h"
#include "InventoryItemDefinition.h"
#include "ItemInstances/InventoryItemInstance_StatTags.h"

TArray<FQualitySetting> UInventoryBlueprintFunctions::GetQualitySettings()
{
	if (auto Settings = GetInventoryProjectSettings())
	{
		return Settings->QualitySettings;
	}
	TArray<FQualitySetting> empty;
	return empty;
}

FLinearColor UInventoryBlueprintFunctions::GetQualityColorByGameplayTag(const FGameplayTag Tag)
{
	if (auto Settings = GetInventoryProjectSettings())
	{
		for (auto setting : Settings->QualitySettings)
		{
			if (setting.QualityTag == Tag)
			{
				return setting.QualityColor;
			}
		}
	}
	return FLinearColor::Transparent;
}

FText UInventoryBlueprintFunctions::GetQualityNameByGameplayTag(const FGameplayTag Tag)
{
	if (auto Settings = GetInventoryProjectSettings())
	{
		for (auto setting : Settings->QualitySettings)
		{
			if (setting.QualityTag == Tag)
			{
				return setting.QualityName;
			}
		}
	}
	return FText::FromString("Not valid tag!");
}

bool UInventoryBlueprintFunctions::IsTextNumeric(const FText& inputText)
{
	return inputText.IsNumeric();
}

int32 UInventoryBlueprintFunctions::GetInventoryCustomDepthStencil()
{
	if (auto Settings = GetInventoryProjectSettings())
	{
		return Settings->CustomDepthStencil;
	}

	return -1;
}

void UInventoryBlueprintFunctions::BeginBuff(UInventoryItemInstance_StatTags* Instance)
{
	check(Instance)
	
	if (auto Settings = GetInventoryProjectSettings())
	{
		for (auto Tag : Instance->GetStatTags())
		{
			if (Settings->BuffInfos.Find(Tag.Tag))
			{
				const TSubclassOf<UInventoryBuffInfoBase> BuffClassPtr = Settings->BuffInfos.Find(Tag.Tag)->LoadSynchronous();
				BuffClassPtr.GetDefaultObject()->OnBuffBegin(Instance, Tag.TagFloatValue);
				return;
			}
		}
	}
}

void UInventoryBlueprintFunctions::EndBuff(UInventoryItemInstance_StatTags* Instance)
{
	check(Instance)
	
	if (auto Settings = GetInventoryProjectSettings())
	{
		for (auto Tag : Instance->GetStatTags())
		{
			if (Settings->BuffInfos.Find(Tag.Tag))
			{
				const TSubclassOf<UInventoryBuffInfoBase> BuffClassPtr = Settings->BuffInfos.Find(Tag.Tag)->LoadSynchronous();
				BuffClassPtr.GetDefaultObject()->OnBuffEnd(Instance, Tag.TagFloatValue);
			}
		}
	}
}

FText UInventoryBlueprintFunctions::GetDescriptionFromBuffObject(FGameplayTagStack Tag)
{
	if (auto Settings = GetInventoryProjectSettings())
	{
		if (Settings->BuffInfos.Find(Tag.Tag))
		{
			const TSubclassOf<UInventoryBuffInfoBase> BuffClassPtr = Settings->BuffInfos.Find(Tag.Tag)->LoadSynchronous();
			return BuffClassPtr.GetDefaultObject()->DescriptionOverride(Tag.TagFloatValue);
		}
	}
	return FText();
}

const UInventoryItemFragment* UInventoryBlueprintFunctions::FindItemDefinitionFragment(
	TSubclassOf<UInventoryItemDefinition> ItemDef, TSubclassOf<UInventoryItemFragment> FragmentClass)
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		return GetDefault<UInventoryItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
	}
	return nullptr;
}

TObjectPtr<UInventorySettings> UInventoryBlueprintFunctions::GetInventoryProjectSettings()
{
	return GetMutableDefault<UInventorySettings>();
}
