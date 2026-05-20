/*
 * ThemisDB | File: test_retry_policy.cpp | Version: 1.0.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Unit tests for include/utils/retry_policy.h and storage/codec_tags.h bridge
//
// Covers:
//   - retry_with_backoff: success on first attempt
//   - retry_with_backoff: success after N failures
//   - retry_with_backoff: exhausted (all attempts fail)
//   - retry_with_backoff: max_attempts=1 (no retry)
//   - ExponentialBackoff: delay progression (no sleep, check counts)
//   - is_known_tag (codec_tags.h smoke)
//   - method_to_tag / tag_to_method (compression_strategy ↔ codec_tags bridge)

#include <gtest/gtest.h>
#include "utils/retry_policy.h"
#include "storage/codec_tags.h"
#include "storage/compression_strategy.h"

#include <atomic>
#include <optional>

using namespace themis::utils;

// ---------------------------------------------------------------------------
// retry_with_backoff tests (use zero-delay config to keep tests fast)
// ---------------------------------------------------------------------------

static RetryConfig no_sleep_config() {
    return RetryConfig{
        .max_attempts       = 5,
        .initial_backoff_ms = 0,
        .max_backoff_ms     = 0,
        .multiplier         = 2.0,
        .jitter_fraction    = 0.0,
    };
}

TEST(RetryPolicy, SuccessOnFirstAttempt) {
    int calls = 0;
    auto result = retry_with_backoff(
        [&]() -> std::optional<int> {
            ++calls;
            return 42;
        },
        no_sleep_config());

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
    EXPECT_EQ(calls, 1);
}

TEST(RetryPolicy, SuccessAfterTwoFailures) {
    int calls = 0;
    auto result = retry_with_backoff(
        [&]() -> std::optional<int> {
            ++calls;
            if (calls < 3) return std::nullopt; // first two fail
            return 99;
        },
        no_sleep_config());

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 99);
    EXPECT_EQ(calls, 3);
}

TEST(RetryPolicy, AllAttemptsExhausted) {
    int calls = 0;
    auto cfg = no_sleep_config();
    cfg.max_attempts = 3;
    auto result = retry_with_backoff(
        [&]() -> std::optional<int> {
            ++calls;
            return std::nullopt;
        },
        cfg);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(calls, 3);
}

TEST(RetryPolicy, MaxAttemptsOne_NoRetry) {
    int calls = 0;
    RetryConfig cfg;
    cfg.max_attempts = 1;
    cfg.initial_backoff_ms = 0;
    auto result = retry_with_backoff(
        [&]() -> std::optional<std::string> {
            ++calls;
            return std::nullopt;
        },
        cfg);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(calls, 1);
}

TEST(RetryPolicy, ExceptionPropagatesImmediately) {
    int calls = 0;
    EXPECT_THROW(
        retry_with_backoff(
            [&]() -> std::optional<int> {
                ++calls;
                throw std::runtime_error("hard error");
                return std::nullopt;
            },
            no_sleep_config()),
        std::runtime_error);
    EXPECT_EQ(calls, 1); // called exactly once before exception
}

TEST(RetryPolicy, ReturnsCorrectValueType) {
    struct MyResult { int x; };
    auto result = retry_with_backoff(
        []() -> std::optional<MyResult> {
            return MyResult{123};
        },
        no_sleep_config());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->x, 123);
}

// ---------------------------------------------------------------------------
// ExponentialBackoff tests (zero delay; check attempt counting)
// ---------------------------------------------------------------------------

TEST(ExponentialBackoff, AttemptsCountedCorrectly) {
    RetryConfig cfg;
    cfg.max_attempts = 4;
    cfg.initial_backoff_ms = 0;
    cfg.max_backoff_ms = 0;
    cfg.jitter_fraction = 0.0;

    ExponentialBackoff bo(cfg);
    EXPECT_EQ(bo.attempts(), 0u);

    bool ok1 = bo.wait(); // attempt 1 of 3 waits
    EXPECT_TRUE(ok1);
    EXPECT_EQ(bo.attempts(), 1u);

    bool ok2 = bo.wait(); // attempt 2
    EXPECT_TRUE(ok2);
    EXPECT_EQ(bo.attempts(), 2u);

    bool ok3 = bo.wait(); // attempt 3
    EXPECT_TRUE(ok3);
    EXPECT_EQ(bo.attempts(), 3u);

    bool ok4 = bo.wait(); // attempt 4 = max_attempts; no more
    EXPECT_FALSE(ok4);
}

TEST(ExponentialBackoff, InitialDelayExposed) {
    RetryConfig cfg;
    cfg.initial_backoff_ms = 500;
    cfg.jitter_fraction = 0.0;
    ExponentialBackoff bo(cfg);
    EXPECT_EQ(bo.current_delay_ms(), 500u);
}

// ---------------------------------------------------------------------------
// codec_tags.h smoke test
// ---------------------------------------------------------------------------

TEST(CodecTags, KnownTagValues) {
    EXPECT_EQ(themis::compression::kTagPassthrough, uint8_t(0x00));
    EXPECT_EQ(themis::compression::kTagLZ4,         uint8_t(0x01));
    EXPECT_EQ(themis::compression::kTagSnappy,       uint8_t(0x02));
    EXPECT_EQ(themis::compression::kTagZstd,         uint8_t(0x03));
}

TEST(CodecTags, IsKnownTag) {
    EXPECT_TRUE(themis::compression::is_known_tag(0x00));
    EXPECT_TRUE(themis::compression::is_known_tag(0x01));
    EXPECT_TRUE(themis::compression::is_known_tag(0x02));
    EXPECT_TRUE(themis::compression::is_known_tag(0x03));
    EXPECT_FALSE(themis::compression::is_known_tag(0x04));
    EXPECT_FALSE(themis::compression::is_known_tag(0xFF));
}

// ---------------------------------------------------------------------------
// codec_tags bridge: method_to_tag / tag_to_method
// ---------------------------------------------------------------------------

using themis::compression::CompressionMethod;

TEST(CodecTagsBridge, MethodToTagRoundTrip) {
    EXPECT_EQ(themis::compression::method_to_tag(CompressionMethod::NONE),
              themis::compression::kTagPassthrough);
    EXPECT_EQ(themis::compression::method_to_tag(CompressionMethod::LZ4),
              themis::compression::kTagLZ4);
    EXPECT_EQ(themis::compression::method_to_tag(CompressionMethod::SNAPPY),
              themis::compression::kTagSnappy);
    EXPECT_EQ(themis::compression::method_to_tag(CompressionMethod::ZSTD),
              themis::compression::kTagZstd);
}

TEST(CodecTagsBridge, GpuVariantsMappedToSameTag) {
    EXPECT_EQ(themis::compression::method_to_tag(CompressionMethod::GPU_LZ4),
              themis::compression::kTagLZ4);
    EXPECT_EQ(themis::compression::method_to_tag(CompressionMethod::GPU_SNAPPY),
              themis::compression::kTagSnappy);
    EXPECT_EQ(themis::compression::method_to_tag(CompressionMethod::GPU_ZSTD),
              themis::compression::kTagZstd);
}

TEST(CodecTagsBridge, InternalMethodsFallbackToPassthrough) {
    EXPECT_EQ(themis::compression::method_to_tag(CompressionMethod::RLE),
              themis::compression::kTagPassthrough);
    EXPECT_EQ(themis::compression::method_to_tag(CompressionMethod::DELTA),
              themis::compression::kTagPassthrough);
    EXPECT_EQ(themis::compression::method_to_tag(CompressionMethod::ADAPTIVE),
              themis::compression::kTagPassthrough);
}

TEST(CodecTagsBridge, TagToMethodKnownTags) {
    EXPECT_EQ(*themis::compression::tag_to_method(themis::compression::kTagPassthrough),
              CompressionMethod::NONE);
    EXPECT_EQ(*themis::compression::tag_to_method(themis::compression::kTagLZ4),
              CompressionMethod::LZ4);
    EXPECT_EQ(*themis::compression::tag_to_method(themis::compression::kTagSnappy),
              CompressionMethod::SNAPPY);
    EXPECT_EQ(*themis::compression::tag_to_method(themis::compression::kTagZstd),
              CompressionMethod::ZSTD);
}

TEST(CodecTagsBridge, TagToMethodUnknownTag) {
    EXPECT_FALSE(themis::compression::tag_to_method(0x04).has_value());
    EXPECT_FALSE(themis::compression::tag_to_method(0xFF).has_value());
}

TEST(CodecTagsBridge, MethodTagRoundTripSymmetry) {
    // For the three codecs with a canonical tag, round-tripping must give
    // back the original method.
    for (auto m : {CompressionMethod::LZ4, CompressionMethod::SNAPPY, CompressionMethod::ZSTD}) {
        const uint8_t tag = themis::compression::method_to_tag(m);
        const auto back = themis::compression::tag_to_method(tag);
        ASSERT_TRUE(back.has_value());
        EXPECT_EQ(*back, m);
    }
}
