#include <gtest/gtest.h>

#include "server/api_auth_config.h"
#include "server/api_security_audit.h"

using namespace themis::server;

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------

static bool has_finding_containing(const ApiSecurityAuditReport& report,
                                   const std::string& substring)
{
    for (const auto& f : report.findings) {
        if (f.finding.find(substring) != std::string::npos ||
            f.recommendation.find(substring) != std::string::npos) {
            return true;
        }
    }
    return false;
}

static uint32_t count_severity(const ApiSecurityAuditReport& report,
                                AuditSeverity severity)
{
    uint32_t n = 0;
    for (const auto& f : report.findings) {
        if (f.severity == severity) {
          ++n;
        }
    }
    return n;
}

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class ApiSecurityAuditTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ---------------------------------------------------------------------------
// Secure defaults pass the audit
// ---------------------------------------------------------------------------

TEST_F(ApiSecurityAuditTest, SecureDefaultsPassAudit) {
    auto config = ApiAuthConfig::createSecureDefaults();
    auto report = ApiSecurityAuditor::audit(config);

    EXPECT_TRUE(report.passed)
        << "createSecureDefaults() should produce a configuration that passes "
           "the security audit (no HIGH or CRITICAL findings)";
    EXPECT_EQ(report.critical_count, 0u);
    EXPECT_EQ(report.high_count, 0u);
}

// ---------------------------------------------------------------------------
// Dev defaults produce a CRITICAL finding (auth disabled)
// ---------------------------------------------------------------------------

TEST_F(ApiSecurityAuditTest, DevDefaultsFailAuditDueToAuthDisabled) {
    auto config = ApiAuthConfig::createDevDefaults();
    auto report = ApiSecurityAuditor::audit(config);

    EXPECT_FALSE(report.passed);
    EXPECT_GT(report.critical_count, 0u);
    EXPECT_TRUE(has_finding_containing(report, "auth_enabled"))
        << "Disabled global auth should produce a CRITICAL finding mentioning auth_enabled";
}

// ---------------------------------------------------------------------------
// Explicitly disabled global auth → CRITICAL
// ---------------------------------------------------------------------------

TEST_F(ApiSecurityAuditTest, GlobalAuthDisabledIsCritical) {
    ApiAuthConfig config;
    config.auth_enabled = false;
    config.rate_limiting_enabled = true;

    auto report = ApiSecurityAuditor::audit(config);

    EXPECT_FALSE(report.passed);
    EXPECT_GE(report.critical_count, 1u);
}

// ---------------------------------------------------------------------------
// Global rate limiting disabled → HIGH
// ---------------------------------------------------------------------------

TEST_F(ApiSecurityAuditTest, RateLimitingDisabledIsHigh) {
    ApiAuthConfig config;
    config.auth_enabled = true;
    config.rate_limiting_enabled = false;

    auto report = ApiSecurityAuditor::audit(config);

    EXPECT_FALSE(report.passed);
    EXPECT_GE(report.high_count, 1u);
    EXPECT_TRUE(has_finding_containing(report, "rate_limiting_enabled"));
}

// ---------------------------------------------------------------------------
// Endpoint with auth_required=true but empty scope → HIGH
// ---------------------------------------------------------------------------

TEST_F(ApiSecurityAuditTest, AuthRequiredWithoutScopeIsHigh) {
    ApiAuthConfig config;
    config.auth_enabled = true;
    config.rate_limiting_enabled = true;

    EndpointAuthConfig ep;
    ep.endpoint_pattern = "/entities/*";
    ep.http_method = "GET";
    ep.auth_required = true;
    ep.required_scope = "";   // missing scope
    ep.rate_limit_per_minute = 100;
    config.endpoint_configs.push_back(ep);

    auto report = ApiSecurityAuditor::audit(config);

    EXPECT_FALSE(report.passed);
    EXPECT_GE(report.high_count, 1u);

    bool found = false;
    for (const auto& f : report.findings) {
        if (f.severity == AuditSeverity::HIGH &&
            f.endpoint_pattern == "/entities/*") {
            found = true;
            EXPECT_FALSE(f.finding.empty());
            EXPECT_FALSE(f.recommendation.empty());
        }
    }
    EXPECT_TRUE(found) << "Should have a HIGH finding for /entities/* missing scope";
}

// ---------------------------------------------------------------------------
// Sensitive endpoint without auth → HIGH
// ---------------------------------------------------------------------------

