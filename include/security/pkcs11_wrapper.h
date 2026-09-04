/**
 * @file pkcs11_wrapper.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// =============================================================================
// PKCS#11 C++ Wrapper Interface
// =============================================================================
//
// Provides RAII-based C++ wrappers over the raw PKCS#11 C API defined in
// pkcs11_minimal.h (or a vendor-supplied pkcs11.h when building with
// -DTHEMIS_USE_VENDOR_PKCS11=ON).
//
// Design goals:
//   1. RAII ownership for library handle, sessions, and login state.
//   2. Clean error reporting: CK_RV codes mapped to std::error_code and
//      human-readable strings via Pkcs11Error.
//   3. Thin helpers that forward to CK_FUNCTION_LIST pointers without
//      hiding the PKCS#11 semantics from the caller.
//   4. Header-only (no additional .cpp required).
//   5. No exceptions thrown internally; all operations return bool or
//      std::optional so callers choose their error-handling strategy.
//
// Typical usage:
// ```cpp
// #include "security/pkcs11_wrapper.h"
//
// using namespace themis::security::pkcs11;
//
// Pkcs11Library lib;
// if (!lib.load("/usr/lib/softhsm/libsofthsm2.so")) {
//     // handle error
// }
//
// Pkcs11Session session(lib.functions());
// if (!session.open(slotId, CKF_SERIAL_SESSION)) { ... }
// if (!session.login(CKU_USER, "1234"))           { ... }
//
// auto privKey = findObject(session, {{CKA_CLASS, CKO_PRIVATE_KEY},
//                                     {CKA_LABEL, "my-key"}});
// if (!privKey) { ... }
//
// auto sig = signData(session, *privKey, CKM_SHA256_RSA_PKCS, data);
// ```
//
// Thread safety:
//   Pkcs11Library is not thread-safe after construction (load() must be
//   called exactly once before any parallel use).  Pkcs11Session objects
//   must not be shared across threads; create one per thread.
//
// See: include/security/pkcs11_minimal.h
//      src/security/hsm_provider_pkcs11.cpp
// =============================================================================

#include "security/pkcs11_minimal.h"

#include <string>
#include <vector>
#include <optional>
#include <system_error>
#include <sstream>
#include <cstring>
#include <utility>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace themis {
namespace security {
namespace pkcs11 {

// =============================================================================
// Error utilities
// =============================================================================

/**
 * @brief Map a PKCS#11 CK_RV return value to a human-readable string.
 *
 * Covers all standard CK_RV codes defined in pkcs11_minimal.h plus a
 * hex-formatted fallback for vendor-specific or unlisted codes.
 */
inline std::string ckrvToString(CK_RV rv) noexcept {
    switch (rv) {
        case CKR_OK:                 return "CKR_OK";
        case CKR_GENERAL_ERROR:      return "CKR_GENERAL_ERROR";
        case CKR_DEVICE_ERROR:       return "CKR_DEVICE_ERROR";
        case CKR_PIN_INCORRECT:      return "CKR_PIN_INCORRECT";
        case CKR_USER_NOT_LOGGED_IN: return "CKR_USER_NOT_LOGGED_IN";
        case CKR_ARGUMENTS_BAD:      return "CKR_ARGUMENTS_BAD";
        case CKR_SIGNATURE_INVALID:  return "CKR_SIGNATURE_INVALID";
        default: {
            std::ostringstream oss;
            oss << "CKR_0x" << std::hex << rv;
            return oss.str();
        }
    }
}

/**
 * @brief Thin std::error_category implementation for PKCS#11 return values.
 *
 * Allows PKCS#11 errors to integrate with standard C++ error-handling
 * patterns (`std::error_code`, `std::system_error`).
 */
class Pkcs11Category : public std::error_category {
public:
    const char* name() const noexcept override { return "pkcs11"; }

    std::string message(int condition) const override {
        return ckrvToString(static_cast<CK_RV>(condition));
    }

    static const Pkcs11Category& instance() noexcept {
        static Pkcs11Category cat;
        return cat;
    }
};

/**
 * @brief Construct an std::error_code from a PKCS#11 CK_RV value.
 *
 * @code
 * CK_RV rv = api->C_Initialize(nullptr);
 * if (rv != CKR_OK) {
 *     return make_pkcs11_error(rv);
 * }
 * @endcode
 */
