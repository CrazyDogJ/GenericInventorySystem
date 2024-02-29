// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemActor_Common.h"
#include "GameFramework/Actor.h"
#include "InventoryItemSpawner.generated.h"

UCLASS(Blueprintable)
class INVENTORY_API AInventoryItemSpawner : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AInventoryItemSpawner(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;

public:
	UPROPERTY(BlueprintReadOnly)
	USceneComponent* SceneRootComponent;
	
#if WITH_EDITORONLY_DATA
	UPROPERTY(BlueprintReadOnly)
	UStaticMeshComponent* PreviewStaticMesh;

	UPROPERTY(BlueprintReadOnly)
	USkeletalMeshComponent* PreviewSkeletalMesh;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TSubclassOf<UInventoryItemDefinition> ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = "1"))
	int Amount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGameplayTagStack> OverrideTagStack;
};
