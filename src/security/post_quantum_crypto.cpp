/**
 * @file post_quantum_crypto.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=24; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=18, Debt=0, C=0, H=5, M=22, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * Post-Quantum Cryptography — Software Simulation Backend
 * =========================================================
 * This translation unit implements the post-quantum API using OpenSSL
 * primitives as a drop-in simulation:
 *
 *   CRYSTALS-Kyber  → X25519 ECDH + HKDF-SHA256  (KYBER_SIM)
 *   CRYSTALS-Dilithium → Ed25519                  (DILITHIUM_SIM)
 *
 * The API is identical to what a liboqs-backed implementation would
 * expose. Once `liboqs` is added as a vcpkg dependency and the build
 * system includes it, only this file needs to be changed; all callers
 * remain source-compatible.
 *
 * Key size mapping (simulation):
 *   KYBER_512  public=32 secret=32 ciphertext=32
 *   KYBER_768  public=32 secret=32 ciphertext=32
 *   KYBER_1024 public=32 secret=32 ciphertext=32
 *   (All three map to X25519 in the simulation; real liboqs sizes differ.)
 *
 *   DILITHIUM_2/3/5: all map to Ed25519 64-byte signatures.
 */

#include "security/post_quantum_crypto.h"
#include "utils/logger.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/kdf.h>
#include <openssl/core_names.h>
#include <openssl/params.h>

// ── liboqs real post-quantum backend (Wave-2: THEMIS_HAS_OQS guard) ──────────
// When -DTHEMIS_HAS_OQS=ON is set in CMake and the liboqs package is found,
// OQS_KEM_* / OQS_SIG_* APIs replace the X25519/Ed25519 simulation below.
// The simulation remains compiled as PERMANENT FALLBACK when OQS is absent.
#ifdef THEMIS_HAS_OQS
#  include <oqs/oqs.h>
#endif

#include <stdexcept>
#include <cstring>
#include <array>
#include <sstream>
#include <cassert>

namespace themis {
namespace security {

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// Collect and return the latest OpenSSL error string.
static std::string ossl_error() {
    unsigned long code = ERR_peek_last_error();
    if (code == 0) return "unknown OpenSSL error";
    char buf[256];
    ERR_error_string_n(code, buf, sizeof(buf));
    ERR_clear_error();
    return std::string(buf);
}

// RAII wrappers for OpenSSL resource types
using EVP_CIPHER_CTX_ptr = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;
using EVP_PKEY_ptr       = std::unique_ptr<EVP_PKEY,       decltype(&EVP_PKEY_free)>;
using EVP_PKEY_CTX_ptr   = std::unique_ptr<EVP_PKEY_CTX,   decltype(&EVP_PKEY_CTX_free)>;
using EVP_MD_CTX_ptr     = std::unique_ptr<EVP_MD_CTX,     decltype(&EVP_MD_CTX_free)>;
using EVP_KDF_CTX_ptr    = std::unique_ptr<EVP_KDF_CTX,    decltype(&EVP_KDF_CTX_free)>;
using EVP_KDF_ptr        = std::unique_ptr<EVP_KDF,        decltype(&EVP_KDF_free)>;

/**
 * @brief HKDF-SHA256 extract-and-expand.
 *
 * @param ikm   Input key material
 * @param salt  Optional salt (may be empty → zeroed salt)
 * @param info  Context / label bytes
 * @param out_len Desired output length (bytes)
 * @return Derived key material of length out_len
 */
static std::vector<uint8_t> hkdf_sha256(
    const std::vector<uint8_t>& ikm,
    const std::vector<uint8_t>& salt,
    const std::vector<uint8_t>& info,
    size_t out_len)
{
    EVP_KDF_ptr kdf(EVP_KDF_fetch(nullptr, "HKDF", nullptr), &EVP_KDF_free);
    if (!kdf.get()) throw std::runtime_error("HKDF: fetch failed: " + ossl_error());

    EVP_KDF_CTX_ptr ctx(EVP_KDF_CTX_new(kdf.get()), &EVP_KDF_CTX_free);
    if (!ctx.get()) throw std::runtime_error("HKDF: ctx alloc failed: " + ossl_error());

    std::vector<uint8_t> out(out_len);

    // Build params
    std::string digest_name = "SHA256";
    OSSL_PARAM params[6];
    int idx = 0;
    params[idx++] = OSSL_PARAM_construct_utf8_string(
        OSSL_KDF_PARAM_DIGEST,
        const_cast<char*>(digest_name.c_str()), 0);
    params[idx++] = OSSL_PARAM_construct_octet_string(
        OSSL_KDF_PARAM_KEY,
        const_cast<uint8_t*>(ikm.data()), ikm.size());
    if (!salt.empty()) {
        params[idx++] = OSSL_PARAM_construct_octet_string(
            OSSL_KDF_PARAM_SALT,
            const_cast<uint8_t*>(salt.data()), salt.size());
    }
    if (!info.empty()) {
        params[idx++] = OSSL_PARAM_construct_octet_string(
            OSSL_KDF_PARAM_INFO,
            const_cast<uint8_t*>(info.data()), info.size());
    }
    params[idx] = OSSL_PARAM_END;

    int rc = EVP_KDF_derive(ctx.get(), out.data(), out_len, params);
    if (rc != 1) throw std::runtime_error("HKDF: derive failed: " + ossl_error());
    return out;
    // RAII wrappers (ctx, kdf) automatically clean up on scope exit
}

/**
 * @brief Generate 12-byte random IV for AES-256-GCM.
 */
static std::array<uint8_t, 12> random_iv() {
    std::array<uint8_t, 12> iv{};
    if (RAND_bytes(iv.data(), 12) != 1)
        throw std::runtime_error("random_iv: RAND_bytes failed: " + ossl_error());
    return iv;
}

/**
 * @brief AES-256-GCM authenticated encryption.
 *
 * @param key        32-byte AES key
 * @param iv         12-byte initialization vector
 * @param plaintext  Data to encrypt
 * @param[out] tag   16-byte authentication tag (appended to output)
 * @return Ciphertext bytes
 */
static std::vector<uint8_t> aes256gcm_encrypt(
    const std::vector<uint8_t>& key,
    const std::array<uint8_t, 12>& iv,
    const std::vector<uint8_t>& plaintext,
    std::array<uint8_t, 16>& tag)
{
    assert(key.size() == 32);

    EVP_CIPHER_CTX_ptr ctx(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    if (!ctx) throw std::runtime_error("aes256gcm_encrypt: ctx alloc: " + ossl_error());

    std::vector<uint8_t> ct(plaintext.size() + 32);
    int len = 0, ct_len = 0;

    auto fail = [&]([[maybe_unused]] const char* where) {
        throw std::runtime_error(std::string("aes256gcm_encrypt: ") + where + ": " + ossl_error());
    };

    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) fail("init");
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1) fail("ivlen");
    if (EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), iv.data()) != 1) fail("key/iv");
    if (EVP_EncryptUpdate(ctx.get(), ct.data(), &len,
                          plaintext.data(), static_cast<int>(plaintext.size())) != 1) fail("update");
    ct_len = len;
    if (EVP_EncryptFinal_ex(ctx.get(), ct.data() + len, &len) != 1) fail("final");
    ct_len += len;
    ct.resize(ct_len);
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, 16, tag.data()) != 1) fail("get_tag");
    return ct;
}

/**
 * @brief AES-256-GCM authenticated decryption.
 *
 * @param key        32-byte AES key
 * @param iv         12-byte initialization vector
 * @param ciphertext Ciphertext bytes
 * @param tag        16-byte GCM authentication tag
 * @return Plaintext bytes
 * @throws std::runtime_error if authentication fails
 */
