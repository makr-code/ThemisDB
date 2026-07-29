// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_exporters_phase23_hardening_focused.cpp
 * @brief Phase 2/3 — Exporters hardening focused tests (EXCH-17..EXCH-24).
 *
 * Tests are fully self-contained: no network I/O, no filesystem I/O.
 * The canonical PRNG seed is kExportersPhase23Seed = 42.
 *
 * ## Test families
 *
 * ### EXCH-17..18 — PolicyDeniedException contract
 *   EXCH-17  PolicyDeniedException is-a ExporterException with ERR_EXPORT_POLICY_DENIED
 *   EXCH-18  isResumableError(EXPORT_ABORTED) returns false (fail-closed contract)
 *
 * ### EXCH-19..22 — ExporterMetrics policy-denial + hub-upload-failure counters
 *   EXCH-19  recordPolicyDenial() increments getPolicyDenials() and "policy_denied" error type
 *   EXCH-20  recordHubUploadFailure(reason) increments getHubUploadFailures() and error type
 *   EXCH-21  toJson() includes "exporter_policy_denials_total" and "exporter_hub_upload_failures_total"
 *   EXCH-22  toString() includes "Policy Denials" and "Hub Upload Failures" lines
 *
 * ### EXCH-23..24 — PolicyDeniedException field round-trip + reset
 *   EXCH-23  PolicyDeniedException denial_reason and requesting_user are preserved
 *   EXCH-24  ExporterMetrics::reset() zeros policy_denials_ and hub_upload_failures_
 *
 * @see include/exporters/exporter_errors.h
 * @see include/exporters/exporter_metrics.h
 * @see include/exporters/exporters_api_contract.h
 * @see src/exporters/ROADMAP.md — Phase 2/3 items
 */

#include <gtest/gtest.h>

#include "exporters/exporter_errors.h"
#include "exporters/exporter_metrics.h"
#include "exporters/exporters_api_contract.h"

#include <cstdint>
#include <stdexcept>
#include <string>

using namespace themis::exporters;

