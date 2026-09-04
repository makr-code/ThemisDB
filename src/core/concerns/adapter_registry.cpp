/**
 * @file adapter_registry.cpp
 * @brief Non-template AdapterRegistry method implementations including runtime plugin loading.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0
 * @note Status: Production Ready
 */

#include "core/concerns/adapter_registry.h"

#include <openssl/evp.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <typeindex>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace themis {
namespace core {
namespace concerns {

namespace {

std::string sha256FileHex(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return {};
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        return {};
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return {};
    }

    std::array<char, 16 * 1024> buffer{};
    while (file.good()) {
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize read_bytes = file.gcount();
        if (read_bytes <= 0) {
            break;
        }
        if (EVP_DigestUpdate(
                ctx, buffer.data(), static_cast<size_t>(read_bytes)) != 1) {
            EVP_MD_CTX_free(ctx);
            return {};
        }
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    if (EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return {};
    }
    EVP_MD_CTX_free(ctx);

    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < hash_len; ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(hash[i]);
    }
    return oss.str();
}

} // namespace

// ---------------------------------------------------------------------------
// PluginHandle destructor — releases the OS library handle
// ---------------------------------------------------------------------------

AdapterRegistry::PluginHandle::~PluginHandle() {
    if (!handle) {
        return;
    }
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
    handle = nullptr;
}

// ---------------------------------------------------------------------------
// AdapterRegistry destructor
// ---------------------------------------------------------------------------

AdapterRegistry::~AdapterRegistry() {
    // plugin_handles_ contains RAII PluginHandle values; the destructor of
    // each PluginHandle calls dlclose/FreeLibrary when the map is cleared.
    // Clear the adapter registry first so that shared_ptrs into plugin code
    // are released before the library handles are closed.
    {
        std::unique_lock<std::shared_mutex> lock(registry_mutex_);
        registry_.clear();
        plugin_handles_.clear();
    }
}

// ---------------------------------------------------------------------------
// count()
// ---------------------------------------------------------------------------

size_t AdapterRegistry::count() const {
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    return static_cast<int>(registry_.size());
}

// ---------------------------------------------------------------------------
// hasAdapter()
// ---------------------------------------------------------------------------

bool AdapterRegistry::hasAdapter(std::type_index type) const {
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    return registry_.find(type) != registry_.end();
}

// ---------------------------------------------------------------------------
// setTrustPolicy()
// ---------------------------------------------------------------------------

void AdapterRegistry::setTrustPolicy(AdapterTrustPolicy policy) {
    std::unique_lock<std::shared_mutex> lock(registry_mutex_);
    trust_policy_ = policy;
}

// ---------------------------------------------------------------------------
// loadFromPlugin()
// ---------------------------------------------------------------------------

bool AdapterRegistry::loadFromPlugin(const std::string& path,
                                     const std::string& adapter_id) {
    // ---- 1. Basic argument validation -----
    if (path.empty()) {
        std::cerr << "[AdapterRegistry] loadFromPlugin: path must not be empty\n";
        return false;
    }

    // ---- 2. File existence check -----
    {
        std::error_code ec = {};
        if (!std::filesystem::exists(path, ec) || ec) {
            std::cerr << "[AdapterRegistry] loadFromPlugin: file not found: "
                      << path << '\n';
            return false;
        }
    }

    // ---- 3. Trust-policy: optional signature verification -----
    {
        AdapterTrustPolicy policy;
        {
            std::shared_lock<std::shared_mutex> lock(registry_mutex_);
            policy = trust_policy_;
        }

        if (policy == AdapterTrustPolicy::kRequireSignature) {
            const std::string sig_path = path + ".sig";
            std::ifstream     sig_file(sig_path);
            if (!sig_file.is_open()) {
                std::cerr << "[AdapterRegistry] loadFromPlugin: signature file "
                             "required but not found: "
                          << sig_path << '\n';
                return false;
            }

            // Signature file format: single line containing the SHA-256 hex digest
            std::string expected_digest = {};
            std::getline(sig_file, expected_digest);
            // Trim trailing whitespace / CR
            while (!expected_digest.empty() &&
                   (expected_digest.back() == '\n' ||
                    expected_digest.back() == '\r' ||
                    expected_digest.back() == ' ')) {
                expected_digest.pop_back();
            }

            const std::string actual_digest = sha256FileHex(path);

            if (actual_digest.empty() || actual_digest != expected_digest) {
                std::cerr << "[AdapterRegistry] loadFromPlugin: SHA-256 "
                             "verification failed for: "
                          << path << '\n';
                return false;
            }
        }
    }

    // ---- 4. Open the shared library -----
    void* handle = nullptr;
#ifdef _WIN32
    handle = static_cast<void*>(LoadLibraryA(path.c_str()));
    if (!handle) {
        std::cerr << "[AdapterRegistry] loadFromPlugin: LoadLibraryA failed for: "
                  << path << " (error=" << GetLastError() << ")\n";
        return false;
    }
#else
    handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        std::cerr << "[AdapterRegistry] loadFromPlugin: dlopen failed for: "
                  << path << " — " << dlerror() << '\n';
        return false;
    }
#endif

    // ---- 5. Resolve plugin entry point -----
    ThemisPluginRegisterFn init_fn = nullptr;
#ifdef _WIN32
    init_fn = reinterpret_cast<ThemisPluginRegisterFn>(
        GetProcAddress(static_cast<HMODULE>(handle), kPluginInitSymbol));
#else
    // POSIX: dlsym returns void*; the double reinterpret_cast via uintptr_t
    // is the idiomatic workaround for ISO C++ forbidding casts between
    // data and function pointers directly, and matches the pattern used in
    // src/acceleration/plugin_loader.cpp.
    init_fn = reinterpret_cast<ThemisPluginRegisterFn>(
        reinterpret_cast<uintptr_t>(dlsym(handle, kPluginInitSymbol)));
#endif

    if (!init_fn) {
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(handle));
#else
        dlclose(handle);
#endif
        std::cerr << "[AdapterRegistry] loadFromPlugin: symbol '"
                  << kPluginInitSymbol << "' not found in: " << path << '\n';
        return false;
    }

    // ---- 6. Invoke plugin init function -----
    const int rc = init_fn(this, adapter_id.c_str(), kPluginAbiVersion);
    if (rc != 0) {
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(handle));
#else
        dlclose(handle);
#endif
        std::cerr << "[AdapterRegistry] loadFromPlugin: plugin init returned "
                  << rc << " for: " << path << '\n';
        return false;
    }

    // ---- 7. Keep the library handle alive -----
    {
        std::unique_lock<std::shared_mutex> lock(registry_mutex_);
        // Erase any previous handle for the same path (e.g., a reload).
        plugin_handles_.erase(path);
        plugin_handles_.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(path),
            std::forward_as_tuple(handle, path));
    }

    return true;
}

} // namespace concerns
} // namespace core
} // namespace themis
