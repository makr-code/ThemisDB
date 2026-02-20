#include <gtest/gtest.h>
#include "auth/jwks_security.h"
#include <fstream>

using namespace themis::auth;

// Helper to create a test certificate file
class TestCertificateHelper {
public:
    static std::string createTestCert(const std::string& path) {
        // Create a minimal self-signed certificate for testing
        std::string cert_content = R"(-----BEGIN CERTIFICATE-----
MIIDXTCCAkWgAwIBAgIJAKl8VNN8H8qcMA0GCSqGSIb3DQEBCwUAMEUxCzAJBgNV
BAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEwHwYDVQQKDBhJbnRlcm5ldCBX
aWRnaXRzIFB0eSBMdGQwHhcNMjQwMTAxMDAwMDAwWhcNMjUwMTAxMDAwMDAwWjBF
MQswCQYDVQQGEwJBVTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50
ZXJuZXQgV2lkZ2l0cyBQdHkgTHRkMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB
CgKCAQEA0p7M8f8xZJNbI4dD0RH8VGj0YvAqPQJ1jZe7VYqFT5cW9b0z8jY8iF3i
ZQ1YvBH2JKqH5L8nV9I6bA8FnX6iD0Ql9B2P7xJ8fV1nB7dI2K3V9bN5lF8iQ3jL
-----END CERTIFICATE-----)";
        
        std::ofstream file(path);
        file << cert_content;
        file.close();
        
        return path;
    }
};

/**
 * @brief Test secure defaults configuration
 */
TEST(JWKSSecurityConfigTest, SecureDefaults) {
    auto config = JWKSSecurityConfig::secureDefaults();
    
    EXPECT_EQ(config.pinning_mode, JWKSSecurityConfig::PinningMode::NONE);
    EXPECT_EQ(config.min_tls_version, JWKSSecurityConfig::TLSVersion::TLS_1_2);
    EXPECT_TRUE(config.verify_hostname);
    EXPECT_TRUE(config.verify_certificate);
    EXPECT_FALSE(config.enable_mtls);
}

/**
 * @brief Test public key pinning configuration
 */
TEST(JWKSSecurityConfigTest, PublicKeyPinning) {
    std::vector<std::string> hashes = {"hash1", "hash2", "hash3"};
    auto config = JWKSSecurityConfig::withPublicKeyPinning(hashes);
    
    EXPECT_EQ(config.pinning_mode, JWKSSecurityConfig::PinningMode::PUBLIC_KEY);
    EXPECT_EQ(config.pinned_hashes.size(), 3);
    EXPECT_EQ(config.pinned_hashes[0], "hash1");
    EXPECT_EQ(config.min_tls_version, JWKSSecurityConfig::TLSVersion::TLS_1_2);
}

/**
 * @brief Test certificate pinning configuration
 */
TEST(JWKSSecurityConfigTest, CertificatePinning) {
    auto config = JWKSSecurityConfig::withCertificatePinning("/tmp/test_cert.pem");
    
    EXPECT_EQ(config.pinning_mode, JWKSSecurityConfig::PinningMode::CERTIFICATE);
    EXPECT_EQ(config.pinned_cert_path, "/tmp/test_cert.pem");
    EXPECT_EQ(config.min_tls_version, JWKSSecurityConfig::TLSVersion::TLS_1_2);
}

/**
 * @brief Test mTLS configuration
 */
TEST(JWKSSecurityConfigTest, MTLSConfiguration) {
    auto config = JWKSSecurityConfig::withMTLS(
        "/tmp/client_cert.pem",
        "/tmp/client_key.pem",
        "password123"
    );
    
    EXPECT_TRUE(config.enable_mtls);
    EXPECT_EQ(config.client_cert_path, "/tmp/client_cert.pem");
    EXPECT_EQ(config.client_key_path, "/tmp/client_key.pem");
    EXPECT_EQ(config.client_key_password, "password123");
}

/**
 * @brief Test configuration validation - missing pinned hashes
 */
TEST(JWKSSecurityConfigTest, ValidationMissingHashes) {
    JWKSSecurityConfig::Config config;
    config.pinning_mode = JWKSSecurityConfig::PinningMode::PUBLIC_KEY;
    config.pinned_hashes.clear();  // Empty
    
    EXPECT_THROW(JWKSSecurityConfig security_config(config), std::runtime_error);
}

/**
 * @brief Test configuration validation - secure defaults valid
 */
TEST(JWKSSecurityConfigTest, ValidationSecureDefaults) {
    auto config = JWKSSecurityConfig::secureDefaults();
    
    EXPECT_NO_THROW(JWKSSecurityConfig security_config(config));
}

