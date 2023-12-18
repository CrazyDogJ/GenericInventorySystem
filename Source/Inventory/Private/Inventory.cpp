// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inventory.h"

#include "InventorySettings.h"
#include "ISettingsModule.h"

#define LOCTEXT_NAMESPACE "FInventoryModule"

void FInventoryModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "Inventory", LOCTEXT("RuntimeSettingsName", "Inventory"), LOCTEXT("RuntimeSettingsDescription", "Configure Inventory"), GetMutableDefault<UInventorySettings>());
	}
}

void FInventoryModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "Inventory");
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FInventoryModule, Inventory)