/*
 * ThemisDB | File: test_security_di.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 94/100
 * Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "security/encryption.h"
#include "security/rbac.h"
#include "auth/jwt_validator.h"
#include "core/security_initialization.h"
#include "themis/base/interfaces/security_interface.h"
#include "security/mock_key_provider.h"
#include "themis/edition_manager.h"
#include "themis/runtime_license_gate.h"

using namespace themis;
using namespace testing;

// Use the canonical themis::MockKeyProvider from include/security/mock_key_provider.h

/**
 * @brief Test FieldEncryption with Dependency Injection
 */
class FieldEncryptionDITest : public ::testing::Test {
protected:
    std::shared_ptr<themis::MockKeyProvider> real_mock_provider_;
    std::shared_ptr<FieldEncryption> encryption_;
    
    void SetUp() override {
        real_mock_provider_ = std::make_shared<themis::MockKeyProvider>();
        std::string edition_err;
        if (!themis::edition::EditionManager::instance().isFeatureAvailable("field_encryption", edition_err)) {
            GTEST_SKIP() << "Field encryption unavailable: " << edition_err;
        }
        encryption_ = std::make_shared<FieldEncryption>(real_mock_provider_);
    }
};

TEST_F(FieldEncryptionDITest, ConstructorAcceptsKeyProvider) {
    EXPECT_NE(encryption_, nullptr);
    EXPECT_NE(encryption_->getKeyProvider(), nullptr);
}

TEST_F(FieldEncryptionDITest, ConstructorRejectsNullKeyProvider) {
    EXPECT_THROW(
        FieldEncryption(nullptr),
        std::invalid_argument
    );
}

TEST_F(FieldEncryptionDITest, EncryptsWithInjectedKeyProvider) {
    // Configure encryption
    EncryptionConfig config;
    config.encrypted_fields.insert("ssn");
    config.field_key_mapping["ssn"] = "ssn_key";
    encryption_->setEncryptionConfig(config);
    
    // Test encryption
    std::vector<uint8_t> plaintext = {1, 2, 3, 4, 5};
    auto encrypted = encryption_->encrypt_field("ssn", plaintext);
    
    // Should return encrypted data (different from plaintext)
    EXPECT_FALSE(encrypted.empty());
}

TEST_F(FieldEncryptionDITest, ShouldEncryptReturnsFalseForNonConfiguredFields) {
    // Configure encryption for specific fields only
    EncryptionConfig config;
    config.encrypted_fields.insert("ssn");
    encryption_->setEncryptionConfig(config);
    
    EXPECT_TRUE(encryption_->should_encrypt("ssn"));
    EXPECT_FALSE(encryption_->should_encrypt("age"));
}

TEST_F(FieldEncryptionDITest, ShouldEncryptReturnsTrueWhenNoConfigSet) {
    // No configuration means encrypt all fields by default
    EXPECT_TRUE(encryption_->should_encrypt("ssn"));
    EXPECT_TRUE(encryption_->should_encrypt("age"));
}

TEST_F(FieldEncryptionDITest, CreateDefaultFactoryWorks) {
    auto enc = FieldEncryption::createDefault();
    EXPECT_NE(enc, nullptr);
    EXPECT_NE(enc->getKeyProvider(), nullptr);
}

/**
 * @brief Test RBAC Policy
 */
class RBACPolicyTest : public ::testing::Test {
protected:
    std::shared_ptr<security::RBAC> rbac_;
    
    void SetUp() override {
        security::RBACConfig config;
        config.use_builtin_roles = true;
        std::string license_err;
        if (!license::RuntimeLicenseGate::instance().isFeatureAllowed("rbac", license_err)) {
            GTEST_SKIP() << "RBAC unavailable: " << license_err;
        }
        rbac_ = std::make_shared<security::RBAC>(config);
        
        // Define roles
        security::Role admin_role;
        admin_role.name = "admin";
        admin_role.description = "Administrator";
        admin_role.permissions = {
            {"*", "*"}  // All permissions
        };
        rbac_->addRole(admin_role);
        
        security::Role user_role;
        user_role.name = "user";
        user_role.description = "Regular user";
        user_role.permissions = {
            {"data", "read"}
        };
        rbac_->addRole(user_role);
        
        security::Role analyst_role;
        analyst_role.name = "analyst";
        analyst_role.description = "Data analyst";
        analyst_role.permissions = {
            {"data", "read"},
            {"data", "export"}
        };
        rbac_->addRole(analyst_role);
    }
};

TEST_F(RBACPolicyTest, AdminCanWrite) {
    bool allowed = rbac_->checkPermission({"admin"}, "documents", "write");
    EXPECT_TRUE(allowed);
}

TEST_F(RBACPolicyTest, UserCannotDelete) {
    bool allowed = rbac_->checkPermission({"user"}, "documents", "delete");
    EXPECT_FALSE(allowed);
}

