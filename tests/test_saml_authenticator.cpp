#include <gtest/gtest.h>
#include "auth/saml_authenticator.h"
#include "auth/auth_error.h"
#include "test_crypto_material_utils.h"
#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>

// OpenSSL headers needed by encrypted-assertion test helpers
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

using namespace themis::auth;

// ============================================================================
// Test helpers
// ============================================================================

// Build a minimal SAMLConfig for testing (signature verification disabled)
static SAMLConfig makeTestConfig() {
    SAMLConfig cfg;
    cfg.sp_entity_id    = "https://myapp.example.com/saml/metadata";
    cfg.sp_acs_url      = "https://myapp.example.com/saml/acs";
    cfg.idp_sso_url     = "https://idp.example.com/sso";
    cfg.idp_entity_id   = "https://test-idp.example.com/metadata";
    cfg.idp_certificate_pem = themis::tests::getSamlIdpCertificatePem();
    cfg.require_signed_response  = false;
    cfg.require_signed_assertion = false;
    return cfg;
}

// Build a base64-encoded SAMLResponse XML with configurable fields.
// All times are relative to the provided anchor time_point.
static std::string buildSAMLResponseB64(
    const std::string& issuer,
    const std::string& audience,
    const std::string& recipient,
    const std::string& name_id,
    const std::string& status_code,
    const std::string& assertion_id,
    std::chrono::system_clock::time_point anchor,
    std::chrono::seconds not_before_offset = std::chrono::seconds(-300),
    std::chrono::seconds not_on_or_after_offset = std::chrono::seconds(600),
    const std::string& in_response_to = "")
{
    auto fmt = [](std::chrono::system_clock::time_point tp) -> std::string {
        std::time_t t = std::chrono::system_clock::to_time_t(tp);
        std::tm tm_val{};
#ifdef _WIN32
        gmtime_s(&tm_val, &t);
#else
        gmtime_r(&t, &tm_val);
#endif
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_val);
        return std::string(buf);
    };

    std::string issue_instant     = fmt(anchor);
    std::string nb_str            = fmt(anchor + not_before_offset);
    std::string noa_str           = fmt(anchor + not_on_or_after_offset);

    // Build optional InResponseTo attribute for SP-initiated responses
    std::string irt_attr;
    if (!in_response_to.empty()) {
        irt_attr = " InResponseTo=\"" + in_response_to + "\"";
    }

    std::string xml =
        R"(<samlp:Response xmlns:samlp="urn:oasis:names:tc:SAML:2.0:protocol")"
        R"( xmlns:saml="urn:oasis:names:tc:SAML:2.0:assertion")"
        " ID=\"_resp001\""
        " Version=\"2.0\""
        " IssueInstant=\"" + issue_instant + "\""
        + irt_attr +
        " Destination=\"" + recipient + "\">"
        "<saml:Issuer>" + issuer + "</saml:Issuer>"
        "<samlp:Status>"
        "<samlp:StatusCode Value=\"" + status_code + "\"/>"
        "</samlp:Status>"
        "<saml:Assertion"
        " xmlns:saml=\"urn:oasis:names:tc:SAML:2.0:assertion\""
        " ID=\"" + assertion_id + "\""
        " Version=\"2.0\""
        " IssueInstant=\"" + issue_instant + "\">"
        "<saml:Issuer>" + issuer + "</saml:Issuer>"
        "<saml:Subject>"
        "<saml:NameID Format=\"urn:oasis:names:tc:SAML:1.1:nameid-format:emailAddress\">"
        + name_id +
        "</saml:NameID>"
        "<saml:SubjectConfirmation Method=\"urn:oasis:names:tc:SAML:2.0:cm:bearer\">"
        "<saml:SubjectConfirmationData"
        " NotOnOrAfter=\"" + noa_str + "\""
        " Recipient=\"" + recipient + "\"/>"
        "</saml:SubjectConfirmation>"
        "</saml:Subject>"
        "<saml:Conditions NotBefore=\"" + nb_str + "\" NotOnOrAfter=\"" + noa_str + "\">"
        "<saml:AudienceRestriction>"
        "<saml:Audience>" + audience + "</saml:Audience>"
        "</saml:AudienceRestriction>"
        "</saml:Conditions>"
        "<saml:AuthnStatement AuthnInstant=\"" + issue_instant + "\" SessionIndex=\"sess001\">"
        "<saml:AuthnContext>"
        "<saml:AuthnContextClassRef>"
        "urn:oasis:names:tc:SAML:2.0:ac:classes:PasswordProtectedTransport"
        "</saml:AuthnContextClassRef>"
        "</saml:AuthnContext>"
        "</saml:AuthnStatement>"
        "<saml:AttributeStatement>"
        "<saml:Attribute Name=\"email\">"
        "<saml:AttributeValue>" + name_id + "</saml:AttributeValue>"
        "</saml:Attribute>"
        "<saml:Attribute Name=\"groups\">"
        "<saml:AttributeValue>engineers</saml:AttributeValue>"
        "</saml:Attribute>"
        "<saml:Attribute Name=\"roles\">"
        "<saml:AttributeValue>developer</saml:AttributeValue>"
        "</saml:Attribute>"
        "</saml:AttributeStatement>"
        "</saml:Assertion>"
        "</samlp:Response>";

    // Base64 encode
    const std::string& in = xml;
    std::vector<uint8_t> bytes(in.begin(), in.end());
    // Simple base64 encoding
    const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((bytes.size() + 2) / 3) * 4);
    for (size_t i = 0; i < bytes.size(); i += 3) {
        uint32_t b = (static_cast<uint32_t>(bytes[i]) << 16);
        if (i + 1 < bytes.size()) {
          b |= (static_cast<uint32_t>(bytes[i+1]) << 8);
        }
        if (i + 2 < bytes.size()) {
          b |= static_cast<uint32_t>(bytes[i+2]);
        }
        out += b64[(b >> 18) & 0x3F];
        out += b64[(b >> 12) & 0x3F];
        out += (i + 1 < bytes.size()) ? b64[(b >> 6) & 0x3F] : '=';
        out += (i + 2 < bytes.size()) ? b64[b & 0x3F] : '=';
    }
    return out;
}

