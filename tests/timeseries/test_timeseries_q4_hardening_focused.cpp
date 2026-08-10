// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_timeseries_q4_hardening_focused.cpp
 * @brief Timeseries Q4 2026 Phase 2/3 hardening focused test suite (TEH-01..TEH-20).
 *
 * Verifies deterministic fail-safe behavior for remote-write validation, encrypted
 * chunk edge cases, buffer pressure, and operator diagnostics.
 *
 * ## Test families
 *
 * ### TEH-01..04 — Remote-write endpoint validation
 *   TEH-01  Empty URL → REMOTE_WRITE_VALIDATION_ERROR
 *   TEH-02  Non-http/https URL → REMOTE_WRITE_VALIDATION_ERROR
 *   TEH-03  Valid https URL → success (nullopt)
 *   TEH-04  URL exceeding max length → REMOTE_WRITE_VALIDATION_ERROR
 *
 * ### TEH-05..08 — Remote-write timeout handling
 *   TEH-05  Timeout handler returns REMOTE_WRITE_RETRIES_EXHAUSTED
 *   TEH-06  Timeout counter increments per call
 *   TEH-07  Incident callback invoked on timeout
 *   TEH-08  URL with control characters → REMOTE_WRITE_VALIDATION_ERROR
 *
 * ### TEH-09..12 — Encryption key validation
 *   TEH-09  Null key → ENCRYPTION_STATE_INVALID
 *   TEH-10  Zero-length key → ENCRYPTION_STATE_INVALID
 *   TEH-11  Invalid key length (e.g. 20) → ENCRYPTION_STATE_INVALID
 *   TEH-12  Valid 16/24/32-byte key → success (nullopt)
 *
 * ### TEH-13..16 — Encryption key rotation
 *   TEH-13  Rotation with invalid key → ENCRYPTION_KEY_NOT_FOUND
 *   TEH-14  Rotation with valid key → success; rotation counter incremented
 *   TEH-15  Rotation incident callback invoked on valid rotation
 *   TEH-16  Multiple rotations increment counter correctly
 *
 * ### TEH-17..20 — Buffer pressure + operator diagnostics
 *   TEH-17  Buffer below kMaxOutOfOrderBuffer → flush not required
 *   TEH-18  Buffer at kMaxOutOfOrderBuffer → flush required
 *   TEH-19  TsOperatorDiagnostics records incident with correct severity
 *   TEH-20  formatSummary includes incident details
 *
 * @see include/timeseries/ts_edge_case_handler.h
 * @see include/timeseries/ts_operator_diagnostics.h
 * @see src/timeseries/ROADMAP.md — Phase 2/3 Q4 2026 items
 */

#include <gtest/gtest.h>

#include "timeseries/ts_edge_case_handler.h"
#include "timeseries/ts_operator_diagnostics.h"

#include <cstdint>
#include <string>
#include <vector>

namespace themis {
namespace timeseries {
namespace test {

/// Canonical PRNG seed for all TEH tests.
static constexpr uint64_t kTehSeed = 42;

// ============================================================================
// § 1  Remote-Write Endpoint Validation (TEH-01..04)
// ============================================================================

/**
 * @test TEH-01: Empty endpoint URL returns REMOTE_WRITE_VALIDATION_ERROR.
 */
TEST(TimeseriesQ4Hardening, TEH01_EmptyEndpointReturnsValidationError) {
    TsEdgeCaseHandler handler;
    auto result = handler.validateRemoteWriteEndpoint("");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, TimeseriesErrorCode::REMOTE_WRITE_VALIDATION_ERROR);
    EXPECT_EQ(handler.validationFailureCount(), 1u);
}

/**
 * @test TEH-02: Non-http/https URL returns REMOTE_WRITE_VALIDATION_ERROR.
 */
TEST(TimeseriesQ4Hardening, TEH02_InvalidProtocolReturnsError) {
    TsEdgeCaseHandler handler;
    auto result = handler.validateRemoteWriteEndpoint("ftp://example.com/write");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, TimeseriesErrorCode::REMOTE_WRITE_VALIDATION_ERROR);
}

/**
 * @test TEH-03: Valid https URL returns success (nullopt).
 */
TEST(TimeseriesQ4Hardening, TEH03_ValidHttpsEndpointReturnsSuccess) {
    TsEdgeCaseHandler handler;
    auto result = handler.validateRemoteWriteEndpoint(
        "https://prometheus.example.com/api/v1/write");
    EXPECT_FALSE(result.has_value()) << "Expected success (nullopt) for valid URL";
    EXPECT_EQ(handler.validationFailureCount(), 0u);
}

/**
 * @test TEH-04: URL exceeding kMaxEndpointLength returns REMOTE_WRITE_VALIDATION_ERROR.
 */
TEST(TimeseriesQ4Hardening, TEH04_TooLongEndpointReturnsError) {
    TsEdgeCaseHandler handler;
    // Build a URL of 2049 characters.
    std::string long_url = "https://";
    long_url.append(2050, 'a');
    long_url += ".com/write";
    auto result = handler.validateRemoteWriteEndpoint(long_url);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, TimeseriesErrorCode::REMOTE_WRITE_VALIDATION_ERROR);
}

