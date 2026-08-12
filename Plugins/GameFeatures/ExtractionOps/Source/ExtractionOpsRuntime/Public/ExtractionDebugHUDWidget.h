// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"

#include "ExtractionDebugHUDWidget.generated.h"

class UTextBlock;

/** Read-only development HUD. Gameplay state remains owned by authoritative components. */
UCLASS(Abstract, Blueprintable)
class EXTRACTIONOPSRUNTIME_API UExtractionDebugHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Named widget supplied by WBP_ExtractionDebugHUD. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="Extraction Ops|Debug")
	TObjectPtr<UTextBlock> SnapshotText;

private:
	void RefreshSnapshotText();

	FTimerHandle RefreshTimerHandle;

	static constexpr float RefreshIntervalSeconds = 0.5f;
};
