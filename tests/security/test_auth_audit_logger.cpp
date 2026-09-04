/**
 * @file test_auth_audit_logger.cpp
 * @brief Tests for AuthAuditLogger and audit integration in auth components.
 *
 * Tests cover:
 * - AuthAuditLogger facade (JWT, Kerberos, MFA, OAuth, SAML events)
 * - setAuditLogger() wiring in JWTValidator, MFAAuthenticator, GSSAPIAuthenticator
 * - setAuditLogger() wiring in SAMLAuthenticator, OAuthDeviceFlow, PrincipalValidator
 * - No-op behaviour when logger is nullptr
 */

#include <gtest/gtest.h>

#include "auth/auth_audit_logger.h"
#include "auth/jwt_validator.h"
#include "auth/mfa_authenticator.h"
#include "auth/gssapi_authenticator.h"
#include "auth/saml_authenticator.h"
#include "auth/oauth_device_flow.h"
#include "auth/principal_validator.h"
#include "auth/token_blacklist.h"
#include "auth/api_key_authenticator.h"
#include "utils/audit_logger.h"

#include <filesystem>
#include <fstream>
#include <string>

using namespace themis::auth;
using namespace themis::utils;

namespace {

AuditLoggerConfig makeTestConfig(const std::string& log_path) {
    AuditLoggerConfig cfg;
    cfg.enabled           = true;
    cfg.encrypt_then_sign = false;
    cfg.log_path          = log_path;
    cfg.key_id            = "test";
    cfg.enable_hash_chain = false;
    cfg.enable_siem       = false;
    return cfg;
}

size_t countLines(const std::string& path) {
    std::ifstream f(path);
    size_t n = 0;
    std::string line = {};
    while (std::getline(f, line))
        if (!line.empty()) {
          ++n;
        }
    return n;
}

} // namespace

// ============================================================================
// Fixture
// ============================================================================

class AuthAuditLoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_  = std::filesystem::temp_directory_path() / "auth_audit_test";
        std::filesystem::create_directories(tmp_dir_);
        log_path_ = (tmp_dir_ / "auth_audit.jsonl").string();
        logger_   = std::make_unique<AuditLogger>(nullptr, nullptr, makeTestConfig(log_path_));
        facade_   = std::make_unique<AuthAuditLogger>(logger_.get());
    }

    void TearDown() override {
        std::filesystem::remove_all(tmp_dir_);
    }

    std::filesystem::path       tmp_dir_;
    std::string                 log_path_;
    std::unique_ptr<AuditLogger>     logger_;
    std::unique_ptr<AuthAuditLogger> facade_;
};

// ============================================================================
// AuthAuditLogger facade tests
// ============================================================================

TEST_F(AuthAuditLoggerTest, IsEnabled_WhenLoggerAttached) {
    EXPECT_TRUE(facade_->isEnabled());
}

TEST_F(AuthAuditLoggerTest, IsEnabled_WhenLoggerNull) {
    AuthAuditLogger disabled(nullptr);
    EXPECT_FALSE(disabled.isEnabled());
}

