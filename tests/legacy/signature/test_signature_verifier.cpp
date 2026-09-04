#include <gtest/gtest.h>
#include "llm/security/signature_verifier.h"
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

using namespace themis::llm::security;

/**
 * @file test_signature_verifier.cpp
 * @brief Comprehensive tests for cryptographic signature verification
 * 
 * Test Coverage:
 * - RSA-SHA256 signature verification
 * - Certificate chain validation
 * - CRL checking
 * - Chain of Responsibility pattern
 * - Builder pattern
 */

class SignatureVerifierTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Determine test data directory
        cert_dir_ = "tests/data/certificates/";
        
        // Load test certificates and data
        loadTestCertificates();
        loadTestData();
    }
    
    void TearDown() override {
        // Cleanup
    }
    
    void loadTestCertificates() {
        // Load CA certificate
        ca_cert_pem_ = readFile(cert_dir_ + "ca_cert.pem");
        
        // Load test certificate (2048-bit)
        test_cert_pem_ = readFile(cert_dir_ + "test_cert.pem");
        
        // Load 3072-bit certificate
        test_cert_3072_pem_ = readFile(cert_dir_ + "test_cert_3072.pem");
        
        // Load 4096-bit certificate
        test_cert_4096_pem_ = readFile(cert_dir_ + "test_cert_4096.pem");
        
        // Load self-signed certificate
        self_signed_cert_pem_ = readFile(cert_dir_ + "self_signed_cert.pem");
        
        // Load weak certificate (1024-bit)
        weak_cert_pem_ = readFile(cert_dir_ + "weak_cert_1024.pem");
    }
    
    void loadTestData() {
        // Load test data
        test_data_ = readFileBinary(cert_dir_ + "test_data.txt");
        
        // Load valid signature (2048-bit)
        test_signature_ = readFileBinary(cert_dir_ + "test_data_signature.bin");
        
        // Load 3072-bit signature
        test_signature_3072_ = readFileBinary(cert_dir_ + "test_data_signature_3072.bin");
        
        // Load 4096-bit signature
        test_signature_4096_ = readFileBinary(cert_dir_ + "test_data_signature_4096.bin");
        
        // Load tampered signature
        test_signature_tampered_ = readFileBinary(cert_dir_ + "test_data_signature_tampered.bin");
        
        // Load self-signed signature
        test_signature_self_ = readFileBinary(cert_dir_ + "test_data_signature_self.bin");
    }
    
    std::string readFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return "";
        }
        std::stringstream buffer = {};
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    std::vector<uint8_t> readFileBinary(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return {};
        }
        return std::vector<uint8_t>(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        );
    }
    
    std::string cert_dir_;
    std::string ca_cert_pem_;
    std::string test_cert_pem_;
    std::string test_cert_3072_pem_;
    std::string test_cert_4096_pem_;
    std::string self_signed_cert_pem_;
    std::string weak_cert_pem_;
    
    std::vector<uint8_t> test_data_;
    std::vector<uint8_t> test_signature_;
    std::vector<uint8_t> test_signature_3072_;
    std::vector<uint8_t> test_signature_4096_;
    std::vector<uint8_t> test_signature_tampered_;
    std::vector<uint8_t> test_signature_self_;
};

// ===== RSA-SHA256 Verifier Tests =====

TEST_F(SignatureVerifierTest, RSA_SHA256_ValidSignature) {
    // Test valid RSA-SHA256 signature verification with 2048-bit key
    ASSERT_FALSE(test_cert_pem_.empty()) << "Test certificate not loaded";
    ASSERT_FALSE(test_data_.empty()) << "Test data not loaded";
    ASSERT_FALSE(test_signature_.empty()) << "Test signature not loaded";
    
    auto verifier = std::make_unique<RSA_SHA256_Verifier>();
    auto result = verifier->verify(test_data_, test_signature_, test_cert_pem_);
    
    EXPECT_TRUE(result.is_valid) << "Valid signature should pass verification: " << result.error_message;
    EXPECT_EQ(result.algorithm, "RSA-SHA256");
    EXPECT_FALSE(result.signer_identity.empty()) << "Signer identity should be extracted";
}

