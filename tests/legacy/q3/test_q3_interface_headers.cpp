/*
 * test_q3_interface_headers.cpp
 *
 * Compilation and basic structural tests for all Q3 2026 interface headers.
 * These tests verify that headers compile cleanly, default values are correct,
 * enum values exist, and struct fields are accessible.  No runtime polymorphism
 * is exercised here — that belongs to implementation-level unit tests.
 *
 * Tests (20 total):
 *   QH-01  SubscriptionFilter construction and defaults
 *   QH-02  SubscriptionEvent field assignment
 *   QH-03  GatewayHookResult default values
 *   QH-04  GatewayHookPhase enum values accessible
 *   QH-05  AuthEventType enum values accessible
 *   QH-06  PasskeyCredential fields and defaults
 *   QH-07  PasskeyVerifyResult enum values
 *   QH-08  SubjectAttributes ABAC fields
 *   QH-09  PolicyDecision enum values
 *   QH-10  AuthEvent construction
 *   QH-11  RedisTLSConfig isValid() — valid with CA cert
 *   QH-12  RedisTLSConfig isValid() — invalid without CA cert
 *   QH-13  L3EncryptionConfig isEncryptionEnabled()
 *   QH-14  L3EncryptionMode::DISABLED check
 *   QH-15  ContentCategory confidence range
 *   QH-16  ContentClassificationRequest defaults
 *   QH-17  PIIMatch offset fields
 *   QH-18  PIIRedactionConfig defaults
 *   QH-19  HealthStatus enum values and AggregateHealthReport helpers
 *   QH-20  QuantizationScheme enum (from acceleration/quantized_backend.h)
 */

#include <gtest/gtest.h>

// Q3 2026 headers under test
#include "api/subscription_multiplexer.h"
#include "api/api_gateway_hook.h"
#include "auth/passkey_authenticator.h"
#include "auth/authorization_policy.h"
#include "auth/auth_event_bus.h"
#include "cache/redis_tls_config.h"
#include "cache/l3_encryption_config.h"
#include "content/content_classifier.h"
#include "content/pii_redactor.h"
#include "core/health_probe.h"
#include "core/config_hot_reloader.h"

// Previous acceleration header (QuantizationScheme)
#include "acceleration/quantized_backend.h"

// ============================================================================
// QH-01: SubscriptionFilter construction and defaults
// ============================================================================

TEST(Q3InterfaceHeaders, QH01_SubscriptionFilterDefaults) {
    themis::api::SubscriptionFilter f;
    EXPECT_TRUE(f.topic.empty());
    EXPECT_TRUE(f.filter_expr.empty());
    EXPECT_EQ(f.last_event_id, -1);
}

TEST(Q3InterfaceHeaders, QH01_SubscriptionFilterAssignment) {
    themis::api::SubscriptionFilter f;
    f.topic = "orders";
    f.filter_expr = "doc.status == 'pending'";
    f.last_event_id = 42;

    EXPECT_EQ(f.topic, "orders");
    EXPECT_EQ(f.filter_expr, "doc.status == 'pending'");
    EXPECT_EQ(f.last_event_id, 42);
}

// ============================================================================
// QH-02: SubscriptionEvent field assignment
// ============================================================================

TEST(Q3InterfaceHeaders, QH02_SubscriptionEventFields) {
    themis::api::SubscriptionEvent ev;
    ev.event_id     = 100;
    ev.topic        = "inventory";
    ev.payload_json = R"({"sku":"ABC","qty":5})";
    ev.timestamp    = std::chrono::system_clock::now();

    EXPECT_EQ(ev.event_id, 100);
    EXPECT_EQ(ev.topic, "inventory");
    EXPECT_FALSE(ev.payload_json.empty());
}

// ============================================================================
// QH-03: GatewayHookResult default values
// ============================================================================

TEST(Q3InterfaceHeaders, QH03_GatewayHookResultDefaults) {
    themis::api::GatewayHookResult result;
    EXPECT_TRUE(result.proceed);
    EXPECT_EQ(result.override_status_code, 0);
    EXPECT_TRUE(result.override_body.empty());
    EXPECT_TRUE(result.add_headers.empty());
}

