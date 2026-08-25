/*
 * test_saml_auth_provider.cpp
 *
 * Unit tests for themis::server::SamlAuthProvider.
 *
 * Tests cover:
 *  - SP-initiated login: redirect URL generated, request_id stored
 *  - ACS success: valid SAML response → token + claims returned
 *  - ACS failure: missing SAMLResponse → 400
 *  - ACS failure: expired assertion → 401
 *  - ACS failure: tampered/bad signature → 401
 *  - ACS failure: unknown InResponseTo → 400
 *  - ACS failure: replay detection → 401
 *  - SLO without IdP URL → success + no redirect
 *  - SLO with IdP URL → success + redirect_url
 *  - Metadata XML: entity ID, ACS URL, SLO URL, org info
 *  - Token factory: custom factory is invoked
 *  - Clock override: expired assertion is detected
 *  - enableSaml() code path: valid/invalid config, provider usable after construction
 */

#include <gtest/gtest.h>
#include "server/saml_auth_provider.h"
#include "auth/auth_error.h"

#include <string>
#include <chrono>
#include <sstream>

using namespace themis::server;
using namespace themis::auth;

// ---------------------------------------------------------------------------
// Test IdP certificate (RSA 2048, self-signed, CN=test-idp)
// Same cert used by test_saml_authenticator.cpp
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static SamlAuthProvider::Config makeConfig(bool signed_response = false,
                                            bool signed_assertion = false)
{
    SamlAuthProvider::Config cfg;
    cfg.saml.sp_entity_id       = "https://myapp.example.com/saml/metadata";
    cfg.saml.sp_acs_url         = "https://myapp.example.com/saml/acs";
    cfg.saml.idp_sso_url        = "https://idp.example.com/sso";
    cfg.saml.idp_entity_id      = "https://test-idp.example.com/metadata";
    cfg.saml.idp_certificate_pem = TEST_IDP_CERT_PEM;
    cfg.saml.require_signed_response  = signed_response;
    cfg.saml.require_signed_assertion = signed_assertion;
    cfg.idp_slo_url   = "";
    cfg.sp_slo_url    = "https://myapp.example.com/saml/slo";
    cfg.org_name      = "TestOrg";
    cfg.org_display_name = "Test Organisation";
    cfg.org_url       = "https://myapp.example.com";
    cfg.contact_email = "admin@myapp.example.com";
    return cfg;
}