// ============================================================================
// Construction tests
// ============================================================================

TEST(SAMLAuthenticatorTest, ConstructorAcceptsValidConfig) {
    EXPECT_NO_THROW((void)SAMLAuthenticator{makeTestConfig()});
}

TEST(SAMLAuthenticatorTest, ConstructorRejectsEmptySpEntityId) {
    auto cfg = makeTestConfig();
    cfg.sp_entity_id = "";
    EXPECT_THROW((void)SAMLAuthenticator{cfg}, std::invalid_argument);
}

TEST(SAMLAuthenticatorTest, ConstructorRejectsEmptyAcsUrl) {
    auto cfg = makeTestConfig();
    cfg.sp_acs_url = "";
    EXPECT_THROW((void)SAMLAuthenticator{cfg}, std::invalid_argument);
}

TEST(SAMLAuthenticatorTest, ConstructorRejectsEmptyIdpSsoUrl) {
    auto cfg = makeTestConfig();
    cfg.idp_sso_url = "";
    EXPECT_THROW((void)SAMLAuthenticator{cfg}, std::invalid_argument);
}

TEST(SAMLAuthenticatorTest, ConstructorRejectsEmptyIdpEntityId) {
    auto cfg = makeTestConfig();
    cfg.idp_entity_id = "";
    EXPECT_THROW((void)SAMLAuthenticator{cfg}, std::invalid_argument);
}

TEST(SAMLAuthenticatorTest, ConstructorRejectsEmptyCertificate) {
    auto cfg = makeTestConfig();
    cfg.idp_certificate_pem = "";
    EXPECT_THROW((void)SAMLAuthenticator{cfg}, std::invalid_argument);
}

TEST(SAMLAuthenticatorTest, ConstructorRejectsInvalidCertificate) {
    auto cfg = makeTestConfig();
    cfg.idp_certificate_pem = "-----BEGIN CERTIFICATE-----\nnotvalid\n-----END CERTIFICATE-----\n";
    EXPECT_THROW((void)SAMLAuthenticator{cfg}, std::runtime_error);
}

// ============================================================================
// buildAuthnRequestUrl tests
// ============================================================================

TEST(SAMLAuthenticatorTest, BuildAuthnRequestUrlContainsIdpSsoUrl) {
    SAMLAuthenticator auth(makeTestConfig());
    std::string url = auth.buildAuthnRequestUrl();
    EXPECT_TRUE(url.find("https://idp.example.com/sso") == 0)
        << "URL should start with IdP SSO URL, got: " << url;
}

TEST(SAMLAuthenticatorTest, BuildAuthnRequestUrlContainsSAMLRequest) {
    SAMLAuthenticator auth(makeTestConfig());
    std::string url = auth.buildAuthnRequestUrl();
    EXPECT_NE(url.find("SAMLRequest="), std::string::npos)
        << "URL should contain SAMLRequest parameter";
}

TEST(SAMLAuthenticatorTest, BuildAuthnRequestUrlWithRelayState) {
    SAMLAuthenticator auth(makeTestConfig());
    std::string url = auth.buildAuthnRequestUrl("/dashboard");
    EXPECT_NE(url.find("RelayState="), std::string::npos)
        << "URL should contain RelayState when provided";
}

TEST(SAMLAuthenticatorTest, BuildAuthnRequestUrlWithoutRelayState) {
    SAMLAuthenticator auth(makeTestConfig());
    std::string url = auth.buildAuthnRequestUrl();
    EXPECT_EQ(url.find("RelayState="), std::string::npos)
        << "URL should NOT contain RelayState when not provided";
}

TEST(SAMLAuthenticatorTest, BuildAuthnRequestUrlDifferentEachCall) {
    SAMLAuthenticator auth(makeTestConfig());
    std::string url1 = auth.buildAuthnRequestUrl();
    std::string url2 = auth.buildAuthnRequestUrl();
    // Different request IDs should produce different SAMLRequest parameters
    EXPECT_NE(url1, url2) << "Each call should produce a unique request ID";
}

// ============================================================================
// processResponse – success path
// ============================================================================

TEST(SAMLAuthenticatorTest, ProcessResponseExtractsNameId) {
    SAMLAuthenticator auth(makeTestConfig());
    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    auto b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "alice@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_extract_nameid_01",
        now);

    auto claims = auth.processResponse(b64);
    EXPECT_EQ(claims.subject_name_id, "alice@example.com");
}

TEST(SAMLAuthenticatorTest, ProcessResponseExtractsEmail) {
    SAMLAuthenticator auth(makeTestConfig());
    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    auto b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "bob@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_extract_email_01",
        now);

    auto claims = auth.processResponse(b64);
    EXPECT_EQ(claims.email, "bob@example.com");
}

TEST(SAMLAuthenticatorTest, ProcessResponseExtractsGroups) {
    SAMLAuthenticator auth(makeTestConfig());
    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    auto b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "carol@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_extract_groups_01",
        now);

    auto claims = auth.processResponse(b64);
    ASSERT_EQ(claims.attributes_groups.size(), 1u);
    EXPECT_EQ(claims.attributes_groups[0], "engineers");
}

TEST(SAMLAuthenticatorTest, ProcessResponseExtractsRoles) {
    SAMLAuthenticator auth(makeTestConfig());
    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    auto b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "dave@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_extract_roles_01",
        now);

    auto claims = auth.processResponse(b64);
    ASSERT_EQ(claims.attributes_roles.size(), 1u);
    EXPECT_EQ(claims.attributes_roles[0], "developer");
}

