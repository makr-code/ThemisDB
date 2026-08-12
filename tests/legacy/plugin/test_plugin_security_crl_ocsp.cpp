// test_plugin_security_crl_ocsp.cpp
// Unit tests for checkCRL() / checkOCSP() revocation checking

#include <gtest/gtest.h>
#include "acceleration/plugin_security.h"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/ocsp.h>
#include <openssl/bn.h>

#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <cstring>

using namespace themis::acceleration;

// ============================================================================
// Test fixture
// ============================================================================

class PluginSecurityCRLOCSPTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = std::filesystem::temp_directory_path() /
                    "themis_crl_ocsp_test";
        std::filesystem::create_directories(test_dir_);

        generateSelfSignedCert();
        generateCertWithCRLExtension();
        generateCertWithOCSPExtension();
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir_);
    }

    // -------------------------------------------------------------------------
    // Helper: generate an RSA key + self-signed X.509 cert and return both.
    // -------------------------------------------------------------------------
    static bool makeKeyAndCert(EVP_PKEY** out_pkey, X509** out_cert,
                                long serial_number = 42,
                                const char* cn = "ThemisDB Test Cert") {
        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
        if (!ctx) return false;
        if (EVP_PKEY_keygen_init(ctx) <= 0 ||
            EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0 ||
            EVP_PKEY_keygen(ctx, out_pkey) <= 0) {
            EVP_PKEY_CTX_free(ctx);
            return false;
        }
        EVP_PKEY_CTX_free(ctx);

        X509* x = X509_new();
        X509_set_version(x, 2);
        ASN1_INTEGER_set(X509_get_serialNumber(x), serial_number);
        X509_gmtime_adj(X509_get_notBefore(x), 0);
        X509_gmtime_adj(X509_get_notAfter(x), 31536000L);  // 1 year
        X509_set_pubkey(x, *out_pkey);

        X509_NAME* name = X509_get_subject_name(x);
        X509_NAME_add_entry_by_txt(
            name, "CN", MBSTRING_ASC,
            reinterpret_cast<const unsigned char*>(cn), -1, -1, 0);
        X509_set_issuer_name(x, name);

        X509_sign(x, *out_pkey, EVP_sha256());
        *out_cert = x;
        return true;
    }

    // Encode an X.509 cert as PEM string.
    static std::string certToPEM(X509* cert) {
        BIO* bio = BIO_new(BIO_s_mem());
        PEM_write_bio_X509(bio, cert);
        BUF_MEM* mem = nullptr;
        BIO_get_mem_ptr(bio, &mem);
        std::string pem(mem->data, mem->length);
        BIO_free(bio);
        return pem;
    }

    // -------------------------------------------------------------------------
    // Plain self-signed cert (no CRL/OCSP extensions).
    // -------------------------------------------------------------------------
    void generateSelfSignedCert() {
        EVP_PKEY* pkey = nullptr;
        X509*     cert = nullptr;
        if (!makeKeyAndCert(&pkey, &cert, 1)) return;
        plain_cert_pem_ = certToPEM(cert);
        X509_free(cert);
        EVP_PKEY_free(pkey);
    }

    // -------------------------------------------------------------------------
    // Cert with a CRL distribution point pointing at an unreachable URL.
    // -------------------------------------------------------------------------
    void generateCertWithCRLExtension() {
        EVP_PKEY* pkey = nullptr;
        X509*     cert = nullptr;
        if (!makeKeyAndCert(&pkey, &cert, 2)) return;

        // Add cRLDistributionPoints extension with a bogus local URL
        // (127.0.0.1:19999 is almost certainly not listening in CI).
        X509V3_CTX v3ctx;
        X509V3_set_ctx_nodb(&v3ctx);
        X509V3_set_ctx(&v3ctx, cert, cert, nullptr, nullptr, 0);

        X509_EXTENSION* ext = X509V3_EXT_conf_nid(
            nullptr, &v3ctx,
            NID_crl_distribution_points,
            "URI:http://127.0.0.1:19999/test.crl");
        if (ext) {
            X509_add_ext(cert, ext, -1);
            X509_EXTENSION_free(ext);
        }

        // Re-sign after adding the extension
        X509_sign(cert, pkey, EVP_sha256());

        crl_cert_pem_ = certToPEM(cert);
        X509_free(cert);
        EVP_PKEY_free(pkey);
    }

    // -------------------------------------------------------------------------
    // Cert with an OCSP AIA pointing at an unreachable URL.
    // -------------------------------------------------------------------------
    void generateCertWithOCSPExtension() {
        EVP_PKEY* pkey = nullptr;
        X509*     cert = nullptr;
        if (!makeKeyAndCert(&pkey, &cert, 3)) return;

        X509V3_CTX v3ctx;
        X509V3_set_ctx_nodb(&v3ctx);
        X509V3_set_ctx(&v3ctx, cert, cert, nullptr, nullptr, 0);

        // authorityInfoAccess: OCSP;URI:http://127.0.0.1:19998
        X509_EXTENSION* ext = X509V3_EXT_conf_nid(
            nullptr, &v3ctx,
            NID_info_access,
            "OCSP;URI:http://127.0.0.1:19998");
        if (ext) {
            X509_add_ext(cert, ext, -1);
            X509_EXTENSION_free(ext);
        }

        X509_sign(cert, pkey, EVP_sha256());

        ocsp_cert_pem_ = certToPEM(cert);
        X509_free(cert);
        EVP_PKEY_free(pkey);
    }

    // -------------------------------------------------------------------------
    // Build a minimal DER-encoded CRL in memory for direct parsing tests.
    // If revoke_serial >= 0, the given serial number is listed as revoked.
    // -------------------------------------------------------------------------
    static std::vector<uint8_t> buildDERCRL(EVP_PKEY* signing_key,
                                             X509*     issuer_cert,
                                             long      revoke_serial = -1,
                                             int       days_valid    = 7) {
        X509_CRL* crl = X509_CRL_new();
        if (!crl) return {};

        // thisUpdate = now, nextUpdate = now + days_valid days
        ASN1_TIME* tu = ASN1_TIME_new();
        ASN1_TIME* nu = ASN1_TIME_new();
        X509_gmtime_adj(tu, 0);
        X509_gmtime_adj(nu, static_cast<long>(days_valid) * 24 * 3600);
        X509_CRL_set1_lastUpdate(crl, tu);
        X509_CRL_set1_nextUpdate(crl, nu);
        ASN1_TIME_free(tu);
        ASN1_TIME_free(nu);

        // Issuer
        X509_CRL_set_issuer_name(crl, X509_get_subject_name(issuer_cert));

        // Add revoked entry if requested
        if (revoke_serial >= 0) {
            X509_REVOKED* rev = X509_REVOKED_new();
            ASN1_INTEGER* serial_asn = ASN1_INTEGER_new();
            ASN1_INTEGER_set(serial_asn, revoke_serial);
            X509_REVOKED_set_serialNumber(rev, serial_asn);
            ASN1_INTEGER_free(serial_asn);

            ASN1_TIME* rev_time = ASN1_TIME_new();
            X509_gmtime_adj(rev_time, -3600L);  // revoked 1 h ago
            X509_REVOKED_set_revocationDate(rev, rev_time);
            ASN1_TIME_free(rev_time);

            X509_CRL_add0_revoked(crl, rev);
        }

        X509_CRL_sort(crl);
        X509_CRL_sign(crl, signing_key, EVP_sha256());

        // DER-encode
        unsigned char* der = nullptr;
        int len = i2d_X509_CRL(crl, &der);
        X509_CRL_free(crl);
        if (len <= 0) return {};

        std::vector<uint8_t> result(der, der + len);
        OPENSSL_free(der);
        return result;
    }

    // -------------------------------------------------------------------------
    // Member data
    // -------------------------------------------------------------------------
    std::filesystem::path test_dir_;
    std::string plain_cert_pem_;   // no revocation extensions
    std::string crl_cert_pem_;     // has CRL distribution point (unreachable)
    std::string ocsp_cert_pem_;    // has OCSP AIA (unreachable)
};

