// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InstancedStruct.h"
#include "InventoryInstancedStructArray.generated.h"

class UInventoryInstancedStructObject;

USTRUCT(BlueprintType)
struct FInventoryInstancedStructEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FInstancedStruct Data;
	
	const UScriptStruct* GetDataStructType() const;

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		return Data.NetSerialize(Ar, Map, bOutSuccess);
	}
};

template<> struct TStructOpsTypeTraits<FInventoryInstancedStructEntry> : public TStructOpsTypeTraitsBase2<FInventoryInstancedStructEntry>
{
	enum { WithNetSerializer = true };
};

USTRUCT(BlueprintType)
struct FInventoryInstancedStructContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FInventoryInstancedStructEntry> Properties;
	
	bool IsPropertiesValid() const;

	FInstancedStruct* GetDataStructByType(const UScriptStruct* InType);
	
	// 复制函数
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FInventoryInstancedStructEntry, FInventoryInstancedStructContainer>(Properties, DeltaParams, *this);
	}
};

template<> struct TStructOpsTypeTraits<FInventoryInstancedStructContainer> : public TStructOpsTypeTraitsBase2<FInventoryInstancedStructContainer>
{
	enum { WithNetDeltaSerializer = true };
};

USTRUCT(BlueprintType)
struct FInventoryInstanceObjectEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UInventoryInstancedStructObject* InstanceObject;
};

USTRUCT(BlueprintType)
struct FInventoryInstanceObjectContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FInventoryInstanceObjectEntry> Instances;
	
	// 复制函数
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FastArrayDeltaSerialize<FInventoryInstanceObjectEntry, FInventoryInstanceObjectContainer>(Instances, DeltaParams, *this);
	}
};

template<> struct TStructOpsTypeTraits<FInventoryInstanceObjectContainer> : public TStructOpsTypeTraitsBase2<FInventoryInstanceObjectContainer>
{
	enum { WithNetDeltaSerializer = true };
};

UCLASS()
class INVENTORY_API UInventoryInstancedStructLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Inventory")
	static void MarkInvContainerItemDirty(UPARAM(ref)FInventoryInstancedStructContainer& InContainer, const int Index);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	static void MarkInvContainerArrayDirty(UPARAM(ref)FInventoryInstancedStructContainer& InContainer);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	static void MarkInstanceContainerItemDirty(UPARAM(ref)FInventoryInstanceObjectContainer& InContainer, const int Index);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	static void MarkInstanceContainerArrayDirty(UPARAM(ref)FInventoryInstanceObjectContainer& InContainer);
	
	UFUNCTION(BlueprintPure, Category="Inventory")
	static bool IsStructTypeEqual(UPARAM(ref) const FInstancedStruct& InstancedStruct, const UScriptStruct* InStructType);

	UFUNCTION(BlueprintCallable)
	static int FindInstancedStruct(const FInventoryInstancedStructContainer& Inventory, UScriptStruct* StructType, bool bMatchChildOf);
};
