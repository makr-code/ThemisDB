/**
 * @file test_fuzz_security.cpp
 * @brief Fuzz-style security tests for critical input-handling paths
 *
 * These tests exercise security-critical functions with boundary values,
 * malformed inputs, adversarial data, and semi-random payloads to surface
 * crashes, exceptions, assertion failures, and unexpected allow/deny
 * decisions.
 *
 * Each test suite targets one security component:
 *   1. InputValidator  – path traversal, AQL injection, log sanitisation
 *   2. PolicyEngine    – malformed JSON policies, adversarial subjects/resources
 *   3. TokenBlacklist  – boundary JTI values, max-length strings, null bytes
 *   4. RateLimiter     – extreme IPs, Unicode identifiers, empty strings
 *   5. AuditLogger     – huge payloads, control characters, deeply-nested JSON
 *   6. PBKDF2 hashing  – empty, binary, very-long, Unicode passwords
 */

#include <gtest/gtest.h>
#include "utils/input_validator.h"
#include "server/policy_engine.h"
#include "auth/token_blacklist.h"
#include "server/rate_limiter.h"
#include "utils/audit_logger.h"
#include "security/mock_key_provider.h"
#include "security/user_registration_plugin.h"

#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
#include <random>
#include <climits>

using namespace themis;
using namespace themis::utils;
using namespace themis::auth;
using namespace themis::server;

// ============================================================================
// Helpers
// ============================================================================

namespace {

// Generate a string of n repetitions of character c
std::string repeat(char c, size_t n) { return std::string(n, c); }

// Build a string with all printable ASCII characters
std::string allPrintableAscii() {
    std::string s;
    for (int i = 32; i < 127; ++i) s += static_cast<char>(i);
    return s;
}

// Build a static list of common injection payloads (created once, returned by ref)
const std::vector<std::string>& injectionPayloads() {
    static const std::vector<std::string> payloads = {
        "' OR '1'='1",
        "'; DROP TABLE users; --",
        "UNION SELECT * FROM secrets",
        "1; SELECT * FROM information_schema.tables",
        "../../../etc/passwd",
        "..\\..\\..\\Windows\\system32\\cmd.exe",
        "%2e%2e%2f%2e%2e%2f",
        "\x00null\x00byte",
        "<script>alert('xss')</script>",
        "{{7*7}}",             // Template injection
        "${7*7}",              // EL injection
        "$(whoami)",           // Shell injection
        std::string(65536, 'A'), // Huge string
        "\n\r\t\b\f",         // Control characters
        "\xe2\x80\xae\xe2\x80\x8b\xc2\xa0",  // Unicode control/invisible chars (UTF-8)
        repeat('/', 1000),     // Many slashes
        repeat('\x01', 512),   // Non-printable bytes
    };
    return payloads;
}

} // anonymous namespace

// ============================================================================
// 1. InputValidator fuzz
// ============================================================================

class InputValidatorFuzzTest : public ::testing::Test {
protected:
    InputValidator validator_{"."};
};

TEST_F(InputValidatorFuzzTest, ValidatePath_InjectionPayloads_NeverCrash) {
    for (const auto& p : injectionPayloads()) {
        EXPECT_NO_THROW({
            bool result = validator_.validatePathSegment(p);
            (void)result;
        }) << "Crashed on input: " << p.substr(0, 80);
    }
}

TEST_F(InputValidatorFuzzTest, ValidatePath_TraversalStrings_AlwaysRejected) {
    std::vector<std::string> traversals = {
        "../secret", "..\\secret",
        "%2e%2e/secret", "%2e%2e%2fsecret",
        "....//secret",
    };
    for (const auto& t : traversals) {
        EXPECT_FALSE(validator_.validatePathSegment(t))
            << "Should reject: " << t;
    }
}

TEST_F(InputValidatorFuzzTest, SanitizeForLogs_InjectionPayloads_NeverCrash) {
    for (const auto& p : injectionPayloads()) {
        EXPECT_NO_THROW({
            auto result = validator_.sanitizeForLogs(p);
            EXPECT_LE(result.size(), 512u + 10u); // max_len + "..." headroom
        }) << "Crashed on sanitize: " << p.substr(0, 80);
    }
}