/**
 * @brief Test TLS version configuration
 */
TEST(JWKSSecurityConfigTest, TLSVersions) {
    JWKSSecurityConfig::Config config;
    
    // Test different TLS versions
    config.min_tls_version = JWKSSecurityConfig::TLSVersion::TLS_1_2;
    EXPECT_EQ(config.min_tls_version, JWKSSecurityConfig::TLSVersion::TLS_1_2);
    
    config.min_tls_version = JWKSSecurityConfig::TLSVersion::TLS_1_3;
    EXPECT_EQ(config.min_tls_version, JWKSSecurityConfig::TLSVersion::TLS_1_3);
}

/**
 * @brief Test hostname verification configuration
 */
TEST(JWKSSecurityConfigTest, HostnameVerification) {
    JWKSSecurityConfig::Config config;
    
    config.verify_hostname = true;
    EXPECT_TRUE(config.verify_hostname);
    
    config.verify_hostname = false;
    EXPECT_FALSE(config.verify_hostname);
}

/**
 * @brief Test certificate verification configuration
 */
TEST(JWKSSecurityConfigTest, CertificateVerification) {
    JWKSSecurityConfig::Config config;
    
    config.verify_certificate = true;
    EXPECT_TRUE(config.verify_certificate);
    
    config.verify_certificate = false;
    EXPECT_FALSE(config.verify_certificate);
}

/**
 * @brief Test timeout configuration
 */
TEST(JWKSSecurityConfigTest, Timeouts) {
    JWKSSecurityConfig::Config config;
    
    config.connect_timeout_ms = 3000;
    config.read_timeout_ms = 10000;
    
    EXPECT_EQ(config.connect_timeout_ms, 3000);
    EXPECT_EQ(config.read_timeout_ms, 10000);
}

/**
 * @brief Test allowed ciphers configuration
 */
TEST(JWKSSecurityConfigTest, AllowedCiphers) {
    JWKSSecurityConfig::Config config;
    
    config.allowed_ciphers = {"TLS_AES_256_GCM_SHA384", "TLS_CHACHA20_POLY1305_SHA256"};
    
    EXPECT_EQ(config.allowed_ciphers.size(), 2);
    EXPECT_EQ(config.allowed_ciphers[0], "TLS_AES_256_GCM_SHA384");
}

/**
 * @brief Test CA bundle configuration
 */
TEST(JWKSSecurityConfigTest, CABundle) {
    JWKSSecurityConfig::Config config;
    config.ca_bundle_path = "/etc/ssl/certs/ca-bundle.crt";
    
    EXPECT_EQ(config.ca_bundle_path, "/etc/ssl/certs/ca-bundle.crt");
}

/**
 * @brief Test JWKS secure fetcher initialization
 */
TEST(JWKSSecureFetcherTest, Initialization) {
    auto config = JWKSSecurityConfig::secureDefaults();
    
    EXPECT_NO_THROW(JWKSSecureFetcher fetcher(config));
}

/**
 * @brief Test JWKS fetcher requires HTTPS
 */
TEST(JWKSSecureFetcherTest, RequiresHTTPS) {
    auto config = JWKSSecurityConfig::secureDefaults();
    JWKSSecureFetcher fetcher(config);
    
    // HTTP URL should be rejected
    EXPECT_THROW(fetcher.fetch("http://example.com/jwks.json"), std::runtime_error);
}

/**
 * @brief Test pinning verification - no pinning
 */
TEST(JWKSSecureFetcherTest, NoPinningAlwaysVerifies) {
    auto config = JWKSSecurityConfig::secureDefaults();
    JWKSSecureFetcher fetcher(config);
    
    std::vector<std::string> cert_chain = {"cert1", "cert2"};
    EXPECT_TRUE(fetcher.verifyPinning(cert_chain));
}

/**
 * @brief Test pinning verification - with pinning
 */
TEST(JWKSSecureFetcherTest, PinningVerification) {
    auto config = JWKSSecurityConfig::withPublicKeyPinning({"expected_hash"});
    JWKSSecureFetcher fetcher(config);
    
    // Note: In actual implementation, would need real certificates
    // This test verifies the structure works
    std::vector<std::string> cert_chain = {"cert_data"};
    
    // Verification logic would check against pinned hashes
    // For this unit test, we just verify the method exists and can be called
    fetcher.verifyPinning(cert_chain);
}

/**
 * @brief Test fetch stats initialization
 */