static std::vector<uint8_t> aes256gcm_decrypt(
    const std::vector<uint8_t>& key,
    const std::array<uint8_t, 12>& iv,
    const std::vector<uint8_t>& ciphertext,
    const std::array<uint8_t, 16>& tag)
{
    assert(key.size() == 32);

    EVP_CIPHER_CTX_ptr ctx(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    if (!ctx) throw std::runtime_error("aes256gcm_decrypt: ctx alloc: " + ossl_error());

    std::vector<uint8_t> pt(ciphertext.size() + 32);
    int len = 0, pt_len = 0;

    auto fail = [&]([[maybe_unused]] const char* where) {
        throw std::runtime_error(std::string("aes256gcm_decrypt: ") + where + ": " + ossl_error());
    };

    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) fail("init");
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1) fail("ivlen");
    if (EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), iv.data()) != 1) fail("key/iv");
    if (EVP_DecryptUpdate(ctx.get(), pt.data(), &len,
                          ciphertext.data(), static_cast<int>(ciphertext.size())) != 1) fail("update");
    pt_len = len;
    // Set expected tag
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, 16,
                             const_cast<uint8_t*>(tag.data())) != 1) fail("set_tag");
    int rc = EVP_DecryptFinal_ex(ctx.get(), pt.data() + len, &len);
    if (rc <= 0)
        throw std::runtime_error("aes256gcm_decrypt: authentication failed (GCM tag mismatch)");
    pt_len += len;
    pt.resize(pt_len);
    return pt;
}

// ─── X25519 helpers (KyberKEM simulation) ────────────────────────────────

#ifdef THEMIS_HAS_OQS
// ── liboqs KyberKEM helpers ────────────────────────────────────────────────
namespace {

/// @brief Select the liboqs algorithm name for a given Kyber security level.
/// @param level  SecurityLevel enum value (KYBER_512, KYBER_768, KYBER_1024).
/// @return Null-terminated algorithm name string for OQS_KEM_new().
static const char* kyberAlgName(KyberKEM::SecurityLevel level) noexcept {
    switch (level) {
        case KyberKEM::SecurityLevel::KYBER_512:  return OQS_KEM_alg_kyber_512;
        case KyberKEM::SecurityLevel::KYBER_768:  return OQS_KEM_alg_kyber_768;
        case KyberKEM::SecurityLevel::KYBER_1024: return OQS_KEM_alg_kyber_1024;
        default:                                   return OQS_KEM_alg_kyber_1024;
    }
}

/// @brief RAII wrapper around OQS_KEM.
struct OqsKemRAII {
    OQS_KEM* kem;
    explicit OqsKemRAII(const char* alg) : kem(OQS_KEM_new(alg)) {}
    ~OqsKemRAII() { if (kem) OQS_KEM_free(kem); }
    OqsKemRAII(const OqsKemRAII&) = delete;
    OqsKemRAII& operator=(const OqsKemRAII&) = delete;
};

} // anonymous namespace
#endif // THEMIS_HAS_OQS

/**
 * @brief Generate a fresh X25519 key pair.
 *
 * @return {public_key_bytes (32), private_key_bytes (32)}
 */
static std::pair<std::vector<uint8_t>, std::vector<uint8_t>> x25519_keygen() {
    EVP_PKEY_CTX_ptr kctx(EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr), &EVP_PKEY_CTX_free);
    if (!kctx) throw std::runtime_error("x25519_keygen: ctx: " + ossl_error());
    if (EVP_PKEY_keygen_init(kctx.get()) <= 0)
        throw std::runtime_error("x25519_keygen: keygen_init: " + ossl_error());
    EVP_PKEY* raw_pkey = nullptr;
    if (EVP_PKEY_keygen(kctx.get(), &raw_pkey) <= 0)
        throw std::runtime_error("x25519_keygen: keygen: " + ossl_error());
    EVP_PKEY_ptr pkey(raw_pkey, &EVP_PKEY_free);

    std::vector<uint8_t> pub(32), priv(32);
    size_t pub_len = 32, priv_len = 32;
    if (EVP_PKEY_get_raw_public_key(pkey.get(), pub.data(), &pub_len) <= 0 ||
        EVP_PKEY_get_raw_private_key(pkey.get(), priv.data(), &priv_len) <= 0)
        throw std::runtime_error("x25519_keygen: export: " + ossl_error());
    return {pub, priv};
}

/**
 * @brief Perform X25519 ECDH to derive a shared secret.
 *
 * @param our_private_key_bytes  32-byte private key
 * @param peer_public_key_bytes  32-byte peer public key
 * @return 32-byte raw shared secret (pass through HKDF before use as key)
 */
static std::vector<uint8_t> x25519_ecdh(
    const std::vector<uint8_t>& our_private_key_bytes,
    const std::vector<uint8_t>& peer_public_key_bytes)
{
    EVP_PKEY_ptr priv_key(
        EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, nullptr,
            our_private_key_bytes.data(), our_private_key_bytes.size()),
        &EVP_PKEY_free);
    if (!priv_key) throw std::runtime_error("x25519_ecdh: priv key: " + ossl_error());

    EVP_PKEY_ptr pub_key(
        EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr,
            peer_public_key_bytes.data(), peer_public_key_bytes.size()),
        &EVP_PKEY_free);
    if (!pub_key) throw std::runtime_error("x25519_ecdh: pub key: " + ossl_error());

    EVP_PKEY_CTX_ptr ctx(EVP_PKEY_CTX_new(priv_key.get(), nullptr), &EVP_PKEY_CTX_free);
    if (!ctx) throw std::runtime_error("x25519_ecdh: ctx: " + ossl_error());
    if (EVP_PKEY_derive_init(ctx.get()) <= 0)
        throw std::runtime_error("x25519_ecdh: derive_init: " + ossl_error());
    if (EVP_PKEY_derive_set_peer(ctx.get(), pub_key.get()) <= 0)
        throw std::runtime_error("x25519_ecdh: set_peer: " + ossl_error());
    size_t secret_len = 0;
    EVP_PKEY_derive(ctx.get(), nullptr, &secret_len);
    std::vector<uint8_t> secret(secret_len);
    if (EVP_PKEY_derive(ctx.get(), secret.data(), &secret_len) <= 0)
        throw std::runtime_error("x25519_ecdh: derive: " + ossl_error());
    return secret;
}

// ─── Ed25519 helpers (DilithiumSigner simulation) ────────────────────────

/**
 * @brief Generate an Ed25519 key pair.
 */
static std::pair<std::vector<uint8_t>, std::vector<uint8_t>> ed25519_keygen() {
    EVP_PKEY_CTX_ptr kctx(EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr), &EVP_PKEY_CTX_free);
    if (!kctx) throw std::runtime_error("ed25519_keygen: ctx: " + ossl_error());
    if (EVP_PKEY_keygen_init(kctx.get()) <= 0)
        throw std::runtime_error("ed25519_keygen: init: " + ossl_error());
    EVP_PKEY* raw_pkey = nullptr;
    if (EVP_PKEY_keygen(kctx.get(), &raw_pkey) <= 0)
        throw std::runtime_error("ed25519_keygen: keygen: " + ossl_error());
    EVP_PKEY_ptr pkey(raw_pkey, &EVP_PKEY_free);

    std::vector<uint8_t> pub(32), priv(32);
    size_t pub_len = 32, priv_len = 32;
    if (EVP_PKEY_get_raw_public_key(pkey.get(), pub.data(), &pub_len) <= 0 ||
        EVP_PKEY_get_raw_private_key(pkey.get(), priv.data(), &priv_len) <= 0)
        throw std::runtime_error("ed25519_keygen: export: " + ossl_error());
    return {pub, priv};
}

/**
 * @brief Sign a message with Ed25519.
 */
static std::vector<uint8_t> ed25519_sign(
    const std::vector<uint8_t>& message,
    const std::vector<uint8_t>& secret_key)
{
    EVP_PKEY_ptr pkey(
        EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, secret_key.data(), secret_key.size()),
        &EVP_PKEY_free);
    if (!pkey) throw std::runtime_error("ed25519_sign: load key: " + ossl_error());

    EVP_MD_CTX_ptr mctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    if (!mctx) throw std::runtime_error("ed25519_sign: md_ctx: " + ossl_error());

    if (EVP_DigestSignInit(mctx.get(), nullptr, nullptr, nullptr, pkey.get()) <= 0)
        throw std::runtime_error("ed25519_sign: DigestSignInit: " + ossl_error());
    size_t sig_len = 0;
    EVP_DigestSign(mctx.get(), nullptr, &sig_len, message.data(), message.size());
    std::vector<uint8_t> sig(sig_len);
    if (EVP_DigestSign(mctx.get(), sig.data(), &sig_len, message.data(), message.size()) <= 0)
        throw std::runtime_error("ed25519_sign: DigestSign: " + ossl_error());
    sig.resize(sig_len);
    return sig;
}