// ============================================================================
// QH-04: GatewayHookPhase enum values accessible
// ============================================================================

TEST(Q3InterfaceHeaders, QH04_GatewayHookPhaseEnumValues) {
    using P = themis::api::GatewayHookPhase;
    EXPECT_NE(P::PRE_AUTH,     P::POST_AUTH);
    EXPECT_NE(P::PRE_HANDLER,  P::POST_HANDLER);
    EXPECT_NE(P::POST_HANDLER, P::ON_ERROR);
    // Verify all five distinct values compile
    [[maybe_unused]] auto phases = {
        P::PRE_AUTH, P::POST_AUTH, P::PRE_HANDLER, P::POST_HANDLER, P::ON_ERROR
    };
}

// ============================================================================
// QH-05: AuthEventType enum values accessible
// ============================================================================

TEST(Q3InterfaceHeaders, QH05_AuthEventTypeEnumValues) {
    using T = themis::auth::AuthEventType;
    EXPECT_NE(T::LOGIN_SUCCESS,        T::LOGIN_FAILED);
    EXPECT_NE(T::PASSKEY_AUTH_SUCCESS, T::PASSKEY_AUTH_FAILED);
    EXPECT_NE(T::TOKEN_ISSUED,         T::TOKEN_REVOKED);
    EXPECT_NE(T::ANOMALY_DETECTED,     T::POLICY_UPDATED);
}

// ============================================================================
// QH-06: PasskeyCredential fields and defaults
// ============================================================================

TEST(Q3InterfaceHeaders, QH06_PasskeyCredentialDefaults) {
    themis::auth::PasskeyCredential cred;
    EXPECT_EQ(cred.sign_count, 0u);
    EXPECT_TRUE(cred.resident_key);
    EXPECT_TRUE(cred.user_verification_required);
}

TEST(Q3InterfaceHeaders, QH06_PasskeyCredentialFields) {
    themis::auth::PasskeyCredential cred;
    cred.credential_id   = "Y3JlZA";
    cred.user_id         = "user-42";
    cred.public_key_cbor = "\xa5\x01\x02";
    cred.sign_count      = 7;
    cred.aaguid          = "00000000-0000-0000-0000-000000000000";

    EXPECT_EQ(cred.sign_count, 7u);
    EXPECT_EQ(cred.user_id, "user-42");
    EXPECT_EQ(cred.aaguid, "00000000-0000-0000-0000-000000000000");
}

// ============================================================================
// QH-07: PasskeyVerifyResult enum values
// ============================================================================

TEST(Q3InterfaceHeaders, QH07_PasskeyVerifyResultEnumValues) {
    using R = themis::auth::PasskeyVerifyResult;
    EXPECT_NE(R::SUCCESS,           R::INVALID_SIGNATURE);
    EXPECT_NE(R::INVALID_CHALLENGE, R::CREDENTIAL_NOT_FOUND);
    EXPECT_NE(R::REPLAY_ATTACK,     R::USER_VERIFICATION_FAILED);
}

// ============================================================================
// QH-08: SubjectAttributes ABAC fields
// ============================================================================

TEST(Q3InterfaceHeaders, QH08_SubjectAttributesFields) {
    themis::auth::SubjectAttributes subj;
    subj.subject_id      = "svc-reporting";
    subj.role            = "analyst";
    subj.groups          = {"finance", "eu-region"};
    subj.tenant_id       = "tenant-acme";
    subj.clearance_level = "confidential";
    subj.attributes["department"] = "finance";

    EXPECT_EQ(subj.role, "analyst");
    EXPECT_EQ(subj.groups.size(), 2u);
    EXPECT_EQ(subj.clearance_level, "confidential");
    EXPECT_EQ(subj.attributes.at("department"), "finance");
}

// ============================================================================
// QH-09: PolicyDecision enum values
// ============================================================================

TEST(Q3InterfaceHeaders, QH09_PolicyDecisionEnumValues) {
    using D = themis::auth::PolicyDecision;
    EXPECT_NE(D::ALLOW, D::DENY);
    EXPECT_NE(D::DENY,  D::NOT_APPLICABLE);
    EXPECT_NE(D::ALLOW, D::NOT_APPLICABLE);
}

