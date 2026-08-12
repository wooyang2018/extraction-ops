// Copyright Extraction Ops. All Rights Reserved.

#include "AbilitySystem/Executions/ExtractionDamageExecution.h"

#include "AbilitySystem/Attributes/ExtractionArmorSet.h"
#include "AbilitySystem/Attributes/LyraCombatSet.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "AbilitySystem/LyraAbilitySourceInterface.h"
#include "AbilitySystem/LyraGameplayEffectContext.h"
#include "Engine/World.h"
#include "Teams/LyraTeamSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionDamageExecution)

namespace ExtractionDamage
{
	struct FStatics
	{
		FGameplayEffectAttributeCaptureDefinition BaseDamage;
		FGameplayEffectAttributeCaptureDefinition Armor;

		FStatics()
			: BaseDamage(ULyraCombatSet::GetBaseDamageAttribute(), EGameplayEffectAttributeCaptureSource::Source, true)
			, Armor(UExtractionArmorSet::GetArmorAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
		{
		}
	};

	const FStatics& Statics()
	{
		static FStatics Value;
		return Value;
	}
}

FExtractionDamageSplit FExtractionDamageRules::SplitDamage(float Damage, float CurrentArmor)
{
	const float ClampedDamage = FMath::Max(Damage, 0.0f);
	FExtractionDamageSplit Result;
	Result.ArmorDamage = FMath::Min(FMath::Max(CurrentArmor, 0.0f), ClampedDamage);
	Result.HealthDamage = ClampedDamage - Result.ArmorDamage;
	return Result;
}

UExtractionDamageExecution::UExtractionDamageExecution()
{
	RelevantAttributesToCapture.Add(ExtractionDamage::Statics().BaseDamage);
	RelevantAttributesToCapture.Add(ExtractionDamage::Statics().Armor);
}

void UExtractionDamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
#if WITH_SERVER_CODE
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const FLyraGameplayEffectContext* Context = FLyraGameplayEffectContext::ExtractEffectContext(Spec.GetContext());
	if (!Context)
	{
		return;
	}

	FAggregatorEvaluateParameters Evaluation;
	Evaluation.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	Evaluation.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float BaseDamage = 0.0f;
	float CurrentArmor = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ExtractionDamage::Statics().BaseDamage, Evaluation, BaseDamage);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ExtractionDamage::Statics().Armor, Evaluation, CurrentArmor);

	AActor* HitActor = nullptr;
	FVector ImpactLocation = FVector::ZeroVector;
	if (const FHitResult* Hit = Context->GetHitResult())
	{
		HitActor = Hit->HitObjectHandle.FetchActor();
		ImpactLocation = Hit->ImpactPoint;
	}
	if (!HitActor)
	{
		if (UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent())
		{
			HitActor = TargetASC->GetAvatarActor_Direct();
			ImpactLocation = HitActor ? HitActor->GetActorLocation() : FVector::ZeroVector;
		}
	}

	float AllowedMultiplier = 0.0f;
	const AActor* EffectCauser = Context->GetEffectCauser();
	if (HitActor)
	{
		if (ULyraTeamSubsystem* Teams = HitActor->GetWorld()->GetSubsystem<ULyraTeamSubsystem>())
		{
			AllowedMultiplier = Teams->CanCauseDamage(EffectCauser, HitActor) ? 1.0f : 0.0f;
		}
	}

	double Distance = WORLD_MAX;
	if (Context->HasOrigin())
	{
		Distance = FVector::Dist(Context->GetOrigin(), ImpactLocation);
	}
	else if (EffectCauser)
	{
		Distance = FVector::Dist(EffectCauser->GetActorLocation(), ImpactLocation);
	}

	float DistanceAttenuation = 1.0f;
	float MaterialAttenuation = 1.0f;
	const UObject* SourceObject = Spec.GetContext().GetSourceObject();
	if (const ILyraAbilitySourceInterface* Source = Cast<ILyraAbilitySourceInterface>(SourceObject))
	{
		DistanceAttenuation = Source->GetDistanceAttenuation(Distance, Evaluation.SourceTags, Evaluation.TargetTags);
		const FHitResult* ContextHit = Spec.GetContext().GetHitResult();
		if (const UPhysicalMaterial* PhysicalMaterial = ContextHit ? ContextHit->PhysMaterial.Get() : nullptr)
		{
			MaterialAttenuation = Source->GetPhysicalMaterialAttenuation(PhysicalMaterial,
				Evaluation.SourceTags, Evaluation.TargetTags);
		}
	}

	const float Damage = FMath::Max(BaseDamage * FMath::Max(DistanceAttenuation, 0.0f)
		* MaterialAttenuation * AllowedMultiplier, 0.0f);
	const FExtractionDamageSplit Split = FExtractionDamageRules::SplitDamage(Damage, CurrentArmor);

	if (Split.ArmorDamage > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UExtractionArmorSet::GetArmorAttribute(), EGameplayModOp::Additive, -Split.ArmorDamage));
	}
	if (Split.HealthDamage > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			ULyraHealthSet::GetDamageAttribute(), EGameplayModOp::Additive, Split.HealthDamage));
	}
#endif
}