/**
 * @brief Verify an Ed25519 signature.
 */
static bool ed25519_verify(
    const std::vector<uint8_t>& message,
    const std::vector<uint8_t>& signature,
    const std::vector<uint8_t>& public_key)
{
    EVP_PKEY_ptr pkey(
        EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr, public_key.data(), public_key.size()),
        &EVP_PKEY_free);
    if (!pkey) return false;  // invalid key

    EVP_MD_CTX_ptr mctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    if (!mctx) return false;

    if (EVP_DigestVerifyInit(mctx.get(), nullptr, nullptr, nullptr, pkey.get()) <= 0)
        return false;
    int rc = EVP_DigestVerify(mctx.get(),
                               signature.data(), signature.size(),
                               message.data(), message.size());
    return rc == 1;
}

// ─── Little-endian integer serialisation ─────────────────────────────────

static void write_u32_le(std::vector<uint8_t>& buf, uint32_t v) {
    buf.push_back(static_cast<uint8_t>(v & 0xff));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xff));
    buf.push_back(static_cast<uint8_t>((v >> 16) & 0xff));
    buf.push_back(static_cast<uint8_t>((v >> 24) & 0xff));
}

static uint32_t read_u32_le(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) << 8)
         | (static_cast<uint32_t>(p[2]) << 16)
         | (static_cast<uint32_t>(p[3]) << 24);
}

} // anonymous namespace

// ============================================================================
// KyberKEM
// ============================================================================

struct KyberKEM::Impl {
    // Nothing extra needed for the simulation; reserved for liboqs state.
};

KyberKEM::KyberKEM(SecurityLevel level)
    : level_(level), impl_(std::make_unique<Impl>())
{
#ifdef THEMIS_HAS_OQS
    THEMIS_INFO("KyberKEM: initialized (liboqs backend, level={})", static_cast<int>(level_));
#else
    THEMIS_INFO("KyberKEM: initialized (KYBER_SIM, level={})",
                static_cast<int>(level_));
#endif
}

KyberKEM::~KyberKEM() = default;
KyberKEM::KyberKEM(KyberKEM&&) noexcept = default;
KyberKEM& KyberKEM::operator=(KyberKEM&&) noexcept = default;

size_t KyberKEM::publicKeySize() const noexcept {
#ifdef THEMIS_HAS_OQS
    OqsKemRAII k(kyberAlgName(level_));
    return k.kem ? k.kem->length_public_key : 0;
#else
    return 32; // PERMANENT FALLBACK: X25519 simulation size
#endif
}
size_t KyberKEM::secretKeySize() const noexcept {
#ifdef THEMIS_HAS_OQS
    OqsKemRAII k(kyberAlgName(level_));
    return k.kem ? k.kem->length_secret_key : 0;
#else
    return 32; // PERMANENT FALLBACK: X25519 simulation size
#endif
}
size_t KyberKEM::ciphertextSize() const noexcept {
#ifdef THEMIS_HAS_OQS
    OqsKemRAII k(kyberAlgName(level_));
    return k.kem ? k.kem->length_ciphertext : 0;
#else
    return 32; // PERMANENT FALLBACK: X25519 simulation size (ephemeral public key)
#endif
}

/**
 * @brief Generate a Kyber key pair.
 *
 * @return KeyPair with public_key and secret_key populated.
 * @throws std::runtime_error on OQS/OpenSSL failure.
 */
KyberKEM::KeyPair KyberKEM::generateKeyPair() {
#ifdef THEMIS_HAS_OQS
    OqsKemRAII k(kyberAlgName(level_));
    if (!k.kem) throw std::runtime_error("KyberKEM::generateKeyPair: OQS_KEM_new failed");
    KeyPair kp;
    kp.public_key.resize(k.kem->length_public_key);
    kp.secret_key.resize(k.kem->length_secret_key);
    if (OQS_KEM_keypair(k.kem, kp.public_key.data(), kp.secret_key.data()) != OQS_SUCCESS)
        throw std::runtime_error("KyberKEM::generateKeyPair: OQS_KEM_keypair failed");
    THEMIS_DEBUG("KyberKEM::generateKeyPair (liboqs): pub={}B sec={}B",
                 kp.public_key.size(), kp.secret_key.size());
    return kp;
#else
    // PERMANENT FALLBACK: X25519 simulation (no liboqs)
    auto [pub, priv] = x25519_keygen();
    THEMIS_DEBUG("KyberKEM::generateKeyPair: pub_len={}, priv_len={}",
                 pub.size(), priv.size());
    return {std::move(pub), std::move(priv)};
#endif
}

/**
 * @brief Encapsulate a shared secret for a Kyber public key.
 *
 * @param public_key  Recipient's Kyber public key bytes.
 * @return EncapsulationResult with ciphertext and shared_secret.
 * @throws std::invalid_argument on wrong key size.
 * @throws std::runtime_error on OQS/OpenSSL failure.
 */
KyberKEM::EncapsulationResult
KyberKEM::encapsulate(const std::vector<uint8_t>& public_key) {
    if (public_key.size() != publicKeySize()) {
        throw std::invalid_argument(
            "KyberKEM::encapsulate: unexpected public key size " +
            std::to_string(public_key.size()) + " (expected " +
            std::to_string(publicKeySize()) + ")");
    }

#ifdef THEMIS_HAS_OQS
    OqsKemRAII k(kyberAlgName(level_));
    if (!k.kem) throw std::runtime_error("KyberKEM::encapsulate: OQS_KEM_new failed");
    EncapsulationResult res;
    res.ciphertext.resize(k.kem->length_ciphertext);
    res.shared_secret.resize(k.kem->length_shared_secret);
    if (OQS_KEM_encaps(k.kem, res.ciphertext.data(), res.shared_secret.data(), public_key.data())
            != OQS_SUCCESS)
        throw std::runtime_error("KyberKEM::encapsulate: OQS_KEM_encaps failed");
    THEMIS_DEBUG("KyberKEM::encapsulate (liboqs): ct={}B ss={}B",
                 res.ciphertext.size(), res.shared_secret.size());
    return res;
#else
    // PERMANENT FALLBACK: X25519 simulation
    // Generate ephemeral sender key pair
    auto [eph_pub, eph_priv] = x25519_keygen();

    // ECDH between ephemeral private and recipient public
    auto dh_secret = x25519_ecdh(eph_priv, public_key);

    // HKDF to derive the 32-byte shared secret
    const std::string info_str = "KYBER_SIM_SHARED_SECRET";
    std::vector<uint8_t> info(info_str.begin(), info_str.end());
    auto shared_secret = hkdf_sha256(dh_secret, {}, info, 32);

    THEMIS_DEBUG("KyberKEM::encapsulate: eph_pub_len={}, shared_secret_len={}",
                 eph_pub.size(), shared_secret.size());

    // The "ciphertext" in the simulation is the ephemeral public key
    return {std::move(eph_pub), std::move(shared_secret)};
#endif
}

/**
 * @brief Decapsulate a Kyber ciphertext to recover the shared secret.
 *
 * @param ciphertext  Ciphertext produced by encapsulate().
 * @param secret_key  Recipient's Kyber secret key.
 * @return Shared secret bytes.
 * @throws std::invalid_argument on wrong input sizes.
 * @throws std::runtime_error on OQS/OpenSSL failure.
 */