namespace {

static constexpr uint64_t kExportersPhase23Seed = 42;

// ─────────────────────────────────────────────────────────────────────────────
// EXCH-17 — PolicyDeniedException is-a ExporterException with correct error code
// ─────────────────────────────────────────────────────────────────────────────

TEST(EXCH17_PolicyDeniedException, IsAExporterException) {
    (void)kExportersPhase23Seed;

    PolicyDeniedException ex("access not allowed", "alice", "my_collection");

    // Must be catchable as ExporterException
    bool caught_as_exporter = false;
    try {
        throw ex;
    } catch (const ExporterException &) {
        caught_as_exporter = true;
    }
    EXPECT_TRUE(caught_as_exporter) << "PolicyDeniedException must be catchable as ExporterException";

    // Must also be catchable as std::runtime_error
    bool caught_as_runtime = false;
    try {
        throw ex;
    } catch (const std::runtime_error &) {
        caught_as_runtime = true;
    }
    EXPECT_TRUE(caught_as_runtime) << "PolicyDeniedException must be catchable as std::runtime_error";

    // Error code must be ERR_EXPORT_POLICY_DENIED
    EXPECT_EQ(ex.getErrorCode(), themis::errors::ErrorCode::ERR_EXPORT_POLICY_DENIED);
}

// ─────────────────────────────────────────────────────────────────────────────
// EXCH-18 — isResumableError(EXPORT_ABORTED) == false (fail-closed contract)
// ─────────────────────────────────────────────────────────────────────────────

TEST(EXCH18_ResumableError, ExportAbortedIsNotResumable) {
    EXPECT_FALSE(isResumableError(ExporterErrorCode::EXPORT_ABORTED))
        << "EXPORT_ABORTED must be fail-closed: isResumableError must return false";
}

TEST(EXCH18_ResumableError, StreamInterruptedIsResumable) {
    // Positive check: ensure the resumable contract still works for the
    // one code that IS supposed to be resumable.
    EXPECT_TRUE(isResumableError(ExporterErrorCode::STREAM_INTERRUPTED));
}

// ─────────────────────────────────────────────────────────────────────────────
// EXCH-19 — recordPolicyDenial() increments counter and error type
// ─────────────────────────────────────────────────────────────────────────────

TEST(EXCH19_PolicyDenialMetric, IncrementsPolicyDenialsAndErrorType) {
    ExporterMetrics m;

    EXPECT_EQ(m.getPolicyDenials(), 0u);

    m.recordPolicyDenial("col_A", "user_bob");
    EXPECT_EQ(m.getPolicyDenials(), 1u);

    m.recordPolicyDenial("col_B", "user_carol");
    EXPECT_EQ(m.getPolicyDenials(), 2u);

    // Must also register in unified error taxonomy
    const auto by_type = m.getErrorsByType();
    const auto it      = by_type.find("policy_denied");
    ASSERT_NE(it, by_type.end()) << "'policy_denied' must appear in getErrorsByType()";
    EXPECT_EQ(it->second, 2u);

    // total_errors must count them
    EXPECT_GE(m.getTotalErrors(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// EXCH-20 — recordHubUploadFailure() increments counter and error type
// ─────────────────────────────────────────────────────────────────────────────

TEST(EXCH20_HubUploadFailureMetric, IncrementsCounterAndErrorType) {
    ExporterMetrics m;

    EXPECT_EQ(m.getHubUploadFailures(), 0u);

    m.recordHubUploadFailure("HTTP_401");
    EXPECT_EQ(m.getHubUploadFailures(), 1u);

    m.recordHubUploadFailure("retry_exhausted:shard0.parquet");
    EXPECT_EQ(m.getHubUploadFailures(), 2u);

    const auto by_type = m.getErrorsByType();
    const auto it      = by_type.find("hub_upload_failure");
    ASSERT_NE(it, by_type.end()) << "'hub_upload_failure' must appear in getErrorsByType()";
    EXPECT_EQ(it->second, 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// EXCH-21 — toJson() includes the canonical metric keys
// ─────────────────────────────────────────────────────────────────────────────

TEST(EXCH21_ToJson, IncludesPolicyAndHubKeys) {
    ExporterMetrics m;
    m.recordPolicyDenial("col_X", "user_dave");
    m.recordHubUploadFailure("HTTP_503");

    const auto j = m.toJson();

    ASSERT_TRUE(j.contains("exporter_policy_denials_total"))
        << "toJson() must include 'exporter_policy_denials_total'";
    EXPECT_EQ(j["exporter_policy_denials_total"].get<std::size_t>(), 1u);

    ASSERT_TRUE(j.contains("exporter_hub_upload_failures_total"))
        << "toJson() must include 'exporter_hub_upload_failures_total'";
    EXPECT_EQ(j["exporter_hub_upload_failures_total"].get<std::size_t>(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// EXCH-22 — toString() includes human-readable lines for both counters
// ─────────────────────────────────────────────────────────────────────────────

TEST(EXCH22_ToString, IncludesPolicyAndHubLines) {
    ExporterMetrics m;
    m.recordPolicyDenial("col_Y", "user_eve");
    m.recordHubUploadFailure("connection_reset");

    const std::string s = m.toString();

    EXPECT_NE(s.find("Policy Denials"), std::string::npos)
        << "toString() must include 'Policy Denials' line";
    EXPECT_NE(s.find("Hub Upload Failures"), std::string::npos)
        << "toString() must include 'Hub Upload Failures' line";
}

// ─────────────────────────────────────────────────────────────────────────────
// EXCH-23 — PolicyDeniedException field round-trip
// ─────────────────────────────────────────────────────────────────────────────

TEST(EXCH23_PolicyDeniedException, FieldsArePreserved) {
    const std::string reason     = "IP not in allowlist";
    const std::string user       = "user_frank";
    const std::string collection = "sensitive_collection";

    PolicyDeniedException ex(reason, user, collection);

    EXPECT_EQ(ex.getDenialReason(), reason);
    EXPECT_EQ(ex.getRequestingUser(), user);
    EXPECT_EQ(ex.getCollection(), collection);

    // what() must mention the reason
    const std::string what_msg = ex.what();
    EXPECT_NE(what_msg.find(reason), std::string::npos)
        << "what() must include the denial reason";
}

TEST(EXCH23_PolicyDeniedException, DefaultFieldsAreEmpty) {
    PolicyDeniedException ex("minimal");

    EXPECT_EQ(ex.getDenialReason(), "minimal");
    EXPECT_EQ(ex.getRequestingUser(), "");
    EXPECT_EQ(ex.getCollection(), "");
}

// ─────────────────────────────────────────────────────────────────────────────
// EXCH-24 — reset() zeros policy_denials_ and hub_upload_failures_
// ─────────────────────────────────────────────────────────────────────────────

TEST(EXCH24_Reset, ZerosBothNewCounters) {
    ExporterMetrics m;

    m.recordPolicyDenial("col_Z", "user_grace");
    m.recordPolicyDenial("col_Z", "user_grace");
    m.recordHubUploadFailure("HTTP_502");

    EXPECT_EQ(m.getPolicyDenials(), 2u);
    EXPECT_EQ(m.getHubUploadFailures(), 1u);

    m.reset();

    EXPECT_EQ(m.getPolicyDenials(), 0u)    << "reset() must zero policy_denials_";
    EXPECT_EQ(m.getHubUploadFailures(), 0u) << "reset() must zero hub_upload_failures_";
    EXPECT_EQ(m.getTotalErrors(), 0u)       << "reset() must zero total_errors_";
}

} // anonymous namespace