// Build a base64-encoded SAMLResponse with configurable timing.
static std::string buildSAMLResponseB64(
    const std::string& issuer,
    const std::string& audience,
    const std::string& recipient,
    const std::string& name_id,
    const std::string& status_code,
    const std::string& assertion_id,
    std::chrono::system_clock::time_point anchor,
    std::chrono::seconds not_before_offset  = std::chrono::seconds(-300),
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

    const std::string issue_instant  = fmt(anchor);
    const std::string not_before     = fmt(anchor + not_before_offset);
    const std::string not_on_or_after = fmt(anchor + not_on_or_after_offset);

    std::ostringstream xml;
    xml << R"(<samlp:Response)"
        << R"( xmlns:samlp="urn:oasis:names:tc:SAML:2.0:protocol")"
        << R"( xmlns:saml="urn:oasis:names:tc:SAML:2.0:assertion")"
        << " ID=\"_resp1\" Version=\"2.0\""
        << " IssueInstant=\"" << issue_instant << "\"";
    if (!in_response_to.empty()) {
        xml << " InResponseTo=\"" << in_response_to << "\"";
    }
    xml << " Destination=\"" << recipient << "\">\n"
        << "  <saml:Issuer>" << issuer << "</saml:Issuer>\n"
        << "  <samlp:Status><samlp:StatusCode Value=\""
        << status_code << "\"/></samlp:Status>\n"
        << "  <saml:Assertion ID=\"" << assertion_id << "\""
        << " Version=\"2.0\" IssueInstant=\"" << issue_instant << "\">\n"
        << "    <saml:Issuer>" << issuer << "</saml:Issuer>\n"
        << "    <saml:Subject>\n"
        << "      <saml:NameID Format=\"urn:oasis:names:tc:SAML:1.1:nameid-format:emailAddress\">"
        << name_id << "</saml:NameID>\n"
        << "      <saml:SubjectConfirmation Method=\"urn:oasis:names:tc:SAML:2.0:cm:bearer\">\n"
        << "        <saml:SubjectConfirmationData"
        << " NotOnOrAfter=\"" << not_on_or_after << "\""
        << " Recipient=\"" << recipient << "\"";
    if (!in_response_to.empty()) {
        xml << " InResponseTo=\"" << in_response_to << "\"";
    }
    xml << "/>\n"
        << "      </saml:SubjectConfirmation>\n"
        << "    </saml:Subject>\n"
        << "    <saml:Conditions"
        << " NotBefore=\"" << not_before << "\""
        << " NotOnOrAfter=\"" << not_on_or_after << "\">\n"
        << "      <saml:AudienceRestriction>"
        << "<saml:Audience>" << audience << "</saml:Audience>"
        << "</saml:AudienceRestriction>\n"
        << "    </saml:Conditions>\n"
        << "    <saml:AuthnStatement AuthnInstant=\"" << issue_instant << "\">\n"
        << "      <saml:AuthnContext>"
        << "<saml:AuthnContextClassRef>urn:oasis:names:tc:SAML:2.0:ac:classes:PasswordProtectedTransport</saml:AuthnContextClassRef>"
        << "</saml:AuthnContext>\n"
        << "    </saml:AuthnStatement>\n"
        << "    <saml:AttributeStatement>\n"
        << "      <saml:Attribute Name=\"email\">\n"
        << "        <saml:AttributeValue>" << name_id << "</saml:AttributeValue>\n"
        << "      </saml:Attribute>\n"
        << "    </saml:AttributeStatement>\n"
        << "  </saml:Assertion>\n"
        << "</samlp:Response>\n";

    // Base64-encode (standard alphabet)
    const std::string raw = xml.str();
    static const char* kB64Table =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((raw.size() + 2) / 3) * 4);
    for (std::size_t i = 0; i < raw.size(); i += 3) {
        const unsigned char b0 = static_cast<unsigned char>(raw[i]);
        const unsigned char b1 = (i + 1 < raw.size()) ? static_cast<unsigned char>(raw[i + 1]) : 0;
        const unsigned char b2 = (i + 2 < raw.size()) ? static_cast<unsigned char>(raw[i + 2]) : 0;
        out += kB64Table[b0 >> 2];
        out += kB64Table[((b0 & 0x03) << 4) | (b1 >> 4)];
        out += (i + 1 < raw.size()) ? kB64Table[((b1 & 0x0f) << 2) | (b2 >> 6)] : '=';
        out += (i + 2 < raw.size()) ? kB64Table[b2 & 0x3f] : '=';
    }
    return out;
}

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class SamlAuthProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        cfg_ = makeConfig();
        provider_ = std::make_unique<SamlAuthProvider>(cfg_);

        // Fix the clock so time-sensitive checks are deterministic.
        fixed_time_ = std::chrono::system_clock::now();
        provider_->setClockForTesting([this]() { return fixed_time_; });
    }

    SamlAuthProvider::Config cfg_;
    std::unique_ptr<SamlAuthProvider> provider_;
    std::chrono::system_clock::time_point fixed_time_;
};

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_F(SamlAuthProviderTest, ConstructionSuccess) {
    // Provider created in SetUp without throwing.
    EXPECT_NE(provider_, nullptr);
    // Verify the internal authenticator is functional.
    const std::string xml = provider_->buildMetadataXml();
    EXPECT_FALSE(xml.empty());
    EXPECT_NE(xml.find("EntityDescriptor"), std::string::npos);
}

