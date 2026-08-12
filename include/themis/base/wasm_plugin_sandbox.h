/**
 * @file wasm_plugin_sandbox.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB WASM Plugin Sandbox
//
// Provides memory-safe, capability-controlled isolation for untrusted plugin
// code packaged as WebAssembly (.wasm) modules.
//
// Design goals:
//   1. **Format validation** – reject non-WASM binaries before any execution.
//   2. **Linear memory isolation** – each plugin gets its own heap; the host
//      memory is never directly accessible from the plugin.
//   3. **Capability model** – the host explicitly allowlists functions exported
//      to the plugin (imports from the plugin's perspective).
//   4. **Resource limits** – integrate with ModuleSandbox for OS-level CPU and
//      memory capping on top of WASM linear memory bounds.
//   5. **Runtime agnosticism** – the API is runtime-neutral; a concrete
//      WasmRuntime implementation can be injected (Wasmtime, WasmEdge, wasm3,
//      etc.).  Without a runtime the sandbox still validates format and
//      enforces the host-function allowlist.
//
// This is an additive, non-breaking API surface; the existing ModuleLoader and
// ModuleSandbox interfaces are unchanged.
//
// See src/base/ROADMAP.md – Phase 3: Marketplace & Sandboxing
// See src/base/wasm_plugin_sandbox.cpp for the implementation.

#pragma once

#include "themis/base/module_sandbox.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace modules {

// =============================================================================
// WasmModuleInfo – metadata parsed from a .wasm binary header
// =============================================================================

/**
 * @brief Lightweight metadata extracted from a WebAssembly binary.
 *
 * Only the module header and the custom "name" section (if present) are read;
 * no full decode of the module is performed at this stage.
 */
struct WasmModuleInfo {
    bool    valid           = false;  ///< true iff the binary starts with `\0asm`
    uint32_t wasm_version   = 0;      ///< WASM binary format version (usually 1)
    size_t   byte_size      = 0;      ///< Total byte size of the .wasm binary
    std::string module_name;          ///< From custom "name" section, if present
    std::vector<std::string> imports; ///< Import descriptors ("module.name")
    std::vector<std::string> exports; ///< Export names found in the export section

    /// @brief Human-readable one-liner summary.
    std::string summary() const;
};

// =============================================================================
// WasmHostFunction – a host-side function exposed to the WASM plugin
// =============================================================================

/**
 * @brief Describes one host function that a WASM plugin may call.
 *
 * The function receives a pointer to the plugin's linear memory (so it can
 * read/write plugin-owned data safely) and an opaque argument blob that the
 * WASM module passes via its stack.
 *
 * @param linear_memory  Pointer to the plugin's linear memory region.
 * @param memory_size    Byte size of the linear memory.
 * @param args           Serialised argument blob (caller-defined format).
 * @param out            Output blob (caller-defined format); may be empty.
 * @return true on success; false signals a trap.
 */
using WasmHostFn = std::function<bool(
    uint8_t*           linear_memory,
    size_t             memory_size,
    const std::vector<uint8_t>& args,
    std::vector<uint8_t>&       out)>;

/**
 * @brief Registration record for a single host function export.
 */
struct WasmHostFunction {
    std::string module_name;   ///< Import module name (e.g. "themis")
    std::string function_name; ///< Import function name (e.g. "log")
    WasmHostFn  fn;            ///< Handler implementation
    std::string description;   ///< Human-readable documentation
};

// =============================================================================
// WasmCallResult – outcome of a call into a WASM plugin
// =============================================================================

/**
 * @brief Result returned by WasmPluginSandbox::callExport().
 */
struct WasmCallResult {
    bool success = false;
    std::string error;           ///< Non-empty when success == false
    std::vector<uint8_t> output; ///< Return value blob (plugin-defined)
    uint64_t duration_us = 0;    ///< Wall-clock time in microseconds
};

// =============================================================================
// WasmRuntime – abstract interface for pluggable WASM execution engines
// =============================================================================