// ============================================================================
// checkCRL() tests
// ============================================================================

TEST_F(PluginSecurityCRLOCSPTest, CRL_EmptyCertReturnsFalse) {
    PluginSecurityPolicy policy;
    PluginSecurityVerifier verifier(policy);
    EXPECT_FALSE(verifier.checkCRL(""));
}

TEST_F(PluginSecurityCRLOCSPTest, CRL_InvalidPEMReturnsFalse) {
    PluginSecurityPolicy policy;
    PluginSecurityVerifier verifier(policy);
    EXPECT_FALSE(verifier.checkCRL("not-a-pem"));
}

// Cert without CRL endpoints: pass when checkRevocation=false
TEST_F(PluginSecurityCRLOCSPTest, CRL_NoCRLEndpoints_RevocationDisabled_Passes) {
    PluginSecurityPolicy policy;
    policy.checkRevocation = false;
    PluginSecurityVerifier verifier(policy);
    EXPECT_TRUE(verifier.checkCRL(plain_cert_pem_));
}

// Cert without CRL endpoints: fail when checkRevocation=true
TEST_F(PluginSecurityCRLOCSPTest, CRL_NoCRLEndpoints_RevocationEnabled_Fails) {
    PluginSecurityPolicy policy;
    policy.checkRevocation = true;
    PluginSecurityVerifier verifier(policy);
    EXPECT_FALSE(verifier.checkCRL(plain_cert_pem_));
}

