#include <gtest/gtest.h>
#include "auth/password_policy.h"
#include <string>
#include <vector>

using namespace themis::auth;

// ============================================================================
// Default policy (12 chars, all four classes required)
// ============================================================================

TEST(PasswordPolicyTest, DefaultPolicy_ValidPassword) {
    PasswordPolicy policy;
    auto result = policy.validate("Secure#Pass1!");
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.violations.empty());
    EXPECT_TRUE(static_cast<bool>(result));
}

TEST(PasswordPolicyTest, DefaultPolicy_TooShort) {
    PasswordPolicy policy;
    auto result = policy.validate("Ab1!");
    EXPECT_FALSE(result.valid);
    ASSERT_FALSE(result.violations.empty());
    EXPECT_NE(result.violations[0].find("at least"), std::string::npos);
}

TEST(PasswordPolicyTest, DefaultPolicy_TooLong) {
    PasswordPolicy policy;
    // 129 characters — exceeds default max_length of 128
    // 120 lowercase + "ABCDE1!fg" (9 chars) = 129 chars total
    std::string long_pwd = std::string(120, 'a') + "ABCDE1!fg";
    auto result = policy.validate(long_pwd);
    EXPECT_FALSE(result.valid);
    bool has_length_violation = false;
    for (const auto& v : result.violations) {
        if (v.find("exceed") != std::string::npos) {
            has_length_violation = true;
            break;
        }
    }
    EXPECT_TRUE(has_length_violation);
}

