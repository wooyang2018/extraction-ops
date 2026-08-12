// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Inventory/ExtractionInventoryComponent.h"

#include "ExtractionInventoryWidget.generated.h"

class UTextBlock;
class UAbilitySystemComponent;

/** Event-driven inventory presentation. It never owns or mutates authoritative slot state. */
UCLASS(Abstract, BlueprintType)
class EXTRACTIONOPSRUNTIME_API UExtractionInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleInventoryChanged(int32 OldVersion, int32 NewVersion);

	UFUNCTION()
	void HandleCommandCompleted(const FExtractionInventoryCommandResult& Result);

	void RefreshFromComponent();
	void HandleInventoryOpenTagChanged(FGameplayTag Tag, int32 NewCount);

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotsText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CommandText;

	UPROPERTY(Transient, BlueprintReadOnly)
	TObjectPtr<UExtractionInventoryComponent> InventoryComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	FDelegateHandle InventoryOpenTagDelegateHandle;
};
