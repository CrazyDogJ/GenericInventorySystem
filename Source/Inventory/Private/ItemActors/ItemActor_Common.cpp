// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemActors/ItemActor_Common.h"

#include "InventoryItemDefinition.h"
#include "Fragments/InventoryFragment_SkeletalMesh.h"
#include "Fragments/InventoryFragment_StaticMesh.h"


// Sets default values
AItemActor_Common::AItemActor_Common()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);
	bEnableAutoLODGeneration = false;

	ItemStaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemStaticMeshComponent"));
	ItemSkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemSkeletalMeshComponent"));
}

void AItemActor_Common::InitComps(const FTransform& Transform)
{
	ItemStaticMeshComponent->SetWorldTransform(Transform);
	ItemSkeletalMeshComponent->SetWorldTransform(Transform);
	
	if (!ItemID)
	{
		return;
	}
	
	switch (ItemID->MeshType)
	{
	case MT_None:
		{
			SetRootComponent(ItemStaticMeshComponent);
			ItemStaticMeshComponent->SetStaticMesh(nullptr);
			ItemSkeletalMeshComponent->SetSkeletalMesh(nullptr);
			break;
		}
	case MT_StaticMesh:
		{
			if (auto Settings = Cast<UInventoryFragment_StaticMesh>(ItemID->MeshSettings))
			{
				ItemStaticMeshComponent->Activate();
				ItemStaticMeshComponent->SetVisibility(true);
				SetRootComponent(ItemStaticMeshComponent);
				
				ItemSkeletalMeshComponent->SetSkeletalMesh(nullptr);
				ItemSkeletalMeshComponent->SetVisibility(false);
				ItemSkeletalMeshComponent->Deactivate();

				if (Settings->PickupStaticMesh_Multiple && Amount > 1)
				{
					ItemStaticMeshComponent->SetStaticMesh(Settings->PickupStaticMesh_Multiple);
				}
				else
				{
					ItemStaticMeshComponent->SetStaticMesh(Settings->PickupStaticMesh);
				}
				const bool bCollideWithPlayer = Settings->bEnableCollisionWithPlayer;
				ItemStaticMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, bCollideWithPlayer ? ECR_Block : ECR_Ignore);
				ItemStaticMeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, IsRuntimeActor() ? ECR_Block : ECR_Ignore);
				ItemStaticMeshComponent->SetCollisionObjectType(ECC_PhysicsBody);
				ItemStaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				ItemStaticMeshComponent->SetGenerateOverlapEvents(true);
				ItemStaticMeshComponent->SetSimulatePhysics(true);
				ItemStaticMeshComponent->SetMassOverrideInKg(NAME_None, Settings->Mass);
				if (!IsRuntimeActor())
				{
					ItemStaticMeshComponent->BodyInstance.bStartAwake = false;
					ItemStaticMeshComponent->PutAllRigidBodiesToSleep();
				}
			}
			break;
		}
	case MT_SkeletalMesh:
		{
			if (auto Settings = Cast<UInventoryFragment_SkeletalMesh>(ItemID->MeshSettings))
			{
				ItemSkeletalMeshComponent->Activate();
				ItemSkeletalMeshComponent->SetVisibility(true);
				SetRootComponent(ItemSkeletalMeshComponent);
				
				ItemStaticMeshComponent->SetStaticMesh(nullptr);
				ItemStaticMeshComponent->SetVisibility(false);
				ItemStaticMeshComponent->Deactivate();
				
				if (Settings->PickupSkeletalMesh_Multiple && Amount > 1)
				{
					ItemSkeletalMeshComponent->SetSkeletalMesh(Settings->PickupSkeletalMesh_Multiple);
				}
				else
				{
					ItemSkeletalMeshComponent->SetSkeletalMesh(Settings->PickupSkeletalMesh);
				}
				const bool bCollideWithPlayer = Settings->bEnableCollisionWithPlayer;
				ItemSkeletalMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, bCollideWithPlayer ? ECR_Block : ECR_Ignore);
				ItemSkeletalMeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, IsRuntimeActor() ? ECR_Block : ECR_Ignore);
				ItemSkeletalMeshComponent->SetCollisionObjectType(ECC_PhysicsBody);
				ItemSkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				ItemSkeletalMeshComponent->SetGenerateOverlapEvents(true);
				ItemSkeletalMeshComponent->SetSimulatePhysics(true);
				if (!IsRuntimeActor())
				{
					ItemSkeletalMeshComponent->BodyInstance.bStartAwake = false;
					for (auto Body : ItemSkeletalMeshComponent->Bodies)
					{
						Body->bStartAwake = false;
					}
					ItemSkeletalMeshComponent->PutAllRigidBodiesToSleep();
				}
			}
			break;
		}
	default: ;
	}
}

