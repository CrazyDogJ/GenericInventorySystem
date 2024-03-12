// Fill out your copyright notice in the Description page of Project Settings.


#include "InventorySettings.h"

#include "GameplayTagsManager.h"

FQualitySetting::FQualitySetting(const FGameplayTag Tag, const FLinearColor Color, const FText& Name)
{
	QualityTag = Tag;
	QualityColor = Color;
	QualityName = Name;
}

FInventoryGameplayTagSetting::FInventoryGameplayTagSetting(const FGameplayTag Tag, const FText LocName,
	const FText LocDesc)
{
	OtherGameplayTag = Tag;
	OtherGameplayTagLocName = LocName;
	OtherGameplayTagLocDesc = LocDesc;
}

#define LOCTEXT_NAMESPACE "UInventorySettings"

UInventorySettings::UInventorySettings(const FObjectInitializer& obj)
{
	const FQualitySetting Common = MakeQualitySetting(TEXT("InventoryQuality.Common"), FLinearColor::Gray, LOCTEXT("InventoryQualitySettingCommon", "Common"));
	const FQualitySetting Good = MakeQualitySetting(TEXT("InventoryQuality.Good"), FLinearColor::Gray, LOCTEXT("InventoryQualitySettingGood", "Good"));
	const FQualitySetting Uncommon = MakeQualitySetting(TEXT("InventoryQuality.Uncommon"), FLinearColor::Gray, LOCTEXT("InventoryQualitySettingUncommon", "Uncommon"));
	
	QualitySettings = TArray<FQualitySetting>{Common, Good, Uncommon};
	CustomDepthStencil = 6;
}

FQualitySetting UInventorySettings::MakeQualitySetting(FName TagName, FLinearColor Color, FText Name)
{
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	Manager.AddNativeGameplayTag(TagName);
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName);
	return FQualitySetting(Tag, Color, Name);
}

#undef LOCTEXT_NAMESPACE
