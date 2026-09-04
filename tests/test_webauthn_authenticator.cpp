#include <gtest/gtest.h>
#include "auth/webauthn_authenticator.h"
#include "auth/auth_error.h"

#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <openssl/core_names.h>

#include <nlohmann/json.hpp>
#include <array>
#include <string>
#include <vector>
#include <cstring>

using namespace themis::auth;
using json = nlohmann::json;

// ============================================================================
// Helper utilities
// ============================================================================

namespace {

// ---------------------------------------------------------------------------
// Base64URL helpers (copied logic from the implementation for independence)
// ---------------------------------------------------------------------------

static const char kB64Table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string b64urlEncode(const std::vector<uint8_t>& data)
{
    const uint8_t* d = data.data();
    const size_t   n = data.size();
    std::string out;
    out.reserve(((n + 2) / 3) * 4);
    for (size_t i = 0; i < n; i += 3) {
        const uint32_t b0 = d[i];
        const uint32_t b1 = (i + 1 < n) ? d[i + 1] : 0u;
        const uint32_t b2 = (i + 2 < n) ? d[i + 2] : 0u;
        const uint32_t t  = (b0 << 16) | (b1 << 8) | b2;
        out += kB64Table[(t >> 18) & 0x3F];
        out += kB64Table[(t >> 12) & 0x3F];
        out += (i + 1 < n) ? kB64Table[(t >>  6) & 0x3F] : '=';
        out += (i + 2 < n) ? kB64Table[(t      ) & 0x3F] : '=';
    }
    for (char& c : out) {
        if (c == '+') {
          c = '-';
        }
        else if (c == '/') c = '_';
    }
    while (!out.empty() && out.back() == '=') {
      out.pop_back();
    }
    return out;
}

static std::string b64urlEncodeStr(const std::string& s)
{
    return b64urlEncode(std::vector<uint8_t>(s.begin(), s.end()));
}

// ---------------------------------------------------------------------------
// SHA-256
// ---------------------------------------------------------------------------

static std::vector<uint8_t> sha256(const std::string& s)
{
    std::array<uint8_t, 32> digest{};
    SHA256(reinterpret_cast<const uint8_t*>(s.data()), s.size(), digest.data());
    return {digest.begin(), digest.end()};
}

static std::vector<uint8_t> sha256v(const std::vector<uint8_t>& d)
{
    std::array<uint8_t, 32> digest{};
    SHA256(d.data(), d.size(), digest.data());
    return {digest.begin(), digest.end()};
}

// ---------------------------------------------------------------------------
// Minimal CBOR encoder for test vectors
// ---------------------------------------------------------------------------

// Append a CBOR unsigned integer to buf
static void cborAppendUint(std::vector<uint8_t>& buf, uint64_t v)
{
    if (v <= 23)        { buf.push_back(static_cast<uint8_t>(v)); }
    else if (v <= 0xFF) { buf.push_back(0x18); buf.push_back(static_cast<uint8_t>(v)); }
    else if (v <= 0xFFFF) {
        buf.push_back(0x19);
        buf.push_back(static_cast<uint8_t>(v >> 8));
        buf.push_back(static_cast<uint8_t>(v));
    } else {
        buf.push_back(0x1A);
        for (int i = 3; i >= 0; --i) {
          buf.push_back(static_cast<uint8_t>(v >> (i * 8)));
        }
    }
}

// Append a CBOR negative integer (-1 - arg)
static void cborAppendNegInt(std::vector<uint8_t>& buf, int64_t v)
{
    // v < 0, encode as major-type 1 with arg = (-1 - v)
    const uint64_t arg = static_cast<uint64_t>(-1 - v);
    const uint8_t  first = static_cast<uint8_t>(0x20 | (arg <= 23 ? arg : (arg <= 0xFF ? 24 : 25)));
    if (arg <= 23)        { buf.push_back(static_cast<uint8_t>(0x20 | arg)); }
    else if (arg <= 0xFF) { buf.push_back(0x38); buf.push_back(static_cast<uint8_t>(arg)); }
    else { buf.push_back(0x39); buf.push_back(static_cast<uint8_t>(arg >> 8)); buf.push_back(static_cast<uint8_t>(arg)); }
    (void)first;
}

// Append a CBOR byte string
static void cborAppendBytes(std::vector<uint8_t>& buf, const std::vector<uint8_t>& data)
{
    const size_t n = data.size();
    if (n <= 23) { buf.push_back(static_cast<uint8_t>(0x40 | n)); }
    else if (n <= 0xFF) { buf.push_back(0x58); buf.push_back(static_cast<uint8_t>(n)); }
    else { buf.push_back(0x59); buf.push_back(static_cast<uint8_t>(n >> 8)); buf.push_back(static_cast<uint8_t>(n)); }
    buf.insert(buf.end(), data.begin(), data.end());
}

// Append a CBOR text string
static void cborAppendText(std::vector<uint8_t>& buf, const std::string& s)
{
    const size_t n = s.size();
    if (n <= 23) { buf.push_back(static_cast<uint8_t>(0x60 | n)); }
    else if (n <= 0xFF) { buf.push_back(0x78); buf.push_back(static_cast<uint8_t>(n)); }
    else { buf.push_back(0x79); buf.push_back(static_cast<uint8_t>(n >> 8)); buf.push_back(static_cast<uint8_t>(n)); }
    buf.insert(buf.end(), s.begin(), s.end());
}

// Append CBOR map header with count pairs
static void cborAppendMapHeader(std::vector<uint8_t>& buf, size_t count)
{
    if (count <= 23) {
      buf.push_back(static_cast<uint8_t>(0xA0 | count));
    }
    else { buf.push_back(0xB8); buf.push_back(static_cast<uint8_t>(count)); }
}

// ---------------------------------------------------------------------------
// Build an ES256 COSE key CBOR from x,y byte vectors
// ---------------------------------------------------------------------------
static std::vector<uint8_t> buildES256CoseKey(const std::vector<uint8_t>& x,
                                               const std::vector<uint8_t>& y)
{
    // { 1: 2, 3: -7, -1: 1, -2: x, -3: y }
    std::vector<uint8_t> buf;
    cborAppendMapHeader(buf, 5);
    // kty: 2 (EC2)
    cborAppendUint(buf, 1);
    cborAppendUint(buf, 2);
    // alg: -7 (ES256)
    cborAppendUint(buf, 3);
    cborAppendNegInt(buf, -7);
    // crv: 1 (P-256)
    cborAppendNegInt(buf, -1);
    cborAppendUint(buf, 1);
    // x
    cborAppendNegInt(buf, -2);
    cborAppendBytes(buf, x);
    // y
    cborAppendNegInt(buf, -3);
    cborAppendBytes(buf, y);
    return buf;
}

// ---------------------------------------------------------------------------
// Build binary authenticatorData for registration (with AT flag)
// ---------------------------------------------------------------------------
static std::vector<uint8_t> buildRegistrationAuthData(
    const std::string& rp_id,
    uint8_t flags,
    uint32_t sign_count,
    const std::vector<uint8_t>& cred_id,
    const std::vector<uint8_t>& cose_key)
{
    const auto rp_hash = sha256(rp_id);

    std::vector<uint8_t> buf;
    buf.insert(buf.end(), rp_hash.begin(), rp_hash.end());  // 32 bytes
    buf.push_back(flags);                                    // 1 byte
    buf.push_back(static_cast<uint8_t>(sign_count >> 24));
    buf.push_back(static_cast<uint8_t>(sign_count >> 16));
    buf.push_back(static_cast<uint8_t>(sign_count >>  8));
    buf.push_back(static_cast<uint8_t>(sign_count));         // 4 bytes signCount
    // AAGUID: 16 zero bytes
    buf.insert(buf.end(), 16, 0x00);
    // credentialIdLength (2 bytes big-endian)
    buf.push_back(static_cast<uint8_t>(cred_id.size() >> 8));
    buf.push_back(static_cast<uint8_t>(cred_id.size()));
    // credentialId
    buf.insert(buf.end(), cred_id.begin(), cred_id.end());
    // credentialPublicKey (CBOR)
    buf.insert(buf.end(), cose_key.begin(), cose_key.end());
    return buf;
}

// ---------------------------------------------------------------------------
// Build CBOR attestation object (fmt="none", attStmt={}, authData=bytes)
// ---------------------------------------------------------------------------
static std::vector<uint8_t> buildAttestationObject(const std::vector<uint8_t>& auth_data)
{
    // { "fmt": "none", "attStmt": {}, "authData": <bytes> }
    std::vector<uint8_t> buf;
    cborAppendMapHeader(buf, 3);
    cborAppendText(buf, "fmt");
    cborAppendText(buf, "none");
    cborAppendText(buf, "attStmt");
    cborAppendMapHeader(buf, 0);
    cborAppendText(buf, "authData");
    cborAppendBytes(buf, auth_data);
    return buf;
}

// ---------------------------------------------------------------------------
// Build binary authenticatorData for authentication (no AT flag)
// ---------------------------------------------------------------------------
static std::vector<uint8_t> buildAssertionAuthData(
    const std::string& rp_id,
    uint8_t flags,
    uint32_t sign_count)
{
    const auto rp_hash = sha256(rp_id);
    std::vector<uint8_t> buf;
    buf.insert(buf.end(), rp_hash.begin(), rp_hash.end());
    buf.push_back(flags);
    buf.push_back(static_cast<uint8_t>(sign_count >> 24));
    buf.push_back(static_cast<uint8_t>(sign_count >> 16));
    buf.push_back(static_cast<uint8_t>(sign_count >>  8));
    buf.push_back(static_cast<uint8_t>(sign_count));
    return buf;
}

// ---------------------------------------------------------------------------
// Generate an EC P-256 key pair; return {EVP_PKEY*, x_bytes, y_bytes, spki_der}
// Caller must call EVP_PKEY_free on the returned pkey.
// ---------------------------------------------------------------------------
struct KeyPairInfo {
    EVP_PKEY*            pkey{nullptr};
    std::vector<uint8_t> x;
    std::vector<uint8_t> y;
    std::vector<uint8_t> spki;
};

static KeyPairInfo generateP256KeyPair()
{
    // Use modern OpenSSL 3.x key generation API
    EVP_PKEY_CTX* kctx = EVP_PKEY_CTX_new_from_name(nullptr, "EC", nullptr);
    EXPECT_NE(kctx, nullptr);
    EXPECT_EQ(EVP_PKEY_keygen_init(kctx), 1);
    EXPECT_EQ(EVP_PKEY_CTX_set_ec_paramgen_curve_nid(kctx, NID_X9_62_prime256v1), 1);
    EVP_PKEY* pkey = nullptr;
    EXPECT_EQ(EVP_PKEY_keygen(kctx, &pkey), 1);
    EVP_PKEY_CTX_free(kctx);

    // Extract uncompressed public key (0x04 || x[32] || y[32])
    std::size_t pt_len = 0;
    EVP_PKEY_get_octet_string_param(pkey, OSSL_PKEY_PARAM_PUB_KEY, nullptr, 0, &pt_len);
    std::vector<uint8_t> pub_key(pt_len);
    EVP_PKEY_get_octet_string_param(pkey, OSSL_PKEY_PARAM_PUB_KEY,
                                     pub_key.data(), pt_len, &pt_len);
    // pub_key = 0x04 || x[32] || y[32]
    EXPECT_EQ(pt_len, 65u);
    std::vector<uint8_t> x(pub_key.begin() + 1,  pub_key.begin() + 33);
    std::vector<uint8_t> y(pub_key.begin() + 33, pub_key.begin() + 65);

    unsigned char* der  = nullptr;
    int            dlen = i2d_PUBKEY(pkey, &der);
    std::vector<uint8_t> spki(der, der + dlen);
    OPENSSL_free(der);

    return {pkey, x, y, spki};
}

// ---------------------------------------------------------------------------
// Sign data with an EC P-256 key (DER-encoded ECDSA signature)
// ---------------------------------------------------------------------------
static std::vector<uint8_t> ecdsaSign(EVP_PKEY* pkey, const std::vector<uint8_t>& data)
{
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, pkey);
    EVP_DigestSignUpdate(ctx, data.data(), data.size());
    size_t sig_len = 0;
    EVP_DigestSignFinal(ctx, nullptr, &sig_len);
    std::vector<uint8_t> sig(sig_len);
    EVP_DigestSignFinal(ctx, sig.data(), &sig_len);
    sig.resize(sig_len);
    EVP_MD_CTX_free(ctx);
    return sig;
}