inline std::error_code makePkcs11Error(CK_RV rv) noexcept {
    return {static_cast<int>(rv), Pkcs11Category::instance()};
}

// =============================================================================
// Pkcs11Library — RAII dynamic library loader
// =============================================================================

/**
 * @brief RAII loader for a PKCS#11 shared library.
 *
 * Loads the library with dlopen/LoadLibrary, resolves C_GetFunctionList,
 * calls C_Initialize, and on destruction calls C_Finalize and unloads
 * the library.
 *
 * Non-copyable; movable.
 */
class Pkcs11Library {
public:
    Pkcs11Library() = default;

    ~Pkcs11Library() { unload(); }

    Pkcs11Library(const Pkcs11Library&) = delete;
    Pkcs11Library& operator=(const Pkcs11Library&) = delete;

    Pkcs11Library(Pkcs11Library&& other) noexcept
        : handle_(other.handle_), functions_(other.functions_) {
        other.handle_    = nullptr;
        other.functions_ = nullptr;
    }

    Pkcs11Library& operator=(Pkcs11Library&& other) noexcept {
        if (this != &other) {
            unload();
            handle_    = other.handle_;
            functions_ = other.functions_;
            other.handle_    = nullptr;
            other.functions_ = nullptr;
        }
        return *this;
    }

    /**
     * @brief Load a PKCS#11 library and initialise it.
     *
     * @param path  Filesystem path to the shared library
     *              (e.g. "/usr/lib/softhsm/libsofthsm2.so").
     * @return true on success; false if the library cannot be loaded,
     *         C_GetFunctionList cannot be resolved, or C_Initialize fails.
     */
    bool load(const std::string& path) noexcept {
        if (functions_) {
            // Already loaded; caller should unload first.
            lastError_ = "already loaded";
            return false;
        }

#if defined(_WIN32)
        handle_ = static_cast<void*>(LoadLibraryA(path.c_str()));
        if (!handle_) {
            lastError_ = "LoadLibraryA failed for: " + path;
            return false;
        }
        auto getFn = reinterpret_cast<CK_C_GetFunctionList>(
            GetProcAddress(static_cast<HMODULE>(handle_), "C_GetFunctionList"));
#else
        handle_ = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle_) {
            const char* err = dlerror();
            lastError_ = err ? err : ("dlopen failed for: " + path);
            return false;
        }
        // POSIX requires this cast dance to avoid -Wpedantic warnings.
        void* sym = dlsym(handle_, "C_GetFunctionList");
        CK_C_GetFunctionList getFn = nullptr;
        std::memcpy(&getFn, &sym, sizeof(getFn));
#endif

        if (!getFn) {
            lastError_ = "C_GetFunctionList not found in: " + path;
            unload();
            return false;
        }

        CK_RV rv = getFn(&functions_);
        if (rv != CKR_OK || !functions_) {
            lastError_ = "C_GetFunctionList failed: " + ckrvToString(rv);
            unload();
            return false;
        }

        if (!functions_->C_Initialize) {
            lastError_ = "C_Initialize function pointer is null";
            unload();
            return false;
        }

        rv = functions_->C_Initialize(nullptr);
        if (rv != CKR_OK) {
            lastError_ = "C_Initialize failed: " + ckrvToString(rv);
            // Unload without calling C_Finalize (it was never initialised).
            functions_ = nullptr;
#if defined(_WIN32)
            FreeLibrary(static_cast<HMODULE>(handle_));
#else
            dlclose(handle_);
#endif
            handle_ = nullptr;
            return false;
        }

        return true;
    }

    /**
     * @brief Finalize the PKCS#11 library and unload it.
     *
     * Safe to call on an already-unloaded library.
     */
    void unload() noexcept {
        if (functions_) {
            if (functions_->C_Finalize) {
                functions_->C_Finalize(nullptr);
            }
            functions_ = nullptr;
        }
        if (handle_) {
#if defined(_WIN32)
            FreeLibrary(static_cast<HMODULE>(handle_));
#else
            dlclose(handle_);
#endif
            handle_ = nullptr;
        }
    }

    /**
     * @brief Returns true if the library is loaded and functions_ is valid.
     */
    bool isLoaded() const noexcept { return functions_ != nullptr; }

    /**
     * @brief Return the resolved PKCS#11 function list pointer.
     *
     * May be nullptr if the library has not been loaded successfully.
     */
    CK_FUNCTION_LIST_PTR functions() const noexcept { return functions_; }

    /**
     * @brief Human-readable description of the last error.
     */
    const std::string& lastError() const noexcept { return lastError_; }