std::vector<uint8_t>
KyberKEM::decapsulate(const std::vector<uint8_t>& ciphertext,
                       const std::vector<uint8_t>& secret_key) {
    if (ciphertext.size() != ciphertextSize()) {
        throw std::invalid_argument(
            "KyberKEM::decapsulate: unexpected ciphertext size " +
            std::to_string(ciphertext.size()));
    }
    if (secret_key.size() != secretKeySize()) {
        throw std::invalid_argument(
            "KyberKEM::decapsulate: unexpected secret key size " +
            std::to_string(secret_key.size()));
    }

#ifdef THEMIS_HAS_OQS
    OqsKemRAII k(kyberAlgName(level_));
    if (!k.kem) throw std::runtime_error("KyberKEM::decapsulate: OQS_KEM_new failed");
    std::vector<uint8_t> shared_secret(k.kem->length_shared_secret);
    if (OQS_KEM_decaps(k.kem, shared_secret.data(), ciphertext.data(), secret_key.data())
            != OQS_SUCCESS)
        throw std::runtime_error("KyberKEM::decapsulate: OQS_KEM_decaps failed");
    THEMIS_DEBUG("KyberKEM::decapsulate (liboqs): ss={}B", shared_secret.size());
    return shared_secret;
#else
    // PERMANENT FALLBACK: X25519 simulation
    auto dh_secret = x25519_ecdh(secret_key, ciphertext);

    const std::string info_str = "KYBER_SIM_SHARED_SECRET";
    std::vector<uint8_t> info(info_str.begin(), info_str.end());
    auto shared_secret = hkdf_sha256(dh_secret, {}, info, 32);

    THEMIS_DEBUG("KyberKEM::decapsulate: shared_secret_len={}",
                 shared_secret.size());
    return shared_secret;
#endif
}

// ============================================================================
// DilithiumSigner
// ============================================================================

#ifdef THEMIS_HAS_OQS
namespace {

/// @brief Select the liboqs algorithm name for a given Dilithium security level.
/// @param level  SecurityLevel enum value (DILITHIUM_2/3/5).
/// @return Null-terminated algorithm name string for OQS_SIG_new().
static const char* dilithiumAlgName(DilithiumSigner::SecurityLevel level) noexcept {
    switch (level) {
        case DilithiumSigner::SecurityLevel::DILITHIUM_2: return OQS_SIG_alg_dilithium_2;
        case DilithiumSigner::SecurityLevel::DILITHIUM_3: return OQS_SIG_alg_dilithium_3;
        case DilithiumSigner::SecurityLevel::DILITHIUM_5: return OQS_SIG_alg_dilithium_5;
        default:                                           return OQS_SIG_alg_dilithium_3;
    }
}

/// @brief RAII wrapper around OQS_SIG.
struct OqsSigRAII {
    OQS_SIG* sig;
    explicit OqsSigRAII(const char* alg) : sig(OQS_SIG_new(alg)) {}
    ~OqsSigRAII() { if (sig) OQS_SIG_free(sig); }
    OqsSigRAII(const OqsSigRAII&) = delete;
    OqsSigRAII& operator=(const OqsSigRAII&) = delete;
};

} // anonymous namespace
#endif // THEMIS_HAS_OQS

struct DilithiumSigner::Impl {
    // Reserved for liboqs state.
};

DilithiumSigner::DilithiumSigner(SecurityLevel level)
    : level_(level), impl_(std::make_unique<Impl>())
{
#ifdef THEMIS_HAS_OQS
    THEMIS_INFO("DilithiumSigner: initialized (liboqs backend, level={})", static_cast<int>(level_));
#else
    THEMIS_INFO("DilithiumSigner: initialized (DILITHIUM_SIM, level={})",
                static_cast<int>(level_));
#endif
}

DilithiumSigner::~DilithiumSigner() = default;
DilithiumSigner::DilithiumSigner(DilithiumSigner&&) noexcept = default;
DilithiumSigner& DilithiumSigner::operator=(DilithiumSigner&&) noexcept = default;

/**
 * @brief Generate a Dilithium key pair.
 *
 * @return KeyPair with public_key and secret_key populated.
 * @throws std::runtime_error on OQS/OpenSSL failure.
 */
DilithiumSigner::KeyPair DilithiumSigner::generateKeyPair() {
#ifdef THEMIS_HAS_OQS
    OqsSigRAII s(dilithiumAlgName(level_));
    if (!s.sig) throw std::runtime_error("DilithiumSigner::generateKeyPair: OQS_SIG_new failed");
    KeyPair kp;
    kp.public_key.resize(s.sig->length_public_key);
    kp.secret_key.resize(s.sig->length_secret_key);
    if (OQS_SIG_keypair(s.sig, kp.public_key.data(), kp.secret_key.data()) != OQS_SUCCESS)
        throw std::runtime_error("DilithiumSigner::generateKeyPair: OQS_SIG_keypair failed");
    THEMIS_DEBUG("DilithiumSigner::generateKeyPair (liboqs): pub={}B sec={}B",
                 kp.public_key.size(), kp.secret_key.size());
    return kp;
#else
    // PERMANENT FALLBACK: Ed25519 simulation
    auto [pub, priv] = ed25519_keygen();
    THEMIS_DEBUG("DilithiumSigner::generateKeyPair: pub_len={}, priv_len={}",
                 pub.size(), priv.size());
    return {std::move(pub), std::move(priv)};
#endif
}

/**
 * @brief Sign a message with a Dilithium secret key.
 *
 * @param message     Message bytes to sign.
 * @param secret_key  Dilithium secret key.
 * @return Signature bytes.
 * @throws std::invalid_argument on wrong key size (fallback path).
 * @throws std::runtime_error on OQS/OpenSSL failure.
 */
std::vector<uint8_t>
DilithiumSigner::sign(const std::vector<uint8_t>& message,
                       const std::vector<uint8_t>& secret_key) {
#ifdef THEMIS_HAS_OQS
    OqsSigRAII s(dilithiumAlgName(level_));
    if (!s.sig) throw std::runtime_error("DilithiumSigner::sign: OQS_SIG_new failed");
    if (secret_key.size() != s.sig->length_secret_key)
        throw std::invalid_argument(
            "DilithiumSigner::sign: unexpected secret key size " +
            std::to_string(secret_key.size()));
    std::vector<uint8_t> sig_buf(s.sig->length_signature);
    size_t sig_len = 0;
    if (OQS_SIG_sign(s.sig, sig_buf.data(), &sig_len,
                     message.data(), message.size(), secret_key.data()) != OQS_SUCCESS)
        throw std::runtime_error("DilithiumSigner::sign: OQS_SIG_sign failed");
    sig_buf.resize(sig_len);
    THEMIS_DEBUG("DilithiumSigner::sign (liboqs): msg={}B sig={}B", message.size(), sig_len);
    return sig_buf;
#else
    // PERMANENT FALLBACK: Ed25519 simulation
    if (secret_key.size() != 32) {
        throw std::invalid_argument(
            "DilithiumSigner::sign: unexpected secret key size " +
            std::to_string(secret_key.size()));
    }
    auto sig = ed25519_sign(message, secret_key);
    THEMIS_DEBUG("DilithiumSigner::sign: msg_len={}, sig_len={}",
                 message.size(), sig.size());
    return sig;
#endif
}

/**
 * @brief Verify a Dilithium signature.
 *
 * @param message     Original message bytes.
 * @param signature   Signature to verify.
 * @param public_key  Signer's Dilithium public key.
 * @return true if valid, false otherwise.
 */
bool DilithiumSigner::verify(const std::vector<uint8_t>& message,
                               const std::vector<uint8_t>& signature,
                               const std::vector<uint8_t>& public_key) {
#ifdef THEMIS_HAS_OQS
    OqsSigRAII s(dilithiumAlgName(level_));
    if (!s.sig || public_key.size() != s.sig->length_public_key) return false;
    bool ok = (OQS_SIG_verify(s.sig, message.data(), message.size(),
                               signature.data(), signature.size(),
                               public_key.data()) == OQS_SUCCESS);
    THEMIS_DEBUG("DilithiumSigner::verify (liboqs): ok={}", ok);
    return ok;
#else
    // PERMANENT FALLBACK: Ed25519 simulation
    if (public_key.size() != 32) return false;
    bool ok = ed25519_verify(message, signature, public_key);
    THEMIS_DEBUG("DilithiumSigner::verify: ok={}", ok);
    return ok;
#endif
}

// ============================================================================
// PostQuantumKeyProvider
// ============================================================================

