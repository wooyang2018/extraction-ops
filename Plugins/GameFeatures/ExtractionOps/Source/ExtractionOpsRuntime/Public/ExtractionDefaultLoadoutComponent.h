// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "Components/ControllerComponent.h"

#include "ExtractionDefaultLoadoutComponent.generated.h"

/** Server-only bridge that grants the two Vertical Slice test weapons to a player controller. */
UCLASS(meta=(BlueprintSpawnableComponent))
class EXTRACTIONOPSRUNTIME_API UExtractionDefaultLoadoutComponent final : public UControllerComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void TryGrantDefaultLoadout();

	FTimerHandle GrantRetryTimerHandle;
	int32 RemainingGrantAttempts = 20;
};
