/**
 * @file test_updates_operator_diagnostics_focused.cpp
 * @brief Focused tests for the current OperatorDiagnostics API.
 */

#include <gtest/gtest.h>

#include "updates/updates_diagnostic_emitter.h"
#include "updates/updates_operator_diagnostics.h"

using namespace themis::updates;

TEST(OperatorDiagnosticsFocused, DetectScenarioAndNameAreUsable) {
    OperatorDiagnostics diagnostics;

    ErrorContext ctx;
    ctx.error_code = DiagnosticErrorCode::NETWORK_PARTITION;
    ctx.severity = DiagnosticSeverity::ERROR;
    ctx.root_cause = RootCauseClass::NETWORK;
    ctx.operation = "apply_patch";
    ctx.phase = "deploying";

    const auto scenario = diagnostics.detectScenario(ctx);
    const auto name = diagnostics.getScenarioName(scenario);

    EXPECT_FALSE(name.empty());
    EXPECT_TRUE(diagnostics.matchesScenario(scenario, ctx));
}

TEST(OperatorDiagnosticsFocused, RecoveryAndAlertingAreAvailable) {
    OperatorDiagnostics diagnostics;

    const auto rules = diagnostics.getAllAlertingRules();
    EXPECT_FALSE(rules.empty());

    const auto action = diagnostics.recommendRecoveryAction(ErrorContext{});
    (void)action;
}

TEST(OperatorDiagnosticsFocused, ScenarioNameForKnownScenarioIsReadable) {
    OperatorDiagnostics diagnostics;
    const auto name = diagnostics.getScenarioName(FailureScenario::COORDINATOR_UNREACHABLE);
    EXPECT_FALSE(name.empty());
}