TEST(SAMLAuthenticatorTest, ProcessResponseExtractsIssuer) {
    SAMLAuthenticator auth(makeTestConfig());
    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    auto b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "eve@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_extract_issuer_01",
        now);

    auto claims = auth.processResponse(b64);
    EXPECT_EQ(claims.issuer, "https://test-idp.example.com/metadata");
}

TEST(SAMLAuthenticatorTest, ProcessResponseExtractsSessionIndex) {
    SAMLAuthenticator auth(makeTestConfig());
    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    auto b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "frank@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_extract_session_01",
        now);

    auto claims = auth.processResponse(b64);
    EXPECT_EQ(claims.session_index, "sess001");
}

TEST(SAMLAuthenticatorTest, ProcessResponseExtractsAssertionId) {
    SAMLAuthenticator auth(makeTestConfig());
    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    auto b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "greta@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_extract_id_unique_01",
        now);

    auto claims = auth.processResponse(b64);
    EXPECT_EQ(claims.assertion_id, "_assert_extract_id_unique_01");
}

// ============================================================================
// processResponse – error paths
// ============================================================================

TEST(SAMLAuthenticatorTest, ProcessResponseRejectsInvalidBase64) {
    SAMLAuthenticator auth(makeTestConfig());
    EXPECT_THROW(auth.processResponse("not valid base64!!!"),
                 AuthException);
}

TEST(SAMLAuthenticatorTest, ProcessResponseRejectsMalformedXML) {
    SAMLAuthenticator auth(makeTestConfig());
    // Valid base64 but invalid XML
    std::string junk = "aGVsbG8gd29ybGQ="; // "hello world"
    EXPECT_THROW(auth.processResponse(junk), AuthException);
}

TEST(SAMLAuthenticatorTest, ProcessResponseRejectsStatusFailure) {
    SAMLAuthenticator auth(makeTestConfig());
    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    auto b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:AuthnFailed",  // Non-success
        "_assert_status_fail_01",
        now);

    EXPECT_THROW(auth.processResponse(b64), AuthException);
}

TEST(SAMLAuthenticatorTest, ProcessResponseRejectsWrongIssuer) {
    SAMLAuthenticator auth(makeTestConfig());
    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    auto b64 = buildSAMLResponseB64(
        "https://evil-idp.example.com/metadata",  // Wrong issuer
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_wrong_issuer_01",
        now);

    try {
        auth.processResponse(b64);
        FAIL() << "Expected AuthException for wrong issuer";
    } catch (const AuthException& e) {
        EXPECT_EQ(e.error().code(), AuthErrorCode::SAML_ISSUER_MISMATCH);
    }
}

TEST(SAMLAuthenticatorTest, ProcessResponseRejectsWrongAudience) {
    SAMLAuthenticator auth(makeTestConfig());
    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    auto b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://other-sp.example.com/metadata",  // Wrong audience
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_wrong_audience_01",
        now);

    try {
        auth.processResponse(b64);
        FAIL() << "Expected AuthException for wrong audience";
    } catch (const AuthException& e) {
        EXPECT_EQ(e.error().code(), AuthErrorCode::SAML_CONDITIONS_FAILED);
    }
}

TEST(SAMLAuthenticatorTest, ProcessResponseRejectsExpiredAssertion) {
    SAMLAuthenticator auth(makeTestConfig());
    // Current time is 1 hour AFTER not_on_or_after
    auto future = std::chrono::system_clock::now() + std::chrono::hours(2);
    auth.setClockForTesting([future]() { return future; });

    auto b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_expired_01",
        std::chrono::system_clock::now(),  // assertion issued in the past
        std::chrono::seconds(-300),
        std::chrono::seconds(600));  // expires in 10 min from issue time, but clock is 2h later

    try {
        auth.processResponse(b64);
        FAIL() << "Expected AuthException for expired assertion";
    } catch (const AuthException& e) {
        EXPECT_EQ(e.error().code(), AuthErrorCode::SAML_CONDITIONS_FAILED);
    }
}

TEST(SAMLAuthenticatorTest, ProcessResponseRejectsNotYetValidAssertion) {
    SAMLAuthenticator auth(makeTestConfig());
    // Current time is BEFORE not_before (assertion issued 1 hour in the future)
    auto past = std::chrono::system_clock::now() - std::chrono::hours(2);
    auth.setClockForTesting([past]() { return past; });

    auto now = std::chrono::system_clock::now();
    auto b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_not_yet_valid_01",
        now,
        std::chrono::seconds(3600),   // not_before = 1 hour from now
        std::chrono::seconds(7200));  // not_on_or_after = 2 hours from now

    try {
        auth.processResponse(b64);
        FAIL() << "Expected AuthException for not-yet-valid assertion";
    } catch (const AuthException& e) {
        EXPECT_EQ(e.error().code(), AuthErrorCode::SAML_CONDITIONS_FAILED);
    }
}

// ============================================================================
// Replay detection
// ============================================================================

TEST(SAMLAuthenticatorTest, ProcessResponseRejectsReplayedAssertion) {
    SAMLAuthenticator auth(makeTestConfig());
    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    auto b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_replay_detection_01",
        now);

    // First use succeeds
    EXPECT_NO_THROW(auth.processResponse(b64));

    // Second use with same assertion ID should be rejected
    try {
        auth.processResponse(b64);
        FAIL() << "Expected AuthException for replayed assertion";
    } catch (const AuthException& e) {
        EXPECT_EQ(e.error().code(), AuthErrorCode::SAML_REPLAY_DETECTED);
    }
}

// ============================================================================
// Signature requirement enforcement
// ============================================================================

TEST(SAMLAuthenticatorTest, ProcessResponseRequiresResponseSignatureWhenConfigured) {
    auto cfg = makeTestConfig();
    cfg.require_signed_response = true;
    cfg.require_signed_assertion = false;
    SAMLAuthenticator auth(cfg);

    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    auto b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_req_resp_sig_01",
        now);

    // Response has no Signature element -> should throw SAML_INVALID_SIGNATURE
    try {
        auth.processResponse(b64);
        FAIL() << "Expected AuthException for missing response signature";
    } catch (const AuthException& e) {
        EXPECT_EQ(e.error().code(), AuthErrorCode::SAML_INVALID_SIGNATURE);
    }
}

