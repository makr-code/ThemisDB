/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_kerberos_security.cpp                         ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:26:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     387                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
