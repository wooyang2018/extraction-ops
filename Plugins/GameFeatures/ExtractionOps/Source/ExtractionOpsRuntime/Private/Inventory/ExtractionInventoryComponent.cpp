// Copyright Extraction Ops. All Rights Reserved.

#include "Inventory/ExtractionInventoryComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "ExtractionRunStateComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/ExtractionWorldPickup.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionInventoryComponent)

DEFINE_LOG_CATEGORY_STATIC(LogExtractionInventory, Log, All);

UExtractionInventoryComponent::UExtractionInventoryComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	WorldPickupClass = AExtractionWorldPickup::StaticClass();
}

void UExtractionInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	NormalizeSlots();
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		UE_LOG(LogExtractionInventory, Log, TEXT("event=inventory_component_ready owner=%s capacity=%d"),
			*GetNameSafe(GetOwner()), Capacity);
	}
}

void UExtractionInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ThisClass, Slots, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ThisClass, InventoryVersion, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ThisClass, LastCommandResult, COND_OwnerOnly);
}

void UExtractionInventoryComponent::RequestMoveItem(int32 SourceSlot, int32 TargetSlot,
	int32 ExpectedVersion, FGuid RequestId)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ServerMoveItem_Implementation(SourceSlot, TargetSlot, ExpectedVersion, RequestId);
	}
	else
	{
		ServerMoveItem(SourceSlot, TargetSlot, ExpectedVersion, RequestId);
	}
}

void UExtractionInventoryComponent::RequestSplitStack(FGuid ItemInstanceId, int32 Quantity,
	int32 TargetSlot, int32 ExpectedVersion, FGuid RequestId)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ServerSplitStack_Implementation(ItemInstanceId, Quantity, TargetSlot, ExpectedVersion, RequestId);
	}
	else
	{
		ServerSplitStack(ItemInstanceId, Quantity, TargetSlot, ExpectedVersion, RequestId);
	}
}

void UExtractionInventoryComponent::RequestUseItem(FGuid ItemInstanceId, int32 ExpectedVersion, FGuid RequestId)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ServerUseItem_Implementation(ItemInstanceId, ExpectedVersion, RequestId);
	}
	else
	{
		ServerUseItem(ItemInstanceId, ExpectedVersion, RequestId);
	}
}

void UExtractionInventoryComponent::RequestDropItem(FGuid ItemInstanceId, int32 Quantity,
	int32 ExpectedVersion, FGuid RequestId)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ServerDropItem_Implementation(ItemInstanceId, Quantity, ExpectedVersion, RequestId);
	}
	else
	{
		ServerDropItem(ItemInstanceId, Quantity, ExpectedVersion, RequestId);
	}
}

bool UExtractionInventoryComponent::ValidateCommand(int32 ExpectedVersion, FGuid RequestId,
	FExtractionInventoryCommandResult& OutResult) const
{
	OutResult.RequestId = RequestId;
	OutResult.AuthoritativeVersion = InventoryVersion;
	if (!GetOwner() || !GetOwner()->HasAuthority() || !RequestId.IsValid())
	{
		OutResult.Result = EExtractionInventoryResult::InvalidRequest;
		return false;
	}
	if (ProcessedRequestIds.Contains(RequestId))
	{
		OutResult.Result = EExtractionInventoryResult::AlreadyConsumed;
		return false;
	}
	if (ExpectedVersion != InventoryVersion)
	{
		OutResult.Result = EExtractionInventoryResult::StaleVersion;
		return false;
	}
	if (const UExtractionRunStateComponent* RunState =
		GetOwner()->FindComponentByClass<UExtractionRunStateComponent>();
		RunState && RunState->GetSnapshot().RunState != EExtractionRunState::InRaid)
	{
		OutResult.Result = EExtractionInventoryResult::InvalidState;
		return false;
	}
	return true;
}

