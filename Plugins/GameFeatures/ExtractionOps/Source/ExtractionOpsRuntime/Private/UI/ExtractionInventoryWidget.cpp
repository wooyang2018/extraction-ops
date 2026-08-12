// Copyright Extraction Ops. All Rights Reserved.

#include "UI/ExtractionInventoryWidget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "ExtractionGameplayTags.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionInventoryWidget)

void UExtractionInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	const APlayerController* Controller = GetOwningPlayer();
	InventoryComponent = Controller && Controller->PlayerState
		? Controller->PlayerState->FindComponentByClass<UExtractionInventoryComponent>() : nullptr;
	AbilitySystemComponent = Controller && Controller->PlayerState
		? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Controller->PlayerState) : nullptr;
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::HandleInventoryChanged);
		InventoryComponent->OnCommandCompleted.AddUniqueDynamic(this, &ThisClass::HandleCommandCompleted);
	}
	if (AbilitySystemComponent)
	{
		InventoryOpenTagDelegateHandle = AbilitySystemComponent->RegisterGameplayTagEvent(
			ExtractionGameplayTags::Inventory_Open, EGameplayTagEventType::NewOrRemoved).AddUObject(
				this, &ThisClass::HandleInventoryOpenTagChanged);
		HandleInventoryOpenTagChanged(ExtractionGameplayTags::Inventory_Open,
			AbilitySystemComponent->GetTagCount(ExtractionGameplayTags::Inventory_Open));
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
	RefreshFromComponent();
}

void UExtractionInventoryWidget::NativeDestruct()
{
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &ThisClass::HandleInventoryChanged);
		InventoryComponent->OnCommandCompleted.RemoveDynamic(this, &ThisClass::HandleCommandCompleted);
	}
	if (AbilitySystemComponent && InventoryOpenTagDelegateHandle.IsValid())
	{
		AbilitySystemComponent->RegisterGameplayTagEvent(
			ExtractionGameplayTags::Inventory_Open,
			EGameplayTagEventType::NewOrRemoved).Remove(InventoryOpenTagDelegateHandle);
	}
	Super::NativeDestruct();
}

void UExtractionInventoryWidget::HandleInventoryOpenTagChanged(FGameplayTag, int32 NewCount)
{
	SetVisibility(NewCount > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void UExtractionInventoryWidget::HandleInventoryChanged(int32, int32)
{
	RefreshFromComponent();
}

void UExtractionInventoryWidget::HandleCommandCompleted(const FExtractionInventoryCommandResult& Result)
{
	if (CommandText)
	{
		CommandText->SetText(FText::Format(NSLOCTEXT("ExtractionOps", "InventoryResult",
			"请求 {0}：结果 {1}，版本 {2}"),
			FText::FromString(Result.RequestId.ToString(EGuidFormats::Short)),
			FText::AsNumber(static_cast<int32>(Result.Result)),
			FText::AsNumber(Result.AuthoritativeVersion)));
	}
}

void UExtractionInventoryWidget::RefreshFromComponent()
{
	if (!SlotsText)
	{
		return;
	}
	if (!InventoryComponent)
	{
		SlotsText->SetText(NSLOCTEXT("ExtractionOps", "InventoryUnavailable", "背包状态尚未就绪"));
		return;
	}
	int32 OccupiedSlotCount = 0;
	for (const FExtractionInventorySlot& InventorySlot : InventoryComponent->GetSlots())
	{
		OccupiedSlotCount += InventorySlot.Item.IsValid() ? 1 : 0;
	}
	FString Lines = FString::Printf(TEXT("Raid Inventory  %d/%d  v%d\n"), OccupiedSlotCount,
		UExtractionInventoryComponent::Capacity, InventoryComponent->GetInventoryVersion());
	for (const FExtractionInventorySlot& InventorySlot : InventoryComponent->GetSlots())
	{
		if (InventorySlot.Item.IsValid())
		{
			const UExtractionItemDefinition* Definition = InventorySlot.Item.Definition.LoadSynchronous();
			Lines += FString::Printf(TEXT("[%02d] %s x%d  %s\n"), InventorySlot.SlotIndex,
				Definition ? *Definition->DisplayName.ToString() : TEXT("<missing>"),
				InventorySlot.Item.Quantity, *InventorySlot.Item.InstanceId.ToString(EGuidFormats::Short));
		}
		else
		{
			Lines += FString::Printf(TEXT("[%02d] --\n"), InventorySlot.SlotIndex);
		}
	}
	SlotsText->SetText(FText::FromString(Lines));
}
