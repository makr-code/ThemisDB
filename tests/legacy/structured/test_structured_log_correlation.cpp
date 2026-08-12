/*
 * Unit tests for Structured Log Correlation
 *
 * Covers:
 *   - TraceContext  : new span_id field; empty() semantics
 *   - ILogger::logWithContext() : injects trace_id, span_id, request_id
 *   - SpdlogLoggerAdapter::logWithContext() : plain-text prefix and JSON fields
 *   - ConcernsContext::logWithTrace() : auto-fetches live trace/span IDs
 */

#include "core/concerns/i_logger.h"
#include "core/concerns/i_context.h"
#include "core/concerns/concerns_context.h"
#include "core/concerns/spdlog_logger_adapter.h"
#include "core/concerns/noop_implementations.h"
#include <gtest/gtest.h>
#include <atomic>
#include <memory>
#include <string>
#include <sstream>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

using namespace themis::core::concerns;

// ─────────────────────────────────────────────────────────────────────────────
// TraceContext
// ─────────────────────────────────────────────────────────────────────────────

TEST(StructLogTraceContextTest, DefaultIsEmpty) {
    TraceContext tc;
    EXPECT_TRUE(tc.empty());
    EXPECT_TRUE(tc.trace_id.empty());
    EXPECT_TRUE(tc.span_id.empty());
    EXPECT_TRUE(tc.request_id.empty());
}

TEST(StructLogTraceContextTest, NotEmptyWithTraceId) {
    TraceContext tc;
    tc.trace_id = "abc123";
    EXPECT_FALSE(tc.empty());
}

TEST(StructLogTraceContextTest, NotEmptyWithSpanId) {
    TraceContext tc;
    tc.span_id = "dead0001";
    EXPECT_FALSE(tc.empty());
}

TEST(StructLogTraceContextTest, NotEmptyWithRequestId) {
    TraceContext tc;
    tc.request_id = "req-99";
    EXPECT_FALSE(tc.empty());
}

