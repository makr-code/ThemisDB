/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wasm_host_api.h                                    ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:04:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     192                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c5a4a68540  2026-03-15  feat(plugins): complete issue audit — unregisterFactory, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file wasm_host_api.h
 * @brief WASM Host API — maps IThemisPlugin vtable to WASM import functions.
 *
 * This header defines the C-ABI host functions that a WASM plugin module may
 * import.  It intentionally mirrors the IThemisPlugin interface so that native
 * and WASM plugins share a single logical contract.
 *
 * Activation:
 *   Compile with -DTHEMIS_WASM_SUPPORT to enable the WasmHostAPI class and
 *   associated runtime bridging.  When the macro is not defined the header
 *   still compiles but only the extern-"C" function declarations and the
 *   WasmPluginRuntime enum are visible.
 *
 * WASM-side usage (wat/C):
 *   The following symbols must be imported from the "themis" module:
 *     (import "themis" "themis_plugin_get_name"    (func ...))
 *     (import "themis" "themis_plugin_get_version" (func ...))
 *     (import "themis" "themis_plugin_initialize"  (func ...))
 *     (import "themis" "themis_plugin_shutdown"    (func ...))
 *     (import "themis" "themis_plugin_get_instance"(func ...))
 *     (import "themis" "themis_plugin_save_state"  (func ...))
 *     (import "themis" "themis_plugin_restore_state"(func ...))
 *
 * @note Actual WASM runtime instantiation (Wasmtime / WasmEdge) is handled by
 *       wasm_plugin_loader.cpp once THEMIS_WASM_SUPPORT is defined and the
 *       chosen runtime library is linked.
 *
 * @see src/plugins/wasm_plugin_loader.cpp
 * @see include/plugins/plugin_interface.h
 */

#include "plugins/plugin_interface.h"
#include <cstdint>
#include <string>

namespace themis {
namespace plugins {

// ============================================================================
// WASM runtime selector
// ============================================================================

/**
 * @brief Identifies the WASM execution backend.
 *
 * Populated from PluginManifest::runtime at load time.
 */
enum class WasmPluginRuntime {
    NONE,       ///< Not a WASM plugin (native dlopen path).
    WASMTIME,   ///< Wasmtime (Bytecode Alliance).
    WASMEDGE,   ///< WasmEdge (CNCF).
};

// ============================================================================
// Host-function C ABI (always visible regardless of THEMIS_WASM_SUPPORT)
// ============================================================================

extern "C" {

/**
 * @brief Host import: return the plugin's null-terminated name.
 * Writes up to `buf_len - 1` bytes into @p buf and null-terminates.
 * @return Number of bytes written (excluding null terminator).
 */
uint32_t themis_plugin_get_name(char* buf, uint32_t buf_len);

/**
 * @brief Host import: return the plugin's null-terminated version string.
 * Writes up to `buf_len - 1` bytes into @p buf and null-terminates.
 * @return Number of bytes written (excluding null terminator).
 */
uint32_t themis_plugin_get_version(char* buf, uint32_t buf_len);

/**
 * @brief Host import: initialize the plugin with a JSON config string.
 * @param config_json  Pointer into WASM linear memory; null-terminated.
 * @return 1 on success, 0 on failure.
 */
int32_t themis_plugin_initialize(const char* config_json);

/**
 * @brief Host import: shut down the plugin and release resources.
 */
void themis_plugin_shutdown(void);

/**
 * @brief Host import: obtain a typed instance pointer (capability-gated).
 * @param capability_id  Numeric capability identifier from the manifest.
 * @return Opaque pointer; null if the capability was not declared.
 */
void* themis_plugin_get_instance(int32_t capability_id);

/**
 * @brief Host import: serialise plugin state to a JSON string.
 * Writes to @p buf; caller must supply a buffer of at least @p buf_len bytes.
 * @return Actual bytes written (0 on empty state, negative on error).
 */
int32_t themis_plugin_save_state(char* buf, uint32_t buf_len);

/**
 * @brief Host import: restore plugin state from a JSON string.
 * @param state_json  Pointer into WASM linear memory; null-terminated.
 * @return 1 on success, 0 on failure.
 */
int32_t themis_plugin_restore_state(const char* state_json);

} // extern "C"

// ============================================================================
// WasmHostAPI — runtime bridge (only available when WASM support is enabled)
// ============================================================================

#ifdef THEMIS_WASM_SUPPORT

/**
 * @brief Bridges IThemisPlugin vtable calls to WASM import functions.
 *
 * WasmHostAPI wraps a loaded WASM module instance and exposes the same
 * IThemisPlugin interface as native plugins, delegating each call through
 * the host-function C ABI above.  The class is instantiated by
 * WasmPluginLoader once the WASM module has been validated and instantiated
 * by the chosen runtime (Wasmtime or WasmEdge).
 *
 * Thread-safety: All public methods are thread-safe; each call acquires only
 *                the shared-reader lock on the registry, consistent with the
 *                shared_mutex upgrade in PluginRegistry.
 */
class WasmHostAPI : public IThemisPlugin {
public:
    /**
     * @brief Construct a WASM bridge for an already-instantiated module.
     *
     * @param runtime     Backend that hosts the WASM module.
     * @param module_name Human-readable name from the plugin manifest.
     */
    explicit WasmHostAPI(WasmPluginRuntime runtime,
                         std::string module_name);
    ~WasmHostAPI() override;

    // IThemisPlugin interface — each method delegates to the corresponding
    // themis_plugin_* host import function.
    const char* getName()    const override;
    const char* getVersion() const override;
    PluginType  getType()    const override;
    PluginCapabilities getCapabilities() const override;
    bool   initialize(const char* config_json) override;
    void   shutdown() override;
    void*  getInstance() override;
    std::string saveState() override;
    bool   restoreState(const std::string& state) override;

    /**
     * @brief Return the WASM runtime backing this instance.
     */
    WasmPluginRuntime runtime() const noexcept { return runtime_; }

private:
    WasmPluginRuntime runtime_;
    std::string       module_name_;
    // Opaque runtime handle; cast to the concrete runtime type in the .cpp.
    void*             wasm_instance_{nullptr};
};

#endif // THEMIS_WASM_SUPPORT

} // namespace plugins
} // namespace themis