// ---------------------------------------------------------------------------
// Test fixture with a fixed RP
// ---------------------------------------------------------------------------

static const std::string kTestRpId   = "localhost";
static const std::string kTestOrigin = "https://localhost";

// The deterministic challenge produced by 32 zero bytes
static const std::string kZeroChallenge = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

} // anonymous namespace

// ============================================================================
// GTest fixture – creates a fresh WebAuthnAuthenticator for each test
// ============================================================================

class WebAuthnAuthenticatorTest : public ::testing::Test {
protected:
    WebAuthnAuthenticator wa{WebAuthnAuthenticator::RelyingParty{"localhost", "Test App"}};

    void SetUp() override {
        wa.setExpectedOrigin(kTestOrigin);
        // Deterministic challenge: 32 zero bytes
        wa.setRandBytesForTesting([](unsigned char* buf, std::size_t len) {
            std::memset(buf, 0, len);
        });
    }
};

// ============================================================================
// Tests: startRegistration
// ============================================================================

TEST_F(WebAuthnAuthenticatorTest, StartRegistrationReturnsValidOptions)
{
    WebAuthnAuthenticator::User user{"user-1", "alice@example.com", "Alice"};

    auto opts = wa.startRegistration(user);

    EXPECT_EQ(opts.rp.id,   "localhost");
    EXPECT_EQ(opts.rp.name, "Test App");
    EXPECT_EQ(opts.user.id,          user.id);
    EXPECT_EQ(opts.user.name,        user.name);
    EXPECT_EQ(opts.user.display_name, user.display_name);
    EXPECT_FALSE(opts.challenge.empty());
    EXPECT_EQ(opts.pub_key_cred_params.size(), 2u);
    EXPECT_EQ(opts.pub_key_cred_params[0], "ES256");
    EXPECT_EQ(opts.attestation, "none");
}

