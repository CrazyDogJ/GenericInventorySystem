#include "ItemActors/ItemActor_Static.h"

#include "InventoryItemDefinition.h"
#include "InventorySettings.h"
#include "Fragments/InventoryFragment_SkeletalMesh.h"
#include "Fragments/InventoryFragment_StaticMesh.h"
#include "ItemActors/ItemActor_Common.h"

AItemActor_Static::AItemActor_Static()
{
	PrimaryActorTick.bCanEverTick = true;
	bEnableAutoLODGeneration = false;
	SetReplicates(true);
	SetTickGroup(TG_PostPhysics);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

#if WITH_EDITOR
void AItemActor_Static::SimulatePhysics()
{
	bIsSimulatingPhysicsInEditor = true;
	if (MeshComponent)
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComponent->SetCollisionObjectType(ECC_PhysicsBody);
		MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
		MeshComponent->SetSimulatePhysics(true);
	}
}

void AItemActor_Static::RefreshMesh()
{
	InitComps(MeshComponent, GetActorTransform());
}
#endif

void AItemActor_Static::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!MeshComponent)
	{
		return;
	}
	
#if WITH_EDITOR
	if (bIsSimulatingPhysicsInEditor)
	{
		Root->SetWorldTransform(MeshComponent->GetComponentTransform());
		return;
	}
#endif
	
	if (HasAuthority())
	{
		if (MeshComponent->IsAnyRigidBodyAwake())
		{
			// Spawn runtime actor
			//use project settings cpp class or bp class
			UClass* Class = AItemActor_Common::StaticClass();
			if (const UInventorySettings* Settings = GetMutableDefault<UInventorySettings>())
			{
				if (Settings->GetDynamicItemActorClass())
				{
					Class = Settings->GetDynamicItemActorClass();
				}
			}
		
			auto Copy = GetWorld()->SpawnActorDeferred<AItemActor_Common>(Class, GetActorTransform());
			Copy->ItemID = ItemID;
			Copy->Amount = Amount;
			Copy->ItemInstances = ItemInstances;
			Copy->bUseDefaultInstance = bUseDefaultInstance;
			Copy->InitVelocity = MeshComponent->GetPhysicsLinearVelocity();
			Copy->InitAngularVelocity = MeshComponent->GetPhysicsAngularVelocityInDegrees();
			Copy->FinishSpawning(GetActorTransform());
			Destroy();
		}
	}
	else
	{
		MeshComponent->SetSimulatePhysics(false);
	}
}

void AItemActor_Static::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	InitComps(MeshComponent, Transform);
}

//The same as common
void AItemActor_Static::NativeOnItemPickedUp()
{
	Super::NativeOnItemPickedUp();
	// When picked up, destroy!
	if (Amount <= 0 || !ItemID)
	{
		Destroy();
	}
}

void AItemActor_Static::OnRep_ItemID()
{
	Super::OnRep_ItemID();

	InitComps(MeshComponent, GetTransform());
}

void AItemActor_Static::InitComps(UMeshComponent*& InMeshComponent, const FTransform& Transform)
{
	/** If item definition or amount is not valid, we will use SceneComponent to store current actor's transform */
	if (!ItemID)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("Item definition not valid")));
		return;
	}
	if (Amount <= 0 || Amount > ItemID->GetMaxMaxStackAmount())
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("Amount property is not right, max stack amount is %d"), ItemID->GetMaxMaxStackAmount()));
		return;
	}
	
	switch (ItemID->MeshType)
	{
	case MT_None:
		{
			break;
		}
	case MT_StaticMesh:
		{
			if (auto Settings = Cast<UInventoryFragment_StaticMesh>(ItemID->MeshSettings))
			{
				if (InMeshComponent)
				{
					InMeshComponent->DestroyComponent();
					InMeshComponent = nullptr;
				}
				
				auto AddedComp = AddComponentByClass(UStaticMeshComponent::StaticClass(), true, Transform, false);
				auto StaticMeshComp = Cast<UStaticMeshComponent>(AddedComp);
				InMeshComponent = StaticMeshComp;
				InMeshComponent->SetIsReplicated(true);

				if (Settings->PickupStaticMesh_Multiple && Amount > 1)
				{
					StaticMeshComp->SetStaticMesh(Settings->PickupStaticMesh_Multiple);
				}
				else
				{
					StaticMeshComp->SetStaticMesh(Settings->PickupStaticMesh);
				}
				const bool bCollideWithPlayer = Settings->bEnableCollisionWithPlayer;
				StaticMeshComp->SetCollisionResponseToChannel(ECC_Pawn, bCollideWithPlayer ? ECR_Block : ECR_Ignore);
				StaticMeshComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
				StaticMeshComp->SetCollisionObjectType(ECC_PhysicsBody);
				StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				StaticMeshComp->SetGenerateOverlapEvents(true);
				StaticMeshComp->SetSimulatePhysics(true);
				StaticMeshComp->SetMassOverrideInKg(NAME_None, Settings->Mass);
				StaticMeshComp->BodyInstance.bStartAwake = false;
				StaticMeshComp->PutAllRigidBodiesToSleep();
				OnMeshReady(StaticMeshComp);
			}
			break;
		}
	case MT_SkeletalMesh:
		{
			if (auto Settings = Cast<UInventoryFragment_SkeletalMesh>(ItemID->MeshSettings))
			{
				if (InMeshComponent)
				{
					InMeshComponent->DestroyComponent();
					InMeshComponent = nullptr;
				}
				
				auto AddedComp = AddComponentByClass(USkeletalMeshComponent::StaticClass(), true, Transform, false);
				auto SkeletalMeshComp = Cast<USkeletalMeshComponent>(AddedComp);
				InMeshComponent = SkeletalMeshComp;
				InMeshComponent->SetIsReplicated(true);
				
				if (Settings->PickupSkeletalMesh_Multiple && Amount > 1)
				{
					SkeletalMeshComp->SetSkeletalMesh(Settings->PickupSkeletalMesh_Multiple);
				}
				else
				{
					SkeletalMeshComp->SetSkeletalMesh(Settings->PickupSkeletalMesh);
				}
				const bool bCollideWithPlayer = Settings->bEnableCollisionWithPlayer;
				SkeletalMeshComp->SetCollisionResponseToChannel(ECC_Pawn, bCollideWithPlayer ? ECR_Block : ECR_Ignore);
				SkeletalMeshComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
				SkeletalMeshComp->SetCollisionObjectType(ECC_PhysicsBody);
				SkeletalMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				SkeletalMeshComp->SetGenerateOverlapEvents(true);
				SkeletalMeshComp->SetSimulatePhysics(true);
				SkeletalMeshComp->BodyInstance.bStartAwake = false;
				for (auto Body : SkeletalMeshComp->Bodies)
				{
					Body->bStartAwake = false;
				}
				SkeletalMeshComp->PutAllRigidBodiesToSleep();
				OnMeshReady(SkeletalMeshComp);
			}
			break;
		}
	default: ;
	}
}