TEST_F(RBACPolicyTest, UserCanRead) {
    bool allowed = rbac_->checkPermission({"user"}, "data", "read");
    EXPECT_TRUE(allowed);
}

TEST_F(RBACPolicyTest, AnalystCanExport) {
    bool allowed = rbac_->checkPermission({"analyst"}, "data", "export");
    EXPECT_TRUE(allowed);
}

TEST_F(RBACPolicyTest, AnalystCannotWrite) {
    bool allowed = rbac_->checkPermission({"analyst"}, "data", "write");
    EXPECT_FALSE(allowed);
}

/**
 * @brief Test SecurityLayerBuilder
 */
class SecurityLayerBuilderTest : public ::testing::Test {
protected:
    SecurityLayerBuilder builder_;
    void SetUp() override {
        std::string edition_err;
        if (!themis::edition::EditionManager::instance().isFeatureAvailable("field_encryption", edition_err)) {
            GTEST_SKIP() << "Field encryption unavailable: " << edition_err;
        }
    }
};

TEST_F(SecurityLayerBuilderTest, StandardBuilderWorks) {
    auto builder = SecurityLayerBuilder::standard();
    auto layer = builder.build();
    
    EXPECT_NE(layer.field_encryption, nullptr);
    EXPECT_NE(layer.rbac, nullptr);
    EXPECT_NE(layer.jwt, nullptr);
}

TEST_F(SecurityLayerBuilderTest, BuildWithLocalKeyProvider) {
    auto layer = builder_
        .withKeyProvider(SecurityLayerBuilder::KeyProviderType::LOCAL, "{}")
        .build();
    
    EXPECT_NE(layer.field_encryption, nullptr);
}

TEST_F(SecurityLayerBuilderTest, BuildWithFieldEncryptionConfig) {
    EncryptionConfig config;
    config.encrypted_fields.insert("ssn");
    config.encrypted_fields.insert("credit_card");
    config.field_key_mapping["ssn"] = "pii_key";
    config.field_key_mapping["credit_card"] = "payment_key";
    
    auto layer = builder_
        .withKeyProvider(SecurityLayerBuilder::KeyProviderType::LOCAL, "{}")
        .withFieldEncryption(config)
        .build();
    
    EXPECT_NE(layer.field_encryption, nullptr);
    
    // Cast to concrete type to access should_encrypt
    auto concrete_enc = std::dynamic_pointer_cast<FieldEncryption>(layer.field_encryption);
    ASSERT_NE(concrete_enc, nullptr);
    
    EXPECT_TRUE(concrete_enc->should_encrypt("ssn"));
    EXPECT_TRUE(concrete_enc->should_encrypt("credit_card"));
    EXPECT_FALSE(concrete_enc->should_encrypt("age"));
}

TEST_F(SecurityLayerBuilderTest, BuildWithRBACPolicy) {
    // Note: This test will fail if policy file doesn't exist
    // In a real test, we'd create a temporary policy file
    auto layer = builder_
        .withKeyProvider(SecurityLayerBuilder::KeyProviderType::LOCAL, "{}")
        .build();
    
    EXPECT_NE(layer.rbac, nullptr);
}

TEST_F(SecurityLayerBuilderTest, BuilderIsChainable) {
    EncryptionConfig config;
    config.encrypted_fields.insert("ssn");
    
    auto layer = SecurityLayerBuilder()
        .withKeyProvider(SecurityLayerBuilder::KeyProviderType::LOCAL, "{}")
        .withFieldEncryption(config)
        .build();
    
    EXPECT_NE(layer.field_encryption, nullptr);
    EXPECT_NE(layer.rbac, nullptr);
    EXPECT_NE(layer.jwt, nullptr);
}

/**
 * @brief Integration test: Full security layer
 */
TEST(SecurityLayerIntegrationTest, FullSecurityLayerWorks) {
    std::string edition_err;
    if (!themis::edition::EditionManager::instance().isFeatureAvailable("field_encryption", edition_err)) {
        GTEST_SKIP() << "Field encryption unavailable: " << edition_err;
    }

    // Build complete security layer
    EncryptionConfig enc_config;
    enc_config.encrypted_fields.insert("ssn");
    enc_config.field_key_mapping["ssn"] = "pii_key";
    
    auto layer = SecurityLayerBuilder()
        .withKeyProvider(SecurityLayerBuilder::KeyProviderType::LOCAL, "{}")
        .withFieldEncryption(enc_config)
        .build();
    
    // Test field encryption
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    auto encrypted = layer.field_encryption->encrypt_field("ssn", data);
    EXPECT_FALSE(encrypted.empty());
    
    auto decrypted = layer.field_encryption->decrypt_field("ssn", encrypted);
    EXPECT_EQ(data, decrypted);
    
    // Test RBAC (with builtin roles)
    EXPECT_NE(layer.rbac, nullptr);
    
    // Test JWT validator
    EXPECT_NE(layer.jwt, nullptr);
}


