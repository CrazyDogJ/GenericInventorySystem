// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InventoryBuffInfoBase.h"
#include "UObject/Object.h"
#include "InventorySettings.generated.h"

USTRUCT(BlueprintType)
struct FQualitySetting
{
	GENERATED_BODY()

	FQualitySetting()
	{
		QualityTag = FGameplayTag::EmptyTag;
		QualityColor = FLinearColor::Transparent;
		QualityName = FText();
	};
	
	FQualitySetting(const FGameplayTag Tag, const FLinearColor Color, const FText& Name);
	
public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Quality", meta = (Categories="InventoryQuality"))
	FGameplayTag QualityTag;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Quality")
	FLinearColor QualityColor;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Quality")
	FText QualityName;
};

USTRUCT(BlueprintType)
struct FInventoryGameplayTagSetting
{
	GENERATED_BODY()

	FInventoryGameplayTagSetting()
	{
		OtherGameplayTag = FGameplayTag::EmptyTag;
		OtherGameplayTagLocName = FText();
		OtherGameplayTagLocDesc = FText();
	};
	
	FInventoryGameplayTagSetting(const FGameplayTag Tag, const FText LocName, const FText LocDesc);

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	FGameplayTag OtherGameplayTag;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	FText OtherGameplayTagLocName;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	FText OtherGameplayTagLocDesc;
};

/**
 * 
 */
UCLASS(config = InventorySetting, DefaultConfig, NotPlaceable)
class INVENTORY_API UInventorySettings : public UObject
{
	GENERATED_BODY()

public:
	UInventorySettings(const FObjectInitializer& obj);

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FQualitySetting> QualitySettings;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta=(UIMin = 0, UIMax = 255, ClampMin = 0, ClampMax = 255))
	int32 CustomDepthStencil;
	
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TMap<FGameplayTag, TSoftClassPtr<UInventoryBuffInfoBase>> BuffInfos;
};
