// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryContainerComponent.h"
#include "ItemInstances/InventoryItemInstance.h"
#include "Crafting/InventoryItemRecipe.h"
#include "ItemActors/ItemActor_Base.h"
#include "Components/ActorComponent.h"
#include "Components/SphereComponent.h"
#include "InventoryManagerComponent.generated.h"

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class INVENTORY_API UInventoryManagerComponent : public UInventoryContainerComponent
{
	GENERATED_BODY()

public:
	
	// Sets default values for this component's properties
	UInventoryManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
#pragma region Properties
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	int PickupSelectedID = -1;

	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	bool bInteractValid = false;
	
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TArray<TObjectPtr<AItemActor_Base>> OverlappedActors;

	UPROPERTY(EditAnywhere, Category = Inventory)
	bool bDebugDraw = false;
	
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	UInventoryItemInstance* EquippedInstance;

	UPROPERTY(ReplicatedUsing = OnRep_KnownRecipes, BlueprintReadWrite, EditAnywhere, Category = Crafting)
	TArray<TSoftObjectPtr<UInventoryItemRecipe>> KnownRecipes;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sphere Collision")
	USphereComponent* SphereComp;

	UPROPERTY(EditDefaultsOnly, Category = "Sphere Collision")
	TEnumAsByte<ECollisionChannel> SphereCollisionObjectType = ECC_WorldDynamic;
	
	UPROPERTY(EditDefaultsOnly, Category = "Sphere Collision")
	TMap<TEnumAsByte<ECollisionChannel>, TEnumAsByte<ECollisionResponse>> SphereCollisionResponses = {{ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap}};
	
	UPROPERTY(EditDefaultsOnly, Category = "Sphere Collision")
	float CanPickUpItemRadius = 80;

	UPROPERTY(EditDefaultsOnly, Category = "Sphere Collision")
	bool bUseSphereDetection = false;
	
#pragma endregion

#pragma region Functions
	UFUNCTION(BlueprintImplementableEvent)
	void OnRep_KnownRecipes();

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Unable To Drop Item")
	void K2_UnableToDropItem();
	
	UFUNCTION(BlueprintCallable, Category = Crafting)
	int RecipeCraftTimes(UInventoryItemRecipe* Recipe);
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = Inventory)
	void PickUpItem(AItemActor_Base* ItemActor);

	UFUNCTION(BlueprintCallable, Category = Crafting)
	bool CheckRecipeNeedItems(const UInventoryItemRecipe* Recipe);
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = Crafting)
	void CraftItem(const UInventoryItemRecipe* Recipe, int Times = 1);
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = Inventory)
	void DropItem(UInventoryContainerComponent* ContainerComponent, int Index, int Amount);
	
	bool DropItemCheck(const UInventoryItemDefinition* ItemDef, FVector& DropLocation) const;

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = Inventory)
	void CreateItemActorInFront(const FInventorySlot SlotToDrop, const FVector DropLocation);
	
	UFUNCTION(BlueprintCallable, Category = Inventory)
	void PickupSelectedIdChange(bool bUpOrDown);

	// declare overlap end function
	UFUNCTION()
	void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Blueprint Events
	UFUNCTION(BlueprintImplementableEvent)
	void OnItemBeginOverlap(AItemActor_Base* ItemActor);

	UFUNCTION(BlueprintImplementableEvent)
	void OnItemEndOverlap(AItemActor_Base* ItemActor);
	
	void UnequipInstance();
	void EquipInstance(UInventoryItemInstance* ItemInstance);
	
	//UFUNCTION(BlueprintCallable, Server, Reliable, Category = Inventory)
	//void DragItemToContainer(UInventoryContainerComponent* Container, int DragIndex, int DropIndex);
#pragma endregion 

#pragma region QuickBar
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Inventory)
	void ForceUnequipItem();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Inventory)
	void CancelForceUnequipItem();
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Inventory)
	bool bForceUnequipped;

	UFUNCTION(BlueprintCallable)
	void ChangeQuickBarIndex(const int& Index);

	UFUNCTION(Server, Reliable)
	void ChangeQuickBarIndex_Server(const int& Index);
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Inventory)
	int SelectedQuickBarIndex = 0;

	UFUNCTION(BlueprintCallable)
	void ChangeEquipmentItem(const UInventoryItemInstance* Instance);

	UFUNCTION(Server, Reliable)
	void ChangeEquipmentItem_Server(const UInventoryItemInstance* Instance);

	void ChangeEquipmentItemImplementation(const UInventoryItemInstance* Instance);
#pragma endregion QuickBar
	
};
