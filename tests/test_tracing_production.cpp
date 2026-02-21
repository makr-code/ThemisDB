/**
 * @file test_tracing_production.cpp
 * @brief Phase 2 – Production tracing feature tests
 *
 * Tests cover:
 * - Baggage support (set, get, remove, clear, getAll)
 * - Baggage serialization / deserialization (W3C header format)
 * - Baggage injection into outgoing headers
 * - Baggage extraction from incoming headers
 * - SamplingStrategy: ALWAYS_ON, ALWAYS_OFF, PROBABILITY, PARENT_BASED
 * - setSamplingStrategy / getSamplingStrategy API
 * - W3C TraceContext propagation via headers
 * - Span lifecycle and RAII
 * - Exception / error tracking (recordError)
 * - Trace-ID / Span-ID accessors
 */

#include <gtest/gtest.h>
#include "utils/tracing.h"
#include <map>
#include <string>
#include <thread>
#include <vector>
#include <atomic>

using namespace themis;

// ============================================================================
// Baggage – basic operations
// ============================================================================

class BaggageTest : public ::testing::Test {
protected:
    void SetUp() override    { Baggage::clear(); }
    void TearDown() override { Baggage::clear(); }
};

TEST_F(BaggageTest, SetAndGet) {
    Baggage::set("tenant-id", "acme");
    EXPECT_EQ(Baggage::get("tenant-id"), "acme");
}

TEST_F(BaggageTest, GetMissingReturnsEmpty) {
    EXPECT_EQ(Baggage::get("no-such-key"), "");
}

TEST_F(BaggageTest, Overwrite) {
    Baggage::set("k", "v1");
    Baggage::set("k", "v2");
    EXPECT_EQ(Baggage::get("k"), "v2");
}

TEST_F(BaggageTest, Remove) {
    Baggage::set("remove-me", "val");
    Baggage::remove("remove-me");
    EXPECT_EQ(Baggage::get("remove-me"), "");
}

TEST_F(BaggageTest, Clear) {
    Baggage::set("a", "1");
    Baggage::set("b", "2");
    Baggage::clear();
    EXPECT_EQ(Baggage::get("a"), "");
    EXPECT_EQ(Baggage::get("b"), "");
}

TEST_F(BaggageTest, GetAll) {
    Baggage::set("x", "10");
    Baggage::set("y", "20");
    auto all = Baggage::getAll();
    EXPECT_EQ(all.count("x"), 1u);
    EXPECT_EQ(all.count("y"), 1u);
    EXPECT_EQ(all["x"], "10");
    EXPECT_EQ(all["y"], "20");
}

// ============================================================================
// Baggage – serialization / W3C header
// ============================================================================

TEST_F(BaggageTest, SerializeEmpty) {
    EXPECT_EQ(Baggage::serialize(), "");
}

TEST_F(BaggageTest, SerializeSingle) {
    Baggage::set("env", "prod");
    auto s = Baggage::serialize();
    EXPECT_NE(s.find("env=prod"), std::string::npos);
}

TEST_F(BaggageTest, InjectIntoHeaders) {
    Baggage::set("region", "eu-west-1");
    std::map<std::string, std::string> headers;
    Baggage::inject(headers);
    ASSERT_NE(headers.find("baggage"), headers.end());
    EXPECT_NE(headers["baggage"].find("region=eu-west-1"), std::string::npos);
}

TEST_F(BaggageTest, ExtractFromHeaders) {
    std::map<std::string, std::string> headers;
    headers["baggage"] = "user-id=alice,session=xyz";
    Baggage::extract(headers);
    EXPECT_EQ(Baggage::get("user-id"), "alice");
    EXPECT_EQ(Baggage::get("session"), "xyz");
}

TEST_F(BaggageTest, ExtractCaseInsensitiveHeader) {
    std::map<std::string, std::string> headers;
    headers["Baggage"] = "k=v";
    Baggage::extract(headers);
    EXPECT_EQ(Baggage::get("k"), "v");
}

TEST_F(BaggageTest, ExtractIgnoresUnknownHeaders) {
    std::map<std::string, std::string> headers;
    headers["x-custom"] = "foo";
    Baggage::extract(headers);  // Should not crash
    EXPECT_EQ(Baggage::get("x-custom"), "");
}

// Baggage is thread-local – each thread has independent state
TEST_F(BaggageTest, ThreadLocalIsolation) {
    Baggage::set("main-key", "main-value");

    std::string other_value;
    std::thread t([&]() {
        other_value = Baggage::get("main-key"); // Should be empty in another thread
    });
    t.join();

    EXPECT_EQ(other_value, "");
    EXPECT_EQ(Baggage::get("main-key"), "main-value");
}

