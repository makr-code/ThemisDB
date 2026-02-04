#include <gtest/gtest.h>
#include "security/pki_stub.h"

using namespace themis::security;

class PKIManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        pki_manager = SecurityManagerFactory::createPKIManager(true);
    }
    
    std::unique_ptr<PKIManager> pki_manager;
};

TEST_F(PKIManagerTest, IsStub) {
    EXPECT_TRUE(pki_manager->isStub());
}

TEST_F(PKIManagerTest, GenerateKeyPair) {
    auto key_id = pki_manager->generateKeyPair(2048);
    EXPECT_FALSE(key_id.empty());
    EXPECT_TRUE(key_id.find("stub_key_") != std::string::npos);
}

TEST_F(PKIManagerTest, GenerateCSR) {
    auto key_id = pki_manager->generateKeyPair(2048);
    auto csr = pki_manager->generateCSR(key_id, "CN=test.example.com");
    
    EXPECT_FALSE(csr.empty());
    EXPECT_TRUE(csr.find("BEGIN CERTIFICATE REQUEST") != std::string::npos);
    EXPECT_TRUE(csr.find("END CERTIFICATE REQUEST") != std::string::npos);
}

TEST_F(PKIManagerTest, SignCertificate) {
    auto key_id = pki_manager->generateKeyPair(2048);
    auto csr = pki_manager->generateCSR(key_id, "CN=test.example.com");
    
    auto ca_cert = "-----BEGIN CERTIFICATE-----\nCA CERT\n-----END CERTIFICATE-----\n";
    auto ca_key = "-----BEGIN PRIVATE KEY-----\nCA KEY\n-----END PRIVATE KEY-----\n";
    
    auto cert = pki_manager->signCertificate(csr, ca_cert, ca_key, 365);
    
    EXPECT_FALSE(cert.empty());
    EXPECT_TRUE(cert.find("BEGIN CERTIFICATE") != std::string::npos);
    EXPECT_TRUE(cert.find("END CERTIFICATE") != std::string::npos);
}

TEST_F(PKIManagerTest, VerifyCertificate) {
    auto cert = "-----BEGIN CERTIFICATE-----\nTEST CERT\n-----END CERTIFICATE-----\n";
    std::vector<std::string> ca_certs = {
        "-----BEGIN CERTIFICATE-----\nCA CERT\n-----END CERTIFICATE-----\n"
    };
    
    // Stub always returns true
    EXPECT_TRUE(pki_manager->verifyCertificate(cert, ca_certs));
}

TEST_F(PKIManagerTest, RevokeCertificate) {
    // Stub always returns true
    EXPECT_TRUE(pki_manager->revokeCertificate("cert_id_123"));
}

TEST_F(PKIManagerTest, ParseCertificate) {
    auto cert = "-----BEGIN CERTIFICATE-----\nTEST CERT\n-----END CERTIFICATE-----\n";
    auto info = pki_manager->parseCertificate(cert);
    
    ASSERT_TRUE(info.has_value());
    EXPECT_FALSE(info->subject.empty());
    EXPECT_FALSE(info->issuer.empty());
    EXPECT_GT(info->not_before, 0);
    EXPECT_GT(info->not_after, info->not_before);
}

TEST_F(PKIManagerTest, ExportPublicKey) {
    auto cert = "-----BEGIN CERTIFICATE-----\nTEST CERT\n-----END CERTIFICATE-----\n";
    auto public_key = pki_manager->exportPublicKey(cert);
    
    EXPECT_FALSE(public_key.empty());
    EXPECT_TRUE(public_key.find("BEGIN PUBLIC KEY") != std::string::npos);
    EXPECT_TRUE(public_key.find("END PUBLIC KEY") != std::string::npos);
}

class SignatureManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        sig_manager = SecurityManagerFactory::createSignatureManager(true);
    }
    
    std::unique_ptr<SignatureManager> sig_manager;
};

TEST_F(SignatureManagerTest, IsStub) {
    EXPECT_TRUE(sig_manager->isStub());
}

TEST_F(SignatureManagerTest, Sign) {
    std::vector<uint8_t> data = {'t', 'e', 's', 't', ' ', 'd', 'a', 't', 'a'};
    auto signature = sig_manager->sign(data, "key_001", "RSA-SHA256");
    
    EXPECT_FALSE(signature.empty());
}

TEST_F(SignatureManagerTest, Verify) {
    std::vector<uint8_t> data = {'t', 'e', 's', 't', ' ', 'd', 'a', 't', 'a'};
    auto signature = sig_manager->sign(data, "key_001", "RSA-SHA256");
    
    std::string public_key = "-----BEGIN PUBLIC KEY-----\nTEST KEY\n-----END PUBLIC KEY-----\n";
    
    // Stub always returns true
    EXPECT_TRUE(sig_manager->verify(data, signature, public_key, "RSA-SHA256"));
}

TEST_F(SignatureManagerTest, SignDetached) {
    std::vector<uint8_t> data = {'t', 'e', 's', 't', ' ', 'd', 'a', 't', 'a'};
    auto signature_b64 = sig_manager->signDetached(data, "key_001");
    
    EXPECT_FALSE(signature_b64.empty());
}

TEST_F(SignatureManagerTest, VerifyDetached) {
    std::vector<uint8_t> data = {'t', 'e', 's', 't', ' ', 'd', 'a', 't', 'a'};
    auto signature_b64 = sig_manager->signDetached(data, "key_001");
    
    std::string public_key = "-----BEGIN PUBLIC KEY-----\nTEST KEY\n-----END PUBLIC KEY-----\n";
    
    // Stub always returns true
    EXPECT_TRUE(sig_manager->verifyDetached(data, signature_b64, public_key));
}

TEST_F(SignatureManagerTest, SignWithTimestamp) {
    std::vector<uint8_t> data = {'t', 'e', 's', 't', ' ', 'd', 'a', 't', 'a'};
    auto ts_signature = sig_manager->signWithTimestamp(data, "key_001");
    
    EXPECT_FALSE(ts_signature.signature.empty());
    EXPECT_GT(ts_signature.timestamp, 0);
    EXPECT_FALSE(ts_signature.tsa_certificate.empty());
}

TEST_F(SignatureManagerTest, GetSupportedAlgorithms) {
    auto algorithms = sig_manager->getSupportedAlgorithms();
    
    EXPECT_FALSE(algorithms.empty());
    EXPECT_GT(algorithms.size(), 0);
    
    // Check that some common algorithms are supported
    bool has_rsa = false;
    bool has_ecdsa = false;
    
    for (const auto& algo : algorithms) {
        if (algo.find("RSA") != std::string::npos) has_rsa = true;
        if (algo.find("ECDSA") != std::string::npos) has_ecdsa = true;
    }
    
    EXPECT_TRUE(has_rsa);
    EXPECT_TRUE(has_ecdsa);
}

class SecurityManagerFactoryTest : public ::testing::Test {};

TEST_F(SecurityManagerFactoryTest, CreatePKIManager) {
    auto pki_manager = SecurityManagerFactory::createPKIManager(true);
    ASSERT_NE(pki_manager, nullptr);
    EXPECT_TRUE(pki_manager->isStub());
}

TEST_F(SecurityManagerFactoryTest, CreateSignatureManager) {
    auto sig_manager = SecurityManagerFactory::createSignatureManager(true);
    ASSERT_NE(sig_manager, nullptr);
    EXPECT_TRUE(sig_manager->isStub());
}
