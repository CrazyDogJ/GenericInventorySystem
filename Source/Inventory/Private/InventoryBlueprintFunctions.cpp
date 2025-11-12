// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryBlueprintFunctions.h"

#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "InventoryBuffInfoBase.h"
#include "InventoryItemDefinition.h"
#include "ItemInstances/InventoryItemInstance_StatTags.h"

TMap<FGameplayTag, FQualitySetting> UInventoryBlueprintFunctions::GetQualitySettings()
{
	if (auto Settings = GetInventoryProjectSettings())
	{
		return Settings->QualitySettings;
	}
	TMap<FGameplayTag, FQualitySetting> Empty;
	return Empty;
}

FLinearColor UInventoryBlueprintFunctions::GetQualityColorByGameplayTag(const FGameplayTag Tag)
{
	if (auto Found = GetQualitySettings().Find(Tag))
	{
		return Found->QualityColor;
	}
	return FLinearColor::Transparent;
}

FText UInventoryBlueprintFunctions::GetQualityNameByGameplayTag(const FGameplayTag Tag)
{
	if (auto Found = GetQualitySettings().Find(Tag))
	{
		return Found->QualityName;
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
	UInventoryItemDefinition* ItemDef, TSubclassOf<UInventoryItemFragment> FragmentClass)
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		return ItemDef->FindFragmentByClass(FragmentClass);
	}
	return nullptr;
}

void UInventoryBlueprintFunctions::PressInputByTag(UAbilitySystemComponent* ASC, const FGameplayTag& InTag)
{
	if (!ASC)
	{
		return;
	}
	
	FScopedAbilityListLock ActiveScopeLock(*ASC);
	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTag(InTag))
		{
			if (Spec.Ability)
			{
				Spec.InputPressed = true;
				if (Spec.IsActive())
				{
					if (Spec.Ability->bReplicateInputDirectly && ASC->IsOwnerActorAuthoritative() == false)
					{
						ASC->ServerSetInputPressed(Spec.Handle);
					}

					ASC->AbilitySpecInputPressed(Spec);

PRAGMA_DISABLE_DEPRECATION_WARNINGS
					// Fixing this up to use the instance activation, but this function should be deprecated as it cannot work with InstancedPerExecution
					TArray<UGameplayAbility*> Instances = Spec.GetAbilityInstances();
					const FGameplayAbilityActivationInfo& ActivationInfo = Instances.IsEmpty() ? Spec.ActivationInfo : Instances.Last()->GetCurrentActivationInfoRef();
PRAGMA_ENABLE_DEPRECATION_WARNINGS
					ASC->InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, ActivationInfo.GetActivationPredictionKey());	
				}
				else
				{
					// Ability is not active, so try to activate it
					ASC->TryActivateAbility(Spec.Handle);
				}
			}
		}
	}
}

void UInventoryBlueprintFunctions::ReleaseInputByTag(UAbilitySystemComponent* ASC, const FGameplayTag& InTag)
{
	if (!ASC)
	{
		return;
	}
	
	FScopedAbilityListLock ActiveScopeLock(*ASC);
	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTag(InTag))
		{
			Spec.InputPressed = false;
			if (Spec.Ability && Spec.IsActive())
			{
				if (Spec.Ability->bReplicateInputDirectly && ASC->IsOwnerActorAuthoritative() == false)
				{
					ASC->ServerSetInputReleased(Spec.Handle);
				}

				ASC->AbilitySpecInputReleased(Spec);

PRAGMA_DISABLE_DEPRECATION_WARNINGS
				// Fixing this up to use the instance activation, but this function should be deprecated as it cannot work with InstancedPerExecution
				TArray<UGameplayAbility*> Instances = Spec.GetAbilityInstances();
				const FGameplayAbilityActivationInfo& ActivationInfo = Instances.IsEmpty() ? Spec.ActivationInfo : Instances.Last()->GetCurrentActivationInfoRef();
PRAGMA_ENABLE_DEPRECATION_WARNINGS
				ASC->InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, ActivationInfo.GetActivationPredictionKey());
			}
		}
	}
}

int UInventoryBlueprintFunctions::GetItemStackCount(const FInventorySlot& InSlot)
{
	return InSlot.GetItemStackCount();
}

bool UInventoryBlueprintFunctions::FindCategoryStruct(FGameplayTag InTag, FItemCategory& OutCategory)
{
	OutCategory = FItemCategory();
	if (auto Settings = GetInventoryProjectSettings())
	{
		TMap<FGameplayTag, FItemCategory> TotalCategories;
		TotalCategories.Add(Settings->DefaultCategoryTag, Settings->DefaultCategory);
		TotalCategories.Append(Settings->ItemCategories);
		if (auto Found = TotalCategories.Find(InTag))
		{
			OutCategory = *Found;
			return true;
		}
		OutCategory = Settings->DefaultCategory;
		return false;
	}
	return false;
}

TObjectPtr<UInventorySettings> UInventoryBlueprintFunctions::GetInventoryProjectSettings()
{
	return GetMutableDefault<UInventorySettings>();
}
