// test_plugin_security_implementation.cpp
// Tests for newly implemented security functions

#include <gtest/gtest.h>
#include "acceleration/plugin_security.h"
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <stdexcept>
#include <iomanip>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <sstream>
#include <vector>
#include <nlohmann/json.hpp>
#include "acceleration/plugin_loader.h"
#include "plugins/plugin_manager.h"

using namespace themis::acceleration;

namespace {
class ScopedPluginSignerPins {
public:
    explicit ScopedPluginSignerPins(const std::string& value) {
        const char* previous = std::getenv("THEMIS_PLUGIN_TRUSTED_SIGNER_FINGERPRINTS");
        if (previous) {
            had_previous_ = true;
            previous_ = previous;
        }
#ifdef _WIN32
        _putenv_s("THEMIS_PLUGIN_TRUSTED_SIGNER_FINGERPRINTS", value.c_str());
#else
        setenv("THEMIS_PLUGIN_TRUSTED_SIGNER_FINGERPRINTS", value.c_str(), 1);
#endif
    }

    ~ScopedPluginSignerPins() {
#ifdef _WIN32
        _putenv_s("THEMIS_PLUGIN_TRUSTED_SIGNER_FINGERPRINTS",
                  had_previous_ ? previous_.c_str() : "");
#else
        if (had_previous_) {
            setenv("THEMIS_PLUGIN_TRUSTED_SIGNER_FINGERPRINTS", previous_.c_str(), 1);
        } else {
            unsetenv("THEMIS_PLUGIN_TRUSTED_SIGNER_FINGERPRINTS");
        }
#endif
    }

private:
    std::string previous_;
    bool had_previous_ = false;
};
} // namespace

namespace themis::plugins {
struct PluginManagerTestAccess {
    static bool verifyPlugin(
        PluginManager& manager,
        const std::string& path,
        std::string& error_message) {
        return manager.verifyPlugin(path, error_message);
    }
};
} // namespace themis::plugins

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
        X509_gmtime_adj(X509_get_notBefore(x509), -60);
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

        BIO* key_bio = BIO_new(BIO_s_mem());
        PEM_write_bio_PrivateKey(key_bio, pkey, nullptr, nullptr, 0, nullptr, nullptr);
        BUF_MEM* key_mem = nullptr;
        BIO_get_mem_ptr(key_bio, &key_mem);
        test_private_key_pem_ = std::string(key_mem->data, key_mem->length);
        BIO_free(key_bio);

        unsigned char digest[EVP_MAX_MD_SIZE]{};
        unsigned int digest_length = 0;
        if (X509_digest(x509, EVP_sha256(), digest, &digest_length) == 1) {
            std::ostringstream fingerprint;
            fingerprint << std::hex << std::setfill('0');
            for (unsigned int index = 0; index < digest_length; ++index) {
                fingerprint << std::setw(2) << static_cast<unsigned int>(digest[index]);
            }
            test_cert_fingerprint_ = fingerprint.str();
        }
        
        // Cleanup
        BIO_free(bio);
        X509_free(x509);
        EVP_PKEY_free(pkey);
    }

    void writeSignedPluginMetadata() {
        PluginSecurityVerifier verifier(PluginSecurityPolicy{});
        const std::string file_hash = verifier.calculateFileHash(test_plugin_path_.string());
        std::vector<uint8_t> hash_bytes;
        for (std::size_t offset = 0; offset + 1 < file_hash.size(); offset += 2) {
            hash_bytes.push_back(static_cast<uint8_t>(
                std::stoul(file_hash.substr(offset, 2), nullptr, 16)));
        }

        BIO* key_bio = BIO_new_mem_buf(
            test_private_key_pem_.data(), static_cast<int>(test_private_key_pem_.size()));
        EVP_PKEY* key = PEM_read_bio_PrivateKey(key_bio, nullptr, nullptr, nullptr);
        BIO_free(key_bio);
        ASSERT_NE(key, nullptr);

        EVP_MD_CTX* context = EVP_MD_CTX_new();
        ASSERT_NE(context, nullptr);
        ASSERT_EQ(EVP_DigestSignInit(context, nullptr, EVP_sha256(), nullptr, key), 1);
        std::size_t signature_size = 0;
        ASSERT_EQ(EVP_DigestSign(
                      context, nullptr, &signature_size, hash_bytes.data(), hash_bytes.size()),
                  1);
        std::vector<uint8_t> signature_bytes(signature_size);
        ASSERT_EQ(EVP_DigestSign(
                      context, signature_bytes.data(), &signature_size,
                      hash_bytes.data(), hash_bytes.size()),
                  1);
        signature_bytes.resize(signature_size);
        EVP_MD_CTX_free(context);
        EVP_PKEY_free(key);

        std::ostringstream signature_hex;
        signature_hex << std::hex << std::setfill('0');
        for (const auto byte : signature_bytes) {
            signature_hex << std::setw(2) << static_cast<unsigned int>(byte);
        }

        nlohmann::json metadata = {
            {"plugin",
             {{"signature",
               {{"sha256", file_hash},
                {"signature", signature_hex.str()},
                {"certificate", test_cert_pem_},
                {"issuer", "CN=ThemisDB Official Plugins, O=ThemisDB, C=DE"}}}}}};
        std::ofstream metadata_file(test_plugin_path_.string() + ".json");
        metadata_file << metadata.dump();
    }
    
    std::filesystem::path test_dir_;
    std::filesystem::path test_plugin_path_;
    std::string test_cert_pem_;
    std::string test_private_key_pem_;
    std::string test_cert_fingerprint_;
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