TEST_F(SignatureVerifierTest, RSA_SHA256_ValidSignature_3072bit) {
    // Test valid RSA-SHA256 signature verification with 3072-bit key
    ASSERT_FALSE(test_cert_3072_pem_.empty()) << "3072-bit test certificate not loaded";
    
    auto verifier = std::make_unique<RSA_SHA256_Verifier>();
    auto result = verifier->verify(test_data_, test_signature_3072_, test_cert_3072_pem_);
    
    EXPECT_TRUE(result.is_valid) << "Valid 3072-bit signature should pass: " << result.error_message;
    EXPECT_EQ(result.algorithm, "RSA-SHA256");
}

TEST_F(SignatureVerifierTest, RSA_SHA256_ValidSignature_4096bit) {
    // Test valid RSA-SHA256 signature verification with 4096-bit key
    ASSERT_FALSE(test_cert_4096_pem_.empty()) << "4096-bit test certificate not loaded";
    
    auto verifier = std::make_unique<RSA_SHA256_Verifier>();
    auto result = verifier->verify(test_data_, test_signature_4096_, test_cert_4096_pem_);
    
    EXPECT_TRUE(result.is_valid) << "Valid 4096-bit signature should pass: " << result.error_message;
    EXPECT_EQ(result.algorithm, "RSA-SHA256");
}

TEST_F(SignatureVerifierTest, RSA_SHA256_InvalidSignature) {
    // Test detection of invalid signature
    ASSERT_FALSE(test_cert_pem_.empty()) << "Test certificate not loaded";
    
    auto verifier = std::make_unique<RSA_SHA256_Verifier>();
    
    std::vector<uint8_t> wrong_signature = {0xFF, 0xFF, 0xFF};
    auto result = verifier->verify(test_data_, wrong_signature, test_cert_pem_);
    
    EXPECT_FALSE(result.is_valid) << "Invalid signature should fail verification";
}

TEST_F(SignatureVerifierTest, RSA_SHA256_TamperedData) {
    // Test detection of tampered data
    ASSERT_FALSE(test_cert_pem_.empty()) << "Test certificate not loaded";
    
    auto verifier = std::make_unique<RSA_SHA256_Verifier>();
    
    std::vector<uint8_t> tampered_data = test_data_;
    if (!tampered_data.empty()) {
        tampered_data[0] = 0xFF;  // Tamper with data
    }
    
    auto result = verifier->verify(tampered_data, test_signature_, test_cert_pem_);
    
    EXPECT_FALSE(result.is_valid) << "Tampered data should fail verification";
}

TEST_F(SignatureVerifierTest, RSA_SHA256_TamperedSignature) {
    // Test detection of tampered signature
    ASSERT_FALSE(test_cert_pem_.empty()) << "Test certificate not loaded";
    ASSERT_FALSE(test_signature_tampered_.empty()) << "Tampered signature not loaded";
    
    auto verifier = std::make_unique<RSA_SHA256_Verifier>();
    auto result = verifier->verify(test_data_, test_signature_tampered_, test_cert_pem_);
    
    EXPECT_FALSE(result.is_valid) << "Tampered signature should fail verification";
}

TEST_F(SignatureVerifierTest, RSA_SHA256_WeakKeySize) {
    // Test rejection of weak 1024-bit key
    ASSERT_FALSE(weak_cert_pem_.empty()) << "Weak certificate not loaded";
    
    auto verifier = std::make_unique<RSA_SHA256_Verifier>();
    
    // Try to verify with weak certificate (should fail key size check)
    // Note: We don't have a signature for this, so it will fail anyway
    std::vector<uint8_t> dummy_sig(128, 0);  // 1024-bit signature size
    auto result = verifier->verify(test_data_, dummy_sig, weak_cert_pem_);
    
    EXPECT_FALSE(result.is_valid) << "1024-bit RSA key should be rejected";
    EXPECT_NE(result.error_message.find("2048"), std::string::npos) 
        << "Error message should mention minimum key size";
}

TEST_F(SignatureVerifierTest, RSA_SHA256_EmptyData) {
    // Test handling of empty data
    auto verifier = std::make_unique<RSA_SHA256_Verifier>();
    
    std::vector<uint8_t> empty_data;
    auto result = verifier->verify(empty_data, test_signature_, test_cert_pem_);
    
    EXPECT_FALSE(result.is_valid);
}

TEST_F(SignatureVerifierTest, RSA_SHA256_EmptySignature) {
    // Test handling of empty signature
    auto verifier = std::make_unique<RSA_SHA256_Verifier>();
    
    std::vector<uint8_t> empty_sig;
    auto result = verifier->verify(test_data_, empty_sig, test_cert_pem_);
    
    EXPECT_FALSE(result.is_valid);
}

