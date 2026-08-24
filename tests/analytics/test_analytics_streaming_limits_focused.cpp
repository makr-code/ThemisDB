// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_analytics_streaming_limits_focused.cpp
 * @brief Streaming + distributed runtime limits under load — Wave B gap closure.
 *
 * Tests the `StreamingRuntimeLimits` struct and `BackPressureMode` semantics
 * defined in `analytics_api_contract.h`.
 *
 * ## Test families (SRL-01..SRL-10)
 *
 * ### SRL-01..03 — Struct semantics
 *   SRL-01  Default StreamingRuntimeLimits is unconstrained (isConstrained() == false)
 *   SRL-02  Setting max_events_per_window makes it constrained
 *   SRL-03  Setting max_window_memory_bytes makes it constrained
 *
 * ### SRL-04..06 — BackPressureMode enum
 *   SRL-04  BackPressureMode::DROP has distinct value from BLOCK and SHED
 *   SRL-05  Default back_pressure_mode is DROP
 *   SRL-06  All three modes are usable in switch statements
 *
 * ### SRL-07..10 — Composition with BoundedExecutionPolicy
 *   SRL-07  StreamingRuntimeLimits with DROP, constrained
 *   SRL-08  StreamingRuntimeLimits with BLOCK mode, both limits set
 *   SRL-09  StreamingRuntimeLimits with SHED mode, memory limit only
 *   SRL-10  Unconstrained limits + unconstrained policy = both unconstrained
 *
 * @see include/analytics/analytics_api_contract.h — StreamingRuntimeLimits
 */

#include <gtest/gtest.h>

#include "analytics/analytics_api_contract.h"

using namespace themis::analytics;

// ─────────────────────────────────────────────────────────────────────────────
// SRL-01..03 — Struct semantics
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test SRL-01: Default-constructed StreamingRuntimeLimits is unconstrained.
 */
TEST(StreamingRuntimeLimits, DefaultIsUnconstrained) {
    StreamingRuntimeLimits limits;
    EXPECT_FALSE(limits.isConstrained());
    EXPECT_EQ(limits.max_events_per_window, 0u);
    EXPECT_EQ(limits.max_window_memory_bytes, 0u);
}

/**
 * @test SRL-02: Setting max_events_per_window makes the struct constrained.
 */
TEST(StreamingRuntimeLimits, EventsPerWindowMakesConstrained) {
    StreamingRuntimeLimits limits;
    limits.max_events_per_window = 10'000u;
    EXPECT_TRUE(limits.isConstrained());
}

/**
 * @test SRL-03: Setting max_window_memory_bytes makes the struct constrained.
 */
TEST(StreamingRuntimeLimits, MemoryLimitMakesConstrained) {
    StreamingRuntimeLimits limits;
    limits.max_window_memory_bytes = 64u * 1024u * 1024u;  // 64 MiB
    EXPECT_TRUE(limits.isConstrained());
}

// ─────────────────────────────────────────────────────────────────────────────
// SRL-04..06 — BackPressureMode enum
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test SRL-04: BackPressureMode values are distinct.
 */
TEST(StreamingRuntimeLimits, BackPressureModeValuesDistinct) {
    EXPECT_NE(BackPressureMode::DROP,  BackPressureMode::BLOCK);
    EXPECT_NE(BackPressureMode::DROP,  BackPressureMode::SHED);
    EXPECT_NE(BackPressureMode::BLOCK, BackPressureMode::SHED);
}

/**
 * @test SRL-05: Default back_pressure_mode is DROP.
 */
TEST(StreamingRuntimeLimits, DefaultModeIsDrop) {
    StreamingRuntimeLimits limits;
    EXPECT_EQ(limits.back_pressure_mode, BackPressureMode::DROP);
}

/**
 * @test SRL-06: All three BackPressureMode values can be used in a switch statement.
 */
TEST(StreamingRuntimeLimits, AllModesHandledInSwitch) {
    auto modeLabel = [](BackPressureMode m) -> const char* {
        switch (m) {
            case BackPressureMode::DROP:  return "DROP";
            case BackPressureMode::BLOCK: return "BLOCK";
            case BackPressureMode::SHED:  return "SHED";
        }
        return "UNKNOWN";
    };
    EXPECT_STREQ(modeLabel(BackPressureMode::DROP),  "DROP");
    EXPECT_STREQ(modeLabel(BackPressureMode::BLOCK), "BLOCK");
    EXPECT_STREQ(modeLabel(BackPressureMode::SHED),  "SHED");
}

// ─────────────────────────────────────────────────────────────────────────────
// SRL-07..10 — Composition with BoundedExecutionPolicy
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test SRL-07: StreamingRuntimeLimits with DROP and events limit is constrained.
 */
TEST(StreamingRuntimeLimits, DropModeWithEventsLimitIsConstrained) {
    StreamingRuntimeLimits limits;
    limits.max_events_per_window = 50'000u;
    limits.back_pressure_mode    = BackPressureMode::DROP;
    EXPECT_TRUE(limits.isConstrained());
}

/**
 * @test SRL-08: StreamingRuntimeLimits with BLOCK mode and both limits set is constrained.
 */
TEST(StreamingRuntimeLimits, BlockModeWithBothLimitsIsConstrained) {
    StreamingRuntimeLimits limits;
    limits.max_events_per_window   = 100'000u;
    limits.max_window_memory_bytes = 128u * 1024u * 1024u;
    limits.back_pressure_mode      = BackPressureMode::BLOCK;
    EXPECT_TRUE(limits.isConstrained());
    EXPECT_EQ(limits.back_pressure_mode, BackPressureMode::BLOCK);
}

/**
 * @test SRL-09: SHED mode with memory-only limit is constrained.
 */
TEST(StreamingRuntimeLimits, ShedModeWithMemoryLimitIsConstrained) {
    StreamingRuntimeLimits limits;
    limits.max_window_memory_bytes = 32u * 1024u * 1024u;
    limits.back_pressure_mode      = BackPressureMode::SHED;
    EXPECT_TRUE(limits.isConstrained());
}

/**
 * @test SRL-10: Unconstrained StreamingRuntimeLimits and unconstrained
 *               BoundedExecutionPolicy are both false.
 */
TEST(StreamingRuntimeLimits, UnconstrainedLimitsAndPolicyBothFalse) {
    StreamingRuntimeLimits limits;
    BoundedExecutionPolicy policy;
    EXPECT_FALSE(limits.isConstrained());
    EXPECT_FALSE(policy.isConstrained());
}