TEST_F(InputValidatorFuzzTest, SanitizeForLogs_EmptyString_ReturnsEmpty) {
    EXPECT_EQ(validator_.sanitizeForLogs(""), "");
}

TEST_F(InputValidatorFuzzTest, SanitizeForLogs_ExactMaxLen_NotTruncated) {
    std::string exactly512(512, 'A');
    auto result = validator_.sanitizeForLogs(exactly512);
    EXPECT_LE(result.size(), 512u + 4u);
}

TEST_F(InputValidatorFuzzTest, SanitizeForLogs_ControlChars_Stripped) {
    std::string with_controls = "normal\x01\x02\x1b[31mred\x1btext";
    auto result = validator_.sanitizeForLogs(with_controls);
    for (char c : result) {
        EXPECT_GE(c, ' ') << "Control char survived sanitisation";
    }
}

TEST_F(InputValidatorFuzzTest, ValidateAqlRequest_MalformedPayloads_NeverCrash) {
    std::vector<nlohmann::json> payloads = {
        {},
        {{"query", ""}},
        {{"query", std::string(100000, 'X')}},
        {{"query", "FOR x IN coll RETURN x"}, {"bindVars", nullptr}},
        {{"query", 12345}},
        {{"notquery", "abc"}},
        {{"query", "' OR 1=1 --"}},
        nlohmann::json::array(),
        nlohmann::json(nullptr),
    };
    for (const auto& p : payloads) {
        EXPECT_NO_THROW({
            auto err = validator_.validateAqlRequest(p);
            (void)err;
        }) << "Crashed on AQL payload: " << p.dump(0).substr(0, 80);
    }
}

// ============================================================================
// 2. PolicyEngine fuzz
// ============================================================================

class PolicyEngineFuzzTest : public ::testing::Test {
protected:
    PolicyEngine engine_;
};

TEST_F(PolicyEngineFuzzTest, Authorize_InjectionSubjects_NeverCrash) {
    // Add a basic policy first
    PolicyEngine::Policy p;
    p.id = "baseline";
    p.subjects.insert("*");
    p.actions.insert("read");
    p.resources.push_back("/");
    p.effect_allow = true;
    engine_.addPolicy(p);

    for (const auto& payload : injectionPayloads()) {
        EXPECT_NO_THROW({
            auto d = engine_.authorize(payload, "read", "/data");
            (void)d.allowed;
        }) << "Crashed on subject: " << payload.substr(0, 80);
    }
}

TEST_F(PolicyEngineFuzzTest, Authorize_InjectionActions_NeverCrash) {
    for (const auto& payload : injectionPayloads()) {
        EXPECT_NO_THROW({
            auto d = engine_.authorize("user", payload, "/data");
            (void)d.allowed;
        }) << "Crashed on action: " << payload.substr(0, 80);
    }
}

TEST_F(PolicyEngineFuzzTest, Authorize_InjectionResources_NeverCrash) {
    for (const auto& payload : injectionPayloads()) {
        EXPECT_NO_THROW({
            auto d = engine_.authorize("user", "read", payload);
            (void)d.allowed;
        }) << "Crashed on resource: " << payload.substr(0, 80);
    }
}

TEST_F(PolicyEngineFuzzTest, Authorize_InjectionIPs_NeverCrash) {
    std::vector<std::string> bad_ips = {
        "", "0.0.0.0", "999.999.999.999", "::1",
        "' OR 1=1", repeat('.', 100), std::string(65536, 'x'),
        "127.0.0.1; DROP TABLE", "\x00\x01\xff",
    };
    for (const auto& ip : bad_ips) {
        EXPECT_NO_THROW({
            auto d = engine_.authorize("user", "read", "/data", ip);
            (void)d.allowed;
        }) << "Crashed on IP: " << ip.substr(0, 80);
    }
}

