/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_saml_authenticator.cpp                        ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-23 03:59:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     664                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d7c4a035d  2026-02-22  Fix SAML encrypted assertion stub: enforce EncryptedAsser... ║
    • 7f9832271  2026-02-22  feat(auth): implement SAML 2.0 identity provider integration ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "auth/saml_authenticator.h"
#include "auth/auth_error.h"
#include <chrono>
#include <string>

using namespace themis::auth;

// ============================================================================
// Test helpers
// ============================================================================

// Minimal self-signed PEM certificate for testing (RSA 2048-bit, CN=test-idp)
// Generated with: openssl req -x509 -newkey rsa:2048 -nodes -subj "/CN=test-idp"
static const char* TEST_IDP_CERT_PEM = R"(
-----BEGIN CERTIFICATE-----
MIIDBzCCAe+gAwIBAgIUUQsDiuQPF+t50DNG8YV3LvTxi2cwDQYJKoZIhvcNAQEL
BQAwEzERMA8GA1UEAwwIdGVzdC1pZHAwHhcNMjYwMjIyMDYxOTEwWhcNMzYwMjIw
MDYxOTEwWjATMREwDwYDVQQDDAh0ZXN0LWlkcDCCASIwDQYJKoZIhvcNAQEBBQAD
ggEPADCCAQoCggEBAORs1ephG0CVlbPA+qd/caAPtLx2b6Pv1mYmRovXfCwe8y8E
4h4lL8iU9mpv76W/fB90ttP6gkHXZ8Ewn0eWWBsS/3VUf0pK51QhckfOKQlubUXq
UKEjl/nQtc69HDjHK8y8MWaN6zRQlo4T04Mq+dK8C31cxH4YN+sNcEyUppqoBUTT
usPyP+jWavT+peOHSqDATpObacwU2GPmZ6ghmfuCMVKWgy6BHeOixak9B+flFwaz
HV3+7UPo4Ffbq+aVAWeoF5mZhK2i/GhtTv+/+2xNdP+fD2QuYG1zL88f63v5yCL3
dmJ3f8oLIte6GMlJ61YxTvteRxN4OtruboDXt4sCAwEAAaNTMFEwHQYDVR0OBBYE
FP/GrEKRzDWlO155kiw6Bs9loxB+MB8GA1UdIwQYMBaAFP/GrEKRzDWlO155kiw6
Bs9loxB+MA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQELBQADggEBAFlX2uuX
A80JlynN3JJRJ5OhpWX8eQZQsmjXengQ1QVMqQ9GamUh4efRSHnkxcevlETnpsZ0
dMV7W37x8eDHqmT07RnjsKSb++3gYgB8wCd2fTj/rm88GAK1mZ9txO8H4MJggN0M
D1eVSgLx0nnt9S8LsA8sRjo/ObLExTtL5Br/F1UkaQT1fANwfJPihNlqTPbDbdVn
AAq2y/6qHeaVpbDc+O4HuWx17HgwpKU4juhrZZzBvcAym5BEHvM2pu9R94tgVEmn
4JMbZ/Y/w8J6ttLEQGPfc+GfSskjNEUX4jyY+TZMq2S2fl/7X2jfNu+tHT+FeG/B
6Jp+vzG3iVyfyH8=
-----END CERTIFICATE-----
)";

// Build a minimal SAMLConfig for testing (signature verification disabled)
static SAMLConfig makeTestConfig() {
    SAMLConfig cfg;
    cfg.sp_entity_id    = "https://myapp.example.com/saml/metadata";
    cfg.sp_acs_url      = "https://myapp.example.com/saml/acs";
    cfg.idp_sso_url     = "https://idp.example.com/sso";
    cfg.idp_entity_id   = "https://test-idp.example.com/metadata";
    cfg.idp_certificate_pem = TEST_IDP_CERT_PEM;
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
    std::chrono::seconds not_on_or_after_offset = std::chrono::seconds(600))
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

    std::string xml =
        R"(<samlp:Response xmlns:samlp="urn:oasis:names:tc:SAML:2.0:protocol")"
        R"( xmlns:saml="urn:oasis:names:tc:SAML:2.0:assertion")"
        " ID=\"_resp001\""
        " Version=\"2.0\""
        " IssueInstant=\"" + issue_instant + "\""
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
        if (i + 1 < bytes.size()) b |= (static_cast<uint32_t>(bytes[i+1]) << 8);
        if (i + 2 < bytes.size()) b |= static_cast<uint32_t>(bytes[i+2]);
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
        if (i + 1 < bytes.size()) b |= (static_cast<uint32_t>(bytes[i+1]) << 8);
        if (i + 2 < bytes.size()) b |= static_cast<uint32_t>(bytes[i+2]);
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

TEST(SAMLAuthenticatorTest, ProcessResponseRejectsEncryptedAssertion) {
    // Even with signatures disabled, a response containing EncryptedAssertion
    // should throw AUTH_NOT_IMPLEMENTED since decryption is not supported.
    SAMLAuthenticator auth(makeTestConfig());

    auto b64 = buildEncryptedAssertionResponseB64();
    try {
        auth.processResponse(b64);
        FAIL() << "Expected AuthException for EncryptedAssertion";
    } catch (const AuthException& e) {
        EXPECT_EQ(e.error().code(), AuthErrorCode::AUTH_NOT_IMPLEMENTED);
    }
}

TEST(SAMLAuthenticatorTest, ProcessResponseRejectsRequireEncryptedAssertionWithPlainAssertion) {
    // When require_encrypted_assertion=true, a plain (unencrypted) Assertion
    // must be rejected with AUTH_NOT_IMPLEMENTED.
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
        EXPECT_EQ(e.error().code(), AuthErrorCode::AUTH_NOT_IMPLEMENTED);
    }
}
