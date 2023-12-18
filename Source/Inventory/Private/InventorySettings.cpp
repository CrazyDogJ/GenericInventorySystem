// Fill out your copyright notice in the Description page of Project Settings.


#include "InventorySettings.h"

#include "GameplayTagsManager.h"

FQualitySetting::FQualitySetting(const FGameplayTag Tag, const FLinearColor Color, const FText& Name)
{
	QualityTag = Tag;
	QualityColor = Color;
	QualityName = Name;
}

#define LOCTEXT_NAMESPACE "UInventorySettings"

UInventorySettings::UInventorySettings(const FObjectInitializer& obj)
{
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	Manager.AddNativeGameplayTag(TEXT("InventoryQuality.Common"));
	const FGameplayTag commonTag = FGameplayTag::RequestGameplayTag(TEXT("InventoryQuality.Common"));
	const FQualitySetting Common = FQualitySetting(commonTag, FLinearColor::Gray, LOCTEXT("InventoryQualitySettingCommon", "Common"));
	QualitySettings = TArray<FQualitySetting>{Common};
}

#undef LOCTEXT_NAMESPACE