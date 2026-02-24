/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_mtls_authenticator.cpp                        ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-24                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     497                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "auth/mtls_authenticator.h"
#include "auth/auth_error.h"

#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/bn.h>
#include <openssl/err.h>

#include <string>
#include <vector>
#include <chrono>

using namespace themis::auth;

// ============================================================================
// Test certificate generation helpers
// ============================================================================

namespace {

struct CertKeyPair {
    std::string cert_pem;
    std::string key_pem;
    EVP_PKEY* pkey = nullptr;
    X509*     cert = nullptr;

    ~CertKeyPair() {
        if (pkey) EVP_PKEY_free(pkey);
        if (cert) X509_free(cert);
    }
};

/** Generate an RSA key pair (1024-bit for test speed). */
EVP_PKEY* generateTestKey() {
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    return EVP_RSA_gen(1024);
#else
    BIGNUM* exponent = BN_new();
    BN_set_word(exponent, RSA_F4);
    RSA* rsa = RSA_new();
    RSA_generate_key_ex(rsa, 1024, exponent, nullptr);
    BN_free(exponent);
    EVP_PKEY* pkey = EVP_PKEY_new();
    EVP_PKEY_assign_RSA(pkey, rsa);
    return pkey;
#endif
}

/**
 * Create a self-signed X.509 certificate.
 * @param subject_cn   Common Name for the Subject (and Issuer for self-signed).
 * @param pkey         Key pair to sign with.
 * @param days_valid   Validity period in days (negative = already expired).
 * @param issuer_pkey  If != pkey, sign with this key (for CA-signed certs).
 * @param issuer_cert  If non-null, use as issuer DN (for CA-signed certs).
 */
X509* buildCert(const std::string& subject_cn,
                EVP_PKEY* pkey,
                int days_valid = 365,
                EVP_PKEY* issuer_pkey = nullptr,
                X509*     issuer_cert = nullptr) {
    X509* cert = X509_new();
    X509_set_version(cert, 2);

    // Serial number
    ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);

    // Validity
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), static_cast<long>(days_valid) * 86400L);

    // Subject
    X509_NAME* name = X509_get_subject_name(cert);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                               reinterpret_cast<const unsigned char*>(subject_cn.c_str()),
                               -1, -1, 0);

    // Issuer
    if (issuer_cert) {
        X509_set_issuer_name(cert, X509_get_subject_name(issuer_cert));
    } else {
        X509_set_issuer_name(cert, name);
    }

    // Public key
    X509_set_pubkey(cert, pkey);

    // Sign
    EVP_PKEY* signing_key = issuer_pkey ? issuer_pkey : pkey;
    X509_sign(cert, signing_key, EVP_sha256());

    return cert;
}

/** Encode X509* to PEM string. */
std::string certToPEM(X509* cert) {
    BIO* bio = BIO_new(BIO_s_mem());
    PEM_write_bio_X509(bio, cert);
    BUF_MEM* bptr = nullptr;
    BIO_get_mem_ptr(bio, &bptr);
    std::string pem(bptr->data, bptr->length);
    BIO_free(bio);
    return pem;
}

/**
 * Create a CA cert + client cert pair.
 * Returns {ca_pem, client_pem}.
 */
std::pair<std::string, std::string> makeCAAndClientCert(
    const std::string& ca_cn     = "TestCA",
    const std::string& client_cn = "test-client.example.com",
    int client_days              = 365)
{
    EVP_PKEY* ca_key     = generateTestKey();
    EVP_PKEY* client_key = generateTestKey();

    X509* ca_cert     = buildCert(ca_cn, ca_key);
    X509* client_cert = buildCert(client_cn, client_key, client_days, ca_key, ca_cert);

    std::string ca_pem     = certToPEM(ca_cert);
    std::string client_pem = certToPEM(client_cert);

    X509_free(ca_cert);
    X509_free(client_cert);
    EVP_PKEY_free(ca_key);
    EVP_PKEY_free(client_key);

    return {ca_pem, client_pem};
}

} // anonymous namespace

// ============================================================================
// MTLSAuthenticator – configuration and initialization tests
// ============================================================================

TEST(MTLSAuthenticatorTest, UninitializedThrowsOnAuthenticate) {
    MTLSAuthenticator auth;
    EXPECT_FALSE(auth.isInitialized());

    EXPECT_THROW(auth.authenticate("some-pem"), AuthException);
}

TEST(MTLSAuthenticatorTest, InitFailsWithoutCA) {
    MTLSAuthenticator auth;
    MTLSConfig cfg;
    // No CA configured
    EXPECT_FALSE(auth.initialize(cfg));
    EXPECT_FALSE(auth.isInitialized());
}

