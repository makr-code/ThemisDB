/*
 * ThemisDB | File: test_core_smoke.cpp | Version: 0.0.1
 * Module: core
 * Purpose: Smoke tests verifying that core concern headers compile and that
 *          the no-op implementations can be constructed and used in isolation.
 *          These tests are the minimum viable CTest entry-point for the core
 *          module and intentionally avoid heavy runtime dependencies.
 */

#include <gtest/gtest.h>
#include "core/concerns/noop_implementations.h"
#include "core/concerns/i_logger.h"

using namespace themis::core::concerns;

// ---------------------------------------------------------------------------
// 1. Header compile + NoOpLogger construction
// ---------------------------------------------------------------------------
TEST(CoreSmokeTest, NoOpLoggerConstructsWithoutThrow) {
    EXPECT_NO_THROW({
        NoOpLogger logger;
        logger.log(ILogger::Level::INFO, "smoke test message");
    });
}

// ---------------------------------------------------------------------------
// 2. TraceContext default construction
// ---------------------------------------------------------------------------
TEST(CoreSmokeTest, TraceContextDefaultIsEmpty) {
    TraceContext ctx;
    EXPECT_TRUE(ctx.empty());
    EXPECT_TRUE(ctx.trace_id.empty());
    EXPECT_TRUE(ctx.span_id.empty());
    EXPECT_TRUE(ctx.request_id.empty());
}
