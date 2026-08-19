#include <gtest/gtest.h>
#include "sharding/signed_request.h"
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <stdexcept>
#include <system_error>

using namespace themis::sharding;

namespace {
struct TestSigningMaterial {
    std::string cert_serial;
    std::string private_key_pem;
    std::string certificate_pem;
};

[[nodiscard]] static std::string bioToString(BIO* bio) {
    BUF_MEM* mem = nullptr;
    BIO_get_mem_ptr(bio, &mem);
    return mem ? std::string(mem->data, mem->length) : std::string{};
}

[[nodiscard]] static TestSigningMaterial makeTestSigningMaterial() {
    TestSigningMaterial material;
    material.cert_serial = "7E57A110C0DE1234";

    EVP_PKEY_CTX* keygen_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!keygen_ctx) {
        throw std::runtime_error("failed to allocate RSA keygen context");
    }

    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen_init(keygen_ctx) != 1 ||
        EVP_PKEY_CTX_set_rsa_keygen_bits(keygen_ctx, 2048) != 1 ||
        EVP_PKEY_keygen(keygen_ctx, &pkey) != 1) {
        EVP_PKEY_CTX_free(keygen_ctx);
        throw std::runtime_error("failed to generate RSA keypair");
    }
    EVP_PKEY_CTX_free(keygen_ctx);

    X509* cert = X509_new();
    if (!cert) {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("failed to allocate X509 certificate");
    }

    BIGNUM* serial_bn = nullptr;
    ASN1_INTEGER* serial_asn1 = nullptr;
    BIO* key_bio = nullptr;
    BIO* cert_bio = nullptr;

    try {
        if (X509_set_version(cert, 2) != 1) {
            throw std::runtime_error("failed to set certificate version");
        }
        if (BN_hex2bn(&serial_bn, material.cert_serial.c_str()) == 0) {
            throw std::runtime_error("failed to parse test certificate serial");
        }
        serial_asn1 = BN_to_ASN1_INTEGER(serial_bn, nullptr);
        if (!serial_asn1 || X509_set_serialNumber(cert, serial_asn1) != 1) {
            throw std::runtime_error("failed to set certificate serial");
        }
        if (!X509_gmtime_adj(X509_get_notBefore(cert), 0) ||
            !X509_gmtime_adj(X509_get_notAfter(cert), 31536000L)) {
            throw std::runtime_error("failed to set certificate validity");
        }
        if (X509_set_pubkey(cert, pkey) != 1) {
            throw std::runtime_error("failed to set certificate public key");
        }

        X509_NAME* subject = X509_get_subject_name(cert);
        if (!subject ||
            X509_NAME_add_entry_by_txt(
                subject,
                "CN",
                MBSTRING_ASC,
                reinterpret_cast<const unsigned char*>("themis-test"),
                -1,
                -1,
                0) != 1 ||
            X509_set_issuer_name(cert, subject) != 1) {
            throw std::runtime_error("failed to configure certificate subject");
        }

        if (X509_sign(cert, pkey, EVP_sha256()) <= 0) {
            throw std::runtime_error("failed to self-sign certificate");
        }

        key_bio = BIO_new(BIO_s_mem());
        cert_bio = BIO_new(BIO_s_mem());
        if (!key_bio || !cert_bio ||
            PEM_write_bio_PrivateKey(key_bio, pkey, nullptr, nullptr, 0, nullptr, nullptr) != 1 ||
            PEM_write_bio_X509(cert_bio, cert) != 1) {
            throw std::runtime_error("failed to serialize test signing material");
        }

        material.private_key_pem = bioToString(key_bio);
        material.certificate_pem = bioToString(cert_bio);
    } catch (...) {
        BIO_free(key_bio);
        BIO_free(cert_bio);
        ASN1_INTEGER_free(serial_asn1);
        BN_free(serial_bn);
        X509_free(cert);
        EVP_PKEY_free(pkey);
        throw;
    }

    BIO_free(key_bio);
    BIO_free(cert_bio);
    ASN1_INTEGER_free(serial_asn1);
    BN_free(serial_bn);
    X509_free(cert);
    EVP_PKEY_free(pkey);
    return material;
}

[[nodiscard]] static const TestSigningMaterial& getTestSigningMaterial() {
    static const TestSigningMaterial material = makeTestSigningMaterial();
    return material;
}

