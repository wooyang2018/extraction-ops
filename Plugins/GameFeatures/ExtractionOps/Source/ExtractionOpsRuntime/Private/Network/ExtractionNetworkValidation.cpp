// Copyright Extraction Ops. All Rights Reserved.

#include "Network/ExtractionNetworkValidation.h"

EExtractionShotRejectionReason FExtractionNetworkValidation::ValidateShot(const FExtractionShotValidationInput& Input)
{
	if (!Input.bOwnedByRequestingConnection) return EExtractionShotRejectionReason::InvalidOwner;
	if (!Input.bWeaponEquipped) return EExtractionShotRejectionReason::WeaponNotEquipped;
	if (Input.bBlockedByState) return EExtractionShotRejectionReason::BlockedByState;
	if (Input.AuthoritativeAmmo <= 0) return EExtractionShotRejectionReason::NoAmmo;
	if (Input.SecondsSinceLastAcceptedShot + UE_DOUBLE_SMALL_NUMBER < Input.MinimumFireInterval)
		return EExtractionShotRejectionReason::FireRateLimited;
	if (Input.OriginErrorCentimeters > Input.MaximumOriginErrorCentimeters)
		return EExtractionShotRejectionReason::InvalidOrigin;
	if (Input.DirectionDot < Input.MinimumDirectionDot)
		return EExtractionShotRejectionReason::InvalidDirection;
	if (Input.RequestedTraceDistance > Input.MaximumTraceDistance)
		return EExtractionShotRejectionReason::OutOfRange;
	if (Input.bDuplicateRequest) return EExtractionShotRejectionReason::DuplicateRequest;
	return EExtractionShotRejectionReason::None;
}

const TCHAR* FExtractionNetworkValidation::ToStableReason(EExtractionShotRejectionReason Reason)
{
	switch (Reason)
	{
	case EExtractionShotRejectionReason::None: return TEXT("accepted");
	case EExtractionShotRejectionReason::InvalidOwner: return TEXT("invalid_owner");
	case EExtractionShotRejectionReason::WeaponNotEquipped: return TEXT("weapon_not_equipped");
	case EExtractionShotRejectionReason::BlockedByState: return TEXT("blocked_by_state");
	case EExtractionShotRejectionReason::NoAmmo: return TEXT("no_ammo");
	case EExtractionShotRejectionReason::FireRateLimited: return TEXT("fire_rate_limited");
	case EExtractionShotRejectionReason::InvalidOrigin: return TEXT("invalid_origin");
	case EExtractionShotRejectionReason::InvalidDirection: return TEXT("invalid_direction");
	case EExtractionShotRejectionReason::OutOfRange: return TEXT("out_of_range");
	case EExtractionShotRejectionReason::DuplicateRequest: return TEXT("duplicate_request");
	default: return TEXT("unknown");
	}
}