private:
    void*                   handle_    = nullptr;
    CK_FUNCTION_LIST_PTR    functions_ = nullptr;
    std::string             lastError_;
};

// =============================================================================
// Pkcs11Session — RAII session + login guard
// =============================================================================

/**
 * @brief RAII guard for a PKCS#11 session.
 *
 * Opens a session via C_OpenSession on construction (or via open()) and
 * closes it (plus logs out if logged in) on destruction.
 *
 * Non-copyable; movable.
 */
class Pkcs11Session {
public:
    /**
     * @brief Construct without opening a session yet; call open() later.
     * @param api  Non-null pointer to the PKCS#11 function list from
     *             Pkcs11Library::functions().
     */
    explicit Pkcs11Session(CK_FUNCTION_LIST_PTR api) noexcept
        : api_(api) {}

    ~Pkcs11Session() { close(); }

    Pkcs11Session(const Pkcs11Session&) = delete;
    Pkcs11Session& operator=(const Pkcs11Session&) = delete;

    Pkcs11Session(Pkcs11Session&& other) noexcept
        : api_(other.api_),
          handle_(other.handle_),
          loggedIn_(other.loggedIn_) {
        other.handle_   = 0;
        other.loggedIn_ = false;
    }

    Pkcs11Session& operator=(Pkcs11Session&& other) noexcept {
        if (this != &other) {
            close();
            api_      = other.api_;
            handle_   = other.handle_;
            loggedIn_ = other.loggedIn_;
            other.handle_   = 0;
            other.loggedIn_ = false;
        }
        return *this;
    }

    /**
     * @brief Open a PKCS#11 session on the given slot.
     *
     * @param slotId  Slot identifier (from C_GetSlotList).
     * @param flags   Session flags; CKF_SERIAL_SESSION is the standard value.
     * @return true on success; false if C_OpenSession failed.
     */
    bool open(CK_SLOT_ID slotId, uint32_t flags = CKF_SERIAL_SESSION) noexcept {
        if (!api_ || !api_->C_OpenSession) {
            lastError_ = "null function list";
            return false;
        }
        close(); // close any existing session first
        CK_RV rv = api_->C_OpenSession(slotId, flags, nullptr, nullptr, &handle_);
        if (rv != CKR_OK) {
            handle_    = 0;
            lastError_ = "C_OpenSession: " + ckrvToString(rv);
            return false;
        }
        return true;
    }

    /**
     * @brief Authenticate the session with a PIN.
     *
     * @param userType  CKU_USER (normal user) or CKU_SO (security officer).
     * @param pin       PIN string.
     * @return true on success.
     */
    bool login(uint32_t userType, const std::string& pin) noexcept {
        if (!api_ || !api_->C_Login || !handle_) {
            lastError_ = "session not open or null api";
            return false;
        }
        CK_RV rv = api_->C_Login(
            handle_,
            userType,
            reinterpret_cast<CK_BYTE_PTR>(const_cast<char*>(pin.data())),
            static_cast<uint32_t>(pin.size()));
        if (rv != CKR_OK) {
            lastError_ = "C_Login: " + ckrvToString(rv);
            return false;
        }
        loggedIn_ = true;
        return true;
    }

    /**
     * @brief Log out and close the session.
     *
     * Safe to call multiple times.
     */
    void close() noexcept {
        if (!api_ || !handle_) {
          return;
        }
        if (loggedIn_ && api_->C_Logout) {
            api_->C_Logout(handle_);
            loggedIn_ = false;
        }
        if (api_->C_CloseSession) {
            api_->C_CloseSession(handle_);
        }
        handle_ = 0;
    }

    /**
     * @brief Return the underlying session handle.
     *
     * 0 if the session is not open.
     */
    CK_SESSION_HANDLE handle() const noexcept { return handle_; }

    /**
     * @brief Return true if the session is currently open.
     */
    bool isOpen() const noexcept { return handle_ != 0; }

    /**
     * @brief Return true if the session has been successfully logged in.
     */
    bool isLoggedIn() const noexcept { return loggedIn_; }

    /**
     * @brief Return the raw PKCS#11 function list.
     */
    CK_FUNCTION_LIST_PTR functions() const noexcept { return api_; }

