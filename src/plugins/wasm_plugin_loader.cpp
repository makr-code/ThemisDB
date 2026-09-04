/**
 * @file wasm_plugin_loader.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "plugins/wasm_host_api.h"
#include "plugins/plugin_manager.h"
#include "themis/edition.h"
#include "themis/runtime_license_gate.h"

#include <string>
#include <memory>
#include <fstream>
#include <iterator>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <vector>
#include <mutex>

// SHA-256 via OpenSSL (same approach as plugin_system_edition.cpp)
#include <openssl/evp.h>

// Optional WASM runtimes — each guarded by its own compile flag.
#ifdef THEMIS_WASM_WASMTIME
#   include <wasmtime.h>
#endif
#ifdef THEMIS_WASM_WASMEDGE
#   include <wasmedge/wasmedge.h>
#endif

namespace themis {
namespace plugins {

// ============================================================================
// Internal helpers (SHA-256 file hash, edition gate)
// ============================================================================

namespace {

WasmPluginLoadFn& wasmPluginLoadFnStorage() {
    static WasmPluginLoadFn fn;
    return fn;
}

std::mutex& wasmPluginLoadFnMutex() {
    static std::mutex m;
    return m;
}

#ifdef THEMIS_WASM_WASMTIME
// RAII wrapper for WasmtimeBundle to ensure all wasmtime resources are cleaned up
struct WasmtimeBundleDeleter {
    void operator()(void* bundle_ptr) const noexcept {
        if (!bundle_ptr) {
          return;
        }
        
        // Re-define the bundle struct for cleanup purposes
        struct WasmtimeBundle {
            wasmtime_engine_t*  engine;
            wasmtime_store_t*   store;
            wasmtime_linker_t*  linker;
            wasmtime_module_t*  module;
            wasmtime_instance_t instance;
        };
        
        auto* b = static_cast<WasmtimeBundle*>(bundle_ptr);
        // Cleanup in reverse order of creation
        wasmtime_linker_delete(b->linker);
        wasmtime_module_delete(b->module);
        wasmtime_store_delete(b->store);
        wasmtime_engine_delete(b->engine);
        delete b;
    }
};

using UniqueWasmtimeBundle = std::unique_ptr<void, WasmtimeBundleDeleter>;
#endif // THEMIS_WASM_WASMTIME

/**
 * @brief Compute the SHA-256 hex digest of a file at @p path.
 * @return Lowercase hex string, or empty string on I/O or crypto error.
 */
static std::string computeFileHash(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return "";
    }

    struct EvpCtxDeleter {
        void operator()(EVP_MD_CTX* p) const noexcept { EVP_MD_CTX_free(p); }
    };
    std::unique_ptr<EVP_MD_CTX, EvpCtxDeleter> ctx(EVP_MD_CTX_new());
    if (!ctx || EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1) {
        return "";
    }

    char buffer[8192];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        if (EVP_DigestUpdate(ctx.get(), buffer,
                             static_cast<size_t>(file.gcount())) != 1) {
            return "";
        }
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int  hash_len = 0;
    if (EVP_DigestFinal_ex(ctx.get(), hash, &hash_len) != 1) {
        return "";
    }

    std::ostringstream oss;
    for (unsigned int i = 0; i < hash_len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(hash[i]);
    }
    return oss.str();
}

/**
 * @brief Return true if the current edition permits WASM plugin execution.
 *
 * WASM runtime support is gated behind the Enterprise edition flag, consistent
 * with the broader plugin subsystem policy in plugin_system_edition.cpp.
 */
static bool isWasmRuntimeAllowed(std::string& error_out) {
    return license::RuntimeLicenseGate::instance()
               .isFeatureAllowed("enterprise_plugins", error_out);
}

} // anonymous namespace

void setWasmPluginLoadFn(WasmPluginLoadFn fn) {
    std::lock_guard<std::mutex> lk(wasmPluginLoadFnMutex());
    wasmPluginLoadFnStorage() = std::move(fn);
}