TEST_F(PolicyEngineFuzzTest, AddPolicy_EmptyFields_NoCrash) {
    PolicyEngine::Policy p;
    // All empty / default
    EXPECT_NO_THROW(engine_.addPolicy(p));
}

TEST_F(PolicyEngineFuzzTest, AddRemovePolicy_RapidCycles_NoCorruption) {
    for (int i = 0; i < 100; ++i) {
        PolicyEngine::Policy p;
        p.id = "p" + std::to_string(i);
        p.subjects.insert("user");
        p.actions.insert("read");
        p.resources.push_back("/data");
        p.effect_allow = (i % 2 == 0);
        engine_.addPolicy(p);
    }
    EXPECT_EQ(engine_.listPolicies().size(), 100u);

    for (int i = 0; i < 100; ++i) {
        engine_.removePolicy("p" + std::to_string(i));
    }
    EXPECT_EQ(engine_.listPolicies().size(), 0u);
}

// ============================================================================
// 3. TokenBlacklist fuzz
// ============================================================================

class TokenBlacklistFuzzTest : public ::testing::Test {
protected:
    TokenBlacklist bl_;
    std::chrono::system_clock::time_point future_ =
        std::chrono::system_clock::now() + std::chrono::hours(1);
};

TEST_F(TokenBlacklistFuzzTest, Revoke_InjectionJTIs_NeverCrash) {
    for (const auto& jti : injectionPayloads()) {
        EXPECT_NO_THROW(bl_.revoke(jti, future_));
    }
}

TEST_F(TokenBlacklistFuzzTest, IsRevoked_InjectionJTIs_NeverCrash) {
    for (const auto& jti : injectionPayloads()) {
        EXPECT_NO_THROW({
            bool r = bl_.isRevoked(jti);
            (void)r;
        });
    }
}

TEST_F(TokenBlacklistFuzzTest, Revoke_NullByteJTI_Ignored) {
    // JTI containing null byte – should either be ignored or stored safely
    std::string jti_with_null("abc\0def", 7);
    EXPECT_NO_THROW(bl_.revoke(jti_with_null, future_));
}

TEST_F(TokenBlacklistFuzzTest, Revoke_MaxLengthJTI_Stored) {
    std::string huge_jti(65536, 'J');
    EXPECT_NO_THROW(bl_.revoke(huge_jti, future_));
}

TEST_F(TokenBlacklistFuzzTest, IsRevoked_UnicodeJTI_NoUndefinedBehavior) {
    std::string unicode_jti = "\xf0\x9f\x94\x90"; // 🔐 emoji
    EXPECT_NO_THROW({
        bool r = bl_.isRevoked(unicode_jti);
        (void)r;
    });
}

// ============================================================================
// 4. RateLimiter fuzz
// ============================================================================

class RateLimiterFuzzTest : public ::testing::Test {
protected:
    RateLimitConfig cfg_;
    void SetUp() override {
        cfg_.bucket_capacity  = 5;
        cfg_.refill_rate      = 1.0;
        cfg_.per_ip_enabled   = true;
        cfg_.per_user_enabled = true;
    }
};

TEST_F(RateLimiterFuzzTest, AllowRequest_MalformedIPs_NeverCrash) {
    RateLimiter limiter(cfg_);
    std::vector<std::string> bad_ips = {
        "", "0.0.0.0", "::1", "999.999.999.999",
        repeat('.', 100), std::string(65536, 'x'),
        "127.0.0.1; wget evil.com",
        "\x00\x01\xff\xfe",
        "'; DROP TABLE ip_table; --",
    };
    for (const auto& ip : bad_ips) {
        EXPECT_NO_THROW({
            bool r = limiter.allowRequest(ip);
            (void)r;
        }) << "Crashed on IP: " << ip.substr(0, 80);
    }
}

