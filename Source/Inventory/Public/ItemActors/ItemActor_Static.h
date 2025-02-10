#pragma once

#include "CoreMinimal.h"
#include "ItemActor_Base.h"
#include "ItemActor_Static.generated.h"

UCLASS(Blueprintable)
class INVENTORY_API AItemActor_Static : public AItemActor_Base
{
	GENERATED_BODY()

public:
	AItemActor_Static();

	// Properties
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Inventory", AdvancedDisplay)
	USceneComponent* Root;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Inventory", AdvancedDisplay)
	UMeshComponent* MeshComponent;

#if WITH_EDITORONLY_DATA
	bool bIsSimulatingPhysicsInEditor = false;
#endif
	
	// Functions
	
#if WITH_EDITOR
	// Useful to build a level with item actors.
	UFUNCTION(CallInEditor, Category = "Inventory|Editor Events")
	void SimulatePhysics();

	// Useful to refresh mesh when changing the item definition's mesh description.
	UFUNCTION(CallInEditor, Category = "Inventory|Editor Events")
	void RefreshMesh();
#endif
	
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	//The same as common
	virtual void NativeOnItemPickedUp() override;
	virtual void OnRep_ItemID() override;
	
	void InitComps(UMeshComponent*& InMeshComponent, const FTransform& Transform);
};