PostQuantumKeyProvider::PostQuantumKeyProvider(
    std::shared_ptr<KeyProvider> classical_provider,
    PQMigrationMode mode)
    : classical_provider_(std::move(classical_provider))
    , mode_(mode)
    , kyber_(KyberKEM::SecurityLevel::KYBER_1024)
{
    if (!classical_provider_) {
        throw std::invalid_argument(
            "PostQuantumKeyProvider: classical_provider must not be null");
    }
    THEMIS_INFO("PostQuantumKeyProvider: initialized, mode={}", static_cast<int>(mode_));
}

PostQuantumKeyProvider::~PostQuantumKeyProvider() = default;

// Delegate all standard KeyProvider operations to the classical provider.

std::vector<uint8_t> PostQuantumKeyProvider::getKey(const std::string& key_id) {
    return classical_provider_->getKey(key_id);
}

std::vector<uint8_t> PostQuantumKeyProvider::getKey(const std::string& key_id,
                                                     uint32_t version) {
    return classical_provider_->getKey(key_id, version);
}

uint32_t PostQuantumKeyProvider::rotateKey(const std::string& key_id) {
    return classical_provider_->rotateKey(key_id);
}

std::vector<KeyMetadata> PostQuantumKeyProvider::listKeys() {
    return classical_provider_->listKeys();
}

KeyMetadata PostQuantumKeyProvider::getKeyMetadata(const std::string& key_id,
                                                    uint32_t version) {
    return classical_provider_->getKeyMetadata(key_id, version);
}

void PostQuantumKeyProvider::deleteKey(const std::string& key_id, uint32_t version) {
    classical_provider_->deleteKey(key_id, version);
}

bool PostQuantumKeyProvider::hasKey(const std::string& key_id, uint32_t version) {
    return classical_provider_->hasKey(key_id, version);
}

uint32_t PostQuantumKeyProvider::createKeyFromBytes(const std::string& key_id,
                                                     const std::vector<uint8_t>& key_bytes,
                                                     const KeyMetadata& metadata) {
    return classical_provider_->createKeyFromBytes(key_id, key_bytes, metadata);
}

/*
 * Kyber-wrapped DEK blob format (all integer fields are LE uint32):
 *   [4] kem_ct_len
 *   [kem_ct_len] Kyber KEM ciphertext (= ephemeral X25519 public key in sim)
 *   [12] AES-GCM IV
 *   [4] enc_dek_len
 *   [enc_dek_len] AES-256-GCM ciphertext of DEK
 *   [16] GCM authentication tag
 */

std::vector<uint8_t>
PostQuantumKeyProvider::wrapKeyWithKyber(
    const std::vector<uint8_t>& dek,
    const std::vector<uint8_t>& recipient_public_key)
{
    if (dek.empty() || dek.size() > 256) {
        // 256-byte upper bound matches common DEK sizes (AES-128: 16 B,
        // AES-256: 32 B, ChaCha20: 32 B) while preventing accidental
        // misuse that would embed large payloads in the wire format blob.
        throw std::runtime_error("wrapKeyWithKyber: DEK size out of range (must be 1–256 bytes)");
    }

    // 1. KEM encapsulate
    KyberKEM::EncapsulationResult result;
    try {
        result = kyber_.encapsulate(recipient_public_key);
    } catch (const std::exception& e) {
        throw std::runtime_error("wrapKeyWithKyber: encapsulate failed: " + std::string(e.what()));
    }
    const auto& kem_ct = result.ciphertext;
    const auto& kem_ss = result.shared_secret;  // 32-byte wrapping key

    // 2. Encrypt the DEK under the shared secret
    auto iv = random_iv();
    std::array<uint8_t, 16> tag{};
    auto enc_dek = aes256gcm_encrypt(kem_ss, iv, dek, tag);

    // 3. Serialise blob
    std::vector<uint8_t> blob;
    blob.reserve(4 + kem_ct.size() + 12 + 4 + enc_dek.size() + 16);
    write_u32_le(blob, static_cast<uint32_t>(kem_ct.size()));
    blob.insert(blob.end(), kem_ct.begin(), kem_ct.end());
    blob.insert(blob.end(), iv.begin(), iv.end());    // 12 bytes
    write_u32_le(blob, static_cast<uint32_t>(enc_dek.size()));
    blob.insert(blob.end(), enc_dek.begin(), enc_dek.end());
    blob.insert(blob.end(), tag.begin(), tag.end());  // 16 bytes

    THEMIS_INFO("PostQuantumKeyProvider::wrapKeyWithKyber: dek_len={}, blob_len={}",
                dek.size(), blob.size());
    return blob;
}

std::vector<uint8_t>
PostQuantumKeyProvider::unwrapKeyWithKyber(
    const std::vector<uint8_t>& wrapped_key,
    const std::vector<uint8_t>& secret_key)
{
    // Minimum size: 4 + 32 (kem_ct) + 12 (iv) + 4 + 1 (enc_dek) + 16 (tag)
    if (wrapped_key.size() < 4 + 32 + 12 + 4 + 1 + 16) {
        throw std::runtime_error("unwrapKeyWithKyber: blob too short");
    }

    const uint8_t* p = wrapped_key.data();
    const uint8_t* end = p + wrapped_key.size();

    // Read KEM ciphertext
    uint32_t kem_ct_len = read_u32_le(p); p += 4;
    if (p + kem_ct_len > end)
        throw std::runtime_error("unwrapKeyWithKyber: kem_ct truncated");
    std::vector<uint8_t> kem_ct(p, p + kem_ct_len); p += kem_ct_len;

    // Read IV (12 bytes)
    if (p + 12 > end)
        throw std::runtime_error("unwrapKeyWithKyber: IV truncated");
    std::array<uint8_t, 12> iv{};
    std::copy(p, p + 12, iv.begin()); p += 12;

    // Read encrypted DEK
    if (p + 4 > end)
        throw std::runtime_error("unwrapKeyWithKyber: enc_dek_len truncated");
    uint32_t enc_dek_len = read_u32_le(p); p += 4;
    if (p + enc_dek_len > end)
        throw std::runtime_error("unwrapKeyWithKyber: enc_dek truncated");
    std::vector<uint8_t> enc_dek(p, p + enc_dek_len); p += enc_dek_len;

    // Read GCM tag (16 bytes)
    if (p + 16 > end)
        throw std::runtime_error("unwrapKeyWithKyber: tag truncated");
    std::array<uint8_t, 16> tag{};
    std::copy(p, p + 16, tag.begin());

    // 1. KEM decapsulate → wrapping key
    std::vector<uint8_t> kem_ss;
    try {
        kem_ss = kyber_.decapsulate(kem_ct, secret_key);
    } catch (const std::exception& e) {
        throw std::runtime_error("unwrapKeyWithKyber: decapsulate failed: " + std::string(e.what()));
    }

    // 2. Decrypt the DEK
    auto dek = aes256gcm_decrypt(kem_ss, iv, enc_dek, tag);

    THEMIS_INFO("PostQuantumKeyProvider::unwrapKeyWithKyber: dek_len={}", dek.size());
    return dek;
}

// ============================================================================
// HybridEncryption
// ============================================================================

HybridEncryption::HybridEncryption(std::shared_ptr<KeyProvider> key_provider,
                                    PQMigrationMode mode)
    : FieldEncryption(std::move(key_provider))
    , mode_(mode)
    , kyber_(KyberKEM::SecurityLevel::KYBER_1024)
{
    THEMIS_INFO("HybridEncryption: initialized, mode={}", static_cast<int>(mode_));
}

HybridEncryption::~HybridEncryption() = default;

/*
 * Hybrid-encrypted blob:
 *   We leverage the existing EncryptedBlob container but embed extra PQ
 *   metadata in the key_id field using a special prefix:
 *
 *     key_id = "pq_hybrid:<base64(eph_pub_key)>:<original_key_id>"
 *
 *   This keeps the blob self-contained and backward-compatible with the
 *   standard FieldEncryption path (which ignores the prefix when the
 *   classical key_id is needed).
 *
 *   Combined key derivation (HYBRID mode):
 *     combined_key = HKDF(kem_ss || aes_key, 32, info="PQ_HYBRID_COMBINED")
 */

