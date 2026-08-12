// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "AbilitySystem/Attributes/LyraAttributeSet.h"
#include "AbilitySystemComponent.h"

#include "ExtractionArmorSet.generated.h"

/** Replicated armor attributes. Damage execution consumes Armor before Lyra Health. */
UCLASS(BlueprintType)
class EXTRACTIONOPSRUNTIME_API UExtractionArmorSet final : public ULyraAttributeSet
{
	GENERATED_BODY()

public:
	UExtractionArmorSet();

	ATTRIBUTE_ACCESSORS(UExtractionArmorSet, Armor);
	ATTRIBUTE_ACCESSORS(UExtractionArmorSet, MaxArmor);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

private:
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxArmor(const FGameplayAttributeData& OldValue);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Armor, Category="Extraction Ops|Armor", meta=(AllowPrivateAccess=true))
	FGameplayAttributeData Armor;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxArmor, Category="Extraction Ops|Armor", meta=(AllowPrivateAccess=true))
	FGameplayAttributeData MaxArmor;
};