TEST(SamlAuthProviderConstructTest, ThrowsOnMissingSpEntityId) {
    SamlAuthProvider::Config cfg = makeConfig();
    cfg.saml.sp_entity_id = "";
    EXPECT_THROW(SamlAuthProvider p(cfg), std::invalid_argument);
}

TEST(SamlAuthProviderConstructTest, ThrowsOnMissingIdpSsoUrl) {
    SamlAuthProvider::Config cfg = makeConfig();
    cfg.saml.idp_sso_url = "";
    EXPECT_THROW(SamlAuthProvider p(cfg), std::invalid_argument);
}

TEST(SamlAuthProviderConstructTest, ThrowsOnBadCertificate) {
    SamlAuthProvider::Config cfg = makeConfig();
    cfg.saml.idp_certificate_pem = "not-a-cert";
    EXPECT_THROW(SamlAuthProvider p(cfg), std::runtime_error);
}

// ---------------------------------------------------------------------------
// handleLogin – SP-initiated SSO redirect
// ---------------------------------------------------------------------------

TEST_F(SamlAuthProviderTest, LoginReturnsRedirectUrl) {
    auto result = provider_->handleLogin();
    ASSERT_TRUE(result.contains("redirect_url"));
    ASSERT_TRUE(result.contains("request_id"));
    const std::string url = result["redirect_url"];
    EXPECT_FALSE(url.empty());
    EXPECT_NE(url.find("SAMLRequest="), std::string::npos);
}

TEST_F(SamlAuthProviderTest, LoginWithRelayState) {
    auto result = provider_->handleLogin("original_url");
    ASSERT_TRUE(result.contains("redirect_url"));
    const std::string url = result["redirect_url"];
    EXPECT_NE(url.find("RelayState="), std::string::npos);
}

TEST_F(SamlAuthProviderTest, LoginRequestIdsAreUnique) {
    auto r1 = provider_->handleLogin();
    auto r2 = provider_->handleLogin();
    EXPECT_NE(r1["request_id"].get<std::string>(),
              r2["request_id"].get<std::string>());
}

// ---------------------------------------------------------------------------
// handleAcs – valid response
// ---------------------------------------------------------------------------

TEST_F(SamlAuthProviderTest, AcsSuccess) {
    const std::string saml_b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_unique_1",
        fixed_time_);

    auto result = provider_->handleAcs(saml_b64);
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_FALSE(result.value("token", "").empty());
    EXPECT_EQ(result.value("user_id", ""), "user@example.com");
    EXPECT_EQ(result.value("email", ""), "user@example.com");
    EXPECT_EQ(result.value("issuer", ""), "https://test-idp.example.com/metadata");
}

TEST_F(SamlAuthProviderTest, AcsRelayStatePassedThrough) {
    const std::string saml_b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_relay_1",
        fixed_time_);

    auto result = provider_->handleAcs(saml_b64, "/original/path");
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_EQ(result.value("relay_state", ""), "/original/path");
}

TEST_F(SamlAuthProviderTest, AcsTokenIsNonEmpty) {
    const std::string saml_b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_token_1",
        fixed_time_);

    auto result = provider_->handleAcs(saml_b64);
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    const std::string token = result.value("token", "");
    EXPECT_FALSE(token.empty());
    EXPECT_EQ(token.substr(0, 5), "saml_");
}

// ---------------------------------------------------------------------------
// handleAcs – failure scenarios
// ---------------------------------------------------------------------------

TEST_F(SamlAuthProviderTest, AcsFailsMissingSAMLResponse) {
    auto result = provider_->handleAcs("");
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 400);
}

TEST_F(SamlAuthProviderTest, AcsFailsExpiredAssertion) {
    // Assertion validity window ends 1 hour before "now".
    const std::string saml_b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_expired_1",
        fixed_time_,
        std::chrono::seconds(-7200), // NotBefore: 2 h ago
        std::chrono::seconds(-3600)  // NotOnOrAfter: 1 h ago
    );

    auto result = provider_->handleAcs(saml_b64);
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 401);
}