// ============================================================================
// WasmPluginLoader — public API
// ============================================================================

/**
 * @brief Verify a WASM module's SHA-256 hash against the manifest value.
 *
 * This function is called by PluginManager before any WASM instantiation so
 * that a tampered or corrupted .wasm file is rejected before any code runs.
 *
 * @param wasm_path    Filesystem path to the .wasm binary.
 * @param expected_sha256  Expected lowercase hex SHA-256 from the manifest.
 * @param error_out    Receives a human-readable error message on failure.
 * @return true if the hash matches, false otherwise.
 * 
 * @note QW-44 Fail-Closed Guard: Rejects empty hashes (fail-closed).
 * Unsigned or unhashed WASM modules are rejected at load time to prevent
 * loading unsigned code in production. Hash must be non-empty, 64 chars, and match.
 */
bool verifyWasmModuleHash(const std::string& wasm_path,
                          const std::string& expected_sha256,
                          std::string&       error_out) {
    // QW-44: Fail-closed guard - reject empty/missing hashes
    // Empty hash indicates unsigned WASM module; reject to prevent unsigned code execution
    if (expected_sha256.empty()) {
        error_out = "QW-44 Guard: WASM module hash validation failed - hash is empty/missing (unsigned module rejected): " + wasm_path;
        return false;  // Fail-closed: reject unsigned modules
    }

    // Hash must be 64 characters (SHA-256 hex = 256 bits = 64 hex chars)
    if (expected_sha256.length() != 64) {
        error_out = "QW-44 Guard: Invalid hash length " + std::to_string(expected_sha256.length()) +
                    " (expected 64 for SHA-256): " + wasm_path;
        return false;  // Fail-closed: reject malformed hashes
    }

    const std::string actual = computeFileHash(wasm_path);
    if (actual.empty()) {
        error_out = "Failed to compute SHA-256 for WASM module: " + wasm_path;
        return false;
    }

    if (actual != expected_sha256) {
        error_out = "SHA-256 mismatch for WASM module '" + wasm_path +
                    "': manifest=" + expected_sha256 +
                    ", actual=" + actual;
        return false;
    }

    return true;
}

// ============================================================================
// WasmHostAPI — IThemisPlugin bridge implementation
// ============================================================================

#ifdef THEMIS_WASM_SUPPORT

/**
 * @brief Load and instantiate a WASM plugin module.
 *
 * Performs in order:
 *   1. Edition + license gate check.
 *   2. SHA-256 hash verification (fail-closed).
 *   3. WASM runtime instantiation.
 *   4. Returns a WasmHostAPI wrapping the live module instance.
 *
 * @param wasm_path       Filesystem path to the .wasm binary.
 * @param expected_sha256 Expected SHA-256 hex from the plugin manifest.
 * @param runtime         Which WASM backend to use.
 * @param module_name     Human-readable plugin name for diagnostics.
 * @param error_out       Receives a human-readable error on failure.
 * @return Owning pointer to an IThemisPlugin-compatible WASM bridge, or nullptr on failure.
 */