EExtractionInventoryResult UExtractionInventoryComponent::TryAddItem(const FExtractionRaidItem& Item, FGuid RequestId)
{
	FExtractionInventoryCommandResult Validation;
	if (!ValidateCommand(InventoryVersion, RequestId, Validation) || !Item.IsValid())
	{
		CommitCommand(RequestId, Item.InstanceId,
			Item.IsValid() ? Validation.Result : EExtractionInventoryResult::InvalidRequest, false);
		return LastCommandResult.Result;
	}

	UExtractionItemDefinition* Definition = Item.Definition.LoadSynchronous();
	if (!Definition)
	{
		CommitCommand(RequestId, Item.InstanceId, EExtractionInventoryResult::ItemNotFound, false);
		return LastCommandResult.Result;
	}

	int32 Remaining = Item.Quantity;
	int32 FreeCapacity = 0;
	for (const FExtractionInventorySlot& Slot : Slots)
	{
		if (!Slot.Item.IsValid())
		{
			FreeCapacity += Definition->MaxStackSize;
		}
		else if (Slot.Item.Definition == Item.Definition)
		{
			FreeCapacity += FMath::Max(Definition->MaxStackSize - Slot.Item.Quantity, 0);
		}
	}
	if (FreeCapacity < Remaining)
	{
		CommitCommand(RequestId, Item.InstanceId, EExtractionInventoryResult::InventoryFull, false);
		return LastCommandResult.Result;
	}

	bool bOriginalInstanceAssigned = false;
	for (FExtractionInventorySlot& Slot : Slots)
	{
		if (Remaining <= 0)
		{
			break;
		}
		if (Slot.Item.IsValid() && Slot.Item.Definition == Item.Definition)
		{
			const int32 Added = FMath::Min(Definition->MaxStackSize - Slot.Item.Quantity, Remaining);
			Slot.Item.Quantity += Added;
			Remaining -= Added;
		}
	}
	for (FExtractionInventorySlot& Slot : Slots)
	{
		if (Remaining <= 0)
		{
			break;
		}
		if (!Slot.Item.IsValid())
		{
			const int32 Added = FMath::Min(Definition->MaxStackSize, Remaining);
			Slot.Item = Item;
			Slot.Item.Quantity = Added;
			if (bOriginalInstanceAssigned)
			{
				Slot.Item.InstanceId = FGuid::NewGuid();
			}
			bOriginalInstanceAssigned = true;
			Remaining -= Added;
		}
	}

	CommitCommand(RequestId, Item.InstanceId, EExtractionInventoryResult::Success, true);
	return EExtractionInventoryResult::Success;
}

void UExtractionInventoryComponent::ServerMoveItem_Implementation(int32 SourceSlot, int32 TargetSlot,
	int32 ExpectedVersion, FGuid RequestId)
{
	FExtractionInventoryCommandResult Validation;
	if (!ValidateCommand(ExpectedVersion, RequestId, Validation))
	{
		CommitCommand(RequestId, FGuid(), Validation.Result, false);
		return;
	}
	if (!Slots.IsValidIndex(SourceSlot) || !Slots.IsValidIndex(TargetSlot) || SourceSlot == TargetSlot)
	{
		CommitCommand(RequestId, FGuid(), EExtractionInventoryResult::InvalidSlot, false);
		return;
	}
	if (!Slots[SourceSlot].Item.IsValid())
	{
		CommitCommand(RequestId, FGuid(), EExtractionInventoryResult::ItemNotFound, false);
		return;
	}

	const FGuid ItemId = Slots[SourceSlot].Item.InstanceId;
	UExtractionItemDefinition* Definition = Slots[SourceSlot].Item.Definition.LoadSynchronous();
	if (Slots[TargetSlot].Item.IsValid() && Definition
		&& Slots[TargetSlot].Item.Definition == Slots[SourceSlot].Item.Definition)
	{
		const int32 Space = FMath::Max(Definition->MaxStackSize - Slots[TargetSlot].Item.Quantity, 0);
		const int32 Moved = FMath::Min(Space, Slots[SourceSlot].Item.Quantity);
		Slots[TargetSlot].Item.Quantity += Moved;
		Slots[SourceSlot].Item.Quantity -= Moved;
		if (Slots[SourceSlot].Item.Quantity <= 0)
		{
			Slots[SourceSlot].Item = FExtractionRaidItem();
		}
		if (Moved == 0)
		{
			Swap(Slots[SourceSlot].Item, Slots[TargetSlot].Item);
		}
	}
	else
	{
		Swap(Slots[SourceSlot].Item, Slots[TargetSlot].Item);
	}
	CommitCommand(RequestId, ItemId, EExtractionInventoryResult::Success, true);
}