TEST_F(ApiSecurityAuditTest, SensitiveEndpointWithoutAuthIsHigh) {
    ApiAuthConfig config;
    config.auth_enabled = true;
    config.rate_limiting_enabled = true;

    // Admin endpoint exposed without auth
    EndpointAuthConfig ep;
    ep.endpoint_pattern = "/admin/*";
    ep.http_method = "*";
    ep.auth_required = false;   // intentionally wrong
    ep.required_scope = "";
    ep.rate_limit_per_minute = 100;
    config.endpoint_configs.push_back(ep);

    auto report = ApiSecurityAuditor::audit(config);

    EXPECT_FALSE(report.passed);

    bool found = false;
    for (const auto& f : report.findings) {
        if (f.severity == AuditSeverity::HIGH &&
            f.endpoint_pattern == "/admin/*") {
            found = true;
        }
    }
    EXPECT_TRUE(found) << "Sensitive /admin/* endpoint without auth should be HIGH";
}

// ---------------------------------------------------------------------------
// Other sensitive prefixes flagged
// ---------------------------------------------------------------------------

TEST_F(ApiSecurityAuditTest, AllSensitivePrefixesWithoutAuthAreHigh) {
    const std::vector<std::string> sensitive = {
        "/config",
        "/api/audit/log",
        "/pii/export",
        "/api/pki/sign",
        "/auth/sessions"
    };

    for (const auto& pattern : sensitive) {
        ApiAuthConfig config;
        config.auth_enabled = true;
        config.rate_limiting_enabled = true;

        EndpointAuthConfig ep;
        ep.endpoint_pattern = pattern;
        ep.auth_required = false;
        ep.rate_limit_per_minute = 100;
        config.endpoint_configs.push_back(ep);

        auto report = ApiSecurityAuditor::audit(config);
        EXPECT_FALSE(report.passed)
            << "Pattern '" << pattern << "' without auth should fail audit";
        EXPECT_GE(report.high_count, 1u)
            << "Pattern '" << pattern << "' without auth should produce a HIGH finding";
    }
}

// ---------------------------------------------------------------------------
// Missing per-endpoint rate limit on auth endpoint → MEDIUM
// ---------------------------------------------------------------------------

