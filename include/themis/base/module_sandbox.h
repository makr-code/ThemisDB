/**
 * @file module_sandbox.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Module Sandbox & ABI Checker – Phase 4 Production Readiness
//
// ModuleSandbox  – resource-limited, platform-jail for untrusted plugin code
// AbiChecker     – deep ABI compatibility check before hot-reload activation
//
// These components extend the existing ModuleLoader (include/themis/base/module_loader.h)
// with security and safety guarantees needed for production hot-reload.
//
// WASM Runtime Injection (v1.8.0):
// ModuleSandbox::Config now has optional WASM isolation fields.  When
// Config::enable_wasm_isolation is true, launch() creates an inner
// WasmPluginSandbox and injects the best available WasmRuntime from the
// WasmRuntimeInjector registry.  The resulting WasmPluginSandbox is
// accessible via wasmSandbox() for loading .wasm plugin binaries.

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace modules {

// ── Forward-declarations from module_loader.h ────────────────────────────────
struct ModuleMetadata;

// ── Forward-declaration to avoid circular includes ───────────────────────────
// wasm_plugin_sandbox.h already includes this header; the full type is only
// needed in module_sandbox.cpp and by callers that use wasmSandbox().
class WasmPluginSandbox;

// =============================================================================
// AbiChecker – deep ABI-compatibility validation for safe hot-reload
// =============================================================================

/**
 * @brief Result of an ABI compatibility check.
 */
struct AbiCheckResult {
    bool   compatible = false;
    std::string summary;           ///< Human-readable one-liner
    std::vector<std::string> issues; ///< Detailed per-issue descriptions

    bool hasIssues() const noexcept { return !issues.empty(); }
};

/**
 * @brief Performs detailed ABI compatibility checks before a module is
 *        hot-reloaded or activated for the first time.
 *
 * Unlike the basic version-integer comparison in `ModuleLoader::isABICompatible()`,
 * `AbiChecker` verifies:
 *
 *   1. **Major-version match** – different majors = breaking ABI change.
 *   2. **Minor forward-compatibility** – module minor must be ≤ host minor.
 *   3. **Required exported symbols** – all symbols on the mandatory list must
 *      be present in the new module handle.
 *   4. **Deprecated symbol detection** – warns if the module still exports
 *      symbols that were removed in the current host version.
 *   5. **Struct-size sentinels** – optional: if the module exports
 *      `themis_abi_struct_sizes`, compare each entry against the host's
 *      compile-time sizes to catch struct-layout breakage.
 *
 * Usage:
 * @code
 *   AbiChecker checker;
 *   checker.addRequiredSymbol("themis_module_init");
 *   checker.addRequiredSymbol("themis_module_shutdown");
 *   checker.addDeprecatedSymbol("themis_legacy_init_v1");
 *
 *   AbiCheckResult r = checker.check(new_handle, new_metadata, host_metadata);
 *   if (!r.compatible) {
 *       LOG_ERROR("ABI check failed: {}", r.summary);
 *       for (auto& issue : r.issues)
 *           LOG_ERROR("  • {}", issue);
 *   }
 * @endcode
 */
class AbiChecker {
public:
    explicit AbiChecker();
    ~AbiChecker();

    // ── Symbol lists ─────────────────────────────────────────────────────

    /**
     * @brief Add a symbol that every valid module must export.
     */
    void addRequiredSymbol(const std::string& symbol);

    /**
     * @brief Add a symbol that was removed from the host ABI;
     *        warn if the new module still exports it (may indicate stale build).
     */
    void addDeprecatedSymbol(const std::string& symbol);

    /**
     * @brief Convenience: load the default ThemisDB required/deprecated lists.
     *
     * Required:  themis_module_init, themis_module_shutdown,
     *            themis_module_version, themis_api_version_major,
     *            themis_api_version_minor
     * Deprecated: (none in v1.x)
     */
    void useDefaultLists();

    // ── Primary check ────────────────────────────────────────────────────

    /**
     * @brief Run all ABI checks.
     *
     * @param module_handle  OS handle to the newly loaded (but not yet active) module.
     * @param module_meta    Metadata extracted from `module_handle`.
     * @param host_major     ThemisDB host ABI major version.
     * @param host_minor     ThemisDB host ABI minor version.
     */
    AbiCheckResult check(void*                  module_handle,
                         const ModuleMetadata&  module_meta,
                         uint32_t               host_major,
                         uint32_t               host_minor) const;