    /**
     * @brief Human-readable description of the last error.
     */
    const std::string& lastError() const noexcept { return lastError_; }

private:
    CK_FUNCTION_LIST_PTR api_       = nullptr;
    CK_SESSION_HANDLE    handle_    = 0;
    bool                 loggedIn_  = false;
    std::string          lastError_;
};

// =============================================================================
// Attribute helpers
// =============================================================================

/**
 * @brief Convenience type for a key-value pair used to build CK_ATTRIBUTE
 *        search templates.
 *
 * The value is stored as a plain uint32_t because the most common
 * attribute types (CKA_CLASS, CKA_KEY_TYPE, …) hold 32-bit constants.
 * For label-based searches use findObjectByLabel().
 */
struct AttributeFilter {
    uint32_t type;   ///< Attribute type (e.g. CKA_CLASS)
    uint32_t value;  ///< Attribute value as a 32-bit unsigned integer
};

// =============================================================================
// Free helper functions — thin C++ wrappers over CK_FUNCTION_LIST operations
// =============================================================================

/**
 * @brief Enumerate all slots that have a token present.
 *
 * @param api  PKCS#11 function list (must not be null).
 * @return Vector of slot IDs, or empty on failure.
 */
inline std::vector<CK_SLOT_ID> listSlots(CK_FUNCTION_LIST_PTR api) noexcept {
    if (!api || !api->C_GetSlotList) return {};
    uint32_t count = 0;
    CK_RV rv = api->C_GetSlotList(1 /*tokenPresent*/, nullptr, &count);
    if (rv != CKR_OK || count == 0) return {};
    std::vector<CK_SLOT_ID> slots(count);
    rv = api->C_GetSlotList(1, slots.data(), &count);
    if (rv != CKR_OK) return {};
    slots.resize(count);
    return slots;
}

/**
 * @brief Find PKCS#11 objects matching a set of attribute filters.
 *
 * Runs C_FindObjectsInit / C_FindObjects / C_FindObjectsFinal and
 * collects up to @p maxObjects handles.
 *
 * @param session     Open Pkcs11Session.
 * @param filters     Attribute constraints (type + 32-bit value).
 * @param maxObjects  Upper bound on results (default: 64).
 * @return Vector of matching object handles; empty on failure or no match.
 */
inline std::vector<CK_OBJECT_HANDLE> findObjects(
        const Pkcs11Session&              session,
        const std::vector<AttributeFilter>& filters,
        uint32_t                           maxObjects = 64) noexcept {
    auto api = session.functions();
    if (!api || !session.isOpen()) return {};

    // Build CK_ATTRIBUTE array (values stored locally)
    std::vector<uint32_t>    storage(filters.size());
    std::vector<CK_ATTRIBUTE> attrs(filters.size());
    for (std::size_t i = 0; i < filters.size(); ++i) {
        storage[i]           = filters[i].value;
        attrs[i].type        = filters[i].type;
        attrs[i].pValue      = &storage[i];
        attrs[i].ulValueLen  = sizeof(uint32_t);
    }

    CK_RV rv = api->C_FindObjectsInit(
        session.handle(),
        attrs.empty() ? nullptr : attrs.data(),
        static_cast<uint32_t>(attrs.size()));
    if (rv != CKR_OK) return {};

    std::vector<CK_OBJECT_HANDLE> handles(maxObjects);
    uint32_t found = 0;
    rv = api->C_FindObjects(session.handle(), handles.data(), maxObjects, &found);
    api->C_FindObjectsFinal(session.handle());

    if (rv != CKR_OK) return {};
    handles.resize(found);
    return handles;
}

/**
 * @brief Find objects matching a label string and optional class filter.
 *
 * @param session    Open Pkcs11Session.
 * @param label      Key/certificate label (CKA_LABEL).
 * @param objClass   Optional object class (e.g. CKO_PRIVATE_KEY).
 *                   Pass 0 to omit this filter.
 * @return Vector of matching object handles; empty on failure.
 */