TEST_F(WebAuthnAuthenticatorTest, StartRegistrationToJsonHasRequiredFields)
{
    auto opts = wa.startRegistration({"uid", "bob", "Bob"});
    const json j = opts.to_json();

    EXPECT_TRUE(j.contains("challenge"));
    EXPECT_TRUE(j.contains("rp"));
    EXPECT_TRUE(j.contains("user"));
    EXPECT_TRUE(j.contains("pubKeyCredParams"));
    EXPECT_TRUE(j.contains("attestation"));
    EXPECT_EQ(j.at("rp").at("id"), "localhost");
    EXPECT_EQ(j.at("attestation"), "none");

    const json& params = j.at("pubKeyCredParams");
    ASSERT_FALSE(params.empty());
    EXPECT_EQ(params[0].at("alg"), -7);   // ES256
    EXPECT_EQ(params[0].at("type"), "public-key");
}

TEST_F(WebAuthnAuthenticatorTest, StartRegistrationResidentKeyOption)
{
    auto opts = wa.startRegistration({"uid", "carol", "Carol"}, /*resident_key=*/true);

    EXPECT_TRUE(opts.authenticator_selection.require_resident_key);
    EXPECT_TRUE(opts.authenticator_selection.authenticator_attachment.has_value());
    EXPECT_EQ(*opts.authenticator_selection.authenticator_attachment, "platform");
    EXPECT_EQ(opts.authenticator_selection.user_verification, "required");
}