    // ── Individual sub-checks (exposed for testing) ───────────────────────

    AbiCheckResult checkVersions(const ModuleMetadata& meta,
                                  uint32_t host_major,
                                  uint32_t host_minor) const;

    AbiCheckResult checkRequiredSymbols(void* handle) const;

    AbiCheckResult checkDeprecatedSymbols(void* handle) const;

private:
    std::vector<std::string> required_symbols_;
    std::vector<std::string> deprecated_symbols_;

    // Platform-portable symbol lookup
    static void* resolveSymbol(void* handle, const std::string& name) noexcept;
};

// =============================================================================
// ModuleSandbox – resource-limited OS-level jail for plugin code
// =============================================================================

/**
 * @brief Resource consumption measured inside a sandbox.
 */
struct SandboxStats {
    uint64_t peak_memory_bytes = 0;
    uint64_t cpu_time_ms       = 0;
    uint64_t syscall_count     = 0;
    bool     killed            = false; ///< True if the sandbox was forcibly killed
    std::string kill_reason;
};

/**
 * @brief Isolates a loaded module inside a resource-constrained OS jail.
 *
 * The sandbox applies whichever isolation mechanisms are available on the
 * current platform:
 *
 * | Mechanism          | Linux                  | Windows              |
 * |--------------------|------------------------|----------------------|
 * | Memory limit       | cgroups memory.max     | Job Object           |
 * | CPU limit          | cgroups cpu.max        | Job Object CPU rate  |
 * | Network isolation  | network namespace      | Not supported        |
 * | Syscall filtering  | seccomp-bpf            | Not supported        |
 * | File-system access | bind mounts / chroot   | Not supported        |
 *
 * When a platform does not support a mechanism, it degrades gracefully:
 * the `launched()` flag is set, but the missing constraints are skipped
 * and noted in `launchWarnings()`.
 *
 * Usage:
 * @code
 *   ModuleSandbox::Config cfg;
 *   cfg.max_memory_mb   = 256;
 *   cfg.max_cpu_percent = 25;
 *   cfg.allow_network   = false;
 *
 *   ModuleSandbox sandbox(cfg);
 *   bool ok = sandbox.launch("themis_analytics");
 *   if (!ok) {
 *       LOG_ERROR("Sandbox launch failed: {}", sandbox.lastError());
 *   }
 *
 *   // … module runs inside sandbox …
 *
 *   sandbox.shutdown();
 *   auto stats = sandbox.stats();
 *   LOG_INFO("Peak memory: {} MB", stats.peak_memory_bytes / (1024*1024));
 * @endcode
 */
class ModuleSandbox {
public:
    // ── Filesystem access level ───────────────────────────────────────────
    enum class FilesystemAccess {
        NONE,       ///< No file-system access at all
        READ_ONLY,  ///< Read-only access to the module's own directory
        READ_WRITE, ///< Read-write access to a dedicated work directory
        FULL,       ///< Unrestricted (sandbox disabled for filesystem)
    };

    // ── Configuration ─────────────────────────────────────────────────────
    struct Config {
        size_t max_memory_mb       = 256;   ///< Hard memory limit
        int    max_cpu_percent     = 50;    ///< CPU share (0 = unlimited); used on Windows
        size_t max_cpu_time_seconds = 0;    ///< Hard CPU-time limit in seconds (0 = unlimited); used as RLIMIT_CPU fallback on Linux
        bool   allow_network       = false; ///< Allow outbound network calls
        FilesystemAccess fs_access = FilesystemAccess::READ_ONLY;
        std::string work_directory;         ///< Used when fs_access = READ_WRITE
        std::vector<std::string> allowed_syscalls; ///< Empty = use default allow-list

        std::chrono::seconds shutdown_timeout{10}; ///< Grace period before SIGKILL

        // ── WASM isolation (v1.8.0) ────────────────────────────────────────

        /// Enable WASM-based plugin isolation on top of OS resource limits.
        /// When true, launch() creates an inner WasmPluginSandbox and injects
        /// the best available WasmRuntime from WasmRuntimeInjector.
        bool enable_wasm_isolation = false;