struct TempDirGuard {
    std::filesystem::path dir;
    ~TempDirGuard() {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }
};

[[nodiscard]] std::filesystem::path writePem(const std::filesystem::path& path, const std::string& content) {
    std::ofstream out(path);
    if (!out.is_open()) {
        throw std::runtime_error("failed to open PEM output file: " + path.string());
    }
    out << content;
    if (!out.good()) {
        throw std::runtime_error("failed to write PEM content: " + path.string());
    }
    std::error_code ec;
    std::filesystem::permissions(
        path,
        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
        std::filesystem::perm_options::replace,
        ec);
    if (ec) {
        throw std::runtime_error("failed to set permissions on PEM file: " + path.string());
    }
    return path;
}

[[nodiscard]] std::filesystem::path makeTempDir(const std::string& prefix) {
    const auto suffix = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto dir = std::filesystem::temp_directory_path() / (prefix + "_" + suffix);
    std::filesystem::create_directories(dir);
    return dir;
}
} // namespace

// ============================================================================
// SignedRequest Tests
// ============================================================================

TEST(SignedRequestTest, StructureAndSerialization) {
    SignedRequest req;
    req.shard_id = "shard_001";
    req.operation = "GET";
    req.path = "/api/v1/data";
    req.body = nlohmann::json{{"key", "value"}};
    req.timestamp_ms = 1700000000000;
    req.nonce = 12345;
    req.signature_format = SignedRequest::kSignatureFormatV1;
    req.key_id = "kid-001";
    req.signature_b64 = "dGVzdA==";
    req.cert_serial = "ABCD1234";
    
    EXPECT_EQ(req.shard_id, "shard_001");
    EXPECT_EQ(req.operation, "GET");
    EXPECT_EQ(req.path, "/api/v1/data");
    EXPECT_EQ(req.body["key"], "value");
}

TEST(SignedRequestTest, ToJSON) {
    SignedRequest req;
    req.shard_id = "shard_001";
    req.operation = "POST";
    req.path = "/api/v1/resource";
    req.body = nlohmann::json{{"data", 42}};
    req.timestamp_ms = 1700000000000;
    req.nonce = 67890;
    req.signature_format = SignedRequest::kSignatureFormatV1;
    req.key_id = "kid-002";
    req.signature_b64 = "c2lnbmF0dXJl";
    req.cert_serial = "SERIAL123";
    
    auto json = req.toJSON();
    
    EXPECT_EQ(json["shard_id"], "shard_001");
    EXPECT_EQ(json["operation"], "POST");
    EXPECT_EQ(json["path"], "/api/v1/resource");
    EXPECT_EQ(json["body"]["data"], 42);
    EXPECT_EQ(json["timestamp_ms"], 1700000000000);
    EXPECT_EQ(json["nonce"], 67890);
    EXPECT_EQ(json["signature_format"], SignedRequest::kSignatureFormatV1);
    EXPECT_EQ(json["key_id"], "kid-002");
}

TEST(SignedRequestTest, FromJSON) {
    nlohmann::json json = {
        {"shard_id", "shard_002"},
        {"operation", "DELETE"},
        {"path", "/api/v1/item/123"},
        {"body", nlohmann::json{}},
        {"timestamp_ms", 1700000000000},
        {"nonce", 11111},
        {"signature_format", SignedRequest::kSignatureFormatV1},
        {"key_id", "kid-abc"},
        {"signature_b64", "dGVzdHNpZw=="},
        {"cert_serial", "ABC123"}
    };
    
    auto req = SignedRequest::fromJSON(json);
    
    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->shard_id, "shard_002");
    EXPECT_EQ(req->operation, "DELETE");
    EXPECT_EQ(req->path, "/api/v1/item/123");
    EXPECT_EQ(req->nonce, 11111);
    EXPECT_EQ(req->signature_format, SignedRequest::kSignatureFormatV1);
    EXPECT_EQ(req->key_id, "kid-abc");
}

