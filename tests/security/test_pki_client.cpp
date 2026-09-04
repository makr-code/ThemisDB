#include <gtest/gtest.h>
#include "utils/pki_client.h"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/rsa.h>

#include <filesystem>
#include <fstream>
#include <vector>
#include <random>

using namespace themis::utils;

namespace {

std::vector<uint8_t> random_bytes(size_t n) {
    std::vector<uint8_t> v(n);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 255);
    for (size_t i = 0; i < n; ++i) {
      v[i] = static_cast<uint8_t>(dist(rng));
    }
    return v;
}

bool generate_rsa_key_and_self_signed_cert(const std::string& key_path,
                                           const std::string& cert_path,
                                           int bits = 2048,
                                           int days_valid = 365) {
    bool ok = false;
    EVP_PKEY* pkey = nullptr;
    X509* x509 = nullptr;
    X509_NAME* name = nullptr;

    RSA* rsa = RSA_new();
    BIGNUM* e = BN_new();
    if (!rsa || !e) {
      goto cleanup;
    }
    if (BN_set_word(e, RSA_F4) != 1) {
      goto cleanup;
    }
    if (RSA_generate_key_ex(rsa, bits, e, nullptr) != 1) {
      goto cleanup;
    }

    pkey = EVP_PKEY_new();
    if (!pkey) {
      goto cleanup;
    }
#if OPENSSL_VERSION_NUMBER < 0x30000000L
    if (EVP_PKEY_assign_RSA(pkey, rsa) != 1) {
      goto cleanup;
    }
    // rsa is now owned by pkey
    rsa = nullptr;
#else
    if (EVP_PKEY_assign_RSA(pkey, rsa) != 1) {
      goto cleanup;
    }
    rsa = nullptr;
#endif

    x509 = X509_new();
    if (!x509) {
      goto cleanup;
    }

    // Version 3 certificate
    X509_set_version(x509, 2);

    // Serial number
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);

    // Validity
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), days_valid * 24 * 3600);

    // Subject/Issuer (self-signed)
    name = X509_get_subject_name(x509);
    if (!name) {
      goto cleanup;
    }
    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char*)"DE", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char*)"ThemisDB", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char*)"themis-test", -1, -1, 0);
    X509_set_issuer_name(x509, name);

    // Public key
    if (X509_set_pubkey(x509, pkey) != 1) {
      goto cleanup;
    }

    // Sign certificate
    if (X509_sign(x509, pkey, EVP_sha256()) == 0) {
      goto cleanup;
    }

    // Write key
    {
        FILE* f = fopen(key_path.c_str(), "wb");
        if (!f) {
          goto cleanup;
        }
        if (PEM_write_PrivateKey(f, pkey, nullptr, nullptr, 0, nullptr, nullptr) != 1) { fclose(f); goto cleanup; }
        fclose(f);
    }

    // Write cert
    {
        FILE* f = fopen(cert_path.c_str(), "wb");
        if (!f) {
          goto cleanup;
        }
        if (PEM_write_X509(f, x509) != 1) { fclose(f); goto cleanup; }
        fclose(f);
    }

    ok = true;

cleanup:
    if (x509) {
      X509_free(x509);
    }
    if (pkey) {
      EVP_PKEY_free(pkey);
    }
    if (rsa) {
      RSA_free(rsa);
    }
    if (e) {
      BN_free(e);
    }
    return ok;
}

} // namespace

#ifdef THEMIS_TEST_MODE
TEST(PKIClientTest, SignVerify_StubMode_Base64Echo) {
    PKIConfig cfg; // no key/cert -> stub mode
    cfg.signature_algorithm = "RSA-SHA256";
    VCCPKIClient client(cfg);

    auto hash = random_bytes(32);
    auto sig = client.signHash(hash);
    ASSERT_TRUE(sig.ok);
    // In stub mode signature is base64(hash); verify should succeed
    EXPECT_TRUE(client.verifyHash(hash, sig));
}

TEST(PKIClientTest, SignVerify_UsesBridgeCallbacksBeforeStubFallback) {
    PKIConfig cfg;
    VCCPKIClient::setSignHashFn([](const std::vector<uint8_t>&) {
        SignatureResult result;
        result.ok = true;
        result.signature_b64 = "bridge-signature";
        result.cert_serial = "bridge-cert";
        return result;
    });
    VCCPKIClient::setVerifyHashFn([](const std::vector<uint8_t>&, const SignatureResult& sig) {
        return sig.signature_b64 == "bridge-signature";
    });

    VCCPKIClient client(cfg);
    auto hash = random_bytes(32);
    auto sig = client.signHash(hash);
    ASSERT_TRUE(sig.ok);
    EXPECT_EQ(sig.signature_b64, "bridge-signature");
    EXPECT_EQ(sig.cert_serial, "bridge-cert");
    EXPECT_TRUE(client.verifyHash(hash, sig));

    VCCPKIClient::setSignHashFn({});
    VCCPKIClient::setVerifyHashFn({});
}
#endif // THEMIS_TEST_MODE