TEST(MTLSAuthenticatorTest, InitSucceedsWithInlineCA) {
    auto [ca_pem, _client_pem] = makeCAAndClientCert();

    MTLSAuthenticator auth;
    MTLSConfig cfg;
    cfg.ca_cert_pem = ca_pem;

    EXPECT_TRUE(auth.initialize(cfg));
    EXPECT_TRUE(auth.isInitialized());
}

TEST(MTLSAuthenticatorTest, InitSucceedsWithInlinePEMBundle) {
    // Build two independent CA certs and concatenate them into a bundle.
    // A client cert signed by CA2 should authenticate when both are trusted.
    auto [ca1_pem, _client1] = makeCAAndClientCert("BundleCA1", "client1");
    auto [ca2_pem, client2]  = makeCAAndClientCert("BundleCA2", "client2");

    std::string bundle = ca1_pem + ca2_pem;

    MTLSAuthenticator auth;
    MTLSConfig cfg;
    cfg.ca_cert_pem = bundle;
    ASSERT_TRUE(auth.initialize(cfg));

    // client2 is signed by CA2 which is in the bundle → should succeed
    EXPECT_NO_THROW(auth.authenticate(client2));
}

TEST(MTLSAuthenticatorTest, InitIdempotent) {
    auto [ca_pem, _] = makeCAAndClientCert();

    MTLSAuthenticator auth;
    MTLSConfig cfg;
    cfg.ca_cert_pem = ca_pem;

    EXPECT_TRUE(auth.initialize(cfg));
    // Second call should succeed silently
    EXPECT_TRUE(auth.initialize(cfg));
    EXPECT_TRUE(auth.isInitialized());
}

// ============================================================================
// MTLSAuthenticator – happy-path authentication
// ============================================================================

TEST(MTLSAuthenticatorTest, ValidCertAuthenticated) {
    const std::string cn = "alice.example.com";
    auto [ca_pem, client_pem] = makeCAAndClientCert("TestCA", cn);

    MTLSAuthenticator auth;
    MTLSConfig cfg;
    cfg.ca_cert_pem = ca_pem;
    ASSERT_TRUE(auth.initialize(cfg));

    MTLSClaims claims = auth.authenticate(client_pem);

    EXPECT_FALSE(claims.subject_dn.empty());
    EXPECT_FALSE(claims.principal.empty());
    // CN should be the client CN
    EXPECT_NE(claims.principal.find(cn), std::string::npos);
    EXPECT_FALSE(claims.issuer_dn.empty());
    EXPECT_FALSE(claims.serial_number.empty());
}

TEST(MTLSAuthenticatorTest, PrincipalIsExtractedCN) {
    const std::string cn = "service-account";
    auto [ca_pem, client_pem] = makeCAAndClientCert("TestCA", cn);

    MTLSAuthenticator auth;
    MTLSConfig cfg;
    cfg.ca_cert_pem       = ca_pem;
    cfg.principal_field   = "CN";
    ASSERT_TRUE(auth.initialize(cfg));

    auto claims = auth.authenticate(client_pem);
    EXPECT_EQ(claims.principal, cn);
}

TEST(MTLSAuthenticatorTest, PrincipalIsFullDNWhenFieldIsDN) {
    const std::string cn = "svc";
    auto [ca_pem, client_pem] = makeCAAndClientCert("TestCA", cn);

    MTLSAuthenticator auth;
    MTLSConfig cfg;
    cfg.ca_cert_pem     = ca_pem;
    cfg.principal_field = "DN";
    ASSERT_TRUE(auth.initialize(cfg));

    auto claims = auth.authenticate(client_pem);
    // DN principal should contain CN value
    EXPECT_NE(claims.principal.find(cn), std::string::npos);
    // And should be longer than the CN alone (full DN)
    EXPECT_GE(claims.principal.size(), cn.size());
}

// ============================================================================
// MTLSAuthenticator – subject mapping
// ============================================================================

TEST(MTLSAuthenticatorTest, SubjectMappingAssignsRole) {
    const std::string cn = "ops-agent";
    auto [ca_pem, client_pem] = makeCAAndClientCert("TestCA", cn);

    MTLSAuthenticator auth;
    MTLSConfig cfg;
    cfg.ca_cert_pem = ca_pem;
    cfg.subject_mappings = {
        {"*ops-agent*", "ops:admin", "tenant-ops"},
    };
    ASSERT_TRUE(auth.initialize(cfg));

    auto claims = auth.authenticate(client_pem);
    ASSERT_EQ(claims.roles.size(), 1u);
    EXPECT_EQ(claims.roles[0], "ops:admin");
    EXPECT_EQ(claims.tenant_id, "tenant-ops");
}

