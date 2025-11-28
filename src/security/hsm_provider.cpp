// HSMProvider implementation: Stub by default, PKCS#11 when THEMIS_ENABLE_PKCS11 is defined.
// Provides deterministic, non-cryptographic operations sufficient for tests in stub mode.
// When PKCS#11 is enabled, integrates with hardware security modules via PKCS#11 library.

#include "security/hsm_provider.h"
#include "utils/logger.h"

#ifdef THEMIS_ENABLE_PKCS11
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <cstring>

// PKCS#11 definitions (minimal fallback if headers unavailable)
#ifdef __linux__
#include <pkcs11/pkcs11.h>
#else
typedef unsigned char CK_BYTE;
typedef CK_BYTE CK_BBOOL;
typedef unsigned long int CK_ULONG;
typedef unsigned long int CK_FLAGS;
typedef CK_ULONG CK_RV;
typedef CK_ULONG CK_SESSION_HANDLE;
typedef CK_ULONG CK_OBJECT_HANDLE;
typedef CK_ULONG CK_SLOT_ID;
typedef void* CK_VOID_PTR;

#define CKR_OK 0x00000000UL
#define CKF_SERIAL_SESSION 0x00000004UL
#define CKF_RW_SESSION 0x00000002UL
#define CKU_USER 1

struct CK_MECHANISM {
    CK_ULONG mechanism;
    void* pParameter;
    CK_ULONG ulParameterLen;
};

struct CK_ATTRIBUTE {
    CK_ULONG type;
    void* pValue;
    CK_ULONG ulValueLen;
};

struct CK_C_INITIALIZE_ARGS {
    void* reserved;
};
#endif

#endif // THEMIS_ENABLE_PKCS11

#include <sstream>
#include <chrono>

namespace themis { namespace security {

// ============================================================================
// Helper functions
// ============================================================================

static std::string to_hex(const std::vector<uint8_t>& data) {
    static const char* d = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (auto b : data) {
        out.push_back(d[(b >> 4) & 0xF]);
        out.push_back(d[b & 0xF]);
    }
    return out;
}

static std::string pseudo_b64(const std::vector<uint8_t>& data) {
    return std::string("hex:") + to_hex(data);
}

#ifdef THEMIS_ENABLE_PKCS11

// Base64 encode/decode for signature handling
static std::string base64_encode(const std::vector<uint8_t>& input) {
    BIO* bmem = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, bmem);
    BIO_write(b64, input.data(), static_cast<int>(input.size()));
    BIO_flush(b64);
    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);
    std::string result(bptr->data, bptr->length);
    BIO_free_all(b64);
    return result;
}

static std::vector<uint8_t> base64_decode(const std::string& encoded_string) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    size_t in_len = encoded_string.size();
    int i = 0;
    int in_ = 0;
    uint8_t char_array_4[4], char_array_3[3];
    std::vector<uint8_t> ret;
    
    while (in_len-- && (encoded_string[in_] != '=') && 
           (isalnum(encoded_string[in_]) || (encoded_string[in_] == '+') || (encoded_string[in_] == '/'))) {
        char_array_4[i++] = encoded_string[in_];
        in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = static_cast<uint8_t>(base64_chars.find(char_array_4[i]));
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (i = 0; i < 3; i++)
                ret.push_back(char_array_3[i]);
            i = 0;
        }
    }
    
    if (i) {
        for (int j = i; j < 4; j++)
            char_array_4[j] = 0;
        
        for (int j = 0; j < 4; j++)
            char_array_4[j] = static_cast<uint8_t>(base64_chars.find(char_array_4[j]));
        
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        
        for (int j = 0; j < i - 1; j++)
            ret.push_back(char_array_3[j]);
    }
    
    return ret;
}

// PKCS#11 function pointer types
typedef CK_RV (*CK_C_Initialize_t)(CK_VOID_PTR);
typedef CK_RV (*CK_C_Finalize_t)(CK_VOID_PTR);
typedef CK_RV (*CK_C_GetSlotList_t)(CK_BBOOL, CK_SLOT_ID*, CK_ULONG*);
typedef CK_RV (*CK_C_OpenSession_t)(CK_SLOT_ID, CK_FLAGS, CK_VOID_PTR, void*, CK_SESSION_HANDLE*);
typedef CK_RV (*CK_C_CloseSession_t)(CK_SESSION_HANDLE);
typedef CK_RV (*CK_C_Login_t)(CK_SESSION_HANDLE, CK_ULONG, CK_BYTE*, CK_ULONG);
typedef CK_RV (*CK_C_Logout_t)(CK_SESSION_HANDLE);
typedef CK_RV (*CK_C_SignInit_t)(CK_SESSION_HANDLE, CK_MECHANISM*, CK_OBJECT_HANDLE);
typedef CK_RV (*CK_C_Sign_t)(CK_SESSION_HANDLE, CK_BYTE*, CK_ULONG, CK_BYTE*, CK_ULONG*);
typedef CK_RV (*CK_C_FindObjectsInit_t)(CK_SESSION_HANDLE, CK_ATTRIBUTE*, CK_ULONG);
typedef CK_RV (*CK_C_FindObjects_t)(CK_SESSION_HANDLE, CK_OBJECT_HANDLE*, CK_ULONG, CK_ULONG*);
typedef CK_RV (*CK_C_FindObjectsFinal_t)(CK_SESSION_HANDLE);

