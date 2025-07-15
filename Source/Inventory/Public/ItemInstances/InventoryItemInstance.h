// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "InventoryItemInstance.generated.h"

class UInventoryItemDefinition;
class UInventoryManagerComponent;
struct FInventorySlot;

/**
 * Inventory Item Instance that can be blueprintable in order to make custom events.
 */
UCLASS(Blueprintable, EditInlineNew)
class INVENTORY_API UInventoryItemInstance : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// Construct
	UInventoryItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// UObject interface
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual UWorld* GetWorld() const override final;
	virtual bool ImplementsGetWorld() const override { return true; }
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;
	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms, FFrame* Stack) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	// End of UObject interface

	UFUNCTION(BlueprintPure)
	UObject* GetInstigator() const { return Instigator; }
	
	void SetInstigator(UObject* InInstigator) { Instigator = InInstigator; }
	
	UFUNCTION(BlueprintPure)
	APawn* GetPawn() const;

	UFUNCTION(BlueprintPure)
	UInventoryManagerComponent* GetInventoryManager() const;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag CategoryTag;
	
	virtual void OnInstanceCreated()
	{
		// Default as outer object.
		SetInstigator(GetOuter());
		K2_OnInstanceCreated();
	}
	
	virtual void OnInstanceDestroyed()
	{
		bUseTick = false;
		K2_OnInstanceDestroyed();
		ConditionalBeginDestroy();
	}

	virtual void OnCategoryChanged(const FGameplayTag& NewCategory)
	{
		if (NewCategory != CategoryTag)
		{
			auto PrevCategory = CategoryTag;
			CategoryTag = NewCategory;
			K2_OnCategoryChanged(PrevCategory, NewCategory);
		}
	}
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName="On Instance Created")
	void K2_OnInstanceCreated();

	UFUNCTION(BlueprintImplementableEvent, DisplayName="On Instance Destroyed")
	void K2_OnInstanceDestroyed();

	UFUNCTION(BlueprintImplementableEvent, DisplayName="On Pre Save Game")
	void K2_OnPreSaveGame();

	UFUNCTION(BlueprintImplementableEvent, DisplayName="On Pose Load Game")
	void K2_OnPostLoadGame();

	UFUNCTION(BlueprintImplementableEvent, DisplayName="On Category Changed")
	void K2_OnCategoryChanged(const FGameplayTag& PrevCategory, const FGameplayTag& NewCategory);
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName="Tick")
	void K2_Tick(float deltaTime);
#pragma region Public Properties

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Instance)
	bool bUseTick = false;
#pragma endregion 
	
	UFUNCTION(BlueprintPure)
	UInventoryItemDefinition* GetItemDef() const
	{
		return ItemDef;
	}
	
	UFUNCTION(BlueprintCallable, BlueprintPure=false, meta=(DeterminesOutputType=FragmentClass))
	const UInventoryItemFragment* FindFragmentByClass(TSubclassOf<UInventoryItemFragment> FragmentClass) const;

	void SetItemDef(const UInventoryItemDefinition* InDef);
private:
	UFUNCTION()
	void OnRep_Instigator();
	
	// The item definition
	UPROPERTY(Replicated)
	TObjectPtr<UInventoryItemDefinition> ItemDef;
	
	UPROPERTY(ReplicatedUsing=OnRep_Instigator)
	TObjectPtr<UObject> Instigator;
};