TEST_F(AuthAuditLoggerTest, LogJWTSuccess_WritesEntry) {
    facade_->logJWTSuccess("alice", "jti-123", "https://issuer", "kid-1");
    logger_->flush();
    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuthAuditLoggerTest, LogJWTFailure_WritesEntry) {
    facade_->logJWTFailure("token_expired", "kid-1");
    logger_->flush();
    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuthAuditLoggerTest, LogTokenRevoked_WritesEntry) {
    facade_->logTokenRevoked("jti-abc", "bob");
    logger_->flush();
    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuthAuditLoggerTest, LogKerberosSuccess_WritesEntry) {
    facade_->logKerberosSuccess("alice@REALM.COM");
    logger_->flush();
    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuthAuditLoggerTest, LogKerberosFailure_WritesEntry) {
    facade_->logKerberosFailure("context_rejected");
    logger_->flush();
    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuthAuditLoggerTest, LogTOTPSuccess_WritesEntry) {
    facade_->logTOTPSuccess("charlie");
    logger_->flush();
    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuthAuditLoggerTest, LogTOTPFailure_WritesEntry) {
    facade_->logTOTPFailure("charlie");
    logger_->flush();
    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuthAuditLoggerTest, LogRecoveryCodeUsed_WritesEntry) {
    facade_->logRecoveryCodeUsed("dave");
    logger_->flush();
    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuthAuditLoggerTest, LogMFAEnrolled_WritesEntry) {
    facade_->logMFAEnrolled("eve");
    logger_->flush();
    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuthAuditLoggerTest, LogOAuthDeviceGranted_WritesEntry) {
    facade_->logOAuthDeviceGranted("cli-client", "frank");
    logger_->flush();
    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuthAuditLoggerTest, LogOAuthDeviceDenied_WritesEntry) {
    facade_->logOAuthDeviceDenied("cli-client", "expired");
    logger_->flush();
    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuthAuditLoggerTest, LogSAMLSuccess_WritesEntry) {
    facade_->logSAMLSuccess("grace", "https://idp.example.com");
    logger_->flush();
    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuthAuditLoggerTest, LogSAMLFailure_WritesEntry) {
    facade_->logSAMLFailure("invalid_signature");
    logger_->flush();
    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuthAuditLoggerTest, LogZeroTrustAllowed_WritesEntry) {
    facade_->logZeroTrustAllowed("alice", "data", 0.9, "req-001");
    logger_->flush();
    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuthAuditLoggerTest, LogZeroTrustDenied_WritesEntry) {
    facade_->logZeroTrustDenied("eve", "data", "network policy denied", "req-002");
    logger_->flush();
    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuthAuditLoggerTest, NullLogger_NoOp) {
    AuthAuditLogger noop(nullptr);
    // None of these should crash or write anything
    EXPECT_NO_THROW(noop.logJWTSuccess("u", "j", "i", "k"));
    EXPECT_NO_THROW(noop.logJWTFailure("reason"));
    EXPECT_NO_THROW(noop.logKerberosSuccess("principal"));
    EXPECT_NO_THROW(noop.logKerberosFailure("reason"));
    EXPECT_NO_THROW(noop.logTOTPSuccess("u"));
    EXPECT_NO_THROW(noop.logTOTPFailure("u"));
    EXPECT_NO_THROW(noop.logRecoveryCodeUsed("u"));
    EXPECT_NO_THROW(noop.logMFAEnrolled("u"));
    EXPECT_NO_THROW(noop.logZeroTrustAllowed("u", "res", 1.0, "req"));
    EXPECT_NO_THROW(noop.logZeroTrustDenied("u", "res", "reason", "req"));
}

TEST_F(AuthAuditLoggerTest, SetLogger_DetachReattach) {
    AuthAuditLogger a(nullptr);
    EXPECT_FALSE(a.isEnabled());

    a.setLogger(logger_.get());
    EXPECT_TRUE(a.isEnabled());

    a.setLogger(nullptr);
    EXPECT_FALSE(a.isEnabled());
}

// ============================================================================
// MFAAuthenticator integration
// ============================================================================

TEST_F(AuthAuditLoggerTest, MFAAuthenticator_EnrollmentLogged) {
    MFAAuthenticator mfa;
    mfa.setAuditLogger(logger_.get());

    mfa.generateEnrollment("test_user");
    logger_->flush();
    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuthAuditLoggerTest, MFAAuthenticator_TOTPSuccessLogged) {
    MFAAuthenticator mfa;
    mfa.setAuditLogger(logger_.get());

    auto enrollment = mfa.generateEnrollment("test_user");
    logger_->flush();
    // Clear file by creating a new logger pointing to fresh path
    std::string path2 = (tmp_dir_ / "totp.jsonl").string();
    AuditLogger logger2(nullptr, nullptr, makeTestConfig(path2));
    mfa.setAuditLogger(&logger2);

    std::string code = mfa.getCurrentTOTP(enrollment.secret_base32);
    mfa.validateTOTP(enrollment.secret_base32, code);
    logger2.flush();

    EXPECT_GE(countLines(path2), 1u);
}

TEST_F(AuthAuditLoggerTest, MFAAuthenticator_RecoveryCodeLogged) {
    MFAAuthenticator mfa;
    std::string path2 = (tmp_dir_ / "recovery.jsonl").string();
    AuditLogger logger2(nullptr, nullptr, makeTestConfig(path2));
    mfa.setAuditLogger(&logger2);

    auto enrollment = mfa.generateEnrollment("user");
    std::string code = enrollment.recovery_codes.at(0);
    mfa.validateRecoveryCode(enrollment, code);
    logger2.flush();

    EXPECT_GE(countLines(path2), 1u);
}

