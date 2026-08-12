// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionOption.h"
#include "Inventory/ExtractionInventoryComponent.h"

#include "ExtractionLootContainer.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/** Server-generated, replicated loot list. Items are removed exactly once by instance ID. */
UCLASS(BlueprintType, Blueprintable)
class EXTRACTIONOPSRUNTIME_API AExtractionLootContainer : public AActor, public IInteractableTarget
{
	GENERATED_BODY()

public:
	AExtractionLootContainer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery,
		FInteractionOptionBuilder& InteractionBuilder) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Extraction Ops|Loot")
	bool InitializeItems(const TArray<FExtractionRaidItem>& InItems, FGuid InContainerId = FGuid());

	EExtractionInventoryResult TryClaimItem(AController* RequestingController,
		FGuid ItemInstanceId, FGuid RequestId);
	EExtractionInventoryResult TryClaimFirstAvailable(AController* RequestingController, FGuid RequestId);

	UFUNCTION(BlueprintPure, Category="Extraction Ops|Loot")
	const TArray<FExtractionRaidItem>& GetItems() const { return Items; }

	UFUNCTION(BlueprintPure, Category="Extraction Ops|Loot")
	FGuid GetContainerId() const { return ContainerId; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Extraction Ops|Loot")
	TObjectPtr<UBoxComponent> InteractionVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Extraction Ops|Loot")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	/** Static authoring list. Runtime GUIDs are generated once by the server. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Extraction Ops|Loot")
	TArray<TSoftObjectPtr<UExtractionItemDefinition>> InitialDefinitions;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Extraction Ops|Loot", meta=(ClampMin="1"))
	int32 InitialQuantityPerDefinition = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Extraction Ops|Loot")
	float MaximumClaimDistance = 350.0f;

private:
	bool ValidateClaim(const AController* RequestingController, EExtractionInventoryResult& OutFailure) const;

	UPROPERTY(Replicated)
	FGuid ContainerId;

	UPROPERTY(Replicated)
	TArray<FExtractionRaidItem> Items;

	TSet<FGuid> ProcessedRequestIds;
};