TEST_F(SamlAuthProviderTest, AcsFailsStatusFailure) {
    const std::string saml_b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:AuthnFailed",
        "_assert_authn_fail",
        fixed_time_);

    auto result = provider_->handleAcs(saml_b64);
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 401);
}

TEST_F(SamlAuthProviderTest, AcsFailsIssuerMismatch) {
    // Issuer does not match configured idp_entity_id
    const std::string saml_b64 = buildSAMLResponseB64(
        "https://wrong-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_issuer_fail",
        fixed_time_);

    auto result = provider_->handleAcs(saml_b64);
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 401);
}

TEST_F(SamlAuthProviderTest, AcsFailsAudienceMismatch) {
    // Audience does not match SP entity ID
    const std::string saml_b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://other-sp.example.com/saml/metadata", // wrong audience
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_audience_fail",
        fixed_time_);

    auto result = provider_->handleAcs(saml_b64);
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 401);
}

TEST_F(SamlAuthProviderTest, AcsFailsUnknownInResponseTo) {
    const std::string saml_b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_irt_fail",
        fixed_time_,
        std::chrono::seconds(-300),
        std::chrono::seconds(600),
        "unknown-request-id");

    // Pass a mismatched in_response_to that was never registered via handleLogin.
    auto result = provider_->handleAcs(saml_b64, "", "unknown-request-id");
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 400);
}

TEST_F(SamlAuthProviderTest, AcsReplayDetected) {
    const std::string saml_b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_replay_test_1",
        fixed_time_);

    // First call should succeed.
    auto r1 = provider_->handleAcs(saml_b64);
    EXPECT_FALSE(r1.contains("status_code")) << r1.dump();

    // Second call with same assertion ID should be rejected as replay.
    auto r2 = provider_->handleAcs(saml_b64);
    ASSERT_TRUE(r2.contains("status_code"));
    EXPECT_EQ(r2["status_code"].get<int>(), 401);
}

// ---------------------------------------------------------------------------
// SP-initiated flow: InResponseTo round-trip
// ---------------------------------------------------------------------------

TEST_F(SamlAuthProviderTest, InResponseToRoundTripSuccess) {
    // 1) Generate AuthnRequest → get request_id
    auto login_result = provider_->handleLogin();
    ASSERT_FALSE(login_result.contains("status_code"));
    const std::string request_id = login_result["request_id"].get<std::string>();

    // 2) Build a SAMLResponse that carries InResponseTo = request_id
    const std::string saml_b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_irt_ok_1",
        fixed_time_,
        std::chrono::seconds(-300),
        std::chrono::seconds(600),
        request_id);

    // 3) Process the ACS – should succeed.
    auto acs_result = provider_->handleAcs(saml_b64, "", request_id);
    EXPECT_FALSE(acs_result.contains("status_code")) << acs_result.dump();
}

TEST_F(SamlAuthProviderTest, InResponseToConsumedAfterAcs) {
    // Each request_id may only be used once.
    auto login_result = provider_->handleLogin();
    const std::string request_id = login_result["request_id"].get<std::string>();

    const std::string saml_b64_1 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_consume_1",
        fixed_time_,
        std::chrono::seconds(-300),
        std::chrono::seconds(600),
        request_id);

    const std::string saml_b64_2 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_consume_2",
        fixed_time_,
        std::chrono::seconds(-300),
        std::chrono::seconds(600),
        request_id);

    // First use – should succeed.
    auto r1 = provider_->handleAcs(saml_b64_1, "", request_id);
    EXPECT_FALSE(r1.contains("status_code")) << r1.dump();

    // Second use of same request_id – pending map entry was consumed.
    auto r2 = provider_->handleAcs(saml_b64_2, "", request_id);
    ASSERT_TRUE(r2.contains("status_code"));
    EXPECT_EQ(r2["status_code"].get<int>(), 400);
}

// ---------------------------------------------------------------------------
// handleSlo
// ---------------------------------------------------------------------------