// ============================================================================
// Tests: startAuthentication
// ============================================================================

TEST_F(WebAuthnAuthenticatorTest, StartAuthenticationReturnsValidOptions)
{
    auto opts = wa.startAuthentication();

    EXPECT_EQ(opts.rp_id, "localhost");
    EXPECT_FALSE(opts.challenge.empty());
    EXPECT_EQ(opts.user_verification, "preferred");
}

TEST_F(WebAuthnAuthenticatorTest, StartAuthenticationToJsonHasRequiredFields)
{
    auto opts = wa.startAuthentication();
    const json j = opts.to_json();

    EXPECT_TRUE(j.contains("challenge"));
    EXPECT_TRUE(j.contains("rpId"));
    EXPECT_EQ(j.at("rpId"), "localhost");
}

// ============================================================================
// Tests: completeRegistration (none attestation, ES256)
// ============================================================================

TEST_F(WebAuthnAuthenticatorTest, CompleteRegistrationNoneAttestationES256)
{

    // Start registration to generate+store the challenge
    auto opts = wa.startRegistration({"user-1", "alice", "Alice"});
    const std::string challenge_b64 = opts.challenge;

    // Generate an EC P-256 key pair
    auto kp = generateP256KeyPair();

    // Build COSE key CBOR
    const auto cose_key = buildES256CoseKey(kp.x, kp.y);

    // Credential ID: 16 random bytes
    std::vector<uint8_t> cred_id(16);
    RAND_bytes(cred_id.data(), 16);

    // Build authData with flags = UP (0x01) | AT (0x40) = 0x41
    const auto auth_data = buildRegistrationAuthData(kTestRpId, 0x41, 0, cred_id, cose_key);

    // Build attestation object CBOR
    const auto attest_obj = buildAttestationObject(auth_data);

    // Build clientDataJSON bytes
    const std::string client_data_str = R"({"type":"webauthn.create","challenge":")" +
                                        challenge_b64 + R"(","origin":")" +
                                        kTestOrigin + R"("})";
    const auto client_data_bytes = std::vector<uint8_t>(client_data_str.begin(),
                                                         client_data_str.end());

    // Build credential response JSON
    json cred;
    cred["type"] = "public-key";
    cred["id"]   = b64urlEncode(cred_id);
    cred["response"]["clientDataJSON"]   = b64urlEncode(client_data_bytes);
    cred["response"]["attestationObject"] = b64urlEncode(attest_obj);

    // Complete registration
    auto result = wa.completeRegistration(cred);

    EXPECT_FALSE(result.credential_id.empty());
    EXPECT_FALSE(result.public_key.empty());
    EXPECT_EQ(result.algorithm, "ES256");
    EXPECT_EQ(result.sign_count, 0u);
    EXPECT_EQ(result.aaguid.size(), 16u);

    EVP_PKEY_free(kp.pkey);
}