/**
 * @brief Abstract base for a concrete WASM execution engine.
 *
 * Implementors wrap Wasmtime, WasmEdge, wasm3, or any other engine.
 * When no runtime is registered, WasmPluginSandbox operates in
 * **validation-only mode**: it checks the binary format and enforces the
 * host-function allowlist but cannot execute WASM code.
 */
class WasmRuntime {
public:
    virtual ~WasmRuntime() = default;

    /**
     * @brief Instantiate a WASM module from its binary bytes.
     *
     * @param wasm_bytes    Raw .wasm binary.
     * @param host_fns      Host functions to bind as imports.
     * @param linear_memory Pre-allocated linear memory for the module.
     * @param memory_size   Size of linear_memory in bytes.
     * @return true on success.
     */
    [[nodiscard]] virtual bool instantiate(
        const std::vector<uint8_t>&         wasm_bytes,
        const std::vector<WasmHostFunction>& host_fns,
        uint8_t*                             linear_memory,
        size_t                               memory_size) = 0;

    /**
     * @brief Call a named exported function.
     *
     * @param export_name  Name of the WASM export to invoke.
     * @param args         Serialised argument blob.
     * @param out          Output blob filled by the runtime.
     * @return true on success; false on trap or missing export.
     */
    [[nodiscard]] virtual bool call(const std::string&          export_name,
                      const std::vector<uint8_t>& args,
                      std::vector<uint8_t>&        out) = 0;

    /// @brief Release all resources held by this runtime instance.
    virtual void destroy() = 0;

    /// @brief Human-readable name of the engine (e.g. "wasmtime-0.35").
    [[nodiscard]] virtual std::string engineName() const = 0;
};

// =============================================================================
// WasmPluginSandbox – WASM-based isolation for untrusted plugin code
// =============================================================================

/**
 * @brief Isolates an untrusted plugin inside a WASM sandbox with an explicit
 *        capability model and OS-level resource limits.
 *
 * ## Isolation guarantees
 * 1. **Memory isolation** – the plugin operates only within its own linear
 *    memory; host addresses are never reachable.
 * 2. **Capability model** – the plugin can call only the host functions
 *    explicitly registered via addHostFunction().
 * 3. **Resource limits** – a ModuleSandbox is created internally; CPU and
 *    memory limits from WasmPluginSandbox::Config are applied at the OS level
 *    in addition to the WASM linear-memory bound.
 * 4. **Format validation** – non-WASM binaries are rejected before loading.
 *
 * ## Usage
 * @code
 *   WasmPluginSandbox::Config cfg;
 *   cfg.linear_memory_pages = 16;  // 1 MiB (16 × 64 KiB)
 *   cfg.max_memory_mb       = 64;  // OS hard cap
 *
 *   WasmPluginSandbox sandbox(cfg);
 *
 *   // Register allowed host functions
 *   sandbox.addHostFunction({
 *       "themis", "log",
 *       [](uint8_t* mem, size_t, const auto& args, auto& out) {
 *           // safe: args contains plugin-owned string bytes
 *           return true;
 *       },
 *       "Write a log message"
 *   });
 *
 *   // Load the plugin binary
 *   auto result = sandbox.loadFromFile("/plugins/my_plugin.wasm");
 *   if (!result) {
 *       LOG_ERROR("WASM load failed: {}", sandbox.lastError());
 *   }
 *
 *   // Call an export
 *   WasmCallResult r = sandbox.callExport("process", {});
 *   if (!r.success) { LOG_ERROR("Call failed: {}", r.error); }
 *
 *   sandbox.unload();
 * @endcode
 */
class WasmPluginSandbox {
public:
    // ── Configuration ─────────────────────────────────────────────────────

    /**
     * @brief Configuration for the WASM plugin sandbox.
     */
    struct Config {
        /// Linear memory: number of 64-KiB WASM pages (default 256 = 16 MiB).
        uint32_t linear_memory_pages = 256;

        /// Hard OS-level memory cap (MiB). Passed to the inner ModuleSandbox.
        size_t max_memory_mb = 64;

        /// Hard OS-level CPU time limit in seconds (0 = unlimited).
        size_t max_cpu_time_seconds = 0;