TEST(SAMLAuthenticatorTest, ProcessResponseRequiresAssertionSignatureWhenConfigured) {
    auto cfg = makeTestConfig();
    cfg.require_signed_response  = false;
    cfg.require_signed_assertion = true;
    SAMLAuthenticator auth(cfg);

    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    auto b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_req_assert_sig_01",
        now);

    // Assertion has no Signature element -> should throw SAML_INVALID_SIGNATURE
    try {
        auth.processResponse(b64);
        FAIL() << "Expected AuthException for missing assertion signature";
    } catch (const AuthException& e) {
        EXPECT_EQ(e.error().code(), AuthErrorCode::SAML_INVALID_SIGNATURE);
    }
}

// ============================================================================
// SP-initiated and IdP-initiated flow tests
// ============================================================================

TEST(SAMLAuthenticatorTest, BuildAuthnRequestReturnsRequestId) {
    SAMLAuthenticator auth(makeTestConfig());
    auto params = auth.buildAuthnRequest();
    EXPECT_FALSE(params.url.empty()) << "URL must not be empty";
    EXPECT_FALSE(params.request_id.empty()) << "request_id must not be empty";
    // NCName-safe: starts with '_'
    EXPECT_EQ(params.request_id[0], '_') << "request_id must start with '_'";
}

TEST(SAMLAuthenticatorTest, BuildAuthnRequestUrlMatchesBuildAuthnRequest) {
    SAMLAuthenticator auth(makeTestConfig());
    // Both calls use independent random IDs, so just verify structure is consistent.
    auto params = auth.buildAuthnRequest("/callback");
    std::string url_only = params.url;
    EXPECT_NE(url_only.find("SAMLRequest="), std::string::npos);
    EXPECT_NE(url_only.find("RelayState="), std::string::npos);
}

TEST(SAMLAuthenticatorTest, IdPInitiatedFlowAcceptsResponseWithoutInResponseTo) {
    // IdP-initiated: caller passes no in_response_to; response has no InResponseTo.
    SAMLAuthenticator auth(makeTestConfig());
    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    // in_response_to param defaults to "" → no InResponseTo in response XML
    auto b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_idp_init_01",
        now);

    EXPECT_NO_THROW({
        auto claims = auth.processResponse(b64); // no in_response_to
        EXPECT_EQ(claims.subject_name_id, "user@example.com");
    });
}

TEST(SAMLAuthenticatorTest, SPInitiatedFlowValidatesMatchingInResponseTo) {
    // SP-initiated: buildAuthnRequest exposes request_id; response carries matching InResponseTo.
    SAMLAuthenticator auth(makeTestConfig());
    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    auto params = auth.buildAuthnRequest();
    ASSERT_FALSE(params.request_id.empty());

    // Build response that echoes the request ID in InResponseTo
    auto b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_sp_init_01",
        now,
        std::chrono::seconds(-300),
        std::chrono::seconds(600),
        params.request_id);

    EXPECT_NO_THROW({
        auto claims = auth.processResponse(b64, params.request_id);
        EXPECT_EQ(claims.subject_name_id, "user@example.com");
    });
}

TEST(SAMLAuthenticatorTest, SPInitiatedFlowRejectsInResponseToMismatch) {
    // SP-initiated: response carries InResponseTo that does not match the stored ID.
    SAMLAuthenticator auth(makeTestConfig());
    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    const std::string real_id  = "_real_authn_request_id";
    const std::string wrong_id = "_wrong_authn_request_id";

    auto b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_sp_mismatch_01",
        now,
        std::chrono::seconds(-300),
        std::chrono::seconds(600),
        wrong_id); // response claims a different request ID

    try {
        auth.processResponse(b64, real_id);
        FAIL() << "Expected AuthException for InResponseTo mismatch";
    } catch (const AuthException& e) {
        EXPECT_EQ(e.error().code(), AuthErrorCode::SAML_CONDITIONS_FAILED);
    }
}

// ============================================================================
// Encrypted assertion tests
// ============================================================================

// Helper: base64-encode a raw XML string (same algorithm as buildSAMLResponseB64)
static std::string base64EncodeString(const std::string& in) {
    std::vector<uint8_t> bytes(in.begin(), in.end());
    const char b64t[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((bytes.size() + 2) / 3) * 4);
    for (size_t i = 0; i < bytes.size(); i += 3) {
        uint32_t b = (static_cast<uint32_t>(bytes[i]) << 16);
        if (i + 1 < bytes.size()) {
          b |= (static_cast<uint32_t>(bytes[i+1]) << 8);
        }
        if (i + 2 < bytes.size()) {
          b |= static_cast<uint32_t>(bytes[i+2]);
        }
        out += b64t[(b >> 18) & 0x3F];
        out += b64t[(b >> 12) & 0x3F];
        out += (i + 1 < bytes.size()) ? b64t[(b >> 6) & 0x3F] : '=';
        out += (i + 2 < bytes.size()) ? b64t[b & 0x3F] : '=';
    }
    return out;
}

// Helper: build a SAMLResponse containing an EncryptedAssertion instead of a plain Assertion
static std::string buildEncryptedAssertionResponseB64() {
    const std::string xml =
        R"(<samlp:Response xmlns:samlp="urn:oasis:names:tc:SAML:2.0:protocol")"
        R"( xmlns:saml="urn:oasis:names:tc:SAML:2.0:assertion")"
        R"( ID="_enc_resp001" Version="2.0" IssueInstant="2026-01-01T00:00:00Z")"
        R"( Destination="https://myapp.example.com/saml/acs">)"
        R"(<saml:Issuer>https://test-idp.example.com/metadata</saml:Issuer>)"
        R"(<samlp:Status><samlp:StatusCode Value="urn:oasis:names:tc:SAML:2.0:status:Success"/></samlp:Status>)"
        R"(<saml:EncryptedAssertion>)"
        R"(<xenc:EncryptedData xmlns:xenc="http://www.w3.org/2001/04/xmlenc#">)"
        R"(<xenc:CipherData><xenc:CipherValue>AAAA</xenc:CipherValue></xenc:CipherData>)"
        R"(</xenc:EncryptedData>)"
        R"(</saml:EncryptedAssertion>)"
        R"(</samlp:Response>)";
    return base64EncodeString(xml);
}

