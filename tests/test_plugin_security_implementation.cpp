// test_plugin_security_implementation.cpp
// Tests for newly implemented security functions

#include <gtest/gtest.h>
#include "acceleration/plugin_security.h"
#include <fstream>
#include <filesystem>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/evp.h>

using namespace themis::acceleration;

class PluginSecurityImplementationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        test_dir_ = std::filesystem::temp_directory_path() / "themis_security_impl_test";
        std::filesystem::create_directories(test_dir_);
        
        // Create a test plugin file
        test_plugin_path_ = test_dir_ / "test_plugin.so";
        std::ofstream file(test_plugin_path_, std::ios::binary);
        file << "FAKE_PLUGIN_DATA_FOR_TESTING";
        file.close();
        
        // Generate a self-signed test certificate
        generateTestCertificate();
    }
    
    void TearDown() override {
        // Clean up test directory
        std::filesystem::remove_all(test_dir_);
    }
    
    void generateTestCertificate() {
        // Generate RSA key pair using EVP API (OpenSSL 3.0+ compatible)
        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
        if (!ctx) {
            return;
        }
        
        if (EVP_PKEY_keygen_init(ctx) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            return;
        }
        
        if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            return;
        }
        
        EVP_PKEY* pkey = nullptr;
        if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            return;
        }
        EVP_PKEY_CTX_free(ctx);
        
        // Create X.509 certificate
        X509* x509 = X509_new();
        X509_set_version(x509, 2);
        ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
        X509_gmtime_adj(X509_get_notBefore(x509), 0);
        X509_gmtime_adj(X509_get_notAfter(x509), 31536000L); // 1 year
        
        X509_set_pubkey(x509, pkey);
        
        X509_NAME* name = X509_get_subject_name(x509);
        X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, 
                                   reinterpret_cast<const unsigned char*>("DE"), -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC,
                                   reinterpret_cast<const unsigned char*>("ThemisDB"), -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                                   reinterpret_cast<const unsigned char*>("ThemisDB Test CA"), -1, -1, 0);
        X509_set_issuer_name(x509, name);
        
        // Sign certificate
        X509_sign(x509, pkey, EVP_sha256());
        
        // Write certificate to PEM string
        BIO* bio = BIO_new(BIO_s_mem());
        PEM_write_bio_X509(bio, x509);
        
        BUF_MEM* mem = nullptr;
        BIO_get_mem_ptr(bio, &mem);
        test_cert_pem_ = std::string(mem->data, mem->length);
        
        // Cleanup
        BIO_free(bio);
        X509_free(x509);
        EVP_PKEY_free(pkey);
    }
    
    std::filesystem::path test_dir_;
    std::filesystem::path test_plugin_path_;
    std::string test_cert_pem_;
};

// Test: Certificate chain validation with self-signed cert
TEST_F(PluginSecurityImplementationTest, VerifyCertificateChain_SelfSigned) {
    PluginSecurityPolicy policy;
    policy.allowUnsigned = true;
    
    PluginSecurityVerifier verifier(policy);
    
    // Self-signed cert should fail chain validation (unless in trusted store)
    bool result = verifier.verifyCertificateChain(test_cert_pem_);
    
    // Self-signed certs not in trust store should fail
    EXPECT_FALSE(result);
}

// Test: Certificate chain validation with empty certificate
TEST_F(PluginSecurityImplementationTest, VerifyCertificateChain_Empty) {
    PluginSecurityPolicy policy;
    PluginSecurityVerifier verifier(policy);
    
    bool result = verifier.verifyCertificateChain("");
    EXPECT_FALSE(result);
}

// Test: CRL checking
TEST_F(PluginSecurityImplementationTest, CheckCRL) {
    PluginSecurityPolicy policy;
    PluginSecurityVerifier verifier(policy);
    
    // Test with valid certificate (may not have CRL endpoints)
    EXPECT_NO_THROW({
        verifier.checkCRL(test_cert_pem_);
    });
    
    // Test with empty certificate
    bool result = verifier.checkCRL("");
    EXPECT_FALSE(result);
}

// Test: OCSP checking
TEST_F(PluginSecurityImplementationTest, CheckOCSP) {
    PluginSecurityPolicy policy;
    PluginSecurityVerifier verifier(policy);
    
    // Test with valid certificate (may not have OCSP responders)
    EXPECT_NO_THROW({
        verifier.checkOCSP(test_cert_pem_);
    });
    
    // Test with empty certificate
    bool result = verifier.checkOCSP("");
    EXPECT_FALSE(result);
}

// Test: Embedded certificate extraction is handled internally
TEST_F(PluginSecurityImplementationTest, Level2_EmbeddedSignature_NoEmbeddedData) {
    PluginSecurityPolicy policy;
    policy.allowUnsigned = false;
    
    EnhancedPluginSecurityVerifier verifier(policy);
    
    // Our test file is not a real PE/ELF/Mach-O with embedded signature
    // Level 2 verification should fail
    auto result = verifier.verifyPlugin(
        test_plugin_path_.string(),
        EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_2_EMBEDDED_SIGNATURE
    );
    
    EXPECT_FALSE(result.passed);
    EXPECT_FALSE(result.embedded_signature_verified);
}

