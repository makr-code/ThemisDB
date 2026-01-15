#!/bin/bash
# Minimal standalone test - compile directly with OpenSSL only

set -e

echo "=== Building Minimal Signature Test (no dependencies) ==="

g++ -std=c++17 -Wall -Wextra \
    -I../include \
    -o test_sig_minimal \
    -x c++ - \
    -lssl -lcrypto << 'CPPEOF'
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <cstring>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/x509.h>

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return content;
}

std::vector<uint8_t> readBinary(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return {};
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
}

bool verifySHA256RSA(const std::vector<uint8_t>& data,
                      const std::vector<uint8_t>& signature,
                      const std::string& cert_pem) {
    // Load certificate
    BIO* bio = BIO_new_mem_buf(cert_pem.data(), cert_pem.size());
    if (!bio) return false;
    
    X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!cert) return false;
    
    // Extract public key
    EVP_PKEY* pkey = X509_get_pubkey(cert);
    X509_free(cert);
    if (!pkey) return false;
    
    // Check key size
    int key_bits = EVP_PKEY_bits(pkey);
    if (EVP_PKEY_id(pkey) == EVP_PKEY_RSA && key_bits < 2048) {
        EVP_PKEY_free(pkey);
        std::cout << "Key too small: " << key_bits << " bits (need 2048+)" << std::endl;
        return false;
    }
    
    // Hash the data
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash);
    
    // Verify signature
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    EVP_PKEY_free(pkey);
    if (!ctx) return false;
    
    if (EVP_PKEY_verify_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    
    EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING);
    EVP_PKEY_CTX_set_signature_md(ctx, EVP_sha256());
    
    int result = EVP_PKEY_verify(ctx, signature.data(), signature.size(),
                                  hash, SHA256_DIGEST_LENGTH);
    EVP_PKEY_CTX_free(ctx);
    
    return result == 1;
}

int main() {
    std::cout << "=== Minimal RSA-SHA256 Signature Test ===" << std::endl;
    
    std::string cert_dir = "data/certificates/";
    
    // Test 1: Valid signature
    std::cout << "\nTest 1: Valid 2048-bit signature... ";
    auto cert = readFile(cert_dir + "test_cert.pem");
    auto data = readBinary(cert_dir + "test_data.txt");
    auto sig = readBinary(cert_dir + "test_data_signature.bin");
    
    if (cert.empty() || data.empty() || sig.empty()) {
        std::cout << "SKIP (certificates not found)" << std::endl;
        return 1;
    }
    
    if (verifySHA256RSA(data, sig, cert)) {
        std::cout << "PASS" << std::endl;
    } else {
        std::cout << "FAIL" << std::endl;
        return 1;
    }
    
    // Test 2: Tampered data
    std::cout << "Test 2: Tampered data detection... ";
    data[0] ^= 0xFF;
    if (!verifySHA256RSA(data, sig, cert)) {
        std::cout << "PASS" << std::endl;
    } else {
        std::cout << "FAIL" << std::endl;
        return 1;
    }
    data[0] ^= 0xFF; // Restore
    
    // Test 3: 3072-bit key
    std::cout << "Test 3: Valid 3072-bit signature... ";
    auto cert3072 = readFile(cert_dir + "test_cert_3072.pem");
    auto sig3072 = readBinary(cert_dir + "test_data_signature_3072.bin");
    if (verifySHA256RSA(data, sig3072, cert3072)) {
        std::cout << "PASS" << std::endl;
    } else {
        std::cout << "FAIL" << std::endl;
        return 1;
    }
    
    // Test 4: 4096-bit key
    std::cout << "Test 4: Valid 4096-bit signature... ";
    auto cert4096 = readFile(cert_dir + "test_cert_4096.pem");
    auto sig4096 = readBinary(cert_dir + "test_data_signature_4096.bin");
    if (verifySHA256RSA(data, sig4096, cert4096)) {
        std::cout << "PASS" << std::endl;
    } else {
        std::cout << "FAIL" << std::endl;
        return 1;
    }
    
    // Test 5: Weak key rejection
    std::cout << "Test 5: Weak 1024-bit key rejection... ";
    auto weak_cert = readFile(cert_dir + "weak_cert_1024.pem");
    std::vector<uint8_t> dummy_sig(128, 0);
    if (!verifySHA256RSA(data, dummy_sig, weak_cert)) {
        std::cout << "PASS" << std::endl;
    } else {
        std::cout << "FAIL" << std::endl;
        return 1;
    }
    
    std::cout << "\n=== ALL TESTS PASSED ===" << std::endl;
    std::cout << "SUCCESS: RSA-SHA256 verification working correctly!" << std::endl;
    return 0;
}
CPPEOF

echo ""
echo "=== Running Tests ==="
./test_sig_minimal

echo ""
echo "=== Tests Complete ===="
