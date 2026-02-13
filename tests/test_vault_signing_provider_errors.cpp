#include <gtest/gtest.h>
#include "security/vault_signing_provider.h"
#include <cstdlib>

using namespace themis;

/**
 * VaultSigningProvider Error Message Tests
 * 
 * Tests that VaultSigningProvider provides clear error messages
 * explaining its signing-only limitation and migration paths.
 */

class VaultSigningProviderErrorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear Vault environment variables
        #ifdef _WIN32
            _putenv_s("THEMIS_VAULT_ADDR", "");
            _putenv_s("THEMIS_VAULT_TOKEN", "");
            _putenv_s("THEMIS_VAULT_TRANSIT_MOUNT", "");
        #else
            unsetenv("THEMIS_VAULT_ADDR");
            unsetenv("THEMIS_VAULT_TOKEN");
            unsetenv("THEMIS_VAULT_TRANSIT_MOUNT");
        #endif
    }
};

TEST_F(VaultSigningProviderErrorTest, GetKeyThrowsWithHelpfulMessage) {
    VaultSigningProvider::Config config;
    VaultSigningProvider provider(config);
    
    try {
        provider.getKey("test-key");
        FAIL() << "Expected KeyOperationException";
    } catch (const KeyOperationException& e) {
        std::string msg(e.what());
        
        // Should mention it's signing-only
        EXPECT_NE(msg.find("signing-only"), std::string::npos);
        
        // Should mention VaultKeyProvider as alternative
        EXPECT_NE(msg.find("VaultKeyProvider"), std::string::npos);
        
        // Should reference documentation
        EXPECT_NE(msg.find("docs/security/VAULT_SIGNING_PROVIDER.md"), std::string::npos);
    }
}

TEST_F(VaultSigningProviderErrorTest, GetKeyWithVersionThrowsWithHelpfulMessage) {
    VaultSigningProvider::Config config;
    VaultSigningProvider provider(config);
    
    try {
        provider.getKey("test-key", 1);
        FAIL() << "Expected KeyOperationException";
    } catch (const KeyOperationException& e) {
        std::string msg(e.what());
        EXPECT_NE(msg.find("signing-only"), std::string::npos);
        EXPECT_NE(msg.find("VaultKeyProvider"), std::string::npos);
        EXPECT_NE(msg.find("docs/security/VAULT_SIGNING_PROVIDER.md"), std::string::npos);
    }
}

TEST_F(VaultSigningProviderErrorTest, RotateKeyThrowsWithHelpfulMessage) {
    VaultSigningProvider::Config config;
    VaultSigningProvider provider(config);
    
    try {
        provider.rotateKey("test-key");
        FAIL() << "Expected KeyOperationException";
    } catch (const KeyOperationException& e) {
        std::string msg(e.what());
        EXPECT_NE(msg.find("signing-only"), std::string::npos);
        
        // Should mention both VaultKeyProvider and Vault CLI
        EXPECT_TRUE(
            msg.find("VaultKeyProvider") != std::string::npos ||
            msg.find("Vault CLI") != std::string::npos
        );
        
        EXPECT_NE(msg.find("docs/security/VAULT_SIGNING_PROVIDER.md"), std::string::npos);
    }
}

TEST_F(VaultSigningProviderErrorTest, ListKeysThrowsWithHelpfulMessage) {
    VaultSigningProvider::Config config;
    VaultSigningProvider provider(config);
    
    try {
        provider.listKeys();
        FAIL() << "Expected KeyOperationException";
    } catch (const KeyOperationException& e) {
        std::string msg(e.what());
        EXPECT_NE(msg.find("signing-only"), std::string::npos);
        
        // Should mention VaultKeyProvider or Vault API
        EXPECT_TRUE(
            msg.find("VaultKeyProvider") != std::string::npos ||
            msg.find("Vault API") != std::string::npos
        );
        
        EXPECT_NE(msg.find("docs/security/VAULT_SIGNING_PROVIDER.md"), std::string::npos);
    }
}