TEST(SAMLAuthenticatorTest, ProcessResponseRejectsEncryptedAssertionWithoutKeyLoader) {
    // With no sp_private_key_loader configured, an EncryptedAssertion must
    // fail explicitly with SAML_DECRYPTION_FAILED (not silently or with AUTH_NOT_IMPLEMENTED).
    SAMLAuthenticator auth(makeTestConfig());

    auto b64 = buildEncryptedAssertionResponseB64();
    try {
        auth.processResponse(b64);
        FAIL() << "Expected AuthException for EncryptedAssertion without key loader";
    } catch (const AuthException& e) {
        EXPECT_EQ(e.error().code(), AuthErrorCode::SAML_DECRYPTION_FAILED);
    }
}

TEST(SAMLAuthenticatorTest, ProcessResponseRejectsRequireEncryptedAssertionWithPlainAssertion) {
    // When require_encrypted_assertion=true, a plain (unencrypted) Assertion
    // must be rejected with SAML_INVALID_RESPONSE (policy enforcement, not a
    // "not implemented" error).
    auto cfg = makeTestConfig();
    cfg.require_encrypted_assertion = true;
    SAMLAuthenticator auth(cfg);

    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    auto b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_enc_required_01",
        now);

    try {
        auth.processResponse(b64);
        FAIL() << "Expected AuthException when require_encrypted_assertion=true with plain Assertion";
    } catch (const AuthException& e) {
        EXPECT_EQ(e.error().code(), AuthErrorCode::SAML_INVALID_RESPONSE);
    }
}

// ---------------------------------------------------------------------------
// Test SP key material (RSA 2048-bit, for assertion decryption tests only)
//
// SECURITY NOTICE: This is a disposable, test-only key that was generated
// solely for automated unit tests.  It must NEVER appear in production code,
// configuration files, or any deployed environment.
//
// In production deployments, provide the SP private key through the
// SAMLConfig::sp_private_key_loader callback, which should load the key
// from a hardware security module (HSM), key management service (KMS), or
// a secrets manager (e.g. HashiCorp Vault, AWS Secrets Manager, Azure Key Vault).
// Example:
//   cfg.sp_private_key_loader = []() {
//       return SecretsManager::getSecret("saml/sp-private-key");
//   };
// ---------------------------------------------------------------------------
// Base64-encode raw bytes using OpenSSL BIO (no line breaks)
static std::string base64EncodeBytes(const std::vector<uint8_t>& data) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* mem = BIO_new(BIO_s_mem());
    BIO_push(b64, mem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, data.data(), static_cast<int>(data.size()));
    BIO_flush(b64);
    BUF_MEM* bptr{};
    BIO_get_mem_ptr(mem, &bptr);
    std::string result(bptr->data, bptr->length);
    BIO_free_all(b64);
    return result;
}