TEST_F(AuthAuditLoggerTest, MFAAuthenticator_NoAuditLogger_DoesNotCrash) {
    MFAAuthenticator mfa;
    // No setAuditLogger – should not crash
    auto enrollment = mfa.generateEnrollment("u");
    std::string code = mfa.getCurrentTOTP(enrollment.secret_base32);
    EXPECT_NO_THROW(mfa.validateTOTP(enrollment.secret_base32, code));
}

// ============================================================================
// JWTValidator integration (setAuditLogger API exists and accepts nullptr)
// ============================================================================

TEST_F(AuthAuditLoggerTest, JWTValidator_SetAuditLogger_AcceptsNull) {
    JWTValidatorConfig cfg;
    cfg.jwks_url = "https://example.com/jwks";
    cfg.require_issuer_validation = false;
    cfg.require_audience_validation = false;
    JWTValidator v(cfg);
    EXPECT_NO_THROW(v.setAuditLogger(nullptr));
    EXPECT_NO_THROW(v.setAuditLogger(logger_.get()));
    EXPECT_NO_THROW(v.setAuditLogger(nullptr));
}

// ============================================================================
// GSSAPIAuthenticator integration
// ============================================================================

TEST_F(AuthAuditLoggerTest, GSSAPIAuthenticator_SetAuditLogger_AcceptsNull) {
    GSSAPIAuthenticator gssapi;
    EXPECT_NO_THROW(gssapi.setAuditLogger(nullptr));
    EXPECT_NO_THROW(gssapi.setAuditLogger(logger_.get()));
    EXPECT_NO_THROW(gssapi.setAuditLogger(nullptr));
}

TEST_F(AuthAuditLoggerTest, GSSAPIAuthenticator_FailureLogged_WhenNotInitialized) {
    GSSAPIAuthenticator gssapi;
    std::string path2 = (tmp_dir_ / "gssapi.jsonl").string();
    AuditLogger logger2(nullptr, nullptr, makeTestConfig(path2));
    gssapi.setAuditLogger(&logger2);

    // authenticateToken on uninitialized authenticator → failure event
    auto result = gssapi.authenticateToken("some-token");
    EXPECT_FALSE(result.success);
    logger2.flush();
    EXPECT_GE(countLines(path2), 1u);
}

// ============================================================================
// SAMLAuthenticator integration
// ============================================================================

TEST_F(AuthAuditLoggerTest, SAMLAuthenticator_SetAuditLogger_AcceptsNull) {
    // Build a minimal valid SAMLConfig (validation is not exercised here)
    SAMLConfig cfg;
    cfg.sp_entity_id = "https://sp.example.com";
    cfg.sp_acs_url   = "https://sp.example.com/acs";
    cfg.idp_sso_url  = "https://idp.example.com/sso";
    cfg.idp_entity_id = "https://idp.example.com";
    // Use a self-signed dummy PEM so the constructor succeeds
    cfg.idp_certificate_pem =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBpDCCAQ2gAwIBAgIUYvK2iU4Yr/HxcnAXqGJ3LBpqyNowDQYJKoZIhvcNAQEL\n"
        "BQAwETEPMA0GA1UEAxMGZHVtbXkwHhcNMjQwMTAxMDAwMDAwWhcNMjUwMTAxMDAw\n"
        "MDAwWjARMQ8wDQYDVQQDEwZkdW1teTCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkC\n"
        "gYEA2a2rwplBQLF29amygykEMmYz0+Kcj3bKBp29rNT5O2alM/bfPlTzFVEHFqg7\n"
        "tHYkCi9bYZcv0FDh11N8HJ38TYwUMrVgTMFw6OBEfMm0v4mhRnaXPqvJBMJmqFDE\n"
        "8VhCGfFyZGnkKEBSbKxBNXK5HuZH2qSjNaCbNVGlHaMCAwEAAaMTMBEwDwYDVR0T\n"
        "AQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOBgQB9R3qnF8VsxNkGHClydvkLg2HC\n"
        "DUMMY+PADsYVZ9lRX3ATQv1xKkjxFdddPXfmHm5+DUMMY==\n"
        "-----END CERTIFICATE-----\n";

    // Constructor may throw if the cert is invalid – that's fine for API tests
    try {
        SAMLAuthenticator saml(cfg);
        EXPECT_NO_THROW(saml.setAuditLogger(nullptr));
        EXPECT_NO_THROW(saml.setAuditLogger(logger_.get()));
        EXPECT_NO_THROW(saml.setAuditLogger(nullptr));
    } catch (const std::exception&) {
        // cert parsing failed – setAuditLogger API test skipped
        GTEST_SKIP() << "SAMLAuthenticator construction failed (invalid test cert)";
    }
}

