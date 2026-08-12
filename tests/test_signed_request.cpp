#include <gtest/gtest.h>
#include "sharding/signed_request.h"
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>

using namespace themis::sharding;

namespace {
const char* kTestCertSerial = "48B74D06993D80166108625ED6F2FD3C5EA21E03";

const char* kTestPrivateKeyPem = R"(-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCUBZVxYroHKaP7
ZPUz4BHAmYIJu+LDFTkDMpDmzvaiz6Szgl+iMp9z8hkPyJYBa9zcI51YsIgd01B8
tVL8oXzXM8moaGpVvaJhyygUmbYNcNhlDi8cbdqm4KHYA9bamae1p069nNBmlAkO
SNNyKgm/EKOtT6aG67lZEF3zDVlooe5JzxawP3+kW4kX+gE9uTKQzvQmjL9+ZRQP
gZztOH814Vhtz9t30e43Ijjm4q7sGDOlpshO4IS3gEZdUblYhQCrHdz6vX61byXY
CA+2YVwl0rBLtephXoK9XmcHyizGBHGsLr8BLjw9NatSQNxAwos0wXCdac6v2Dfd
2OSMvdrxAgMBAAECggEADJEcyu+S06DZZzJ/DtRrLWra+3CUI/PdT0QvwSi5D8GJ
PiRHckHR/8oCDrD860n5CkeLyEfUhpO2NaA/V2tuaebfrAiRXo4nBx50XKtgJWv7
MzktDsDU224ez3Qj/ZMmBeoaaUay+sJ+slPEf+DJywrimog0nyfMKJ+XGld7y8Iy
puPDvp+KX7njGM1cyWAofMzHyrK/XRGOoEdzQqbVfLH4wdoCec6MF80qUra/cSAM
E/gqnqdRbLKcIxEL8xZN1K+f2osWbLAKSdmPrdj9KW215pztEs/abK3Ae+xmrhbI
9FjcKEf0hbcll5tBY4JUSLbslUR9CnJSg25KR6aqQQKBgQDEDUZkxZlMU2Z04TuK
Ub+nwa4kN0utPTZJwJKhvoMQ0BDypzg2Z1SEDUakCijOX+dTN6Aw8zthAFNUgIEo
i94UFNmCy7ywFgAPADlZnQfBTxWOGJ6I16DCNk5958+7x/ztn7KmISd0tzlPvccQ
u5qsXtjFbwENa3qQsHW5NiAvLQKBgQDBSJHGmnDIbC7r+CYwrSxvhu0R2FD86hbh
C3qwXYrpW7d8PJ5wb/WwivkdPdmd7k7R48zeFW/DpVJzWYofm8cWx1a7QdLvwqgB
OSvzCyXvYthnBAfvoRJai3iTeBfEnbV6VaY4HAS3zZe4NafxfUFHC958nDtf9XEd
/JRp0p2VVQKBgH5gqE3xan5fDJ6vSfhB9i+nlB/YXToRSVuFfYAyFL7TwPkKfhvj
xAFTpYO84M0X2UA1jVfAkzhWQ2EnbRq7/q4nOocr2jgQFn2VAWMY7JTZ6+UrTZac
mQ5Yf4R919UOd6ad2UVp/cspCRK0HQxWQsB2a3npsl36OInolhqMH3cJAoGAfhLT
43PBtTm7sXd/kaijn2unH0i2KwKWQdy2/mtFvMlyebsgrb6Zx7oenBBH7lgPWaD5
dLMO39LqYQs6qTt3NUh5LlPVP0b9Ry7/l9zhtbAH78oVUx0BV8Y5tBdViY5FXbKY
ZWkBOtMD78gv77j/51O6oBVYNVNnoWOrewtCdrECgYEAt3sF7HUp/kDZqGmHRLfE
pz1mJXMje0FZzJNq6HcmsKFOkn/uyMDvcUrC4aRyt9FVIA7ISUmqr5YurXuxAmRB
l9tm715r6Oz8cnfMv6MkP82QCJEB56Gdsaqxz/ut2ggeKRgjjDyyghdKq4dkuVhX
eukGp9mcz0FoprBOfAfW3UA=
-----END PRIVATE KEY-----
)";

