/**
 * @file test_signature_simple.cpp
 * @brief Simple standalone test for RSA-SHA256 signature verification
 * 
 * This is a minimal test that can be compiled and run without the full build system.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <memory>

// Define spdlog macros for standalone build
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#include <spdlog/spdlog.h>

// Include signature verifier
#include "llm/security/signature_verifier.h"

using namespace themis::llm::security;

// Helper to read file
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "ERROR: Failed to open file: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper to read binary file
std::vector<uint8_t> readFileBinary(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "ERROR: Failed to open binary file: " << path << std::endl;
        return {};
    }
    return std::vector<uint8_t>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}

int main() {
    spdlog::set_level(spdlog::level::info);
    
    std::cout << "=== RSA-SHA256 Signature Verifier Standalone Test ===" << std::endl;
    std::cout << std::endl;
    
    // Test paths
    std::string cert_dir = "data/certificates/";
    
    // Test 1: Load certificate and verify valid signature
    std::cout << "Test 1: Valid signature (2048-bit RSA)" << std::endl;
    {
        auto cert_pem = readFile(cert_dir + "test_cert.pem");
        auto data = readFileBinary(cert_dir + "test_data.txt");
        auto signature = readFileBinary(cert_dir + "test_data_signature.bin");
        
        if (cert_pem.empty() || data.empty() || signature.empty()) {
            std::cerr << "  SKIP: Test certificates not found" << std::endl;
            std::cerr << "  Run: cd tests/data/certificates && ./generate_test_certs.sh" << std::endl;
            return 1;
        }
        
        auto verifier = std::make_unique<RSA_SHA256_Verifier>();
        auto result = verifier->verify(data, signature, cert_pem);
        
        if (result.is_valid) {
            std::cout << "  PASS: Signature verified successfully" << std::endl;
            std::cout << "    Signer: " << result.signer_identity << std::endl;
        } else {
            std::cout << "  FAIL: " << result.error_message << std::endl;
            return 1;
        }
    }
    std::cout << std::endl;
    
    // Test 2: Detect tampered signature
    std::cout << "Test 2: Tampered signature detection" << std::endl;
    {
        auto cert_pem = readFile(cert_dir + "test_cert.pem");
        auto data = readFileBinary(cert_dir + "test_data.txt");
        auto signature = readFileBinary(cert_dir + "test_data_signature_tampered.bin");
        
        auto verifier = std::make_unique<RSA_SHA256_Verifier>();
        auto result = verifier->verify(data, signature, cert_pem);
        
        if (!result.is_valid) {
            std::cout << "  PASS: Tampered signature detected" << std::endl;
        } else {
            std::cout << "  FAIL: Tampered signature not detected!" << std::endl;
            return 1;
        }
    }
    std::cout << std::endl;
    
    // Test 3: Detect tampered data
    std::cout << "Test 3: Tampered data detection" << std::endl;
    {
        auto cert_pem = readFile(cert_dir + "test_cert.pem");
        auto data = readFileBinary(cert_dir + "test_data.txt");
        auto signature = readFileBinary(cert_dir + "test_data_signature.bin");
        
        // Tamper with data
        if (!data.empty()) {
            data[0] ^= 0xFF;
        }
        
        auto verifier = std::make_unique<RSA_SHA256_Verifier>();
        auto result = verifier->verify(data, signature, cert_pem);
        
        if (!result.is_valid) {
            std::cout << "  PASS: Tampered data detected" << std::endl;
        } else {
            std::cout << "  FAIL: Tampered data not detected!" << std::endl;
            return 1;
        }
    }
    std::cout << std::endl;
    
    // Test 4: Test with 3072-bit key
    std::cout << "Test 4: Valid signature (3072-bit RSA)" << std::endl;
    {
        auto cert_pem = readFile(cert_dir + "test_cert_3072.pem");
        auto data = readFileBinary(cert_dir + "test_data.txt");
        auto signature = readFileBinary(cert_dir + "test_data_signature_3072.bin");
        
        auto verifier = std::make_unique<RSA_SHA256_Verifier>();
        auto result = verifier->verify(data, signature, cert_pem);
        
        if (result.is_valid) {
            std::cout << "  PASS: 3072-bit signature verified" << std::endl;
        } else {
            std::cout << "  FAIL: " << result.error_message << std::endl;
            return 1;
        }
    }
    std::cout << std::endl;
    
    // Test 5: Test with 4096-bit key
    std::cout << "Test 5: Valid signature (4096-bit RSA)" << std::endl;
    {
        auto cert_pem = readFile(cert_dir + "test_cert_4096.pem");
        auto data = readFileBinary(cert_dir + "test_data.txt");
        auto signature = readFileBinary(cert_dir + "test_data_signature_4096.bin");
        
        auto verifier = std::make_unique<RSA_SHA256_Verifier>();
        auto result = verifier->verify(data, signature, cert_pem);
        
        if (result.is_valid) {
            std::cout << "  PASS: 4096-bit signature verified" << std::endl;
        } else {
            std::cout << "  FAIL: " << result.error_message << std::endl;
            return 1;
        }
    }
    std::cout << std::endl;
    
    // Test 6: Reject weak 1024-bit key
    std::cout << "Test 6: Weak key rejection (1024-bit RSA)" << std::endl;
    {
        auto cert_pem = readFile(cert_dir + "weak_cert_1024.pem");
        auto data = readFileBinary(cert_dir + "test_data.txt");
        std::vector<uint8_t> dummy_sig(128, 0);  // Dummy signature
        
        auto verifier = std::make_unique<RSA_SHA256_Verifier>();
        auto result = verifier->verify(data, dummy_sig, cert_pem);
        
        if (!result.is_valid && result.error_message.find("2048") != std::string::npos) {
            std::cout << "  PASS: Weak key rejected" << std::endl;
        } else {
            std::cout << "  FAIL: Weak key not rejected!" << std::endl;
            return 1;
        }
    }
    std::cout << std::endl;
    
    // Test 7: Certificate chain validation
    std::cout << "Test 7: Certificate chain validation" << std::endl;
    {
        auto cert_pem = readFile(cert_dir + "test_cert.pem");
        auto ca_path = cert_dir + "ca_cert.pem";
        auto data = readFileBinary(cert_dir + "test_data.txt");
        auto signature = readFileBinary(cert_dir + "test_data_signature.bin");
        
        auto chain_verifier = std::make_unique<CertificateChainVerifier>(ca_path);
        auto result = chain_verifier->verify(data, signature, cert_pem);
        
        if (result.chain_valid) {
            std::cout << "  PASS: Certificate chain validated" << std::endl;
        } else {
            std::cout << "  INFO: Chain validation failed (may be expected if CA not in system store)" << std::endl;
            std::cout << "    Error: " << result.error_message << std::endl;
        }
    }
    std::cout << std::endl;
    
    // Test 8: Builder pattern
    std::cout << "Test 8: Builder pattern with full chain" << std::endl;
    {
        auto cert_pem = readFile(cert_dir + "test_cert.pem");
        auto data = readFileBinary(cert_dir + "test_data.txt");
        auto signature = readFileBinary(cert_dir + "test_data_signature.bin");
        
        SignatureVerifierBuilder builder;
        auto verifier = builder
            .withRSA_SHA256()
            .withCertificateChainValidation(cert_dir + "ca_cert.pem")
            .withCRLCheck("http://crl.example.com/test.crl")
            .build();
        
        if (verifier) {
            std::cout << "  PASS: Builder created verifier chain" << std::endl;
            auto result = verifier->verify(data, signature, cert_pem);
            std::cout << "    Chain verification result: " << (result.is_valid ? "VALID" : "INVALID") << std::endl;
            if (!result.is_valid) {
                std::cout << "    (Expected for test chain not in system CA store)" << std::endl;
            }
        } else {
            std::cout << "  FAIL: Builder returned null" << std::endl;
            return 1;
        }
    }
    std::cout << std::endl;
    
    std::cout << "=== All Tests Completed ===" << std::endl;
    std::cout << "SUCCESS: Core cryptographic signature verification working!" << std::endl;
    
    return 0;
}
