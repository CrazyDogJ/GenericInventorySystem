// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryBlueprintFunctions.h"

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