static const std::string PQ_HYBRID_PREFIX = "pq_hybrid";

// Base64 encode for embedding PQ metadata (reuse field_encryption.cpp impl
// via a local copy declared in the anonymous namespace).
namespace {

static const std::string B64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

// O(1) reverse-lookup table for b64_dec — avoids O(n²) B64_CHARS.find() in inner loop
static const std::array<uint8_t, 256> B64_DEC_TABLE = []() {
    std::array<uint8_t, 256> t{};
    t.fill(0xFF);
    for (size_t i = 0; i < B64_CHARS.size(); ++i)
        t[static_cast<unsigned char>(B64_CHARS[i])] = static_cast<uint8_t>(i);
    return t;
}();

static std::string b64_enc(const std::vector<uint8_t>& data) {
    std::string ret;
    ret.reserve((data.size() + 2) / 3 * 4);  // Pre-allocate for base64 output
    size_t i = 0;
    const uint8_t* p = data.data();
    size_t n = data.size();
    while (n >= 3) {
        ret += B64_CHARS[(p[i] & 0xfc) >> 2];
        ret += B64_CHARS[((p[i] & 0x03) << 4) | ((p[i+1] & 0xf0) >> 4)];
        ret += B64_CHARS[((p[i+1] & 0x0f) << 2) | ((p[i+2] & 0xc0) >> 6)];
        ret += B64_CHARS[p[i+2] & 0x3f];
        i += 3; n -= 3;
    }
    if (n == 2) {
        ret += B64_CHARS[(p[i] & 0xfc) >> 2];
        ret += B64_CHARS[((p[i] & 0x03) << 4) | ((p[i+1] & 0xf0) >> 4)];
        ret += B64_CHARS[(p[i+1] & 0x0f) << 2];
        ret += '=';
    } else if (n == 1) {
        ret += B64_CHARS[(p[i] & 0xfc) >> 2];
        ret += B64_CHARS[(p[i] & 0x03) << 4];
        ret += "==";
    }
    return ret;
}

static bool is_b64(uint8_t c) {
    return isalnum(c) || c == '+' || c == '/';
}

static std::vector<uint8_t> b64_dec(const std::string& s) {
    std::vector<uint8_t> ret;
    // Pre-allocate approximate size: each 4 base64 chars = 3 bytes
    ret.reserve((s.size() / 4 + 1) * 3);
    
    int i = 0;
    size_t in_pos = 0;
    uint8_t ca4[4] = {0}, ca3[3] = {0};  // Initialize arrays to prevent uninitialized read
    while (in_pos < s.size() && s[in_pos] != '=' && is_b64(s[in_pos])) {
        ca4[i++] = s[in_pos++];
        if (i == 4) {
            for (int k = 0; k < 4; ++k)
                ca4[k] = B64_DEC_TABLE[ca4[k]];
            ca3[0] = (ca4[0] << 2) | ((ca4[1] & 0x30) >> 4);
            ca3[1] = ((ca4[1] & 0x0f) << 4) | ((ca4[2] & 0x3c) >> 2);
            ca3[2] = ((ca4[2] & 0x03) << 6) | ca4[3];
            for (int k = 0; k < 3; ++k) ret.push_back(ca3[k]);
            i = 0;
        }
    }
    if (i) {
        for (int k = i; k < 4; ++k) ca4[k] = 0;
        for (int k = 0; k < 4; ++k)
            ca4[k] = B64_DEC_TABLE[ca4[k]];
        ca3[0] = (ca4[0] << 2) | ((ca4[1] & 0x30) >> 4);
        ca3[1] = ((ca4[1] & 0x0f) << 4) | ((ca4[2] & 0x3c) >> 2);
        for (int k = 0; k < i - 1; ++k) ret.push_back(ca3[k]);
    }
    return ret;
}

} // anonymous namespace

EncryptedBlob
HybridEncryption::encryptHybrid(const std::string& key_id,
                                 const std::string& plaintext)
{
    if (mode_ == PQMigrationMode::CLASSICAL_ONLY) {
        // Fall back to pure classical AES-256-GCM
        return encrypt(plaintext, key_id);
    }

    // 1. Get classical AES key
    auto aes_key = getKeyProvider()->getKey(key_id);
    auto metadata = getKeyProvider()->getKeyMetadata(key_id);

    // Generate ephemeral Kyber key pair for this encryption event.
    // Self-encapsulation: in single-process field encryption the engine is
    // both encapsulator and decapsulator.  The ephemeral secret key is stored
    // in the blob's key_id field so decryptHybrid() can recover it.
    // For multi-party use, replace this with encapsulation under a long-term
    // recipient public key provided externally.
    KyberKEM::KeyPair kp;
    try {
        kp = kyber_.generateKeyPair();
    } catch (const std::exception& e) {
        throw std::runtime_error("encryptHybrid: generateKeyPair failed: " + std::string(e.what()));
    }
     
    // Encapsulate using the ephemeral public key as the recipient key
    KyberKEM::EncapsulationResult enc_result;
    try {
        enc_result = kyber_.encapsulate(kp.public_key);
    } catch (const std::exception& e) {
        throw std::runtime_error("encryptHybrid: encapsulate failed: " + std::string(e.what()));
    }

    // 3. Derive combined key: HKDF(kem_ss || aes_key)
    std::vector<uint8_t> ikm;
    ikm.insert(ikm.end(), enc_result.shared_secret.begin(), enc_result.shared_secret.end());
    ikm.insert(ikm.end(), aes_key.begin(), aes_key.end());
    const std::string info_str = "PQ_HYBRID_COMBINED";
    std::vector<uint8_t> info(info_str.begin(), info_str.end());
    auto combined_key = hkdf_sha256(ikm, {}, info, 32);

    // 4. Encrypt plaintext with AES-256-GCM(combined_key)
    auto iv_arr = random_iv();
    std::array<uint8_t, 16> tag{};
    std::vector<uint8_t> pt_bytes(plaintext.begin(), plaintext.end());
    auto ct = aes256gcm_encrypt(combined_key, iv_arr, pt_bytes, tag);

    // 5. Build EncryptedBlob
    // Encode PQ metadata into key_id field:
    //   "pq_hybrid:<b64(kem_ct)>:<b64(eph_sk)>:<original_key_id>"
    std::string pq_key_id = PQ_HYBRID_PREFIX + ":"
        + b64_enc(enc_result.ciphertext) + ":"
        + b64_enc(kp.secret_key) + ":"
        + key_id;

    EncryptedBlob blob;
    blob.key_id = pq_key_id;
    blob.key_version = metadata.version;
    blob.iv = std::vector<uint8_t>(iv_arr.begin(), iv_arr.end());
    blob.ciphertext = std::move(ct);
    blob.tag = std::vector<uint8_t>(tag.begin(), tag.end());

    THEMIS_INFO("HybridEncryption::encryptHybrid: key_id={}, ct_len={}",
                key_id, blob.ciphertext.size());
    return blob;
}

