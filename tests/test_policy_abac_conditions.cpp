/**
 * @file test_policy_abac_conditions.cpp
 * @brief Tests for PolicyEngine ABAC time-window and User-Agent conditions (issue §1.1)
 */

#include <gtest/gtest.h>
#include "server/policy_engine.h"
#include <chrono>
#include <ctime>
#include <string>

using namespace themis;

// ──────────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────────

static PolicyEngine::Policy makeBasicPolicy(const std::string& id,
                                             const std::string& user,
                                             bool allow = true) {
    PolicyEngine::Policy p;
    p.id           = id;
    p.name         = id;
    p.subjects     = {user};
    p.actions      = {"read"};
    p.resources    = {"/data"};
    p.effect_allow = allow;
    return p;
}

/** Return current UTC hour of day (0-23). */
static int currentUtcHour() {
    std::time_t now_t = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &now_t);
#else
    gmtime_r(&now_t, &utc);
#endif
    return utc.tm_hour;
}

// ──────────────────────────────────────────────────────────────────────────────
// Time-window ABAC tests
// ──────────────────────────────────────────────────────────────────────────────

class PolicyAbacTimeTest : public ::testing::Test {
protected:
    PolicyEngine engine;
    int now_hour = currentUtcHour();
};

TEST_F(PolicyAbacTimeTest, NoTimeWindowAlwaysPasses) {
    auto p = makeBasicPolicy("p1", "alice");
    // time_window fields default to -1 (no restriction)
    engine.addPolicy(p);
    auto d = engine.authorize("alice", "read", "/data/x");
    EXPECT_TRUE(d.allowed);
}

TEST_F(PolicyAbacTimeTest, TimeWindowIncludesCurrentHour) {
    // Build a window that definitely contains the current hour
    int start = (now_hour == 0) ? 0 : now_hour - 1;
    int end   = (now_hour == 23) ? 23 : now_hour + 1;

    auto p = makeBasicPolicy("p2", "alice");
    p.time_window_utc_hours_start = start;
    p.time_window_utc_hours_end   = end;
    engine.addPolicy(p);

    auto d = engine.authorize("alice", "read", "/data/x");
    EXPECT_TRUE(d.allowed) << "current hour=" << now_hour
                           << " start=" << start << " end=" << end;
}

TEST_F(PolicyAbacTimeTest, TimeWindowExcludesCurrentHour) {
    // Build a one-hour window that does NOT contain the current hour
    int start = (now_hour + 2) % 24;
    int end   = (now_hour + 3) % 24;
    // Skip if the window wraps and actually includes now_hour (edge case: hour 22/23)
    if ((start > end && (now_hour >= start || now_hour <= end)) {
        GTEST_SKIP() << "skipping: hour=" << now_hour << " wraps into test window";
    }
    if (start <= end && (now_hour >= start && now_hour <= end)) {
        GTEST_SKIP() << "skipping: calculated window unexpectedly includes current hour";
    }

    auto p = makeBasicPolicy("p3", "alice");
    p.time_window_utc_hours_start = start;
    p.time_window_utc_hours_end   = end;
    engine.addPolicy(p);

    auto d = engine.authorize("alice", "read", "/data/x");
    EXPECT_FALSE(d.allowed) << "current hour=" << now_hour
                            << " start=" << start << " end=" << end;
}

TEST_F(PolicyAbacTimeTest, OvernightWindowNormal) {
    // Overnight window e.g. 22-06: start > end
    // Test that it accepts hour 0 (always within 22-06 overnight range)
    // We cannot control the clock here, so just verify the policy serialises and
    // round-trips the fields correctly.
    auto p = makeBasicPolicy("p4", "alice");
    p.time_window_utc_hours_start = 22;
    p.time_window_utc_hours_end   = 6;

    engine.addPolicy(p);
    auto list = engine.listPolicies();
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].time_window_utc_hours_start, 22);
    EXPECT_EQ(list[0].time_window_utc_hours_end,   6);
}

TEST_F(PolicyAbacTimeTest, TimeWindowRoundTripsJson) {
    auto p = makeBasicPolicy("p5", "bob");
    p.time_window_utc_hours_start = 8;
    p.time_window_utc_hours_end   = 18;

    auto j  = PolicyEngine::toJson(p);
    auto p2 = PolicyEngine::fromJson(j);
    ASSERT_TRUE(p2.has_value());
    EXPECT_EQ(p2->time_window_utc_hours_start, 8);
    EXPECT_EQ(p2->time_window_utc_hours_end,  18);
}

TEST_F(PolicyAbacTimeTest, NoTimeWindowFieldInJsonWhenDefault) {
    auto p = makeBasicPolicy("p6", "bob");
    // Defaults: -1 / -1 -> should NOT appear in JSON
    auto j = PolicyEngine::toJson(p);
    EXPECT_FALSE(j.contains("time_window_utc_hours_start"));
    EXPECT_FALSE(j.contains("time_window_utc_hours_end"));
}

// ──────────────────────────────────────────────────────────────────────────────
// User-Agent ABAC tests
// ──────────────────────────────────────────────────────────────────────────────

class PolicyAbacUserAgentTest : public ::testing::Test {
protected:
    PolicyEngine engine;
};

TEST_F(PolicyAbacUserAgentTest, NoUAPatternNeverFilters) {
    engine.addPolicy(makeBasicPolicy("p1", "alice"));
    // No user_agent provided – should still be allowed
    auto d = engine.authorize("alice", "read", "/data");
    EXPECT_TRUE(d.allowed);
}

