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
