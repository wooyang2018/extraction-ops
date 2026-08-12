// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "GameplayEffectExecutionCalculation.h"

#include "ExtractionDamageExecution.generated.h"

struct EXTRACTIONOPSRUNTIME_API FExtractionDamageSplit
{
	float ArmorDamage = 0.0f;
	float HealthDamage = 0.0f;
};

struct EXTRACTIONOPSRUNTIME_API FExtractionDamageRules
{
	static FExtractionDamageSplit SplitDamage(float Damage, float CurrentArmor);
};

/** Lyra-compatible damage calculation that consumes Extraction Armor before Health. */
UCLASS()
class EXTRACTIONOPSRUNTIME_API UExtractionDamageExecution final : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UExtractionDamageExecution();
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