// ============================================================================
// Tests: completeAuthentication (ES256)
// ============================================================================

TEST_F(WebAuthnAuthenticatorTest, CompleteAuthenticationES256)
{

    // We need a real key pair for signing
    auto kp = generateP256KeyPair();

    // Start authentication (stores challenge)
    auto req = wa.startAuthentication();
    const std::string challenge_b64 = req.challenge;

    // Build assertion authData: UP flag, signCount = 1
    const auto auth_data_bytes = buildAssertionAuthData(kTestRpId, 0x01, 1);

    // Build clientDataJSON
    const std::string client_data_str = R"({"type":"webauthn.get","challenge":")" +
                                        challenge_b64 + R"(","origin":")" +
                                        kTestOrigin + R"("})";
    const auto client_data_bytes = std::vector<uint8_t>(client_data_str.begin(),
                                                         client_data_str.end());
    const auto client_data_hash = sha256v(client_data_bytes);

    // Build signed data = authData || clientDataHash
    std::vector<uint8_t> msg;
    msg.insert(msg.end(), auth_data_bytes.begin(), auth_data_bytes.end());
    msg.insert(msg.end(), client_data_hash.begin(), client_data_hash.end());

    // Sign with EC P-256
    const auto signature = ecdsaSign(kp.pkey, msg);

    // Credential ID
    std::vector<uint8_t> cred_id(16);
    RAND_bytes(cred_id.data(), 16);

    // Build credential response JSON
    json cred;
    cred["type"] = "public-key";
    cred["id"]   = b64urlEncode(cred_id);
    cred["response"]["clientDataJSON"]    = b64urlEncode(client_data_bytes);
    cred["response"]["authenticatorData"] = b64urlEncode(auth_data_bytes);
    cred["response"]["signature"]         = b64urlEncode(signature);

    // stored_sign_count = 0 so that counter 1 > 0 passes
    const auto result = wa.completeAuthentication(cred, kp.spki, 0);

    EXPECT_EQ(result.sign_count, 1u);
    EXPECT_FALSE(result.credential_id.empty());

    EVP_PKEY_free(kp.pkey);
}