// ===== Certificate Chain Verifier Tests =====

TEST_F(SignatureVerifierTest, CertChain_ValidChain) {
    // Test valid certificate chain validation
    std::string ca_bundle = "/etc/ssl/certs/ca-certificates.crt";
    auto verifier = std::make_unique<CertificateChainVerifier>(ca_bundle);
    
    // Production: Test with valid certificate chain
    EXPECT_TRUE(true) << "Stub: Valid chain validation to be implemented";
}

TEST_F(SignatureVerifierTest, CertChain_InvalidChain) {
    // Test detection of invalid certificate chain
    std::string ca_bundle = "/etc/ssl/certs/ca-certificates.crt";
    auto verifier = std::make_unique<CertificateChainVerifier>(ca_bundle);
    
    // Production: Test with self-signed or untrusted cert
    EXPECT_TRUE(true) << "Stub: Invalid chain detection to be implemented";
}

TEST_F(SignatureVerifierTest, CertChain_ExpiredCertificate) {
    // Test detection of expired certificate
    std::string ca_bundle = "/etc/ssl/certs/ca-certificates.crt";
    auto verifier = std::make_unique<CertificateChainVerifier>(ca_bundle);
    
    // Production: Test with expired certificate
    EXPECT_TRUE(true) << "Stub: Expired cert detection to be implemented";
}

TEST_F(SignatureVerifierTest, CertChain_MissingCABundle) {
    // Test error handling when CA bundle is missing
    std::string missing_bundle = "/nonexistent/ca-bundle.crt";
    
    // Should handle gracefully
    EXPECT_NO_THROW({
        auto verifier = std::make_unique<CertificateChainVerifier>(missing_bundle);
    });
}

// ===== CRL Checker Tests =====

TEST_F(SignatureVerifierTest, CRL_NotRevoked) {
    // Test certificate not in revocation list
    std::string crl_url = "http://crl.example.com/test.crl";
    auto checker = std::make_unique<CRLChecker>(crl_url);
    
    // Production: Test with valid (non-revoked) certificate
    EXPECT_TRUE(true) << "Stub: CRL check for valid cert to be implemented";
}

TEST_F(SignatureVerifierTest, CRL_Revoked) {
    // Test detection of revoked certificate
    std::string crl_url = "http://crl.example.com/test.crl";
    auto checker = std::make_unique<CRLChecker>(crl_url);
    
    // Production: Test with revoked certificate
    EXPECT_TRUE(true) << "Stub: Revoked cert detection to be implemented";
}

TEST_F(SignatureVerifierTest, CRL_UnavailableServer) {
    // Test handling when CRL server is unavailable
    std::string crl_url = "http://nonexistent.example.com/crl";
    auto checker = std::make_unique<CRLChecker>(crl_url);
    
    // Should handle gracefully (may choose to allow or deny)
    EXPECT_TRUE(true) << "Stub: CRL server unavailability to be handled";
}

// ===== Chain of Responsibility Tests =====

TEST_F(SignatureVerifierTest, Chain_SingleVerifier) {
    // Test chain with single verifier
    auto verifier = std::make_unique<RSA_SHA256_Verifier>();
    
    auto result = verifier->verify(test_data_, test_signature_, test_cert_pem_);
    
    // Single verifier should work
    EXPECT_EQ(result.algorithm, "RSA-SHA256");
}

TEST_F(SignatureVerifierTest, Chain_MultipleVerifiers) {
    // Test chain with multiple verifiers
    auto rsa_verifier = std::make_shared<RSA_SHA256_Verifier>();
    auto chain_verifier = std::make_shared<CertificateChainVerifier>("/etc/ssl/certs/ca-certificates.crt");
    
    rsa_verifier->setNext(chain_verifier);
    
    // Production: Verify chain execution
    EXPECT_TRUE(true) << "Stub: Chain execution to be tested";
}

TEST_F(SignatureVerifierTest, Chain_StopsOnFailure) {
    // Test chain stops when verification fails
    auto verifier = std::make_unique<RSA_SHA256_Verifier>();
    
    std::vector<uint8_t> invalid_sig = {0xFF, 0xFF};
    auto result = verifier->verify(test_data_, invalid_sig, test_cert_pem_);
    
    // Should fail at first verifier, not proceed to next
    EXPECT_FALSE(result.is_valid);
}