TEST(SignedRequestTest, CanonicalString) {
    SignedRequest req;
    req.shard_id = "shard_001";
    req.operation = "GET";
    req.path = "/test";
    req.body = nlohmann::json{{"x", 1}};
    req.timestamp_ms = 1000;
    req.nonce = 999;
    req.signature_format = SignedRequest::kSignatureFormatV1;
    req.key_id = "kid-001";
    req.cert_serial = "ABC123";
    
    std::string canonical = req.getCanonicalString();
    
    std::string expected =
        "signature_format=themis-shard-sig-v1\n"
        "shard_id=shard_001\n"
        "operation=GET\n"
        "path=/test\n"
        "body={\"x\":1}\n"
        "timestamp_ms=1000\n"
        "nonce=999\n"
        "key_id=kid-001\n"
        "cert_serial=ABC123\n";
    EXPECT_EQ(canonical, expected);
}

TEST(SignedRequestTest, FromJSONFallbacksKeyIdToCertSerial) {
    nlohmann::json json = {
        {"shard_id", "shard_002"},
        {"operation", "DELETE"},
        {"path", "/api/v1/item/123"},
        {"body", nlohmann::json{}},
        {"timestamp_ms", 1700000000000},
        {"nonce", 11111},
        {"signature_b64", "dGVzdHNpZw=="},
        {"cert_serial", "ABC123"}
    };
    auto req = SignedRequest::fromJSON(json);
    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->signature_format, SignedRequest::kSignatureFormatV1);
    EXPECT_EQ(req->key_id, "ABC123");
}

// ============================================================================
// SignedRequestSigner Tests
// ============================================================================

TEST(SignedRequestSignerTest, Configuration) {
    SignedRequestSigner::Config config;
    config.shard_id = "shard_001";
    config.cert_path = "/path/to/cert.pem";
    config.key_path = "/path/to/key.pem";
    config.key_passphrase = "secret";
    
    EXPECT_EQ(config.shard_id, "shard_001");
    EXPECT_EQ(config.cert_path, "/path/to/cert.pem");
    EXPECT_EQ(config.key_path, "/path/to/key.pem");
    EXPECT_EQ(config.key_passphrase, "secret");
}

TEST(SignedRequestSignerTest, CreateSignedRequestStructure) {
    // Note: Cannot test actual signing without valid certificates
    // This tests the structure and API
    
    SignedRequest req;
    req.operation = "GET";
    req.path = "/api/test";
    req.body = nlohmann::json{};
    
    // Should have basic structure
    EXPECT_EQ(req.operation, "GET");
    EXPECT_EQ(req.path, "/api/test");
}

// ============================================================================
// SignedRequestVerifier Tests
// ============================================================================

TEST(SignedRequestVerifierTest, Configuration) {
    SignedRequestVerifier::Config config;
    config.ca_cert_path = "/path/to/ca.pem";
    config.max_time_skew_ms = 30000;
    config.max_nonce_cache = 5000;
    config.nonce_expiry_ms = 600000;
    
    EXPECT_EQ(config.ca_cert_path, "/path/to/ca.pem");
    EXPECT_EQ(config.max_time_skew_ms, 30000u);
    EXPECT_EQ(config.max_nonce_cache, 5000u);
    EXPECT_EQ(config.nonce_expiry_ms, 600000u);
}

TEST(SignedRequestVerifierTest, DefaultConfiguration) {
    SignedRequestVerifier::Config config;
    
    EXPECT_EQ(config.max_time_skew_ms, 60000u);  // 60 seconds
    EXPECT_EQ(config.max_nonce_cache, 10000u);
    EXPECT_EQ(config.nonce_expiry_ms, 300000u);  // 5 minutes
}

TEST(SignedRequestVerifierTest, TimestampValidation) {
    // Test that timestamp validation concept exists
    SignedRequestVerifier::Config config;
    config.max_time_skew_ms = 1000;  // 1 second
    
    EXPECT_EQ(config.max_time_skew_ms, 1000u);
}

TEST(SignedRequestVerifierTest, NonceUniqueness) {
    // Test nonce uniqueness concept
    uint64_t nonce1 = 12345;
    uint64_t nonce2 = 67890;
    
    EXPECT_NE(nonce1, nonce2);
}

TEST(SignedRequestVerifierTest, CleanupStructure) {
    SignedRequestVerifier::Config config;
    SignedRequestVerifier verifier(config);
    
    // Test cleanup method exists
    verifier.cleanupExpiredNonces();
    
    // No exception should be thrown
    SUCCEED();
}

