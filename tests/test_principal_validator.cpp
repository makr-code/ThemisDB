/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_principal_validator.cpp                       ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     488                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "auth/principal_validator.h"

using namespace themis::auth;

/**
 * @brief Test default deny behavior
 */
TEST(PrincipalValidatorTest, DefaultDeny) {
    PrincipalValidator::Config config;
    config.default_allow = false;
    PrincipalValidator validator(config);
    
    auto result = validator.validate("alice@EXAMPLE.COM");
    
    EXPECT_FALSE(result.allowed);
    EXPECT_FALSE(result.denial_reason.empty());
}

/**
 * @brief Test default allow behavior
 */
TEST(PrincipalValidatorTest, DefaultAllow) {
    PrincipalValidator::Config config;
    config.default_allow = true;
    PrincipalValidator validator(config);
    
    auto result = validator.validate("alice@EXAMPLE.COM");
    
    EXPECT_TRUE(result.allowed);
}

/**
 * @brief Test whitelist exact match
 */
TEST(PrincipalValidatorTest, WhitelistExactMatch) {
    PrincipalValidator::Config config;
    config.default_allow = false;
    
    PrincipalValidator::Rule rule;
    rule.type = PrincipalValidator::RuleType::WHITELIST;
    rule.pattern = "alice@EXAMPLE.COM";
    rule.is_regex = false;
    config.rules.push_back(rule);
    
    PrincipalValidator validator(config);
    
    EXPECT_TRUE(validator.validate("alice@EXAMPLE.COM").allowed);
    EXPECT_FALSE(validator.validate("bob@EXAMPLE.COM").allowed);
}

/**
 * @brief Test blacklist exact match
 */
TEST(PrincipalValidatorTest, BlacklistExactMatch) {
    PrincipalValidator::Config config;
    config.default_allow = true;
    
    PrincipalValidator::Rule rule;
    rule.type = PrincipalValidator::RuleType::BLACKLIST;
    rule.pattern = "malicious@EXAMPLE.COM";
    rule.is_regex = false;
    config.rules.push_back(rule);
    
    PrincipalValidator validator(config);
    
    EXPECT_FALSE(validator.validate("malicious@EXAMPLE.COM").allowed);
    EXPECT_TRUE(validator.validate("alice@EXAMPLE.COM").allowed);
}

/**
 * @brief Test blacklist takes precedence over whitelist
 */
TEST(PrincipalValidatorTest, BlacklistPrecedence) {
    PrincipalValidator::Config config;
    config.default_allow = false;
    
    // Whitelist
    PrincipalValidator::Rule whitelist;
    whitelist.type = PrincipalValidator::RuleType::WHITELIST;
    whitelist.pattern = ".*@EXAMPLE.COM";
    whitelist.is_regex = true;
    whitelist.priority = 100;
    config.rules.push_back(whitelist);
    
    // Blacklist specific user
    PrincipalValidator::Rule blacklist;
    blacklist.type = PrincipalValidator::RuleType::BLACKLIST;
    blacklist.pattern = "banned@EXAMPLE.COM";
    blacklist.is_regex = false;
    blacklist.priority = 1000;  // Higher priority
    config.rules.push_back(blacklist);
    
    PrincipalValidator validator(config);
    
    EXPECT_TRUE(validator.validate("alice@EXAMPLE.COM").allowed);
    EXPECT_FALSE(validator.validate("banned@EXAMPLE.COM").allowed);
}

/**
 * @brief Test regex whitelist
 */
TEST(PrincipalValidatorTest, RegexWhitelist) {
    PrincipalValidator::Config config;
    config.default_allow = false;
    
    PrincipalValidator::Rule rule;
    rule.type = PrincipalValidator::RuleType::WHITELIST;
    rule.pattern = "^[a-z]+@EXAMPLE\\.COM$";
    rule.is_regex = true;
    config.rules.push_back(rule);
    
    PrincipalValidator validator(config);
    
    EXPECT_TRUE(validator.validate("alice@EXAMPLE.COM").allowed);
    EXPECT_TRUE(validator.validate("bob@EXAMPLE.COM").allowed);
    EXPECT_FALSE(validator.validate("alice@OTHER.COM").allowed);
    EXPECT_FALSE(validator.validate("Alice123@EXAMPLE.COM").allowed);  // Has numbers
}

/**
 * @brief Test regex blacklist
 */