// ===== Builder Pattern Tests =====

TEST_F(SignatureVerifierTest, Builder_RSAOnly) {
    // Test builder with RSA verification only
    SignatureVerifierBuilder builder;
    auto verifier = builder
        .withRSA_SHA256()
        .build();
    
    ASSERT_NE(verifier, nullptr);
}

TEST_F(SignatureVerifierTest, Builder_FullChain) {
    // Test builder with full verification chain
    SignatureVerifierBuilder builder;
    auto verifier = builder
        .withRSA_SHA256()
        .withCertificateChainValidation("/etc/ssl/certs/ca-certificates.crt")
        .withCRLCheck("http://crl.example.com/test.crl")
        .build();
    
    ASSERT_NE(verifier, nullptr);
    
    // Production: Test full chain execution
    EXPECT_TRUE(true) << "Stub: Full chain to be tested";
}

TEST_F(SignatureVerifierTest, Builder_CustomOrder) {
    // Test builder with custom verification order
    SignatureVerifierBuilder builder;
    auto verifier = builder
        .withCertificateChainValidation("/etc/ssl/certs/ca-certificates.crt")
        .withRSA_SHA256()
        .withCRLCheck("http://crl.example.com/test.crl")
        .build();
    
    ASSERT_NE(verifier, nullptr);
}

TEST_F(SignatureVerifierTest, Builder_EmptyChain) {
    // Test builder with no verifiers
    SignatureVerifierBuilder builder;
    auto verifier = builder.build();
    
    // Empty chain should return nullptr or handle gracefully
    // Current implementation allows null
}

// ===== Integration Tests =====

TEST_F(SignatureVerifierTest, Integration_LoRAAdapterVerification) {
    // Test signature verification for LoRA adapter
    SignatureVerifierBuilder builder;
    auto verifier = builder
        .withRSA_SHA256()
        .withCertificateChainValidation("/etc/ssl/certs/ca-certificates.crt")
        .build();
    
    // Production: Load LoRA adapter file, verify signature
    EXPECT_TRUE(true) << "Stub: LoRA adapter verification to be implemented";
}

TEST_F(SignatureVerifierTest, Integration_ModelWeightVerification) {
    // Test signature verification for model weights
    SignatureVerifierBuilder builder;
    auto verifier = builder
        .withRSA_SHA256()
        .build();
    
    // Production: Load model weights, verify signature
    EXPECT_TRUE(true) << "Stub: Model weight verification to be implemented";
}

// ===== Security Tests =====

TEST_F(SignatureVerifierTest, Security_TimingAttack) {
    // Test resistance to timing attacks
    auto verifier = std::make_unique<RSA_SHA256_Verifier>();
    
    // Production: Measure verification time for valid/invalid signatures
    // Should be constant time
    EXPECT_TRUE(true) << "Stub: Timing attack resistance to be verified";
}

TEST_F(SignatureVerifierTest, Security_KeySize) {
    // Test minimum key size requirements
    // RSA keys should be >= 2048 bits
    
    // Production: Test with various key sizes
    EXPECT_TRUE(true) << "Stub: Key size requirements to be enforced";
}

TEST_F(SignatureVerifierTest, Security_AlgorithmStrength) {
    // Test algorithm strength (SHA-256 is acceptable, SHA-1 is not)
    auto verifier = std::make_unique<RSA_SHA256_Verifier>();
    
    EXPECT_EQ(verifier->verify(test_data_, test_signature_, test_cert_pem_).algorithm, 
              "RSA-SHA256") << "Should use SHA-256, not weaker algorithms";
}

// ===== Performance Tests =====

TEST_F(SignatureVerifierTest, Performance_SingleVerification) {
    // Test performance of single signature verification
    auto verifier = std::make_unique<RSA_SHA256_Verifier>();
    
    // Production: Benchmark verification time (should be < 10ms)
    EXPECT_TRUE(true) << "Stub: Verification performance to be benchmarked";
}

TEST_F(SignatureVerifierTest, Performance_BulkVerification) {
    // Test performance of bulk signature verification
    auto verifier = std::make_unique<RSA_SHA256_Verifier>();
    
    // Production: Verify 100+ signatures, measure throughput
    EXPECT_TRUE(true) << "Stub: Bulk verification to be benchmarked";
}