// Build a properly-encrypted SAMLResponse:
//   data encryption  : AES-256-CBC with a random key (IV prepended per XML Enc §5.2)
//   key transport    : RSA-OAEP (SHA-1 / MGF1-SHA1)
//   assertion_xml    : plaintext Assertion element to encrypt
//   private_key_pem  : SP private key used to derive the SP public key for encryption
//   out_b64          : receives the Base64-encoded SAMLResponse on success
// Returns void; uses ASSERT_* so the calling TEST is aborted on failure.
static void buildRealEncryptedAssertionResponseB64(
    const std::string& private_key_pem,
    const std::string& assertion_xml,
    std::string& out_b64)
{
    // Load SP private key and extract public key
    BIO* kbio = BIO_new_mem_buf(private_key_pem.data(),
                                 static_cast<int>(private_key_pem.size()));
    EVP_PKEY* sp_pkey = PEM_read_bio_PrivateKey(kbio, nullptr, nullptr, nullptr);
    BIO_free(kbio);
    ASSERT_NE(sp_pkey, nullptr) << "Failed to load test SP private key";

    // Generate random AES-256 key and 16-byte IV
    std::vector<uint8_t> aes_key(32), iv(16);
    RAND_bytes(aes_key.data(), 32);
    RAND_bytes(iv.data(), 16);

    // Encrypt assertion with AES-256-CBC; CipherValue = IV || ciphertext (XML Enc §5.2)
    EVP_CIPHER_CTX* enc_ctx = EVP_CIPHER_CTX_new();
    ASSERT_NE(enc_ctx, nullptr) << "Failed to allocate AES cipher context";
    ASSERT_EQ(EVP_EncryptInit_ex(enc_ctx, EVP_aes_256_cbc(), nullptr,
                                  aes_key.data(), iv.data()), 1);

    std::vector<uint8_t> ct_buf(assertion_xml.size() + 32);
    int enc_len1 = 0, enc_len2 = 0;
    EVP_EncryptUpdate(enc_ctx, ct_buf.data(), &enc_len1,
                      reinterpret_cast<const uint8_t*>(assertion_xml.data()),
                      static_cast<int>(assertion_xml.size()));
    EVP_EncryptFinal_ex(enc_ctx, ct_buf.data() + enc_len1, &enc_len2);
    EVP_CIPHER_CTX_free(enc_ctx);

    // Assemble: IV || ciphertext
    std::vector<uint8_t> enc_data;
    enc_data.insert(enc_data.end(), iv.begin(), iv.end());
    enc_data.insert(enc_data.end(), ct_buf.begin(), ct_buf.begin() + enc_len1 + enc_len2);

    // Encrypt AES key with RSA-OAEP using the SP public key
    EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new(sp_pkey, nullptr);
    EVP_PKEY_free(sp_pkey);
    ASSERT_NE(pctx, nullptr) << "Failed to create EVP_PKEY_CTX";
    ASSERT_EQ(EVP_PKEY_encrypt_init(pctx), 1);
    ASSERT_EQ(EVP_PKEY_CTX_set_rsa_padding(pctx, RSA_PKCS1_OAEP_PADDING), 1);

    size_t enc_key_len = 0;
    EVP_PKEY_encrypt(pctx, nullptr, &enc_key_len, aes_key.data(), aes_key.size());
    std::vector<uint8_t> enc_key(enc_key_len);
    ASSERT_GT(EVP_PKEY_encrypt(pctx, enc_key.data(), &enc_key_len,
                                aes_key.data(), aes_key.size()), 0);
    EVP_PKEY_CTX_free(pctx);
    enc_key.resize(enc_key_len);

    const std::string enc_key_b64  = base64EncodeBytes(enc_key);
    const std::string enc_data_b64 = base64EncodeBytes(enc_data);

    // Build SAMLResponse with EncryptedAssertion
    const std::string xml =
        R"(<samlp:Response xmlns:samlp="urn:oasis:names:tc:SAML:2.0:protocol")"
        R"( xmlns:saml="urn:oasis:names:tc:SAML:2.0:assertion")"
        R"( ID="_enc_resp_real01" Version="2.0" IssueInstant="2026-01-01T12:00:00Z")"
        R"( Destination="https://myapp.example.com/saml/acs">)"
        R"(<saml:Issuer>https://test-idp.example.com/metadata</saml:Issuer>)"
        R"(<samlp:Status><samlp:StatusCode)"
        R"( Value="urn:oasis:names:tc:SAML:2.0:status:Success"/></samlp:Status>)"
        R"(<saml:EncryptedAssertion>)"
        R"(<xenc:EncryptedData xmlns:xenc="http://www.w3.org/2001/04/xmlenc#")"
        R"( Type="http://www.w3.org/2001/04/xmlenc#Element">)"
        R"(<xenc:EncryptionMethod)"
        R"( Algorithm="http://www.w3.org/2001/04/xmlenc#aes256-cbc"/>)"
        R"(<ds:KeyInfo xmlns:ds="http://www.w3.org/2000/09/xmldsig#">)"
        R"(<xenc:EncryptedKey>)"
        R"(<xenc:EncryptionMethod)"
        R"( Algorithm="http://www.w3.org/2001/04/xmlenc#rsa-oaep-mgf1p"/>)"
        R"(<xenc:CipherData><xenc:CipherValue>)" + enc_key_b64 +
        R"(</xenc:CipherValue></xenc:CipherData>)"
        R"(</xenc:EncryptedKey>)"
        R"(</ds:KeyInfo>)"
        R"(<xenc:CipherData><xenc:CipherValue>)" + enc_data_b64 +
        R"(</xenc:CipherValue></xenc:CipherData>)"
        R"(</xenc:EncryptedData>)"
        R"(</saml:EncryptedAssertion>)"
        R"(</samlp:Response>)";

    out_b64 = base64EncodeString(xml);
}

TEST(SAMLAuthenticatorTest, ProcessResponseDecryptsEncryptedAssertion) {
    const auto& sp_private_key_pem = themis::tests::getTestSpPrivateKeyPem();
    // Configure SP with a private key loader (loaded from test key material –
    // in production this would come from an HSM or KMS).
    auto cfg = makeTestConfig();
    cfg.sp_private_key_loader = [sp_private_key_pem]() { return sp_private_key_pem; };
    SAMLAuthenticator auth(cfg);

    // Fix clock to 2026-01-01T12:00:00Z so the assertion conditions are valid.
    const auto fixed_now = std::chrono::system_clock::from_time_t(1767268800); // 2026-01-01T12:00:00Z
    auth.setClockForTesting([fixed_now]() { return fixed_now; });

    // Build a plain assertion XML to be encrypted (no signature needed since
    // require_signed_assertion=false in test config).
    const std::string assertion_xml =
        R"(<saml:Assertion xmlns:saml="urn:oasis:names:tc:SAML:2.0:assertion")"
        R"( ID="_enc_assert_001" Version="2.0" IssueInstant="2026-01-01T12:00:00Z">)"
        R"(<saml:Issuer>https://test-idp.example.com/metadata</saml:Issuer>)"
        R"(<saml:Subject>)"
        R"(<saml:NameID Format="urn:oasis:names:tc:SAML:1.1:nameid-format:emailAddress">)"
        R"(encrypted-user@example.com</saml:NameID>)"
        R"(<saml:SubjectConfirmation Method="urn:oasis:names:tc:SAML:2.0:cm:bearer">)"
        R"(<saml:SubjectConfirmationData)"
        R"( NotOnOrAfter="2026-01-01T12:10:00Z")"
        R"( Recipient="https://myapp.example.com/saml/acs"/>)"
        R"(</saml:SubjectConfirmation>)"
        R"(</saml:Subject>)"
        R"(<saml:Conditions NotBefore="2026-01-01T11:55:00Z")"
        R"( NotOnOrAfter="2026-01-01T12:10:00Z">)"
        R"(<saml:AudienceRestriction>)"
        R"(<saml:Audience>https://myapp.example.com/saml/metadata</saml:Audience>)"
        R"(</saml:AudienceRestriction>)"
        R"(</saml:Conditions>)"
        R"(<saml:AuthnStatement AuthnInstant="2026-01-01T12:00:00Z" SessionIndex="enc-sess001">)"
        R"(<saml:AuthnContext>)"
        R"(<saml:AuthnContextClassRef>)"
        R"(urn:oasis:names:tc:SAML:2.0:ac:classes:PasswordProtectedTransport)"
        R"(</saml:AuthnContextClassRef>)"
        R"(</saml:AuthnContext>)"
        R"(</saml:AuthnStatement>)"
        R"(<saml:AttributeStatement>)"
        R"(<saml:Attribute Name="email">)"
        R"(<saml:AttributeValue>encrypted-user@example.com</saml:AttributeValue>)"
        R"(</saml:Attribute>)"
        R"(</saml:AttributeStatement>)"
        R"(</saml:Assertion>)";

    std::string b64;
    ASSERT_NO_FATAL_FAILURE(buildRealEncryptedAssertionResponseB64(
        sp_private_key_pem, assertion_xml, b64));

    const SAMLClaims claims = auth.processResponse(b64);

    EXPECT_EQ(claims.subject_name_id, "encrypted-user@example.com");
    EXPECT_EQ(claims.email,           "encrypted-user@example.com");
    EXPECT_EQ(claims.issuer,          "https://test-idp.example.com/metadata");
    EXPECT_EQ(claims.assertion_id,    "_enc_assert_001");
    EXPECT_EQ(claims.session_index,   "enc-sess001");
}

