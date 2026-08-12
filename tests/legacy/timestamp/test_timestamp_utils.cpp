#include <gtest/gtest.h>
#include "utils/timestamp_utils.h"
#include <chrono>
#include <string>

using namespace themis::utils;
using namespace std::chrono_literals;

// ============================================================================
// Round-trip: format → parse
// ============================================================================

TEST(TimestampUtils, FormatAndParseRoundTrip) {
    // Use a fixed epoch point for determinism
    auto tp = std::chrono::system_clock::from_time_t(1741556904); // 2025-03-09 21:28:24 UTC
    std::string s = TimestampUtils::format(tp, false);
    // Should produce an ISO 8601 string without ms
    EXPECT_FALSE(s.empty());
    auto tp2 = TimestampUtils::parse(s);
    // Allow ±1 second rounding
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(tp2 - tp).count();
    EXPECT_LE(std::abs(diff), 1);
}

TEST(TimestampUtils, FormatWithMilliseconds) {
    auto tp = std::chrono::system_clock::from_time_t(1741556904);
    std::string s = TimestampUtils::format(tp, true);
    // Should contain a decimal point for milliseconds
    EXPECT_NE(s.find('.'), std::string::npos);
}

// ============================================================================
// Parse valid strings
// ============================================================================

TEST(TimestampUtils, ParseZuluTimestamp) {
    std::string s = "2026-03-09T21:28:24Z";
    EXPECT_NO_THROW(TimestampUtils::parse(s));
}

TEST(TimestampUtils, ParseTimestampWithMillis) {
    std::string s = "2026-03-09T21:28:24.912Z";
    EXPECT_NO_THROW(TimestampUtils::parse(s));
}

TEST(TimestampUtils, ParseThrowsOnMalformed) {
    EXPECT_THROW(TimestampUtils::parse("not-a-timestamp"), std::invalid_argument);
    EXPECT_THROW(TimestampUtils::parse(""), std::invalid_argument);
}

// ============================================================================
// Unix millis helpers
// ============================================================================

TEST(TimestampUtils, UnixMsRoundTrip) {
    auto tp = std::chrono::system_clock::from_time_t(1741556904);
    int64_t ms = TimestampUtils::toUnixMs(tp);
    auto tp2   = TimestampUtils::fromUnixMs(ms);
    EXPECT_EQ(tp, tp2);
}

TEST(TimestampUtils, ToUnixMsIsPositive) {
    auto tp = std::chrono::system_clock::now();
    EXPECT_GT(TimestampUtils::toUnixMs(tp), 0);
}

// ============================================================================
// now() returns a non-empty string
// ============================================================================

TEST(TimestampUtils, NowReturnsNonEmptyString) {
    std::string s = TimestampUtils::now();
    EXPECT_FALSE(s.empty());
}

// ============================================================================
// formatDuration
// ============================================================================

TEST(TimestampUtils, FormatDurationSeconds) {
    auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(5s);
    std::string s = TimestampUtils::formatDuration(d);
    EXPECT_NE(s.find('s'), std::string::npos);
}

TEST(TimestampUtils, FormatDurationMinutes) {
    auto d = std::chrono::duration_cast<std::chrono::nanoseconds>(90s);
    std::string s = TimestampUtils::formatDuration(d);
    EXPECT_NE(s.find('m'), std::string::npos);
}