TEST(PKIClientTest, SignVerify_RSA_SHA256_Succeeds) {
    // Prepare temp files
    std::filesystem::create_directories("data/test_pki");
    const std::string key_path = "data/test_pki/test_key.pem";
    const std::string cert_path = "data/test_pki/test_cert.pem";

    ASSERT_TRUE(generate_rsa_key_and_self_signed_cert(key_path, cert_path));

    PKIConfig cfg;
    cfg.key_path = key_path;
    cfg.cert_path = cert_path;
    cfg.signature_algorithm = "RSA-SHA256";

    VCCPKIClient client(cfg);

    auto hash = random_bytes(32); // SHA-256 length
    auto sig = client.signHash(hash);

    ASSERT_TRUE(sig.ok);
    EXPECT_EQ(sig.algorithm, "RSA-SHA256");
    EXPECT_FALSE(sig.signature_b64.empty());
    EXPECT_FALSE(sig.cert_serial.empty());

    EXPECT_TRUE(client.verifyHash(hash, sig));

    // Negative: modify hash
    hash[0] ^= 0xFF;
    EXPECT_FALSE(client.verifyHash(hash, sig));
}

#ifdef THEMIS_TEST_MODE
TEST(PKIClientTest, SignVerify_AlgoMismatch_FallsBackStub) {
    PKIConfig cfg; // no key/cert
    cfg.signature_algorithm = "RSA-SHA512"; // expects 64-byte hash
    VCCPKIClient client(cfg);

    auto hash = random_bytes(32); // wrong length for RSA-SHA512 -> stub
    auto sig = client.signHash(hash);
    ASSERT_TRUE(sig.ok);
    // Should verify via stub comparison
    EXPECT_TRUE(client.verifyHash(hash, sig));
}
#endif // THEMIS_TEST_MODE

// In production mode (THEMIS_TEST_MODE not defined), signHash without a configured key
// must return ok=false so that callers are not misled by a fake stub signature.
TEST(PKIClientTest, SignHash_NoKeyConfigured_ReturnsFailureInProdMode) {
#ifdef THEMIS_TEST_MODE
    GTEST_SKIP() << "Stub fallback is active in THEMIS_TEST_MODE; skipping production-mode check.";
#endif
    PKIConfig cfg; // no key, no endpoint
    cfg.signature_algorithm = "RSA-SHA256";
    VCCPKIClient client(cfg);
    auto hash = random_bytes(32);
    auto sig = client.signHash(hash);
    EXPECT_FALSE(sig.ok) << "signHash without a configured key must fail in production mode";
}

// In production mode, verifyHash without a configured cert must return false (fail-closed).
TEST(PKIClientTest, VerifyHash_NoCertConfigured_ReturnsFalseInProdMode) {
#ifdef THEMIS_TEST_MODE
    GTEST_SKIP() << "Stub fallback is active in THEMIS_TEST_MODE; skipping production-mode check.";
#endif
    PKIConfig cfg; // no cert, no endpoint
    cfg.signature_algorithm = "RSA-SHA256";
    VCCPKIClient client(cfg);
    auto hash = random_bytes(32);

    SignatureResult fake_sig;
    fake_sig.ok = true;
    fake_sig.algorithm = "RSA-SHA256";
    fake_sig.signature_b64 = "dGVzdA=="; // arbitrary base64

    EXPECT_FALSE(client.verifyHash(hash, fake_sig))
        << "verifyHash without a configured cert must fail-closed in production mode";
}

// Verify that verifyHash rejects a signature when the certificate chain is invalid
// (i.e. the cert is self-signed and the trust store does not contain it as a trusted CA).
TEST(PKIClientTest, VerifyHash_ChainVerify_UntrustedCert_Rejected) {
    std::filesystem::create_directories("data/test_pki_chain");
    const std::string key_path  = "data/test_pki_chain/key.pem";
    const std::string cert_path = "data/test_pki_chain/cert.pem";
    // Use a different self-signed cert as the (wrong) trust store — chain must be rejected.
    const std::string other_key  = "data/test_pki_chain/other_key.pem";
    const std::string other_cert = "data/test_pki_chain/other_cert.pem";

    ASSERT_TRUE(generate_rsa_key_and_self_signed_cert(key_path, cert_path));
    ASSERT_TRUE(generate_rsa_key_and_self_signed_cert(other_key, other_cert));

    PKIConfig cfg;
    cfg.key_path         = key_path;
    cfg.cert_path        = cert_path;
    cfg.trust_store_path = other_cert; // intentionally wrong CA
    cfg.signature_algorithm = "RSA-SHA256";

    VCCPKIClient client(cfg);
    auto hash = random_bytes(32);
    auto sig  = client.signHash(hash);
    ASSERT_TRUE(sig.ok) << "Signing with a local key must succeed";

    // Verification must fail because the cert is not trusted by the (wrong) trust store.
    EXPECT_FALSE(client.verifyHash(hash, sig))
        << "Certificate chain validation must reject an untrusted certificate";
}