#endif // THEMIS_ENABLE_PKCS11

// ============================================================================
// HSMProvider::Impl
// ============================================================================

#ifdef THEMIS_ENABLE_PKCS11

class HSMProvider::Impl {
public:
    void* library_handle = nullptr;
    CK_SESSION_HANDLE session = 0;
    bool session_open = false;
    bool logged_in = false;
    
    // PKCS#11 function pointers
    CK_C_Initialize_t C_Initialize = nullptr;
    CK_C_Finalize_t C_Finalize = nullptr;
    CK_C_GetSlotList_t C_GetSlotList = nullptr;
    CK_C_OpenSession_t C_OpenSession = nullptr;
    CK_C_CloseSession_t C_CloseSession = nullptr;
    CK_C_Login_t C_Login = nullptr;
    CK_C_Logout_t C_Logout = nullptr;
    CK_C_SignInit_t C_SignInit = nullptr;
    CK_C_Sign_t C_Sign = nullptr;
    CK_C_FindObjectsInit_t C_FindObjectsInit = nullptr;
    CK_C_FindObjects_t C_FindObjects = nullptr;
    CK_C_FindObjectsFinal_t C_FindObjectsFinal = nullptr;
    
    bool loadLibrary(const std::string& path, std::string& error) {
#if defined(_WIN32)
        HMODULE h = LoadLibraryA(path.c_str());
        if (!h) {
            error = "Failed to load PKCS#11 library (LoadLibraryA)";
            return false;
        }
        library_handle = reinterpret_cast<void*>(h);

        auto load = [&](auto& fn, const char* name) -> bool {
            FARPROC p = GetProcAddress(h, name);
            if (!p) {
                error = std::string("Failed to load function ") + name;
                FreeLibrary(h);
                library_handle = nullptr;
                return false;
            }
            fn = reinterpret_cast<decltype(fn)>(p);
            return true;
        };

        if (!load(C_Initialize, "C_Initialize")) return false;
        if (!load(C_Finalize, "C_Finalize")) return false;
        if (!load(C_GetSlotList, "C_GetSlotList")) return false;
        if (!load(C_OpenSession, "C_OpenSession")) return false;
        if (!load(C_CloseSession, "C_CloseSession")) return false;
        if (!load(C_Login, "C_Login")) return false;
        if (!load(C_Logout, "C_Logout")) return false;
        if (!load(C_SignInit, "C_SignInit")) return false;
        if (!load(C_Sign, "C_Sign")) return false;
        if (!load(C_FindObjectsInit, "C_FindObjectsInit")) return false;
        if (!load(C_FindObjects, "C_FindObjects")) return false;
        if (!load(C_FindObjectsFinal, "C_FindObjectsFinal")) return false;

        return true;
#else
        library_handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!library_handle) {
            error = std::string("Failed to load PKCS#11 library: ") + dlerror();
            return false;
        }

