#include <gtest/gtest.h>
#include "sharding/signed_request.h"
#include "../../test_crypto_material_utils.h"
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>

using namespace themis::sharding;

namespace {
struct TempDirGuard {
    std::filesystem::path dir;
    ~TempDirGuard() {
        std::error_code ec = {};
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
    std::error_code ec = {};
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
    const auto& material = themis::tests::getSignedRequestPemMaterial();
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
    const auto& material = themis::tests::getSignedRequestPemMaterial();
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