TEST(PrincipalValidatorTest, RegexBlacklist) {
    PrincipalValidator::Config config;
    config.default_allow = true;
    
    PrincipalValidator::Rule rule;
    rule.type = PrincipalValidator::RuleType::REGEX_DENY;
    rule.pattern = "^(service|svc)-.*@EXAMPLE\\.COM$";
    rule.is_regex = true;
    config.rules.push_back(rule);
    
    PrincipalValidator validator(config);
    
    EXPECT_FALSE(validator.validate("service-account@EXAMPLE.COM").allowed);
    EXPECT_FALSE(validator.validate("svc-app@EXAMPLE.COM").allowed);
    EXPECT_TRUE(validator.validate("alice@EXAMPLE.COM").allowed);
}

/**
 * @brief Test role mapping with exact match
 */
TEST(PrincipalValidatorTest, RoleMappingExact) {
    PrincipalValidator::Config config;
    config.default_allow = true;
    
    PrincipalValidator::MappingRule mapping;
    mapping.principal_pattern = "admin@EXAMPLE.COM";
    mapping.is_regex = false;
    mapping.roles = {"admin", "user"};
    config.mapping_rules.push_back(mapping);
    
    PrincipalValidator validator(config);
    
    auto result = validator.validate("admin@EXAMPLE.COM");
    EXPECT_TRUE(result.allowed);
    EXPECT_EQ(result.roles.size(), 2);
    EXPECT_TRUE(std::find(result.roles.begin(), result.roles.end(), "admin") != result.roles.end());
    EXPECT_TRUE(std::find(result.roles.begin(), result.roles.end(), "user") != result.roles.end());
}

/**
 * @brief Test role mapping with wildcard
 */
TEST(PrincipalValidatorTest, RoleMappingWildcard) {
    PrincipalValidator::Config config;
    config.default_allow = true;
    
    PrincipalValidator::MappingRule mapping;
    mapping.principal_pattern = "*@EXAMPLE.COM";
    mapping.is_regex = false;
    mapping.roles = {"user"};
    config.mapping_rules.push_back(mapping);
    
    PrincipalValidator validator(config);
    
    auto result = validator.validate("alice@EXAMPLE.COM");
    EXPECT_TRUE(result.allowed);
    EXPECT_EQ(result.roles.size(), 1);
    EXPECT_EQ(result.roles[0], "user");
}

/**
 * @brief Test role mapping with regex
 */
TEST(PrincipalValidatorTest, RoleMappingRegex) {
    PrincipalValidator::Config config;
    config.default_allow = true;
    
    PrincipalValidator::MappingRule mapping;
    mapping.principal_pattern = "^dev-.*@EXAMPLE\\.COM$";
    mapping.is_regex = true;
    mapping.roles = {"developer", "user"};
    config.mapping_rules.push_back(mapping);
    
    PrincipalValidator validator(config);
    
    auto result = validator.validate("dev-alice@EXAMPLE.COM");
    EXPECT_TRUE(result.allowed);
    EXPECT_EQ(result.roles.size(), 2);
    
    auto result2 = validator.validate("alice@EXAMPLE.COM");
    EXPECT_TRUE(result2.allowed);
    EXPECT_EQ(result2.roles.size(), 0);  // No mapping
}

/**
 * @brief Test multiple mapping rules accumulate roles
 */
TEST(PrincipalValidatorTest, MultipleRoleMappings) {
    PrincipalValidator::Config config;
    config.default_allow = true;
    
    // Map all realm users to "user" role
    PrincipalValidator::MappingRule user_mapping;
    user_mapping.principal_pattern = "*@EXAMPLE.COM";
    user_mapping.roles = {"user"};
    config.mapping_rules.push_back(user_mapping);
    
    // Map specific user to "admin" role
    PrincipalValidator::MappingRule admin_mapping;
    admin_mapping.principal_pattern = "admin@EXAMPLE.COM";
    admin_mapping.roles = {"admin"};
    config.mapping_rules.push_back(admin_mapping);
    
    PrincipalValidator validator(config);
    
    auto result = validator.validate("admin@EXAMPLE.COM");
    EXPECT_TRUE(result.allowed);
    EXPECT_EQ(result.roles.size(), 2);  // Both "user" and "admin"
}

/**
 * @brief Test case-insensitive matching
 */
