// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "Components/PlayerStateComponent.h"
#include "Inventory/ExtractionItemDefinition.h"

#include "ExtractionInventoryComponent.generated.h"

class AExtractionWorldPickup;

UENUM(BlueprintType)
enum class EExtractionInventoryResult : uint8
{
	Success,
	InvalidRequest,
	StaleVersion,
	ItemNotFound,
	InventoryFull,
	InvalidSlot,
	InvalidQuantity,
	InvalidState,
	AlreadyConsumed,
	OutOfRange,
	LineOfSightBlocked,
	UnsupportedOperation
};

USTRUCT(BlueprintType)
struct EXTRACTIONOPSRUNTIME_API FExtractionRaidItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGuid InstanceId;

	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UExtractionItemDefinition> Definition;

	UPROPERTY(BlueprintReadOnly)
	int32 Quantity = 0;

	bool IsValid() const { return InstanceId.IsValid() && !Definition.IsNull() && Quantity > 0; }
};

USTRUCT(BlueprintType)
struct EXTRACTIONOPSRUNTIME_API FExtractionInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	FExtractionRaidItem Item;
};

USTRUCT(BlueprintType)
struct EXTRACTIONOPSRUNTIME_API FExtractionInventoryCommandResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGuid RequestId;

	UPROPERTY(BlueprintReadOnly)
	EExtractionInventoryResult Result = EExtractionInventoryResult::InvalidRequest;

	UPROPERTY(BlueprintReadOnly)
	int32 AuthoritativeVersion = 0;

	UPROPERTY(BlueprintReadOnly)
	FGuid ItemInstanceId;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FExtractionInventoryChanged, int32, OldVersion, int32, NewVersion);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FExtractionInventoryCommandCompleted,
	const FExtractionInventoryCommandResult&, Result);

struct EXTRACTIONOPSRUNTIME_API FExtractionInventoryRules
{
	static bool IsSlotIndexValid(int32 SlotIndex) { return SlotIndex >= 0 && SlotIndex < 12; }
	static bool CanSplitStack(int32 CurrentQuantity, int32 SplitQuantity)
	{
		return SplitQuantity > 0 && SplitQuantity < CurrentQuantity;
	}
};

/**
 * Owner-only replicated, server-authoritative raid loot ledger.
 * Lyra Inventory remains the equipment/ammo runtime; this component supplies stable IDs,
 * fixed slots and versioned container transfers needed by extraction/reconnect/settlement.
 */
UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class EXTRACTIONOPSRUNTIME_API UExtractionInventoryComponent final : public UPlayerStateComponent
{
	GENERATED_BODY()

public:
	UExtractionInventoryComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	static constexpr int32 Capacity = 12;

	UFUNCTION(BlueprintPure, Category="Extraction Ops|Inventory")
	const TArray<FExtractionInventorySlot>& GetSlots() const { return Slots; }

	UFUNCTION(BlueprintPure, Category="Extraction Ops|Inventory")
	int32 GetInventoryVersion() const { return InventoryVersion; }

	UFUNCTION(BlueprintPure, Category="Extraction Ops|Inventory")
	const FExtractionInventoryCommandResult& GetLastCommandResult() const { return LastCommandResult; }

	UFUNCTION(BlueprintCallable, Category="Extraction Ops|Inventory")
	void RequestMoveItem(int32 SourceSlot, int32 TargetSlot, int32 ExpectedVersion, FGuid RequestId);

	UFUNCTION(BlueprintCallable, Category="Extraction Ops|Inventory")
	void RequestSplitStack(FGuid ItemInstanceId, int32 Quantity, int32 TargetSlot,
		int32 ExpectedVersion, FGuid RequestId);

	UFUNCTION(BlueprintCallable, Category="Extraction Ops|Inventory")
	void RequestUseItem(FGuid ItemInstanceId, int32 ExpectedVersion, FGuid RequestId);

	UFUNCTION(BlueprintCallable, Category="Extraction Ops|Inventory")
	void RequestDropItem(FGuid ItemInstanceId, int32 Quantity, int32 ExpectedVersion, FGuid RequestId);

	/** Server-only atomic world/container -> inventory transfer. */
	EExtractionInventoryResult TryAddItem(const FExtractionRaidItem& Item, FGuid RequestId);

	/** Server-only atomic removal used by death containers and settlement snapshots. */
	TArray<FExtractionRaidItem> RemoveAllDroppableItems(FGuid RequestId);

	UPROPERTY(BlueprintAssignable, Category="Extraction Ops|Inventory")
	FExtractionInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category="Extraction Ops|Inventory")
	FExtractionInventoryCommandCompleted OnCommandCompleted;

private:
	UFUNCTION(Server, Reliable)
	void ServerMoveItem(int32 SourceSlot, int32 TargetSlot, int32 ExpectedVersion, FGuid RequestId);

	UFUNCTION(Server, Reliable)
	void ServerSplitStack(FGuid ItemInstanceId, int32 Quantity, int32 TargetSlot,
		int32 ExpectedVersion, FGuid RequestId);

	UFUNCTION(Server, Reliable)
	void ServerUseItem(FGuid ItemInstanceId, int32 ExpectedVersion, FGuid RequestId);

	UFUNCTION(Server, Reliable)
	void ServerDropItem(FGuid ItemInstanceId, int32 Quantity, int32 ExpectedVersion, FGuid RequestId);

	UFUNCTION()
	void OnRep_InventoryState();

	UFUNCTION()
	void OnRep_LastCommandResult();

	bool ValidateCommand(int32 ExpectedVersion, FGuid RequestId, FExtractionInventoryCommandResult& OutResult) const;
	int32 FindSlotByInstanceId(FGuid InstanceId) const;
	void CommitCommand(FGuid RequestId, FGuid ItemId, EExtractionInventoryResult Result, bool bMutated);
	bool SpawnWorldPickup(const FExtractionRaidItem& Item);
	void NormalizeSlots();

	UPROPERTY(ReplicatedUsing=OnRep_InventoryState)
	TArray<FExtractionInventorySlot> Slots;

	UPROPERTY(ReplicatedUsing=OnRep_InventoryState)
	int32 InventoryVersion = 0;

	UPROPERTY(ReplicatedUsing=OnRep_LastCommandResult)
	FExtractionInventoryCommandResult LastCommandResult;

	UPROPERTY(EditDefaultsOnly, Category="Extraction Ops|Inventory")
	TSubclassOf<AExtractionWorldPickup> WorldPickupClass;

	TSet<FGuid> ProcessedRequestIds;
};
