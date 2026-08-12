// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionOption.h"
#include "Inventory/ExtractionInventoryComponent.h"

#include "ExtractionWorldPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/** Replicated world container for exactly one raid item instance. */
UCLASS(BlueprintType, Blueprintable)
class EXTRACTIONOPSRUNTIME_API AExtractionWorldPickup : public AActor, public IInteractableTarget
{
	GENERATED_BODY()

public:
	AExtractionWorldPickup(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery,
		FInteractionOptionBuilder& InteractionBuilder) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Extraction Ops|Pickup")
	bool InitializePickup(const FExtractionRaidItem& InItem);

	EExtractionInventoryResult TryClaim(AController* RequestingController, FGuid RequestId);

	UFUNCTION(BlueprintPure, Category="Extraction Ops|Pickup")
	const FExtractionRaidItem& GetItem() const { return Item; }

	UFUNCTION(BlueprintPure, Category="Extraction Ops|Pickup")
	bool IsAvailable() const { return bAvailable; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Extraction Ops|Pickup")
	TObjectPtr<USphereComponent> InteractionVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Extraction Ops|Pickup")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(EditDefaultsOnly, Category="Extraction Ops|Pickup")
	float MaximumClaimDistance = 350.0f;

private:
	bool ValidateClaim(const AController* RequestingController, EExtractionInventoryResult& OutFailure) const;

	UPROPERTY(Replicated)
	FExtractionRaidItem Item;

	UPROPERTY(Replicated)
	bool bAvailable = false;
};