void UExtractionInventoryComponent::ServerSplitStack_Implementation(FGuid ItemInstanceId, int32 Quantity,
	int32 TargetSlot, int32 ExpectedVersion, FGuid RequestId)
{
	FExtractionInventoryCommandResult Validation;
	if (!ValidateCommand(ExpectedVersion, RequestId, Validation))
	{
		CommitCommand(RequestId, ItemInstanceId, Validation.Result, false);
		return;
	}
	const int32 SourceSlot = FindSlotByInstanceId(ItemInstanceId);
	if (!Slots.IsValidIndex(SourceSlot))
	{
		CommitCommand(RequestId, ItemInstanceId, EExtractionInventoryResult::ItemNotFound, false);
		return;
	}
	if (!Slots.IsValidIndex(TargetSlot) || Slots[TargetSlot].Item.IsValid() || SourceSlot == TargetSlot)
	{
		CommitCommand(RequestId, ItemInstanceId, EExtractionInventoryResult::InvalidSlot, false);
		return;
	}
	if (Quantity <= 0 || Quantity >= Slots[SourceSlot].Item.Quantity)
	{
		CommitCommand(RequestId, ItemInstanceId, EExtractionInventoryResult::InvalidQuantity, false);
		return;
	}

	Slots[TargetSlot].Item = Slots[SourceSlot].Item;
	Slots[TargetSlot].Item.InstanceId = FGuid::NewGuid();
	Slots[TargetSlot].Item.Quantity = Quantity;
	Slots[SourceSlot].Item.Quantity -= Quantity;
	CommitCommand(RequestId, ItemInstanceId, EExtractionInventoryResult::Success, true);
}

void UExtractionInventoryComponent::ServerUseItem_Implementation(FGuid ItemInstanceId,
	int32 ExpectedVersion, FGuid RequestId)
{
	FExtractionInventoryCommandResult Validation;
	if (!ValidateCommand(ExpectedVersion, RequestId, Validation))
	{
		CommitCommand(RequestId, ItemInstanceId, Validation.Result, false);
		return;
	}
	const int32 SlotIndex = FindSlotByInstanceId(ItemInstanceId);
	if (!Slots.IsValidIndex(SlotIndex))
	{
		CommitCommand(RequestId, ItemInstanceId, EExtractionInventoryResult::ItemNotFound, false);
		return;
	}
	UExtractionItemDefinition* Definition = Slots[SlotIndex].Item.Definition.LoadSynchronous();
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!Definition || Definition->Category != EExtractionItemCategory::Consumable
		|| !Definition->UseGameplayEffect || !ASC)
	{
		CommitCommand(RequestId, ItemInstanceId, EExtractionInventoryResult::UnsupportedOperation, false);
		return;
	}
	const float CurrentHealth = ASC->GetNumericAttribute(ULyraHealthSet::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(ULyraHealthSet::GetMaxHealthAttribute());
	if (CurrentHealth <= 0.0f || CurrentHealth >= MaxHealth)
	{
		CommitCommand(RequestId, ItemInstanceId, EExtractionInventoryResult::InvalidState, false);
		return;
	}

	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Definition->UseGameplayEffect, 1.0f, ASC->MakeEffectContext());
	if (!Spec.IsValid())
	{
		CommitCommand(RequestId, ItemInstanceId, EExtractionInventoryResult::InvalidState, false);
		return;
	}
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	if (--Slots[SlotIndex].Item.Quantity <= 0)
	{
		Slots[SlotIndex].Item = FExtractionRaidItem();
	}
	CommitCommand(RequestId, ItemInstanceId, EExtractionInventoryResult::Success, true);
}

void UExtractionInventoryComponent::ServerDropItem_Implementation(FGuid ItemInstanceId, int32 Quantity,
	int32 ExpectedVersion, FGuid RequestId)
{
	FExtractionInventoryCommandResult Validation;
	if (!ValidateCommand(ExpectedVersion, RequestId, Validation))
	{
		CommitCommand(RequestId, ItemInstanceId, Validation.Result, false);
		return;
	}
	const int32 SlotIndex = FindSlotByInstanceId(ItemInstanceId);
	if (!Slots.IsValidIndex(SlotIndex))
	{
		CommitCommand(RequestId, ItemInstanceId, EExtractionInventoryResult::ItemNotFound, false);
		return;
	}
	UExtractionItemDefinition* Definition = Slots[SlotIndex].Item.Definition.LoadSynchronous();
	if (Quantity <= 0 || Quantity > Slots[SlotIndex].Item.Quantity)
	{
		CommitCommand(RequestId, ItemInstanceId, EExtractionInventoryResult::InvalidQuantity, false);
		return;
	}
	if (!Definition || !Definition->bDroppable)
	{
		CommitCommand(RequestId, ItemInstanceId, EExtractionInventoryResult::UnsupportedOperation, false);
		return;
	}

	FExtractionRaidItem Dropped = Slots[SlotIndex].Item;
	Dropped.Quantity = Quantity;
	if (Quantity != Slots[SlotIndex].Item.Quantity)
	{
		Dropped.InstanceId = FGuid::NewGuid();
	}
	if (!SpawnWorldPickup(Dropped))
	{
		CommitCommand(RequestId, ItemInstanceId, EExtractionInventoryResult::InvalidState, false);
		return;
	}
	Slots[SlotIndex].Item.Quantity -= Quantity;
	if (Slots[SlotIndex].Item.Quantity <= 0)
	{
		Slots[SlotIndex].Item = FExtractionRaidItem();
	}
	CommitCommand(RequestId, ItemInstanceId, EExtractionInventoryResult::Success, true);
}