// ============================================================================
// § 2  Remote-Write Timeout Handling (TEH-05..08)
// ============================================================================

/**
 * @test TEH-05: Timeout handler returns REMOTE_WRITE_RETRIES_EXHAUSTED.
 */
TEST(TimeseriesQ4Hardening, TEH05_TimeoutHandlerReturnsRetriesExhausted) {
    TsEdgeCaseHandler handler;
    auto result = handler.handleRemoteWriteTimeout("https://prom.example.com/write", 5000);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, TimeseriesErrorCode::REMOTE_WRITE_RETRIES_EXHAUSTED);
}

/**
 * @test TEH-06: Each timeout call increments the timeout counter.
 */
TEST(TimeseriesQ4Hardening, TEH06_TimeoutCounterIncrementsPerCall) {
    TsEdgeCaseHandler handler;
    handler.handleRemoteWriteTimeout("https://a.com/write", 1000);
    handler.handleRemoteWriteTimeout("https://b.com/write", 2000);
    handler.handleRemoteWriteTimeout("https://c.com/write", 3000);
    EXPECT_EQ(handler.remoteWriteTimeoutCount(), 3u);
}

/**
 * @test TEH-07: Incident callback is invoked on remote-write timeout.
 */
TEST(TimeseriesQ4Hardening, TEH07_IncidentCallbackInvokedOnTimeout) {
    std::vector<std::string> captured_ids;
    TsEdgeCaseHandler handler([&](std::string_view id, std::string_view) {
        captured_ids.emplace_back(id);
    });
    handler.handleRemoteWriteTimeout("https://prom.example.com/write", 5000);
    ASSERT_FALSE(captured_ids.empty());
    EXPECT_NE(std::find_if(captured_ids.begin(), captured_ids.end(),
                           [](const std::string& s) {
                               return s.find("TIMEOUT") != std::string::npos;
                           }),
              captured_ids.end());
}

/**
 * @test TEH-08: URL containing control characters returns REMOTE_WRITE_VALIDATION_ERROR.
 */
TEST(TimeseriesQ4Hardening, TEH08_ControlCharInUrlReturnsError) {
    TsEdgeCaseHandler handler;
    std::string url = "https://example.com/wri\x01te";
    auto result = handler.validateRemoteWriteEndpoint(url);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, TimeseriesErrorCode::REMOTE_WRITE_VALIDATION_ERROR);
}

// ============================================================================
// § 3  Encryption Key Validation (TEH-09..12)
// ============================================================================

/**
 * @test TEH-09: Null key pointer returns ENCRYPTION_STATE_INVALID.
 */
TEST(TimeseriesQ4Hardening, TEH09_NullKeyReturnsEncryptionError) {
    TsEdgeCaseHandler handler;
    auto result = handler.validateEncryptionKey(nullptr, 16);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, TimeseriesErrorCode::ENCRYPTION_STATE_INVALID);
}

/**
 * @test TEH-10: Zero-length key returns ENCRYPTION_STATE_INVALID.
 */
TEST(TimeseriesQ4Hardening, TEH10_ZeroLengthKeyReturnsEncryptionError) {
    TsEdgeCaseHandler handler;
    const uint8_t dummy = 0;
    auto result = handler.validateEncryptionKey(&dummy, 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, TimeseriesErrorCode::ENCRYPTION_STATE_INVALID);
}

/**
 * @test TEH-11: Invalid key length (e.g. 20 bytes) returns ENCRYPTION_STATE_INVALID.
 */
TEST(TimeseriesQ4Hardening, TEH11_InvalidKeyLengthReturnsEncryptionError) {
    TsEdgeCaseHandler handler;
    std::vector<uint8_t> key(20, 0xAB);
    auto result = handler.validateEncryptionKey(key.data(), key.size());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, TimeseriesErrorCode::ENCRYPTION_STATE_INVALID);
}

/**
 * @test TEH-12: Valid AES key lengths (16/24/32) return success (nullopt).
 */
TEST(TimeseriesQ4Hardening, TEH12_ValidKeyLengthsReturnSuccess) {
    TsEdgeCaseHandler handler;
    for (std::size_t len : {16u, 24u, 32u}) {
        std::vector<uint8_t> key(len, 0x42);
        auto result = handler.validateEncryptionKey(key.data(), key.size());
        EXPECT_FALSE(result.has_value())
            << "Expected success for key length " << len;
    }
    EXPECT_EQ(handler.validationFailureCount(), 0u);
}

// ============================================================================
// § 4  Encryption Key Rotation (TEH-13..16)
// ============================================================================

/**
 * @test TEH-13: Rotation with invalid new key returns ENCRYPTION_KEY_NOT_FOUND.
 */
TEST(TimeseriesQ4Hardening, TEH13_RotationWithInvalidKeyReturnsError) {
    TsEdgeCaseHandler handler;
    const uint8_t bad_key[20] = {};
    auto result = handler.handleKeyRotationDuringWrite(bad_key, sizeof(bad_key));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, TimeseriesErrorCode::ENCRYPTION_KEY_NOT_FOUND);
    EXPECT_EQ(handler.keyRotationCount(), 0u) << "Counter must not increment on failure";
}

