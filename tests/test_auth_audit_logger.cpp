/**
 * @file test_auth_audit_logger.cpp
 * @brief Tests for AuthAuditLogger and audit integration in auth components.
 *
 * Tests cover:
 * - AuthAuditLogger facade (JWT, Kerberos, MFA, OAuth, SAML events)
 * - setAuditLogger() wiring in JWTValidator, MFAAuthenticator, GSSAPIAuthenticator
 * - No-op behaviour when logger is nullptr
 */

#include <gtest/gtest.h>

#include "auth/auth_audit_logger.h"
#include "auth/jwt_validator.h"
#include "auth/mfa_authenticator.h"
#include "auth/gssapi_authenticator.h"
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
    std::string line;
    while (std::getline(f, line))
        if (!line.empty()) ++n;
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