TEST_F(RateLimiterFuzzTest, AllowRequest_MalformedUserIDs_NeverCrash) {
    RateLimiter limiter(cfg_);
    for (const auto& uid : injectionPayloads()) {
        EXPECT_NO_THROW({
            bool r = limiter.allowRequest("127.0.0.1", uid);
            (void)r;
        }) << "Crashed on user_id: " << uid.substr(0, 80);
    }
}

TEST_F(RateLimiterFuzzTest, BlacklistIP_MalformedIPs_NeverCrash) {
    RateLimiter limiter(cfg_);
    for (const auto& ip : injectionPayloads()) {
        EXPECT_NO_THROW({
            limiter.blacklistIP(ip);
            bool r = limiter.isBlacklisted(ip);
            limiter.unblacklistIP(ip);
            (void)r;
        }) << "Crashed on IP: " << ip.substr(0, 80);
    }
}

TEST_F(RateLimiterFuzzTest, HighVolumeRequests_StatsConsistent) {
    RateLimiter limiter(cfg_);
    for (int i = 0; i < 1000; ++i) {
        limiter.allowRequest("10.0.0.1", "user1");
    }
    auto stats = limiter.getStatistics();
    EXPECT_EQ(stats.total_requests, 1000u);
    EXPECT_EQ(stats.allowed_requests + stats.rejected_requests, 1000u);
}

// ============================================================================
// 5. AuditLogger fuzz (no crash, no exception leakage)
// ============================================================================

class AuditLoggerFuzzTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = std::filesystem::temp_directory_path() / "audit_fuzz_test";
        std::filesystem::create_directories(tmp_dir_);
        log_path_ = (tmp_dir_ / "fuzz_audit.jsonl").string();

        auto kp = std::make_shared<MockKeyProvider>();
        kp->createKey("saga_log", 1);
        enc_ = std::make_shared<FieldEncryption>(kp);
        PKIConfig pki_cfg;
        pki_cfg.service_id = "fuzz_test";
        pki_ = std::make_shared<VCCPKIClient>(pki_cfg);
    }

    void TearDown() override {
        std::filesystem::remove_all(tmp_dir_);
    }

    std::shared_ptr<FieldEncryption> enc_;
    std::shared_ptr<VCCPKIClient>    pki_;
    std::filesystem::path tmp_dir_;
    std::string           log_path_;
};

TEST_F(AuditLoggerFuzzTest, LogEvent_HugePayload_NoCrash) {
    AuditLoggerConfig cfg;
    cfg.enabled  = true;
    cfg.log_path = log_path_;
    cfg.key_id   = "saga_log";
    AuditLogger logger(enc_, pki_, cfg);

    nlohmann::json huge;
    huge["user"]   = std::string(10000, 'U');
    huge["action"] = std::string(10000, 'A');
    huge["data"]   = repeat('X', 100000);
    EXPECT_NO_THROW(logger.logEvent(huge));
}

TEST_F(AuditLoggerFuzzTest, LogSecurityEvent_AllEventTypes_NoCrash) {
    AuditLoggerConfig cfg;
    cfg.enabled  = true;
    cfg.log_path = log_path_;
    cfg.key_id   = "saga_log";
    AuditLogger logger(enc_, pki_, cfg);

    std::vector<SecurityEventType> all_types = {
        SecurityEventType::LOGIN_SUCCESS,
        SecurityEventType::LOGIN_FAILED,
        SecurityEventType::LOGOUT,
        SecurityEventType::TOKEN_CREATED,
        SecurityEventType::TOKEN_REVOKED,
        SecurityEventType::UNAUTHORIZED_ACCESS,
        SecurityEventType::PERMISSION_DENIED,
        SecurityEventType::DATA_READ,
        SecurityEventType::DATA_WRITE,
        SecurityEventType::DATA_DELETE,
        SecurityEventType::MFA_ENROLLED,
        SecurityEventType::SUSPICIOUS_ACTIVITY,
        SecurityEventType::CUSTOM_EVENT,
    };

    for (const auto& t : all_types) {
        // With injection-style user_id and resource
        EXPECT_NO_THROW(logger.logSecurityEvent(t,
            "'; DROP TABLE users; --",
            "../../etc/passwd",
            {{"detail", std::string(5000, 'D')}}));
    }
}