TEST(PrincipalValidatorTest, CaseInsensitive) {
    PrincipalValidator::Config config;
    config.default_allow = false;
    config.case_sensitive = false;
    
    PrincipalValidator::Rule rule;
    rule.type = PrincipalValidator::RuleType::WHITELIST;
    rule.pattern = "alice@example.com";
    rule.is_regex = false;
    config.rules.push_back(rule);
    
    PrincipalValidator validator(config);
    
    EXPECT_TRUE(validator.validate("alice@example.com").allowed);
    EXPECT_TRUE(validator.validate("ALICE@EXAMPLE.COM").allowed);
    EXPECT_TRUE(validator.validate("Alice@Example.Com").allowed);
}

/**
 * @brief Test case-sensitive matching
 */
TEST(PrincipalValidatorTest, CaseSensitive) {
    PrincipalValidator::Config config;
    config.default_allow = false;
    config.case_sensitive = true;
    
    PrincipalValidator::Rule rule;
    rule.type = PrincipalValidator::RuleType::WHITELIST;
    rule.pattern = "alice@EXAMPLE.COM";
    rule.is_regex = false;
    config.rules.push_back(rule);
    
    PrincipalValidator validator(config);
    
    EXPECT_TRUE(validator.validate("alice@EXAMPLE.COM").allowed);
    EXPECT_FALSE(validator.validate("alice@example.com").allowed);
}

/**
 * @brief Test statistics tracking
 */
TEST(PrincipalValidatorTest, Statistics) {
    PrincipalValidator::Config config;
    config.default_allow = false;
    
    PrincipalValidator::Rule rule;
    rule.type = PrincipalValidator::RuleType::WHITELIST;
    rule.pattern = "alice@EXAMPLE.COM";
    config.rules.push_back(rule);
    
    PrincipalValidator validator(config);
    
    validator.validate("alice@EXAMPLE.COM");  // Allowed
    validator.validate("bob@EXAMPLE.COM");    // Denied
    validator.validate("charlie@EXAMPLE.COM"); // Denied
    
    auto stats = validator.getStatistics();
    
    EXPECT_EQ(stats.total_validations, 3);
    EXPECT_EQ(stats.allowed, 1);
    EXPECT_EQ(stats.denied, 2);
}

/**
 * @brief Test addRule method
 */
TEST(PrincipalValidatorTest, AddRule) {
    PrincipalValidator::Config config;
    config.default_allow = false;
    
    PrincipalValidator validator(config);
    
    // Initially nothing allowed
    EXPECT_FALSE(validator.validate("alice@EXAMPLE.COM").allowed);
    
    // Add rule
    PrincipalValidator::Rule rule;
    rule.type = PrincipalValidator::RuleType::WHITELIST;
    rule.pattern = "alice@EXAMPLE.COM";
    validator.addRule(rule);
    
    // Now allowed
    EXPECT_TRUE(validator.validate("alice@EXAMPLE.COM").allowed);
}

/**
 * @brief Test clearRules method
 */
TEST(PrincipalValidatorTest, ClearRules) {
    PrincipalValidator::Config config;
    config.default_allow = true;
    
    PrincipalValidator::Rule rule;
    rule.type = PrincipalValidator::RuleType::BLACKLIST;
    rule.pattern = "banned@EXAMPLE.COM";
    config.rules.push_back(rule);
    
    PrincipalValidator validator(config);
    
    // Initially banned
    EXPECT_FALSE(validator.validate("banned@EXAMPLE.COM").allowed);
    
    // Clear blacklist
    validator.clearRules(PrincipalValidator::RuleType::BLACKLIST);
    
    // Now allowed (falls through to default_allow)
    EXPECT_TRUE(validator.validate("banned@EXAMPLE.COM").allowed);
}

/**
 * @brief Test realm-restricted preset
 */
TEST(PrincipalValidatorPresetsTest, RealmRestricted) {
    auto validator = PrincipalValidatorPresets::realmRestricted("EXAMPLE.COM");
    
    EXPECT_TRUE(validator.validate("alice@EXAMPLE.COM").allowed);
    EXPECT_TRUE(validator.validate("bob@EXAMPLE.COM").allowed);
    EXPECT_FALSE(validator.validate("alice@OTHER.COM").allowed);
}

/**
 * @brief Test blacklist preset
 */