TEST_F(PluginSecurityImplementationTest, PluginSignatureRejectsUnknownSignerFingerprint) {
    writeSignedPluginMetadata();
    PluginSecurityPolicy policy;
    policy.trustedSignerFingerprints = {std::string(64, '0')};
    PluginSecurityVerifier verifier(policy);

    std::string error_message;
    EXPECT_FALSE(verifier.verifyPlugin(test_plugin_path_.string(), error_message));
    EXPECT_EQ(error_message, "Plugin signer identity is not pinned");
}

TEST_F(PluginSecurityImplementationTest, PluginSignatureAcceptsPinnedSignerFingerprint) {
    writeSignedPluginMetadata();
    ASSERT_EQ(test_cert_fingerprint_.size(), 64u);
    PluginSecurityPolicy policy;
    policy.trustedSignerFingerprints = {
        std::string(64, '0'), test_cert_fingerprint_};
    PluginSecurityVerifier verifier(policy);

    std::string error_message;
    EXPECT_TRUE(verifier.verifyPlugin(test_plugin_path_.string(), error_message))
        << error_message;
}

TEST_F(PluginSecurityImplementationTest, TrustLevelRejectsTrustedIssuerWithoutSignerPin) {
    PluginSecurityPolicy policy;
    policy.trustedIssuers = {"CN=ThemisDB Test CA"};
    PluginSecurityVerifier verifier(policy);
    PluginMetadata metadata;
    metadata.signature.verified = true;
    metadata.signature.signingCertificate = test_cert_pem_;
    metadata.signature.issuer = "CN=ThemisDB Test CA";

    EXPECT_EQ(verifier.getTrustLevel(metadata), PluginTrustLevel::UNTRUSTED);
}

TEST_F(PluginSecurityImplementationTest, PluginManagerRejectsUnsignedPlugin) {
    themis::plugins::PluginManager manager;
    std::string error_message;

    EXPECT_FALSE(themis::plugins::PluginManagerTestAccess::verifyPlugin(
        manager, test_plugin_path_.string(), error_message));
    EXPECT_NE(error_message.find("signature"), std::string::npos);
}

TEST_F(PluginSecurityImplementationTest, PluginManagerUsesConfiguredSignerPins) {
    writeSignedPluginMetadata();
    ScopedPluginSignerPins pins(test_cert_fingerprint_);
    themis::plugins::PluginManager manager;
    std::string error_message;

    EXPECT_TRUE(themis::plugins::PluginManagerTestAccess::verifyPlugin(
        manager, test_plugin_path_.string(), error_message))
        << error_message;
}

TEST_F(PluginSecurityImplementationTest, PluginManagerRejectsUnknownConfiguredSigner) {
    writeSignedPluginMetadata();
    ScopedPluginSignerPins pins(std::string(64, '0'));
    themis::plugins::PluginManager manager;
    std::string error_message;

    EXPECT_FALSE(themis::plugins::PluginManagerTestAccess::verifyPlugin(
        manager, test_plugin_path_.string(), error_message));
    EXPECT_EQ(error_message, "Plugin signer identity is not pinned");
}

TEST_F(PluginSecurityImplementationTest, PluginLoaderRejectsUnknownConfiguredSigner) {
    writeSignedPluginMetadata();
    ScopedPluginSignerPins pins(std::string(64, '0'));
    themis::acceleration::PluginLoader loader;

    EXPECT_FALSE(loader.loadPlugin(test_plugin_path_.string()));
}

TEST_F(PluginSecurityImplementationTest, PluginManagerRejectsMalformedConfiguredSigner) {
    ScopedPluginSignerPins pins("not-a-fingerprint");
    EXPECT_THROW(
        {
            themis::plugins::PluginManager manager;
        },
        std::invalid_argument);
}

TEST_F(PluginSecurityImplementationTest, VerifierRejectsDisabledSignaturePolicy) {
    PluginSecurityPolicy policy;
    policy.requireSignature = false;
    policy.allowUnsigned = true;
    PluginSecurityVerifier verifier(policy);

    std::string error_message;
    EXPECT_FALSE(verifier.verifyPlugin(test_plugin_path_.string(), error_message));
    EXPECT_EQ(error_message, "Plugin signature verification cannot be disabled");
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
TEST_F(PluginSecurityImplementationTest, VerifyFullChain_UnsignedRejected) {
    PluginSecurityPolicy policy;
    policy.allowUnsigned = true;
    policy.checkRevocation = false;  // Disable revocation checking for test
    
    EnhancedPluginSecurityVerifier verifier(policy);
    
    auto result = verifier.verifyPlugin(
        test_plugin_path_.string(),
        EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_4_FULL_CHAIN
    );
    EXPECT_FALSE(result.passed);

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
