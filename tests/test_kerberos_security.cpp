#include <gtest/gtest.h>
#include "auth/kerberos_security.h"

using namespace themis::auth;

/**
 * @brief Test default configuration
 */
TEST(KerberosSecurityValidatorTest, DefaultConfiguration) {
    KerberosSecurityValidator::Config config;
    
    EXPECT_FALSE(config.enable_channel_bindings);
    EXPECT_TRUE(config.strict_asn1_validation);
    EXPECT_TRUE(config.verify_service_target);
    EXPECT_TRUE(config.validate_token_structure);
    EXPECT_EQ(config.max_token_depth, 10);
    EXPECT_EQ(config.max_sequence_length, 10000);
}

/**
 * @brief Test with channel bindings configuration
 */
TEST(KerberosSecurityValidatorTest, WithChannelBindings) {
    auto config = KerberosSecurityValidator::withChannelBindings(
        KerberosSecurityValidator::ChannelBindingType::TLS_SERVER_ENDPOINT
    );
    
    EXPECT_TRUE(config.enable_channel_bindings);
    EXPECT_EQ(config.binding_type, 
             KerberosSecurityValidator::ChannelBindingType::TLS_SERVER_ENDPOINT);
    EXPECT_TRUE(config.strict_asn1_validation);
}

/**
 * @brief Test strict validation configuration
 */
TEST(KerberosSecurityValidatorTest, StrictValidation) {
    auto config = KerberosSecurityValidator::strictValidation();
    
    EXPECT_TRUE(config.enable_channel_bindings);
    EXPECT_TRUE(config.strict_asn1_validation);
    EXPECT_TRUE(config.verify_service_target);
    EXPECT_TRUE(config.require_mutual_auth);
    EXPECT_TRUE(config.require_integrity);
    EXPECT_TRUE(config.reject_expired_tickets);
}

/**
 * @brief Test for service configuration
 */
TEST(KerberosSecurityValidatorTest, ForService) {
    std::string service_principal = "HTTP/server.example.com@REALM.COM";
    auto config = KerberosSecurityValidator::forService(service_principal);
    
    EXPECT_TRUE(config.verify_service_target);
    EXPECT_EQ(config.expected_service_principal, service_principal);
    EXPECT_TRUE(config.strict_asn1_validation);
}

/**
 * @brief Test validator initialization
 */
TEST(KerberosSecurityValidatorTest, Initialization) {
    auto config = KerberosSecurityValidator::strictValidation();
    
    EXPECT_NO_THROW(KerberosSecurityValidator validator(config));
}

/**
 * @brief Test channel binding type enumeration
 */
TEST(KerberosSecurityValidatorTest, ChannelBindingTypes) {
    KerberosSecurityValidator::Config config;
    
    config.binding_type = KerberosSecurityValidator::ChannelBindingType::NONE;
    EXPECT_EQ(config.binding_type, KerberosSecurityValidator::ChannelBindingType::NONE);
    
    config.binding_type = KerberosSecurityValidator::ChannelBindingType::TLS_UNIQUE;
    EXPECT_EQ(config.binding_type, KerberosSecurityValidator::ChannelBindingType::TLS_UNIQUE);
    
    config.binding_type = KerberosSecurityValidator::ChannelBindingType::TLS_SERVER_ENDPOINT;
    EXPECT_EQ(config.binding_type, KerberosSecurityValidator::ChannelBindingType::TLS_SERVER_ENDPOINT);
    
    config.binding_type = KerberosSecurityValidator::ChannelBindingType::TLS_EXPORTER;
    EXPECT_EQ(config.binding_type, KerberosSecurityValidator::ChannelBindingType::TLS_EXPORTER);
}

/**
 * @brief Test ASN.1 validation with empty data
 */
TEST(KerberosSecurityValidatorTest, ASN1ValidationEmpty) {
    auto config = KerberosSecurityValidator::strictValidation();
    KerberosSecurityValidator validator(config);
    
    std::vector<uint8_t> empty_data;
    
    // Empty data should be considered invalid
    EXPECT_FALSE(validator.validateASN1Structure(empty_data));
}

/**
 * @brief Test ASN.1 validation with simple valid sequence
 */
