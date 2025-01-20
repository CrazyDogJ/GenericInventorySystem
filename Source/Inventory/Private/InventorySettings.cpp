// Fill out your copyright notice in the Description page of Project Settings.


#include "InventorySettings.h"

#include "GameplayTagsManager.h"
#include "ItemActors/ItemActor_Common.h"

#define LOCTEXT_NAMESPACE "UInventorySettings"

UInventorySettings::UInventorySettings(const FObjectInitializer& obj)
{
	// Quality
	QualitySettings.Add(MakeGameplayTag("Inventory.Quality.Common"), FQualitySetting(FLinearColor::Green, LOCTEXT("InventoryQualitySettingCommon", "Common")));
	QualitySettings.Add(MakeGameplayTag("Inventory.Quality.Good"), FQualitySetting(FLinearColor::Blue, LOCTEXT("InventoryQualitySettingGood", "Good")));
	QualitySettings.Add(MakeGameplayTag("Inventory.Quality.Uncommon"),FQualitySetting(FLinearColor(128,0,128), LOCTEXT("InventoryQualitySettingUncommon", "Uncommon")));

	CustomDepthStencil = 6;
	// Default category
	DefaultCategoryTag = MakeGameplayTag("Inventory.Category.Default");
	DefaultCategory = FItemCategory(FText::FromString("Default"), FText::FromString("These items are in Default Category"));
	// Quick bar category
	ItemCategories.Add(MakeGameplayTag("Inventory.Category.QuickBar"), FItemCategory(FText::FromString("Quick Bar"), FText::FromString("Quick bar category to store items")));
}

TSubclassOf<AItemActor_Common> UInventorySettings::GetDynamicItemActorClass() const
{
	return DynamicItemActorClass.TryLoadClass<AItemActor_Base>();
}

FGameplayTag UInventorySettings::MakeGameplayTag(const FName TagName)
{
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	Manager.AddNativeGameplayTag(TagName);
	return FGameplayTag::RequestGameplayTag(TagName);
}

#undef LOCTEXT_NAMESPACE
