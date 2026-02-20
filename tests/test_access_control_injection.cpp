/**
 * @file test_access_control_injection.cpp
 * @brief Tests for AQL injection detection in AccessControl
 *
 * These tests verify that AccessControl::detectSQLInjection correctly uses
 * the AQLInjectionDetector plus the heuristic fallback to identify dangerous
 * query patterns.
 */

#include <gtest/gtest.h>
#include "security/access_control.h"

using namespace themis::security;

// ============================================================================
// Test fixture
// ============================================================================

class InjectionDetectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        AccessControl::Config cfg;
        cfg.threat_detection_config.enable_sql_injection_detection    = true;
        cfg.threat_detection_config.enable_suspicious_query_detection = true;
        ac_ = std::make_unique<AccessControl>(cfg);
    }

    std::unique_ptr<AccessControl> ac_;
};

// ============================================================================
// detectSQLInjection – safe queries
// ============================================================================

TEST_F(InjectionDetectionTest, SafeQuery_FOR_FILTER_RETURN_IsClean) {
    EXPECT_FALSE(ac_->detectSQLInjection(
        "FOR doc IN users FILTER doc.age > 18 RETURN doc"));
}

TEST_F(InjectionDetectionTest, SafeQuery_EmptyString_IsClean) {
    EXPECT_FALSE(ac_->detectSQLInjection(""));
}

TEST_F(InjectionDetectionTest, SafeQuery_BindVarParameter_IsClean) {
    EXPECT_FALSE(ac_->detectSQLInjection(
        "FOR u IN users FILTER u.name == @name RETURN u"));
}

// ============================================================================
// detectSQLInjection – heuristic patterns
// ============================================================================

TEST_F(InjectionDetectionTest, HeuristicPattern_DropTable_Detected) {
    EXPECT_TRUE(ac_->detectSQLInjection("'; DROP TABLE users; --"));
}

TEST_F(InjectionDetectionTest, HeuristicPattern_OrTrue_Detected) {
    EXPECT_TRUE(ac_->detectSQLInjection("' OR '1'='1"));
}

TEST_F(InjectionDetectionTest, HeuristicPattern_UnionSelect_Detected) {
    EXPECT_TRUE(ac_->detectSQLInjection("UNION SELECT * FROM secrets"));
}

TEST_F(InjectionDetectionTest, HeuristicPattern_CommentDash_Detected) {
    EXPECT_TRUE(ac_->detectSQLInjection("admin' -- password=anything"));
}

TEST_F(InjectionDetectionTest, HeuristicPattern_BlockComment_Open_Detected) {
    EXPECT_TRUE(ac_->detectSQLInjection("/* injected comment"));
}

TEST_F(InjectionDetectionTest, HeuristicPattern_BlockComment_Close_Detected) {
    EXPECT_TRUE(ac_->detectSQLInjection("*/ UNION SELECT password FROM users"));
}

// ============================================================================
// detectSQLInjection – disabled config
// ============================================================================

TEST(InjectionDetectionDisabledTest, WhenDisabled_AlwaysReturnsFalse) {
    AccessControl::Config cfg;
    cfg.threat_detection_config.enable_sql_injection_detection = false;
    AccessControl ac(cfg);

    // Even a known-bad query returns false when detection is off
    EXPECT_FALSE(ac.detectSQLInjection("'; DROP TABLE users; --"));
    EXPECT_FALSE(ac.detectSQLInjection("' OR '1'='1"));
    EXPECT_FALSE(ac.detectSQLInjection("UNION SELECT * FROM secrets"));
}

// ============================================================================
// detectSuspiciousQuery – oversized query
// ============================================================================

TEST_F(InjectionDetectionTest, OversizedQuery_Detected) {
    std::string giant(10001, 'A');
    EXPECT_TRUE(ac_->detectSuspiciousQuery(giant, "attacker"));
}

TEST_F(InjectionDetectionTest, NormalSizeQuery_NotSuspicious) {
    EXPECT_FALSE(ac_->detectSuspiciousQuery(
        "FOR doc IN collection FILTER doc.id == 1 RETURN doc", "alice"));
}

TEST_F(InjectionDetectionTest, InjectionInSuspiciousQuery_Detected) {
    EXPECT_TRUE(ac_->detectSuspiciousQuery(
        "'; DROP TABLE users; --", "attacker"));
}

// ============================================================================
// detectSuspiciousQuery – disabled config
// ============================================================================

TEST(SuspiciousQueryDisabledTest, WhenDisabled_AlwaysReturnsFalse) {
    AccessControl::Config cfg;
    cfg.threat_detection_config.enable_suspicious_query_detection = false;
    cfg.threat_detection_config.enable_sql_injection_detection    = false;
    AccessControl ac(cfg);

    std::string giant(10001, 'A');
    EXPECT_FALSE(ac.detectSuspiciousQuery(giant, "attacker"));
    EXPECT_FALSE(ac.detectSuspiciousQuery("'; DROP TABLE users; --", "attacker"));
}