TEST(PasswordPolicyTest, DefaultPolicy_MissingUppercase) {
    PasswordPolicy policy;
    auto result = policy.validate("secure#pass12!");
    EXPECT_FALSE(result.valid);
    bool found = false;
    for (const auto& v : result.violations) {
        if (v.find("uppercase") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(PasswordPolicyTest, DefaultPolicy_MissingLowercase) {
    PasswordPolicy policy;
    auto result = policy.validate("SECURE#PASS12!");
    EXPECT_FALSE(result.valid);
    bool found = false;
    for (const auto& v : result.violations) {
        if (v.find("lowercase") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(PasswordPolicyTest, DefaultPolicy_MissingDigit) {
    PasswordPolicy policy;
    auto result = policy.validate("Secure#Password!");
    EXPECT_FALSE(result.valid);
    bool found = false;
    for (const auto& v : result.violations) {
        if (v.find("digit") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(PasswordPolicyTest, DefaultPolicy_MissingSpecial) {
    PasswordPolicy policy;
    auto result = policy.validate("SecurePassword1");
    EXPECT_FALSE(result.valid);
    bool found = false;
    for (const auto& v : result.violations) {
        if (v.find("special") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

// ============================================================================
// Multiple violations reported simultaneously
// ============================================================================

TEST(PasswordPolicyTest, MultipleViolations) {
    PasswordPolicy policy;
    // Missing uppercase, digit, special; too short
    auto result = policy.validate("abc");
    EXPECT_FALSE(result.valid);
    EXPECT_GE(result.violations.size(), 2u);
}

// ============================================================================
// Custom special characters
// ============================================================================

TEST(PasswordPolicyTest, CustomSpecialChars_Accept) {
    PasswordPolicy::Config cfg;
    cfg.min_length = 8;
    cfg.require_special = true;
    cfg.special_chars = "_-";  // Only underscore and dash
    PasswordPolicy policy(cfg);

    auto result = policy.validate("Password_1");
    EXPECT_TRUE(result.valid);
}

TEST(PasswordPolicyTest, CustomSpecialChars_Reject) {
    PasswordPolicy::Config cfg;
    cfg.min_length = 8;
    cfg.require_special = true;
    cfg.special_chars = "_-";
    PasswordPolicy policy(cfg);

    // '!' is NOT in the custom special chars set
    auto result = policy.validate("Password!1");
    EXPECT_FALSE(result.valid);
}

// ============================================================================
// Minimum unique characters
// ============================================================================

TEST(PasswordPolicyTest, MinUniqueChars_Pass) {
    PasswordPolicy::Config cfg;
    cfg.min_length = 8;
    cfg.require_uppercase = false;
    cfg.require_lowercase = false;
    cfg.require_digit = false;
    cfg.require_special = false;
    cfg.min_unique_chars = 4;
    PasswordPolicy policy(cfg);

    EXPECT_TRUE(policy.isCompliant("abcdabcd"));  // 4 unique: a, b, c, d
}

TEST(PasswordPolicyTest, MinUniqueChars_Fail) {
    PasswordPolicy::Config cfg;
    cfg.min_length = 8;
    cfg.require_uppercase = false;
    cfg.require_lowercase = false;
    cfg.require_digit = false;
    cfg.require_special = false;
    cfg.min_unique_chars = 5;
    PasswordPolicy policy(cfg);

    EXPECT_FALSE(policy.isCompliant("abcdabcd"));  // Only 4 unique chars
}

// ============================================================================
// Maximum consecutive identical characters
// ============================================================================

TEST(PasswordPolicyTest, MaxConsecutiveIdentical_Pass) {
    PasswordPolicy::Config cfg;
    cfg.min_length = 8;
    cfg.require_uppercase = false;
    cfg.require_lowercase = false;
    cfg.require_digit = false;
    cfg.require_special = false;
    cfg.max_consecutive_identical = 2;
    PasswordPolicy policy(cfg);

    EXPECT_TRUE(policy.isCompliant("aabbccdd"));   // max run = 2 — OK
}

TEST(PasswordPolicyTest, MaxConsecutiveIdentical_Fail) {
    PasswordPolicy::Config cfg;
    cfg.min_length = 8;
    cfg.require_uppercase = false;
    cfg.require_lowercase = false;
    cfg.require_digit = false;
    cfg.require_special = false;
    cfg.max_consecutive_identical = 2;
    PasswordPolicy policy(cfg);

    EXPECT_FALSE(policy.isCompliant("aaabbbccc"));  // run of 3 — fail
}

// ============================================================================
// Forbidden patterns
// ============================================================================

TEST(PasswordPolicyTest, ForbiddenPattern_Blocked) {
    PasswordPolicy::Config cfg;
    cfg.min_length = 8;
    cfg.require_uppercase = false;
    cfg.require_lowercase = false;
    cfg.require_digit = false;
    cfg.require_special = false;
    cfg.forbidden_patterns = {"password", "qwerty"};
    PasswordPolicy policy(cfg);

    EXPECT_FALSE(policy.isCompliant("MyPassword123"));   // contains "password"
    EXPECT_FALSE(policy.isCompliant("Qwerty1234567"));   // contains "qwerty" (case-insensitive)
}

TEST(PasswordPolicyTest, ForbiddenPattern_Allowed) {
    PasswordPolicy::Config cfg;
    cfg.min_length = 8;
    cfg.require_uppercase = false;
    cfg.require_lowercase = false;
    cfg.require_digit = false;
    cfg.require_special = false;
    cfg.forbidden_patterns = {"password"};
    PasswordPolicy policy(cfg);

    EXPECT_TRUE(policy.isCompliant("hunter2xy_z!"));   // does not match
}

TEST(PasswordPolicyTest, ForbiddenPattern_MalformedRegex) {
    PasswordPolicy::Config cfg;
    cfg.min_length = 4;
    cfg.require_uppercase = false;
    cfg.require_lowercase = false;
    cfg.require_digit = false;
    cfg.require_special = false;
    cfg.forbidden_patterns = {"[invalid("};  // malformed regex — must not throw
    PasswordPolicy policy(cfg);

    EXPECT_NO_THROW(policy.isCompliant("testpass"));
}

// ============================================================================
// isCompliant convenience method
// ============================================================================

TEST(PasswordPolicyTest, IsCompliant_TrueForValid) {
    PasswordPolicy policy;
    EXPECT_TRUE(policy.isCompliant("Secure#Pass1!"));
}

TEST(PasswordPolicyTest, IsCompliant_FalseForInvalid) {
    PasswordPolicy policy;
    EXPECT_FALSE(policy.isCompliant("short"));
}

// ============================================================================
// getConfig / setConfig round-trip
// ============================================================================

TEST(PasswordPolicyTest, SetConfig_Updates) {
    PasswordPolicy policy;
    PasswordPolicy::Config cfg = policy.getConfig();
    cfg.min_length = 20;
    policy.setConfig(cfg);
    EXPECT_EQ(policy.getConfig().min_length, 20u);
    EXPECT_FALSE(policy.isCompliant("Secure#Pass1!"));  // < 20 chars now
}

// ============================================================================
// Preset: NIST guidelines
// ============================================================================

TEST(PasswordPolicyTest, Preset_NIST_AcceptsSimplePassword) {
    auto policy = PasswordPolicy::nistGuidelines();
    // NIST only requires length >= 8; no character-class requirements
    EXPECT_TRUE(policy.isCompliant("longenoughpass"));
    EXPECT_TRUE(policy.isCompliant("alllowercase"));
}

TEST(PasswordPolicyTest, Preset_NIST_RejectsTooShort) {
    auto policy = PasswordPolicy::nistGuidelines();
    EXPECT_FALSE(policy.isCompliant("short"));  // 5 chars < 8
}

// ============================================================================
// Preset: strict
// ============================================================================

TEST(PasswordPolicyTest, Preset_Strict_AcceptsComplexPassword) {
    auto policy = PasswordPolicy::strict();
    // 16+ chars, upper, lower, digit, special, >=8 unique, max 2 consecutive
    EXPECT_TRUE(policy.isCompliant("Secure@Pass1!XYZab"));
}

TEST(PasswordPolicyTest, Preset_Strict_RejectsTooShort) {
    auto policy = PasswordPolicy::strict();
    EXPECT_FALSE(policy.isCompliant("Sec@Pass1!"));  // < 16 chars
}

// ============================================================================
// Preset: basic
// ============================================================================

TEST(PasswordPolicyTest, Preset_Basic_AcceptsEightCharPassword) {
    auto policy = PasswordPolicy::basic();
    // Basic: >= 8 chars, upper + lower + digit required; special not required
    EXPECT_TRUE(policy.isCompliant("Password1"));
}

TEST(PasswordPolicyTest, Preset_Basic_RejectsAllLowercase) {
    auto policy = PasswordPolicy::basic();
    EXPECT_FALSE(policy.isCompliant("password1"));  // Missing uppercase
}

// ============================================================================
// Empty password edge case
// ============================================================================

TEST(PasswordPolicyTest, EmptyPassword_Fails) {
    PasswordPolicy policy;
    auto result = policy.validate("");
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.violations.empty());
}

// ============================================================================
// Shannon entropy checks
// ============================================================================

TEST(PasswordPolicyTest, ComputeEntropy_EmptyPassword) {
    EXPECT_DOUBLE_EQ(PasswordPolicy::computeEntropy(""), 0.0);
}

TEST(PasswordPolicyTest, ComputeEntropy_AllSameChars) {
    // "aaaa" → only one distinct char → 0 bits entropy
    EXPECT_DOUBLE_EQ(PasswordPolicy::computeEntropy("aaaa"), 0.0);
}

TEST(PasswordPolicyTest, ComputeEntropy_TwoDifferentChars) {
    // "abab" → 2 equally-likely chars → H = 1.0 bit/char × 4 chars = 4.0 bits
    EXPECT_NEAR(PasswordPolicy::computeEntropy("abab"), 4.0, 1e-9);
}

TEST(PasswordPolicyTest, ComputeEntropy_GrowsWithDiversity) {
    // More distinct characters → higher entropy
    double low  = PasswordPolicy::computeEntropy("aaabbbccc");
    double high = PasswordPolicy::computeEntropy("abcdefghi");
    EXPECT_GT(high, low);
}

TEST(PasswordPolicyTest, MinEntropy_Pass) {
    PasswordPolicy::Config cfg;
    cfg.min_length = 4;
    cfg.require_uppercase = false;
    cfg.require_lowercase = false;
    cfg.require_digit = false;
    cfg.require_special = false;
    cfg.min_entropy_bits = 4.0;
    PasswordPolicy policy(cfg);

    // "abcd" → 4 equally-likely distinct chars, p=0.25 each,
    //  H = -4*(0.25*log2(0.25))*4 = log2(4)*4 = 8.0 bits > 4 → pass
    EXPECT_TRUE(policy.isCompliant("abcd"));
}

TEST(PasswordPolicyTest, MinEntropy_Fail) {
    PasswordPolicy::Config cfg;
    cfg.min_length = 4;
    cfg.require_uppercase = false;
    cfg.require_lowercase = false;
    cfg.require_digit = false;
    cfg.require_special = false;
    cfg.min_entropy_bits = 10.0;
    PasswordPolicy policy(cfg);

    // "aaaa" → 0 bits entropy < 10 → fail
    EXPECT_FALSE(policy.isCompliant("aaaa"));
}

TEST(PasswordPolicyTest, MinEntropy_ViolationMessage) {
    PasswordPolicy::Config cfg;
    cfg.min_length = 4;
    cfg.require_uppercase = false;
    cfg.require_lowercase = false;
    cfg.require_digit = false;
    cfg.require_special = false;
    cfg.min_entropy_bits = 10.0;
    PasswordPolicy policy(cfg);

    auto result = policy.validate("aaaa");
    EXPECT_FALSE(result.valid);
    bool found = false;
    for (const auto& v : result.violations) {
        if (v.find("entropy") != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(PasswordPolicyTest, MinEntropy_ZeroDisablesCheck) {
    PasswordPolicy::Config cfg;
    cfg.min_length = 4;
    cfg.require_uppercase = false;
    cfg.require_lowercase = false;
    cfg.require_digit = false;
    cfg.require_special = false;
    cfg.min_entropy_bits = 0.0;  // disabled
    PasswordPolicy policy(cfg);

    // Passes because entropy check is disabled
    EXPECT_TRUE(policy.isCompliant("aaaa"));
}

TEST(PasswordPolicyTest, Preset_Strict_HasEntropyRequirement) {
    auto policy = PasswordPolicy::strict();
    EXPECT_GT(policy.getConfig().min_entropy_bits, 0.0);
}