TEST(KerberosSecurityValidatorTest, ASN1ValidationSimpleSequence) {
    auto config = KerberosSecurityValidator::strictValidation();
    KerberosSecurityValidator validator(config);
    
    // Simple ASN.1 SEQUENCE with one INTEGER
    std::vector<uint8_t> valid_sequence = {
        0x30, 0x03,  // SEQUENCE, length 3
        0x02, 0x01,  // INTEGER, length 1
        0x05         // value 5
    };
    
    EXPECT_TRUE(validator.validateASN1Structure(valid_sequence));
}

/**
 * @brief Test ASN.1 validation with depth limit
 */
TEST(KerberosSecurityValidatorTest, ASN1DepthLimit) {
    KerberosSecurityValidator::Config config;
    config.strict_asn1_validation = true;
    config.max_token_depth = 2;  // Very shallow for testing
    
    KerberosSecurityValidator validator(config);
    
    // Deeply nested structure (3 levels)
    std::vector<uint8_t> deep_sequence = {
        0x30, 0x08,  // SEQUENCE level 1
        0x30, 0x06,  // SEQUENCE level 2
        0x30, 0x04,  // SEQUENCE level 3 (exceeds limit)
        0x02, 0x02,  // INTEGER
        0x00, 0x01   // value
    };
    
    // Should fail due to depth limit
    EXPECT_FALSE(validator.validateASN1Structure(deep_sequence));
}

/**
 * @brief Test ASN.1 validation with length limit
 */
TEST(KerberosSecurityValidatorTest, ASN1LengthLimit) {
    KerberosSecurityValidator::Config config;
    config.strict_asn1_validation = true;
    config.max_sequence_length = 100;  // Small limit
    
    KerberosSecurityValidator validator(config);
    
    // Create a sequence claiming to be longer than allowed
    std::vector<uint8_t> long_sequence = {
        0x30, 0x81, 0xFF  // SEQUENCE, long form length = 255 (exceeds limit)
    };
    
    // Should fail due to length limit
    EXPECT_FALSE(validator.validateASN1Structure(long_sequence));
}

/**
 * @brief Test clock skew configuration
 */
TEST(KerberosSecurityValidatorTest, ClockSkewConfiguration) {
    KerberosSecurityValidator::Config config;
    
    config.max_clock_skew_seconds = 60;  // 1 minute
    EXPECT_EQ(config.max_clock_skew_seconds, 60);
    
    config.max_clock_skew_seconds = 600;  // 10 minutes
    EXPECT_EQ(config.max_clock_skew_seconds, 600);
}

/**
 * @brief Test security flags configuration
 */
TEST(KerberosSecurityValidatorTest, SecurityFlags) {
    KerberosSecurityValidator::Config config;
    
    config.require_mutual_auth = true;
    EXPECT_TRUE(config.require_mutual_auth);
    
    config.require_integrity = true;
    EXPECT_TRUE(config.require_integrity);
    
    config.require_confidentiality = true;
    EXPECT_TRUE(config.require_confidentiality);
}

/**
 * @brief Test service principal extraction
 */
TEST(KerberosSecurityValidatorTest, ExtractServicePrincipal) {
    std::string expected_principal = "HTTP/server.example.com@REALM.COM";
    auto config = KerberosSecurityValidator::forService(expected_principal);
    KerberosSecurityValidator validator(config);
    
    // Create dummy token (in real scenario, would be valid GSSAPI token)
    std::vector<uint8_t> token_data(100, 0);
    
    std::string extracted = validator.extractServicePrincipal(token_data);
    
    // In this simplified implementation, returns expected principal
    EXPECT_EQ(extracted, expected_principal);
}

/**
 * @brief Test token info extraction
 */
TEST(KerberosSecurityValidatorTest, GetTokenInfo) {
    auto config = KerberosSecurityValidator::strictValidation();
    config.expected_service_principal = "HTTP/server@REALM";
    
    KerberosSecurityValidator validator(config);
    
    std::vector<uint8_t> token_data(100, 0);
    auto info = validator.getTokenInfo(token_data);
    
    EXPECT_FALSE(info.service_principal.empty());
    EXPECT_FALSE(info.client_principal.empty());
    EXPECT_GT(info.end_time, 0);
}

/**
 * @brief Test channel binding generator - TLS certificate
 */
TEST(ChannelBindingGeneratorTest, TLSCertificate) {
    std::vector<uint8_t> cert_data(256, 0xAB);  // Dummy certificate
    
    auto binding = ChannelBindingGenerator::generateFromTLSCertificate(cert_data);
    
    EXPECT_FALSE(binding.empty());
    EXPECT_EQ(binding.size(), 32);  // SHA256 hash size
}