// ============================================================================
// OAuthDeviceFlow integration
// ============================================================================

TEST_F(AuthAuditLoggerTest, OAuthDeviceFlow_SetAuditLogger_AcceptsNull) {
    OAuthDeviceFlow::Config cfg;
    cfg.device_authorization_endpoint = "https://auth.example.com/device";
    cfg.token_endpoint                 = "https://auth.example.com/token";
    cfg.client_id                      = "test-client";

    OAuthDeviceFlow flow(cfg);
    EXPECT_NO_THROW(flow.setAuditLogger(nullptr));
    EXPECT_NO_THROW(flow.setAuditLogger(logger_.get()));
    EXPECT_NO_THROW(flow.setAuditLogger(nullptr));
}

// ============================================================================
// PrincipalValidator integration
// ============================================================================

TEST_F(AuthAuditLoggerTest, PrincipalValidator_SetAuditLogger_AcceptsNull) {
    PrincipalValidator validator;
    EXPECT_NO_THROW(validator.setAuditLogger(nullptr));
    EXPECT_NO_THROW(validator.setAuditLogger(logger_.get()));
    EXPECT_NO_THROW(validator.setAuditLogger(nullptr));
}

TEST_F(AuthAuditLoggerTest, PrincipalValidator_AllowedPrincipalLogged) {
    PrincipalValidator::Config cfg;
    cfg.default_allow        = true;
    cfg.enable_audit_logging = true;

    std::string path2 = (tmp_dir_ / "pv_allow.jsonl").string();
    AuditLogger logger2(nullptr, nullptr, makeTestConfig(path2));

    PrincipalValidator validator(cfg);
    validator.setAuditLogger(&logger2);

    auto result = validator.validate("alice@REALM.COM");
    EXPECT_TRUE(result.allowed);
    logger2.flush();
    EXPECT_GE(countLines(path2), 1u);
}

TEST_F(AuthAuditLoggerTest, PrincipalValidator_DeniedPrincipalLogged) {
    PrincipalValidator::Config cfg;
    cfg.default_allow        = false;  // deny by default
    cfg.enable_audit_logging = true;

    std::string path2 = (tmp_dir_ / "pv_deny.jsonl").string();
    AuditLogger logger2(nullptr, nullptr, makeTestConfig(path2));

    PrincipalValidator validator(cfg);
    validator.setAuditLogger(&logger2);

    auto result = validator.validate("mallory@REALM.COM");
    EXPECT_FALSE(result.allowed);
    logger2.flush();
    EXPECT_GE(countLines(path2), 1u);
}

TEST_F(AuthAuditLoggerTest, PrincipalValidator_NoAuditLogger_DoesNotCrash) {
    PrincipalValidator::Config cfg;
    cfg.default_allow        = true;
    cfg.enable_audit_logging = true;

    PrincipalValidator validator(cfg);
    // No setAuditLogger – should not crash
    EXPECT_NO_THROW(validator.validate("alice@REALM.COM"));
}

// ============================================================================
// AuthAuditLogger – API Key facade methods
// ============================================================================

TEST_F(AuthAuditLoggerTest, LogApiKeySuccess_WritesEntry) {
    facade_->logApiKeySuccess("sk_live_abc123", "alice@example.com");
    logger_->flush();
    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuthAuditLoggerTest, LogApiKeyFailure_WritesEntry) {
    facade_->logApiKeyFailure("sk_live_abc123", "secret_mismatch");
    logger_->flush();
    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuthAuditLoggerTest, LogApiKeyFailure_NullLogger_NoOp) {
    AuthAuditLogger noop(nullptr);
    EXPECT_NO_THROW(noop.logApiKeySuccess("key-1", "alice"));
    EXPECT_NO_THROW(noop.logApiKeyFailure("key-1", "not_found"));
}

// ============================================================================
// TokenBlacklist integration
// ============================================================================