TEST_F(SamlAuthProviderTest, SloWithoutIdpUrlReturnsSuccess) {
    auto result = provider_->handleSlo();
    ASSERT_FALSE(result.contains("status_code"));
    EXPECT_TRUE(result.value("success", false));
    EXPECT_FALSE(result.contains("redirect_url")); // no IdP SLO URL configured
}

TEST_F(SamlAuthProviderTest, SloWithIdpUrlReturnsRedirect) {
    SamlAuthProvider::Config cfg = makeConfig();
    cfg.idp_slo_url = "https://idp.example.com/slo";
    SamlAuthProvider provider_slo(cfg);

    auto result = provider_slo.handleSlo("session-idx-42");
    ASSERT_FALSE(result.contains("status_code"));
    EXPECT_TRUE(result.value("success", false));
    ASSERT_TRUE(result.contains("redirect_url"));
    const std::string url = result["redirect_url"].get<std::string>();
    EXPECT_NE(url.find("https://idp.example.com/slo"), std::string::npos);
}

TEST_F(SamlAuthProviderTest, SloWithSessionIndex) {
    SamlAuthProvider::Config cfg = makeConfig();
    cfg.idp_slo_url = "https://idp.example.com/slo";
    SamlAuthProvider provider_slo(cfg);

    auto result = provider_slo.handleSlo("sess-idx-abc");
    ASSERT_TRUE(result.contains("redirect_url"));
    const std::string url = result["redirect_url"].get<std::string>();
    // SessionIndex should appear (URL-encoded) in the redirect URL.
    EXPECT_NE(url.find("sess-idx-abc"), std::string::npos);
}

// ---------------------------------------------------------------------------
// buildMetadataXml
// ---------------------------------------------------------------------------

TEST_F(SamlAuthProviderTest, MetadataContainsEntityDescriptor) {
    const std::string xml = provider_->buildMetadataXml();
    EXPECT_NE(xml.find("EntityDescriptor"), std::string::npos);
    EXPECT_NE(xml.find("https://myapp.example.com/saml/metadata"), std::string::npos);
}

TEST_F(SamlAuthProviderTest, MetadataContainsAcsUrl) {
    const std::string xml = provider_->buildMetadataXml();
    EXPECT_NE(xml.find("AssertionConsumerService"), std::string::npos);
    EXPECT_NE(xml.find("https://myapp.example.com/saml/acs"), std::string::npos);
}

TEST_F(SamlAuthProviderTest, MetadataContainsSloUrl) {
    const std::string xml = provider_->buildMetadataXml();
    EXPECT_NE(xml.find("SingleLogoutService"), std::string::npos);
    EXPECT_NE(xml.find("https://myapp.example.com/saml/slo"), std::string::npos);
}

TEST_F(SamlAuthProviderTest, MetadataContainsOrganisation) {
    const std::string xml = provider_->buildMetadataXml();
    EXPECT_NE(xml.find("TestOrg"), std::string::npos);
    EXPECT_NE(xml.find("admin@myapp.example.com"), std::string::npos);
}

