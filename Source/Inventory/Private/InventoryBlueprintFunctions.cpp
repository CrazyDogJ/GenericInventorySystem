// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryBlueprintFunctions.h"

#include "InventoryBuffInfoBase.h"
#include "InventoryItemInstance_StatTags.h"

TArray<FQualitySetting> UInventoryBlueprintFunctions::GetQualitySettings()
{
	if (UInventorySettings* Settings = GetMutableDefault<UInventorySettings>())
	{
		return Settings->QualitySettings;
	}
	TArray<FQualitySetting> empty;
	return empty;
}

FLinearColor UInventoryBlueprintFunctions::GetQualityColorByGameplayTag(const FGameplayTag Tag)
{
	if (UInventorySettings* Settings = GetMutableDefault<UInventorySettings>())
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
	if (UInventorySettings* Settings = GetMutableDefault<UInventorySettings>())
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
	if (UInventorySettings* Settings = GetMutableDefault<UInventorySettings>())
	{
		return Settings->CustomDepthStencil;
	}

	return -1;
}

void UInventoryBlueprintFunctions::BeginBuff(UInventoryItemInstance_StatTags* Instance)
{
	check(Instance)
	
	if (UInventorySettings* Settings = GetMutableDefault<UInventorySettings>())
	{
		for (auto Tag : Instance->GetStatTags())
		{
			if (Settings->BuffInfos.Find(Tag.Tag))
			{
				const TSubclassOf<UInventoryBuffInfoBase> BuffClassPtr = Settings->BuffInfos.Find(Tag.Tag)->LoadSynchronous();
				BuffClassPtr.GetDefaultObject()->OnBuffBegin(Instance, Tag.TagFloatValue);
			}
		}
	}
}

void UInventoryBlueprintFunctions::EndBuff(UInventoryItemInstance_StatTags* Instance)
{
	check(Instance)
	
	if (UInventorySettings* Settings = GetMutableDefault<UInventorySettings>())
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
	if (UInventorySettings* Settings = GetMutableDefault<UInventorySettings>())
	{
		if (Settings->BuffInfos.Find(Tag.Tag))
		{
			const TSubclassOf<UInventoryBuffInfoBase> BuffClassPtr = Settings->BuffInfos.Find(Tag.Tag)->LoadSynchronous();
			return BuffClassPtr.GetDefaultObject()->DescriptionOverride(Tag.TagFloatValue);
		}
	}
	return FText();
}