TEST(MTLSAuthenticatorTest, NoMatchingMappingYieldsEmptyRoles) {
    const std::string cn = "unknown-client";
    auto [ca_pem, client_pem] = makeCAAndClientCert("TestCA", cn);

    MTLSAuthenticator auth;
    MTLSConfig cfg;
    cfg.ca_cert_pem  = ca_pem;
    cfg.subject_mappings = {
        {"*known-service*", "svc:read", ""},
    };
    ASSERT_TRUE(auth.initialize(cfg));

    auto claims = auth.authenticate(client_pem);
    EXPECT_TRUE(claims.roles.empty());
}

TEST(MTLSAuthenticatorTest, MultipleMatchingMappingsAggregateRoles) {
    const std::string cn = "worker.ops.example.com";
    auto [ca_pem, client_pem] = makeCAAndClientCert("TestCA", cn);

    MTLSAuthenticator auth;
    MTLSConfig cfg;
    cfg.ca_cert_pem  = ca_pem;
    cfg.subject_mappings = {
        {"*ops*",     "ops:read",  "tenant-ops"},
        {"*worker*",  "jobs:exec", ""},
    };
    ASSERT_TRUE(auth.initialize(cfg));

    auto claims = auth.authenticate(client_pem);
    EXPECT_EQ(claims.roles.size(), 2u);
    EXPECT_EQ(claims.tenant_id, "tenant-ops"); // first match wins for tenant
}

// ============================================================================
// MTLSAuthenticator – error cases
// ============================================================================

TEST(MTLSAuthenticatorTest, InvalidPEMThrows) {
    auto [ca_pem, _] = makeCAAndClientCert();

    MTLSAuthenticator auth;
    MTLSConfig cfg;
    cfg.ca_cert_pem = ca_pem;
    ASSERT_TRUE(auth.initialize(cfg));

    EXPECT_THROW(auth.authenticate("not-a-pem"), AuthException);
    EXPECT_THROW(auth.authenticate("-----BEGIN CERTIFICATE-----\ninvalid\n-----END CERTIFICATE-----"),
                 AuthException);
}

TEST(MTLSAuthenticatorTest, CertSignedByUnknownCAThrows) {
    // Create cert signed by a different CA than the one configured
    auto [ca_pem_1, _client1] = makeCAAndClientCert("CA1", "client1");
    auto [ca_pem_2, client2]  = makeCAAndClientCert("CA2", "client2");

    MTLSAuthenticator auth;
    MTLSConfig cfg;
    cfg.ca_cert_pem = ca_pem_1;  // Trust CA1 only
    ASSERT_TRUE(auth.initialize(cfg));

    // client2 is signed by CA2 → should fail
    EXPECT_THROW(auth.authenticate(client2), AuthException);
}

TEST(MTLSAuthenticatorTest, ExpiredCertThrowsWhenVerifyEnabled) {
    // Create a cert that expired 1 day ago
    auto [ca_pem, client_pem] = makeCAAndClientCert("TestCA", "expired-client", -1);

    MTLSAuthenticator auth;
    MTLSConfig cfg;
    cfg.ca_cert_pem  = ca_pem;
    cfg.verify_expiry = true;
    ASSERT_TRUE(auth.initialize(cfg));

    EXPECT_THROW(auth.authenticate(client_pem), AuthException);
}

TEST(MTLSAuthenticatorTest, ExpiredCertAcceptedWhenVerifyDisabled) {
    auto [ca_pem, client_pem] = makeCAAndClientCert("TestCA", "expired-client", -1);

    MTLSAuthenticator auth;
    MTLSConfig cfg;
    cfg.ca_cert_pem   = ca_pem;
    cfg.verify_expiry = false;
    ASSERT_TRUE(auth.initialize(cfg));

    // With verify_expiry=false, X509_V_FLAG_NO_CHECK_TIME is set so OpenSSL
    // ignores the validity period; the cert should authenticate successfully.
    EXPECT_NO_THROW(auth.authenticate(client_pem));
}