        /// WASM runtime name to inject (empty = auto-select highest-priority
        /// registered backend via WasmRuntimeInjector::create()).
        std::string wasm_runtime_name;

        /// Number of 64-KiB WASM linear-memory pages for the plugin (default
        /// 256 = 16 MiB).  Passed to the inner WasmPluginSandbox.
        uint32_t wasm_linear_memory_pages = 256;

        /// When false (default), imports that are not registered as host
        /// functions cause the WASM module load to fail.
        bool wasm_allow_unregistered_imports = false;

        static Config defaults() { return {}; }
    };

    explicit ModuleSandbox(const Config& config = Config::defaults());
    ~ModuleSandbox();

    // Non-copyable
    ModuleSandbox(const ModuleSandbox&)            = delete;
    ModuleSandbox& operator=(const ModuleSandbox&) = delete;

    // ── Lifecycle ──────────────────────────────────────────────────────────

    /**
     * @brief Apply sandbox constraints to the current process/thread context
     *        and associate the sandbox with @p module_name.
     *
     * On Linux this configures cgroups and sets up seccomp-bpf.
     * On Windows this creates and assigns a Job Object.
     *
     * @param module_name  Human-readable name for logging/stats.
     * @return true on success (constraints applied); false on fatal error.
     */
    bool launch(const std::string& module_name);

    /**
     * @brief Remove all sandbox constraints and release OS resources.
     *
     * Must be called before the module is unloaded.
     */
    void shutdown();

    bool isActive() const noexcept { return active_; }

    // ── State ──────────────────────────────────────────────────────────────

    /**
     * @brief Warnings produced during `launch()` for unsupported mechanisms.
     */
    const std::vector<std::string>& launchWarnings() const noexcept {
        return launch_warnings_;
    }

    const std::string& lastError() const noexcept { return last_error_; }

    // ── Statistics ────────────────────────────────────────────────────────

    /**
     * @brief Sample current resource usage.
     *
     * On Linux reads `/sys/fs/cgroup/…/memory.current` and `cpuacct.usage`.
     * On Windows queries the Job Object.
     */
    SandboxStats stats() const;

    // ── WASM isolation (v1.8.0) ────────────────────────────────────────────

    /**
     * @brief True if WASM isolation is active.
     *
     * WASM isolation is active when Config::enable_wasm_isolation was true
     * *and* a WasmRuntime was successfully injected during launch().
     */
    bool isWasmIsolationActive() const noexcept;

    /**
     * @brief Return the inner WasmPluginSandbox.
     *
     * Non-null only when isWasmIsolationActive() is true.
     * Use this to load .wasm plugin binaries and call their exports.
     */
    WasmPluginSandbox*       wasmSandbox() noexcept;
    const WasmPluginSandbox* wasmSandbox() const noexcept;

private:
    Config                   config_;
    std::string              module_name_;
    bool                     active_       = false;
    std::vector<std::string> launch_warnings_;
    std::string              last_error_;

    // Platform handles (pimpl-lite)
    struct PlatformHandle;
    std::unique_ptr<PlatformHandle> platform_;

    // WASM isolation (v1.8.0)
    std::unique_ptr<WasmPluginSandbox> wasm_sandbox_;
    bool                               wasm_isolation_active_ = false;

    bool applyMemoryLimit();
    bool applyCpuLimit();
    bool applyNetworkIsolation();
    bool applyFilesystemRestrictions();
    bool applySyscallFilter();

#if defined(__linux__)
    /// @brief Set up a cgroup v2 sub-hierarchy for this sandbox instance.
    ///
    /// Creates `/sys/fs/cgroup/themis/<sandbox_id>/`, enables the memory and
    /// cpu controllers in `cgroup.subtree_control`, writes `memory.max`, then
    /// moves the current process into the new cgroup.  If CPU limiting is
    /// enabled, `cpu.max` is written later by applyCpuLimit() when active.
    /// Returns true on success; on failure emits spdlog::warn and the caller
    /// falls back to RLIMIT_* enforcement.
    bool setupCgroupV2();

    /// @brief Remove the cgroup v2 directory created by setupCgroupV2().
    ///
    /// Migrates the current process back to the root cgroup before issuing
    /// rmdir(2) on the sandbox-specific sub-directory.
    void teardownCgroupV2();
#endif
};

} // namespace modules
} // namespace themis