/**
 * @brief Test channel binding generator - TLS finished
 */
TEST(ChannelBindingGeneratorTest, TLSFinished) {
    std::vector<uint8_t> finished_message = {0x01, 0x02, 0x03, 0x04};
    
    auto binding = ChannelBindingGenerator::generateFromTLSFinished(finished_message);
    
    EXPECT_EQ(binding, finished_message);
}

/**
 * @brief Test channel binding generator - TLS exporter
 */
TEST(ChannelBindingGeneratorTest, TLSExporter) {
    std::vector<uint8_t> exporter_value = {0x0A, 0x0B, 0x0C};
    
    auto binding = ChannelBindingGenerator::generateFromTLSExporter(exporter_value);
    
    EXPECT_EQ(binding, exporter_value);
}

/**
 * @brief Test channel binding formatting
 */
TEST(ChannelBindingGeneratorTest, FormatChannelBinding) {
    std::vector<uint8_t> init_addr = {192, 168, 1, 1};
    std::vector<uint8_t> acc_addr = {192, 168, 1, 2};
    std::vector<uint8_t> app_data = {0x01, 0x02, 0x03};
    
    auto formatted = ChannelBindingGenerator::formatChannelBinding(
        init_addr, acc_addr, app_data
    );
    
    EXPECT_FALSE(formatted.empty());
    // Format includes: 4 bytes (init type) + 4 bytes (init len) + init_addr +
    //                  4 bytes (acc type) + 4 bytes (acc len) + acc_addr +
    //                  4 bytes (app len) + app_data
    size_t expected_size = 4 + 4 + init_addr.size() + 4 + 4 + acc_addr.size() + 4 + app_data.size();
    EXPECT_EQ(formatted.size(), expected_size);
}

/**
 * @brief Test empty channel binding formatting
 */
TEST(ChannelBindingGeneratorTest, EmptyChannelBinding) {
    std::vector<uint8_t> empty;
    
    auto formatted = ChannelBindingGenerator::formatChannelBinding(empty, empty, empty);
    
    // Should still have header fields
    EXPECT_FALSE(formatted.empty());
}

/**
 * @brief Test config accessor
 */
TEST(KerberosSecurityValidatorTest, ConfigAccessor) {
    auto config = KerberosSecurityValidator::strictValidation();
    KerberosSecurityValidator validator(config);
    
    const auto& retrieved_config = validator.getConfig();
    
    EXPECT_EQ(retrieved_config.enable_channel_bindings, config.enable_channel_bindings);
    EXPECT_EQ(retrieved_config.strict_asn1_validation, config.strict_asn1_validation);
}

/**
 * @brief Test ASN.1 validation with invalid tag
 */
TEST(KerberosSecurityValidatorTest, ASN1InvalidTag) {
    auto config = KerberosSecurityValidator::strictValidation();
    KerberosSecurityValidator validator(config);
    
    // Invalid ASN.1: length extends beyond data
    std::vector<uint8_t> invalid_data = {
        0x30, 0xFF  // SEQUENCE claiming length 255, but data is too short
    };
    
    EXPECT_FALSE(validator.validateASN1Structure(invalid_data));
}

/**
 * @brief Test multiple channel binding types in same config
 */
TEST(KerberosSecurityValidatorTest, ChannelBindingConfiguration) {
    auto config_tls_unique = KerberosSecurityValidator::withChannelBindings(
        KerberosSecurityValidator::ChannelBindingType::TLS_UNIQUE
    );
    EXPECT_EQ(config_tls_unique.binding_type, 
             KerberosSecurityValidator::ChannelBindingType::TLS_UNIQUE);
    
    auto config_tls_endpoint = KerberosSecurityValidator::withChannelBindings(
        KerberosSecurityValidator::ChannelBindingType::TLS_SERVER_ENDPOINT
    );
    EXPECT_EQ(config_tls_endpoint.binding_type, 
             KerberosSecurityValidator::ChannelBindingType::TLS_SERVER_ENDPOINT);
}

/**
 * @brief Test reject expired tickets configuration
 */
TEST(KerberosSecurityValidatorTest, RejectExpiredTickets) {
    KerberosSecurityValidator::Config config;
    
    config.reject_expired_tickets = true;
    EXPECT_TRUE(config.reject_expired_tickets);
    
    config.reject_expired_tickets = false;
    EXPECT_FALSE(config.reject_expired_tickets);
}

