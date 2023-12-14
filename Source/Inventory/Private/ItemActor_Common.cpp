// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemActor_Common.h"

#include "InventoryFragment_SkeletalMesh.h"
#include "InventoryFragment_StaticMesh.h"


AItemActor_Common::AItemActor_Common(const FObjectInitializer& ObjectInitializer)
	: AItemActor_Base(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	SkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	SkeletalMeshComp->SetIsReplicated(true);
	StaticMeshComp->SetIsReplicated(true);
}

void AItemActor_Common::SetUp(const FTransform& Transform)
{
	if (ItemID != nullptr)
	{
		if (ItemID->GetDefaultObject<UInventoryItemDefinition>())
		{
			if (const UInventoryFragment_StaticMesh* StaticMeshFragment = Cast<UInventoryFragment_StaticMesh>(UInventoryFunctionLibrary::FindItemDefinitionFragment(ItemID, UInventoryFragment_StaticMesh::StaticClass())))
			{
				StaticMeshComp->SetStaticMesh(StaticMeshFragment->PickupStaticMesh);
				StaticMeshComp->SetSimulatePhysics(StaticMeshFragment->bEnablePhysics);
				if (StaticMeshFragment->bEnableCollisionWithPlayer)
				{
					StaticMeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
				}
				else
				{
					StaticMeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
				}
				SkeletalMeshComp->SetSkeletalMesh(nullptr);
				SetRootComponent(StaticMeshComp);
				SkeletalMeshComp->SetWorldTransform(StaticMeshComp->GetComponentTransform());
				SetActorTransform(Transform);
			}
			if (const UInventoryFragment_SkeletalMesh* SkeletalMeshFragment = Cast<UInventoryFragment_SkeletalMesh>(UInventoryFunctionLibrary::FindItemDefinitionFragment(ItemID, UInventoryFragment_SkeletalMesh::StaticClass())))
			{
				SkeletalMeshComp->SetSkeletalMesh(SkeletalMeshFragment->PickupSkeletalMesh);
				if (SkeletalMeshComp->GetPhysicsAsset() != nullptr)
				{
					SkeletalMeshComp->SetSimulatePhysics(SkeletalMeshFragment->bEnablePhysics);
				}
				if (SkeletalMeshFragment->bEnableCollisionWithPlayer)
				{
					SkeletalMeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
				}
				else
				{
					SkeletalMeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
				}
				StaticMeshComp->SetStaticMesh(nullptr);
				SetRootComponent(SkeletalMeshComp);
				StaticMeshComp->SetWorldTransform(SkeletalMeshComp->GetComponentTransform());
				SetActorTransform(Transform,false,nullptr,ETeleportType::TeleportPhysics);
			}
		}
	}
}

void AItemActor_Common::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetUp(Transform);
}