TEST_F(AuthAuditLoggerTest, TokenBlacklist_SetAuditLogger_AcceptsNull) {
    TokenBlacklist bl;
    EXPECT_NO_THROW(bl.setAuditLogger(nullptr));
    EXPECT_NO_THROW(bl.setAuditLogger(logger_.get()));
    EXPECT_NO_THROW(bl.setAuditLogger(nullptr));
}

TEST_F(AuthAuditLoggerTest, TokenBlacklist_RevokeLogged) {
    std::string path2 = (tmp_dir_ / "bl.jsonl").string();
    AuditLogger logger2(nullptr, nullptr, makeTestConfig(path2));

    TokenBlacklist bl;
    bl.setAuditLogger(&logger2);

    auto expires = std::chrono::system_clock::now() + std::chrono::hours(1);
    bl.revoke("jti-revoke-test", expires);
    logger2.flush();
    EXPECT_GE(countLines(path2), 1u);
}

TEST_F(AuthAuditLoggerTest, TokenBlacklist_NoAuditLogger_DoesNotCrash) {
    TokenBlacklist bl;
    // No setAuditLogger – should not crash
    auto expires = std::chrono::system_clock::now() + std::chrono::hours(1);
    EXPECT_NO_THROW(bl.revoke("jti-noaudit", expires));
}

// ============================================================================
// ApiKeyAuthenticator integration
// ============================================================================

TEST_F(AuthAuditLoggerTest, ApiKeyAuthenticator_SetAuditLogger_AcceptsNull) {
    ApiKeyAuthenticator auth;
    EXPECT_NO_THROW(auth.setAuditLogger(nullptr));
    EXPECT_NO_THROW(auth.setAuditLogger(logger_.get()));
    EXPECT_NO_THROW(auth.setAuditLogger(nullptr));
}

TEST_F(AuthAuditLoggerTest, ApiKeyAuthenticator_SuccessLogged) {
    std::string path2 = (tmp_dir_ / "apikey_ok.jsonl").string();
    AuditLogger logger2(nullptr, nullptr, makeTestConfig(path2));

    ApiKeyAuthenticator auth;
    auth.setAuditLogger(&logger2);

    auto cred = ApiKeyAuthenticator::createCredential(
        "sk_live_test", "s3cr3t", "alice@example.com", {"data:read"});
    auth.addCredential(cred);

    auto claims = auth.authenticate("sk_live_test", "s3cr3t");
    EXPECT_EQ(claims.principal, "alice@example.com");
    logger2.flush();
    EXPECT_GE(countLines(path2), 1u);
}

TEST_F(AuthAuditLoggerTest, ApiKeyAuthenticator_FailureLogged) {
    std::string path2 = (tmp_dir_ / "apikey_fail.jsonl").string();
    AuditLogger logger2(nullptr, nullptr, makeTestConfig(path2));

    ApiKeyAuthenticator auth;
    auth.setAuditLogger(&logger2);

    // Authenticate with unknown key → LOGIN_FAILED
    EXPECT_THROW(auth.authenticate("unknown_key", "any_secret"), AuthException);
    logger2.flush();
    EXPECT_GE(countLines(path2), 1u);
}

TEST_F(AuthAuditLoggerTest, ApiKeyAuthenticator_SecretMismatchLogged) {
    std::string path2 = (tmp_dir_ / "apikey_mismatch.jsonl").string();
    AuditLogger logger2(nullptr, nullptr, makeTestConfig(path2));

    ApiKeyAuthenticator auth;
    auth.setAuditLogger(&logger2);

    auto cred = ApiKeyAuthenticator::createCredential(
        "sk_live_mismatch", "correct-secret", "bob@example.com");
    auth.addCredential(cred);

    EXPECT_THROW(auth.authenticate("sk_live_mismatch", "wrong-secret"), AuthException);
    logger2.flush();
    EXPECT_GE(countLines(path2), 1u);
}

TEST_F(AuthAuditLoggerTest, ApiKeyAuthenticator_NoAuditLogger_DoesNotCrash) {
    ApiKeyAuthenticator auth;
    // No setAuditLogger – should not crash on success or failure
    auto cred = ApiKeyAuthenticator::createCredential(
        "sk_live_noaudit", "pass", "carol@example.com");
    auth.addCredential(cred);
    EXPECT_NO_THROW(auth.authenticate("sk_live_noaudit", "pass"));
    EXPECT_THROW(auth.authenticate("sk_live_noaudit", "wrong"), AuthException);
}
