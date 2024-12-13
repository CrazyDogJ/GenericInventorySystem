// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemActor_Base.h"
#include "ItemActor_Common.generated.h"

UCLASS()
class INVENTORY_API AItemActor_Common : public AItemActor_Base
{
	GENERATED_BODY()

public:
	AItemActor_Common();

	void InitComps(const FTransform& Transform);

	UFUNCTION(BlueprintPure)
	bool IsRuntimeActor() const;

#if WITH_EDITORONLY_DATA
	bool bIsSimulatingPhysicsInEditor = false;
#endif
	
	UFUNCTION(CallInEditor)
	void SimulatePhysics();
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UStaticMeshComponent* ItemStaticMeshComponent;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	USkeletalMeshComponent* ItemSkeletalMeshComponent;
	
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NativeOnItemPickedUp() override;
	virtual void OnRep_ItemID() override;
};