/**
 * @brief Test validation with different token sizes
 */
TEST(KerberosSecurityValidatorTest, TokenSizeVariations) {
    auto config = KerberosSecurityValidator::strictValidation();
    config.verify_service_target = false;  // Skip for this test
    
    KerberosSecurityValidator validator(config);
    
    // Small token
    std::vector<uint8_t> small_token = {0x30, 0x02, 0x02, 0x00};
    EXPECT_TRUE(validator.validateASN1Structure(small_token));
    
    // Medium token
    std::vector<uint8_t> medium_token(100);
    medium_token[0] = 0x30;  // SEQUENCE
    medium_token[1] = 0x60;  // length 96
    EXPECT_TRUE(validator.validateASN1Structure(medium_token));
}

// ============================================================================
// Tests for GSSAPI/KRB5 DER parsing (stubs #139, #156, #191)
// ============================================================================

namespace {

// Build a minimal but structurally valid GSSAPI KRB5 AP-REQ token.
// Ticket sname = "HTTP/testhost" @ realm "TEST.REALM"
// The resulting token is cleartext-only (no real encryption) and is just
// large enough for the parser helpers to traverse.
static std::vector<uint8_t> buildMinimalKrb5ApReqToken(
    const std::string& service_name,   // e.g. "HTTP"
    const std::string& host_name,      // e.g. "testhost"
    const std::string& realm,          // e.g. "TEST.REALM"
    bool mutual_required = false)
{
    // Helper lambdas to DER-encode length and build TLVs.
    auto encLen = [](size_t n, std::vector<uint8_t>& out) {
        if (n < 128) {
            out.push_back(static_cast<uint8_t>(n));
        } else if (n < 256) {
            out.push_back(0x81);
            out.push_back(static_cast<uint8_t>(n));
        } else {
            out.push_back(0x82);
            out.push_back(static_cast<uint8_t>(n >> 8));
            out.push_back(static_cast<uint8_t>(n));
        }
    };
    auto tlv = [&encLen](uint8_t tag, const std::vector<uint8_t>& val) {
        std::vector<uint8_t> r;
        r.push_back(tag);
        encLen(val.size(), r);
        r.insert(r.end(), val.begin(), val.end());
        return r;
    };
    auto wrap = [&tlv](uint8_t tag, const std::vector<uint8_t>& inner) {
        return tlv(tag, inner);
    };
    auto str2gs = [&tlv](const std::string& s) {
        std::vector<uint8_t> sv(s.begin(), s.end());
        return tlv(0x1B, sv); // GeneralString
    };
    auto intTlv = [&tlv](uint32_t v) {
        std::vector<uint8_t> iv;
        iv.push_back(0x02); // INTEGER
        iv.push_back(0x01);
        iv.push_back(static_cast<uint8_t>(v));
        return iv;
    };

    // PrincipalName: SEQUENCE { [0] name-type=1, [1] SEQUENCE OF {svc, host} }
    auto nameType = wrap(0xA0, intTlv(1));  // NT-SRV-HST
    auto nameStrings = [&]() {
        std::vector<uint8_t> combined;
        auto s1 = str2gs(service_name);
        auto s2 = str2gs(host_name);
        combined.insert(combined.end(), s1.begin(), s1.end());
        combined.insert(combined.end(), s2.begin(), s2.end());
        std::vector<uint8_t> outer;
        outer.push_back(0x30); // SEQUENCE
        encLen(combined.size(), outer);
        outer.insert(outer.end(), combined.begin(), combined.end());
        return outer;
    }();
    auto nameStringWrapped = wrap(0xA1, nameStrings);
    std::vector<uint8_t> pnSeqContent;
    pnSeqContent.insert(pnSeqContent.end(), nameType.begin(), nameType.end());
    pnSeqContent.insert(pnSeqContent.end(), nameStringWrapped.begin(), nameStringWrapped.end());
    auto pnSeq = tlv(0x30, pnSeqContent);  // PrincipalName SEQUENCE
    auto snameWrapped = wrap(0xA2, pnSeq); // [2] sname

    // Realm [1]
    auto realmWrapped = wrap(0xA1, str2gs(realm));

    // tkt-vno [0]
    auto tktVno = wrap(0xA0, intTlv(5));

    // enc-part [3] — dummy minimal EncryptedData
    std::vector<uint8_t> encPartSeq = {0x30, 0x05, 0xA0, 0x03, 0x02, 0x01, 0x11};
    auto encPartWrapped = wrap(0xA3, encPartSeq);

    // Assemble Ticket SEQUENCE content
    std::vector<uint8_t> ticketSeqContent;
    ticketSeqContent.insert(ticketSeqContent.end(), tktVno.begin(), tktVno.end());
    ticketSeqContent.insert(ticketSeqContent.end(), realmWrapped.begin(), realmWrapped.end());
    ticketSeqContent.insert(ticketSeqContent.end(), snameWrapped.begin(), snameWrapped.end());
    ticketSeqContent.insert(ticketSeqContent.end(), encPartWrapped.begin(), encPartWrapped.end());
    auto ticketSeq = tlv(0x30, ticketSeqContent); // SEQUENCE
    auto ticketApp = wrap(0x61, ticketSeq);        // [APPLICATION 1]

    // AP-REQ fields
    auto pvno    = wrap(0xA0, intTlv(5));   // [0] pvno=5
    auto msgType = wrap(0xA1, intTlv(14));  // [1] msg-type=14

    // ap-options [2] BIT STRING — 5 bytes: 0x03 0x05 0x00 <4 flag bytes>
    uint32_t opts = mutual_required ? 0x20000000u : 0u;
    std::vector<uint8_t> bsContent = {
        0x03, 0x05, 0x00,
        static_cast<uint8_t>(opts >> 24),
        static_cast<uint8_t>(opts >> 16),
        static_cast<uint8_t>(opts >>  8),
        static_cast<uint8_t>(opts)
    };
    auto apOptions = wrap(0xA2, bsContent);

    // ticket [3]
    auto ticketWrapped = wrap(0xA3, ticketApp);

    // authenticator [4] — dummy minimal EncryptedData
    std::vector<uint8_t> authSeq = {0x30, 0x05, 0xA0, 0x03, 0x02, 0x01, 0x12};
    auto authWrapped = wrap(0xA4, authSeq);

    // AP-REQ SEQUENCE content
    std::vector<uint8_t> apReqSeqContent;
    apReqSeqContent.insert(apReqSeqContent.end(), pvno.begin(), pvno.end());
    apReqSeqContent.insert(apReqSeqContent.end(), msgType.begin(), msgType.end());
    apReqSeqContent.insert(apReqSeqContent.end(), apOptions.begin(), apOptions.end());
    apReqSeqContent.insert(apReqSeqContent.end(), ticketWrapped.begin(), ticketWrapped.end());
    apReqSeqContent.insert(apReqSeqContent.end(), authWrapped.begin(), authWrapped.end());
    auto apReqSeq = tlv(0x30, apReqSeqContent);
    auto apReqApp = wrap(0x6E, apReqSeq);   // [APPLICATION 14] AP-REQ

    // GSSAPI outer wrapper: [APPLICATION 0] CONSTRUCTED = 0x60
    // KRB5 OID + 2-byte token ID (0x01 0x00) + AP-REQ
    std::vector<uint8_t> gssInner = {
        0x06, 0x09,  // OID tag + length=9
        0x2A, 0x86, 0x48, 0x86, 0xF7, 0x12, 0x01, 0x02, 0x02, // KRB5 OID
        0x01, 0x00   // AP-REQ token ID
    };
    gssInner.insert(gssInner.end(), apReqApp.begin(), apReqApp.end());

    std::vector<uint8_t> token;
    token.push_back(0x60); // APPLICATION 0 CONSTRUCTED
    encLen(gssInner.size(), token);
    token.insert(token.end(), gssInner.begin(), gssInner.end());
    return token;
}

} // anonymous test namespace

