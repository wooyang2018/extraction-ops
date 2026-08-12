// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "ExtractionRaidLifecycleComponent.h"

#include "ExtractionResultWidget.generated.h"

class UTextBlock;

/** Owner-only result presentation backed solely by the replicated server snapshot. */
UCLASS(Abstract, BlueprintType)
class EXTRACTIONOPSRUNTIME_API UExtractionResultWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleResultChanged(const FExtractionRunResultSnapshot& Result);

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultText;

	UPROPERTY(Transient)
	TObjectPtr<UExtractionRaidLifecycleComponent> LifecycleComponent;
};