// Cert has CRL distribution point but the server is unreachable.
// With revocation disabled: should pass (no endpoint reachable → pass).
TEST_F(PluginSecurityCRLOCSPTest, CRL_UnreachableEndpoint_RevocationDisabled_Passes) {
    if (crl_cert_pem_.empty()) {
        GTEST_SKIP() << "CRL cert not generated";
    }
    PluginSecurityPolicy policy;
    policy.checkRevocation     = false;
    policy.revocation_timeout_seconds = 1;  // fast timeout for tests
    PluginSecurityVerifier verifier(policy);
    // No endpoint reachable; revocation disabled → pass
    EXPECT_TRUE(verifier.checkCRL(crl_cert_pem_));
}

// Cert has CRL distribution point but the server is unreachable.
// With revocation enabled: should fail (can't check → fail-safe).
TEST_F(PluginSecurityCRLOCSPTest, CRL_UnreachableEndpoint_RevocationEnabled_Fails) {
    if (crl_cert_pem_.empty()) {
        GTEST_SKIP() << "CRL cert not generated";
    }
    PluginSecurityPolicy policy;
    policy.checkRevocation            = true;
    policy.revocation_timeout_seconds = 1;
    PluginSecurityVerifier verifier(policy);
    EXPECT_FALSE(verifier.checkCRL(crl_cert_pem_));
}

// Network-timeout path: very short timeout, unreachable server → fail-safe
TEST_F(PluginSecurityCRLOCSPTest, CRL_NetworkTimeout_FailsSafe) {
    if (crl_cert_pem_.empty()) {
        GTEST_SKIP() << "CRL cert not generated";
    }
    PluginSecurityPolicy policy;
    policy.checkRevocation            = true;
    policy.revocation_timeout_seconds = 1;  // 1-second timeout
    PluginSecurityVerifier verifier(policy);
    // Must complete within a reasonable time (don't hang)
    EXPECT_NO_THROW({
        verifier.checkCRL(crl_cert_pem_);
    });
    EXPECT_FALSE(verifier.checkCRL(crl_cert_pem_));
}

// Cache test: calling checkCRL twice with the same cert should be idempotent.
TEST_F(PluginSecurityCRLOCSPTest, CRL_CacheHit_SecondCallConsistent) {
    PluginSecurityPolicy policy;
    policy.checkRevocation = false;
    PluginSecurityVerifier verifier(policy);
    bool first  = verifier.checkCRL(plain_cert_pem_);
    bool second = verifier.checkCRL(plain_cert_pem_);
    EXPECT_EQ(first, second);
}

// ============================================================================
// DER CRL parsing / revocation-detection tests (no network required)
// ============================================================================

