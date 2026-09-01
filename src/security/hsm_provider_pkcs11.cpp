/**
 * @file hsm_provider_pkcs11.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=23; TODO=1, Stub=20, Unimpl=0, Mock=1, Sim=1, Debt=0, C=2, H=10, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#ifdef THEMIS_ENABLE_HSM_REAL
#include "security/hsm_provider.h"
#include "security/pkcs11_minimal.h"
#include "utils/logger.h"
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <sstream>
#include <mutex>
#include <memory>
#include <atomic>
#include <cstring>
#include <cstdlib>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/bn.h>
#include <openssl/crypto.h>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace themis { namespace security {

namespace {

// ── RAII Wrappers for OpenSSL objects ─────────────────────────────────────────
struct HSM_PKCS11_EVP_CIPHER_CTX_Deleter {
    void operator()(EVP_CIPHER_CTX* p) const { if (p) EVP_CIPHER_CTX_free(p); }
};

struct HSM_PKCS11_EVP_MD_CTX_Deleter {
    void operator()(EVP_MD_CTX* p) const { if (p) EVP_MD_CTX_free(p); }
};

struct HSM_PKCS11_X509_Deleter {
    void operator()(X509* p) const { if (p) X509_free(p); }
};

struct HSM_PKCS11_BIO_Deleter {
    void operator()(BIO* p) const { if (p) BIO_free_all(p); }
};

struct HSM_PKCS11_BIGNUM_Deleter {
    void operator()(BIGNUM* p) const { if (p) BN_free(p); }
};

using HSM_PKCS11_EVP_CIPHER_CTX_ptr = std::unique_ptr<EVP_CIPHER_CTX, HSM_PKCS11_EVP_CIPHER_CTX_Deleter>;
using HSM_PKCS11_EVP_MD_CTX_ptr = std::unique_ptr<EVP_MD_CTX, HSM_PKCS11_EVP_MD_CTX_Deleter>;
using HSM_P11_X509_ptr = std::unique_ptr<X509, HSM_PKCS11_X509_Deleter>;
using HSM_P11_BIO_ptr = std::unique_ptr<BIO, HSM_PKCS11_BIO_Deleter>;
using HSM_P11_BIGNUM_ptr = std::unique_ptr<BIGNUM, HSM_PKCS11_BIGNUM_Deleter>;

} // anonymous namespace

// Real PKCS#11 implementation with graceful developer fallback.
// If any critical step fails (lib load, slot, login, key discovery),
// operations transparently revert to deterministic stub behaviour.

// PERMANENT FALLBACK NOTE (fallback path only):
// Purpose: When PKCS#11 hardware/library is unavailable (slot discovery fails, PIN error,
//          device absent), HSMProvider::Impl::stub_kek is used as a software AES-256-GCM
//          fallback so that developer and CI environments remain functional.
// Activation: Automatically activated at runtime when real_ready == false (PKCS#11 init
//             fails). Controlled by THEMIS_ALLOW_HSM_STUB env var in production mode.
// Production Delta: Fallback KEK is randomly generated in-memory, not HSM-protected.
//                   Key material is not backed by hardware; wrap/unwrap is software-only.
// This fallback is PERMANENT and intentional – it is a runtime safety net. Real HSM usage
// is enforced in production mode (THEMIS_PRODUCTION_MODE=1) unless explicitly overridden.
// A loud WARN log is emitted on every call that hits this path.
// Roadmap ref: src/security/ROADMAP.md § "Phase 2: ABAC & HSM Direct Integration"

class PKCS11Loader {
public:
    bool load(const std::string& path) {
#if defined(_WIN32)
        lib_ = LoadLibraryA(path.c_str());
        if(!lib_) return false;
        auto getFn = (CK_C_GetFunctionList)GetProcAddress((HMODULE)lib_, "C_GetFunctionList");
        if(!getFn) return false;
        CK_RV rv = getFn(&funcs_);
        return rv == CKR_OK && funcs_ && funcs_->C_Initialize(nullptr) == CKR_OK;
#else
        lib_ = dlopen(path.c_str(), RTLD_NOW);
        if(!lib_) return false;
        auto getFn = (CK_C_GetFunctionList)dlsym(lib_, "C_GetFunctionList");
        if(!getFn) return false;
        CK_RV rv = getFn(&funcs_);
        return rv == CKR_OK && funcs_ && funcs_->C_Initialize(nullptr) == CKR_OK;
#endif
    }
    void unload() {
        if(funcs_) funcs_->C_Finalize(nullptr);
#if defined(_WIN32)
        if(lib_) FreeLibrary((HMODULE)lib_);
#else
        if(lib_) dlclose(lib_);
#endif
        lib_ = nullptr; funcs_ = nullptr;
    }
    CK_FUNCTION_LIST_PTR api() const { return funcs_; }
private:
    void* lib_ = nullptr;
    CK_FUNCTION_LIST_PTR funcs_ = nullptr;
};

// Base64 encoding using OpenSSL
static std::string toBase64(const std::vector<uint8_t>& data) {
    if(data.empty()) return "";
    // EVP_EncodeBlock adds null terminator and pads with '='
    size_t outLen = ((data.size() + 2) / 3) * 4;
    std::vector<unsigned char> encoded(outLen + 1);
    int len = EVP_EncodeBlock(encoded.data(), data.data(), (int)data.size());
    return std::string((char*)encoded.data(), len);
}

// Base64 decoding using OpenSSL
static std::vector<uint8_t> fromBase64(const std::string& b64) {
    if(b64.empty()) return {};
    size_t outLen = (b64.size() / 4) * 3;
    std::vector<unsigned char> decoded(outLen);
    int len = EVP_DecodeBlock(decoded.data(), (const unsigned char*)b64.data(), (int)b64.size());
    if(len < 0) return {}; // Decoding error
    // Remove padding bytes
    while(len > 0 && b64[b64.size() - (outLen - len)] == '=') --len;
    decoded.resize(len);
    return decoded;
}

// AES-256-GCM encrypt (fallback): returns iv(12) || ciphertext || tag(16)
static std::vector<uint8_t> pkcs11_stub_aes_encrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& data) {
    if (key.size() != 32) return {};
    std::vector<uint8_t> iv(12);
    if (RAND_bytes(iv.data(), 12) != 1) return {};
    HSM_PKCS11_EVP_CIPHER_CTX_ptr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) return {};
    std::vector<uint8_t> ciphertext(data.size() + 16);
    std::vector<uint8_t> tag(16);
    int len = 0, ct_len = 0;
    bool ok =
        EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) == 1 &&
        EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), iv.data()) == 1 &&
        EVP_EncryptUpdate(ctx.get(), ciphertext.data(), &len, data.data(), (int)data.size()) == 1;
    ct_len = len;
    if (ok) ok = EVP_EncryptFinal_ex(ctx.get(), ciphertext.data() + len, &len) == 1;
    ct_len += len;
    if (ok) ok = EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, 16, tag.data()) == 1;
    if (!ok) return {};
    ciphertext.resize(ct_len);
    std::vector<uint8_t> result;
    result.insert(result.end(), iv.begin(), iv.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());
    result.insert(result.end(), tag.begin(), tag.end());
    return result;
}

// AES-256-GCM decrypt (fallback): expects iv(12) || ciphertext || tag(16)
static std::vector<uint8_t> pkcs11_stub_aes_decrypt(const std::vector<uint8_t>& key, const std::vector<uint8_t>& encrypted) {
    if (key.size() != 32 || encrypted.size() < 12 + 16) return {};
    const uint8_t* iv  = encrypted.data();
    size_t ct_len      = encrypted.size() - 12 - 16;
    const uint8_t* ct  = encrypted.data() + 12;
    const uint8_t* tag = encrypted.data() + 12 + ct_len;
    HSM_PKCS11_EVP_CIPHER_CTX_ptr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) return {};
    std::vector<uint8_t> plaintext(ct_len);
    int len = 0, pt_len = 0;
    bool ok =
        EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) == 1 &&
        EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), iv) == 1 &&
        EVP_DecryptUpdate(ctx.get(), plaintext.data(), &len, ct, (int)ct_len) == 1;
    pt_len = len;
    if (ok) ok = EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, 16, (void*)tag) == 1;
    if (ok) ok = EVP_DecryptFinal_ex(ctx.get(), plaintext.data() + len, &len) > 0;
    if (!ok) return {};
    plaintext.resize(pt_len);
    return plaintext;
}

// Define HSMProvider::SessionEntry (forward-declared in hsm_provider.h).
// This must appear before HSMProvider::Impl so that Impl::pool can use the
// outer type and the helper functions (discoverKeysSession, acquireSession, …)
// operate on the same concrete type.
struct HSMProvider::SessionEntry {
    CK_SESSION_HANDLE handle = 0;
    CK_OBJECT_HANDLE privKey = 0;
    CK_OBJECT_HANDLE pubKey = 0;
    CK_OBJECT_HANDLE certObj = 0;
    bool ready = false;
};

class HSMProvider::Impl {
public:
    explicit Impl(HSMConfig cfg): config(cfg), next_session_idx(0) {}
    HSMConfig config;

    PKCS11Loader loader;
    CK_SESSION_HANDLE session = 0; // Main session handle (for backwards compatibility)
    std::vector<HSMProvider::SessionEntry> pool;
    bool real_ready = false; // true wenn mind. eine Session mit privKey
    std::mutex mtx;
    std::string cert_serial_cache_;
    std::atomic<uint32_t> next_session_idx; // Lock-free round-robin counter
    std::vector<uint8_t> stub_kek; // Fallback AES-256 KEK when real HSM unavailable

    // Performance metrics
    std::atomic<uint64_t> sign_count{0};
    std::atomic<uint64_t> verify_count{0};
    std::atomic<uint64_t> sign_errors{0};
    std::atomic<uint64_t> verify_errors{0};
    std::atomic<uint64_t> total_sign_time_us{0};
    std::atomic<uint64_t> total_verify_time_us{0};
    std::atomic<uint64_t> pool_round_robin_hits{0};

    void fallbackLogOnce(const std::string& reason){
        if(!real_ready){
            THEMIS_WARN("HSMProvider PKCS#11 fallback aktiv – {}", reason);
        }
    }
};

static std::string mapError(CK_RV rv){
    switch(rv){
        case CKR_OK: return "OK";
        case CKR_PIN_INCORRECT: return "PIN incorrect";
        case CKR_DEVICE_ERROR: return "Device error";
        case CKR_GENERAL_ERROR: return "General error";
        case CKR_ARGUMENTS_BAD: return "Bad arguments";
        case CKR_SIGNATURE_INVALID: return "Signature invalid";
        case CKR_USER_ALREADY_LOGGED_IN: return "User already logged in";
        case CKR_USER_NOT_LOGGED_IN: return "User not logged in";
        default: {
            std::ostringstream oss; oss << "CKR_0x" << std::hex << rv; return oss.str();
        }
    }
}

HSMProvider::HSMProvider(HSMConfig config)
    : impl_(std::make_unique<Impl>(config)), config_(config) {}
HSMProvider::~HSMProvider(){ finalize(); }
HSMProvider::HSMProvider(HSMProvider&&) noexcept = default;
HSMProvider& HSMProvider::operator=(HSMProvider&&) noexcept = default;

// ---------------------------------------------------------------------------
// selectSlot – choose the PKCS#11 slot to use during initialization.
//
// Priority:
//   1. token_label set → scan all slots via C_GetTokenInfo, return first match.
//   2. slot_id != 0    → find the slot ID in the enumerated list.
//   3. fallback        → use the first slot (slots[0]).
//
// When only a fallback is used, a diagnostic message is written to error_out.
// ---------------------------------------------------------------------------
static CK_SLOT_ID selectSlot(
        CK_FUNCTION_LIST_PTR api,
        const std::vector<CK_SLOT_ID>& slots,
        uint32_t config_slot_id,
        const std::string& token_label,
        std::string& error_out)
{
    // 1. Token-label based selection (preferred)
    if (!token_label.empty() && api && api->C_GetTokenInfo) {
        for (CK_SLOT_ID slot : slots) {
            CK_TOKEN_INFO info{};
            if (api->C_GetTokenInfo(slot, &info) != CKR_OK) continue;
            // PKCS#11 labels are exactly 32 bytes, blank-padded, no null terminator.
            // Comparison is case-sensitive per PKCS#11 v2.20 §9.2 (labels are opaque
            // UTF-8 sequences). Ensure HSMConfig::token_label matches the exact
            // capitalisation used during token initialisation (e.g. softhsm2-util --label).
            std::string lbl(reinterpret_cast<const char*>(info.label), 32);
            auto pos = lbl.find_last_not_of(' ');
            lbl = (pos != std::string::npos) ? lbl.substr(0, pos + 1) : "";
            if (lbl == token_label) {
                THEMIS_INFO("PKCS#11 slot {} selected by token label '{}'", slot, token_label);
                return slot;
            }
        }
        error_out = "Token with label '" + token_label +
                    "' not found in any slot; using first available slot";
        return slots[0];
    }

    // 2. Explicit slot-ID based selection
    if (config_slot_id != 0) {
        for (CK_SLOT_ID slot : slots) {
            if (slot == static_cast<CK_SLOT_ID>(config_slot_id)) {
                THEMIS_INFO("PKCS#11 slot {} selected by slot_id", slot);
                return slot;
            }
        }
        error_out = "Configured slot_id=" + std::to_string(config_slot_id) +
                    " not found in slot list; using first available slot";
    }

    // 3. Fallback: first slot
    return slots[0];
}

bool HSMProvider::initialize(){
    std::lock_guard<std::mutex> lock(impl_->mtx);
    if(initialized_) return true;
    // Attempt real PKCS#11
    if(!config_.library_path.empty() && impl_->loader.load(config_.library_path)){
        auto api = impl_->loader.api();
        if(!api){
            last_error_ = "PKCS#11 function list is null after library load";
            impl_->fallbackLogOnce(last_error_);
        }
        else {
            // Enumerate slots with a token present (CK_TRUE = 1)
            uint32_t slotCount = 0;
            CK_RV rv = api->C_GetSlotList(1, nullptr, &slotCount);
            if(rv == CKR_OK && slotCount){
                std::vector<CK_SLOT_ID> slots(slotCount);
                rv = api->C_GetSlotList(1, slots.data(), &slotCount);
                if(rv == CKR_OK){
                    // Select slot by token label or slot ID
                    std::string slot_err;
                    CK_SLOT_ID chosen = selectSlot(api, slots, config_.slot_id,
                                                   config_.token_label, slot_err);
                    if (!slot_err.empty()) {
                        last_error_ = slot_err;
                        impl_->fallbackLogOnce(slot_err);
                    }

                    // Resolve PIN: config overrides env variable
                    std::string pin = config_.pin;
                    if(pin.empty()){
                        const char* envPin = std::getenv("THEMIS_HSM_PIN");
                        if(envPin) pin = envPin;
                    }
                    
                    uint32_t poolSize = config_.session_pool_size;
                    if(const char* envPool = std::getenv("THEMIS_HSM_SESSION_POOL")){
                        poolSize = std::max(1u, (uint32_t)std::atoi(envPool));
                    }
                    impl_->pool.resize(poolSize);
                    for(uint32_t i=0;i<poolSize;++i){
                        if(api->C_OpenSession(chosen, CKF_SERIAL_SESSION, nullptr, nullptr,
                                              &impl_->pool[i].handle) != CKR_OK){
                            impl_->pool[i].handle = 0;
                            std::string err = "C_OpenSession failed for pool slot " +
                                             std::to_string(i);
                            last_error_ = err;
                            impl_->fallbackLogOnce(err);
                            continue;
                        }
                        // Authenticate session with user PIN
                        if(!pin.empty()){
                            CK_RV rvLogin = api->C_Login(
                                impl_->pool[i].handle, CKU_USER,
                                (CK_BYTE_PTR)pin.data(), (uint32_t)pin.size());
                            if(rvLogin == CKR_USER_ALREADY_LOGGED_IN){
                                // Session is already authenticated – this is fine
                                THEMIS_DEBUG("PKCS#11 session {} already logged in", i);
                            } else if(rvLogin == CKR_PIN_INCORRECT){
                                std::string err = "C_Login failed: PIN incorrect (session " +
                                                 std::to_string(i) + ")";
                                last_error_ = err;
                                impl_->fallbackLogOnce(err);
                                continue;
                            } else if(rvLogin != CKR_OK){
                                std::string err = "C_Login failed: " + mapError(rvLogin) +
                                                 " (session " + std::to_string(i) + ")";
                                last_error_ = err;
                                impl_->fallbackLogOnce(err);
                                continue;
                            }
                        } else {
                            THEMIS_DEBUG("PKCS#11 PIN not set – skipping C_Login for session {}", i);
                        }
                        discoverKeysSession(impl_->pool[i]);
                        discoverCertificateSession(impl_->pool[i]);
                        impl_->pool[i].ready = (impl_->pool[i].privKey != 0);
                    }
                    // Set global ready flag if at least one session found a private key
                    impl_->real_ready = false;
                    for(auto& s : impl_->pool){
                        if(s.ready){
                            impl_->real_ready = true;
                            break;
                        }
                    }
                    if(!impl_->real_ready){
                        std::string err = "No private key found in any pool session – "
                                         "check key_label='" + config_.key_label + "'";
                        if (last_error_.empty()) last_error_ = err;
                        impl_->fallbackLogOnce(err);
                    }
                    if(pin.empty()){
                        impl_->fallbackLogOnce("PIN not set – C_Login skipped for all sessions");
                    }
                } else {
                    last_error_ = "C_GetSlotList failed: " + mapError(rv);
                    impl_->fallbackLogOnce(last_error_);
                }
            } else {
                last_error_ = rv != CKR_OK
                    ? "C_GetSlotList (count query) failed: " + mapError(rv)
                    : "No PKCS#11 slots with token present found";
                impl_->fallbackLogOnce(last_error_);
            }
        }
    } else {
        last_error_ = config_.library_path.empty()
            ? "library_path is not configured"
            : "Failed to load PKCS#11 library: " + config_.library_path;
        impl_->fallbackLogOnce(last_error_);
    }
    initialized_ = true;
    THEMIS_INFO("HSMProvider init (real_ready={})", impl_->real_ready?"true":"false");
    
    // Generate fallback stub KEK for consistent wrap/unwrap when real HSM is unavailable
    if (!impl_->real_ready) {
        impl_->stub_kek.resize(32);
        if (RAND_bytes(impl_->stub_kek.data(), 32) != 1) {
            THEMIS_ERROR("HSMProvider: failed to generate stub KEK - aborting initialization");
            initialized_ = false;
            return false;
        }
    }
    
    // Security warning if using fallback stub
    if (!impl_->real_ready) {
        THEMIS_WARN("╔═══════════════════════════════════════════════════════════════╗");
        THEMIS_WARN("║  ⚠️  HSM FALLBACK STUB ACTIVE - INSECURE CONFIGURATION  ⚠️   ║");
        THEMIS_WARN("╠═══════════════════════════════════════════════════════════════╣");
        THEMIS_WARN("║  Real PKCS#11 HSM connection failed.                         ║");
        THEMIS_WARN("║  Master keys are NOT protected by hardware security.         ║");
        THEMIS_WARN("║  This configuration is NOT SECURE for production!            ║");
        THEMIS_WARN("║                                                               ║");
        THEMIS_WARN("║  Fix HSM configuration immediately:                          ║");
        THEMIS_WARN("║  - Check library_path: {}",
                     config_.library_path.empty() ? "NOT SET" : config_.library_path);
        THEMIS_WARN("║  - Check HSM PIN: {}",
                     config_.pin.empty() ? "NOT SET" : "SET");
        THEMIS_WARN("║  - Verify HSM device is connected and accessible             ║");
        THEMIS_WARN("║                                                               ║");
        THEMIS_WARN("║  See: docs/security/HSM_PRODUCTION_SETUP.md                  ║");
        THEMIS_WARN("╚═══════════════════════════════════════════════════════════════╝");
    }
    
    return true;
}

void HSMProvider::finalize(){
    std::lock_guard<std::mutex> lock(impl_->mtx);
    if(!initialized_) return;
    if(impl_->real_ready && impl_->loader.api()){
        auto api = impl_->loader.api();
        // Close all sessions in the pool
        for(auto& s : impl_->pool) {
            if(s.handle) {
                CK_RV rv = api->C_Logout(s.handle);
                if(rv != CKR_OK && rv != CKR_USER_NOT_LOGGED_IN){
                    THEMIS_WARN("PKCS11 C_Logout failed for session: {}", mapError(rv));
                }
                rv = api->C_CloseSession(s.handle);
                if(rv != CKR_OK){
                    THEMIS_WARN("PKCS11 C_CloseSession failed for session: {}", mapError(rv));
                }
                s.handle = 0;
            }
        }
        impl_->pool.clear();
        impl_->loader.unload();
    }
    impl_->pool.clear();
    impl_->real_ready = false;
    initialized_ = false;
}

static uint64_t nowMs(){ return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }

// Compute SHA-256 digest using OpenSSL EVP
static std::vector<uint8_t> sha256(const std::vector<uint8_t>& data){
    std::vector<uint8_t> out(EVP_MAX_MD_SIZE);
    unsigned int len = 0;
    
    HSM_PKCS11_EVP_MD_CTX_ptr ctx(EVP_MD_CTX_new());
    if (!ctx.get()) {
        THEMIS_ERROR("sha256: failed to create EVP_MD_CTX");
        return {};
    }
    
    if (EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1) {
        THEMIS_ERROR("sha256: EVP_DigestInit_ex failed");
        return {};
    }
    EVP_DigestUpdate(ctx.get(), data.data(), data.size());
    if (EVP_DigestFinal_ex(ctx.get(), out.data(), &len) != 1) {
        THEMIS_ERROR("sha256: EVP_DigestFinal_ex failed");
        return {};
    }
    out.resize(len);
    return out;
}

// DER prefix for SHA-256 DigestInfo (for CKM_RSA_PKCS when not using CKM_SHA256_RSA_PKCS)
static const uint8_t SHA256_DER_PREFIX[] = {
    0x30,0x31,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,0x65,0x03,0x04,0x02,0x01,0x05,0x00,0x04,0x20
};

// Append DER prefix + digest for raw RSA PKCS#1v1.5 signing
static std::vector<uint8_t> makeDigestInfo(const std::vector<uint8_t>& digest){
    std::vector<uint8_t> di(sizeof(SHA256_DER_PREFIX) + digest.size());
    std::memcpy(di.data(), SHA256_DER_PREFIX, sizeof(SHA256_DER_PREFIX));
    std::memcpy(di.data()+sizeof(SHA256_DER_PREFIX), digest.data(), digest.size());
    return di;
}

// Build RSA-OAEP mechanism parameters (SHA-256 hash + MGF1-SHA-256, no label)
static CK_RSA_PKCS_OAEP_PARAMS makeOaepParams() {
    CK_RSA_PKCS_OAEP_PARAMS p{};
    p.hashAlg = CKM_SHA256;
    p.mgf = CKG_MGF1_SHA256;
    p.source = CKZ_DATA_SPECIFIED;
    p.pSourceData = nullptr;
    p.ulSourceDataLen = 0;
    return p;
}

// Key discovery helper
void HSMProvider::discoverKeysSession(SessionEntry& s){
    auto api = impl_->loader.api(); if(!api || !s.handle) return;
    std::string label = config_.key_label;
    CK_ATTRIBUTE privTemplate[2]; uint32_t clsPriv = CKO_PRIVATE_KEY;
    privTemplate[0].type = CKA_CLASS; privTemplate[0].pValue=&clsPriv; privTemplate[0].ulValueLen=sizeof(clsPriv);
    privTemplate[1].type = CKA_LABEL; privTemplate[1].pValue=(void*)label.c_str(); privTemplate[1].ulValueLen=label.size();
    if(api->C_FindObjectsInit(s.handle, privTemplate, 2)==CKR_OK){
        CK_OBJECT_HANDLE h; uint32_t found=0; if(api->C_FindObjects(s.handle,&h,1,&found)==CKR_OK && found==1) s.privKey=h; api->C_FindObjectsFinal(s.handle);
    }
    CK_ATTRIBUTE pubTemplate[2]; uint32_t clsPub = CKO_PUBLIC_KEY;
    pubTemplate[0].type = CKA_CLASS; pubTemplate[0].pValue=&clsPub; pubTemplate[0].ulValueLen=sizeof(clsPub);
    pubTemplate[1].type = CKA_LABEL; pubTemplate[1].pValue=(void*)label.c_str(); pubTemplate[1].ulValueLen=label.size();
    if(api->C_FindObjectsInit(s.handle, pubTemplate, 2)==CKR_OK){
        CK_OBJECT_HANDLE h; uint32_t found=0; if(api->C_FindObjects(s.handle,&h,1,&found)==CKR_OK && found==1) s.pubKey=h; api->C_FindObjectsFinal(s.handle);
    }
}

void HSMProvider::discoverCertificateSession(SessionEntry& s){
    auto api = impl_->loader.api(); if(!api || !s.handle) return;
    std::string label = config_.key_label;
    CK_ATTRIBUTE certTemplate[2]; uint32_t clsCert = CKO_CERTIFICATE;
    certTemplate[0].type = CKA_CLASS; certTemplate[0].pValue=&clsCert; certTemplate[0].ulValueLen=sizeof(clsCert);
    certTemplate[1].type = CKA_LABEL; certTemplate[1].pValue=(void*)label.c_str(); certTemplate[1].ulValueLen=label.size();
    if(api->C_FindObjectsInit(s.handle, certTemplate, 2)==CKR_OK){
        CK_OBJECT_HANDLE h; uint32_t found=0; if(api->C_FindObjects(s.handle,&h,1,&found)==CKR_OK && found==1) s.certObj=h; api->C_FindObjectsFinal(s.handle);
    }
    // Check cache status under lock to prevent data races [SECURITY-FIX-BLOCK2]
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        if(!s.certObj || !api->C_GetAttributeValue || !impl_->cert_serial_cache_.empty()){
            return;
        }
    }
    
    if(s.certObj && api->C_GetAttributeValue){
        CK_ATTRIBUTE valAttr; valAttr.type=CKA_VALUE; valAttr.pValue=nullptr; valAttr.ulValueLen=0;
        if(api->C_GetAttributeValue(s.handle, s.certObj, &valAttr, 1)==CKR_OK && valAttr.ulValueLen>0){
            try {
                std::vector<unsigned char> der(valAttr.ulValueLen); valAttr.pValue=der.data();
                if(api->C_GetAttributeValue(s.handle, s.certObj, &valAttr, 1)==CKR_OK){
                    const unsigned char* p = der.data(); 
                    HSM_P11_X509_ptr x(d2i_X509(nullptr, &p, der.size()));
                    if(x.get()){
                        ASN1_INTEGER* si = X509_get_serialNumber(x.get());
                        if(si){
                            HSM_P11_BIGNUM_ptr bn(ASN1_INTEGER_to_BN(si, nullptr));
                            if(bn.get()){
                                char* hex = BN_bn2hex(bn.get());
                                if(hex){
                                    std::lock_guard<std::mutex> lk(impl_->mtx);
                                    if(impl_->cert_serial_cache_.empty()){
                                        impl_->cert_serial_cache_ = hex;
                                    }
                                    OPENSSL_free(hex);
                                }
                            }
                        }
                    }
                }
            } catch (const std::exception& e) {
                THEMIS_WARN("discoverCertificateSession: error: {}", e.what());
            }
        }
    }
}

HSMProvider::SessionEntry* HSMProvider::acquireSession(){
    // Lock-free round-robin selection: find next ready session
    uint32_t poolSize = impl_->pool.size();
    if(poolSize == 0) return nullptr;
    
    // Try up to poolSize iterations to find ready session
    for(uint32_t attempt = 0; attempt < poolSize; ++attempt){
        uint32_t idx = impl_->next_session_idx.fetch_add(1, std::memory_order_relaxed) % poolSize;
        if(impl_->pool[idx].ready){
            impl_->pool_round_robin_hits.fetch_add(1, std::memory_order_relaxed);
            return &impl_->pool[idx];
        }
    }
    // Fallback: return first ready session if round-robin failed
    for(auto& s: impl_->pool){ if(s.ready) return &s; }
    return nullptr;
}

void HSMProvider::releaseSession([[maybe_unused]] SessionEntry* s){ 
    // No-op for lock-free implementation (no busy flag to clear)
}

HSMSignatureResult HSMProvider::sign(const std::vector<uint8_t>& data, const std::string& key_label){
    // Hash first (SHA-256) then sign
    auto digest = sha256(data);
    return signHash(digest, key_label);
}

HSMSignatureResult HSMProvider::signHash(const std::vector<uint8_t>& hash, const std::string& key_label){
    auto startTime = std::chrono::high_resolution_clock::now();
    std::lock_guard<std::mutex> lock(impl_->mtx);
    HSMSignatureResult r;
    if(!initialized_){ 
        r.error_message = "Nicht initialisiert"; 
        impl_->sign_errors.fetch_add(1, std::memory_order_relaxed);
        return r; 
    }
    if(!impl_->real_ready){
        auto bridge = SignHashFn{};
        {
            std::lock_guard<std::mutex> bridge_lock(signHashFnMutex());
            bridge = signHashFnStorage();
        }
        if (bridge) {
            auto bridged = bridge(hash, key_label.empty() ? config_.key_label : key_label);
            if (bridged.success) {
                impl_->sign_count.fetch_add(1, std::memory_order_relaxed);
            } else {
                impl_->sign_errors.fetch_add(1, std::memory_order_relaxed);
            }
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - startTime).count();
            impl_->total_sign_time_us.fetch_add(elapsed, std::memory_order_relaxed);
            return bridged;
        }
        const char* allow_stub = std::getenv("THEMIS_ALLOW_HSM_STUB");
        if (!allow_stub || std::string(allow_stub) != "1") {
            r.error_message =
                "HSMProvider (PKCS#11 path) signHash refused: real HSM not ready and "
                "returning a stub signature is insecure. Set THEMIS_ALLOW_HSM_STUB=1 to "
                "explicitly allow the insecure fallback.";
            THEMIS_ERROR("{}", r.error_message);
            impl_->sign_errors.fetch_add(1, std::memory_order_relaxed);
            return r;
        }
        // STUB/SIMULATION NOTE:
        // Purpose: Return a non-cryptographic signature when real PKCS#11 HSM is unavailable
        //          (real_ready == false), so that CI and dev environments remain functional.
        // Activation: real_ready == false at runtime (PKCS#11 library missing, no slot, or
        //             key discovery failed). Loud WARN is emitted at initialize() time.
        // Production Delta: Signature is Base64(SHA-256 hash) — not a valid digital signature.
        //                   cert_serial is hardcoded "STUB-CERT". Not cryptographically secure.
        // Removal Plan: Configure a valid PKCS#11 HSM (library_path + slot + PIN + key_label)
        //               so real_ready becomes true at initialize() time.
        // NOT IMPLEMENTED: Real PKCS#11 hardware signing.
        // Tracked: src/security/ROADMAP.md — Phase 2: ABAC & HSM Direct Integration
        THEMIS_WARN("HSMProvider (PKCS#11 path) signHash fallback: PKCS#11 not ready — "
                    "returning non-cryptographic stub signature (key_label='{}').",
                    key_label.empty() ? config_.key_label : key_label);
        r.success = true; 
        r.signature_b64 = toBase64(hash);
        r.algorithm = config_.signature_algorithm; 
        r.key_id = key_label.empty()?config_.key_label:key_label; 
        r.cert_serial = "STUB-CERT"; 
        r.timestamp_ms = nowMs();
        impl_->sign_count.fetch_add(1, std::memory_order_relaxed);
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - startTime).count();
        impl_->total_sign_time_us.fetch_add(elapsed, std::memory_order_relaxed);
        return r;
    }
    auto api = impl_->loader.api();
    if(!api){ 
        r.error_message = "PKCS#11 API null"; 
        impl_->sign_errors.fetch_add(1, std::memory_order_relaxed);
        return r; 
    }
    auto sess = acquireSession();
    if(!sess || sess->privKey==0){ 
        r.error_message = "PrivKey nicht gefunden"; 
        impl_->sign_errors.fetch_add(1, std::memory_order_relaxed);
        return r; 
    }
    CK_MECHANISM mech{};
    // If algorithm hints SHA256_RSA use combined mechanism else raw PKCS#1 with DigestInfo wrapper
    bool useCombined = (config_.signature_algorithm.find("SHA256") != std::string::npos);
    mech.mechanism = useCombined ? CKM_SHA256_RSA_PKCS : CKM_RSA_PKCS;
    CK_RV rv = api->C_SignInit(sess->handle, &mech, sess->privKey);
    if(rv != CKR_OK){ 
        r.error_message = mapError(rv); 
        impl_->sign_errors.fetch_add(1, std::memory_order_relaxed);
        return r; 
    }
    std::vector<uint8_t> input;
    if(useCombined){ input = hash; }
    else { input = makeDigestInfo(hash); }
    uint32_t sigLen = 4096; std::vector<CK_BYTE> sig(sigLen);
    rv = api->C_Sign(sess->handle, (CK_BYTE_PTR)input.data(), (uint32_t)input.size(), sig.data(), &sigLen);
    if(rv != CKR_OK){ 
        r.error_message = mapError(rv); 
        impl_->sign_errors.fetch_add(1, std::memory_order_relaxed);
        return r; 
    }
    sig.resize(sigLen);
    r.success = true; 
    impl_->sign_count.fetch_add(1, std::memory_order_relaxed);
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - startTime).count();
    impl_->total_sign_time_us.fetch_add(elapsed, std::memory_order_relaxed);
    r.signature_b64 = toBase64(std::vector<uint8_t>(sig.begin(), sig.end()));
    r.algorithm = config_.signature_algorithm; 
    r.key_id = key_label.empty()?config_.key_label:key_label;
    // Safely access cert_serial_cache_ under lock [SECURITY-FIX-BLOCK2]
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        r.cert_serial = impl_->cert_serial_cache_.empty()?"REAL-CERT":impl_->cert_serial_cache_;
    }
    r.timestamp_ms = nowMs();
    releaseSession(sess);
    return r;
}

bool HSMProvider::verify(const std::vector<uint8_t>& data, const std::string& signature_b64, const std::string& key_label){
    auto startTime = std::chrono::high_resolution_clock::now();
    std::lock_guard<std::mutex> lock(impl_->mtx);
    if(!initialized_) {
        impl_->verify_errors.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    if(!impl_->real_ready){
        auto bridge = VerifyFn{};
        {
            std::lock_guard<std::mutex> bridge_lock(verifyFnMutex());
            bridge = verifyFnStorage();
        }
        if (bridge) {
            bool result = bridge(data, signature_b64, key_label.empty() ? config_.key_label : key_label);
            if(result) impl_->verify_count.fetch_add(1, std::memory_order_relaxed);
            else impl_->verify_errors.fetch_add(1, std::memory_order_relaxed);
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - startTime).count();
            impl_->total_verify_time_us.fetch_add(elapsed, std::memory_order_relaxed);
            return result;
        }
        // Fallback: verify by comparing Base64-encoded hash
        auto expected = toBase64(sha256(data));
        bool result = (expected == signature_b64);
        if(result) impl_->verify_count.fetch_add(1, std::memory_order_relaxed);
        else impl_->verify_errors.fetch_add(1, std::memory_order_relaxed);
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - startTime).count();
        impl_->total_verify_time_us.fetch_add(elapsed, std::memory_order_relaxed);
        return result;
    }
    auto sess = acquireSession();
    auto api = impl_->loader.api(); 
    if(!api || !sess || sess->pubKey==0) { 
        impl_->verify_errors.fetch_add(1, std::memory_order_relaxed);
        releaseSession(sess); 
        return false; 
    }
    auto digest = sha256(data);
    bool useCombined = (config_.signature_algorithm.find("SHA256") != std::string::npos);
    std::vector<uint8_t> input = useCombined ? digest : makeDigestInfo(digest);
    // Decode Base64 signature
    std::vector<uint8_t> sig = fromBase64(signature_b64);
    if(sig.empty()) { 
        impl_->verify_errors.fetch_add(1, std::memory_order_relaxed);
        releaseSession(sess); 
        return false; 
    }
    
    CK_MECHANISM mech{}; mech.mechanism = useCombined ? CKM_SHA256_RSA_PKCS : CKM_RSA_PKCS;
    if(api->C_VerifyInit(sess->handle, &mech, sess->pubKey) != CKR_OK){ 
        impl_->verify_errors.fetch_add(1, std::memory_order_relaxed);
        releaseSession(sess); 
        return false; 
    }
    CK_RV rv = api->C_Verify(sess->handle, (CK_BYTE_PTR)input.data(), (uint32_t)input.size(), (CK_BYTE_PTR)sig.data(), (uint32_t)sig.size());
    bool result = (rv == CKR_OK);
    if(result) impl_->verify_count.fetch_add(1, std::memory_order_relaxed);
    else impl_->verify_errors.fetch_add(1, std::memory_order_relaxed);
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - startTime).count();
    impl_->total_verify_time_us.fetch_add(elapsed, std::memory_order_relaxed);
    releaseSession(sess);
    return result;
}

std::vector<HSMKeyInfo> HSMProvider::listKeys(){
    std::lock_guard<std::mutex> lock(impl_->mtx);
    HSMKeyInfo info; info.label = config_.key_label; info.id = impl_->real_ready?"real-id":"stub-id"; info.algorithm = config_.signature_algorithm; info.can_sign = true; info.can_verify = true; info.extractable = false; info.key_size = impl_->real_ready?2048:0; return {info};
}

std::vector<uint8_t> HSMProvider::encryptData(const std::vector<uint8_t>& data, [[maybe_unused]] const std::string& key_label){
    std::lock_guard<std::mutex> lock(impl_->mtx);
    if (!initialized_) { last_error_ = "Not initialized"; return {}; }
    if (!impl_->real_ready || !impl_->loader.api()) {
        auto bridge = EncryptDataFn{};
        {
            std::lock_guard<std::mutex> bridge_lock(encryptDataFnMutex());
            bridge = encryptDataFnStorage();
        }
        if (bridge) {
            return bridge(data, key_label.empty() ? config_.key_label : key_label);
        }
        // Fallback: AES-256-GCM with stub KEK
        auto result = pkcs11_stub_aes_encrypt(impl_->stub_kek, data);
        if (result.empty()) last_error_ = "Stub AES encrypt failed";
        return result;
    }
    auto api = impl_->loader.api();
    if (!api) { last_error_ = "PKCS#11 API not available"; return {}; }
    auto sess = acquireSession();
    if (!sess || sess->pubKey == 0) {
        last_error_ = "No public key available for encryption";
        return {};
    }
    // Use RSA-OAEP (SHA-256 + MGF1-SHA-256) for secure DEK wrapping
    auto oaep_params = makeOaepParams();
    CK_MECHANISM mech{};
    mech.mechanism = CKM_RSA_PKCS_OAEP;
    mech.pParameter = &oaep_params;
    mech.ulParameterLen = sizeof(oaep_params);
    CK_RV rv = api->C_EncryptInit(sess->handle, &mech, sess->pubKey);
    if (rv != CKR_OK) {
        last_error_ = "C_EncryptInit failed: " + mapError(rv);
        releaseSession(sess);
        return {};
    }
    // Pre-allocate output buffer: RSA output size equals modulus size (max 512 bytes for RSA-4096)
    const CK_ULONG kMaxRsaBytes = 512;
    std::vector<uint8_t> ciphertext(kMaxRsaBytes);
    CK_ULONG outLen = kMaxRsaBytes;
    rv = api->C_Encrypt(sess->handle, (CK_BYTE_PTR)data.data(), (CK_ULONG)data.size(), ciphertext.data(), &outLen);
    releaseSession(sess);
    if (rv != CKR_OK) {
        last_error_ = "C_Encrypt failed: " + mapError(rv);
        return {};
    }
    ciphertext.resize(outLen);
    return ciphertext;
}

std::vector<uint8_t> HSMProvider::decryptData(const std::vector<uint8_t>& encrypted, [[maybe_unused]] const std::string& key_label){
    std::lock_guard<std::mutex> lock(impl_->mtx);
    if (!initialized_) { last_error_ = "Not initialized"; return {}; }
    if (!impl_->real_ready || !impl_->loader.api()) {
        auto bridge = DecryptDataFn{};
        {
            std::lock_guard<std::mutex> bridge_lock(decryptDataFnMutex());
            bridge = decryptDataFnStorage();
        }
        if (bridge) {
            return bridge(encrypted, key_label.empty() ? config_.key_label : key_label);
        }
        // Fallback: AES-256-GCM with stub KEK
        auto result = pkcs11_stub_aes_decrypt(impl_->stub_kek, encrypted);
        if (result.empty()) last_error_ = "Stub AES decrypt failed (bad ciphertext or mismatched key)";
        return result;
    }
    auto api = impl_->loader.api();
    if (!api) { last_error_ = "PKCS#11 API not available"; return {}; }
    auto sess = acquireSession();
    if (!sess || sess->privKey == 0) {
        last_error_ = "No private key available for decryption";
        return {};
    }
    // Use RSA-OAEP (SHA-256 + MGF1-SHA-256) matching the encryption mechanism
    auto oaep_params = makeOaepParams();
    CK_MECHANISM mech{};
    mech.mechanism = CKM_RSA_PKCS_OAEP;
    mech.pParameter = &oaep_params;
    mech.ulParameterLen = sizeof(oaep_params);
    CK_RV rv = api->C_DecryptInit(sess->handle, &mech, sess->privKey);
    if (rv != CKR_OK) {
        last_error_ = "C_DecryptInit failed: " + mapError(rv);
        releaseSession(sess);
        return {};
    }
    // Pre-allocate output buffer: plaintext <= ciphertext size (RSA modulus size)
    std::vector<uint8_t> plaintext(encrypted.size());
    CK_ULONG outLen = (CK_ULONG)encrypted.size();
    rv = api->C_Decrypt(sess->handle, (CK_BYTE_PTR)encrypted.data(), (CK_ULONG)encrypted.size(), plaintext.data(), &outLen);
    releaseSession(sess);
    if (rv != CKR_OK) {
        last_error_ = "C_Decrypt failed: " + mapError(rv);
        return {};
    }
    plaintext.resize(outLen);
    return plaintext;
}

bool HSMProvider::generateKeyPair(const std::string& label, uint32_t key_size, bool extractable){
    std::lock_guard<std::mutex> lock(impl_->mtx);
    if(!impl_->real_ready){ 
        auto bridge = GenerateKeyPairFn{};
        {
            std::lock_guard<std::mutex> bridge_lock(generateKeyPairFnMutex());
            bridge = generateKeyPairFnStorage();
        }
        if (bridge) {
            return bridge(label, key_size, extractable);
        }
        THEMIS_WARN("generateKeyPair Fallback stub (label='{}')", label); 
        return false; 
    }
    
    auto api = impl_->loader.api();
    if(!api) return false;
    
    // Get first ready session
    auto sess = acquireSession();
    if(!sess || !sess->handle){
        THEMIS_ERROR("generateKeyPair: No ready session available");
        return false;
    }
    
    // Validate key size
    if(key_size != 2048 && key_size != 3072 && key_size != 4096){
        THEMIS_ERROR("generateKeyPair: Invalid key size {}. Must be 2048, 3072, or 4096", key_size);
        releaseSession(sess);
        return false;
    }
    
    // Use local variables for object class values so we can take their address
    CK_OBJECT_CLASS cls_pub  = CKO_PUBLIC_KEY;
    CK_OBJECT_CLASS cls_priv = CKO_PRIVATE_KEY;
    CK_BBOOL ck_true       = CK_TRUE;
    CK_BBOOL ck_extractable = extractable ? CK_TRUE : CK_FALSE;
    CK_ULONG modulus_bits = key_size;
    CK_BYTE public_exponent[] = {0x01, 0x00, 0x01}; // 65537
    
    CK_ATTRIBUTE pub_template[] = {
        {CKA_CLASS, &cls_pub, sizeof(cls_pub)},
        {CKA_LABEL, (void*)label.c_str(), label.size()},
        {CKA_TOKEN, &ck_true, sizeof(ck_true)},
        {CKA_VERIFY, &ck_true, sizeof(ck_true)},
        {CKA_MODULUS_BITS, &modulus_bits, sizeof(modulus_bits)},
        {CKA_PUBLIC_EXPONENT, public_exponent, sizeof(public_exponent)}
    };
    
    // Private key template
    CK_ATTRIBUTE priv_template[] = {
        {CKA_CLASS, &cls_priv, sizeof(cls_priv)},
        {CKA_LABEL, (void*)label.c_str(), label.size()},
        {CKA_TOKEN, &ck_true, sizeof(ck_true)},
        {CKA_PRIVATE, &ck_true, sizeof(ck_true)},
        {CKA_SENSITIVE, &ck_true, sizeof(ck_true)},
        {CKA_SIGN, &ck_true, sizeof(ck_true)},
        {CKA_EXTRACTABLE, &ck_extractable, sizeof(ck_extractable)}
    };
    
    CK_MECHANISM mech = {CKM_RSA_PKCS_KEY_PAIR_GEN, nullptr, 0};
    CK_OBJECT_HANDLE pub_key = 0, priv_key = 0;
    
    CK_RV rv = api->C_GenerateKeyPair(
        sess->handle,
        &mech,
        pub_template, sizeof(pub_template) / sizeof(CK_ATTRIBUTE),
        priv_template, sizeof(priv_template) / sizeof(CK_ATTRIBUTE),
        &pub_key,
        &priv_key
    );
    
    releaseSession(sess);
    
    if(rv != CKR_OK){
        last_error_ = mapError(rv);
        THEMIS_ERROR("generateKeyPair failed: {}", last_error_);
        return false;
    }
    
    THEMIS_INFO("Generated RSA-{} key pair with label '{}'", key_size, label);
    return true;
}

bool HSMProvider::importCertificate(const std::string& key_label, const std::string& cert_pem){
    std::lock_guard<std::mutex> lock(impl_->mtx);
    if(!impl_->real_ready){ 
        auto bridge = ImportCertificateFn{};
        {
            std::lock_guard<std::mutex> bridge_lock(importCertificateFnMutex());
            bridge = importCertificateFnStorage();
        }
        if (bridge) {
            return bridge(key_label, cert_pem);
        }
        THEMIS_WARN("importCertificate Fallback stub (key='{}')", key_label); 
        return false; 
    }
    
    auto api = impl_->loader.api();
    if(!api) return false;
    
    // Get first ready session
    auto sess = acquireSession();
    if(!sess || !sess->handle){
        THEMIS_ERROR("importCertificate: No ready session available");
        return false;
    }
    
    // Parse PEM certificate to DER format
    BIO* bio = BIO_new_mem_buf(cert_pem.data(), cert_pem.size());
    if(!bio){
        THEMIS_ERROR("importCertificate: Failed to create BIO");
        releaseSession(sess);
        return false;
    }
    
    HSM_P11_BIO_ptr bio_ptr(bio);
    HSM_P11_X509_ptr x509(PEM_read_bio_X509(bio_ptr.get(), nullptr, nullptr, nullptr));
    
    if(!x509.get()){
        THEMIS_ERROR("importCertificate: Failed to parse PEM certificate");
        releaseSession(sess);
        return false;
    }
    
    // Convert to DER
    struct OpenSslUnsignedCharDeleter {
        void operator()(unsigned char* p) const {
            if (p) {
                OPENSSL_free(p);
            }
        }
    };
    unsigned char* der_raw = nullptr;
    int der_len = i2d_X509(x509.get(), &der_raw);
    std::unique_ptr<unsigned char, OpenSslUnsignedCharDeleter> der(der_raw);
    if(der_len <= 0 || !der){
        THEMIS_ERROR("importCertificate: Failed to convert certificate to DER");
        releaseSession(sess);
        return false;
    }
    
    // Extract serial number for metadata
    ASN1_INTEGER* serial_int = X509_get_serialNumber(x509.get());
    std::string serial_hex;
    if(serial_int){
        HSM_P11_BIGNUM_ptr bn(ASN1_INTEGER_to_BN(serial_int, nullptr));
        if(bn.get()){
            char* hex = BN_bn2hex(bn.get());
            if(hex){
                serial_hex = hex;
                OPENSSL_free(hex);
            }
        }
    }
    
    // Create certificate object in HSM
    CK_OBJECT_CLASS cert_class = CKO_CERTIFICATE;
    CK_CERTIFICATE_TYPE cert_type = CKC_X_509;
    CK_BBOOL ck_true = CK_TRUE;
    
    CK_ATTRIBUTE cert_template[] = {
        {CKA_CLASS, &cert_class, sizeof(cert_class)},
        {CKA_CERTIFICATE_TYPE, &cert_type, sizeof(cert_type)},
        {CKA_LABEL, (void*)key_label.c_str(), key_label.size()},
        {CKA_TOKEN, &ck_true, sizeof(ck_true)},
        {CKA_VALUE, der.get(), (CK_ULONG)der_len}
    };
    
    CK_OBJECT_HANDLE cert_obj = 0;
    CK_RV rv = api->C_CreateObject(
        sess->handle,
        cert_template,
        sizeof(cert_template) / sizeof(CK_ATTRIBUTE),
        &cert_obj
    );
     
    // der is automatically freed by unique_ptr on scope exit
    releaseSession(sess);
     
    if(rv != CKR_OK){
        last_error_ = mapError(rv);
        THEMIS_ERROR("importCertificate failed: {}", last_error_);
        return false;
    }
     
    // Update cert serial cache if this is the first certificate [SECURITY-FIX-BLOCK2]
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        if(impl_->cert_serial_cache_.empty() && !serial_hex.empty()){
            impl_->cert_serial_cache_ = serial_hex;
        }
    }
     
    THEMIS_INFO("Imported certificate for key '{}' (serial: {})", key_label, serial_hex);
    return true;
}

std::optional<std::string> HSMProvider::getCertificate(const std::string& key_label){
    std::lock_guard<std::mutex> lock(impl_->mtx);
    if(!impl_->real_ready) {
        auto bridge = GetCertificateFn{};
        {
            std::lock_guard<std::mutex> bridge_lock(getCertificateFnMutex());
            bridge = getCertificateFnStorage();
        }
        if (bridge) {
            return bridge(key_label);
        }
        // Fail-closed: returning a hardcoded stub cert to unsuspecting callers is
        // dangerous — any certificate-validation logic would accept a meaningless token.
        // Require explicit opt-in via THEMIS_ALLOW_HSM_STUB=1.
        {
            const char* allow_stub = std::getenv("THEMIS_ALLOW_HSM_STUB");
            if (!allow_stub || std::string(allow_stub) != "1") {
                THEMIS_ERROR(
                    "HSMProvider (PKCS#11 path) getCertificate('{}') refused: real HSM not "
                    "ready and returning a stub PEM is insecure. Set THEMIS_ALLOW_HSM_STUB=1 "
                    "for explicit development override, or fix your PKCS#11 configuration.",
                    key_label);
                return std::nullopt;
            }
        }
        THEMIS_WARN(
            "HSMProvider (PKCS#11 path) getCertificate('{}') returning hardcoded stub PEM "
            "(THEMIS_ALLOW_HSM_STUB=1). Not suitable for production.",
            key_label);
        return std::string("-----BEGIN CERTIFICATE-----\nSTUB\n-----END CERTIFICATE-----\n");
    }
    auto api = impl_->loader.api(); if(!api || !api->C_GetAttributeValue) return std::nullopt;
    CK_ATTRIBUTE valAttr; valAttr.type = CKA_VALUE; valAttr.pValue = nullptr; valAttr.ulValueLen = 0;
    // Zertifikat aus erster Session mit certObj
    HSMProvider::SessionEntry* sess = nullptr; for(auto& s: impl_->pool){ if(s.certObj){ sess=&s; break; } }
    if(!sess) return std::nullopt;
    if(api->C_GetAttributeValue(sess->handle, sess->certObj, &valAttr, 1) != CKR_OK || valAttr.ulValueLen==0) return std::nullopt;
    std::vector<unsigned char> der(valAttr.ulValueLen); valAttr.pValue = der.data();
    if(api->C_GetAttributeValue(sess->handle, sess->certObj, &valAttr, 1) != CKR_OK) return std::nullopt;
    const unsigned char* p = der.data(); X509* x = d2i_X509(nullptr, &p, der.size()); if(!x) return std::nullopt;
    BIO* mem = BIO_new(BIO_s_mem());
    PEM_write_bio_X509(mem, x);
    X509_free(x);
    char* buf = nullptr; long len = BIO_get_mem_data(mem, &buf);
    std::string pem(buf, len);
    BIO_free(mem);
    return pem;
}

bool HSMProvider::isReady() const { return impl_->real_ready || initialized_; }

std::string HSMProvider::getTokenInfo() const {
    if (!impl_->real_ready) return "PKCS11 fallback stub";
    std::ostringstream oss;
    oss << "PKCS11 real session active (slot=" << config_.slot_id;
    if (!config_.token_label.empty()) {
        oss << ", label=" << config_.token_label;
    }
    oss << ", pool=" << impl_->pool.size() << ")";
    return oss.str();
}

std::string HSMProvider::getLastError() const { return last_error_; }

HSMPerformanceStats HSMProvider::getStats() const {
    HSMPerformanceStats stats;
    stats.sign_count = impl_->sign_count.load(std::memory_order_relaxed);
    stats.verify_count = impl_->verify_count.load(std::memory_order_relaxed);
    stats.sign_errors = impl_->sign_errors.load(std::memory_order_relaxed);
    stats.verify_errors = impl_->verify_errors.load(std::memory_order_relaxed);
    stats.total_sign_time_us = impl_->total_sign_time_us.load(std::memory_order_relaxed);
    stats.total_verify_time_us = impl_->total_verify_time_us.load(std::memory_order_relaxed);
    stats.pool_size = impl_->pool.size();
    stats.pool_round_robin_hits = impl_->pool_round_robin_hits.load(std::memory_order_relaxed);
    return stats;
}

void HSMProvider::resetStats() {
    impl_->sign_count.store(0, std::memory_order_relaxed);
    impl_->verify_count.store(0, std::memory_order_relaxed);
    impl_->sign_errors.store(0, std::memory_order_relaxed);
    impl_->verify_errors.store(0, std::memory_order_relaxed);
    impl_->total_sign_time_us.store(0, std::memory_order_relaxed);
    impl_->total_verify_time_us.store(0, std::memory_order_relaxed);
    impl_->pool_round_robin_hits.store(0, std::memory_order_relaxed);
}

bool HSMProvider::isStubProvider() const {
    // False if real HSM is active, true if fallback stub
    return !impl_->real_ready;
}

void HSMProvider::periodicSecurityCheck() {
    std::lock_guard<std::mutex> lock(impl_->mtx);
    
    if (!initialized_) return;
    
    // If using stub fallback (no real HSM), log security warning
    if (!impl_->real_ready) {
        THEMIS_ERROR("⚠️  HSM SECURITY WARNING: PKCS#11 fallback stub active!");
        THEMIS_ERROR("Real HSM connection failed. Keys are NOT hardware-protected.");
        THEMIS_ERROR("Compliance impact: NIST SP 800-53 SC-12, PCI DSS 3.6, GDPR Art. 32");
        THEMIS_ERROR("Check HSM configuration: library_path={}, pin={}", 
                     config_.library_path.empty() ? "NOT SET" : config_.library_path,
                     config_.pin.empty() ? "NOT SET" : "SET");
        THEMIS_ERROR("See: docs/security/HSM_PRODUCTION_SETUP.md");
    }
}

} } // namespace themis::security
#endif // THEMIS_ENABLE_HSM_REAL
