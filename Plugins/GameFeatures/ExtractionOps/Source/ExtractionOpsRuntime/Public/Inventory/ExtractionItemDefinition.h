// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayEffect.h"

#include "ExtractionItemDefinition.generated.h"

class ULyraInventoryItemDefinition;

UENUM(BlueprintType)
enum class EExtractionItemCategory : uint8
{
	Equipment,
	Consumable,
	Valuable,
	Ammo
};

UENUM(BlueprintType)
enum class EExtractionItemTier : uint8
{
	Common,
	Uncommon,
	Rare
};

/** Type data for raid loot. Runtime identity and quantity live in inventory/world instances. */
UCLASS(BlueprintType)
class EXTRACTIONOPSRUNTIME_API UExtractionItemDefinition final : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Extraction Ops|Item")
	FName DefinitionId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Extraction Ops|Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Extraction Ops|Item")
	EExtractionItemCategory Category = EExtractionItemCategory::Valuable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Extraction Ops|Item")
	EExtractionItemTier Tier = EExtractionItemTier::Common;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Extraction Ops|Item", meta=(ClampMin="0"))
	int32 BaseValue = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Extraction Ops|Item", meta=(ClampMin="1", ClampMax="99"))
	int32 MaxStackSize = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Extraction Ops|Item")
	bool bDroppable = true;

	/** Optional GAS effect applied atomically before a consumable stack is decremented. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Extraction Ops|Item")
	TSubclassOf<UGameplayEffect> UseGameplayEffect;

	/** Optional bridge to Lyra Equipment/QuickBar for equipment definitions. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Extraction Ops|Item")
	TSubclassOf<ULyraInventoryItemDefinition> LyraEquipmentItemDefinition;
};