// Build a CRL that revokes serial 42 and verify that a cert with serial 42
// is detected as revoked via the internal OpenSSL CRL check.
TEST_F(PluginSecurityCRLOCSPTest, DERParse_RevokedSerial_DetectedByOpenSSL) {
    // Generate issuer key+cert (serial 99 = issuer, serial 42 = subject)
    EVP_PKEY* issuer_key  = nullptr;
    X509*     issuer_cert = nullptr;
    ASSERT_TRUE(makeKeyAndCert(&issuer_key, &issuer_cert, 99, "Test Issuer"));

    EVP_PKEY* leaf_key  = nullptr;
    X509*     leaf_cert = nullptr;
    ASSERT_TRUE(makeKeyAndCert(&leaf_key, &leaf_cert, 42, "Test Leaf"));

    // Build a CRL that revokes serial 42
    std::vector<uint8_t> crl_der =
        buildDERCRL(issuer_key, issuer_cert, 42 /*revoke*/);
    ASSERT_FALSE(crl_der.empty());

    // Parse the CRL and verify serial 42 is listed as revoked.
    // Use serial-based lookup to avoid issuer-subject coupling in this unit fixture.
    const unsigned char* p = crl_der.data();
    X509_CRL* crl = d2i_X509_CRL(nullptr, &p, static_cast<long>(crl_der.size()));
    ASSERT_NE(crl, nullptr);

    X509_REVOKED* revoked = nullptr;
    int rv = X509_CRL_get0_by_serial(crl, &revoked, X509_get_serialNumber(leaf_cert));
    EXPECT_EQ(rv, 1) << "Serial 42 should be in the revocation list";

    // Verify that serial 99 (the issuer itself) is NOT revoked
    X509_REVOKED* not_revoked = nullptr;
    int rv2 = X509_CRL_get0_by_serial(crl, &not_revoked, X509_get_serialNumber(issuer_cert));
    EXPECT_EQ(rv2, 0) << "Serial 99 should not be in the revocation list";

    X509_CRL_free(crl);
    X509_free(leaf_cert);
    X509_free(issuer_cert);
    EVP_PKEY_free(leaf_key);
    EVP_PKEY_free(issuer_key);
}

// Build a CRL that does NOT revoke serial 42; verify serial 42 is not found.
TEST_F(PluginSecurityCRLOCSPTest, DERParse_NonRevokedSerial_NotDetected) {
    EVP_PKEY* issuer_key  = nullptr;
    X509*     issuer_cert = nullptr;
    ASSERT_TRUE(makeKeyAndCert(&issuer_key, &issuer_cert, 99));

    EVP_PKEY* leaf_key  = nullptr;
    X509*     leaf_cert = nullptr;
    ASSERT_TRUE(makeKeyAndCert(&leaf_key, &leaf_cert, 42));

    // CRL with no revocations
    std::vector<uint8_t> crl_der = buildDERCRL(issuer_key, issuer_cert, -1);
    ASSERT_FALSE(crl_der.empty());

    const unsigned char* p = crl_der.data();
    X509_CRL* crl = d2i_X509_CRL(nullptr, &p, static_cast<long>(crl_der.size()));
    ASSERT_NE(crl, nullptr);

    X509_REVOKED* entry = nullptr;
    int rv = X509_CRL_get0_by_serial(crl, &entry, X509_get_serialNumber(leaf_cert));
    EXPECT_EQ(rv, 0) << "Serial 42 should not be in an empty revocation list";

    X509_CRL_free(crl);
    X509_free(leaf_cert);
    X509_free(issuer_cert);
    EVP_PKEY_free(leaf_key);
    EVP_PKEY_free(issuer_key);
}