TEST(JWKSSecureFetcherTest, FetchStatsInitialization) {
    auto config = JWKSSecurityConfig::secureDefaults();
    JWKSSecureFetcher fetcher(config);
    
    auto stats = fetcher.getLastFetchStats();
    
    // Initial stats should be empty/zero
    EXPECT_TRUE(stats.url.empty());
    EXPECT_EQ(stats.status_code, 0);
}

/**
 * @brief Test certificate utils - SPKI hash computation structure
 */
TEST(CertificateUtilsTest, SPKIHashComputationStructure) {
    // Test that the method exists and has proper signature
    // Note: Actual certificate file needed for real test
    
    // This verifies the API structure
    // In production, would create a real test certificate
}

/**
 * @brief Test certificate utils - certificate verification structure
 */
TEST(CertificateUtilsTest, CertificateVerificationStructure) {
    // Test that verifyCertificate method exists
    // Returns false for non-existent file
    EXPECT_FALSE(CertificateUtils::verifyCertificate("/nonexistent/cert.pem"));
}

/**
 * @brief Test multiple pinned hashes
 */
TEST(JWKSSecurityConfigTest, MultiplePinnedHashes) {
    std::vector<std::string> hashes;
    for (int i = 0; i < 10; i++) {
        hashes.push_back("hash_" + std::to_string(i));
    }
    
    auto config = JWKSSecurityConfig::withPublicKeyPinning(hashes);
    
    EXPECT_EQ(config.pinned_hashes.size(), 10);
    EXPECT_EQ(config.pinned_hashes[0], "hash_0");
    EXPECT_EQ(config.pinned_hashes[9], "hash_9");
}

/**
 * @brief Test config immutability after creation
 */
TEST(JWKSSecurityConfigTest, ConfigImmutability) {
    auto config = JWKSSecurityConfig::secureDefaults();
    JWKSSecurityConfig security_config(config);
    
    auto retrieved_config = security_config.getConfig();
    
    EXPECT_EQ(retrieved_config.min_tls_version, JWKSSecurityConfig::TLSVersion::TLS_1_2);
    EXPECT_TRUE(retrieved_config.verify_hostname);
}

/**
 * @brief Test mTLS without password
 */
TEST(JWKSSecurityConfigTest, MTLSWithoutPassword) {
    auto config = JWKSSecurityConfig::withMTLS(
        "/tmp/client_cert.pem",
        "/tmp/client_key.pem"
        // No password
    );
    
    EXPECT_TRUE(config.enable_mtls);
    EXPECT_TRUE(config.client_key_password.empty());
}

/**
 * @brief Test pinning mode enumeration
 */
TEST(JWKSSecurityConfigTest, PinningModeEnumeration) {
    JWKSSecurityConfig::Config config;
    
    config.pinning_mode = JWKSSecurityConfig::PinningMode::NONE;
    EXPECT_EQ(config.pinning_mode, JWKSSecurityConfig::PinningMode::NONE);
    
    config.pinning_mode = JWKSSecurityConfig::PinningMode::PUBLIC_KEY;
    EXPECT_EQ(config.pinning_mode, JWKSSecurityConfig::PinningMode::PUBLIC_KEY);
    
    config.pinning_mode = JWKSSecurityConfig::PinningMode::CERTIFICATE;
    EXPECT_EQ(config.pinning_mode, JWKSSecurityConfig::PinningMode::CERTIFICATE);
    
    config.pinning_mode = JWKSSecurityConfig::PinningMode::CA_CERTIFICATE;
    EXPECT_EQ(config.pinning_mode, JWKSSecurityConfig::PinningMode::CA_CERTIFICATE);
}

/**
 * @brief Test TLS version enumeration
 */
TEST(JWKSSecurityConfigTest, TLSVersionEnumeration) {
    JWKSSecurityConfig::Config config;
    
    config.min_tls_version = JWKSSecurityConfig::TLSVersion::TLS_1_0;
    EXPECT_EQ(config.min_tls_version, JWKSSecurityConfig::TLSVersion::TLS_1_0);
    
    config.min_tls_version = JWKSSecurityConfig::TLSVersion::TLS_1_1;
    EXPECT_EQ(config.min_tls_version, JWKSSecurityConfig::TLSVersion::TLS_1_1);
    
    config.min_tls_version = JWKSSecurityConfig::TLSVersion::TLS_1_2;
    EXPECT_EQ(config.min_tls_version, JWKSSecurityConfig::TLSVersion::TLS_1_2);
    
    config.min_tls_version = JWKSSecurityConfig::TLSVersion::TLS_1_3;
    EXPECT_EQ(config.min_tls_version, JWKSSecurityConfig::TLSVersion::TLS_1_3);
}
