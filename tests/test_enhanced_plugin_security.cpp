// test_enhanced_plugin_security.cpp
// Unit tests for EnhancedPluginSecurityVerifier

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "acceleration/plugin_security.h"
#include <fstream>
#include <filesystem>

using namespace themis::acceleration;

class EnhancedPluginSecurityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        test_dir_ = std::filesystem::temp_directory_path() / "themis_security_test";
        std::filesystem::create_directories(test_dir_);
        
        // Create a test plugin file
        test_plugin_path_ = test_dir_ / "test_plugin.so";
        std::ofstream file(test_plugin_path_, std::ios::binary);
        file << "FAKE_PLUGIN_DATA_FOR_TESTING";
        file.close();
    }
    
    void TearDown() override {
        // Clean up test directory
        std::filesystem::remove_all(test_dir_);
    }
    
    std::filesystem::path test_dir_;
    std::filesystem::path test_plugin_path_;
};

// Test: Basic construction
TEST_F(EnhancedPluginSecurityTest, Construction) {
    PluginSecurityPolicy policy;
    policy.allowUnsigned = true;  // For testing
    
    EnhancedPluginSecurityVerifier verifier(policy);
    
    EXPECT_EQ(verifier.getPolicy().allowUnsigned, true);
}

// Test: Hash verification alone cannot authorize a plugin
TEST_F(EnhancedPluginSecurityTest, Level1_HashOnlyRejected) {
    PluginSecurityPolicy policy;
    policy.allowUnsigned = true;
    policy.verifyFileHash = true;
    
    EnhancedPluginSecurityVerifier verifier(policy);
    
    auto result = verifier.verifyPlugin(
        test_plugin_path_.string(),
        EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_1_HASH_ONLY
    );
    
    EXPECT_FALSE(result.passed);
    EXPECT_FALSE(result.hash_verified);
    EXPECT_THAT(result.error_message, testing::HasSubstr("signer identity"));
}

// Test: Level 2 - Embedded signature (should fail without embedded cert)
TEST_F(EnhancedPluginSecurityTest, Level2_EmbeddedSignature_MissingCert) {
    PluginSecurityPolicy policy;
    policy.allowUnsigned = false;
    
    EnhancedPluginSecurityVerifier verifier(policy);
    
    auto result = verifier.verifyPlugin(
        test_plugin_path_.string(),
        EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_2_EMBEDDED_SIGNATURE
    );
    
    // Should fail because no embedded certificate
    EXPECT_FALSE(result.passed);
    EXPECT_FALSE(result.embedded_signature_verified);
    EXPECT_THAT(result.error_message, testing::HasSubstr("embedded certificate"));
}

// Test: Unsigned policy cannot bypass embedded signer verification
TEST_F(EnhancedPluginSecurityTest, Level2_EmbeddedSignature_AllowUnsignedRejected) {
    PluginSecurityPolicy policy;
    policy.allowUnsigned = true;
    
    EnhancedPluginSecurityVerifier verifier(policy);
    
    auto result = verifier.verifyPlugin(
        test_plugin_path_.string(),
        EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_2_EMBEDDED_SIGNATURE
    );
    
    EXPECT_FALSE(result.passed);
    EXPECT_TRUE(result.hash_verified);
}

TEST_F(EnhancedPluginSecurityTest, DisabledSignatureVerificationRejected) {
    PluginSecurityPolicy policy;
    policy.requireSignature = false;
    policy.allowUnsigned = true;
    EnhancedPluginSecurityVerifier verifier(policy);

    const auto result = verifier.verifyPlugin(
        test_plugin_path_.string(),
        EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_2_EMBEDDED_SIGNATURE);

    EXPECT_FALSE(result.passed);
    EXPECT_EQ(result.error_message, "Plugin signature verification cannot be disabled");
}

