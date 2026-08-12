#include <gtest/gtest.h>
#include "security/vcc_pki_client.h"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/rsa.h>

#include <chrono>
#include <filesystem>
#include <fstream>

using namespace themis;

namespace {

// Helper to generate a self-signed X.509 certificate with specific validity period
std::string generate_test_certificate(int days_before_now, int days_after_now) {
    // Create RSA key
    EVP_PKEY* pkey = EVP_PKEY_new();
    RSA* rsa = RSA_new();
    BIGNUM* e = BN_new();
    
    if (!pkey || !rsa || !e) {
        if (pkey) EVP_PKEY_free(pkey);
        if (rsa) RSA_free(rsa);
        if (e) BN_free(e);
        return "";
    }
    
    if (BN_set_word(e, RSA_F4) != 1) {
        EVP_PKEY_free(pkey);
        RSA_free(rsa);
        BN_free(e);
        return "";
    }
    
    if (RSA_generate_key_ex(rsa, 2048, e, nullptr) != 1) {
        EVP_PKEY_free(pkey);
        RSA_free(rsa);
        BN_free(e);
        return "";
    }
    
    EVP_PKEY_assign_RSA(pkey, rsa);
    rsa = nullptr;  // owned by pkey now
    
    // Create X.509 certificate
    X509* x509 = X509_new();
    if (!x509) {
        EVP_PKEY_free(pkey);
        BN_free(e);
        return "";
    }
    
    // Set version (v3)
    X509_set_version(x509, 2);
    
    // Set serial number
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 12345);
    
    // Set validity period
    X509_gmtime_adj(X509_get_notBefore(x509), -days_before_now * 24 * 3600);
    X509_gmtime_adj(X509_get_notAfter(x509), days_after_now * 24 * 3600);
    
    // Set subject
    X509_NAME* name = X509_get_subject_name(x509);
    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char*)"DE", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char*)"ThemisDB", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char*)"test-cert", -1, -1, 0);
    
    // Set issuer (self-signed)
    X509_set_issuer_name(x509, name);
    
    // Set public key
    X509_set_pubkey(x509, pkey);
    
    // Sign certificate
    X509_sign(x509, pkey, EVP_sha256());
    
    // Convert to PEM string
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio) {
        X509_free(x509);
        EVP_PKEY_free(pkey);
        BN_free(e);
        return "";
    }
    
    if (PEM_write_bio_X509(bio, x509) != 1) {
        BIO_free(bio);
        X509_free(x509);
        EVP_PKEY_free(pkey);
        BN_free(e);
        return "";
    }
    
    BUF_MEM* mem = nullptr;
    BIO_get_mem_ptr(bio, &mem);
    std::string pem(mem->data, mem->length);
    
    // Cleanup
    BIO_free(bio);
    X509_free(x509);
    EVP_PKEY_free(pkey);
    if (rsa) RSA_free(rsa);
    BN_free(e);
    
    return pem;
}

} // namespace

// Test that certificate timestamps are correctly parsed and not set to 0
TEST(VCCPKIClientTest, ParseCertificate_ExtractsValidTimestamps) {
    // Generate a test certificate valid for 30 days from now
    std::string pem = generate_test_certificate(0, 30);
    ASSERT_FALSE(pem.empty());
    
    TLSConfig tls_config;
    VCCPKIClient client("https://test.example.com", tls_config, 5000);
    
    X509Certificate cert = client.parseCertificate(pem);
    
    // Verify timestamps are not placeholders (0)
    EXPECT_NE(cert.not_before_ms, 0);
    EXPECT_NE(cert.not_after_ms, 0);
    
    // Verify not_before is before not_after
    EXPECT_LT(cert.not_before_ms, cert.not_after_ms);
    
    // Verify timestamps are reasonable (within last year and next year)
    int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    int64_t one_year_ms = 365LL * 24 * 3600 * 1000;
    EXPECT_GT(cert.not_before_ms, now_ms - one_year_ms);
    EXPECT_LT(cert.not_after_ms, now_ms + one_year_ms);
}