bool AItemActor_Common::IsRuntimeActor() const
{
	// Copy from spud lib.
	
	// RF_WasLoaded means it was part of a level
	// But not being part of a level might not means it needs to be respawned, it might have been
	// auto-spawned e.g. Game Modes, pawns

	bool Ret;
	if (IsChildActor())
	{
		Ret = !GetParentComponent()->GetOwner()->HasAnyFlags(RF_WasLoaded);
	}
	else
	{
		Ret = IsValid(this) && !HasAnyFlags(RF_WasLoaded);
	}

	// Interestingly, objects which have been created in the editor but the level hasn't been saved yet will
	// cause the object to incorrectly be marked as spawned after level load, because RF_WasLoaded is false
	// In fact the only flags these unsaved objects have is RF_Transactional, but all actors have that
	// We can detect this by checking if the package is dirty, but this can only really be done in an editor lib
	// to avoid circular references.
	

	return Ret;
}

void AItemActor_Common::SimulatePhysics()
{
	bIsSimulatingPhysicsInEditor = true;
	if (GetRootComponent())
	{
		if (auto PrimComp = Cast<UPrimitiveComponent>(GetRootComponent()))
		{
			PrimComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			PrimComp->SetCollisionObjectType(ECC_PhysicsBody);
			PrimComp->SetCollisionResponseToAllChannels(ECR_Block);
			PrimComp->SetSimulatePhysics(true);
		}
	}
}

void AItemActor_Common::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	/** If item definition or amount is not valid, we will use SceneComponent to store current actor's transform */
	if (!ItemID)
	{
		SetRootComponent(ItemStaticMeshComponent);
		ItemStaticMeshComponent->SetStaticMesh(nullptr);
		ItemSkeletalMeshComponent->SetSkeletalMesh(nullptr);
		ItemStaticMeshComponent->SetWorldTransform(Transform);
		ItemSkeletalMeshComponent->SetWorldTransform(Transform);
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("Item definition not valid")));
		return;
	}
	if (Amount <= 0 || Amount > ItemID->MaxStackAmount)
	{
		SetRootComponent(ItemStaticMeshComponent);
		ItemStaticMeshComponent->SetStaticMesh(nullptr);
		ItemSkeletalMeshComponent->SetSkeletalMesh(nullptr);
		ItemStaticMeshComponent->SetWorldTransform(Transform);
		ItemSkeletalMeshComponent->SetWorldTransform(Transform);
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("Amount property is not right, max stack amount is %d"), ItemID->MaxStackAmount));
		return;
	}

	/** Initialize components */
	InitComps(Transform);
}

void AItemActor_Common::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// If not primitive then return.
	const auto RootPrimitive = Cast<UPrimitiveComponent>(GetRootComponent());
	if (!RootPrimitive)
	{
		return;
	}
	
#if WITH_EDITOR
	if (bIsSimulatingPhysicsInEditor)
	{
		return;
	}
#endif
	
	if (!IsRuntimeActor() && RootPrimitive->IsAnyRigidBodyAwake())
	{
		// Spawn runtime actor
		auto Copy = GetWorld()->SpawnActorDeferred<AItemActor_Common>(GetClass(), GetActorTransform());
		Copy->ItemID = ItemID;
		Copy->Amount = Amount;
		Copy->OverrideTagStack = OverrideTagStack;
		Copy->FinishSpawning(GetActorTransform());
		
		auto CopyRoot = Cast<UPrimitiveComponent>(Copy->GetRootComponent());
		auto CurrentRoot = Cast<UPrimitiveComponent>(GetRootComponent());
		if (CopyRoot && CurrentRoot)
		{
			CopyRoot->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
			CopyRoot->WakeAllRigidBodies();
			CopyRoot->SetAllPhysicsLinearVelocity(CurrentRoot->GetPhysicsLinearVelocity());
			CopyRoot->SetAllPhysicsAngularVelocityInDegrees(CurrentRoot->GetPhysicsAngularVelocityInDegrees());
		}
		Destroy();
	}
}

void AItemActor_Common::NativeOnItemPickedUp()
{
	Super::NativeOnItemPickedUp();
	// When picked up, destroy!
	if (Amount <= 0 || !ItemID)
	{
		Destroy();
	}
}

void AItemActor_Common::OnRep_ItemID()
{
	Super::OnRep_ItemID();

	InitComps(GetTransform());
}