inline std::vector<CK_OBJECT_HANDLE> findObjectsByLabel(
        const Pkcs11Session& session,
        const std::string&   label,
        uint32_t             objClass = 0) noexcept {
    auto api = session.functions();
    if (!api || !session.isOpen() || label.empty()) return {};

    std::vector<CK_ATTRIBUTE> attrs;

    // Class filter (optional).
    // The class value must remain addressable for the duration of this call;
    // a local variable is used to avoid dangling pointer issues.
    uint32_t classVal = objClass;
    if (objClass != 0) {
        CK_ATTRIBUTE a;
        a.type       = CKA_CLASS;
        a.pValue     = &classVal;
        a.ulValueLen = sizeof(classVal);
        attrs.push_back(a);
    }

    // Label filter
    {
        CK_ATTRIBUTE a;
        a.type       = CKA_LABEL;
        a.pValue     = const_cast<char*>(label.data());
        a.ulValueLen = label.size();
        attrs.push_back(a);
    }

    CK_RV rv = api->C_FindObjectsInit(
        session.handle(),
        attrs.data(),
        static_cast<uint32_t>(attrs.size()));
    if (rv != CKR_OK) return {};

    const uint32_t kMax = 16;
    std::vector<CK_OBJECT_HANDLE> handles(kMax);
    uint32_t found = 0;
    rv = api->C_FindObjects(session.handle(), handles.data(), kMax, &found);
    api->C_FindObjectsFinal(session.handle());

    if (rv != CKR_OK) return {};
    handles.resize(found);
    return handles;
}

/**
 * @brief Sign data (or a pre-computed hash) with a private key object.
 *
 * Uses C_SignInit / C_Sign.  The caller is responsible for pre-hashing
 * @p data if the chosen @p mechanism expects a digest rather than raw data
 * (e.g. CKM_RSA_PKCS expects a DER-encoded DigestInfo; use
 * CKM_SHA256_RSA_PKCS to have the HSM compute the hash).
 *
 * @param session    Open, logged-in Pkcs11Session.
 * @param privateKey Handle of the private key object.
 * @param mechanism  PKCS#11 mechanism type (e.g. CKM_SHA256_RSA_PKCS).
 * @param data       Data (or hash) to sign.
 * @return Signature bytes, or empty on failure.
 */
inline std::vector<uint8_t> signData(
        const Pkcs11Session&       session,
        CK_OBJECT_HANDLE           privateKey,
        uint32_t                   mechanism,
        const std::vector<uint8_t>& data) noexcept {
    auto api = session.functions();
    if (!api || !session.isOpen() || !privateKey) return {};

    CK_MECHANISM mech{mechanism, nullptr, 0};
    CK_RV rv = api->C_SignInit(session.handle(), &mech, privateKey);
    if (rv != CKR_OK) return {};

    uint32_t sigLen = 0;
    rv = api->C_Sign(
        session.handle(),
        const_cast<CK_BYTE_PTR>(data.data()),
        static_cast<uint32_t>(data.size()),
        nullptr, &sigLen);
    if (rv != CKR_OK || sigLen == 0) return {};

    std::vector<uint8_t> sig(sigLen);
    rv = api->C_Sign(
        session.handle(),
        const_cast<CK_BYTE_PTR>(data.data()),
        static_cast<uint32_t>(data.size()),
        sig.data(), &sigLen);
    if (rv != CKR_OK) return {};

    sig.resize(sigLen);
    return sig;
}

/**
 * @brief Verify a signature with a public key object.
 *
 * Uses C_VerifyInit / C_Verify.
 *
 * @param session    Open, logged-in Pkcs11Session.
 * @param publicKey  Handle of the public key object.
 * @param mechanism  PKCS#11 mechanism type (must match the one used for signing).
 * @param data       Original data (or hash) that was signed.
 * @param signature  Signature bytes to verify.
 * @return true if the signature is valid; false on verification failure or error.
 */
inline bool verifyData(
        const Pkcs11Session&       session,
        CK_OBJECT_HANDLE           publicKey,
        uint32_t                   mechanism,
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature) noexcept {
    auto api = session.functions();
    if (!api || !session.isOpen() || !publicKey) {
      return false;
    }

    CK_MECHANISM mech{mechanism, nullptr, 0};
    CK_RV rv = api->C_VerifyInit(session.handle(), &mech, publicKey);
    if (rv != CKR_OK) {
      return false;
    }

    rv = api->C_Verify(
        session.handle(),
        const_cast<CK_BYTE_PTR>(data.data()),
        static_cast<uint32_t>(data.size()),
        const_cast<CK_BYTE_PTR>(signature.data()),
        static_cast<uint32_t>(signature.size()));
    return rv == CKR_OK;
}