const char* kTestCertPem = R"(-----BEGIN CERTIFICATE-----
MIIDDTCCAfWgAwIBAgIUSLdNBpk9gBZhCGJe1vL9PF6iHgMwDQYJKoZIhvcNAQEL
BQAwFjEUMBIGA1UEAwwLdGhlbWlzLXRlc3QwHhcNMjYwNTEzMTgzMDQ5WhcNMjcw
NTEzMTgzMDQ5WjAWMRQwEgYDVQQDDAt0aGVtaXMtdGVzdDCCASIwDQYJKoZIhvcN
AQEBBQADggEPADCCAQoCggEBAJQFlXFiugcpo/tk9TPgEcCZggm74sMVOQMykObO
9qLPpLOCX6Iyn3PyGQ/IlgFr3NwjnViwiB3TUHy1UvyhfNczyahoalW9omHLKBSZ
tg1w2GUOLxxt2qbgodgD1tqZp7WnTr2c0GaUCQ5I03IqCb8Qo61PpobruVkQXfMN
WWih7knPFrA/f6RbiRf6AT25MpDO9CaMv35lFA+BnO04fzXhWG3P23fR7jciOObi
ruwYM6WmyE7ghLeARl1RuViFAKsd3Pq9frVvJdgID7ZhXCXSsEu16mFegr1eZwfK
LMYEcawuvwEuPD01q1JA3EDCizTBcJ1pzq/YN93Y5Iy92vECAwEAAaNTMFEwHQYD
VR0OBBYEFHrcg82/ipPE4YoZZn1fUoFukId5MB8GA1UdIwQYMBaAFHrcg82/ipPE
4YoZZn1fUoFukId5MA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQELBQADggEB
ABVUwbzD6Ek5EBv13RwLep+0bNVICv+lbUIVrqx+DSZCm8HaZH9ozm8Gxc31YX0d
pZeNis1OSeUJqJW2ANsxsYjjyIc6+SRMGD4DYAOhIOlJC35cS+eUhccueRwTM/Vs
whiBVbe+RY9/TwJ5FE+csgjUx0bdi3qi/RweqY2UEGimS7LlA+Uqfjk6WmZL2TX6
BwT6gM7TZXbxRCNNg5aFOq24BgpEDpQXbDJc5PgIh4h+Qar42PiJtNvVAQSniXDi
rCOHQ5ATKkpOo8SPkrQuelEJ8Nc7PQU5pOUmEkZueue5YkZi5UI/UUVOLSJRJHp0
AQcttznszBO+ug2ilJnGRic=
-----END CERTIFICATE-----
)";

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
    const auto base_dir = makeTempDir("themis_signed_request_test");
    TempDirGuard guard{base_dir};
    std::filesystem::create_directories(base_dir / "trusted");

    const auto cert_path = writePem(base_dir / "cert.pem", kTestCertPem);
    const auto key_path = writePem(base_dir / "key.pem", kTestPrivateKeyPem);
    writePem(base_dir / "trusted" / (std::string(kTestCertSerial) + ".pem"), kTestCertPem);

    SignedRequestSigner::Config signer_cfg;
    signer_cfg.shard_id = "shard_001";
    signer_cfg.cert_path = cert_path.string();
    signer_cfg.key_path = key_path.string();
    SignedRequestSigner signer(signer_cfg);

    SignedRequest request = signer.createSignedRequest("POST", "/api/test", nlohmann::json{{"x", 42}});
    ASSERT_FALSE(request.signature_b64.empty());
    EXPECT_EQ(request.signature_format, SignedRequest::kSignatureFormatV1);
    EXPECT_EQ(request.key_id, kTestCertSerial);

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
    const auto base_dir = makeTempDir("themis_signed_request_test_reject");
    TempDirGuard guard{base_dir};
    std::filesystem::create_directories(base_dir / "trusted");

    const auto cert_path = writePem(base_dir / "cert.pem", kTestCertPem);
    const auto key_path = writePem(base_dir / "key.pem", kTestPrivateKeyPem);
    writePem(base_dir / "trusted" / (std::string(kTestCertSerial) + ".pem"), kTestCertPem);

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