// ============================================================================
// QH-10: AuthEvent construction
// ============================================================================

TEST(Q3InterfaceHeaders, QH10_AuthEventConstruction) {
    themis::auth::AuthEvent ev;
    ev.event_id      = "evt-001";
    ev.type          = themis::auth::AuthEventType::MFA_SUCCESS;
    ev.user_id       = "alice";
    ev.session_id    = "sess-abc";
    ev.client_ip     = "10.0.0.1";
    ev.correlation_id = "corr-xyz";
    ev.metadata["mfa_method"] = "totp";

    EXPECT_EQ(ev.event_id, "evt-001");
    EXPECT_EQ(ev.type, themis::auth::AuthEventType::MFA_SUCCESS);
    EXPECT_EQ(ev.metadata.at("mfa_method"), "totp");
}

// ============================================================================
// QH-11: RedisTLSConfig isValid() — valid when CA cert present
// ============================================================================

TEST(Q3InterfaceHeaders, QH11_RedisTLSConfigValidWithCACert) {
    themis::cache::RedisTLSConfig cfg;
    cfg.require_tls   = true;
    cfg.ca_cert_path  = "/etc/ssl/certs/redis-ca.pem";
    EXPECT_TRUE(cfg.isValid());
}

// ============================================================================
// QH-12: RedisTLSConfig isValid() — invalid when TLS required but no CA cert
// ============================================================================

TEST(Q3InterfaceHeaders, QH12_RedisTLSConfigInvalidMissingCACert) {
    themis::cache::RedisTLSConfig cfg;
    cfg.require_tls  = true;
    cfg.ca_cert_path = "";
    EXPECT_FALSE(cfg.isValid());
}

TEST(Q3InterfaceHeaders, QH12_RedisTLSConfigValidWhenTLSDisabled) {
    themis::cache::RedisTLSConfig cfg;
    cfg.require_tls  = false;
    cfg.ca_cert_path = "";
    EXPECT_TRUE(cfg.isValid());
}

// ============================================================================
// QH-13: L3EncryptionConfig isEncryptionEnabled()
// ============================================================================

TEST(Q3InterfaceHeaders, QH13_L3EncryptionConfigEnabled) {
    themis::cache::L3EncryptionConfig cfg;
    cfg.mode = themis::cache::L3EncryptionMode::AES_256_GCM;
    EXPECT_TRUE(cfg.isEncryptionEnabled());
}

TEST(Q3InterfaceHeaders, QH13_L3EncryptionConfigDefaults) {
    themis::cache::L3EncryptionConfig cfg;
    EXPECT_EQ(cfg.mode, themis::cache::L3EncryptionMode::AES_256_GCM);
    EXPECT_TRUE(cfg.require_auth_tag);
    EXPECT_EQ(cfg.key_rotation_interval_hours, 24);
    EXPECT_FALSE(cfg.encrypt_keys_in_cache);
}

// ============================================================================
// QH-14: L3EncryptionMode::DISABLED check
// ============================================================================

TEST(Q3InterfaceHeaders, QH14_L3EncryptionModeDisabled) {
    themis::cache::L3EncryptionConfig cfg;
    cfg.mode = themis::cache::L3EncryptionMode::DISABLED;
    EXPECT_FALSE(cfg.isEncryptionEnabled());
}

TEST(Q3InterfaceHeaders, QH14_L3EncryptionModeAllValues) {
    using M = themis::cache::L3EncryptionMode;
    EXPECT_NE(M::DISABLED,          M::AES_256_GCM);
    EXPECT_NE(M::AES_256_GCM,       M::CHACHA20_POLY1305);
    EXPECT_NE(M::CHACHA20_POLY1305, M::XTS_AES_256);
}

// ============================================================================
// QH-15: ContentCategory confidence range
// ============================================================================

TEST(Q3InterfaceHeaders, QH15_ContentCategoryConfidenceRange) {
    themis::content::ContentCategory cat;
    cat.category_id = "IAB1";
    cat.label       = "Arts & Entertainment";
    cat.confidence  = 0.87f;
    cat.taxonomy    = "IAB-QAG";

    EXPECT_GE(cat.confidence, 0.0f);
    EXPECT_LE(cat.confidence, 1.0f);
    EXPECT_EQ(cat.taxonomy, "IAB-QAG");
}

