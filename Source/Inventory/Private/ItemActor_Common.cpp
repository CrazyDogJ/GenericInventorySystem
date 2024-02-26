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
	//SkeletalMeshComp->SetIsReplicated(true);
	//StaticMeshComp->SetIsReplicated(true);
	SetRootComponent(StaticMeshComp);
}

void AItemActor_Common::OnRep_ItemID()
{
	SetUp(GetTransform());
}

void AItemActor_Common::SetPawnCollisionChannel(bool bOverlaped) const
{
	Cast<UPrimitiveComponent>(GetRootComponent())->SetCollisionResponseToChannel(ECC_Pawn, bOverlaped ? ECR_Overlap : ECR_Block);
}

void AItemActor_Common::SetUp(const FTransform& Transform)
{
	if (ItemID != nullptr)
	{
		if (ItemID->GetDefaultObject<UInventoryItemDefinition>())
		{
			if (const UInventoryFragment_StaticMesh* StaticMeshFragment = Cast<UInventoryFragment_StaticMesh>(UInventoryFunctionLibrary::FindItemDefinitionFragment(ItemID, UInventoryFragment_StaticMesh::StaticClass())))
			{
				StaticMeshComp->SetVisibility(true);
				StaticMeshComp->SetStaticMesh(StaticMeshFragment->PickupStaticMesh);
				SetPawnCollisionChannel(!StaticMeshFragment->bEnableCollisionWithPlayer);
				StaticMeshComp->SetCollisionObjectType(ECC_WorldDynamic);
				StaticMeshComp->SetSimulatePhysics(StaticMeshFragment->bEnablePhysics);
				SkeletalMeshComp->SetSkeletalMesh(nullptr);
				SkeletalMeshComp->SetVisibility(false);
				SetRootComponent(StaticMeshComp);
				SkeletalMeshComp->SetWorldTransform(StaticMeshComp->GetComponentTransform());
				SetActorTransform(Transform);
			}
			if (const UInventoryFragment_SkeletalMesh* SkeletalMeshFragment = Cast<UInventoryFragment_SkeletalMesh>(UInventoryFunctionLibrary::FindItemDefinitionFragment(ItemID, UInventoryFragment_SkeletalMesh::StaticClass())))
			{
				SkeletalMeshComp->SetVisibility(true);
				SkeletalMeshComp->SetSkeletalMesh(SkeletalMeshFragment->PickupSkeletalMesh);
				SetPawnCollisionChannel(!SkeletalMeshFragment->bEnableCollisionWithPlayer);
				SkeletalMeshComp->SetCollisionObjectType(ECC_WorldDynamic);
				if (SkeletalMeshComp->GetPhysicsAsset() != nullptr)
				{
					SkeletalMeshComp->SetSimulatePhysics(SkeletalMeshFragment->bEnablePhysics);
				}
				StaticMeshComp->SetStaticMesh(nullptr);
				StaticMeshComp->SetVisibility(false);
				SetRootComponent(SkeletalMeshComp);
				StaticMeshComp->SetWorldTransform(SkeletalMeshComp->GetComponentTransform());
				SetActorTransform(Transform,false,nullptr,ETeleportType::TeleportPhysics);
			}
		}
	}
}

void AItemActor_Common::OnConstruction(const FTransform& Transform)
{
	SetUp(Transform);
	Super::OnConstruction(Transform);
}

void AItemActor_Common::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		SetUp(GetTransform());
	}
}
