// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/LyraGameplayAbility.h"

#include "ExtractionGameplayAbility_ToggleInventory.generated.h"

/** Local-only UI intent. Authoritative inventory data remains on the PlayerState component. */
UCLASS()
class EXTRACTIONOPSRUNTIME_API UExtractionGameplayAbility_ToggleInventory final : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	UExtractionGameplayAbility_ToggleInventory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