TArray<FExtractionRaidItem> UExtractionInventoryComponent::RemoveAllDroppableItems(FGuid RequestId)
{
	TArray<FExtractionRaidItem> Removed;
	if (!GetOwner() || !GetOwner()->HasAuthority() || !RequestId.IsValid()
		|| ProcessedRequestIds.Contains(RequestId))
	{
		return Removed;
	}
	for (FExtractionInventorySlot& Slot : Slots)
	{
		if (Slot.Item.IsValid())
		{
			if (const UExtractionItemDefinition* Definition = Slot.Item.Definition.LoadSynchronous();
				Definition && Definition->bDroppable)
			{
				Removed.Add(Slot.Item);
				Slot.Item = FExtractionRaidItem();
			}
		}
	}
	CommitCommand(RequestId, FGuid(), EExtractionInventoryResult::Success, !Removed.IsEmpty());
	return Removed;
}

void UExtractionInventoryComponent::CommitCommand(FGuid RequestId, FGuid ItemId,
	EExtractionInventoryResult Result, bool bMutated)
{
	const int32 OldVersion = InventoryVersion;
	if (RequestId.IsValid())
	{
		ProcessedRequestIds.Add(RequestId);
	}
	if (bMutated)
	{
		++InventoryVersion;
	}
	LastCommandResult.RequestId = RequestId;
	LastCommandResult.Result = Result;
	LastCommandResult.AuthoritativeVersion = InventoryVersion;
	LastCommandResult.ItemInstanceId = ItemId;
	OnCommandCompleted.Broadcast(LastCommandResult);
	if (bMutated)
	{
		OnInventoryChanged.Broadcast(OldVersion, InventoryVersion);
		GetOwner()->ForceNetUpdate();
	}
	UE_LOG(LogExtractionInventory, Log,
		TEXT("event=inventory_command request_id=%s item_instance_id=%s result=%d version=%d mutated=%s"),
		*RequestId.ToString(EGuidFormats::DigitsWithHyphensLower),
		*ItemId.ToString(EGuidFormats::DigitsWithHyphensLower), static_cast<int32>(Result), InventoryVersion,
		bMutated ? TEXT("true") : TEXT("false"));
}

int32 UExtractionInventoryComponent::FindSlotByInstanceId(FGuid InstanceId) const
{
	return Slots.IndexOfByPredicate([InstanceId](const FExtractionInventorySlot& Slot)
	{
		return Slot.Item.InstanceId == InstanceId;
	});
}

bool UExtractionInventoryComponent::SpawnWorldPickup(const FExtractionRaidItem& Item)
{
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	const APawn* Pawn = PlayerState ? PlayerState->GetPawn() : nullptr;
	if (!Pawn || !WorldPickupClass)
	{
		return false;
	}
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AExtractionWorldPickup* Pickup = GetWorld()->SpawnActor<AExtractionWorldPickup>(WorldPickupClass,
		Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 100.0f, FRotator::ZeroRotator, Parameters);
	return Pickup && Pickup->InitializePickup(Item);
}

void UExtractionInventoryComponent::NormalizeSlots()
{
	Slots.SetNum(Capacity);
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		Slots[Index].SlotIndex = Index;
	}
}

void UExtractionInventoryComponent::OnRep_InventoryState()
{
	const int32 PreviousVersion = FMath::Max(InventoryVersion - 1, 0);
	NormalizeSlots();
	OnInventoryChanged.Broadcast(PreviousVersion, InventoryVersion);
}

void UExtractionInventoryComponent::OnRep_LastCommandResult()
{
	OnCommandCompleted.Broadcast(LastCommandResult);
}