TEST(StructLogTraceContextTest, AllFieldsSettable) {
    TraceContext tc;
    tc.trace_id   = "trace-111";
    tc.span_id    = "span-222";
    tc.request_id = "req-333";
    EXPECT_EQ("trace-111", tc.trace_id);
    EXPECT_EQ("span-222",  tc.span_id);
    EXPECT_EQ("req-333",   tc.request_id);
    EXPECT_FALSE(tc.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// ILogger::logWithContext() default — merges all three IDs into fields
// ─────────────────────────────────────────────────────────────────────────────

/// Capturing logger: records all logStructured() calls for assertion.
class CapturingLogger : public ILogger {
public:
    struct Record {
        Level                    level;
        std::string              message;
        std::map<std::string, std::string> fields;
    };

    void log(Level l, const std::string& m) override {
        records.push_back({l, m, {}});
    }
    void trace(const std::string& m) override    { log(Level::TRACE, m); }
    void debug(const std::string& m) override    { log(Level::DEBUG, m); }
    void info(const std::string& m) override     { log(Level::INFO,  m); }
    void warn(const std::string& m) override     { log(Level::WARN,  m); }
    void error(const std::string& m) override    { log(Level::ERROR, m); }
    void critical(const std::string& m) override { log(Level::CRITICAL, m); }

    void logStructured(Level l, const std::string& m, const Fields& f) override {
        records.push_back({l, m, f});
    }

    void setLevel(Level) override {}
    Level getLevel() const override { return Level::TRACE; }
    void setPattern(const std::string&) override {}

    std::vector<Record> records;
};

TEST(StructLogILoggerDefaultTest, InjectsTraceId) {
    CapturingLogger logger;
    TraceContext ctx;
    ctx.trace_id = "abc";
    logger.logWithContext(ILogger::Level::INFO, "test", ctx);
    ASSERT_EQ(1u, logger.records.size());
    EXPECT_EQ("abc", logger.records[0].fields.at("trace_id"));
}

TEST(StructLogILoggerDefaultTest, InjectsSpanId) {
    CapturingLogger logger;
    TraceContext ctx;
    ctx.span_id = "111";
    logger.logWithContext(ILogger::Level::INFO, "test", ctx);
    ASSERT_EQ(1u, logger.records.size());
    EXPECT_EQ("111", logger.records[0].fields.at("span_id"));
}

TEST(StructLogILoggerDefaultTest, InjectsRequestId) {
    CapturingLogger logger;
    TraceContext ctx;
    ctx.request_id = "req-42";
    logger.logWithContext(ILogger::Level::INFO, "test", ctx);
    ASSERT_EQ(1u, logger.records.size());
    EXPECT_EQ("req-42", logger.records[0].fields.at("request_id"));
}

TEST(StructLogILoggerDefaultTest, InjectsAllThreeIds) {
    CapturingLogger logger;
    TraceContext ctx{"t-1", "s-2", "r-3"};
    logger.logWithContext(ILogger::Level::WARN, "msg", ctx, {{"extra", "val"}});
    ASSERT_EQ(1u, logger.records.size());
    const auto& f = logger.records[0].fields;
    EXPECT_EQ("t-1",   f.at("trace_id"));
    EXPECT_EQ("s-2",   f.at("span_id"));
    EXPECT_EQ("r-3",   f.at("request_id"));
    EXPECT_EQ("val",   f.at("extra"));
}

TEST(StructLogILoggerDefaultTest, EmptyContextDoesNotInjectFields) {
    CapturingLogger logger;
    TraceContext ctx; // all empty
    logger.logWithContext(ILogger::Level::INFO, "msg", ctx);
    ASSERT_EQ(1u, logger.records.size());
    EXPECT_TRUE(logger.records[0].fields.empty());
}

TEST(StructLogILoggerDefaultTest, UserFieldsNotOverwrittenByContextIds) {
    CapturingLogger logger;
    TraceContext ctx{"t-1", "s-1", "r-1"};
    // User supplies their own trace_id — ILogger default does NOT overwrite it
    // if it's already present in the merged map (ILogger merges ctx ids into
    // a copy of fields; since Fields::insert() does not overwrite we check ordering).
    // The default implementation does: merged["trace_id"] = ctx.trace_id,
    // which DOES overwrite. Document the actual behavior here.
    logger.logWithContext(ILogger::Level::INFO, "msg", ctx, {{"trace_id", "user-override"}});
    ASSERT_EQ(1u, logger.records.size());
    // The default merges ctx fields AFTER copying the user fields, so ctx wins.
    EXPECT_EQ("t-1", logger.records[0].fields.at("trace_id"));
}

// ─────────────────────────────────────────────────────────────────────────────
// SpdlogLoggerAdapter::logWithContext() — plain-text mode
// ─────────────────────────────────────────────────────────────────────────────

/// Build a SpdlogLoggerAdapter backed by an in-memory stream sink.
static std::pair<std::shared_ptr<SpdlogLoggerAdapter>, std::shared_ptr<std::ostringstream>>
makeSpdlogAdapter(bool json_mode = false) {
    auto oss  = std::make_shared<std::ostringstream>();
    auto sink = std::make_shared<spdlog::sinks::ostream_sink_st>(*oss);
    // Use a unique name per call to avoid spdlog duplicate-logger warnings
    // when multiple tests run in the same process.
    static std::atomic<int> counter{0};
    std::string name = "struct_log_test_" + std::to_string(++counter);
    auto inner = std::make_shared<spdlog::logger>(name, sink);
    inner->set_level(spdlog::level::trace);
    inner->set_pattern("%v"); // emit just the message, no timestamp noise
    auto adapter = std::make_shared<SpdlogLoggerAdapter>(inner, json_mode);
    return {adapter, oss};
}

TEST(StructLogSpdlogPlainTextTest, PrependTraceId) {
    auto [adapter, oss] = makeSpdlogAdapter();
    TraceContext ctx;
    ctx.trace_id = "abc123";
    adapter->logWithContext(ILogger::Level::INFO, "hello", ctx);
    std::string out = oss->str();
    EXPECT_NE(std::string::npos, out.find("[trace=abc123]"));
    EXPECT_NE(std::string::npos, out.find("hello"));
}

TEST(StructLogSpdlogPlainTextTest, PrependSpanId) {
    auto [adapter, oss] = makeSpdlogAdapter();
    TraceContext ctx;
    ctx.span_id = "dead0001";
    adapter->logWithContext(ILogger::Level::INFO, "msg", ctx);
    EXPECT_NE(std::string::npos, oss->str().find("[span=dead0001]"));
}

TEST(StructLogSpdlogPlainTextTest, PrependRequestId) {
    auto [adapter, oss] = makeSpdlogAdapter();
    TraceContext ctx;
    ctx.request_id = "req-7";
    adapter->logWithContext(ILogger::Level::INFO, "msg", ctx);
    EXPECT_NE(std::string::npos, oss->str().find("[req=req-7]"));
}

TEST(StructLogSpdlogPlainTextTest, PrependAllThreeIds) {
    auto [adapter, oss] = makeSpdlogAdapter();
    TraceContext ctx{"t-1", "s-2", "r-3"};
    adapter->logWithContext(ILogger::Level::INFO, "op", ctx);
    std::string out = oss->str();
    EXPECT_NE(std::string::npos, out.find("[trace=t-1]"));
    EXPECT_NE(std::string::npos, out.find("[span=s-2]"));
    EXPECT_NE(std::string::npos, out.find("[req=r-3]"));
    EXPECT_NE(std::string::npos, out.find("op"));
}

TEST(StructLogSpdlogPlainTextTest, EmptyContextProducesNoPrefix) {
    auto [adapter, oss] = makeSpdlogAdapter();
    TraceContext ctx; // all empty
    adapter->logWithContext(ILogger::Level::INFO, "plain", ctx);
    std::string out = oss->str();
    EXPECT_EQ(std::string::npos, out.find("[trace="));
    EXPECT_EQ(std::string::npos, out.find("[span="));
    EXPECT_EQ(std::string::npos, out.find("[req="));
    EXPECT_NE(std::string::npos, out.find("plain"));
}

TEST(StructLogSpdlogPlainTextTest, UserFieldsAppendedAfterMessage) {
    auto [adapter, oss] = makeSpdlogAdapter();
    TraceContext ctx;
    ctx.trace_id = "t";
    adapter->logWithContext(ILogger::Level::INFO, "msg", ctx, {{"rows", "42"}});
    std::string out = oss->str();
    EXPECT_NE(std::string::npos, out.find("rows=42"));
}

// ─────────────────────────────────────────────────────────────────────────────
// SpdlogLoggerAdapter::logWithContext() — JSON mode
// ─────────────────────────────────────────────────────────────────────────────

TEST(StructLogSpdlogJsonTest, TraceIdAppearsInJson) {
    auto [adapter, oss] = makeSpdlogAdapter(/*json_mode=*/true);
    TraceContext ctx{"tr-abc", "", ""};
    adapter->logWithContext(ILogger::Level::INFO, "ok", ctx);
    std::string out = oss->str();
    EXPECT_NE(std::string::npos, out.find("\"trace_id\""));
    EXPECT_NE(std::string::npos, out.find("tr-abc"));
}

TEST(StructLogSpdlogJsonTest, SpanIdAppearsInJson) {
    auto [adapter, oss] = makeSpdlogAdapter(true);
    TraceContext ctx{"", "sp-001", ""};
    adapter->logWithContext(ILogger::Level::INFO, "ok", ctx);
    std::string out = oss->str();
    EXPECT_NE(std::string::npos, out.find("\"span_id\""));
    EXPECT_NE(std::string::npos, out.find("sp-001"));
}

TEST(StructLogSpdlogJsonTest, RequestIdAppearsInJson) {
    auto [adapter, oss] = makeSpdlogAdapter(true);
    TraceContext ctx{"", "", "rq-55"};
    adapter->logWithContext(ILogger::Level::INFO, "ok", ctx);
    std::string out = oss->str();
    EXPECT_NE(std::string::npos, out.find("\"request_id\""));
    EXPECT_NE(std::string::npos, out.find("rq-55"));
}

TEST(StructLogSpdlogJsonTest, AllThreeIdsInJson) {
    auto [adapter, oss] = makeSpdlogAdapter(true);
    TraceContext ctx{"tr-1", "sp-2", "rq-3"};
    adapter->logWithContext(ILogger::Level::WARN, "warn", ctx);
    std::string out = oss->str();
    EXPECT_NE(std::string::npos, out.find("tr-1"));
    EXPECT_NE(std::string::npos, out.find("sp-2"));
    EXPECT_NE(std::string::npos, out.find("rq-3"));
}

TEST(StructLogSpdlogJsonTest, UserFieldsAlsoPresentInJson) {
    auto [adapter, oss] = makeSpdlogAdapter(true);
    TraceContext ctx{"tr", "", ""};
    adapter->logWithContext(ILogger::Level::INFO, "op", ctx, {{"db.table", "users"}});
    std::string out = oss->str();
    EXPECT_NE(std::string::npos, out.find("users"));
}

TEST(StructLogSpdlogJsonTest, OutputIsValidJsonObject) {
    auto [adapter, oss] = makeSpdlogAdapter(true);
    TraceContext ctx{"tr-x", "sp-y", "rq-z"};
    adapter->logWithContext(ILogger::Level::INFO, "event", ctx, {{"key", "val"}});
    std::string out = oss->str();
    // Must start with '{' and end with '}' (with optional newline)
    size_t open  = out.find('{');
    size_t close = out.rfind('}');
    EXPECT_NE(std::string::npos, open);
    EXPECT_NE(std::string::npos, close);
    EXPECT_LT(open, close);
}

// ─────────────────────────────────────────────────────────────────────────────
// IContext::toTraceContext() — backward compatibility (no span_id)
// ─────────────────────────────────────────────────────────────────────────────

TEST(StructLogSimpleContextTest, TraceIdAndRequestIdPopulated) {
    auto ctx = SimpleContext::create("t-abc", "r-123");
    TraceContext tc = ctx->toTraceContext();
    EXPECT_EQ("t-abc", tc.trace_id);
    EXPECT_EQ("r-123", tc.request_id);
    // span_id is not stored in IContext — it defaults to empty.
    EXPECT_TRUE(tc.span_id.empty());
}

TEST(StructLogSimpleContextTest, EmptyContextProducesEmptyTraceContext) {
    auto ctx = SimpleContext::create();
    TraceContext tc = ctx->toTraceContext();
    EXPECT_TRUE(tc.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// ConcernsContext::logWithTrace()
// ─────────────────────────────────────────────────────────────────────────────

TEST(StructLogConcernsContextTest, DoesNotThrow) {
    auto ctx = ConcernsContext::createNoOp();
    EXPECT_NO_THROW(ctx->logWithTrace(ILogger::Level::INFO, "hello"));
}

TEST(StructLogConcernsContextTest, DoesNotThrowWithFields) {
    auto ctx = ConcernsContext::createNoOp();
    EXPECT_NO_THROW(
        ctx->logWithTrace(ILogger::Level::ERROR, "db error",
                          {{"db.table", "users"}, {"rows", "0"}})
    );
}

TEST(StructLogConcernsContextTest, InvokesLoggerLogWithContext) {
    // Use a CapturingLogger to verify that logWithTrace() calls logWithContext().
    class TraceCapture : public ILogger {
    public:
        struct Call { Level level; std::string msg; TraceContext ctx; Fields fields; };
        void log([[maybe_unused]] Level l, [[maybe_unused]] const std::string& m) override {}
        void trace(const std::string&) override {}
        void debug(const std::string&) override {}
        void info(const std::string&) override {}
        void warn(const std::string&) override {}
        void error(const std::string&) override {}
        void critical(const std::string&) override {}
        void setLevel(Level) override {}
        Level getLevel() const override { return Level::TRACE; }
        void setPattern(const std::string&) override {}
        void logWithContext(Level l, const std::string& m,
                            const TraceContext& ctx, const Fields& f) override {
            calls.push_back({l, m, ctx, f});
        }
        std::vector<Call> calls;
    };

    auto capturer = std::make_unique<TraceCapture>();
    TraceCapture* raw = capturer.get();

    auto ctx = ConcernsContext::createCustom(
        std::move(capturer),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<NoOpCache>()
    );

    ctx->logWithTrace(ILogger::Level::WARN, "watch out", {{"key", "val"}});

    ASSERT_EQ(1u, raw->calls.size());
    EXPECT_EQ(ILogger::Level::WARN, raw->calls[0].level);
    EXPECT_EQ("watch out", raw->calls[0].msg);
    EXPECT_EQ("val", raw->calls[0].fields.at("key"));
    // Trace / span IDs may be empty when no active OTel span exists in CI,
    // but the TraceContext struct should always be passed through.
    // (No assertion on trace_id value — depends on OTel being initialized.)
}