/**
 * @test TEH-14: Rotation with valid 32-byte key returns success and increments counter.
 */
TEST(TimeseriesQ4Hardening, TEH14_RotationWithValidKeySucceeds) {
    TsEdgeCaseHandler handler;
    const uint8_t key[32] = {};
    auto result = handler.handleKeyRotationDuringWrite(key, sizeof(key));
    EXPECT_FALSE(result.has_value()) << "Expected success for valid key rotation";
    EXPECT_EQ(handler.keyRotationCount(), 1u);
}

/**
 * @test TEH-15: Incident callback is invoked on successful key rotation.
 */
TEST(TimeseriesQ4Hardening, TEH15_IncidentCallbackInvokedOnValidRotation) {
    std::vector<std::string> captured_ids;
    TsEdgeCaseHandler handler([&](std::string_view id, std::string_view) {
        captured_ids.emplace_back(id);
    });
    const uint8_t key[16] = {};
    handler.handleKeyRotationDuringWrite(key, sizeof(key));
    ASSERT_FALSE(captured_ids.empty());
    EXPECT_NE(std::find_if(captured_ids.begin(), captured_ids.end(),
                           [](const std::string& s) {
                               return s.find("ROTATION") != std::string::npos;
                           }),
              captured_ids.end());
}

/**
 * @test TEH-16: Multiple successful rotations increment counter correctly.
 */
TEST(TimeseriesQ4Hardening, TEH16_MultipleRotationsIncrementCounter) {
    TsEdgeCaseHandler handler;
    const uint8_t key[16] = {};
    for (int i = 0; i < 5; ++i) {
        handler.handleKeyRotationDuringWrite(key, sizeof(key));
    }
    EXPECT_EQ(handler.keyRotationCount(), 5u);
}

// ============================================================================
// § 5  Buffer Pressure + Operator Diagnostics (TEH-17..20)
// ============================================================================

/**
 * @test TEH-17: Buffer below kMaxOutOfOrderBuffer does not require flush.
 */
TEST(TimeseriesQ4Hardening, TEH17_BufferBelowCapacityNoFlushRequired) {
    TsEdgeCaseHandler handler;
    EXPECT_FALSE(handler.isOutOfOrderFlushRequired(0));
    EXPECT_FALSE(handler.isOutOfOrderFlushRequired(kMaxOutOfOrderBuffer - 1));
}

/**
 * @test TEH-18: Buffer at kMaxOutOfOrderBuffer triggers flush requirement.
 */
TEST(TimeseriesQ4Hardening, TEH18_BufferAtCapacityRequiresFlush) {
    TsEdgeCaseHandler handler;
    EXPECT_TRUE(handler.isOutOfOrderFlushRequired(kMaxOutOfOrderBuffer));
    EXPECT_TRUE(handler.isOutOfOrderFlushRequired(kMaxOutOfOrderBuffer + 100));
}

/**
 * @test TEH-19: TsOperatorDiagnostics records incident with correct severity.
 */
TEST(TimeseriesQ4Hardening, TEH19_OperatorDiagnosticsRecordsIncidentCorrectly) {
    TsOperatorDiagnostics diag;
    diag.recordIncident("TS-ECH-RW-TIMEOUT", TsIncidentSeverity::ERROR,
                        "timeout after 5000 ms", "Check endpoint",
                        TimeseriesErrorCode::REMOTE_WRITE_RETRIES_EXHAUSTED);
    EXPECT_EQ(diag.totalIncidentCount(), 1u);
    auto recent = diag.recentIncidents(5);
    ASSERT_EQ(recent.size(), 1u);
    EXPECT_EQ(recent[0].incident_id, "TS-ECH-RW-TIMEOUT");
    EXPECT_EQ(recent[0].severity, TsIncidentSeverity::ERROR);
    ASSERT_TRUE(recent[0].error_code.has_value());
    EXPECT_EQ(recent[0].error_code.value(), TimeseriesErrorCode::REMOTE_WRITE_RETRIES_EXHAUSTED);
}

/**
 * @test TEH-20: formatSummary includes incident ID and severity.
 */
TEST(TimeseriesQ4Hardening, TEH20_FormatSummaryIncludesIncidentDetails) {
    TsOperatorDiagnostics diag;
    diag.recordIncident("TS-ECH-ENC-ROTATION-INVALID",
                        TsIncidentSeverity::CRITICAL,
                        "key rotation rejected", "Provide valid AES key");
    auto summary = diag.formatSummary(5);
    EXPECT_FALSE(summary.empty());
    EXPECT_NE(summary.find("TS-ECH-ENC-ROTATION-INVALID"), std::string::npos)
        << "Summary must include incident ID";
    EXPECT_NE(summary.find("CRITICAL"), std::string::npos)
        << "Summary must include severity";
    EXPECT_TRUE(diag.hasCriticalIncidents());
}

} // namespace test
} // namespace timeseries
} // namespace themis