std::string
HybridEncryption::decryptHybrid(const EncryptedBlob& blob)
{
    // Check for PQ hybrid metadata
    if (blob.key_id.rfind(PQ_HYBRID_PREFIX + ":", 0) != 0) {
        // Not a hybrid blob – fall back to classical decryption
        return decrypt(blob);
    }

    // Parse pq_key_id: "pq_hybrid:<b64(kem_ct)>:<b64(eph_sk)>:<key_id>"
    const std::string pq_prefix = PQ_HYBRID_PREFIX + ":";
    std::string rest = blob.key_id.substr(pq_prefix.size());

    // Split on ':'
    auto split3 = [](const std::string& s) -> std::tuple<std::string,std::string,std::string> {
        size_t p1 = s.find(':');
        if (p1 == std::string::npos)
            throw std::runtime_error("HybridEncryption::decryptHybrid: malformed key_id (1)");
        size_t p2 = s.find(':', p1 + 1);
        if (p2 == std::string::npos)
            throw std::runtime_error("HybridEncryption::decryptHybrid: malformed key_id (2)");
        return {s.substr(0, p1), s.substr(p1+1, p2-p1-1), s.substr(p2+1)};
    };

    auto [kem_ct_b64, eph_sk_b64, key_id] = split3(rest);
    auto kem_ct = b64_dec(kem_ct_b64);
    auto eph_sk = b64_dec(eph_sk_b64);

    // 1. Recover KEM shared secret: decapsulate(kem_ct, eph_sk)
    //    (In the self-encapsulation scheme, eph_sk is the ephemeral secret key
    //     whose corresponding public key was used for encapsulation.)
    std::vector<uint8_t> kem_ss;
    try {
        kem_ss = kyber_.decapsulate(kem_ct, eph_sk);
    } catch (const std::exception& e) {
        throw DecryptionException("decryptHybrid: decapsulate failed: " + std::string(e.what()));
    }

    // 2. Retrieve classical AES key
    auto aes_key = getKeyProvider()->getKey(key_id, blob.key_version);

    // 3. Re-derive combined key
    std::vector<uint8_t> ikm;
    ikm.insert(ikm.end(), kem_ss.begin(), kem_ss.end());
    ikm.insert(ikm.end(), aes_key.begin(), aes_key.end());
    const std::string info_str = "PQ_HYBRID_COMBINED";
    std::vector<uint8_t> info(info_str.begin(), info_str.end());
    auto combined_key = hkdf_sha256(ikm, {}, info, 32);

    // 4. Decrypt with AES-256-GCM
    if (blob.iv.size() != 12 || blob.tag.size() != 16) {
        throw DecryptionException("HybridEncryption::decryptHybrid: invalid IV/tag size");
    }
    std::array<uint8_t, 12> iv_arr{};
    std::copy(blob.iv.begin(), blob.iv.end(), iv_arr.begin());
    std::array<uint8_t, 16> tag_arr{};
    std::copy(blob.tag.begin(), blob.tag.end(), tag_arr.begin());

    auto pt_bytes = aes256gcm_decrypt(combined_key, iv_arr, blob.ciphertext, tag_arr);

    THEMIS_INFO("HybridEncryption::decryptHybrid: key_id={}, pt_len={}",
                key_id, pt_bytes.size());
    return std::string(pt_bytes.begin(), pt_bytes.end());
}

} // namespace security
} // namespace themis

// ============================================================================
// SPHINCS+ implementation — Phase 7.1
// ============================================================================
//
// PERMANENT FALLBACK NOTE (Ed25519 simulation when THEMIS_HAS_OQS is not set):
// Purpose:    Provide an API-stable SPHINCS+ interface before liboqs is
//             integrated. The signing/verification is performed with OpenSSL
//             Ed25519 (same primitive as the Dilithium fallback) so that all
//             callers compile and test against a correct interface.
// Activation: Always active when THEMIS_HAS_OQS is NOT defined.
//             Build with -DTHEMIS_HAS_OQS=ON (Wave-2 CMake guard) to activate
//             the real SPHINCS+-SHA2-256s/256f implementation via liboqs.
// Production Delta: Real SPHINCS+-SHA2-256s/256f from liboqs will produce
//             hash-tree-based signatures (~8 KB / ~50 KB) rather than the
//             64-byte Ed25519 signatures emitted here.  Key sizes also differ.
// This Ed25519 simulation is a PERMANENT FALLBACK for no-liboqs builds.

namespace themis {
namespace security {

struct themis::security::SphincsPlus::Impl {
    // Simulation: re-use Ed25519 via EVP_PKEY
};

// PERMANENT FALLBACK NOTE (SphincsPlus injectable bridge):
// Purpose:    Allow injection of a real liboqs-backed SphincsPlus implementation
//             at runtime (for integration tests or phased production rollout),
//             bypassing the Ed25519 fallback simulation without changing the public API.
// Activation: Runtime — when setGenerateKeyPairFn / setSignFn / setVerifyFn is
//             called with a non-empty function object before the first use.
// Production Delta: With no fn injected and THEMIS_HAS_OQS not set, the Ed25519
//             simulation is used; with a fn injected OR with THEMIS_HAS_OQS set,
//             the real OQS_SIG_alg_sphincs_sha2_256{s,f} path runs instead.
// This bridge is PERMANENT — it remains even when liboqs is compiled in, to allow
// runtime injection of alternative implementations for testing.
static std::mutex s_sphincs_fn_mutex_;
static themis::security::SphincsPlus::GenerateKeyPairFn s_generate_key_pair_fn_;
static themis::security::SphincsPlus::SignFn            s_sign_fn_;
static themis::security::SphincsPlus::VerifyFn          s_verify_fn_;

themis::security::SphincsPlus::SphincsPlus(Variant variant)
    : variant_(variant), impl_(std::make_unique<Impl>()) {}

themis::security::SphincsPlus::~SphincsPlus() = default;

themis::security::SphincsPlus::SphincsPlus(SphincsPlus&&) noexcept = default;
themis::security::SphincsPlus& themis::security::SphincsPlus::operator=(SphincsPlus&&) noexcept = default;

#ifdef THEMIS_HAS_OQS
namespace {
/// @brief Select liboqs SPHINCS+ algorithm by variant.
static const char* sphincsAlgName(themis::security::SphincsPlus::Variant v) noexcept {
    // SHA2-256s is the recommended conservative parameter set.
    return (v == themis::security::SphincsPlus::Variant::SHAKE_256f)
               ? OQS_SIG_alg_sphincs_shake_256f
               : OQS_SIG_alg_sphincs_sha2_256s;
}
} // anonymous namespace
#endif // THEMIS_HAS_OQS

size_t SphincsPlus::publicKeySize() const noexcept {
#ifdef THEMIS_HAS_OQS
    OqsSigRAII s(sphincsAlgName(variant_));
    return s.sig ? s.sig->length_public_key : 0;
#else
    // PERMANENT FALLBACK: Ed25519 public key (32 bytes)
    return 32;
#endif
}

size_t SphincsPlus::secretKeySize() const noexcept {
#ifdef THEMIS_HAS_OQS
    OqsSigRAII s(sphincsAlgName(variant_));
    return s.sig ? s.sig->length_secret_key : 0;
#else
    // PERMANENT FALLBACK: Ed25519 secret key (64 bytes in OpenSSL representation)
    return 64;
#endif
}

size_t SphincsPlus::signatureSize() const noexcept {
#ifdef THEMIS_HAS_OQS
    OqsSigRAII s(sphincsAlgName(variant_));
    return s.sig ? s.sig->length_signature : 0;
#else
    // PERMANENT FALLBACK: Ed25519 signature (64 bytes).
    // Real SPHINCS+-SHA2-256s: 29 792 bytes; SPHINCS+-SHA2-256f: 49 856 bytes.
    return 64;
#endif
}

/**
 * @brief Generate a SPHINCS+ key pair.
 *
 * When THEMIS_HAS_OQS is defined, uses liboqs OQS_SIG_keypair().
 * Otherwise falls back to Ed25519 simulation (PERMANENT FALLBACK).
 *
 * @return KeyPair with public_key and secret_key.
 * @throws std::runtime_error on OQS/OpenSSL failure.
 */
themis::security::SphincsPlus::KeyPair themis::security::SphincsPlus::generateKeyPair() {
    themis::security::SphincsPlus::GenerateKeyPairFn fn;
    {
        std::lock_guard<std::mutex> lk(s_sphincs_fn_mutex_);
        fn = s_generate_key_pair_fn_;
    }
    if (fn) [[unlikely]] {
        return fn();
    }

#ifdef THEMIS_HAS_OQS
    OqsSigRAII s(sphincsAlgName(variant_));
    if (!s.sig) throw std::runtime_error("SphincsPlus::generateKeyPair: OQS_SIG_new failed");
    KeyPair kp;
    kp.public_key.resize(s.sig->length_public_key);
    kp.secret_key.resize(s.sig->length_secret_key);
    if (OQS_SIG_keypair(s.sig, kp.public_key.data(), kp.secret_key.data()) != OQS_SUCCESS)
        throw std::runtime_error("SphincsPlus::generateKeyPair: OQS_SIG_keypair failed");
    THEMIS_DEBUG("SphincsPlus::generateKeyPair (liboqs variant={}) pub={}B sec={}B",
                 static_cast<int>(variant_), kp.public_key.size(), kp.secret_key.size());
    return kp;
#else
    // PERMANENT FALLBACK: Ed25519 simulation
    EVP_PKEY_CTX_ptr pctx(EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr), &EVP_PKEY_CTX_free);
    if (!pctx) throw std::runtime_error("SphincsPlus::generateKeyPair: EVP_PKEY_CTX_new_id failed");
    if (EVP_PKEY_keygen_init(pctx.get()) <= 0)
        throw std::runtime_error("SphincsPlus::generateKeyPair: keygen_init failed");
    EVP_PKEY* raw_pkey = nullptr;
    if (EVP_PKEY_keygen(pctx.get(), &raw_pkey) <= 0)
        throw std::runtime_error("SphincsPlus::generateKeyPair: keygen failed");
    EVP_PKEY_ptr pkey(raw_pkey, &EVP_PKEY_free);