std::unique_ptr<IThemisPlugin> loadWasmPlugin(
    const std::string& wasm_path,
    const std::string& expected_sha256,
    WasmPluginRuntime  runtime,
    const std::string& module_name,
    std::string&       error_out)
{
    WasmPluginLoadFn fn;
    {
        std::lock_guard<std::mutex> lk(wasmPluginLoadFnMutex());
        fn = wasmPluginLoadFnStorage();
    }
    if (fn) {
        return fn(wasm_path, expected_sha256, runtime, module_name, error_out);
    }
    // 1. Enterprise edition gate
    if (!isWasmRuntimeAllowed(error_out)) {
        return nullptr;
    }

    // 2. SHA-256 verification (fail-closed per FUTURE_ENHANCEMENTS.md §Security)
    if (!verifyWasmModuleHash(wasm_path, expected_sha256, error_out)) {
        return nullptr;
    }

    // 3. Runtime instantiation — guarded by per-runtime compile flags.
    //    Pass THEMIS_WASM_WASMTIME=1 to link against the Wasmtime C API, or
    //    THEMIS_WASM_WASMEDGE=1 to link against the WasmEdge C API.

#if defined(THEMIS_WASM_WASMTIME)
    // ---- Wasmtime (Bytecode Alliance) ----------------------------------------
    if (runtime == WasmPluginRuntime::WASMTIME || runtime == WasmPluginRuntime::NONE) {
        // Read the WASM binary into memory.
        std::ifstream wasm_file(wasm_path, std::ios::binary);
        if (!wasm_file) {
            error_out = "Cannot open WASM file: " + wasm_path;
            return nullptr;
        }
        const std::vector<uint8_t> wasm_bytes{
            std::istreambuf_iterator<char>(wasm_file),
            std::istreambuf_iterator<char>()};

        wasmtime_engine_t* engine = wasmtime_engine_new();
        wasmtime_store_t*  store  = wasmtime_store_new(engine, nullptr, nullptr);
        wasmtime_context_t* ctx   = wasmtime_store_context(store);

        wasmtime_module_t* module = nullptr;
        wasmtime_error_t*  err    = wasmtime_module_new(
            engine,
            reinterpret_cast<const uint8_t*>(wasm_bytes.data()),
            wasm_bytes.size(),
            &module);
        if (err) {
            wasm_byte_vec_t msg;
            wasmtime_error_message(err, &msg);
            error_out = std::string(msg.data, msg.size);
            wasm_byte_vec_delete(&msg);
            wasmtime_error_delete(err);
            wasmtime_store_delete(store);
            wasmtime_engine_delete(engine);
            return nullptr;
        }

        wasmtime_linker_t* linker = wasmtime_linker_new(engine);
        wasmtime_linker_define_wasi(linker);

        wasmtime_instance_t instance{};
        wasmtime_trap_t*    trap = nullptr;
        err = wasmtime_linker_instantiate(linker, ctx, module, &instance, &trap);
        if (err || trap) {
            if (trap) {
                wasm_byte_vec_t msg;
                wasmtime_trap_message(trap, &msg);
                error_out = std::string(msg.data, msg.size);
                wasm_byte_vec_delete(&msg);
                wasm_trap_delete(reinterpret_cast<wasm_trap_t*>(trap));
            } else {
                wasm_byte_vec_t msg;
                wasmtime_error_message(err, &msg);
                error_out = std::string(msg.data, msg.size);
                wasm_byte_vec_delete(&msg);
                wasmtime_error_delete(err);
            }
            wasmtime_linker_delete(linker);
            wasmtime_module_delete(module);
            wasmtime_store_delete(store);
            wasmtime_engine_delete(engine);
            return nullptr;
        }

        // Bundle all handles into a heap-allocated structure stored in
        // WasmHostAPI::wasm_instance_ (freed in ~WasmHostAPI).
        struct WasmtimeBundle {
            wasmtime_engine_t*  engine;
            wasmtime_store_t*   store;
            wasmtime_linker_t*  linker;
            wasmtime_module_t*  module;
            wasmtime_instance_t instance;
        };
        auto* bundle = new WasmtimeBundle{engine, store, linker, module, instance};

        auto api = std::make_unique<WasmHostAPI>(WasmPluginRuntime::WASMTIME, module_name);
        api->setRuntimeInstance(bundle);
        return api;
    }
#endif // THEMIS_WASM_WASMTIME

#if defined(THEMIS_WASM_WASMEDGE)
    // ---- WasmEdge (CNCF) -------------------------------------------------------
    if (runtime == WasmPluginRuntime::WASMEDGE || runtime == WasmPluginRuntime::NONE) {
        WasmEdge_VMContext* vm = WasmEdge_VMCreate(nullptr, nullptr);
        if (!vm) {
            error_out = "WasmEdge_VMCreate failed for plugin: " + module_name;
            return nullptr;
        }

        WasmEdge_Result res = WasmEdge_VMLoadWasmFromFile(vm, wasm_path.c_str());
        if (!WasmEdge_ResultOK(res)) {
            error_out = "WasmEdge_VMLoadWasmFromFile failed: " +
                        std::string(WasmEdge_ResultGetMessage(res));
            WasmEdge_VMDelete(vm);
            return nullptr;
        }

        res = WasmEdge_VMValidate(vm);
        if (!WasmEdge_ResultOK(res)) {
            error_out = "WasmEdge_VMValidate failed: " +
                        std::string(WasmEdge_ResultGetMessage(res));
            WasmEdge_VMDelete(vm);
            return nullptr;
        }

        res = WasmEdge_VMInstantiate(vm);
        if (!WasmEdge_ResultOK(res)) {
            error_out = "WasmEdge_VMInstantiate failed: " +
                        std::string(WasmEdge_ResultGetMessage(res));
            WasmEdge_VMDelete(vm);
            return nullptr;
        }

        auto api = std::make_unique<WasmHostAPI>(WasmPluginRuntime::WASMEDGE, module_name);
        api->setRuntimeInstance(vm);   // freed in ~WasmHostAPI
        return api;
    }
#endif // THEMIS_WASM_WASMEDGE

    error_out = "No WASM runtime available. Rebuild with THEMIS_WASM_WASMTIME=1 or "
                "THEMIS_WASM_WASMEDGE=1. Plugin: " + module_name;
    return nullptr;
}