// Test: Level 3 - Platform signature (should fail without platform signature)
TEST_F(EnhancedPluginSecurityTest, Level3_PlatformSignature_Missing) {
    PluginSecurityPolicy policy;
    policy.allowUnsigned = false;
    
    EnhancedPluginSecurityVerifier verifier(policy);
    
    auto result = verifier.verifyPlugin(
        test_plugin_path_.string(),
        EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_3_PLATFORM_SIGNATURE
    );
    
    // Should fail because no platform signature
    EXPECT_FALSE(result.passed);
    EXPECT_FALSE(result.platform_signature_verified);
}

// Test: Unsigned policy cannot bypass pinned signer verification
TEST_F(EnhancedPluginSecurityTest, Level3_PlatformSignature_AllowUnsignedRejected) {
    PluginSecurityPolicy policy;
    policy.allowUnsigned = true;
    
    EnhancedPluginSecurityVerifier verifier(policy);
    
    auto result = verifier.verifyPlugin(
        test_plugin_path_.string(),
        EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_3_PLATFORM_SIGNATURE
    );
    
    EXPECT_FALSE(result.passed);
    EXPECT_TRUE(result.hash_verified);
}

// Test: Non-existent file
TEST_F(EnhancedPluginSecurityTest, NonExistentFile) {
    PluginSecurityPolicy policy;
    policy.allowUnsigned = true;
    
    EnhancedPluginSecurityVerifier verifier(policy);
    
    auto result = verifier.verifyPlugin(
        "/nonexistent/plugin.so",
        EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_1_HASH_ONLY
    );
    
    EXPECT_FALSE(result.passed);
    EXPECT_FALSE(result.hash_verified);
}

// Test: Policy update
TEST_F(EnhancedPluginSecurityTest, PolicyUpdate) {
    PluginSecurityPolicy policy1;
    policy1.allowUnsigned = true;
    
    EnhancedPluginSecurityVerifier verifier(policy1);
    EXPECT_TRUE(verifier.getPolicy().allowUnsigned);
    
    PluginSecurityPolicy policy2;
    policy2.allowUnsigned = false;
    policy2.requireSignature = true;
    
    verifier.updatePolicy(policy2);
    EXPECT_FALSE(verifier.getPolicy().allowUnsigned);
    EXPECT_TRUE(verifier.getPolicy().requireSignature);
}

// Test: Verification level progression
TEST_F(EnhancedPluginSecurityTest, VerificationLevelProgression) {
    PluginSecurityPolicy policy;
    policy.allowUnsigned = true;
    
    EnhancedPluginSecurityVerifier verifier(policy);
    
    // Hash-only verification is insufficient to authorize a plugin.
    auto result1 = verifier.verifyPlugin(
        test_plugin_path_.string(),
        EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_1_HASH_ONLY
    );
    EXPECT_FALSE(result1.passed);
    EXPECT_THAT(result1.error_message, testing::HasSubstr("signer identity"));
    
    // allowUnsigned cannot downgrade signature verification to hash-only.
    auto result2 = verifier.verifyPlugin(
        test_plugin_path_.string(),
        EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_2_EMBEDDED_SIGNATURE
    );
    EXPECT_FALSE(result2.passed);
    EXPECT_TRUE(result2.hash_verified);
}

// Test: VerificationResult structure
TEST_F(EnhancedPluginSecurityTest, VerificationResultStructure) {
    EnhancedPluginSecurityVerifier::VerificationResult result;
    
    // Check default values
    EXPECT_FALSE(result.passed);
    EXPECT_FALSE(result.hash_verified);
    EXPECT_FALSE(result.embedded_signature_verified);
    EXPECT_FALSE(result.platform_signature_verified);
    EXPECT_FALSE(result.certificate_chain_verified);
    EXPECT_FALSE(result.certificate_not_revoked);
    EXPECT_FALSE(result.is_themisdb_official);
    EXPECT_TRUE(result.error_message.empty());
    EXPECT_TRUE(result.issuer.empty());
    EXPECT_TRUE(result.subject.empty());
}