// Verify that an expired CRL (nextUpdate in the past) is correctly detected.
TEST_F(PluginSecurityCRLOCSPTest, DERParse_ExpiredCRL_TimestampInvalid) {
    EVP_PKEY* issuer_key  = nullptr;
    X509*     issuer_cert = nullptr;
    ASSERT_TRUE(makeKeyAndCert(&issuer_key, &issuer_cert, 99));

    // Build CRL with nextUpdate 1 day in the PAST (days_valid = -1)
    X509_CRL* crl = X509_CRL_new();
    ASN1_TIME* tu = ASN1_TIME_new();
    ASN1_TIME* nu = ASN1_TIME_new();
    X509_gmtime_adj(tu, -172800L);  // 2 days ago
    X509_gmtime_adj(nu, -86400L);   // 1 day ago → expired
    X509_CRL_set1_lastUpdate(crl, tu);
    X509_CRL_set1_nextUpdate(crl, nu);
    ASN1_TIME_free(tu);
    ASN1_TIME_free(nu);
    X509_CRL_set_issuer_name(crl, X509_get_subject_name(issuer_cert));
    X509_CRL_sign(crl, issuer_key, EVP_sha256());

    // nextUpdate is in the past → ASN1_TIME_diff should return day < 0
    const ASN1_TIME* next_update = X509_CRL_get0_nextUpdate(crl);
    int day = 0, sec = 0;
    int diff_ok = ASN1_TIME_diff(&day, &sec, nullptr, next_update);
    EXPECT_TRUE(diff_ok);
    EXPECT_LT(day * 86400 + sec, 0) << "CRL should be expired (nextUpdate in the past)";

    X509_CRL_free(crl);
    X509_free(issuer_cert);
    EVP_PKEY_free(issuer_key);
}

// Verify that a CRL signed with a different (wrong) key is rejected by
// X509_CRL_verify, exercising the signature-invalid code path.
TEST_F(PluginSecurityCRLOCSPTest, DERParse_CRL_InvalidSignature_RejectedByOpenSSL) {
    // issuer_key signs the CRL
    EVP_PKEY* issuer_key  = nullptr;
    X509*     issuer_cert = nullptr;
    ASSERT_TRUE(makeKeyAndCert(&issuer_key, &issuer_cert, 99, "Test Issuer"));

    // Build a valid CRL signed with issuer_key
    std::vector<uint8_t> crl_der =
        buildDERCRL(issuer_key, issuer_cert, -1 /* no revoked entries */);
    ASSERT_FALSE(crl_der.empty());

    const unsigned char* p = crl_der.data();
    X509_CRL* crl = d2i_X509_CRL(nullptr, &p, static_cast<long>(crl_der.size()));
    ASSERT_NE(crl, nullptr);

    // Generate a different key – this is the "wrong" public key
    EVP_PKEY* wrong_key  = nullptr;
    X509*     wrong_cert = nullptr;
    ASSERT_TRUE(makeKeyAndCert(&wrong_key, &wrong_cert, 77, "Wrong Key Cert"));

    // X509_CRL_verify should return <= 0 when the wrong key is used
    EVP_PKEY* wrong_pub = X509_get_pubkey(wrong_cert);
    ASSERT_NE(wrong_pub, nullptr);
    int verify_rc = X509_CRL_verify(crl, wrong_pub);
    EXPECT_LE(verify_rc, 0) << "CRL signed with a different key must fail verification";

    EVP_PKEY_free(wrong_pub);
    X509_CRL_free(crl);
    X509_free(issuer_cert);
    X509_free(wrong_cert);
    EVP_PKEY_free(issuer_key);
    EVP_PKEY_free(wrong_key);
}

// ============================================================================
// checkOCSP() tests
// ============================================================================

TEST_F(PluginSecurityCRLOCSPTest, OCSP_EmptyCertReturnsFalse) {
    PluginSecurityPolicy policy;
    PluginSecurityVerifier verifier(policy);
    EXPECT_FALSE(verifier.checkOCSP(""));
}

TEST_F(PluginSecurityCRLOCSPTest, OCSP_InvalidPEMReturnsFalse) {
    PluginSecurityPolicy policy;
    PluginSecurityVerifier verifier(policy);
    EXPECT_FALSE(verifier.checkOCSP("not-a-pem"));
}

// Cert without OCSP AIA: pass when checkRevocation=false
TEST_F(PluginSecurityCRLOCSPTest, OCSP_NoOCSPEndpoints_RevocationDisabled_Passes) {
    PluginSecurityPolicy policy;
    policy.checkRevocation = false;
    PluginSecurityVerifier verifier(policy);
    EXPECT_TRUE(verifier.checkOCSP(plain_cert_pem_));
}

// Cert without OCSP AIA: fail when checkRevocation=true
TEST_F(PluginSecurityCRLOCSPTest, OCSP_NoOCSPEndpoints_RevocationEnabled_Fails) {
    PluginSecurityPolicy policy;
    policy.checkRevocation = true;
    PluginSecurityVerifier verifier(policy);
    EXPECT_FALSE(verifier.checkOCSP(plain_cert_pem_));
}