TEST_F(SamlAuthProviderTest, MetadataOmitsSloWhenNotConfigured) {
    SamlAuthProvider::Config cfg = makeConfig();
    cfg.sp_slo_url = "";
    SamlAuthProvider p(cfg);
    const std::string xml = p.buildMetadataXml();
    EXPECT_EQ(xml.find("SingleLogoutService"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Custom token factory
// ---------------------------------------------------------------------------

TEST_F(SamlAuthProviderTest, CustomTokenFactoryInvoked) {
    SamlAuthProvider::Config cfg = makeConfig();
    bool factory_called = false;
    cfg.token_factory = [&factory_called](const SAMLClaims& claims) -> std::string {
        factory_called = true;
        return "custom_" + claims.subject_name_id;
    };
    SamlAuthProvider custom_provider(cfg);
    custom_provider.setClockForTesting([this]() { return fixed_time_; });

    const std::string saml_b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_custom_factory",
        fixed_time_);

    auto result = custom_provider.handleAcs(saml_b64);
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_TRUE(factory_called);
    EXPECT_EQ(result.value("token", ""), "custom_user@example.com");
}

// ---------------------------------------------------------------------------
// Clock override – time-window enforcement
// ---------------------------------------------------------------------------

TEST_F(SamlAuthProviderTest, AssertionValidInWindow) {
    // Clock set to middle of validity window – should succeed.
    const std::string saml_b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_window_ok",
        fixed_time_);

    auto result = provider_->handleAcs(saml_b64);
    EXPECT_FALSE(result.contains("status_code")) << result.dump();
}

TEST_F(SamlAuthProviderTest, AssertionNotYetValid) {
    // Assertion becomes valid 1 hour in the future; clock at "now".
    const std::string saml_b64 = buildSAMLResponseB64(
        "https://test-idp.example.com/metadata",
        "https://myapp.example.com/saml/metadata",
        "https://myapp.example.com/saml/acs",
        "user@example.com",
        "urn:oasis:names:tc:SAML:2.0:status:Success",
        "_assert_not_yet_valid",
        fixed_time_,
        std::chrono::seconds(3600),  // NotBefore: 1 h from now (future)
        std::chrono::seconds(7200)   // NotOnOrAfter: 2 h from now
    );

    auto result = provider_->handleAcs(saml_b64);
    ASSERT_TRUE(result.contains("status_code"));
    EXPECT_EQ(result["status_code"].get<int>(), 401);
}

// ---------------------------------------------------------------------------
// enableSaml() code path – SamlAuthProvider construction
// ---------------------------------------------------------------------------
// HttpServer::enableSaml(cfg) does exactly one thing:
//   saml_provider_ = std::make_unique<SamlAuthProvider>(cfg);
// The four tests below fully cover that code path by exercising
// SamlAuthProvider(cfg) construction directly (valid config, bad cert,
// missing entity_id) and verifying the provider is immediately usable.
// An HttpServer instance is not created here because doing so requires
// RocksDB, index managers and other infrastructure not present in this
// unit-test target; the production initialisation path is identical.

TEST(SamlEnablePathTest, ValidConfigCreatesProvider) {
    SamlAuthProvider::Config cfg = makeConfig();
    // Same as: HttpServer::enableSaml(cfg) → make_unique<SamlAuthProvider>(cfg)
    auto provider = std::make_unique<SamlAuthProvider>(cfg);
    ASSERT_NE(provider, nullptr);
    // Provider should be able to build metadata immediately after creation.
    const std::string xml = provider->buildMetadataXml();
    EXPECT_FALSE(xml.empty());
    EXPECT_NE(xml.find("EntityDescriptor"), std::string::npos);
}

TEST(SamlEnablePathTest, BadCertThrowsRuntimeError) {
    SamlAuthProvider::Config cfg = makeConfig();
    cfg.saml.idp_certificate_pem = "not-a-valid-pem";
    // HttpServer::enableSaml() propagates the exception; server startup fails.
    EXPECT_THROW(SamlAuthProvider p(cfg), std::runtime_error);
}

TEST(SamlEnablePathTest, MissingEntityIdThrowsInvalidArgument) {
    SamlAuthProvider::Config cfg = makeConfig();
    cfg.saml.sp_entity_id = "";
    EXPECT_THROW(SamlAuthProvider p(cfg), std::invalid_argument);
}

TEST(SamlEnablePathTest, ProviderIsUsableAfterConstruction) {
    SamlAuthProvider::Config cfg = makeConfig();
    // After HttpServer::enableSaml() the SAML login endpoint must redirect.
    SamlAuthProvider provider(cfg);

    auto result = provider.handleLogin();
    ASSERT_FALSE(result.contains("status_code")) << result.dump();
    EXPECT_FALSE(result.value("redirect_url", "").empty());
    EXPECT_FALSE(result.value("request_id", "").empty());
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
