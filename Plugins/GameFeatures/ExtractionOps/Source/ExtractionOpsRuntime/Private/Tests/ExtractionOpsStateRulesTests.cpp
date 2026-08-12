// Copyright Extraction Ops. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "ExtractionDebugDataLibrary.h"
#include "ExtractionOpsTypes.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExtractionDebugSnapshotInvalidContextTest,
	"ExtractionOps.DebugSnapshot.InvalidContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FExtractionDebugSnapshotInvalidContextTest::RunTest(const FString& Parameters)
{
	FExtractionNetworkDebugSnapshot Snapshot;
	Snapshot.NetMode = TEXT("Stale");
	Snapshot.bHasAuthority = true;

	TestFalse(TEXT("A null world context cannot produce a snapshot"),
		UExtractionDebugDataLibrary::GetNetworkDebugSnapshot(nullptr, nullptr, nullptr, Snapshot));
	TestTrue(TEXT("Failure resets the output snapshot"), Snapshot.NetMode.IsEmpty());
	TestFalse(TEXT("Failure cannot preserve stale authority"), Snapshot.bHasAuthority);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExtractionMatchStateRulesTest,
	"ExtractionOps.StateRules.Match",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FExtractionMatchStateRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Loading can enter raid"), FExtractionStateRules::CanTransitionMatch(
		EExtractionMatchState::Loading, EExtractionMatchState::InRaid));
	TestTrue(TEXT("Raid can complete"), FExtractionStateRules::CanTransitionMatch(
		EExtractionMatchState::InRaid, EExtractionMatchState::Completed));
	TestFalse(TEXT("Completed is terminal"), FExtractionStateRules::CanTransitionMatch(
		EExtractionMatchState::Completed, EExtractionMatchState::InRaid));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExtractionRunStateRulesTest,
	"ExtractionOps.StateRules.Run",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FExtractionRunStateRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Raid can start extraction"), FExtractionStateRules::CanTransitionRun(
		EExtractionRunState::InRaid, EExtractionRunState::Extracting));
	TestTrue(TEXT("Extraction can be cancelled"), FExtractionStateRules::CanTransitionRun(
		EExtractionRunState::Extracting, EExtractionRunState::InRaid));
	TestTrue(TEXT("Extraction can succeed"), FExtractionStateRules::CanTransitionRun(
		EExtractionRunState::Extracting, EExtractionRunState::Extracted));
	TestFalse(TEXT("Extracted is terminal"), FExtractionStateRules::CanTransitionRun(
		EExtractionRunState::Extracted, EExtractionRunState::InRaid));
	TestFalse(TEXT("Death cannot become extraction"), FExtractionStateRules::CanTransitionRun(
		EExtractionRunState::Dead, EExtractionRunState::Extracting));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExtractionThreatRulesTest,
	"ExtractionOps.StateRules.Threat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FExtractionThreatRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("No terminal means no threat"),
		FExtractionStateRules::GetThreatLevelForTerminalCount(0), EExtractionThreatLevel::None);
	TestEqual(TEXT("First terminal sets low threat"),
		FExtractionStateRules::GetThreatLevelForTerminalCount(1), EExtractionThreatLevel::Low);
	TestEqual(TEXT("Second terminal sets high threat"),
		FExtractionStateRules::GetThreatLevelForTerminalCount(2), EExtractionThreatLevel::High);
	TestEqual(TEXT("Third terminal sets critical threat"),
		FExtractionStateRules::GetThreatLevelForTerminalCount(3), EExtractionThreatLevel::Critical);
	TestEqual(TEXT("Critical reward is capped"),
		FExtractionStateRules::GetRewardMultiplier(EExtractionThreatLevel::Critical), 2.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExtractionZoneStateRulesTest,
	"ExtractionOps.StateRules.Zone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FExtractionZoneStateRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Terminal unlocks the selected zone"), FExtractionStateRules::CanTransitionZone(
		EExtractionZoneState::Locked, EExtractionZoneState::Available));
	TestTrue(TEXT("An available zone can start a countdown"), FExtractionStateRules::CanTransitionZone(
		EExtractionZoneState::Available, EExtractionZoneState::Countdown));
	TestTrue(TEXT("A countdown can succeed"), FExtractionStateRules::CanTransitionZone(
		EExtractionZoneState::Countdown, EExtractionZoneState::Extracted));
	TestTrue(TEXT("A cancelled countdown can accept another player"), FExtractionStateRules::CanTransitionZone(
		EExtractionZoneState::Cancelled, EExtractionZoneState::Available));
	TestFalse(TEXT("A locked zone cannot skip the unlock requirement"), FExtractionStateRules::CanTransitionZone(
		EExtractionZoneState::Locked, EExtractionZoneState::Countdown));
	return true;
}

#endif