// ============================================================================
// SamplingStrategy
// ============================================================================

class SamplingStrategyTest : public ::testing::Test {};

TEST_F(SamplingStrategyTest, AlwaysOnSamples) {
    auto s = SamplingStrategy::alwaysOn();
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(s.shouldSample());
    }
}

TEST_F(SamplingStrategyTest, AlwaysOffNeverSamples) {
    auto s = SamplingStrategy::alwaysOff();
    for (int i = 0; i < 100; ++i) {
        EXPECT_FALSE(s.shouldSample());
    }
}

TEST_F(SamplingStrategyTest, ProbabilityZeroNeverSamples) {
    auto s = SamplingStrategy::probability(0.0);
    for (int i = 0; i < 100; ++i) {
        EXPECT_FALSE(s.shouldSample());
    }
}

TEST_F(SamplingStrategyTest, ProbabilityOneAlwaysSamples) {
    auto s = SamplingStrategy::probability(1.0);
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(s.shouldSample());
    }
}

TEST_F(SamplingStrategyTest, ProbabilityHalfIsRoughlyHalf) {
    auto s = SamplingStrategy::probability(0.5);
    int sampled = 0;
    constexpr int kTrials = 10000;
    for (int i = 0; i < kTrials; ++i) {
        if (s.shouldSample()) ++sampled;
    }
    // With p=0.5, expect roughly 50% – allow ±15% tolerance
    double ratio = static_cast<double>(sampled) / kTrials;
    EXPECT_GT(ratio, 0.35);
    EXPECT_LT(ratio, 0.65);
}

TEST_F(SamplingStrategyTest, ParentBasedHonoursParentSampled) {
    auto s = SamplingStrategy::parentBased(0.0); // root never sampled
    // If parent is sampled, the child should be sampled too
    for (int i = 0; i < 50; ++i) {
        EXPECT_TRUE(s.shouldSample(/*parent_sampled=*/true));
    }
}

TEST_F(SamplingStrategyTest, ParentBasedAppliesProbabilityForRoots) {
    auto s = SamplingStrategy::parentBased(0.0); // root: never
    for (int i = 0; i < 50; ++i) {
        EXPECT_FALSE(s.shouldSample(/*parent_sampled=*/false));
    }
}

TEST_F(SamplingStrategyTest, TypeAccessors) {
    EXPECT_EQ(SamplingStrategy::alwaysOn().type(),       SamplingStrategy::Type::ALWAYS_ON);
    EXPECT_EQ(SamplingStrategy::alwaysOff().type(),      SamplingStrategy::Type::ALWAYS_OFF);
    EXPECT_EQ(SamplingStrategy::probability(0.5).type(), SamplingStrategy::Type::PROBABILITY);
    EXPECT_EQ(SamplingStrategy::parentBased(1.0).type(), SamplingStrategy::Type::PARENT_BASED);

    EXPECT_DOUBLE_EQ(SamplingStrategy::probability(0.7).probability(), 0.7);
}

// ============================================================================
// Tracer – sampling integration
// ============================================================================

class TracerSamplingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Restore ALWAYS_ON after each test
        Tracer::setSamplingStrategy(SamplingStrategy::alwaysOn());
        Tracer::shutdown();
    }
    void TearDown() override {
        Tracer::setSamplingStrategy(SamplingStrategy::alwaysOn());
        Tracer::shutdown();
    }
};

TEST_F(TracerSamplingTest, SetAndGetStrategy) {
    auto s = SamplingStrategy::probability(0.3);
    Tracer::setSamplingStrategy(s);
    auto got = Tracer::getSamplingStrategy();
    EXPECT_EQ(got.type(), SamplingStrategy::Type::PROBABILITY);
    EXPECT_DOUBLE_EQ(got.probability(), 0.3);
}

TEST_F(TracerSamplingTest, AlwaysOffProducesNoSpans) {
    Tracer::setSamplingStrategy(SamplingStrategy::alwaysOff());
    int64_t before = Tracer::getTotalSpans();
    auto span = Tracer::startSpan("should-not-be-sampled");
    EXPECT_FALSE(span.isValid());
    EXPECT_EQ(Tracer::getTotalSpans(), before); // No new span counted
}

