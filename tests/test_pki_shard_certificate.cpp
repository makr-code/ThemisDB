#include <gtest/gtest.h>
#include "sharding/pki_shard_certificate.h"
#include <fstream>

using namespace themis::sharding;

// Note: These tests validate the OID parsing infrastructure
// Full integration tests would require actual test certificates with custom OIDs

TEST(PKIShardCertificateTest, ParseCertificateStructure) {
    // Test the structure is defined correctly
    ShardCertificateInfo info;
    info.shard_id = "shard_001";
    info.datacenter = "dc1";
    info.capabilities = {"read", "write", "replicate"};
    
    EXPECT_TRUE(info.hasCapability("read"));
    EXPECT_TRUE(info.hasCapability("write"));
    EXPECT_FALSE(info.hasCapability("admin"));
}

TEST(PKIShardCertificateTest, ValidateShardCertificate) {
    ShardCertificateInfo info;
    info.shard_id = "shard_001";
    info.not_before = "2025-01-01";
    info.not_after = "2026-01-01";
    info.capabilities = {"read", "write"};
    info.token_range_start = 0;
    info.token_range_end = 0x7FFFFFFFFFFFFFFFULL;
    
    EXPECT_TRUE(PKIShardCertificate::validateShardCertificate(info));
}

TEST(PKIShardCertificateTest, ValidateInvalidCertificate) {
    ShardCertificateInfo info;
    // Missing shard_id
    info.not_before = "2025-01-01";
    info.not_after = "2026-01-01";
    info.capabilities = {"read"};
    
    EXPECT_FALSE(PKIShardCertificate::validateShardCertificate(info));
}

TEST(PKIShardCertificateTest, ValidateNoCapabilities) {
    ShardCertificateInfo info;
    info.shard_id = "shard_001";
    info.not_before = "2025-01-01";
    info.not_after = "2026-01-01";
    // No capabilities
    
    EXPECT_FALSE(PKIShardCertificate::validateShardCertificate(info));
}

TEST(PKIShardCertificateTest, ValidateInvalidTokenRange) {
    ShardCertificateInfo info;
    info.shard_id = "shard_001";
    info.not_before = "2025-01-01";
    info.not_after = "2026-01-01";
    info.capabilities = {"read"};
    info.token_range_start = 1000;
    info.token_range_end = 100;  // Invalid: start > end
    
    EXPECT_FALSE(PKIShardCertificate::validateShardCertificate(info));
}

// ============================================================================
// New tests for OID parsing and validation modes
// ============================================================================

TEST(PKIShardCertificateTest, OIDRegistryConstants) {
    // Test that OID constants are defined correctly
    EXPECT_STREQ(OIDRegistry::COMPANY_ID, "1.3.6.1.4.1.99999");
    EXPECT_STREQ(OIDRegistry::SHARD_ID_OID, "1.3.6.1.4.1.99999.1.1");
    EXPECT_STREQ(OIDRegistry::REGION_OID, "1.3.6.1.4.1.99999.1.2");
    EXPECT_STREQ(OIDRegistry::ROLE_OID, "1.3.6.1.4.1.99999.1.3");
    EXPECT_STREQ(OIDRegistry::NODE_AUTH_EKU, "1.3.6.1.4.1.99999.2.1");
}

TEST(PKIShardCertificateTest, ValidationModeStrict) {
    // Test STRICT mode: requires OID-based extraction
    ShardCertificateInfo info;
    info.shard_id = "shard_001";
    info.not_before = "2025-01-01";
    info.not_after = "2026-01-01";
    info.capabilities = {"read", "write"};
    info.token_range_start = 0;
    info.token_range_end = 0x7FFFFFFFFFFFFFFFULL;
    
    // With shard_id_from_oid = false, STRICT mode should fail
    info.shard_id_from_oid = false;
    EXPECT_FALSE(PKIShardCertificate::validateWithMode(info, ValidationMode::STRICT));
    
    // With shard_id_from_oid = true, STRICT mode should pass
    info.shard_id_from_oid = true;
    EXPECT_TRUE(PKIShardCertificate::validateWithMode(info, ValidationMode::STRICT));
}

TEST(PKIShardCertificateTest, ValidationModeCompatible) {
    // Test COMPATIBLE mode: allows both OID and CN extraction
    ShardCertificateInfo info;
    info.shard_id = "shard_001";
    info.not_before = "2025-01-01";
    info.not_after = "2026-01-01";
    info.capabilities = {"read", "write"};
    info.token_range_start = 0;
    info.token_range_end = 0x7FFFFFFFFFFFFFFFULL;
    
    // COMPATIBLE mode accepts both OID and CN sources
    info.shard_id_from_oid = false;
    EXPECT_TRUE(PKIShardCertificate::validateWithMode(info, ValidationMode::COMPATIBLE));
    
    info.shard_id_from_oid = true;
    EXPECT_TRUE(PKIShardCertificate::validateWithMode(info, ValidationMode::COMPATIBLE));
}