/**
 * @brief Encrypt data with a public key object.
 *
 * Uses C_EncryptInit / C_Encrypt.  Intended primarily for RSA-OAEP DEK
 * wrapping where the public key is stored in the HSM.
 *
 * @param session    Open, logged-in Pkcs11Session.
 * @param publicKey  Handle of the public (or wrapping) key object.
 * @param mechanism  PKCS#11 mechanism (e.g. CKM_RSA_PKCS_OAEP).
 * @param mechParam  Optional mechanism parameter bytes (e.g. OAEP params).
 * @param plaintext  Data to encrypt.
 * @return Ciphertext bytes, or empty on failure.
 */
inline std::vector<uint8_t> encryptData(
        const Pkcs11Session&       session,
        CK_OBJECT_HANDLE           publicKey,
        uint32_t                   mechanism,
        void*                      mechParam,
        size_t                     mechParamLen,
        const std::vector<uint8_t>& plaintext) noexcept {
    auto api = session.functions();
    if (!api || !session.isOpen() || !publicKey) return {};

    CK_MECHANISM mech{mechanism, mechParam, mechParamLen};
    CK_RV rv = api->C_EncryptInit(session.handle(), &mech, publicKey);
    if (rv != CKR_OK) return {};

    CK_ULONG ctLen = 0;
    rv = api->C_Encrypt(
        session.handle(),
        const_cast<CK_BYTE_PTR>(plaintext.data()),
        static_cast<CK_ULONG>(plaintext.size()),
        nullptr, &ctLen);
    if (rv != CKR_OK || ctLen == 0) return {};

    std::vector<uint8_t> ct(ctLen);
    rv = api->C_Encrypt(
        session.handle(),
        const_cast<CK_BYTE_PTR>(plaintext.data()),
        static_cast<CK_ULONG>(plaintext.size()),
        ct.data(), &ctLen);
    if (rv != CKR_OK) return {};

    ct.resize(ctLen);
    return ct;
}

/**
 * @brief Decrypt data with a private key object.
 *
 * Uses C_DecryptInit / C_Decrypt.
 *
 * @param session    Open, logged-in Pkcs11Session.
 * @param privateKey Handle of the private (or unwrapping) key object.
 * @param mechanism  PKCS#11 mechanism (must match encryption mechanism).
 * @param mechParam  Optional mechanism parameter bytes.
 * @param mechParamLen Length of mechParam.
 * @param ciphertext Ciphertext to decrypt.
 * @return Plaintext bytes, or empty on failure.
 */
inline std::vector<uint8_t> decryptData(
        const Pkcs11Session&       session,
        CK_OBJECT_HANDLE           privateKey,
        uint32_t                   mechanism,
        void*                      mechParam,
        size_t                     mechParamLen,
        const std::vector<uint8_t>& ciphertext) noexcept {
    auto api = session.functions();
    if (!api || !session.isOpen() || !privateKey) return {};

    CK_MECHANISM mech{mechanism, mechParam, mechParamLen};
    CK_RV rv = api->C_DecryptInit(session.handle(), &mech, privateKey);
    if (rv != CKR_OK) return {};

    CK_ULONG ptLen = 0;
    rv = api->C_Decrypt(
        session.handle(),
        const_cast<CK_BYTE_PTR>(ciphertext.data()),
        static_cast<CK_ULONG>(ciphertext.size()),
        nullptr, &ptLen);
    if (rv != CKR_OK || ptLen == 0) return {};

    std::vector<uint8_t> pt(ptLen);
    rv = api->C_Decrypt(
        session.handle(),
        const_cast<CK_BYTE_PTR>(ciphertext.data()),
        static_cast<CK_ULONG>(ciphertext.size()),
        pt.data(), &ptLen);
    if (rv != CKR_OK) return {};

    pt.resize(ptLen);
    return pt;
}

/**
 * @brief Generate an RSA key pair in the HSM.
 *
 * Creates non-extractable, token-persistent keys labelled with @p label.
 *
 * @param session    Open, logged-in Pkcs11Session.
 * @param label      CKA_LABEL for the generated key pair.
 * @param keyBits    Key size in bits (e.g. 2048, 3072, 4096).
 * @param extractable Whether the private key may be extracted.
 *                   Should be false for production keys.
 * @return {publicKeyHandle, privateKeyHandle}, both 0 on failure.
 */
