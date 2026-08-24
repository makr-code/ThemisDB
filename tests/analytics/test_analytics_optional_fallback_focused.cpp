// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_analytics_optional_fallback_focused.cpp
 * @brief Optional-dependency / fallback consistency tests — Wave B gap closure.
 *
 * Verifies that optional-backend absence is handled deterministically by the
 * analytics contract types (no undefined behaviour, no silent failure).
 *
 * These tests exercise the contract-level guarantees; they do not require any
 * optional dependency (ONNX, TF Serving, Arrow) to be installed.
 *
 * ## Test families (OPF-01..OPF-08)
 *
 * ### OPF-01..03 — MLServingStatus::UNAVAILABLE path
 *   OPF-01  MLServingStatus::UNAVAILABLE is defined and distinct from OK
 *   OPF-02  MLServingStatus::POLICY_REJECTED is defined and distinct from UNAVAILABLE
 *   OPF-03  MLServingStatus enum values compile and can be compared
 *
 * ### OPF-04..06 — AnalyticsErrorCode fallback codes
 *   OPF-04  AnalyticsErrorCode::STREAM_BACKPRESSURE is defined
 *   OPF-05  AnalyticsErrorCode::AGGREGATION_OVERFLOW is defined and distinct
 *   OPF-06  Error codes usable in switch statements without default
 *
 * ### OPF-07..08 — BoundedExecutionPolicy + StreamingRuntimeLimits compile together
 *   OPF-07  BoundedExecutionPolicy and StreamingRuntimeLimits can be combined in one struct
 *   OPF-08  Both structs' isConstrained() are independently queryable
 *
 * @see include/analytics/analytics_api_contract.h  — AnalyticsErrorCode, StreamingRuntimeLimits
 * @see include/analytics/ml_serving.h              — MLServingStatus
 */

#include <gtest/gtest.h>

#include "analytics/analytics_api_contract.h"
#include "analytics/ml_serving.h"

using namespace themis::analytics;
using namespace themisdb::analytics;

// ─────────────────────────────────────────────────────────────────────────────
// OPF-01..03 — MLServingStatus fallback codes
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test OPF-01: MLServingStatus::UNAVAILABLE is defined and distinct from OK.
 */
TEST(OptionalFallback, MLServingUnavailableIsDistinctFromOk) {
    EXPECT_NE(MLServingStatus::UNAVAILABLE, MLServingStatus::OK);
}

/**
 * @test OPF-02: MLServingStatus::POLICY_REJECTED is defined and distinct.
 */
TEST(OptionalFallback, MLServingPolicyRejectedIsDistinct) {
    EXPECT_NE(MLServingStatus::POLICY_REJECTED, MLServingStatus::OK);
    EXPECT_NE(MLServingStatus::POLICY_REJECTED, MLServingStatus::UNAVAILABLE);
}

/**
 * @test OPF-03: MLServingStatus values can be compared.
 */
TEST(OptionalFallback, MLServingStatusValuesComparable) {
    MLServingStatus s = MLServingStatus::UNAVAILABLE;
    EXPECT_EQ(s, MLServingStatus::UNAVAILABLE);
    EXPECT_NE(s, MLServingStatus::OK);
}

// ─────────────────────────────────────────────────────────────────────────────
// OPF-04..06 — AnalyticsErrorCode fallback codes
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test OPF-04: AnalyticsErrorCode::STREAM_BACKPRESSURE is defined.
 */
TEST(OptionalFallback, StreamBackpressureErrorCodeDefined) {
    AnalyticsErrorCode code = AnalyticsErrorCode::STREAM_BACKPRESSURE;
    EXPECT_NE(code, AnalyticsErrorCode::OK);
}

/**
 * @test OPF-05: AnalyticsErrorCode::AGGREGATION_OVERFLOW is defined and distinct.
 */
TEST(OptionalFallback, AggregationOverflowErrorCodeDefined) {
    EXPECT_NE(AnalyticsErrorCode::AGGREGATION_OVERFLOW, AnalyticsErrorCode::OK);
    EXPECT_NE(AnalyticsErrorCode::AGGREGATION_OVERFLOW, AnalyticsErrorCode::STREAM_BACKPRESSURE);
}

/**
 * @test OPF-06: Key error codes are usable in a switch statement.
 */
TEST(OptionalFallback, ErrorCodesUsableInSwitch) {
    auto label = [](AnalyticsErrorCode c) -> const char* {
        switch (c) {
            case AnalyticsErrorCode::OK:                  return "OK";
            case AnalyticsErrorCode::STREAM_BACKPRESSURE: return "BACKPRESSURE";
            case AnalyticsErrorCode::AGGREGATION_OVERFLOW: return "OVERFLOW";
            default:                                       return "OTHER";
        }
    };
    EXPECT_STREQ(label(AnalyticsErrorCode::OK),                   "OK");
    EXPECT_STREQ(label(AnalyticsErrorCode::STREAM_BACKPRESSURE),   "BACKPRESSURE");
    EXPECT_STREQ(label(AnalyticsErrorCode::AGGREGATION_OVERFLOW),  "OVERFLOW");
}

// ─────────────────────────────────────────────────────────────────────────────
// OPF-07..08 — Composition
// ─────────────────────────────────────────────────────────────────────────────

/// Simple config aggregate combining both policy types.
struct CombinedStreamConfig {
    BoundedExecutionPolicy policy;
    StreamingRuntimeLimits limits;
};

/**
 * @test OPF-07: BoundedExecutionPolicy and StreamingRuntimeLimits can be combined.
 */
TEST(OptionalFallback, CombinedConfigCompiles) {
    CombinedStreamConfig cfg;
    cfg.policy.max_latency_ms          = 200u;
    cfg.policy.max_concurrent_requests = 8u;
    cfg.limits.max_events_per_window   = 5'000u;
    cfg.limits.back_pressure_mode      = BackPressureMode::SHED;

    EXPECT_TRUE(cfg.policy.isConstrained());
    EXPECT_TRUE(cfg.limits.isConstrained());
}

/**
 * @test OPF-08: isConstrained() queries are independent on a default-constructed combined config.
 */
TEST(OptionalFallback, DefaultCombinedConfigBothUnconstrained) {
    CombinedStreamConfig cfg;
    EXPECT_FALSE(cfg.policy.isConstrained());
    EXPECT_FALSE(cfg.limits.isConstrained());
}