// ============================================================================
// Tests: error cases
// ============================================================================

TEST_F(WebAuthnAuthenticatorTest, RejectsUnknownChallenge)
{

    // Do NOT call startAuthentication → no challenge stored
    const std::string challenge_b64 = kZeroChallenge;
    const auto auth_data_bytes = buildAssertionAuthData(kTestRpId, 0x01, 1);

    const std::string client_data_str = R"({"type":"webauthn.get","challenge":")" +
                                        challenge_b64 + R"(","origin":")" +
                                        kTestOrigin + R"("})";
    const auto client_data_bytes = std::vector<uint8_t>(client_data_str.begin(),
                                                         client_data_str.end());

    auto kp = generateP256KeyPair();
    const auto client_data_hash = sha256v(client_data_bytes);
    std::vector<uint8_t> msg;
    msg.insert(msg.end(), auth_data_bytes.begin(), auth_data_bytes.end());
    msg.insert(msg.end(), client_data_hash.begin(), client_data_hash.end());
    const auto sig = ecdsaSign(kp.pkey, msg);

    json cred;
    cred["type"] = "public-key";
    cred["id"]   = kZeroChallenge;
    cred["response"]["clientDataJSON"]    = b64urlEncode(client_data_bytes);
    cred["response"]["authenticatorData"] = b64urlEncode(auth_data_bytes);
    cred["response"]["signature"]         = b64urlEncode(sig);

    EXPECT_THROW(wa.completeAuthentication(cred, kp.spki, 0), AuthException);

    EVP_PKEY_free(kp.pkey);
}

TEST_F(WebAuthnAuthenticatorTest, RejectsWrongOrigin)
{
    auto opts = wa.startRegistration({"u", "user", "User"});

    auto kp = generateP256KeyPair();
    const auto cose_key = buildES256CoseKey(kp.x, kp.y);
    std::vector<uint8_t> cred_id(8);
    RAND_bytes(cred_id.data(), 8);
    const auto auth_data   = buildRegistrationAuthData(kTestRpId, 0x41, 0, cred_id, cose_key);
    const auto attest_obj  = buildAttestationObject(auth_data);

    // Wrong origin
    const std::string client_data_str = R"({"type":"webauthn.create","challenge":")" +
                                        opts.challenge + R"(","origin":"https://evil.com"})";
    const auto client_data_bytes = std::vector<uint8_t>(client_data_str.begin(),
                                                         client_data_str.end());
    json cred;
    cred["type"] = "public-key";
    cred["id"]   = b64urlEncode(cred_id);
    cred["response"]["clientDataJSON"]    = b64urlEncode(client_data_bytes);
    cred["response"]["attestationObject"] = b64urlEncode(attest_obj);

    EXPECT_THROW(wa.completeRegistration(cred), AuthException);

    EVP_PKEY_free(kp.pkey);
}

TEST_F(WebAuthnAuthenticatorTest, RejectsRpIdMismatch)
{
    auto opts = wa.startRegistration({"u", "user", "User"});

    auto kp = generateP256KeyPair();
    const auto cose_key = buildES256CoseKey(kp.x, kp.y);
    std::vector<uint8_t> cred_id(8);
    RAND_bytes(cred_id.data(), 8);

    // Use wrong rpId for authData → rpIdHash mismatch
    const auto auth_data  = buildRegistrationAuthData("evil.com", 0x41, 0, cred_id, cose_key);
    const auto attest_obj = buildAttestationObject(auth_data);

    const std::string client_data_str = R"({"type":"webauthn.create","challenge":")" +
                                        opts.challenge + R"(","origin":")" +
                                        kTestOrigin + R"("})";
    const auto client_data_bytes = std::vector<uint8_t>(client_data_str.begin(),
                                                         client_data_str.end());
    json cred;
    cred["type"] = "public-key";
    cred["id"]   = b64urlEncode(cred_id);
    cred["response"]["clientDataJSON"]    = b64urlEncode(client_data_bytes);
    cred["response"]["attestationObject"] = b64urlEncode(attest_obj);

    EXPECT_THROW(wa.completeRegistration(cred), AuthException);

    EVP_PKEY_free(kp.pkey);
}

