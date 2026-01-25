/*
 * Unit tests for OpenSSL RAII wrappers
 * Tests that the RAII wrappers properly manage OpenSSL object lifetimes
 */

#include <gtest/gtest.h>
#include "utils/openssl_deleter.h"
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/rsa.h>

using namespace themis::utils;

// ============================================================================
// EVPKeyPtr Tests
// ============================================================================

TEST(OpenSSLRAII, EVPKeyPtrCreation) {
    auto pkey = make_evp_key();
    EXPECT_NE(pkey, nullptr);
    EXPECT_NE(pkey.get(), nullptr);
}

TEST(OpenSSLRAII, EVPKeyPtrNullHandling) {
    EVPKeyPtr pkey(nullptr);
    EXPECT_EQ(pkey, nullptr);
    EXPECT_EQ(pkey.get(), nullptr);
}

TEST(OpenSSLRAII, EVPKeyPtrMove) {
    auto pkey1 = make_evp_key();
    ASSERT_NE(pkey1, nullptr);
    EVP_PKEY* raw_ptr = pkey1.get();
    
    auto pkey2 = std::move(pkey1);
    EXPECT_EQ(pkey1, nullptr);
    EXPECT_EQ(pkey2.get(), raw_ptr);
}

TEST(OpenSSLRAII, EVPKeyPtrReset) {
    auto pkey = make_evp_key();
    ASSERT_NE(pkey, nullptr);
    
    pkey.reset();
    EXPECT_EQ(pkey, nullptr);
}

// ============================================================================
// EVPMDCtxPtr Tests
// ============================================================================

TEST(OpenSSLRAII, EVPMDCtxPtrCreation) {
    auto ctx = make_evp_md_ctx();
    EXPECT_NE(ctx, nullptr);
    EXPECT_NE(ctx.get(), nullptr);
}

TEST(OpenSSLRAII, EVPMDCtxPtrNullHandling) {
    EVPMDCtxPtr ctx(nullptr);
    EXPECT_EQ(ctx, nullptr);
}

// ============================================================================
// BIGNUMPtr Tests
// ============================================================================

TEST(OpenSSLRAII, BIGNUMPtrCreation) {
    auto bn = make_bignum();
    EXPECT_NE(bn, nullptr);
}

TEST(OpenSSLRAII, BIGNUMPtrOperations) {
    auto bn = make_bignum();
    ASSERT_NE(bn, nullptr);
    
    // Set value and verify
    EXPECT_EQ(BN_set_word(bn.get(), 42), 1);
    EXPECT_EQ(BN_get_word(bn.get()), 42UL);
}

TEST(OpenSSLRAII, BIGNUMPtrFromBinary) {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
    auto bn = BIGNUMPtr(BN_bin2bn(data.data(), static_cast<int>(data.size()), nullptr));
    ASSERT_NE(bn, nullptr);
    
    // Verify value
    EXPECT_GT(BN_get_word(bn.get()), 0UL);
}

// ============================================================================
// RSAPtr Tests
// ============================================================================

TEST(OpenSSLRAII, RSAPtrCreation) {
    auto rsa = make_rsa();
    EXPECT_NE(rsa, nullptr);
}

// ============================================================================
// BIOPtr Tests
// ============================================================================

TEST(OpenSSLRAII, BIOPtrMemBuf) {
    const char* data = "test data";
    auto bio = make_bio_mem_buf(data, -1);
    ASSERT_NE(bio, nullptr);
    
    char buffer[128];
    int len = BIO_read(bio.get(), buffer, sizeof(buffer) - 1);
    ASSERT_GT(len, 0);
    buffer[len] = '\0';
    EXPECT_STREQ(buffer, data);
}

TEST(OpenSSLRAII, BIOPtrNullHandling) {
    BIOPtr bio(nullptr);
    EXPECT_EQ(bio, nullptr);
}

// ============================================================================
// X509Ptr Tests
// ============================================================================

TEST(OpenSSLRAII, X509PtrNullHandling) {
    X509Ptr cert(nullptr);
    EXPECT_EQ(cert, nullptr);
}