TEST_F(VaultSigningProviderErrorTest, GetKeyMetadataThrowsWithHelpfulMessage) {
    VaultSigningProvider::Config config;
    VaultSigningProvider provider(config);
    
    try {
        provider.getKeyMetadata("test-key");
        FAIL() << "Expected KeyOperationException";
    } catch (const KeyOperationException& e) {
        std::string msg(e.what());
        EXPECT_NE(msg.find("signing-only"), std::string::npos);
        EXPECT_NE(msg.find("VaultKeyProvider"), std::string::npos);
        EXPECT_NE(msg.find("docs/security/VAULT_SIGNING_PROVIDER.md"), std::string::npos);
    }
}

TEST_F(VaultSigningProviderErrorTest, DeleteKeyThrowsWithHelpfulMessage) {
    VaultSigningProvider::Config config;
    VaultSigningProvider provider(config);
    
    try {
        provider.deleteKey("test-key", 1);
        FAIL() << "Expected KeyOperationException";
    } catch (const KeyOperationException& e) {
        std::string msg(e.what());
        EXPECT_NE(msg.find("signing-only"), std::string::npos);
        
        // Should mention VaultKeyProvider or Vault API
        EXPECT_TRUE(
            msg.find("VaultKeyProvider") != std::string::npos ||
            msg.find("Vault API") != std::string::npos
        );
        
        EXPECT_NE(msg.find("docs/security/VAULT_SIGNING_PROVIDER.md"), std::string::npos);
    }
}

TEST_F(VaultSigningProviderErrorTest, HasKeyThrowsWithHelpfulMessage) {
    VaultSigningProvider::Config config;
    VaultSigningProvider provider(config);
    
    try {
        provider.hasKey("test-key");
        FAIL() << "Expected KeyOperationException";
    } catch (const KeyOperationException& e) {
        std::string msg(e.what());
        EXPECT_NE(msg.find("signing-only"), std::string::npos);
        EXPECT_NE(msg.find("VaultKeyProvider"), std::string::npos);
        EXPECT_NE(msg.find("docs/security/VAULT_SIGNING_PROVIDER.md"), std::string::npos);
    }
}

TEST_F(VaultSigningProviderErrorTest, CreateKeyFromBytesThrowsWithHelpfulMessage) {
    VaultSigningProvider::Config config;
    VaultSigningProvider provider(config);
    
    std::vector<uint8_t> key_bytes = {1, 2, 3, 4};
    
    try {
        provider.createKeyFromBytes("test-key", key_bytes);
        FAIL() << "Expected KeyOperationException";
    } catch (const KeyOperationException& e) {
        std::string msg(e.what());
        EXPECT_NE(msg.find("signing-only"), std::string::npos);
        
        // Should mention VaultKeyProvider or Vault API
        EXPECT_TRUE(
            msg.find("VaultKeyProvider") != std::string::npos ||
            msg.find("Vault API") != std::string::npos
        );
        
        EXPECT_NE(msg.find("docs/security/VAULT_SIGNING_PROVIDER.md"), std::string::npos);
    }
}

// Test that all error messages are consistent in format
TEST_F(VaultSigningProviderErrorTest, AllErrorMessagesHaveConsistentFormat) {
    VaultSigningProvider::Config config;
    VaultSigningProvider provider(config);
    
    std::vector<std::string> error_messages;
    
    // Collect all error messages
    try { provider.getKey("k"); } catch (const KeyOperationException& e) { 
        error_messages.push_back(e.what()); 
    }
    try { provider.rotateKey("k"); } catch (const KeyOperationException& e) { 
        error_messages.push_back(e.what()); 
    }
    try { provider.listKeys(); } catch (const KeyOperationException& e) { 
        error_messages.push_back(e.what()); 
    }
    
    // All should start with "VaultSigningProvider:"
    for (const auto& msg : error_messages) {
        EXPECT_EQ(msg.find("VaultSigningProvider:"), 0u);
    }
    
    // All should mention "signing-only"
    for (const auto& msg : error_messages) {
        EXPECT_NE(msg.find("signing-only"), std::string::npos);
    }
    
    // All should reference documentation
    for (const auto& msg : error_messages) {
        EXPECT_NE(msg.find("docs/security"), std::string::npos);
    }
}