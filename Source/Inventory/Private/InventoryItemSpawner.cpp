// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryItemSpawner.h"

#include "InventoryFragment_SkeletalMesh.h"
#include "InventoryFragment_StaticMesh.h"
#include "InventoryItemDefinition.h"


class UInventoryFragment_SkeletalMesh;
class UInventoryFragment_StaticMesh;
// Sets default values
AInventoryItemSpawner::AInventoryItemSpawner(const FObjectInitializer& ObjectInitializer)
	: AActor(ObjectInitializer)
{
	SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRootComponent);
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
#if WITH_EDITOR
	PreviewStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewStaticMesh"));
	PreviewStaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewStaticMesh->AttachToComponent(SceneRootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);
	PreviewSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewSkeletalMesh"));
	PreviewSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewSkeletalMesh->AttachToComponent(SceneRootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);
#endif
	
}

// Called when the game starts or when spawned
void AInventoryItemSpawner::BeginPlay()
{
#if WITH_EDITOR
	if (PreviewStaticMesh)
	{
		PreviewStaticMesh->DestroyComponent();
	}
	if (PreviewSkeletalMesh)
	{
		PreviewSkeletalMesh->DestroyComponent();
	}
#endif
	Super::BeginPlay();
}

void AInventoryItemSpawner::OnConstruction(const FTransform& Transform)
{
#if WITH_EDITOR
	if (ItemID != nullptr)
	{
		if (ItemID->GetDefaultObject<UInventoryItemDefinition>())
		{
			if (const UInventoryFragment_StaticMesh* StaticMeshFragment = Cast<UInventoryFragment_StaticMesh>(UInventoryFunctionLibrary::FindItemDefinitionFragment(ItemID, UInventoryFragment_StaticMesh::StaticClass())))
			{
				PreviewStaticMesh->SetVisibility(true);
				PreviewStaticMesh->SetStaticMesh(StaticMeshFragment->PickupStaticMesh);
				PreviewSkeletalMesh->SetSkeletalMesh(nullptr);
				PreviewSkeletalMesh->SetVisibility(false);
			}
			if (const UInventoryFragment_SkeletalMesh* SkeletalMeshFragment = Cast<UInventoryFragment_SkeletalMesh>(UInventoryFunctionLibrary::FindItemDefinitionFragment(ItemID, UInventoryFragment_SkeletalMesh::StaticClass())))
			{
				PreviewSkeletalMesh->SetVisibility(true);
				PreviewSkeletalMesh->SetSkeletalMesh(SkeletalMeshFragment->PickupSkeletalMesh);
				PreviewStaticMesh->SetStaticMesh(nullptr);
				PreviewStaticMesh->SetVisibility(false);
			}
		}
	}
#endif
	Super::OnConstruction(Transform);
}