// Verify that verifyHash accepts a valid chain when the trust store contains the CA cert.
TEST(PKIClientTest, VerifyHash_ChainVerify_TrustedSelfSignedCert_Accepted) {
    std::filesystem::create_directories("data/test_pki_chain");
    const std::string key_path  = "data/test_pki_chain/self_key.pem";
    const std::string cert_path = "data/test_pki_chain/self_cert.pem";

    ASSERT_TRUE(generate_rsa_key_and_self_signed_cert(key_path, cert_path));

    PKIConfig cfg;
    cfg.key_path         = key_path;
    cfg.cert_path        = cert_path;
    cfg.trust_store_path = cert_path; // self-signed cert is its own CA
    cfg.signature_algorithm = "RSA-SHA256";

    VCCPKIClient client(cfg);
    auto hash = random_bytes(32);
    auto sig  = client.signHash(hash);
    ASSERT_TRUE(sig.ok);

    EXPECT_TRUE(client.verifyHash(hash, sig))
        << "Certificate chain validation must accept a self-signed cert trusted by itself";
}

// ---------------------------------------------------------------------------
// PKCS#10 CSR generation tests (X509_REQ_* API)
// ---------------------------------------------------------------------------

// generateCSR() must produce a non-empty, PEM-encoded, self-signed PKCS#10
// request that can be parsed back by OpenSSL.
TEST(PKIClientTest, GenerateCSR_WithKey_ProducesValidX509Req) {
    std::filesystem::create_directories("data/test_pki_csr");
    const std::string key_path  = "data/test_pki_csr/csr_key.pem";
    const std::string cert_path = "data/test_pki_csr/csr_cert.pem"; // generated but not used

    ASSERT_TRUE(generate_rsa_key_and_self_signed_cert(key_path, cert_path));

    PKIConfig cfg;
    cfg.key_path   = key_path;
    cfg.service_id = "themis-test-service";
    cfg.signature_algorithm = "RSA-SHA256";

    VCCPKIClient client(cfg);
    std::string csr_pem = client.generateCSR();

    ASSERT_FALSE(csr_pem.empty()) << "generateCSR() must return a non-empty PEM string";
    EXPECT_NE(csr_pem.find("-----BEGIN CERTIFICATE REQUEST-----"), std::string::npos)
        << "CSR PEM must contain CERTIFICATE REQUEST header";

    // Parse the CSR back with OpenSSL to confirm it is well-formed
    BIO* bio = BIO_new_mem_buf(csr_pem.data(), static_cast<int>(csr_pem.size()));
    ASSERT_NE(bio, nullptr);
    X509_REQ* req = PEM_read_bio_X509_REQ(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    ASSERT_NE(req, nullptr) << "OpenSSL must be able to parse the generated CSR";

    // Verify the CSR self-signature
    EVP_PKEY* pub = X509_REQ_get_pubkey(req);
    ASSERT_NE(pub, nullptr);
    int verify_rc = X509_REQ_verify(req, pub);
    EVP_PKEY_free(pub);
    X509_REQ_free(req);

    EXPECT_EQ(verify_rc, 1) << "CSR self-signature must be valid";
}

// generateCSR() must fail gracefully when no private key is configured.
TEST(PKIClientTest, GenerateCSR_NoKey_ReturnsEmpty) {
    PKIConfig cfg; // no key_path
    cfg.service_id = "themis-test";
    VCCPKIClient client(cfg);
    EXPECT_TRUE(client.generateCSR().empty())
        << "generateCSR() without a configured key must return an empty string";
}

// When ca_url is configured but the CA server is unavailable, signHash must
// not fall back to the base64 stub — it must return ok=false (fail-closed).
TEST(PKIClientTest, SignHash_CaUrlConfigured_UnavailableCA_FailsClosed) {
    std::filesystem::create_directories("data/test_pki_csr");
    const std::string key_path  = "data/test_pki_csr/capath_key.pem";
    const std::string cert_path = "data/test_pki_csr/capath_cert.pem";
    ASSERT_TRUE(generate_rsa_key_and_self_signed_cert(key_path, cert_path));

    PKIConfig cfg;
    cfg.key_path   = key_path;
    // No cert_path — triggers the ca_url provisioning path
    cfg.ca_url     = "http://127.0.0.1:19999"; // unreachable
    cfg.service_id = "themis-test";
    cfg.signature_algorithm = "RSA-SHA256";

    VCCPKIClient client(cfg);
    auto hash = random_bytes(32);
    auto sig  = client.signHash(hash);

    // CA is unreachable: provisioned cert will be empty.
    // Production: ok=false (no THEMIS_TEST_MODE fallback)
    // Test mode: the THEMIS_TEST_MODE stub kicks in.
#ifdef THEMIS_TEST_MODE
    // In test mode the base64 stub is still active — just verify we get a result.
    EXPECT_TRUE(sig.ok);
#else
    EXPECT_FALSE(sig.ok)
        << "signHash must fail-closed when ca_url is unreachable and no local cert exists";
#endif
}