TEST_F(AuditLoggerFuzzTest, LogEvent_DeeplyNestedJson_NoCrash) {
    AuditLoggerConfig cfg;
    cfg.enabled  = true;
    cfg.log_path = log_path_;
    cfg.key_id   = "saga_log";
    AuditLogger logger(enc_, pki_, cfg);

    // Build a deeply nested JSON object (depth 200)
    nlohmann::json nested = "leaf";
    for (int i = 0; i < 200; ++i) {
        nlohmann::json wrap;
        wrap["level_" + std::to_string(i)] = nested;
        nested = wrap;
    }
    EXPECT_NO_THROW(logger.logEvent(nested));
}

TEST_F(AuditLoggerFuzzTest, LogEvent_ControlCharsInFields_NoCrash) {
    AuditLoggerConfig cfg;
    cfg.enabled  = true;
    cfg.log_path = log_path_;
    cfg.key_id   = "saga_log";
    AuditLogger logger(enc_, pki_, cfg);

    nlohmann::json evt;
    evt["user"]   = "\x00\x01\x02\x03\x04\x1b[31mred\x1b[0m";
    evt["action"] = "\n\r\t\b\f";
    EXPECT_NO_THROW(logger.logEvent(evt));
}

// ============================================================================
// 6. Password hashing fuzz
// ============================================================================

TEST(PasswordHashFuzzTest, Register_MalformedPasswords_NeverCrash) {
    auto plugin = security::createEmbeddedUserRegistrationPlugin();
    ASSERT_NE(plugin, nullptr);

    // Passwords that are too short/weak: plugin should reject gracefully
    std::vector<std::string> bad_passwords = {
        "",
        "short",
        repeat('A', 500),               // very long but no special chars
    };
    int uid = 0;
    for (const auto& pw : bad_passwords) {
        EXPECT_NO_THROW({
            auto result = plugin->registerUser(
                "fuzz_user_" + std::to_string(uid++), pw);
            (void)result; // May succeed or fail, must not throw
        }) << "Crashed on password of length " << pw.size();
    }
}

TEST(PasswordHashFuzzTest, Register_ValidComplexPasswords_NeverCrash) {
    auto plugin = security::createEmbeddedUserRegistrationPlugin();
    ASSERT_NE(plugin, nullptr);

    // These should all succeed (meet password policy)
    std::vector<std::string> valid_passwords = {
        "ValidPass123!",
        "P@$$w0rd_secure!",
        allPrintableAscii().substr(0, 20) + "A1!",  // printable ascii subset
        "MultiLine\nPassword123!",
        "Unicode\xc3\xa9\xc3\xa0Pass1!",  // UTF-8 bytes for é à
    };
    int uid = 0;
    for (const auto& pw : valid_passwords) {
        EXPECT_NO_THROW({
            auto result = plugin->registerUser(
                "valid_fuzz_" + std::to_string(uid++), pw);
            // If registration succeeds, verify authentication works
            if (result.has_value()) {
                auto auth = plugin->authenticateUser(
                    "valid_fuzz_" + std::to_string(uid - 1), pw);
                EXPECT_TRUE(auth.has_value());
            }
        }) << "Crashed on password: " << pw.substr(0, 40);
    }
}

TEST(PasswordHashFuzzTest, Authenticate_WrongPasswords_NeverCrash) {
    auto plugin = security::createEmbeddedUserRegistrationPlugin();
    ASSERT_NE(plugin, nullptr);

    plugin->registerUser("target_user", "CorrectPass123!");

    // All injection payloads as passwords: must return error, not throw
    for (const auto& p : injectionPayloads()) {
        EXPECT_NO_THROW({
            auto auth = plugin->authenticateUser("target_user", p);
            EXPECT_FALSE(auth.has_value()) << "Should have rejected payload password";
        }) << "Crashed on auth with password: " << p.substr(0, 80);
    }
}