// Test: Embedded signature extraction is handled internally  
TEST_F(PluginSecurityImplementationTest, Level3_PlatformSignature_NoSignature) {
    PluginSecurityPolicy policy;
    policy.allowUnsigned = false;
    
    EnhancedPluginSecurityVerifier verifier(policy);
    
    // Our test file is not signed with platform signature
    // Level 3 verification should fail
    auto result = verifier.verifyPlugin(
        test_plugin_path_.string(),
        EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_3_PLATFORM_SIGNATURE
    );
    
    EXPECT_FALSE(result.passed);
    EXPECT_FALSE(result.platform_signature_verified);
}

// Test: Platform signature verification through public API
TEST_F(PluginSecurityImplementationTest, VerifyPlatformSignature_UnsignedFile) {
    PluginSecurityPolicy policy;
    policy.allowUnsigned = false;
    
    EnhancedPluginSecurityVerifier verifier(policy);
    
    // Test file is not signed, so platform verification should fail
    auto result = verifier.verifyPlugin(
        test_plugin_path_.string(),
        EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_3_PLATFORM_SIGNATURE
    );
    
    EXPECT_FALSE(result.passed);
    EXPECT_FALSE(result.platform_signature_verified);
    EXPECT_FALSE(result.error_message.empty());
}

// Test: Full chain verification with development mode
TEST_F(PluginSecurityImplementationTest, VerifyFullChain_DevelopmentMode) {
    PluginSecurityPolicy policy;
    policy.allowUnsigned = true;
    policy.checkRevocation = false;  // Disable revocation checking for test
    
    EnhancedPluginSecurityVerifier verifier(policy);
    
    // Should gracefully handle unsigned plugins in development mode
    auto result = verifier.verifyPlugin(
        test_plugin_path_.string(),
        EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_4_FULL_CHAIN
    );
    
    // In development mode with allowUnsigned, some checks may pass
    // Just verify it doesn't crash
    EXPECT_NO_THROW({
        verifier.verifyPlugin(
            test_plugin_path_.string(),
            EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_4_FULL_CHAIN
        );
    });
}

// Test: PE format detection
TEST_F(PluginSecurityImplementationTest, PEFormatDetection) {
    // Create a minimal PE header
    std::vector<uint8_t> pe_data;
    
    // DOS header
    pe_data.push_back('M');
    pe_data.push_back('Z');
    
    // Fill rest of DOS header (to reach 0x3C where PE offset is stored)
    for (int i = 2; i < 0x3C; i++) {
        pe_data.push_back(0);
    }
    
    // PE offset (pointing to offset 0x80)
    uint32_t pe_offset = 0x80;
    for (int i = 0; i < 4; i++) {
        pe_data.push_back(reinterpret_cast<uint8_t*>(&pe_offset)[i]);
    }
    
    // Fill up to PE offset
    while (pe_data.size() < pe_offset) {
        pe_data.push_back(0);
    }
    
    // PE signature "PE\0\0"
    pe_data.push_back('P');
    pe_data.push_back('E');
    pe_data.push_back(0);
    pe_data.push_back(0);
    
    // Write to file
    std::filesystem::path pe_file = test_dir_ / "test.dll";
    std::ofstream file(pe_file, std::ios::binary);
    file.write(reinterpret_cast<const char*>(pe_data.data()), pe_data.size());
    file.close();
    
    PluginSecurityPolicy policy;
    policy.allowUnsigned = false;
    EnhancedPluginSecurityVerifier verifier(policy);
    
    // Should detect PE format but fail verification (no signature)
    auto result = verifier.verifyPlugin(
        pe_file.string(),
        EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_2_EMBEDDED_SIGNATURE
    );
    
    EXPECT_FALSE(result.passed);
}

// Test: ELF format with external signature file
TEST_F(PluginSecurityImplementationTest, ELFFormatWithSignature) {
    // Create a minimal ELF header
    std::vector<uint8_t> elf_data;
    elf_data.push_back(0x7F);
    elf_data.push_back('E');
    elf_data.push_back('L');
    elf_data.push_back('F');
    
    // Add some dummy data
    for (int i = 0; i < 100; i++) {
        elf_data.push_back(0);
    }
    
    std::filesystem::path elf_file = test_dir_ / "test.so";
    std::ofstream file(elf_file, std::ios::binary);
    file.write(reinterpret_cast<const char*>(elf_data.data()), elf_data.size());
    file.close();
    
    // Create a signature file
    std::filesystem::path sig_file = test_dir_ / "test.so.sig";
    std::ofstream sig_stream(sig_file, std::ios::binary);
    sig_stream << "FAKE_SIGNATURE_DATA";
    sig_stream.close();
    
    PluginSecurityPolicy policy;
    policy.allowUnsigned = false;
    EnhancedPluginSecurityVerifier verifier(policy);
    
    // Should find the .sig file during embedded signature extraction
    // but fail validation (invalid signature format)
    auto result = verifier.verifyPlugin(
        elf_file.string(),
        EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_2_EMBEDDED_SIGNATURE
    );
    
    // May pass hash but fail signature verification
    EXPECT_TRUE(result.hash_verified);
}
