// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemActor_Common.h"

#include "InventoryFragment_SkeletalMesh.h"
#include "InventoryFragment_StaticMesh.h"
#include "InventorySubsystem.h"

AItemActor_Common::AItemActor_Common(const FObjectInitializer& ObjectInitializer)
	: AItemActor_Base(ObjectInitializer)
{
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bCanEverTick = false;
	SkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	//SkeletalMeshComp->SetIsReplicated(true);
	//StaticMeshComp->SetIsReplicated(true);
	SetRootComponent(StaticMeshComp);
	bAlwaysRelevant = true;
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
				UStaticMesh* Mesh = nullptr;
				if (StaticMeshFragment->PickupStaticMesh)
				{
					if (StaticMeshFragment->PickupStaticMesh_Multiple && Amount > 1)
					{
						Mesh = StaticMeshFragment->PickupStaticMesh_Multiple;
					}
					else
					{
						Mesh = StaticMeshFragment->PickupStaticMesh;
					}
				}
				StaticMeshComp->SetStaticMesh(Mesh);
				SetPawnCollisionChannel(!StaticMeshFragment->bEnableCollisionWithPlayer);
				StaticMeshComp->SetCollisionObjectType(ECC_Pawn);
				StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				StaticMeshComp->SetSimulatePhysics(StaticMeshFragment->bEnablePhysics);
				StaticMeshComp->SetGenerateOverlapEvents(true);
				SkeletalMeshComp->SetSkeletalMesh(nullptr);
				SkeletalMeshComp->SetVisibility(false);
				SkeletalMeshComp->SetSimulatePhysics(false);
				SkeletalMeshComp->SetGenerateOverlapEvents(false);
				SetRootComponent(StaticMeshComp);
				SkeletalMeshComp->SetWorldTransform(StaticMeshComp->GetComponentTransform());
				SetActorTransform(Transform);
			}
			if (const UInventoryFragment_SkeletalMesh* SkeletalMeshFragment = Cast<UInventoryFragment_SkeletalMesh>(UInventoryFunctionLibrary::FindItemDefinitionFragment(ItemID, UInventoryFragment_SkeletalMesh::StaticClass())))
			{
				SkeletalMeshComp->SetVisibility(true);
				USkeletalMesh* Mesh = nullptr;
				if (SkeletalMeshFragment->PickupSkeletalMesh)
				{
					if (SkeletalMeshFragment->PickupSkeletalMesh_Multiple && Amount > 1)
					{
						Mesh = SkeletalMeshFragment->PickupSkeletalMesh_Multiple;
					}
					else
					{
						Mesh = SkeletalMeshFragment->PickupSkeletalMesh;
					}
				}
				SkeletalMeshComp->SetSkeletalMesh(Mesh);
				SkeletalMeshComp->SetGenerateOverlapEvents(true);
				SetPawnCollisionChannel(!SkeletalMeshFragment->bEnableCollisionWithPlayer);
				SkeletalMeshComp->SetCollisionObjectType(ECC_Pawn);
				SkeletalMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				if (SkeletalMeshComp->GetPhysicsAsset() != nullptr)
				{
					SkeletalMeshComp->SetSimulatePhysics(SkeletalMeshFragment->bEnablePhysics);
				}
				StaticMeshComp->SetGenerateOverlapEvents(false);
				StaticMeshComp->SetStaticMesh(nullptr);
				StaticMeshComp->SetVisibility(false);
				StaticMeshComp->SetSimulatePhysics(false);
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

	if (const auto InventorySubsystem = GetWorld()->GetSubsystem<UInventorySubsystem>())
	{
		InventorySubsystem->RegisterItemActorToLoadedPool(this);
	}
}

void AItemActor_Common::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (const auto InventorySubsystem = GetWorld()->GetSubsystem<UInventorySubsystem>())
	{
		InventorySubsystem->ItemActorDestroyed(this);
	}
}