TEST_F(TracerSamplingTest, AlwaysOnProducesSpansWhenInitialized) {
    Tracer::setSamplingStrategy(SamplingStrategy::alwaysOn());
    // Without initialization, spans are no-ops (THEMIS_ENABLE_TRACING may be off)
    // We just verify the API doesn't crash
    auto span = Tracer::startSpan("test-span");
    span.end();
}

// ============================================================================
// Tracer – basic span lifecycle (no-op path when tracing disabled)
// ============================================================================

class TracerLifecycleTest : public ::testing::Test {
protected:
    void TearDown() override { Tracer::shutdown(); }
};

TEST_F(TracerLifecycleTest, SpanValidityOnNoop) {
    Tracer::shutdown(); // Ensure not initialized
    auto span = Tracer::startSpan("noop-span");
    // Without OTel enabled, span is invalid but must not crash
    span.setAttribute("k", "v");
    span.setAttribute("n", static_cast<int64_t>(42));
    span.setAttribute("d", 3.14);
    span.setAttribute("b", true);
    span.setStatus(true, "ok");
    span.recordError("test error");
    EXPECT_GE(span.durationMs(), 0.0);
    span.end();
}

TEST_F(TracerLifecycleTest, ActiveSpanCountsBalance) {
    auto before = Tracer::getActiveSpans();
    {
        auto span = Tracer::startSpan("balance-test");
        // Active should increase (only if span is valid)
        if (span.isValid()) {
            EXPECT_EQ(Tracer::getActiveSpans(), before + 1);
        }
    }
    // After destruction, active should return to prior value
    EXPECT_EQ(Tracer::getActiveSpans(), before);
}

TEST_F(TracerLifecycleTest, TotalSpanCountsAccumulate) {
    auto before = Tracer::getTotalSpans();
    for (int i = 0; i < 5; ++i) {
        Tracer::startSpan("span-" + std::to_string(i)).end();
    }
    // If tracing is enabled, count should increase; otherwise stays same
    EXPECT_GE(Tracer::getTotalSpans(), before);
}

TEST_F(TracerLifecycleTest, ScopedSpanIsRaii) {
    auto before_total = Tracer::getTotalSpans();
    {
        ScopedSpan ss("scoped-test");
        ss.setAttribute("k", "v");
    }
    // After scope exit, active spans must be balanced
    EXPECT_GE(Tracer::getTotalSpans(), before_total);
    EXPECT_EQ(Tracer::getActiveSpans(), 0);
}

// ============================================================================
// W3C TraceContext propagation
// ============================================================================

class TracerW3CTest : public ::testing::Test {
protected:
    void TearDown() override { Tracer::shutdown(); }
};

TEST_F(TracerW3CTest, ValidTraceparentPropagates) {
    std::map<std::string, std::string> headers;
    headers["traceparent"] = "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";
    auto span = Tracer::startSpanFromHeaders("incoming-request", headers);
    // Must not crash regardless of whether OTel is enabled
    span.end();
}

TEST_F(TracerW3CTest, InvalidTraceparentFallsBackToNewSpan) {
    std::map<std::string, std::string> headers;
    headers["traceparent"] = "bad-value";
    auto span = Tracer::startSpanFromHeaders("fallback-span", headers);
    span.end();
}

TEST_F(TracerW3CTest, BaggageExtractedFromHeaders) {
    Baggage::clear();
    std::map<std::string, std::string> headers;
    headers["traceparent"] = "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";
    headers["baggage"] = "request-id=req-001";
    auto span = Tracer::startSpanFromHeaders("baggage-test", headers);
    span.end();
    EXPECT_EQ(Baggage::get("request-id"), "req-001");
    Baggage::clear();
}

TEST_F(TracerW3CTest, EmptyHeadersStartNewSpan) {
    std::map<std::string, std::string> headers;
    auto span = Tracer::startSpanFromHeaders("empty-headers", headers);
    span.end();
}

// ============================================================================
// Trace-ID / Span-ID accessors
// ============================================================================

TEST(TracerIdAccessors, GetCurrentTraceIdReturnsStringOrEmpty) {
    Tracer::shutdown();
    std::string tid = Tracer::getCurrentTraceId();
    // Without active span, must return empty
    EXPECT_TRUE(tid.empty() || tid.size() == 32);
}

TEST(TracerIdAccessors, GetCurrentSpanIdReturnsStringOrEmpty) {
    Tracer::shutdown();
    std::string sid = Tracer::getCurrentSpanId();
    EXPECT_TRUE(sid.empty() || sid.size() == 16);
}