inline std::pair<CK_OBJECT_HANDLE, CK_OBJECT_HANDLE> generateRsaKeyPair(
        const Pkcs11Session& session,
        const std::string&   label,
        uint32_t             keyBits    = 2048,
        bool                 extractable = false) noexcept {
    auto api = session.functions();
    if (!api || !session.isOpen() || label.empty()) return {0, 0};

    // Public exponent 65537
    static const uint8_t kPublicExponent[] = {0x01, 0x00, 0x01};

    CK_BBOOL ck_true  = CK_TRUE;
    CK_BBOOL extractableVal = extractable ? CK_TRUE : CK_FALSE;

    CK_ATTRIBUTE pubTemplate[] = {
        {CKA_TOKEN,           &ck_true,                   sizeof(ck_true)},
        {CKA_VERIFY,          &ck_true,                   sizeof(ck_true)},
        {CKA_MODULUS_BITS,    &keyBits,                   sizeof(keyBits)},
        {CKA_PUBLIC_EXPONENT, const_cast<uint8_t*>(kPublicExponent),
                                                          sizeof(kPublicExponent)},
        {CKA_LABEL,           const_cast<char*>(label.data()), label.size()},
    };

    CK_ATTRIBUTE privTemplate[] = {
        {CKA_TOKEN,       &ck_true,        sizeof(ck_true)},
        {CKA_PRIVATE,     &ck_true,        sizeof(ck_true)},
        {CKA_SENSITIVE,   &ck_true,        sizeof(ck_true)},
        {CKA_SIGN,        &ck_true,        sizeof(ck_true)},
        {CKA_EXTRACTABLE, &extractableVal, sizeof(extractableVal)},
        {CKA_LABEL,       const_cast<char*>(label.data()), label.size()},
    };

    CK_MECHANISM mech{CKM_RSA_PKCS_KEY_PAIR_GEN, nullptr, 0};
    CK_OBJECT_HANDLE pubKey = 0, privKey = 0;

    CK_RV rv = api->C_GenerateKeyPair(
        session.handle(),
        &mech,
        pubTemplate,  sizeof(pubTemplate)  / sizeof(CK_ATTRIBUTE),
        privTemplate, sizeof(privTemplate) / sizeof(CK_ATTRIBUTE),
        &pubKey, &privKey);

    if (rv != CKR_OK) return {0, 0};
    return {pubKey, privKey};
}

/**
 * @brief Retrieve a uint32_t scalar attribute value from a PKCS#11 object.
 *
 * @param session    Open Pkcs11Session.
 * @param object     Object handle.
 * @param attrType   Attribute type (e.g. CKA_CLASS).
 * @return Attribute value, or std::nullopt on failure.
 */
inline std::optional<uint32_t> getAttribute(
        const Pkcs11Session& session,
        CK_OBJECT_HANDLE     object,
        uint32_t             attrType) noexcept {
    auto api = session.functions();
    if (!api || !api->C_GetAttributeValue || !session.isOpen() || !object)
        return std::nullopt;

    uint32_t value = 0;
    CK_ATTRIBUTE attr{attrType, &value, sizeof(value)};
    CK_RV rv = api->C_GetAttributeValue(session.handle(), object, &attr, 1);
    if (rv != CKR_OK) {
      return std::nullopt;
    }
    return value;
}

/**
 * @brief Retrieve a variable-length byte attribute from a PKCS#11 object.
 *
 * Performs two calls: first to determine the length, then to fetch data.
 *
 * @param session    Open Pkcs11Session.
 * @param object     Object handle.
 * @param attrType   Attribute type (e.g. CKA_VALUE for certificate DER).
 * @return Attribute bytes, or empty on failure.
 */
inline std::vector<uint8_t> getAttributeBytes(
        const Pkcs11Session& session,
        CK_OBJECT_HANDLE     object,
        uint32_t             attrType) noexcept {
    auto api = session.functions();
    if (!api || !api->C_GetAttributeValue || !session.isOpen() || !object)
        return {};

    // First call: determine length
    CK_ATTRIBUTE attr{attrType, nullptr, 0};
    CK_RV rv = api->C_GetAttributeValue(session.handle(), object, &attr, 1);
    if (rv != CKR_OK || attr.ulValueLen == 0) return {};

    // Second call: fetch data
    std::vector<uint8_t> buf(attr.ulValueLen);
    attr.pValue = buf.data();
    rv = api->C_GetAttributeValue(session.handle(), object, &attr, 1);
    if (rv != CKR_OK) return {};

    return buf;
}

} // namespace pkcs11
} // namespace security
} // namespace themis

