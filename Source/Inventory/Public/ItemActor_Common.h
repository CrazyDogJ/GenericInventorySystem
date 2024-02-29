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
	// Sets default values for this actor's properties
	AItemActor_Common(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* StaticMeshComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMeshComponent* SkeletalMeshComp;

	UPROPERTY()
	UPrimitiveComponent* PrimitiveRootComponent;
	
	virtual void OnRep_ItemID() override;

	void SetPawnCollisionChannel(bool bOverlaped) const;
	
	UFUNCTION(BlueprintCallable)
	void SetUp(const FTransform& Transform);
	
protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