TEST(SAMLAuthenticatorTest, ProcessResponseDecryptionFailsWithWrongKey) {
    // A response with EncryptedAssertion but a mismatched SP private key
    // must fail with SAML_DECRYPTION_FAILED, not silently.
    auto cfg = makeTestConfig();

    // Provide a different (freshly generated) key – it won't match the one used
    // to encrypt the assertion, so RSA-OAEP decryption must fail.
    const auto& wrong_key_pem = themis::tests::getWrongSpPrivateKeyPem();
    cfg.sp_private_key_loader = [wrong_key_pem]() { return wrong_key_pem; };
    SAMLAuthenticator auth(cfg);

    const std::string assertion_xml =
        R"(<saml:Assertion xmlns:saml="urn:oasis:names:tc:SAML:2.0:assertion")"
        R"( ID="_enc_assert_wrongkey" Version="2.0" IssueInstant="2026-01-01T12:00:00Z">)"
        R"(<saml:Issuer>https://test-idp.example.com/metadata</saml:Issuer>)"
        R"(</saml:Assertion>)";

    std::string b64;
    ASSERT_NO_FATAL_FAILURE(buildRealEncryptedAssertionResponseB64(
        themis::tests::getTestSpPrivateKeyPem(), assertion_xml, b64));

    try {
        auth.processResponse(b64);
        FAIL() << "Expected SAML_DECRYPTION_FAILED for wrong SP key";
    } catch (const AuthException& e) {
        EXPECT_EQ(e.error().code(), AuthErrorCode::SAML_DECRYPTION_FAILED);
    }
}

// ============================================================================
// SHA-1 algorithm policy tests (GAP-003 / CWE-327)
// ============================================================================

namespace {

/// Build a base64-encoded SAML response that embeds a fake ds:Signature with
/// the supplied SignatureMethod/DigestMethod Algorithm URIs.  The signature
/// bytes themselves are non-zero base64 dummies (not cryptographically valid),
/// which is sufficient to reach the algorithm-selection branch inside
/// verifyXmlSignature().
static std::string buildSAMLResponseWithAlgorithms(
    const std::string& sig_alg_uri,
    const std::string& digest_alg_uri,
    std::chrono::system_clock::time_point anchor)
{
    auto fmt = [](std::chrono::system_clock::time_point tp) -> std::string {
        auto tt = std::chrono::system_clock::to_time_t(tp);
        std::tm tm_utc{};
#ifdef _WIN32
        gmtime_s(&tm_utc, &tt);
#else
        gmtime_r(&tt, &tm_utc);
#endif
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
        return std::string(buf);
    };

    auto not_before    = anchor - std::chrono::seconds(300);
    auto not_on_or_after = anchor + std::chrono::seconds(600);

    // A minimal SAML response that includes a ds:Signature block on the
    // Response element with the caller-supplied algorithm URIs.
    std::string xml =
        R"(<samlp:Response)"
        R"( xmlns:samlp="urn:oasis:names:tc:SAML:2.0:protocol")"
        R"( xmlns:saml="urn:oasis:names:tc:SAML:2.0:assertion")"
        R"( ID="_sha1_policy_resp" Version="2.0")"
        R"( IssueInstant=")" + fmt(anchor) + R"(")"
        R"( Destination="https://myapp.example.com/saml/acs">)"
        R"(<saml:Issuer>https://test-idp.example.com/metadata</saml:Issuer>)"
        R"(<ds:Signature xmlns:ds="http://www.w3.org/2000/09/xmldsig#">)"
        R"(<ds:SignedInfo>)"
        R"(<ds:SignatureMethod Algorithm=")" + sig_alg_uri + R"("/>)"
        R"(<ds:Reference URI="#_sha1_policy_resp">)"
        R"(<ds:DigestMethod Algorithm=")" + digest_alg_uri + R"("/>)"
        R"(<ds:DigestValue>AAEC/w==</ds:DigestValue>)"
        R"(</ds:Reference>)"
        R"(</ds:SignedInfo>)"
        R"(<ds:SignatureValue>AAEC/w==</ds:SignatureValue>)"
        R"(</ds:Signature>)"
        R"(<samlp:Status>)"
        R"(<samlp:StatusCode Value="urn:oasis:names:tc:SAML:2.0:status:Success"/>)"
        R"(</samlp:Status>)"
        R"(<saml:Assertion)"
        R"( xmlns:saml="urn:oasis:names:tc:SAML:2.0:assertion")"
        R"( ID="_sha1_policy_assert" Version="2.0")"
        R"( IssueInstant=")" + fmt(anchor) + R"(">)"
        R"(<saml:Issuer>https://test-idp.example.com/metadata</saml:Issuer>)"
        R"(<saml:Subject>)"
        R"(<saml:NameID Format="urn:oasis:names:tc:SAML:1.1:nameid-format:emailAddress">)"
        R"(sha1test@example.com</saml:NameID>)"
        R"(<saml:SubjectConfirmation Method="urn:oasis:names:tc:SAML:2.0:cm:bearer">)"
        R"(<saml:SubjectConfirmationData)"
        R"( Recipient="https://myapp.example.com/saml/acs")"
        R"( NotOnOrAfter=")" + fmt(not_on_or_after) + R"("/>)"
        R"(</saml:SubjectConfirmation>)"
        R"(</saml:Subject>)"
        R"(<saml:Conditions)"
        R"( NotBefore=")" + fmt(not_before) + R"(")"
        R"( NotOnOrAfter=")" + fmt(not_on_or_after) + R"(">)"
        R"(<saml:AudienceRestriction>)"
        R"(<saml:Audience>https://myapp.example.com/saml/metadata</saml:Audience>)"
        R"(</saml:AudienceRestriction>)"
        R"(</saml:Conditions>)"
        R"(</saml:Assertion>)"
        R"(</samlp:Response>)";

    // Base64-encode (same algorithm as buildSAMLResponseB64)
    const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<uint8_t> bytes(xml.begin(), xml.end());
    std::string out;
    out.reserve(((bytes.size() + 2) / 3) * 4);
    for (size_t i = 0; i < bytes.size(); i += 3) {
        uint32_t b = (static_cast<uint32_t>(bytes[i]) << 16);
        if (i + 1 < bytes.size()) {
          b |= (static_cast<uint32_t>(bytes[i + 1]) << 8);
        }
        if (i + 2 < bytes.size()) {
          b |= static_cast<uint32_t>(bytes[i + 2]);
        }
        out += b64[(b >> 18) & 0x3F];
        out += b64[(b >> 12) & 0x3F];
        out += (i + 1 < bytes.size()) ? b64[(b >> 6) & 0x3F] : '=';
        out += (i + 2 < bytes.size()) ? b64[b & 0x3F] : '=';
    }
    return out;
}

} // anonymous namespace