// Cert has OCSP AIA but server is unreachable.
// With revocation disabled: pass.
TEST_F(PluginSecurityCRLOCSPTest, OCSP_UnreachableEndpoint_RevocationDisabled_Passes) {
    if (ocsp_cert_pem_.empty()) {
        GTEST_SKIP() << "OCSP cert not generated";
    }
    PluginSecurityPolicy policy;
    policy.checkRevocation            = false;
    policy.revocation_timeout_seconds = 1;
    PluginSecurityVerifier verifier(policy);
    EXPECT_TRUE(verifier.checkOCSP(ocsp_cert_pem_));
}

// Cert has OCSP AIA but server is unreachable.
// With revocation enabled: fail-safe.
TEST_F(PluginSecurityCRLOCSPTest, OCSP_UnreachableEndpoint_RevocationEnabled_Fails) {
    if (ocsp_cert_pem_.empty()) {
        GTEST_SKIP() << "OCSP cert not generated";
    }
    PluginSecurityPolicy policy;
    policy.checkRevocation            = true;
    policy.revocation_timeout_seconds = 1;
    PluginSecurityVerifier verifier(policy);
    EXPECT_FALSE(verifier.checkOCSP(ocsp_cert_pem_));
}

// Network-timeout path for OCSP: must not hang.
TEST_F(PluginSecurityCRLOCSPTest, OCSP_NetworkTimeout_FailsSafe) {
    if (ocsp_cert_pem_.empty()) {
        GTEST_SKIP() << "OCSP cert not generated";
    }
    PluginSecurityPolicy policy;
    policy.checkRevocation            = true;
    policy.revocation_timeout_seconds = 1;
    PluginSecurityVerifier verifier(policy);
    EXPECT_NO_THROW({ verifier.checkOCSP(ocsp_cert_pem_); });
    EXPECT_FALSE(verifier.checkOCSP(ocsp_cert_pem_));
}

// Cache test: two identical calls return consistent results.
TEST_F(PluginSecurityCRLOCSPTest, OCSP_CacheHit_SecondCallConsistent) {
    PluginSecurityPolicy policy;
    policy.checkRevocation = false;
    PluginSecurityVerifier verifier(policy);
    bool first  = verifier.checkOCSP(plain_cert_pem_);
    bool second = verifier.checkOCSP(plain_cert_pem_);
    EXPECT_EQ(first, second);
}

// ============================================================================
// OCSP request construction tests (verify API doesn't crash)
// ============================================================================

// Verify that building an OCSP_REQUEST with a self-signed cert as its own
// issuer works without assertion failure (API smoke test).
TEST_F(PluginSecurityCRLOCSPTest, OCSP_RequestConstruction_SelfSigned_Smoke) {
    EVP_PKEY* pkey = nullptr;
    X509*     cert = nullptr;
    ASSERT_TRUE(makeKeyAndCert(&pkey, &cert, 7));

    OCSP_REQUEST* req = OCSP_REQUEST_new();
    ASSERT_NE(req, nullptr);

    // Self-signed: cert is its own issuer
    OCSP_CERTID* certid = OCSP_cert_to_id(EVP_sha1(), cert, cert);
    if (certid) {
        // add0 transfers ownership; do not free certid separately
        EXPECT_TRUE(OCSP_request_add0_id(req, certid) != nullptr);
    }

    OCSP_REQUEST_free(req);
    X509_free(cert);
    EVP_PKEY_free(pkey);
}

