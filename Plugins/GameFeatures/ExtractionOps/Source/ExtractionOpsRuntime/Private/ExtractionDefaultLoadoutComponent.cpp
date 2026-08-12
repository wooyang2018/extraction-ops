// Copyright Extraction Ops. All Rights Reserved.

#include "ExtractionDefaultLoadoutComponent.h"

#include "Equipment/LyraQuickBarComponent.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryManagerComponent.h"

#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionDefaultLoadoutComponent)

DEFINE_LOG_CATEGORY_STATIC(LogExtractionLoadout, Log, All);

namespace ExtractionDefaultLoadout
{
	constexpr float RetryIntervalSeconds = 0.25f;

	const FSoftClassPath RifleDefinitionPath(
		TEXT("/ExtractionOps/Weapons/Rifle/ID_ExtractionRifle.ID_ExtractionRifle_C"));
	const FSoftClassPath ShotgunDefinitionPath(
		TEXT("/ExtractionOps/Weapons/Shotgun/ID_ExtractionShotgun.ID_ExtractionShotgun_C"));

	bool IsItemInQuickBar(const ULyraQuickBarComponent& QuickBar, const ULyraInventoryItemInstance* Item)
	{
		return QuickBar.GetSlots().Contains(Item);
	}

	void AddItemToQuickBar(ULyraQuickBarComponent& QuickBar, int32 SlotIndex, ULyraInventoryItemInstance* Item)
	{
		struct FAddItemToSlotParameters
		{
			int32 SlotIndex = INDEX_NONE;
			ULyraInventoryItemInstance* Item = nullptr;
		};

		FAddItemToSlotParameters Parameters{SlotIndex, Item};
		QuickBar.ProcessEvent(
			QuickBar.FindFunctionChecked(GET_FUNCTION_NAME_CHECKED(ULyraQuickBarComponent, AddItemToSlot)),
			&Parameters);
	}

	void ActivateQuickBarSlot(ULyraQuickBarComponent& QuickBar, int32 SlotIndex)
	{
		struct FSetActiveSlotIndexParameters
		{
			int32 NewIndex = INDEX_NONE;
		};

		FSetActiveSlotIndexParameters Parameters{SlotIndex};
		QuickBar.ProcessEvent(
			QuickBar.FindFunctionChecked(GET_FUNCTION_NAME_CHECKED(ULyraQuickBarComponent, SetActiveSlotIndex)),
			&Parameters);
	}
}

void UExtractionDefaultLoadoutComponent::BeginPlay()
{
	Super::BeginPlay();

	const AController* Controller = GetController<AController>();
	if (Controller == nullptr || !Controller->HasAuthority())
	{
		return;
	}

	TryGrantDefaultLoadout();
}

void UExtractionDefaultLoadoutComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GrantRetryTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UExtractionDefaultLoadoutComponent::TryGrantDefaultLoadout()
{
	AController* Controller = GetController<AController>();
	UWorld* World = GetWorld();
	if (Controller == nullptr || World == nullptr || !Controller->HasAuthority())
	{
		return;
	}

	ULyraInventoryManagerComponent* Inventory = Controller->FindComponentByClass<ULyraInventoryManagerComponent>();
	ULyraQuickBarComponent* QuickBar = Controller->FindComponentByClass<ULyraQuickBarComponent>();
	if (Inventory == nullptr || QuickBar == nullptr)
	{
		if (--RemainingGrantAttempts > 0)
		{
			World->GetTimerManager().SetTimer(
				GrantRetryTimerHandle,
				this,
				&ThisClass::TryGrantDefaultLoadout,
				ExtractionDefaultLoadout::RetryIntervalSeconds,
				false);
		}
		else
		{
			UE_LOG(LogExtractionLoadout, Error,
				TEXT("Default loadout grant timed out: Inventory=%s QuickBar=%s Owner=%s"),
				Inventory != nullptr ? TEXT("valid") : TEXT("missing"),
				QuickBar != nullptr ? TEXT("valid") : TEXT("missing"),
				*GetNameSafe(Controller));
		}
		return;
	}

	const TSubclassOf<ULyraInventoryItemDefinition> ItemDefinitions[] = {
		ExtractionDefaultLoadout::RifleDefinitionPath.TryLoadClass<ULyraInventoryItemDefinition>(),
		ExtractionDefaultLoadout::ShotgunDefinitionPath.TryLoadClass<ULyraInventoryItemDefinition>()
	};

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ItemDefinitions); ++Index)
	{
		const TSubclassOf<ULyraInventoryItemDefinition> ItemDefinition = ItemDefinitions[Index];
		if (ItemDefinition == nullptr)
		{
			UE_LOG(LogExtractionLoadout, Error, TEXT("Default loadout item %d could not be loaded"), Index);
			continue;
		}

		ULyraInventoryItemInstance* Item = Inventory->FindFirstItemStackByDefinition(ItemDefinition);
		if (Item == nullptr)
		{
			Item = Inventory->AddItemDefinition(ItemDefinition);
		}

		if (Item != nullptr && !ExtractionDefaultLoadout::IsItemInQuickBar(*QuickBar, Item))
		{
			ExtractionDefaultLoadout::AddItemToQuickBar(*QuickBar, Index, Item);
		}
	}

	ExtractionDefaultLoadout::ActivateQuickBarSlot(*QuickBar, 0);
	World->GetTimerManager().ClearTimer(GrantRetryTimerHandle);
	UE_LOG(LogExtractionLoadout, Log, TEXT("Granted Extraction rifle and shotgun to %s"), *GetNameSafe(Controller));
}