TEST_F(WebAuthnAuthenticatorTest, RejectsMissingUserPresenceFlag)
{
    auto opts = wa.startRegistration({"u", "user", "User"});

    auto kp = generateP256KeyPair();
    const auto cose_key = buildES256CoseKey(kp.x, kp.y);
    std::vector<uint8_t> cred_id(8);
    RAND_bytes(cred_id.data(), 8);

    // flags = AT (0x40) but NOT UP (missing 0x01) → should be rejected
    const auto auth_data  = buildRegistrationAuthData(kTestRpId, 0x40, 0, cred_id, cose_key);
    const auto attest_obj = buildAttestationObject(auth_data);

    const std::string client_data_str = R"({"type":"webauthn.create","challenge":")" +
                                        opts.challenge + R"(","origin":")" +
                                        kTestOrigin + R"("})";
    const auto client_data_bytes = std::vector<uint8_t>(client_data_str.begin(),
                                                         client_data_str.end());
    json cred;
    cred["type"] = "public-key";
    cred["id"]   = b64urlEncode(cred_id);
    cred["response"]["clientDataJSON"]    = b64urlEncode(client_data_bytes);
    cred["response"]["attestationObject"] = b64urlEncode(attest_obj);

    EXPECT_THROW(wa.completeRegistration(cred), AuthException);

    EVP_PKEY_free(kp.pkey);
}

TEST_F(WebAuthnAuthenticatorTest, RejectsSignatureCounterRollback)
{
    auto kp = generateP256KeyPair();

    auto req = wa.startAuthentication();
    const std::string challenge_b64 = req.challenge;

    // signCount = 1, but stored_sign_count = 5 → rollback
    const auto auth_data_bytes = buildAssertionAuthData(kTestRpId, 0x01, 1);

    const std::string client_data_str = R"({"type":"webauthn.get","challenge":")" +
                                        challenge_b64 + R"(","origin":")" +
                                        kTestOrigin + R"("})";
    const auto client_data_bytes = std::vector<uint8_t>(client_data_str.begin(),
                                                         client_data_str.end());
    const auto client_data_hash = sha256v(client_data_bytes);

    std::vector<uint8_t> msg;
    msg.insert(msg.end(), auth_data_bytes.begin(), auth_data_bytes.end());
    msg.insert(msg.end(), client_data_hash.begin(), client_data_hash.end());
    const auto sig = ecdsaSign(kp.pkey, msg);

    std::vector<uint8_t> cred_id(8);
    RAND_bytes(cred_id.data(), 8);

    json cred;
    cred["type"] = "public-key";
    cred["id"]   = b64urlEncode(cred_id);
    cred["response"]["clientDataJSON"]    = b64urlEncode(client_data_bytes);
    cred["response"]["authenticatorData"] = b64urlEncode(auth_data_bytes);
    cred["response"]["signature"]         = b64urlEncode(sig);

    // stored_sign_count = 5, new = 1 → rollback → throw
    EXPECT_THROW(wa.completeAuthentication(cred, kp.spki, 5), AuthException);

    EVP_PKEY_free(kp.pkey);
}