/**
 * @brief Test extractServicePrincipal with a real KRB5 AP-REQ token
 */
TEST(KerberosSecurityValidatorTest, ExtractServicePrincipalFromRealToken) {
    auto config = KerberosSecurityValidator::forService("HTTP/realhost@TEST.REALM");
    KerberosSecurityValidator validator(config);

    auto token = buildMinimalKrb5ApReqToken("HTTP", "realhost", "TEST.REALM");
    std::string principal = validator.extractServicePrincipal(token);

    // Parser should find "HTTP/realhost@TEST.REALM" from the ticket sname
    EXPECT_EQ(principal, "HTTP/realhost@TEST.REALM");
}

/**
 * @brief Test extractServicePrincipal falls back for non-KRB5 tokens
 */
TEST(KerberosSecurityValidatorTest, ExtractServicePrincipalFallbackOnBadToken) {
    const std::string expected = "HTTP/server.example.com@REALM.COM";
    auto config = KerberosSecurityValidator::forService(expected);
    KerberosSecurityValidator validator(config);

    // Dummy token (not a GSSAPI/KRB5 structure)
    std::vector<uint8_t> bad_token(100, 0);
    EXPECT_EQ(validator.extractServicePrincipal(bad_token), expected);
}

/**
 * @brief Test getTokenInfo extracts realm and sname from real KRB5 token
 */