        #define LOAD_FUNC(name) \
            name = reinterpret_cast<decltype(name)>(dlsym(library_handle, #name)); \
            if (!name) { \
                error = std::string("Failed to load function ") + #name; \
                dlclose(library_handle); \
                library_handle = nullptr; \
                return false; \
            }

        LOAD_FUNC(C_Initialize);
        LOAD_FUNC(C_Finalize);
        LOAD_FUNC(C_GetSlotList);
        LOAD_FUNC(C_OpenSession);
        LOAD_FUNC(C_CloseSession);
        LOAD_FUNC(C_Login);
        LOAD_FUNC(C_Logout);
        LOAD_FUNC(C_SignInit);
        LOAD_FUNC(C_Sign);
        LOAD_FUNC(C_FindObjectsInit);
        LOAD_FUNC(C_FindObjects);
        LOAD_FUNC(C_FindObjectsFinal);

        #undef LOAD_FUNC

        return true;
#endif
    }
    
    ~Impl() {
        if (logged_in && C_Logout) {
            C_Logout(session);
        }
        if (session_open && C_CloseSession) {
            C_CloseSession(session);
        }
        if (library_handle && C_Finalize) {
            C_Finalize(nullptr);
        }
        if (library_handle) {
#if defined(_WIN32)
            FreeLibrary(reinterpret_cast<HMODULE>(library_handle));
#else
            dlclose(library_handle);
#endif
        }
    }
};

#else

// Stub implementation (no PKCS#11)
class HSMProvider::Impl {
    // Empty stub
};

#endif // THEMIS_ENABLE_PKCS11

// ============================================================================
// HSMProvider implementation
// ============================================================================

HSMProvider::HSMProvider(HSMConfig config)
    : impl_(std::make_unique<Impl>()), config_(std::move(config)) {}

HSMProvider::~HSMProvider() = default;
HSMProvider::HSMProvider(HSMProvider&&) noexcept = default;
HSMProvider& HSMProvider::operator=(HSMProvider&&) noexcept = default;

bool HSMProvider::initialize() {
    if (initialized_) {
        THEMIS_WARN("HSMProvider already initialized");
        return true;
    }

#ifdef THEMIS_ENABLE_PKCS11
    // Load PKCS#11 library
    if (!impl_->loadLibrary(config_.library_path, last_error_)) {
        THEMIS_ERROR("HSM initialization failed: {}", last_error_);
        return false;
    }
    
    // Initialize PKCS#11
    CK_C_INITIALIZE_ARGS init_args = {nullptr};
    CK_RV rv = impl_->C_Initialize(&init_args);
    if (rv != CKR_OK) {
        last_error_ = "C_Initialize failed with code " + std::to_string(rv);
        THEMIS_ERROR("HSM: {}", last_error_);
        return false;
    }
    
    // Open session
    rv = impl_->C_OpenSession(
        config_.slot_id,
        CKF_SERIAL_SESSION | CKF_RW_SESSION,
        nullptr,
        nullptr,
        &impl_->session
    );
    
    if (rv != CKR_OK) {
        last_error_ = "C_OpenSession failed with code " + std::to_string(rv);
        THEMIS_ERROR("HSM: {}", last_error_);
        impl_->C_Finalize(nullptr);
        return false;
    }
    
    impl_->session_open = true;
    
    // Login
    rv = impl_->C_Login(
        impl_->session,
        CKU_USER,
        reinterpret_cast<CK_BYTE*>(const_cast<char*>(config_.pin.c_str())),
        static_cast<CK_ULONG>(config_.pin.length())
    );
    
    if (rv != CKR_OK) {
        last_error_ = "C_Login failed with code " + std::to_string(rv);
        THEMIS_ERROR("HSM: {}", last_error_);
        impl_->C_CloseSession(impl_->session);
        impl_->C_Finalize(nullptr);
        return false;
    }
    
    impl_->logged_in = true;
    initialized_ = true;
    THEMIS_INFO("HSMProvider PKCS#11 initialized (label='{}')", config_.key_label);
#else
    // Stub mode
    initialized_ = true;
    THEMIS_INFO("HSMProvider stub initialized (label='{}')", config_.key_label);
#endif

    return true;
}

void HSMProvider::finalize() {
    initialized_ = false;
    impl_.reset();
    THEMIS_INFO("HSMProvider finalized");
}

HSMSignatureResult HSMProvider::sign(const std::vector<uint8_t>& data, const std::string& key_label) {
    return signHash(data, key_label); // treat data as pre-hash in stub
}

HSMSignatureResult HSMProvider::signHash(const std::vector<uint8_t>& hash, const std::string& key_label) {
    HSMSignatureResult r;
    if (!initialized_) {
        r.error_message = "HSM not initialized";
        return r;
    }

#ifdef THEMIS_ENABLE_PKCS11
    // PKCS#11 signing (not fully implemented; placeholder)
    THEMIS_WARN("HSM PKCS#11 signHash() - using stub signature for now");
    r.success = true;
    r.signature_b64 = base64_encode(hash);
    r.algorithm = config_.signature_algorithm;
    r.key_id = key_label.empty() ? config_.key_label : key_label;
    r.cert_serial = "PKCS11-CERT";
    r.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
#else
    // Stub signature
    r.success = true;
    r.signature_b64 = pseudo_b64(hash);
    r.algorithm = config_.signature_algorithm;
    r.key_id = key_label.empty() ? config_.key_label : key_label;
    r.cert_serial = "STUB-CERT";
    r.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
#endif

    return r;
}

bool HSMProvider::verify(const std::vector<uint8_t>& data,
                         const std::string& signature_b64,
                         const std::string& key_label) {
    (void)key_label;
    if (!initialized_) {
        THEMIS_ERROR("HSM not initialized");
        return false;
    }

#ifdef THEMIS_ENABLE_PKCS11
    // Hash data
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    SHA256(data.data(), data.size(), hash.data());
    
    // Decode signature
    auto signature = base64_decode(signature_b64);
    
    // Stub verification - in production, would use C_VerifyInit/C_Verify
    THEMIS_WARN("HSM PKCS#11 verify() - using stub comparison");
    std::string hash_b64 = base64_encode(hash);
    return hash_b64 == signature_b64;
#else
    // Stub verification
    auto expected = pseudo_b64(data);
    bool ok = (expected == signature_b64);
    THEMIS_DEBUG("HSMProvider stub verify key='{}' ok={}", key_label.empty() ? config_.key_label : key_label, ok);
    return ok;
#endif
}

std::vector<HSMKeyInfo> HSMProvider::listKeys() {
    HSMKeyInfo info;
    info.label = config_.key_label;
    info.id = "stub-id";
    info.algorithm = config_.signature_algorithm;
    info.can_sign = true;
    info.can_verify = true;
    info.extractable = false;
    info.key_size = 0;
    return {info};
}

bool HSMProvider::generateKeyPair(const std::string& label,
                                  uint32_t key_size,
                                  bool extractable) {
    (void)extractable;
    if (!initialized_) {
        THEMIS_ERROR("HSM not initialized");
        return false;
    }

#ifdef THEMIS_ENABLE_PKCS11
    THEMIS_WARN("HSM PKCS#11 generateKeyPair() - not implemented");
    THEMIS_INFO("To generate keys, use: pkcs11-tool --module {} --login --keypairgen --key-type RSA:{} --label '{}'",
                config_.library_path, key_size, label);
#else
    THEMIS_WARN("HSMProvider stub generateKeyPair ignored (label='{}')", label);
#endif

    return false;
}

bool HSMProvider::importCertificate(const std::string& key_label,
                                    const std::string& cert_pem) {
    (void)key_label;
    (void)cert_pem;
    if (!initialized_) {
        THEMIS_ERROR("HSM not initialized");
        return false;
    }

#ifdef THEMIS_ENABLE_PKCS11
    THEMIS_WARN("HSM PKCS#11 importCertificate() - not implemented");
#else
    THEMIS_WARN("HSMProvider stub importCertificate ignored (key='{}')", key_label);
#endif

    return false;
}

std::optional<std::string> HSMProvider::getCertificate(const std::string& key_label) {
    (void)key_label;
    if (!initialized_) {
        THEMIS_ERROR("HSM not initialized");
        return std::nullopt;
    }

#ifdef THEMIS_ENABLE_PKCS11
    THEMIS_WARN("HSM PKCS#11 getCertificate() - not implemented");
    return std::nullopt;
#else
    return std::string("-----BEGIN CERTIFICATE-----\nSTUB\n-----END CERTIFICATE-----\n");
#endif
}

bool HSMProvider::isReady() const {
    return initialized_;
}

std::string HSMProvider::getTokenInfo() const {
    std::ostringstream oss;
#ifdef THEMIS_ENABLE_PKCS11
    oss << "HSM PKCS#11 label=" << config_.key_label << " ready=" << (initialized_ ? "true" : "false");
#else
    oss << "HSM STUB label=" << config_.key_label << " ready=" << (initialized_ ? "true" : "false");
#endif
    return oss.str();
}

std::string HSMProvider::getLastError() const {
    return last_error_;
}

// ============================================================================
// HSMPKIClient
// ============================================================================

HSMPKIClient::HSMPKIClient(HSMConfig config)
    : hsm_(std::make_unique<HSMProvider>(std::move(config))) {
    hsm_->initialize();
}

HSMPKIClient::~HSMPKIClient() {
    if (hsm_) hsm_->finalize();
}

HSMSignatureResult HSMPKIClient::sign(const std::vector<uint8_t>& data) {
    return hsm_->sign(data);
}

bool HSMPKIClient::verify(const std::vector<uint8_t>& data, const std::string& signature_b64) {
    return hsm_->verify(data, signature_b64);
}

std::optional<std::string> HSMPKIClient::getCertSerial() {
    return std::string("STUB-SERIAL");
}

bool HSMPKIClient::isReady() const {
    return hsm_->isReady();
}

} } // namespace themis::security