TEST_F(PolicyAbacUserAgentTest, MatchingUAPatternAllows) {
    auto p = makeBasicPolicy("p2", "alice");
    p.allowed_user_agent_patterns = {"ThemisClient"};
    engine.addPolicy(p);

    auto d = engine.authorize("alice", "read", "/data", std::nullopt, "ThemisClient/1.0");
    EXPECT_TRUE(d.allowed);
}

TEST_F(PolicyAbacUserAgentTest, NonMatchingUAPatternDenies) {
    auto p = makeBasicPolicy("p3", "alice");
    p.allowed_user_agent_patterns = {"ThemisClient"};
    engine.addPolicy(p);

    auto d = engine.authorize("alice", "read", "/data", std::nullopt, "curl/7.68.0");
    EXPECT_FALSE(d.allowed);
}

TEST_F(PolicyAbacUserAgentTest, MissingUADeniesWhenPatternsRequired) {
    auto p = makeBasicPolicy("p4", "alice");
    p.allowed_user_agent_patterns = {"ThemisClient"};
    engine.addPolicy(p);

    // No user_agent supplied
    auto d = engine.authorize("alice", "read", "/data", std::nullopt, std::nullopt);
    EXPECT_FALSE(d.allowed);
}

TEST_F(PolicyAbacUserAgentTest, MultiplePatternFirstMatchSuffices) {
    auto p = makeBasicPolicy("p5", "alice");
    p.allowed_user_agent_patterns = {"SDK-Go", "SDK-Python", "SDK-Java"};
    engine.addPolicy(p);

    EXPECT_TRUE(engine.authorize("alice", "read", "/data", std::nullopt, "ThemisDB SDK-Go/2.1").allowed);
    EXPECT_TRUE(engine.authorize("alice", "read", "/data", std::nullopt, "ThemisDB SDK-Python/3.0").allowed);
    EXPECT_FALSE(engine.authorize("alice", "read", "/data", std::nullopt, "UnknownClient/1.0").allowed);
}

TEST_F(PolicyAbacUserAgentTest, UARoundTripsJson) {
    auto p = makeBasicPolicy("p6", "alice");
    p.allowed_user_agent_patterns = {"ThemisClient", "SDK-"};

    auto j  = PolicyEngine::toJson(p);
    ASSERT_TRUE(j.contains("allowed_user_agent_patterns"));
    auto p2 = PolicyEngine::fromJson(j);
    ASSERT_TRUE(p2.has_value());
    EXPECT_EQ(p2->allowed_user_agent_patterns.size(), 2u);
}

TEST_F(PolicyAbacUserAgentTest, NoUAPatternFieldInJsonWhenEmpty) {
    auto p = makeBasicPolicy("p7", "alice");
    auto j = PolicyEngine::toJson(p);
    EXPECT_FALSE(j.contains("allowed_user_agent_patterns"));
}

// ──────────────────────────────────────────────────────────────────────────────
// Combined IP + UA + time conditions
// ──────────────────────────────────────────────────────────────────────────────

TEST(PolicyAbacCombinedTest, AllConditionsMustPass) {
    PolicyEngine engine;
    int now_h = currentUtcHour();
    int start = (now_h == 0) ? 0 : now_h - 1;
    int end   = (now_h == 23) ? 23 : now_h + 1;

    PolicyEngine::Policy p;
    p.id           = "combined";
    p.subjects     = {"alice"};
    p.actions      = {"read"};
    p.resources    = {"/secure"};
    p.effect_allow = true;
    p.allowed_ip_prefixes             = {"10.0."};
    p.time_window_utc_hours_start     = start;
    p.time_window_utc_hours_end       = end;
    p.allowed_user_agent_patterns     = {"TrustedSDK"};
    engine.addPolicy(p);

    // All three conditions satisfied
    EXPECT_TRUE(engine.authorize("alice", "read", "/secure",
                                 "10.0.1.5", "TrustedSDK/1.0").allowed);

    // IP condition fails
    EXPECT_FALSE(engine.authorize("alice", "read", "/secure",
                                  "192.168.0.1", "TrustedSDK/1.0").allowed);

    // UA condition fails
    EXPECT_FALSE(engine.authorize("alice", "read", "/secure",
                                  "10.0.1.5", "UnknownSDK/1.0").allowed);

    // Both IP and UA fail
    EXPECT_FALSE(engine.authorize("alice", "read", "/secure",
                                  "8.8.8.8", "curl/7.68").allowed);
}

TEST(PolicyAbacCombinedTest, DenyPolicyWithUACondition) {
    PolicyEngine engine;

    // Allow policy for everyone on internal IPs
    PolicyEngine::Policy allow_p;
    allow_p.id           = "internal-allow";
    allow_p.subjects     = {"*"};
    allow_p.actions      = {"read"};
    allow_p.resources    = {"/data"};
    allow_p.effect_allow = true;
    allow_p.allowed_ip_prefixes = {"10."};
    engine.addPolicy(allow_p);

    // Deny policy for scrapers (matched by UA)
    PolicyEngine::Policy deny_p;
    deny_p.id           = "deny-scraper";
    deny_p.subjects     = {"*"};
    deny_p.actions      = {"read"};
    deny_p.resources    = {"/data"};
    deny_p.effect_allow = false;
    deny_p.allowed_user_agent_patterns = {"Scrapy", "wget"};
    engine.addPolicy(deny_p);

    // Legitimate request from internal IP with normal UA (hits first allow policy)
    EXPECT_TRUE(engine.authorize("alice", "read", "/data/x",
                                 "10.0.0.1", "ThemisClient/1.0").allowed);
}