TEST(MTLSAuthenticatorTest, OversizedInputThrows) {
    auto [ca_pem, _] = makeCAAndClientCert();

    MTLSAuthenticator auth;
    MTLSConfig cfg;
    cfg.ca_cert_pem = ca_pem;
    ASSERT_TRUE(auth.initialize(cfg));

    // Build a string larger than MAX_MTLS_CERT_PEM_SIZE
    std::string huge(MAX_MTLS_CERT_PEM_SIZE + 1, 'A');
    EXPECT_THROW(auth.authenticate(huge), AuthException);
}

// ============================================================================
// MTLSAuthenticator – extractPrincipal and mapSubjectToRoles unit tests
// ============================================================================

TEST(MTLSAuthenticatorTest, ExtractPrincipalFromCNField) {
    MTLSAuthenticator auth;
    MTLSConfig cfg;
    cfg.ca_cert_pem     = "placeholder"; // won't call initialize
    cfg.principal_field = "CN";

    // Manually set config via const_cast for unit testing helper
    // (alternative: expose via friend or test-only constructor)
    // Instead: initialize with a real CA cert so the method is accessible
    auto [ca_pem, _] = makeCAAndClientCert();
    cfg.ca_cert_pem = ca_pem;
    ASSERT_TRUE(auth.initialize(cfg));

    EXPECT_EQ(auth.extractPrincipal("CN=alice,O=Corp,C=US"), "alice");
    EXPECT_EQ(auth.extractPrincipal("CN=svc.example.com,OU=Ops"), "svc.example.com");
    EXPECT_EQ(auth.extractPrincipal("O=Corp,C=US"), "");   // no CN → empty
}

TEST(MTLSAuthenticatorTest, ExtractPrincipalFullDN) {
    auto [ca_pem, _] = makeCAAndClientCert();

    MTLSAuthenticator auth;
    MTLSConfig cfg;
    cfg.ca_cert_pem     = ca_pem;
    cfg.principal_field = "DN";
    ASSERT_TRUE(auth.initialize(cfg));

    const std::string dn = "CN=worker,O=Corp,C=US";
    EXPECT_EQ(auth.extractPrincipal(dn), dn);
}

TEST(MTLSAuthenticatorTest, MapSubjectToRolesWildcard) {
    auto [ca_pem, _] = makeCAAndClientCert();

    MTLSAuthenticator auth;
    MTLSConfig cfg;
    cfg.ca_cert_pem  = ca_pem;
    cfg.subject_mappings = {
        {"CN=admin*", "admin:full", "admin-tenant"},
        {"*",         "read:any",  ""},
    };
    ASSERT_TRUE(auth.initialize(cfg));

    auto [roles1, tenant1] = auth.mapSubjectToRoles("CN=admin-user,O=Corp");
    ASSERT_EQ(roles1.size(), 2u);
    EXPECT_EQ(roles1[0], "admin:full");
    EXPECT_EQ(roles1[1], "read:any");
    EXPECT_EQ(tenant1, "admin-tenant");

    auto [roles2, tenant2] = auth.mapSubjectToRoles("CN=regular-user,O=Corp");
    ASSERT_EQ(roles2.size(), 1u);
    EXPECT_EQ(roles2[0], "read:any");
    EXPECT_TRUE(tenant2.empty());
}

// ============================================================================
// AuthMiddleware mTLS integration (via AuthMiddleware::enableMTLS)
// ============================================================================

#include "server/auth_middleware.h"
using namespace themis;

TEST(AuthMiddlewareMTLSTest, EnableMTLSAndAuthorize) {
    auto [ca_pem, client_pem] = makeCAAndClientCert("TestCA", "svc.example.com");

    AuthMiddleware middleware;

    MTLSConfig cfg;
    cfg.ca_cert_pem  = ca_pem;
    cfg.subject_mappings = {
        {"*svc.example.com*", "api:access", "tenant-1"},
    };
    middleware.enableMTLS(cfg);

    EXPECT_TRUE(middleware.isEnabled());

    // The client certificate PEM is used as the "token"
    auto result = middleware.authorize(client_pem, "api:access");
    EXPECT_TRUE(result.authorized);
    EXPECT_EQ(result.user_id, "svc.example.com");
    EXPECT_EQ(result.tenant_id, "tenant-1");
}

TEST(AuthMiddlewareMTLSTest, InvalidCertDenied) {
    auto [ca_pem, _] = makeCAAndClientCert();

    AuthMiddleware middleware;
    MTLSConfig cfg;
    cfg.ca_cert_pem = ca_pem;
    middleware.enableMTLS(cfg);

    auto result = middleware.authorize("not-a-certificate", "any:scope");
    EXPECT_FALSE(result.authorized);
    EXPECT_FALSE(result.reason.empty());
}