// ============================================================================
// QH-16: ContentClassificationRequest defaults
// ============================================================================

TEST(Q3InterfaceHeaders, QH16_ContentClassificationRequestDefaults) {
    themis::content::ContentClassificationRequest req;
    EXPECT_EQ(req.language, "en");
    EXPECT_EQ(req.max_categories, 5);
    EXPECT_TRUE(req.taxonomy_filters.empty());
}

// ============================================================================
// QH-17: PIIMatch offset fields
// ============================================================================

TEST(Q3InterfaceHeaders, QH17_PIIMatchOffsetFields) {
    themis::content::PIIMatch m;
    m.type          = themis::content::PIIType::EMAIL;
    m.start_offset  = 10;
    m.end_offset    = 25;
    m.original_value = "user@example.com";
    m.confidence    = 0.99f;

    EXPECT_EQ(m.start_offset, 10u);
    EXPECT_EQ(m.end_offset, 25u);
    EXPECT_GT(m.end_offset, m.start_offset);
    EXPECT_EQ(m.original_value, "user@example.com");
}

// ============================================================================
// QH-18: PIIRedactionConfig defaults
// ============================================================================

TEST(Q3InterfaceHeaders, QH18_PIIRedactionConfigDefaults) {
    themis::content::PIIRedactionConfig cfg;
    EXPECT_EQ(cfg.mode, themis::content::RedactionMode::REPLACE);
    EXPECT_FLOAT_EQ(cfg.min_confidence, 0.75f);
    EXPECT_FALSE(cfg.preserve_format);
    EXPECT_TRUE(cfg.types_to_redact.empty());
}

// ============================================================================
// QH-19: HealthStatus enum values and AggregateHealthReport helpers
// ============================================================================

TEST(Q3InterfaceHeaders, QH19_HealthStatusEnumValues) {
    using S = themis::core::HealthStatus;
    EXPECT_NE(S::HEALTHY,   S::DEGRADED);
    EXPECT_NE(S::DEGRADED,  S::UNHEALTHY);
    EXPECT_NE(S::UNHEALTHY, S::UNKNOWN);
}

TEST(Q3InterfaceHeaders, QH19_AggregateHealthReportHelpers) {
    themis::core::AggregateHealthReport report;

    report.overall_status = themis::core::HealthStatus::HEALTHY;
    EXPECT_TRUE(report.isHealthy());
    EXPECT_TRUE(report.isReady());

    report.overall_status = themis::core::HealthStatus::DEGRADED;
    EXPECT_FALSE(report.isHealthy());
    EXPECT_TRUE(report.isReady());

    report.overall_status = themis::core::HealthStatus::UNHEALTHY;
    EXPECT_FALSE(report.isHealthy());
    EXPECT_FALSE(report.isReady());
}

// ============================================================================
// QH-20: QuantizationScheme enum (acceleration/quantized_backend.h)
// ============================================================================

TEST(Q3InterfaceHeaders, QH20_QuantizationSchemeEnumValues) {
    using Q = themis::acceleration::QuantizationScheme;
    EXPECT_NE(Q::INT8_SYMMETRIC,  Q::INT8_ASYMMETRIC);
    EXPECT_NE(Q::INT4_SYMMETRIC,  Q::INT4_GROUPED);
    EXPECT_NE(Q::INT8_SYMMETRIC,  Q::FP8_E4M3);
}

// ============================================================================
// QH-20 bonus: ConfigChange field assignment
// ============================================================================

TEST(Q3InterfaceHeaders, QH20b_ConfigChangeFields) {
    themis::core::ConfigChange change;
    change.config_key      = "cache.max_size";
    change.change_type     = themis::core::ConfigChangeType::MODIFIED;
    change.old_value_json  = "\"1024\"";
    change.new_value_json  = "\"2048\"";
    change.changed_at      = std::chrono::system_clock::now();

    EXPECT_EQ(change.config_key, "cache.max_size");
    EXPECT_EQ(change.change_type, themis::core::ConfigChangeType::MODIFIED);
    EXPECT_NE(change.old_value_json, change.new_value_json);
}