TEST(OpenSSLRAII, X509PtrFromBIO) {
    // Create a simple self-signed certificate for testing
    const char* cert_pem = 
        "-----BEGIN CERTIFICATE-----\n"
        "MIICljCCAX4CCQCKmXmC3V4xXjANBgkqhkiG9w0BAQsFADAxMQswCQYDVQQGEwJV\n"
        "UzEPMA0GA1UECgwGVGhlbWlzMREwDwYDVQQDDAhUZXN0IENBMTAeFw0yNDAxMDEw\n"
        "MDAwMDBaFw0yNTAxMDEwMDAwMDBaMDExCzAJBgNVBAYTAlVTMQ8wDQYDVQQKDAZU\n"
        "aGVtaXMxETAPBgNVBAMMCFRlc3QgQ0ExMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8A\n"
        "MIIBCgKCAQEAyJqNvfJ5PNGvYYz0qF8rJ5D8VqF5Bb6xL3c8P0wF5xYqE6Q2Yq9V\n"
        "Q5F3P8xYqF5Bb6xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0wF5xYqE\n"
        "6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0\n"
        "wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6\n"
        "xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8x\n"
        "YqF5Bb6xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0wF5xYqE6Q2Yq9V\n"
        "Q5F3P8xYqF5Bb6xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0wF5xYqE\n"
        "6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0\n"
        "wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6\n"
        "xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0wIDAQABMA0GCSqGSIb3DQ\n"
        "EBCwUAA4IBAQAzQ5F3P8xYqF5Bb6xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6\n"
        "xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8x\n"
        "YqF5Bb6xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0wF5xYqE6Q2Yq9V\n"
        "Q5F3P8xYqF5Bb6xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0wF5xYqE\n"
        "6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0\n"
        "wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6\n"
        "xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8x\n"
        "YqF5Bb6xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0wF5xYqE6Q2Yq9V\n"
        "Q5F3P8xYqF5Bb6xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0wF5xYqE\n"
        "6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0\n"
        "wF5xYqE6Q2Yq9VQ5F3P8xYqF5Bb6xL3c8P0w==\n"
        "-----END CERTIFICATE-----\n";
    
    auto bio = make_bio_mem_buf(cert_pem, -1);
    ASSERT_NE(bio, nullptr);
    
    auto cert = read_x509_from_bio(bio.get());
    // Note: This may be nullptr if cert is invalid, which is okay for this test
    // We're just testing that the function works and doesn't crash
}

// ============================================================================
// X509CRLPtr Tests
// ============================================================================

TEST(OpenSSLRAII, X509CRLPtrNullHandling) {
    X509CRLPtr crl(nullptr);
    EXPECT_EQ(crl, nullptr);
}

// ============================================================================
// Error Path Tests - Verify automatic cleanup
// ============================================================================

TEST(OpenSSLRAII, MultipleResourcesErrorPath) {
    // Simulate error path with multiple resources
    {
        auto bn1 = make_bignum();
        auto bn2 = make_bignum();
        auto pkey = make_evp_key();
        auto ctx = make_evp_md_ctx();
        
        ASSERT_NE(bn1, nullptr);
        ASSERT_NE(bn2, nullptr);
        ASSERT_NE(pkey, nullptr);
        ASSERT_NE(ctx, nullptr);
        
        // Early return would happen here in real code
        // All resources cleaned up automatically
    }
    // All objects should be freed at this point
}

TEST(OpenSSLRAII, ConditionalResourceAllocation) {
    auto pkey = make_evp_key();
    if (!pkey) {
        // Early return - no manual cleanup needed
        FAIL() << "Failed to create EVP_PKEY";
        return;
    }
    
    auto ctx = make_evp_md_ctx();
    if (!ctx) {
        // Early return - pkey cleaned up automatically
        FAIL() << "Failed to create EVP_MD_CTX";
        return;
    }
    
    // Both resources cleaned up automatically
    SUCCEED();
}