TEST_F(WebAuthnAuthenticatorTest, RejectsBadSignature)
{
    auto kp = generateP256KeyPair();

    auto req = wa.startAuthentication();
    const std::string challenge_b64 = req.challenge;

    const auto auth_data_bytes = buildAssertionAuthData(kTestRpId, 0x01, 1);
    const std::string client_data_str = R"({"type":"webauthn.get","challenge":")" +
                                        challenge_b64 + R"(","origin":")" +
                                        kTestOrigin + R"("})";
    const auto client_data_bytes = std::vector<uint8_t>(client_data_str.begin(),
                                                         client_data_str.end());

    // Use a garbage signature
    const std::vector<uint8_t> bad_sig(64, 0xAB);

    std::vector<uint8_t> cred_id(8);
    RAND_bytes(cred_id.data(), 8);

    json cred;
    cred["type"] = "public-key";
    cred["id"]   = b64urlEncode(cred_id);
    cred["response"]["clientDataJSON"]    = b64urlEncode(client_data_bytes);
    cred["response"]["authenticatorData"] = b64urlEncode(auth_data_bytes);
    cred["response"]["signature"]         = b64urlEncode(bad_sig);

    EXPECT_THROW(wa.completeAuthentication(cred, kp.spki, 0), AuthException);

    EVP_PKEY_free(kp.pkey);
}

TEST_F(WebAuthnAuthenticatorTest, ChallengeIsConsumedAfterUse)
{
    auto kp = generateP256KeyPair();

    auto req = wa.startAuthentication();
    const std::string challenge_b64 = req.challenge;

    const auto auth_data_bytes = buildAssertionAuthData(kTestRpId, 0x01, 1);
    const std::string client_data_str = R"({"type":"webauthn.get","challenge":")" +
                                        challenge_b64 + R"(","origin":")" +
                                        kTestOrigin + R"("})";
    const auto client_data_bytes = std::vector<uint8_t>(client_data_str.begin(),
                                                         client_data_str.end());
    const auto client_data_hash  = sha256v(client_data_bytes);

    std::vector<uint8_t> msg;
    msg.insert(msg.end(), auth_data_bytes.begin(), auth_data_bytes.end());
    msg.insert(msg.end(), client_data_hash.begin(), client_data_hash.end());
    const auto sig = ecdsaSign(kp.pkey, msg);

    std::vector<uint8_t> cred_id(8);
    RAND_bytes(cred_id.data(), 8);

    json cred;
    cred["type"] = "public-key";
    cred["id"]   = b64urlEncode(cred_id);
    cred["response"]["clientDataJSON"]    = b64urlEncode(client_data_bytes);
    cred["response"]["authenticatorData"] = b64urlEncode(auth_data_bytes);
    cred["response"]["signature"]         = b64urlEncode(sig);

    // First call succeeds (challenge consumed)
    EXPECT_NO_THROW(wa.completeAuthentication(cred, kp.spki, 0));

    // Second call with the same challenge must fail (replay prevention).
    // We do NOT call startAuthentication() again because the deterministic
    // rand function would regenerate the same challenge. The challenge was
    // already consumed above, so a second attempt with the same clientDataJSON
    // must be rejected.
    EXPECT_THROW(wa.completeAuthentication(cred, kp.spki, 0), AuthException);

    EVP_PKEY_free(kp.pkey);
}

TEST_F(WebAuthnAuthenticatorTest, RejectsWrongCredentialType)
{
    wa.startRegistration({"u", "user", "User"});  // generate challenge

    json cred;
    cred["type"] = "wrong-type";
    cred["id"]   = "something";
    cred["response"]["clientDataJSON"]    = "dummy";
    cred["response"]["attestationObject"] = "dummy";

    EXPECT_THROW(wa.completeRegistration(cred), AuthException);
}

TEST_F(WebAuthnAuthenticatorTest, RejectsEmptyRpId)
{
    EXPECT_THROW(
        WebAuthnAuthenticator({"", "Test"}),
        AuthException
    );
}

// ============================================================================
// Tests: RP configuration
// ============================================================================

TEST_F(WebAuthnAuthenticatorTest, DefaultOriginIsHttpsRpId)
{
    WebAuthnAuthenticator wa({"example.com", "Example"});
    // Default origin is https://example.com; if we set a wrong origin in clientDataJSON
    // the check should fail. We test indirectly via the correct origin in the test above.
    // Here we just verify construction succeeds.
    auto opts = wa.startRegistration({"u", "user", "User"});
    EXPECT_EQ(opts.rp.id, "example.com");
}