// ---- WasmHostAPI constructor / destructor -----------------------------------

WasmHostAPI::WasmHostAPI(WasmPluginRuntime runtime, std::string module_name)
    : runtime_(runtime), module_name_(std::move(module_name)) {}

WasmHostAPI::~WasmHostAPI() {
    if (!wasm_instance_) {
      return;
    }

    switch (runtime_) {
#ifdef THEMIS_WASM_WASMTIME
    case WasmPluginRuntime::WASMTIME: {
        // Use RAII deleter to ensure cleanup even in exception scenarios
        UniqueWasmtimeBundle bundle(wasm_instance_, WasmtimeBundleDeleter{});
        // bundle will be automatically cleaned up when it goes out of scope
        wasm_instance_ = nullptr;
        break;
    }
#endif
#ifdef THEMIS_WASM_WASMEDGE
    case WasmPluginRuntime::WASMEDGE:
        WasmEdge_VMDelete(static_cast<WasmEdge_VMContext*>(wasm_instance_));
        break;
#endif
    default:
        break;
    }
    wasm_instance_ = nullptr;
}

// ---- IThemisPlugin forwarding stubs (to be replaced by host-call dispatch) --

const char* WasmHostAPI::getName()    const { return module_name_.c_str(); }
const char* WasmHostAPI::getVersion() const { return "0.0.0-wasm"; }

PluginType WasmHostAPI::getType() const {
    return PluginType::CUSTOM; // resolved from manifest at load time
}

PluginCapabilities WasmHostAPI::getCapabilities() const {
    return {}; // capabilities are frozen from the manifest at registration
}

bool WasmHostAPI::initialize(const char* config_json) {
    return themis_plugin_initialize(config_json) == 1;
}

void WasmHostAPI::shutdown() {
    themis_plugin_shutdown();
}

void* WasmHostAPI::getInstance() {
    return themis_plugin_get_instance(0);
}

std::string WasmHostAPI::saveState() {
    char buf[65536];
    int32_t n = themis_plugin_save_state(buf, sizeof(buf));
    return (n > 0) ? std::string(buf, static_cast<size_t>(n)) : "";
}

bool WasmHostAPI::restoreState(const std::string& state) {
    return themis_plugin_restore_state(state.c_str()) == 1;
}

// ---- Host-function C ABI stubs (called from WASM linear memory) -------------
// In the real integration these would be registered as WASM host imports
// via the runtime API and would call back into the host process.