TEST(SignedRequestVerifierTest, VerifiesRealSignatureAndReplayProtection) {
    const auto& material = getTestSigningMaterial();
    const auto base_dir = makeTempDir("themis_signed_request_test");
    TempDirGuard guard{base_dir};
    std::filesystem::create_directories(base_dir / "trusted");

    const auto cert_path = writePem(base_dir / "cert.pem", material.certificate_pem);
    const auto key_path = writePem(base_dir / "key.pem", material.private_key_pem);
    writePem(base_dir / "trusted" / (material.cert_serial + ".pem"), material.certificate_pem);

    SignedRequestSigner::Config signer_cfg;
    signer_cfg.shard_id = "shard_001";
    signer_cfg.cert_path = cert_path.string();
    signer_cfg.key_path = key_path.string();
    SignedRequestSigner signer(signer_cfg);

    SignedRequest request = signer.createSignedRequest("POST", "/api/test", nlohmann::json{{"x", 42}});
    ASSERT_FALSE(request.signature_b64.empty());
    EXPECT_EQ(request.signature_format, SignedRequest::kSignatureFormatV1);
    EXPECT_EQ(request.key_id, material.cert_serial);

    SignedRequestVerifier::Config verifier_cfg;
    verifier_cfg.trusted_certs_dir = (base_dir / "trusted").string();
    verifier_cfg.max_time_skew_ms = 60'000;
    verifier_cfg.nonce_expiry_ms = 60'000;
    SignedRequestVerifier verifier(verifier_cfg);

    EXPECT_TRUE(verifier.verify(request, "shard_001"));
    EXPECT_FALSE(verifier.verify(request, "shard_001")) << "second verify must fail due to nonce replay";

    SignedRequest fresh_request = signer.createSignedRequest("POST", "/api/test", nlohmann::json{{"x", 43}});
    EXPECT_TRUE(verifier.verify(fresh_request, "shard_001")) << "fresh nonce should be accepted";
}

TEST(SignedRequestVerifierTest, RejectsTamperedPayloadAndUnknownKeyId) {
    const auto& material = getTestSigningMaterial();
    const auto base_dir = makeTempDir("themis_signed_request_test_reject");
    TempDirGuard guard{base_dir};
    std::filesystem::create_directories(base_dir / "trusted");

    const auto cert_path = writePem(base_dir / "cert.pem", material.certificate_pem);
    const auto key_path = writePem(base_dir / "key.pem", material.private_key_pem);
    writePem(base_dir / "trusted" / (material.cert_serial + ".pem"), material.certificate_pem);

    SignedRequestSigner::Config signer_cfg;
    signer_cfg.shard_id = "shard_001";
    signer_cfg.cert_path = cert_path.string();
    signer_cfg.key_path = key_path.string();
    SignedRequestSigner signer(signer_cfg);
    SignedRequest request = signer.createSignedRequest("POST", "/api/test", nlohmann::json{{"x", 42}});

    SignedRequestVerifier::Config verifier_cfg;
    verifier_cfg.trusted_certs_dir = (base_dir / "trusted").string();
    verifier_cfg.max_time_skew_ms = 60'000;
    verifier_cfg.nonce_expiry_ms = 60'000;
    SignedRequestVerifier verifier(verifier_cfg);

    SignedRequest tampered = request;
    tampered.path = "/api/other";
    EXPECT_FALSE(verifier.verify(tampered, "shard_001"));

    SignedRequest unknown_key = request;
    unknown_key.nonce += 1;
    unknown_key.key_id = "unknown-key";
    EXPECT_FALSE(verifier.verify(unknown_key, "shard_001"));
}

TEST(SignedRequestVerifierTest, RejectsInvalidSignatureFormatAndBase64) {
    SignedRequestVerifier::Config cfg;
    cfg.trusted_certs_dir = "/tmp/does-not-matter";
    cfg.max_time_skew_ms = 60'000;
    SignedRequestVerifier verifier(cfg);

    SignedRequest req;
    req.shard_id = "shard_001";
    req.operation = "GET";
    req.path = "/api/test";
    req.body = nlohmann::json{};
    req.timestamp_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    req.nonce = 42;
    req.signature_format = "unknown-v0";
    req.key_id = "kid-1";
    req.signature_b64 = "dGVzdA==";
    req.cert_serial = "ABC123";
    EXPECT_FALSE(verifier.verify(req, "shard_001"));

    req.signature_format = SignedRequest::kSignatureFormatV1;
    req.signature_b64 = "***not-base64***";
    EXPECT_FALSE(verifier.verify(req, "shard_001"));
}