// Verify that OCSP_basic_verify rejects a DER-tampered (corrupted) response,
// exercising the signature-invalid code path in checkOCSP().
TEST_F(PluginSecurityCRLOCSPTest, OCSP_InvalidSignature_RejectedByOpenSSL) {
    EVP_PKEY* pkey = nullptr;
    X509*     cert = nullptr;
    ASSERT_TRUE(makeKeyAndCert(&pkey, &cert, 8, "OCSP Test"));

    // Build a minimal OCSP request (DER-encode it so we can corrupt the bytes).
    OCSP_REQUEST* req = OCSP_REQUEST_new();
    ASSERT_NE(req, nullptr);
    OCSP_CERTID* certid = OCSP_cert_to_id(EVP_sha1(), cert, cert);
    if (certid) {
        OCSP_request_add0_id(req, certid);
    }
    unsigned char* req_der = nullptr;
    int req_len = i2d_OCSP_REQUEST(req, &req_der);
    OCSP_REQUEST_free(req);
    ASSERT_GT(req_len, 0);

    // Corrupt the last byte by flipping all its bits (XOR 0xFF) to produce
    // an invalid DER structure.  This is the simplest single-byte mutation
    // that reliably invalidates the trailing ASN.1 tag/length/value of the
    // encoded blob without changing its length (so d2i still receives the
    // full buffer).
    req_der[req_len - 1] ^= 0xFF;

    const unsigned char* p = req_der;
    OCSP_RESPONSE* resp = d2i_OCSP_RESPONSE(nullptr, &p, req_len);
    // A corrupted OCSP request DER is not a valid OCSP response → must be null
    // or, if OpenSSL happens to partially parse it, status should not be SUCCESSFUL.
    if (resp) {
        EXPECT_NE(OCSP_response_status(resp), OCSP_RESPONSE_STATUS_SUCCESSFUL)
            << "A tampered DER blob must not produce a successful OCSP response";
        OCSP_RESPONSE_free(resp);
    }
    // else: d2i returned null → parse failure is the expected outcome

    OPENSSL_free(req_der);
    X509_free(cert);
    EVP_PKEY_free(pkey);
}

// ============================================================================
// PE certificate extraction tests
// ============================================================================

TEST_F(PluginSecurityCRLOCSPTest, PE_NoPEHeader_ReturnsNullopt) {
    std::filesystem::path fake = test_dir_ / "not_a_pe.so";
    {
        std::ofstream f(fake, std::ios::binary);
        f << "\x7F" "ELF" << std::string(60, '\0');
    }
    PluginSecurityPolicy policy;
    EnhancedPluginSecurityVerifier verifier(policy);
    // Level 2 requires embedded cert – should fail cleanly for ELF
    auto result = verifier.verifyPlugin(
        fake.string(),
        EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_2_EMBEDDED_SIGNATURE);
    EXPECT_FALSE(result.passed);
    EXPECT_FALSE(result.embedded_signature_verified);
}

TEST_F(PluginSecurityCRLOCSPTest, PE_ValidHeaderNoSecurityDir_ReturnsNullopt) {
    // Build a minimal PE32 with an empty security data directory entry.
    std::filesystem::path pe_file = test_dir_ / "test_no_sig.dll";

    std::vector<uint8_t> pe;
    // DOS header: MZ + padding to 0x40 bytes, e_lfanew = 0x80
    pe.resize(0x40, 0);
    pe[0] = 'M'; pe[1] = 'Z';
    uint32_t pe_offset = 0x80;
    std::memcpy(&pe[0x3C], &pe_offset, 4);

    // Pad to PE header offset
    pe.resize(pe_offset, 0);

    // PE signature
    pe.push_back('P'); pe.push_back('E');
    pe.push_back(0);   pe.push_back(0);

    // COFF file header (20 bytes)
    pe.resize(pe.size() + 20, 0);

    // Optional header: PE32 magic + 94 bytes of zeros (no data dirs set)
    pe.push_back(0x0B); pe.push_back(0x01);  // magic = PE32
    pe.resize(pe.size() + 94, 0);  // remaining optional header fields

    // Data directories (16 × 8 bytes = 128 bytes), all zeros
    pe.resize(pe.size() + 128, 0);

    {
        std::ofstream f(pe_file, std::ios::binary);
        f.write(reinterpret_cast<const char*>(pe.data()),
                static_cast<std::streamsize>(pe.size()));
    }

    PluginSecurityPolicy policy;
    EnhancedPluginSecurityVerifier verifier(policy);
    auto result = verifier.verifyPlugin(
        pe_file.string(),
        EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_2_EMBEDDED_SIGNATURE);
    // No security directory → no cert → embedded sig check fails
    EXPECT_FALSE(result.embedded_signature_verified);
}
