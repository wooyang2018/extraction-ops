// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ExtractionNetworkValidation.generated.h"

UENUM(BlueprintType)
enum class EExtractionShotRejectionReason : uint8
{
	None, InvalidOwner, WeaponNotEquipped, BlockedByState, NoAmmo,
	FireRateLimited, InvalidOrigin, InvalidDirection, OutOfRange, DuplicateRequest
};

USTRUCT(BlueprintType)
struct EXTRACTIONOPSRUNTIME_API FExtractionShotValidationInput
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bOwnedByRequestingConnection = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bWeaponEquipped = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bBlockedByState = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 AuthoritativeAmmo = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) double SecondsSinceLastAcceptedShot = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) double MinimumFireInterval = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float OriginErrorCentimeters = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaximumOriginErrorCentimeters = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float DirectionDot = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MinimumDirectionDot = 0.7f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RequestedTraceDistance = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaximumTraceDistance = 10000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bDuplicateRequest = false;
};

/** Project audit contract around Lyra's authoritative Ranged Ability/TargetData path. */
struct EXTRACTIONOPSRUNTIME_API FExtractionNetworkValidation
{
	static EExtractionShotRejectionReason ValidateShot(const FExtractionShotValidationInput& Input);
	static const TCHAR* ToStableReason(EExtractionShotRejectionReason Reason);
};