        /// Whether to allow the plugin to call any non-registered host function.
        /// When false (default), unregistered imports cause load to fail.
        bool allow_unregistered_imports = false;

        /// Path to the .wasm binary (set by loadFromFile; empty when loading
        /// from bytes directly).
        std::string wasm_path;

        /// Total instruction fuel budget for this sandbox (0 = unlimited).
        ///
        /// Each callExport() invocation deducts @ref fuel_check_interval units
        /// from the budget. When the budget reaches zero, callExport() returns a
        /// structured fuel-exhaustion error without invoking the runtime, bounding
        /// runaway plugin execution. Reset the budget by reloading the module.
        uint64_t max_instructions = 0;

        /// Fuel units consumed per callExport() invocation (default: 1).
        ///
        /// In a real WASM interpreter this would correspond to how often fuel is
        /// decremented in the bytecode dispatch loop. At the sandbox boundary
        /// level it sets the granularity of the per-call fuel deduction.
        ///
        /// @note A value of 0 is treated as 1 (minimum cost of one unit per
        ///       call) to avoid infinite free calls when a budget is set.
        uint64_t fuel_check_interval = 1;

        static Config defaults() { return {}; }
    };

    // ── Construction ───────────────────────────────────────────────────────

    explicit WasmPluginSandbox(const Config& config = Config::defaults());
    ~WasmPluginSandbox();

    WasmPluginSandbox(const WasmPluginSandbox&)            = delete;
    WasmPluginSandbox& operator=(const WasmPluginSandbox&) = delete;

    // ── Runtime injection ─────────────────────────────────────────────────

    /**
     * @brief Inject a concrete WASM execution engine.
     *
     * If no runtime is set the sandbox operates in **validation-only mode**:
     * it validates the binary and the host-function allowlist but
     * callExport() will return an error.
     *
     * @param runtime  Owning pointer to the runtime.
     */
    void setRuntime(std::unique_ptr<WasmRuntime> runtime);

    /// @brief Return true if a runtime has been injected.
    [[nodiscard]] bool hasRuntime() const noexcept;

    /// @brief Return the engine name (empty string if no runtime).
    [[nodiscard]] std::string engineName() const;

    // ── Host-function allowlist ───────────────────────────────────────────

    /**
     * @brief Register a host function that the WASM plugin is allowed to call.
     *
     * Must be called *before* loadFromFile() / loadFromBytes().
     */
    void addHostFunction(WasmHostFunction fn);

    /**
     * @brief Remove all registered host functions.
     */
    void clearHostFunctions();

    /// @brief Return the number of registered host functions.
    [[nodiscard]] size_t hostFunctionCount() const noexcept;

    // ── Loading ────────────────────────────────────────────────────────────

    /**
     * @brief Validate and load a .wasm plugin from a file path.
     *
     * Steps:
     *  1. Read the file into memory.
     *  2. Validate the WASM magic bytes and version.
     *  3. Parse import/export sections to check against the allowlist.
     *  4. Allocate the linear memory arena.
     *  5. Instantiate via the injected WasmRuntime (if any).
     *  6. Launch the inner ModuleSandbox resource limits.
     *
     * @return true on success.
     */
    [[nodiscard]] bool loadFromFile(const std::string& path);

    /**
     * @brief Validate and load a .wasm plugin from an in-memory byte buffer.
     *
     * Same steps as loadFromFile() but reads from @p bytes directly.
     *
     * @return true on success.
     */
    [[nodiscard]] bool loadFromBytes(const std::vector<uint8_t>& bytes,
                       const std::string& module_name = "anonymous");

    /**
     * @brief Unload the plugin and release all resources.
     */
    void unload();

    // ── Execution ──────────────────────────────────────────────────────────

    /**
     * @brief Call a named export in the loaded WASM plugin.
     *
     * Requires a runtime to be injected; otherwise returns an error result.
     *
     * @param export_name  Name of the WASM export function.
     * @param args         Serialised argument blob (format is plugin-defined).
     * @return WasmCallResult describing success, output, duration.
     */
    [[nodiscard]] WasmCallResult callExport(const std::string&          export_name,
                              const std::vector<uint8_t>& args = {});