    KeyPair kp;
    kp.public_key.resize(32);
    kp.secret_key.resize(64);
    size_t pub_len = 32, sec_len = 64;
    if (EVP_PKEY_get_raw_public_key(pkey.get(), kp.public_key.data(), &pub_len) != 1 ||
        EVP_PKEY_get_raw_private_key(pkey.get(), kp.secret_key.data(), &sec_len) != 1)
        throw std::runtime_error("SphincsPlus::generateKeyPair: raw key extraction failed");
    kp.public_key.resize(pub_len);
    kp.secret_key.resize(sec_len);
    THEMIS_DEBUG("SphincsPlus::generateKeyPair (SPHINCSPLUS_SIM variant={}) pub={}B sec={}B",
                 static_cast<int>(variant_), pub_len, sec_len);
    return kp;
#endif
}

/**
 * @brief Sign a message with a SPHINCS+ secret key.
 *
 * @param message     Message bytes.
 * @param secret_key  SPHINCS+ secret key.
 * @return Signature bytes (real SPHINCS+ or Ed25519 simulation).
 * @throws std::invalid_argument on wrong key size.
 * @throws std::runtime_error on OQS/OpenSSL failure.
 */
std::vector<uint8_t> themis::security::SphincsPlus::sign(const std::vector<uint8_t>& message,
                                                         const std::vector<uint8_t>& secret_key) {
    themis::security::SphincsPlus::SignFn fn;
    {
        std::lock_guard<std::mutex> lk(s_sphincs_fn_mutex_);
        fn = s_sign_fn_;
    }
    if (fn) [[unlikely]] {
        try {
            return fn(message, secret_key);
        } catch (...) {
            THEMIS_WARN([[maybe_unused]] "SphincsPlus::sign: exception from user callback (suppressed)");
            return {};
        }
    }

    if (secret_key.size() != secretKeySize()) {
        throw std::invalid_argument("SphincsPlus::sign: unexpected secret key size");
    }

#ifdef THEMIS_HAS_OQS
    OqsSigRAII s(sphincsAlgName(variant_));
    if (!s.sig) throw std::runtime_error("SphincsPlus::sign: OQS_SIG_new failed");
    std::vector<uint8_t> sig_buf(s.sig->length_signature);
    size_t sig_len = 0;
    if (OQS_SIG_sign(s.sig, sig_buf.data(), &sig_len,
                     message.data(), message.size(), secret_key.data()) != OQS_SUCCESS)
        throw std::runtime_error("SphincsPlus::sign: OQS_SIG_sign failed");
    sig_buf.resize(sig_len);
    THEMIS_DEBUG("SphincsPlus::sign (liboqs): msg={}B sig={}B", message.size(), sig_len);
    return sig_buf;
#else
    // PERMANENT FALLBACK: Ed25519 simulation
    EVP_PKEY_ptr pkey(EVP_PKEY_new_raw_private_key(
        EVP_PKEY_ED25519, nullptr, secret_key.data(), secret_key.size()),
        &EVP_PKEY_free);
    if (!pkey.get()) throw std::runtime_error("SphincsPlus::sign: key import failed");

    EVP_MD_CTX_ptr ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    if (!ctx.get()) throw std::runtime_error("SphincsPlus::sign: MD_CTX alloc");

    if (EVP_DigestSignInit(ctx.get(), nullptr, nullptr, nullptr, pkey.get()) != 1) {
        throw std::runtime_error("SphincsPlus::sign: DigestSignInit failed");
    }

    size_t sig_len = 0;
    if (EVP_DigestSign(ctx.get(), nullptr, &sig_len, message.data(), message.size()) != 1) {
        throw std::runtime_error("SphincsPlus::sign: DigestSign size query failed");
    }
    std::vector<uint8_t> sig(sig_len);
    if (EVP_DigestSign(ctx.get(), sig.data(), &sig_len, message.data(), message.size()) != 1) {
        throw std::runtime_error("SphincsPlus::sign: DigestSign failed");
    }
    sig.resize(sig_len);
    return sig;
    // RAII wrappers (ctx, pkey) automatically clean up on scope exit
#endif
}

/**
 * @brief Verify a SPHINCS+ signature.
 *
 * @param message     Original message bytes.
 * @param signature   Signature to verify.
 * @param public_key  Signer's SPHINCS+ public key.
 * @return true if valid, false otherwise.
 */
bool themis::security::SphincsPlus::verify(const std::vector<uint8_t>& message,
                                           const std::vector<uint8_t>& signature,
                                           const std::vector<uint8_t>& public_key) {
    themis::security::SphincsPlus::VerifyFn fn;
    {
        std::lock_guard<std::mutex> lk(s_sphincs_fn_mutex_);
        fn = s_verify_fn_;
    }
    if (fn) [[unlikely]] {
        try {
            return fn(message, signature, public_key);
        } catch (...) {
            THEMIS_WARN([[maybe_unused]] "SphincsPlus::verify: exception from user callback (suppressed)");
            return false;
        }
    }

    if (public_key.size() != publicKeySize()) return false;

#ifdef THEMIS_HAS_OQS
    OqsSigRAII s(sphincsAlgName(variant_));
    if (!s.sig) return false;
    bool ok = (OQS_SIG_verify(s.sig, message.data(), message.size(),
                               signature.data(), signature.size(),
                               public_key.data()) == OQS_SUCCESS);
    THEMIS_DEBUG("SphincsPlus::verify (liboqs): ok={}", ok);
    return ok;
#else
    // PERMANENT FALLBACK: Ed25519 simulation
    EVP_PKEY_ptr pkey(
        EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr, public_key.data(), public_key.size()),
        &EVP_PKEY_free);
    if (!pkey.get()) return false;

    EVP_MD_CTX_ptr ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    if (!ctx.get()) return false;

    bool ok = false;
    if (EVP_DigestVerifyInit(ctx.get(), nullptr, nullptr, nullptr, pkey.get()) == 1) {
        ok = (EVP_DigestVerify(ctx.get(), signature.data(), signature.size(),
                                message.data(), message.size()) == 1);
    }
    // RAII wrappers (ctx, pkey) automatically clean up on scope exit
    return ok;
#endif
}

// ── SphincsPlus bridge setters ────────────────────────────────────────────────

void themis::security::SphincsPlus::setGenerateKeyPairFn(themis::security::SphincsPlus::GenerateKeyPairFn fn) {
    std::lock_guard<std::mutex> lk(s_sphincs_fn_mutex_);
    s_generate_key_pair_fn_ = std::move(fn);
}

void themis::security::SphincsPlus::setSignFn(themis::security::SphincsPlus::SignFn fn) {
    std::lock_guard<std::mutex> lk(s_sphincs_fn_mutex_);
    s_sign_fn_ = std::move(fn);
}

void themis::security::SphincsPlus::setVerifyFn(themis::security::SphincsPlus::VerifyFn fn) {
    std::lock_guard<std::mutex> lk(s_sphincs_fn_mutex_);
    s_verify_fn_ = std::move(fn);
}

} // namespace security
} // namespace themis