TEST(KerberosSecurityValidatorTest, GetTokenInfoFromRealToken) {
    KerberosSecurityValidator::Config config;
    config.verify_service_target       = false;
    config.validate_token_structure    = false;
    config.reject_expired_tickets      = false;
    KerberosSecurityValidator validator(config);

    auto token = buildMinimalKrb5ApReqToken("HTTP", "myhost", "MY.REALM");
    auto info  = validator.getTokenInfo(token);

    EXPECT_EQ(info.service_principal, "HTTP/myhost@MY.REALM");
    EXPECT_EQ(info.realm,             "MY.REALM");
    EXPECT_FALSE(info.client_principal.empty());
    EXPECT_GT(info.end_time, 0);
}

/**
 * @brief Test getTokenInfo has_mutual_auth reflects AP-REQ ap-options
 */
TEST(KerberosSecurityValidatorTest, GetTokenInfoMutualAuthFlag) {
    KerberosSecurityValidator::Config config;
    config.verify_service_target    = false;
    config.validate_token_structure = false;
    config.reject_expired_tickets   = false;
    KerberosSecurityValidator validator(config);

    // Token without mutual-required flag
    auto token_no_mutual = buildMinimalKrb5ApReqToken("SVC", "host", "REALM", false);
    auto info_no = validator.getTokenInfo(token_no_mutual);
    EXPECT_FALSE(info_no.has_mutual_auth);

    // Token with mutual-required flag
    auto token_mutual = buildMinimalKrb5ApReqToken("SVC", "host", "REALM", true);
    auto info_yes = validator.getTokenInfo(token_mutual);
    EXPECT_TRUE(info_yes.has_mutual_auth);
}

/**
 * @brief Test verifyChannelBinding fails for non-KRB5 tokens (fail-closed)
 */
TEST(KerberosSecurityValidatorTest, VerifyChannelBindingFailClosedForBadToken) {
    auto config = KerberosSecurityValidator::withChannelBindings(
        KerberosSecurityValidator::ChannelBindingType::TLS_SERVER_ENDPOINT);
    KerberosSecurityValidator validator(config);

    std::vector<uint8_t> garbage_token(50, 0xAB);
    std::vector<uint8_t> binding(32, 0x01);

    // Must reject non-KRB5 token fail-closed
    EXPECT_FALSE(validator.verifyChannelBinding(garbage_token, binding));
}

/**
 * @brief Test verifyChannelBinding accepts valid KRB5 token structurally
 */
TEST(KerberosSecurityValidatorTest, VerifyChannelBindingAcceptsValidKrb5) {
    auto config = KerberosSecurityValidator::withChannelBindings(
        KerberosSecurityValidator::ChannelBindingType::TLS_SERVER_ENDPOINT);
    KerberosSecurityValidator validator(config);

    auto token = buildMinimalKrb5ApReqToken("HTTP", "server", "REALM.COM");
    std::vector<uint8_t> binding(32, 0x42);

    // Structural check passes for a valid KRB5 AP-REQ
    EXPECT_TRUE(validator.verifyChannelBinding(token, binding));
}

/**
 * @brief Test verifyChannelBinding passes when binding is empty (no binding requested)
 */
TEST(KerberosSecurityValidatorTest, VerifyChannelBindingEmptyAlwaysPasses) {
    auto config = KerberosSecurityValidator::strictValidation();
    KerberosSecurityValidator validator(config);

    std::vector<uint8_t> garbage_token(50, 0);
    std::vector<uint8_t> empty_binding;

    EXPECT_TRUE(validator.verifyChannelBinding(garbage_token, empty_binding));
}