    // ── State ──────────────────────────────────────────────────────────────

    [[nodiscard]] bool isLoaded() const noexcept  { return loaded_; }
    [[nodiscard]] const std::string& lastError() const noexcept { return last_error_; }

    /// @brief Metadata parsed from the loaded .wasm binary.
    [[nodiscard]] const WasmModuleInfo& moduleInfo() const noexcept { return module_info_; }

    /// @brief Warnings produced during load (e.g. sandbox limitations).
    [[nodiscard]] const std::vector<std::string>& loadWarnings() const noexcept {
        return load_warnings_;
    }

    // ── Linear memory access (for testing / host-function helpers) ────────

    /**
     * @brief Return a pointer to the plugin's linear memory.
     * @return nullptr if not loaded.
     */
    [[nodiscard]] const uint8_t* linearMemory() const noexcept;
    [[nodiscard]] uint8_t*       linearMemory() noexcept;

    /// @brief Byte size of the linear memory arena.
    [[nodiscard]] size_t linearMemorySize() const noexcept;

    // ── Statistics ─────────────────────────────────────────────────────────

    struct Stats {
        uint64_t calls_attempted = 0; ///< Total callExport() attempts
        uint64_t calls_succeeded = 0; ///< Calls that returned success
        uint64_t calls_trapped   = 0; ///< Calls that trapped / errored
        uint64_t total_call_us   = 0; ///< Accumulated call duration (µs)
    };

    [[nodiscard]] Stats stats() const noexcept { return stats_; }

    // ── Fuel / instruction metering ────────────────────────────────────────

    /**
     * @brief Return the number of fuel units remaining in the sandbox budget.
     *
     * When @ref Config::max_instructions is 0 (unlimited), this always returns
     * `UINT64_MAX`. After a fuel-exhaustion error the value is 0.
     *
     * Fuel is reset to @ref Config::max_instructions when a new module is loaded
     * via loadFromBytes() / loadFromFile().
     */
    [[nodiscard]] uint64_t remainingFuel() const noexcept;

private:
    Config                           config_;
    std::unique_ptr<WasmRuntime>     runtime_;
    std::vector<WasmHostFunction>    host_fns_;
    WasmModuleInfo                   module_info_;
    std::vector<uint8_t>             wasm_bytes_;
    std::unique_ptr<uint8_t[]>       linear_memory_;
    size_t                           linear_memory_size_ = 0;
    bool                             loaded_             = false;
    std::string                      last_error_;
    std::vector<std::string>         load_warnings_;
    Stats                            stats_{};
    std::unique_ptr<ModuleSandbox>   os_sandbox_;
    uint64_t                         fuel_remaining_     = 0; ///< Remaining fuel units (UINT64_MAX when unlimited)

    // ── Helpers ──────────────────────────────────────────────────────────
    bool validateWasmHeader(const std::vector<uint8_t>& bytes);
    bool parseImportsExports(const std::vector<uint8_t>& bytes);
    bool checkImportAllowlist();
    bool allocateLinearMemory();
    bool launchOsSandbox(const std::string& module_name);
};

// =============================================================================
// WasmModuleValidator – standalone binary format checker (no sandbox overhead)
// =============================================================================

/**
 * @brief Lightweight validator for .wasm binaries (no runtime required).
 *
 * Use this as a fast pre-check before constructing a full WasmPluginSandbox,
 * e.g. in a plugin registry that scans many files.
 */
class WasmModuleValidator {
public:
    /**
     * @brief Check that @p bytes starts with the WASM magic bytes and a
     *        supported version.
     * @return A WasmModuleInfo with valid==true on success.
     */
    [[nodiscard]] static WasmModuleInfo validate(const std::vector<uint8_t>& bytes);

    /**
     * @brief Convenience overload: read the file at @p path and validate.
     */
    [[nodiscard]] static WasmModuleInfo validateFile(const std::string& path);

    /// @brief Return the 4 WASM magic bytes { 0x00, 0x61, 0x73, 0x6d }.
    [[nodiscard]] static const uint8_t* magicBytes() noexcept;
};

} // namespace modules
} // namespace themis