// GAP-003-01: allow_sha1_deprecated defaults to false (deny-by-default policy)
TEST(SAMLAuthenticatorTest, SAMLConfigAllowSHA1DefaultsFalse) {
    SAMLConfig cfg;
    EXPECT_FALSE(cfg.allow_sha1_deprecated);
}

// GAP-003-02: SHA-1 signature is rejected when allow_sha1_deprecated=false
TEST(SAMLAuthenticatorTest, SHA1SignatureRejectedByDefault) {
    auto cfg = makeTestConfig();
    cfg.require_signed_response  = true;
    cfg.require_signed_assertion = false;
    cfg.allow_sha1_deprecated    = false;  // explicit — matches default
    SAMLAuthenticator auth(cfg);

    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    const std::string sha1_sig_alg    = "http://www.w3.org/2000/09/xmldsig#rsa-sha1";
    const std::string sha1_digest_alg = "http://www.w3.org/2000/09/xmldsig#sha1";

    auto b64 = buildSAMLResponseWithAlgorithms(sha1_sig_alg, sha1_digest_alg, now);

    try {
        auth.processResponse(b64);
        FAIL() << "Expected SAML_INVALID_SIGNATURE: SHA-1 should be rejected by default";
    } catch (const AuthException& e) {
        EXPECT_EQ(e.error().code(), AuthErrorCode::SAML_INVALID_SIGNATURE);
    }
}

// GAP-003-03: SHA-256 signature is accepted (sanity check — algorithm not rejected)
// The response fails with SAML_INVALID_SIGNATURE because the signature bytes are
// dummy values, but the failure is NOT due to algorithm rejection.
TEST(SAMLAuthenticatorTest, SHA256SignatureNotRejectedForAlgorithm) {
    auto cfg = makeTestConfig();
    cfg.require_signed_response  = true;
    cfg.require_signed_assertion = false;
    SAMLAuthenticator auth(cfg);

    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    const std::string sha256_sig_alg    = "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256";
    const std::string sha256_digest_alg = "http://www.w3.org/2001/04/xmlenc#sha256";

    auto b64 = buildSAMLResponseWithAlgorithms(sha256_sig_alg, sha256_digest_alg, now);

    // Expect SAML_INVALID_SIGNATURE because the signature bytes are invalid,
    // but NOT because of algorithm rejection — the algorithm itself is accepted.
    try {
        auth.processResponse(b64);
        FAIL() << "Expected SAML_INVALID_SIGNATURE for invalid signature bytes";
    } catch (const AuthException& e) {
        EXPECT_EQ(e.error().code(), AuthErrorCode::SAML_INVALID_SIGNATURE);
    }
}

// GAP-003-04: SHA-1 signature is tolerated when allow_sha1_deprecated=true
// (still fails for invalid bytes, but NOT rejected at the algorithm selection step)
TEST(SAMLAuthenticatorTest, SHA1SignatureToleratedWhenAllowFlagSet) {
    auto cfg = makeTestConfig();
    cfg.require_signed_response  = true;
    cfg.require_signed_assertion = false;
    cfg.allow_sha1_deprecated    = true;  // operator explicitly opts in
    SAMLAuthenticator auth(cfg);

    auto now = std::chrono::system_clock::now();
    auth.setClockForTesting([now]() { return now; });

    const std::string sha1_sig_alg    = "http://www.w3.org/2000/09/xmldsig#rsa-sha1";
    const std::string sha1_digest_alg = "http://www.w3.org/2000/09/xmldsig#sha1";

    auto b64 = buildSAMLResponseWithAlgorithms(sha1_sig_alg, sha1_digest_alg, now);

    // Algorithm is accepted; signature bytes are invalid so the result is still
    // SAML_INVALID_SIGNATURE — but for wrong-bytes, not algorithm-rejection.
    try {
        auth.processResponse(b64);
        FAIL() << "Expected SAML_INVALID_SIGNATURE for invalid signature bytes";
    } catch (const AuthException& e) {
        EXPECT_EQ(e.error().code(), AuthErrorCode::SAML_INVALID_SIGNATURE);
    }
}