TEST(PKIShardCertificateTest, ValidationModeLegacy) {
    // Test LEGACY mode: accepts any valid certificate
    ShardCertificateInfo info;
    info.shard_id = "shard_001";
    info.not_before = "2025-01-01";
    info.not_after = "2026-01-01";
    info.capabilities = {"read", "write"};
    info.token_range_start = 0;
    info.token_range_end = 0x7FFFFFFFFFFFFFFFULL;
    
    // LEGACY mode doesn't care about source
    info.shard_id_from_oid = false;
    EXPECT_TRUE(PKIShardCertificate::validateWithMode(info, ValidationMode::LEGACY));
    
    info.shard_id_from_oid = true;
    EXPECT_TRUE(PKIShardCertificate::validateWithMode(info, ValidationMode::LEGACY));
}

TEST(PKIShardCertificateTest, ShardIdentityStructure) {
    // Test ShardIdentity structure
    ShardIdentity identity;
    identity.shard_id = "shard_001";
    identity.region = "us-east-1";
    identity.role = "node";
    identity.sans = {"shard-001.example.com", "192.168.1.100"};
    identity.from_oid = true;
    
    EXPECT_EQ(identity.shard_id, "shard_001");
    EXPECT_EQ(identity.region, "us-east-1");
    EXPECT_EQ(identity.role, "node");
    EXPECT_EQ(identity.sans.size(), 2);
    EXPECT_TRUE(identity.from_oid);
}

TEST(PKIShardCertificateTest, CNSpoofingPreventionStrict) {
    // Test that STRICT mode prevents CN-based spoofing
    ShardCertificateInfo info;
    info.shard_id = "shard_malicious";  // Could be spoofed via CN
    info.not_before = "2025-01-01";
    info.not_after = "2026-01-01";
    info.capabilities = {"read", "write"};
    info.token_range_start = 0;
    info.token_range_end = 0x7FFFFFFFFFFFFFFFULL;
    info.shard_id_from_oid = false;  // CN-based extraction
    
    // STRICT mode should reject CN-based extraction
    EXPECT_FALSE(PKIShardCertificate::validateWithMode(info, ValidationMode::STRICT));
}

TEST(PKIShardCertificateTest, RegionAndRoleExtraction) {
    // Test that region and role fields are populated
    ShardCertificateInfo info;
    info.shard_id = "shard_001";
    info.region = "us-west-2";
    info.role = "admin";
    info.not_before = "2025-01-01";
    info.not_after = "2026-01-01";
    info.capabilities = {"read", "write", "admin"};
    info.token_range_start = 0;
    info.token_range_end = 0x7FFFFFFFFFFFFFFFULL;
    info.shard_id_from_oid = true;
    
    EXPECT_EQ(info.region, "us-west-2");
    EXPECT_EQ(info.role, "admin");
    EXPECT_TRUE(PKIShardCertificate::validateWithMode(info, ValidationMode::STRICT));
}

TEST(PKIShardCertificateTest, MultipleRoleSupport) {
    // Test that multiple roles are supported
    std::vector<std::string> supported_roles = {
        "node", "client", "admin", "primary", "replica"
    };
    
    for (const auto& role : supported_roles) {
        ShardCertificateInfo info;
        info.shard_id = "shard_001";
        info.role = role;
        info.not_before = "2025-01-01";
        info.not_after = "2026-01-01";
        info.capabilities = {"read", "write"};
        info.token_range_start = 0;
        info.token_range_end = 0x7FFFFFFFFFFFFFFFULL;
        info.shard_id_from_oid = true;
        
        EXPECT_TRUE(PKIShardCertificate::validateWithMode(info, ValidationMode::COMPATIBLE));
    }
}

TEST(PKIShardCertificateTest, EmptyShardIdValidation) {
    // Test that empty shard_id fails validation in all modes
    ShardCertificateInfo info;
    info.shard_id = "";  // Empty
    info.not_before = "2025-01-01";
    info.not_after = "2026-01-01";
    info.capabilities = {"read", "write"};
    info.token_range_start = 0;
    info.token_range_end = 0x7FFFFFFFFFFFFFFFULL;
    
    EXPECT_FALSE(PKIShardCertificate::validateWithMode(info, ValidationMode::STRICT));
    EXPECT_FALSE(PKIShardCertificate::validateWithMode(info, ValidationMode::COMPATIBLE));
    EXPECT_FALSE(PKIShardCertificate::validateWithMode(info, ValidationMode::LEGACY));
}

TEST(PKIShardCertificateTest, TokenRangeValidation) {
    // Test token range validation logic
    ShardCertificateInfo info;
    info.shard_id = "shard_001";
    info.not_before = "2025-01-01";
    info.not_after = "2026-01-01";
    info.capabilities = {"read", "write"};
    info.shard_id_from_oid = true;
    
    // Valid range: start < end
    info.token_range_start = 0;
    info.token_range_end = 1000;
    EXPECT_TRUE(PKIShardCertificate::validateWithMode(info, ValidationMode::STRICT));
    
    // Invalid range: start > end
    info.token_range_start = 1000;
    info.token_range_end = 100;
    EXPECT_FALSE(PKIShardCertificate::validateWithMode(info, ValidationMode::STRICT));
    
    // Edge case: start == end (invalid unless end is 0)
    info.token_range_start = 500;
    info.token_range_end = 500;
    EXPECT_FALSE(PKIShardCertificate::validateWithMode(info, ValidationMode::STRICT));
    
    // Full range (special case: end = 0 means full range)
    info.token_range_start = 0;
    info.token_range_end = 0;
    EXPECT_TRUE(PKIShardCertificate::validateWithMode(info, ValidationMode::STRICT));
}