extern "C" {

uint32_t themis_plugin_get_name(char* buf, uint32_t buf_len) {
    static const char kName[] = "wasm_plugin";
    uint32_t len = static_cast<uint32_t>(sizeof(kName) - 1);
    if (buf && buf_len > 0) {
        uint32_t n = (len < buf_len - 1) ? len : buf_len - 1;
        __builtin_memcpy(buf, kName, n);
        buf[n] = '\0';
        return n;
    }
    return len;
}

uint32_t themis_plugin_get_version(char* buf, uint32_t buf_len) {
    static const char kVer[] = "0.0.0-wasm";
    uint32_t len = static_cast<uint32_t>(sizeof(kVer) - 1);
    if (buf && buf_len > 0) {
        uint32_t n = (len < buf_len - 1) ? len : buf_len - 1;
        __builtin_memcpy(buf, kVer, n);
        buf[n] = '\0';
        return n;
    }
    return len;
}

int32_t themis_plugin_initialize(const char* /*config_json*/) { return 0; }
void    themis_plugin_shutdown(void) {}
void*   themis_plugin_get_instance(int32_t /*capability_id*/) { return nullptr; }
int32_t themis_plugin_save_state(char* buf, uint32_t buf_len) {
    if (buf && buf_len > 0) { buf[0] = '\0'; }
    return 0;
}
int32_t themis_plugin_restore_state(const char* /*state_json*/) { return 0; }

} // extern "C"

#else // !THEMIS_WASM_SUPPORT

// ---------------------------------------------------------------------------
// STUB/SIMULATION NOTE:
// Purpose: Satisfy link-time references from any TU that includes wasm_host_api.h
//          without THEMIS_WASM_SUPPORT so the server binary compiles and starts
//          even when the WebAssembly runtime (Wasmtime/Wasmer) is not available.
// Activation: THEMIS_WASM_SUPPORT is NOT defined at compile time (default).
// Production Delta: loadWasmPlugin() always returns nullptr with an error string.
//                   All `themis_plugin_*` C-ABI entry points return 0/empty.
//                   No WASM plugin can be loaded, initialized, or executed;
//                   any plugin-manager request for a WASM plugin will fail gracefully.
// Removal Plan: Build with -DTHEMIS_WASM_SUPPORT=ON after vcpkg-installing
//               Wasmtime or Wasmer; the real implementation above the #else
//               replaces these stubs.  See src/plugins/FUTURE_ENHANCEMENTS.md §WASMRuntime.
// ---------------------------------------------------------------------------

std::unique_ptr<IThemisPlugin> loadWasmPlugin(
    const std::string& wasm_path,
    const std::string& expected_sha256,
    WasmPluginRuntime  runtime,
    const std::string& module_name,
    std::string&       error_out)
{
    WasmPluginLoadFn fn;
    {
        std::lock_guard<std::mutex> lk(wasmPluginLoadFnMutex());
        fn = wasmPluginLoadFnStorage();
    }
    if (fn) {
        return fn(wasm_path, expected_sha256, runtime, module_name, error_out);
    }
    error_out = "WASM plugin support is not enabled in this build. "
                "Recompile with -DTHEMIS_WASM_SUPPORT to enable. "
                "Requested plugin: " + module_name;
    return nullptr;
}

extern "C" {
uint32_t themis_plugin_get_name(char* buf, uint32_t buf_len) {
    if (buf && buf_len > 0) {
      buf[0] = '\0';
    }
    return 0;
}
uint32_t themis_plugin_get_version(char* buf, uint32_t buf_len) {
    if (buf && buf_len > 0) {
      buf[0] = '\0';
    }
    return 0;
}
int32_t themis_plugin_initialize(const char* /*config_json*/) { return 0; }
void    themis_plugin_shutdown(void) {}
void*   themis_plugin_get_instance(int32_t /*capability_id*/) { return nullptr; }
int32_t themis_plugin_save_state(char* buf, uint32_t buf_len) {
    if (buf && buf_len > 0) {
      buf[0] = '\0';
    }
    return 0;
}
int32_t themis_plugin_restore_state(const char* /*state_json*/) { return 0; }
} // extern "C"

#endif // THEMIS_WASM_SUPPORT

} // namespace plugins
} // namespace themis