TEST(PrincipalValidatorPresetsTest, Blacklist) {
    auto validator = PrincipalValidatorPresets::withBlacklist({
        "banned1@EXAMPLE.COM",
        "banned2@EXAMPLE.COM"
    });
    
    EXPECT_FALSE(validator.validate("banned1@EXAMPLE.COM").allowed);
    EXPECT_FALSE(validator.validate("banned2@EXAMPLE.COM").allowed);
    EXPECT_TRUE(validator.validate("alice@EXAMPLE.COM").allowed);
}

/**
 * @brief Test whitelist preset
 */
TEST(PrincipalValidatorPresetsTest, Whitelist) {
    auto validator = PrincipalValidatorPresets::withWhitelist({
        "alice@EXAMPLE.COM",
        "bob@EXAMPLE.COM"
    });
    
    EXPECT_TRUE(validator.validate("alice@EXAMPLE.COM").allowed);
    EXPECT_TRUE(validator.validate("bob@EXAMPLE.COM").allowed);
    EXPECT_FALSE(validator.validate("charlie@EXAMPLE.COM").allowed);
}

/**
 * @brief Test enterprise standard preset
 */
TEST(PrincipalValidatorPresetsTest, EnterpriseStandard) {
    auto validator = PrincipalValidatorPresets::enterpriseStandard("EXAMPLE.COM");
    
    // Regular users allowed
    EXPECT_TRUE(validator.validate("alice@EXAMPLE.COM").allowed);
    EXPECT_TRUE(validator.validate("bob_smith@EXAMPLE.COM").allowed);
    
    // Service accounts blocked
    EXPECT_FALSE(validator.validate("service-account@EXAMPLE.COM").allowed);
    EXPECT_FALSE(validator.validate("svc_app@EXAMPLE.COM").allowed);
    
    // Admin accounts blocked (should use admin realm)
    EXPECT_FALSE(validator.validate("admin-user@EXAMPLE.COM").allowed);
    
    // Different realm blocked
    EXPECT_FALSE(validator.validate("alice@OTHER.COM").allowed);
    
    // Check role mapping for allowed user
    auto result = validator.validate("alice@EXAMPLE.COM");
    EXPECT_TRUE(result.allowed);
    EXPECT_FALSE(result.roles.empty());
    EXPECT_TRUE(std::find(result.roles.begin(), result.roles.end(), "user") != result.roles.end());
}

/**
 * @brief Test priority ordering
 */
TEST(PrincipalValidatorTest, PriorityOrdering) {
    PrincipalValidator::Config config;
    config.default_allow = false;
    
    // Low priority whitelist for realm
    PrincipalValidator::Rule whitelist;
    whitelist.type = PrincipalValidator::RuleType::WHITELIST;
    whitelist.pattern = ".*@EXAMPLE\\.COM";
    whitelist.is_regex = true;
    whitelist.priority = 100;
    config.rules.push_back(whitelist);
    
    // High priority blacklist for specific user
    PrincipalValidator::Rule blacklist;
    blacklist.type = PrincipalValidator::RuleType::BLACKLIST;
    blacklist.pattern = "evil@EXAMPLE.COM";
    blacklist.priority = 1000;
    config.rules.push_back(blacklist);
    
    PrincipalValidator validator(config);
    
    // Blacklist evaluated first due to higher priority
    EXPECT_FALSE(validator.validate("evil@EXAMPLE.COM").allowed);
    EXPECT_TRUE(validator.validate("alice@EXAMPLE.COM").allowed);
}

/**
 * @brief Test validation result contains all information
 */
TEST(PrincipalValidatorTest, ValidationResultComplete) {
    PrincipalValidator::Config config;
    config.default_allow = false;
    
    PrincipalValidator::Rule rule;
    rule.type = PrincipalValidator::RuleType::WHITELIST;
    rule.pattern = "alice@EXAMPLE.COM";
    rule.description = "Alice whitelist";
    config.rules.push_back(rule);
    
    PrincipalValidator::MappingRule mapping;
    mapping.principal_pattern = "alice@EXAMPLE.COM";
    mapping.roles = {"admin"};
    config.mapping_rules.push_back(mapping);
    
    PrincipalValidator validator(config);
    
    auto result = validator.validate("alice@EXAMPLE.COM");
    
    EXPECT_TRUE(result.allowed);
    EXPECT_EQ(result.principal, "alice@EXAMPLE.COM");
    EXPECT_FALSE(result.matched_rule.empty());
    EXPECT_EQ(result.roles.size(), 1);
    EXPECT_EQ(result.roles[0], "admin");
    EXPECT_TRUE(result.denial_reason.empty());
}