TEST_F(ApiSecurityAuditTest, MissingRateLimitOnAuthEndpointIsMedium) {
    ApiAuthConfig config;
    config.auth_enabled = true;
    config.rate_limiting_enabled = true;

    EndpointAuthConfig ep;
    ep.endpoint_pattern = "/query";
    ep.http_method = "*";
    ep.auth_required = true;
    ep.required_scope = "data:read";
    ep.rate_limit_per_minute = 0;   // no per-endpoint limit
    config.endpoint_configs.push_back(ep);

    auto report = ApiSecurityAuditor::audit(config);

    // Should pass overall (no HIGH/CRITICAL) but have a MEDIUM finding
    EXPECT_TRUE(report.passed)
        << "Missing per-endpoint rate limit is MEDIUM, not HIGH or CRITICAL";
    EXPECT_GE(report.medium_count, 1u);

    bool found = false;
    for (const auto& f : report.findings) {
        if (f.severity == AuditSeverity::MEDIUM &&
            f.endpoint_pattern == "/query") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// ---------------------------------------------------------------------------
// Excessive burst capacity → LOW
// ---------------------------------------------------------------------------

TEST_F(ApiSecurityAuditTest, ExcessiveBurstCapacityIsLow) {
    ApiAuthConfig config;
    config.auth_enabled = true;
    config.rate_limiting_enabled = true;

    EndpointAuthConfig ep;
    ep.endpoint_pattern = "/vector/*";
    ep.http_method = "*";
    ep.auth_required = true;
    ep.required_scope = "data:read";
    ep.rate_limit_per_minute = 100;
    ep.rate_limit_burst = 80;   // > 100/2 = 50 → LOW
    config.endpoint_configs.push_back(ep);

    auto report = ApiSecurityAuditor::audit(config);

    EXPECT_TRUE(report.passed);   // LOW only → still passes
    EXPECT_GE(report.low_count, 1u);

    bool found = false;
    for (const auto& f : report.findings) {
        if (f.severity == AuditSeverity::LOW &&
            f.endpoint_pattern == "/vector/*") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// ---------------------------------------------------------------------------
// Burst within limit does not trigger LOW finding
// ---------------------------------------------------------------------------

TEST_F(ApiSecurityAuditTest, ReasonableBurstCapacityIsClean) {
    ApiAuthConfig config;
    config.auth_enabled = true;
    config.rate_limiting_enabled = true;

    EndpointAuthConfig ep;
    ep.endpoint_pattern = "/vector/*";
    ep.http_method = "*";
    ep.auth_required = true;
    ep.required_scope = "data:read";
    ep.rate_limit_per_minute = 100;
    ep.rate_limit_burst = 50;   // exactly 100/2 → acceptable
    config.endpoint_configs.push_back(ep);

    auto report = ApiSecurityAuditor::audit(config);

    EXPECT_TRUE(report.passed);
    EXPECT_EQ(report.low_count, 0u);
}

// ---------------------------------------------------------------------------
// Report counters match findings vector
// ---------------------------------------------------------------------------

TEST_F(ApiSecurityAuditTest, ReportCountersMatchFindings) {
    // Inject a mix: 1 CRITICAL (auth off) + 1 HIGH (sensitive endpoint no auth)
    ApiAuthConfig config;
    config.auth_enabled = false;
    config.rate_limiting_enabled = true;

    EndpointAuthConfig ep;
    ep.endpoint_pattern = "/admin/*";
    ep.http_method = "*";
    ep.auth_required = false;
    ep.required_scope = "";
    ep.rate_limit_per_minute = 100;
    config.endpoint_configs.push_back(ep);

    auto report = ApiSecurityAuditor::audit(config);

    uint32_t manual_critical = count_severity(report, AuditSeverity::CRITICAL);
    uint32_t manual_high     = count_severity(report, AuditSeverity::HIGH);
    uint32_t manual_medium   = count_severity(report, AuditSeverity::MEDIUM);
    uint32_t manual_low      = count_severity(report, AuditSeverity::LOW);

    EXPECT_EQ(report.critical_count, manual_critical);
    EXPECT_EQ(report.high_count,     manual_high);
    EXPECT_EQ(report.medium_count,   manual_medium);
    EXPECT_EQ(report.low_count,      manual_low);
}

// ---------------------------------------------------------------------------
// Empty config (no endpoints) passes with secure global settings
// ---------------------------------------------------------------------------

TEST_F(ApiSecurityAuditTest, EmptyEndpointListWithSecureGlobalsPassesAudit) {
    ApiAuthConfig config;
    config.auth_enabled = true;
    config.rate_limiting_enabled = true;
    config.global_rate_limit_per_minute = 100;
    config.global_rate_limit_burst = 50;
    // No endpoint-specific configs

    auto report = ApiSecurityAuditor::audit(config);

    EXPECT_TRUE(report.passed);
    EXPECT_EQ(report.critical_count, 0u);
    EXPECT_EQ(report.high_count, 0u);
}

// ---------------------------------------------------------------------------
// Public endpoint (no auth) is accepted when non-sensitive
// ---------------------------------------------------------------------------

TEST_F(ApiSecurityAuditTest, NonSensitivePublicEndpointDoesNotTriggerHighFinding) {
    ApiAuthConfig config;
    config.auth_enabled = true;
    config.rate_limiting_enabled = true;

    // /health is public and non-sensitive — expected to be fine
    EndpointAuthConfig ep;
    ep.endpoint_pattern = "/health";
    ep.http_method = "*";
    ep.auth_required = false;
    ep.required_scope = "";
    ep.rate_limit_per_minute = 1000;
    config.endpoint_configs.push_back(ep);

    auto report = ApiSecurityAuditor::audit(config);

    EXPECT_TRUE(report.passed);
    // Verify no HIGH finding targets /health
    for (const auto& f : report.findings) {
        if (f.endpoint_pattern == "/health") {
            EXPECT_NE(f.severity, AuditSeverity::HIGH)
                << "/health should not produce a HIGH finding";
            EXPECT_NE(f.severity, AuditSeverity::CRITICAL)
                << "/health should not produce a CRITICAL finding";
        }
    }
}

// ---------------------------------------------------------------------------
// Every finding has non-empty text and recommendation
// ---------------------------------------------------------------------------

TEST_F(ApiSecurityAuditTest, AllFindingsHaveNonEmptyMessages) {
    auto config = ApiAuthConfig::createDevDefaults();
    auto report = ApiSecurityAuditor::audit(config);

    for (const auto& f : report.findings) {
        EXPECT_FALSE(f.finding.empty())
            << "Every finding must have a non-empty finding message";
        EXPECT_FALSE(f.recommendation.empty())
            << "Every finding must have a non-empty recommendation";
    }
}