// Test that a valid certificate is recognized as valid
TEST(VCCPKIClientTest, ValidCertificate_IsValid) {
    // Generate certificate valid from 1 day ago to 30 days from now
    std::string pem = generate_test_certificate(1, 30);
    ASSERT_FALSE(pem.empty());
    
    TLSConfig tls_config;
    VCCPKIClient client("https://test.example.com", tls_config, 5000);
    
    X509Certificate cert = client.parseCertificate(pem);
    
    // Certificate should be currently valid
    EXPECT_TRUE(cert.isValid());
}

// Test that an expired certificate is recognized as expired
TEST(VCCPKIClientTest, ExpiredCertificate_IsNotValid) {
    // Generate certificate that expired 10 days ago (valid from 20 days ago to 10 days ago)
    std::string pem = generate_test_certificate(20, -10);
    ASSERT_FALSE(pem.empty());
    
    TLSConfig tls_config;
    VCCPKIClient client("https://test.example.com", tls_config, 5000);
    
    X509Certificate cert = client.parseCertificate(pem);
    
    // Certificate should not be valid
    EXPECT_FALSE(cert.isValid());
    
    // Certificate should be expired
    int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    EXPECT_TRUE(cert.isExpired(now_ms));
}

// Test that a not-yet-valid certificate is recognized as not valid
TEST(VCCPKIClientTest, NotYetValidCertificate_IsNotValid) {
    // Generate certificate that starts 10 days from now (valid from 10 days from now to 40 days from now)
    std::string pem = generate_test_certificate(-10, 40);
    ASSERT_FALSE(pem.empty());
    
    TLSConfig tls_config;
    VCCPKIClient client("https://test.example.com", tls_config, 5000);
    
    X509Certificate cert = client.parseCertificate(pem);
    
    // Certificate should not be valid yet
    EXPECT_FALSE(cert.isValid());
}

// Test that timestamp values are correctly serialized to JSON
TEST(VCCPKIClientTest, CertificateToJson_IncludesTimestamps) {
    std::string pem = generate_test_certificate(1, 30);
    ASSERT_FALSE(pem.empty());
    
    TLSConfig tls_config;
    VCCPKIClient client("https://test.example.com", tls_config, 5000);
    
    X509Certificate cert = client.parseCertificate(pem);
    
    nlohmann::json json = cert.toJson();
    
    // Verify JSON contains the timestamp fields
    ASSERT_TRUE(json.contains("not_before_ms"));
    ASSERT_TRUE(json.contains("not_after_ms"));
    
    // Verify values are not zero
    EXPECT_NE(json["not_before_ms"].get<int64_t>(), 0);
    EXPECT_NE(json["not_after_ms"].get<int64_t>(), 0);
    
    // Verify roundtrip: fromJson should restore the values
    X509Certificate cert_restored = X509Certificate::fromJson(json);
    EXPECT_EQ(cert_restored.not_before_ms, cert.not_before_ms);
    EXPECT_EQ(cert_restored.not_after_ms, cert.not_after_ms);
}

// Test certificate with subject and issuer information
TEST(VCCPKIClientTest, ParseCertificate_ExtractsSubjectAndIssuer) {
    std::string pem = generate_test_certificate(1, 30);
    ASSERT_FALSE(pem.empty());
    
    TLSConfig tls_config;
    VCCPKIClient client("https://test.example.com", tls_config, 5000);
    
    X509Certificate cert = client.parseCertificate(pem);
    
    // Verify basic fields are extracted
    EXPECT_FALSE(cert.id.empty());
    EXPECT_FALSE(cert.subject.empty());
    EXPECT_FALSE(cert.issuer.empty());
    EXPECT_FALSE(cert.pem.empty());
    
    // Verify subject contains expected values
    EXPECT_NE(cert.subject.find("ThemisDB"), std::string::npos);
    EXPECT_NE(cert.subject.find("test-cert"), std::string::npos);
}
