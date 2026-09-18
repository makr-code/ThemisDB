/**
 * @file base_error_taxonomy.h
 * @brief Explicit failure taxonomy for all ThemisDB base module error classes.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Production Ready
 *
 * Provides standardized integer error codes, human-readable descriptions, and
 * diagnostic-message builders for every failure class emitted by the base
 * module subsystems: loader, sandbox, hot-reload, dependency graph, and remote
 * registry.
 *
 * Taxonomy groups and integer ranges:
 *  - BASE_LOADER_xxx   (1100–1149): load-stage failures, signature rejection,
 *                                   ABI mismatch, path-not-found
 *  - BASE_SANDBOX_xxx  (1150–1199): launch failure, resource limit violation,
 *                                   timeout, degraded-state
 *  - BASE_RELOAD_xxx   (1200–1249): no-backup, rollback-failed,
 *                                   candidate-load-failed, state-restore-failed
 *  - BASE_DEPENDENCY_xxx (1250–1299): conflict, cycle, missing-required,
 *                                     version-range-mismatch
 *  - BASE_REGISTRY_xxx (1300–1349): network-error, auth-failure,
 *                                   checksum-mismatch, download-failed
 *
 * Design principles:
 *  - No external dependencies — C++17 std only.
 *  - All constants are @c constexpr; zero overhead in production code.
 *  - Each error type has a @c description() and a @c format(...) builder.
 *  - All types live in @c themis::modules::BaseErrorTaxonomy.
 *
 * Usage:
 * @code
 *   using namespace themis::modules::BaseErrorTaxonomy;
 *
 *   // Use a code directly:
 *   if (result.errorCode == BASE_LOADER_PATH_NOT_FOUND::code) { ... }
 *
 *   // Log a formatted diagnostic:
 *   LOG_ERROR(BASE_LOADER_ABI_MISMATCH::format(
 *       "themis_analytics", "1.2", "1.0"));
 *
 *   // Resolve a code to a description at runtime:
 *   std::string msg = resolveDescription(BASE_SANDBOX_LAUNCH_FAILED::code);
 * @endcode
 */

#pragma once

#include <string>
#include <string_view>

namespace themis {
namespace modules {

/**
 * @namespace themis::modules::BaseErrorTaxonomy
 * @brief Standardized error codes and diagnostic builders for the base module.
 *
 * Each inner struct contains:
 *  - @c code              — @c constexpr int unique error code.
 *  - @c description()     — @c static std::string_view one-line description.
 *  - @c remediationHint() — @c static std::string_view actionable operator hint.
 *  - @c format(...)       — @c static std::string diagnostic with context args.
 */
namespace BaseErrorTaxonomy {

// =============================================================================
// BASE_LOADER_xxx  (1100–1149)  — load-stage failures
// =============================================================================

/**
 * @brief Module binary was not found at the supplied path.
 * @code BASE_LOADER_PATH_NOT_FOUND::code == 1100 @endcode
 */
struct BASE_LOADER_PATH_NOT_FOUND {
    static constexpr int code = 1100;

    /// @brief One-line description.
    static constexpr std::string_view description() noexcept {
        return "module binary not found at the supplied path";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Verify that the module artifact exists at the configured path "
               "and that the process has read permission. Check deployment "
               "scripts for missing copy/install steps.";
    }

    /**
     * @brief Build a formatted diagnostic message.
     * @param module_name  Logical module name.
     * @param path         Filesystem path that was not found.
     * @return Formatted diagnostic string.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& module_name,
                              const std::string& path) {
        return "[BASE_LOADER_PATH_NOT_FOUND:" + std::to_string(code) + "] "
               "module '" + module_name + "' binary not found: path='" + path + "'";
    }
};

/**
 * @brief Module signature verification was rejected.
 * @code BASE_LOADER_SIGNATURE_REJECTED::code == 1101 @endcode
 */
struct BASE_LOADER_SIGNATURE_REJECTED {
    static constexpr int code = 1101;

    static constexpr std::string_view description() noexcept {
        return "module signature verification failed";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Re-sign the module artifact with a key trusted by this host. "
               "Check that the signing key has not expired or been revoked, "
               "and that the module binary has not been modified in transit.";
    }

    /**
     * @brief Build a formatted diagnostic message.
     * @param module_name  Logical module name.
     * @param reason       Human-readable rejection reason.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& module_name,
                              const std::string& reason) {
        return "[BASE_LOADER_SIGNATURE_REJECTED:" + std::to_string(code) + "] "
               "module '" + module_name + "' signature rejected: " + reason;
    }
};

/**
 * @brief ABI version of the module binary is incompatible with the host.
 * @code BASE_LOADER_ABI_MISMATCH::code == 1102 @endcode
 */
struct BASE_LOADER_ABI_MISMATCH {
    static constexpr int code = 1102;

    static constexpr std::string_view description() noexcept {
        return "module ABI version is incompatible with the host";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Rebuild the module against the current ThemisDB SDK version "
               "or deploy a host binary that matches the module's ABI. "
               "Confirm that major version numbers align.";
    }

    /**
     * @brief Build a formatted diagnostic message.
     * @param module_name   Logical module name.
     * @param module_abi    ABI version string reported by the module.
     * @param host_abi      ABI version string required by the host.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& module_name,
                              const std::string& module_abi,
                              const std::string& host_abi) {
        return "[BASE_LOADER_ABI_MISMATCH:" + std::to_string(code) + "] "
               "module '" + module_name + "' ABI mismatch: "
               "module=" + module_abi + " host=" + host_abi;
    }
};

/**
 * @brief dlopen / LoadLibrary system call failed.
 * @code BASE_LOADER_LOAD_FAILED::code == 1103 @endcode
 */
struct BASE_LOADER_LOAD_FAILED {
    static constexpr int code = 1103;

    static constexpr std::string_view description() noexcept {
        return "OS-level library load failed";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Check dlerror()/GetLastError() output for the root cause. "
               "Verify that all shared-library dependencies of the module are "
               "present and accessible (use ldd on Linux). Confirm file "
               "permissions and SELinux/AppArmor policy allow loading.";
    }

    /**
     * @param module_name  Logical module name.
     * @param path         Path attempted.
     * @param os_error     dlerror() / GetLastError() message.
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& module_name,
                              const std::string& path,
                              const std::string& os_error) {
        return "[BASE_LOADER_LOAD_FAILED:" + std::to_string(code) + "] "
               "module '" + module_name + "' OS load failed at '" + path
               + "': " + os_error;
    }
};

/**
 * @brief Module initialization function returned a failure code.
 * @code BASE_LOADER_INIT_FAILED::code == 1104 @endcode
 */
struct BASE_LOADER_INIT_FAILED {
    static constexpr int code = 1104;

    static constexpr std::string_view description() noexcept {
        return "module initialization function returned failure";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Inspect the module's init function logs for the root cause. "
               "Ensure required runtime resources (config, connections) are "
               "available at load time. Check module documentation for "
               "prerequisites and environment variables.";
    }

    /**
     * @param module_name   Logical module name.
     * @param init_symbol   Name of the init symbol that was called.
     * @param return_code   Return value from the init function.
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& module_name,
                              const std::string& init_symbol,
                              int return_code) {
        return "[BASE_LOADER_INIT_FAILED:" + std::to_string(code) + "] "
               "module '" + module_name + "' init symbol '"
               + init_symbol + "' returned " + std::to_string(return_code);
    }
};

/**
 * @brief A staged-loading health check failed during module activation.
 * @code BASE_LOADER_HEALTH_CHECK_FAILED::code == 1105 @endcode
 */
struct BASE_LOADER_HEALTH_CHECK_FAILED {
    static constexpr int code = 1105;

    static constexpr std::string_view description() noexcept {
        return "staged-loading health check failed during activation";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Review the health check name and detail in the diagnostic "
               "message to identify the failing condition. Verify that the "
               "module's required services and endpoints are reachable before "
               "activation, and that staged-loading prerequisites are met.";
    }

    /**
     * @param module_name   Logical module name.
     * @param check_name    Name of the health check that failed.
     * @param detail        Diagnostic detail from the health check.
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& module_name,
                              const std::string& check_name,
                              const std::string& detail) {
        return "[BASE_LOADER_HEALTH_CHECK_FAILED:" + std::to_string(code) + "] "
               "module '" + module_name + "' health check '" + check_name
               + "' failed: " + detail;
    }
};

// =============================================================================
// BASE_SANDBOX_xxx  (1150–1199)  — sandbox lifecycle failures
// =============================================================================

/**
 * @brief ModuleSandbox::launch() returned false.
 * @code BASE_SANDBOX_LAUNCH_FAILED::code == 1150 @endcode
 */
struct BASE_SANDBOX_LAUNCH_FAILED {
    static constexpr int code = 1150;

    static constexpr std::string_view description() noexcept {
        return "module sandbox launch failed";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Check the sandbox last error detail for the specific failure. "
               "Verify that seccomp/AppArmor/SELinux profiles allow the "
               "required sandbox syscalls. Inspect OS resource limits (ulimit) "
               "and ensure sufficient memory and file descriptors are available.";
    }

    /**
     * @param module_name  Name of the module being sandboxed.
     * @param last_error   ModuleSandbox::lastError() string.
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& module_name,
                              const std::string& last_error) {
        return "[BASE_SANDBOX_LAUNCH_FAILED:" + std::to_string(code) + "] "
               "sandbox launch failed for module '" + module_name
               + "': " + last_error;
    }
};

/**
 * @brief Sandbox hit its configured resource limit (memory or CPU).
 * @code BASE_SANDBOX_RESOURCE_LIMIT::code == 1151 @endcode
 */
struct BASE_SANDBOX_RESOURCE_LIMIT {
    static constexpr int code = 1151;

    static constexpr std::string_view description() noexcept {
        return "module sandbox exceeded configured resource limit";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Increase the sandbox resource limit in the module configuration "
               "if the workload legitimately requires more resources, or "
               "investigate the module for resource leaks and unbounded "
               "allocations.";
    }

    /**
     * @param module_name   Module name.
     * @param resource      "memory" or "cpu".
     * @param limit_value   Configured limit (e.g. "256 MB").
     * @param measured      Observed value.
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& module_name,
                              const std::string& resource,
                              const std::string& limit_value,
                              const std::string& measured) {
        return "[BASE_SANDBOX_RESOURCE_LIMIT:" + std::to_string(code) + "] "
               "module '" + module_name + "' sandbox " + resource
               + " limit exceeded: limit=" + limit_value
               + " measured=" + measured;
    }
};

/**
 * @brief Sandbox operation timed out before completing.
 * @code BASE_SANDBOX_TIMEOUT::code == 1152 @endcode
 */
struct BASE_SANDBOX_TIMEOUT {
    static constexpr int code = 1152;

    static constexpr std::string_view description() noexcept {
        return "module sandbox operation timed out";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Increase the sandbox timeout in the module configuration if the "
               "operation is expected to take longer. Investigate the module for "
               "deadlocks, blocking I/O without timeout, or runaway computation.";
    }

    /**
     * @param module_name     Module name.
     * @param timeout_seconds Configured timeout value.
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& module_name,
                              unsigned int timeout_seconds) {
        return "[BASE_SANDBOX_TIMEOUT:" + std::to_string(code) + "] "
               "module '" + module_name + "' sandbox timed out after "
               + std::to_string(timeout_seconds) + "s";
    }
};

/**
 * @brief Sandbox is in a degraded state — constraints are partially applied.
 * @code BASE_SANDBOX_DEGRADED::code == 1153 @endcode
 */
struct BASE_SANDBOX_DEGRADED {
    static constexpr int code = 1153;

    static constexpr std::string_view description() noexcept {
        return "module sandbox is in degraded state (partial constraints only)";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Review the unsupported constraint warnings in the diagnostic. "
               "Upgrade the host kernel or runtime to support the required "
               "sandbox capabilities, or accept the degraded state only if the "
               "missing constraints are not security-critical for this module.";
    }

    /**
     * @param module_name   Module name.
     * @param warnings      Comma-separated list of launchWarnings().
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& module_name,
                              const std::string& warnings) {
        return "[BASE_SANDBOX_DEGRADED:" + std::to_string(code) + "] "
               "module '" + module_name + "' sandbox degraded; "
               "unsupported constraints: [" + warnings + "]";
    }
};

/**
 * @brief Sandbox::stats() was called on an inactive (unstarted) sandbox.
 * @code BASE_SANDBOX_INACTIVE_STATS::code == 1154 @endcode
 */
struct BASE_SANDBOX_INACTIVE_STATS {
    static constexpr int code = 1154;

    static constexpr std::string_view description() noexcept {
        return "stats() called on an inactive sandbox (launch() not completed)";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Ensure that ModuleSandbox::launch() is called and returns true "
               "before invoking stats(). Check the sandbox lifecycle in the "
               "calling code path.";
    }

    /**
     * @param module_name  Module name.
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& module_name) {
        return "[BASE_SANDBOX_INACTIVE_STATS:" + std::to_string(code) + "] "
               "sandbox stats requested for inactive module '"
               + module_name + "' (was launch() called?)";
    }
};

// =============================================================================
// BASE_RELOAD_xxx  (1200–1249)  — hot-reload lifecycle failures
// =============================================================================

/**
 * @brief rollback() was requested but no backup slot exists.
 * @code BASE_RELOAD_NO_BACKUP::code == 1200 @endcode
 */
struct BASE_RELOAD_NO_BACKUP {
    static constexpr int code = 1200;

    static constexpr std::string_view description() noexcept {
        return "rollback requested but no backup version is available";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "A rollback is only possible after at least one successful "
               "reload has stored a backup slot. Restore the module manually "
               "from the deployment artifact store, or perform a fresh "
               "deployment of the last known-good version.";
    }

    /**
     * @param module_name  Module name.
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& module_name) {
        return "[BASE_RELOAD_NO_BACKUP:" + std::to_string(code) + "] "
               "rollback unavailable for module '" + module_name
               + "': no backup slot stored";
    }
};

/**
 * @brief rollback() failed — backup binary could not be re-activated.
 * @code BASE_RELOAD_ROLLBACK_FAILED::code == 1201 @endcode
 */
struct BASE_RELOAD_ROLLBACK_FAILED {
    static constexpr int code = 1201;

    static constexpr std::string_view description() noexcept {
        return "rollback failed — backup version could not be re-activated";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "The backup binary may be corrupted or missing. Manually redeploy "
               "the last known-good module artifact. Inspect loader and sandbox "
               "logs for the root cause of the re-activation failure.";
    }

    /**
     * @param module_name   Module name.
     * @param backup_path   Path to the backup binary.
     * @param reason        Failure reason.
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& module_name,
                              const std::string& backup_path,
                              const std::string& reason) {
        return "[BASE_RELOAD_ROLLBACK_FAILED:" + std::to_string(code) + "] "
               "rollback of '" + module_name + "' from backup '"
               + backup_path + "' failed: " + reason;
    }
};

/**
 * @brief Candidate module at new_path could not be loaded during hot-reload.
 * @code BASE_RELOAD_CANDIDATE_LOAD_FAILED::code == 1202 @endcode
 */
struct BASE_RELOAD_CANDIDATE_LOAD_FAILED {
    static constexpr int code = 1202;

    static constexpr std::string_view description() noexcept {
        return "candidate module could not be loaded during hot-reload";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Verify that the new module artifact at the supplied path is a "
               "valid, signed, and ABI-compatible binary. The hot-reload "
               "manager will retain the previous version. Fix the artifact and "
               "trigger a new reload.";
    }

    /**
     * @param module_name  Module name.
     * @param new_path     Path to the candidate binary.
     * @param reason       Load failure reason.
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& module_name,
                              const std::string& new_path,
                              const std::string& reason) {
        return "[BASE_RELOAD_CANDIDATE_LOAD_FAILED:" + std::to_string(code) + "] "
               "hot-reload candidate for '" + module_name + "' at '"
               + new_path + "' failed to load: " + reason;
    }
};

/**
 * @brief StateRestoreCallback returned false after a successful reload.
 * @code BASE_RELOAD_STATE_RESTORE_FAILED::code == 1203 @endcode
 */
struct BASE_RELOAD_STATE_RESTORE_FAILED {
    static constexpr int code = 1203;

    static constexpr std::string_view description() noexcept {
        return "module state restoration failed after successful reload";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "The module was successfully reloaded but its state restore "
               "callback reported failure. Inspect module-specific logs for the "
               "restoration error. The module may be in a partial state; "
               "consider triggering a rollback to the backup version.";
    }

    /**
     * @param module_name  Module name.
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& module_name) {
        return "[BASE_RELOAD_STATE_RESTORE_FAILED:" + std::to_string(code) + "] "
               "state restore callback failed for module '"
               + module_name + "' after reload";
    }
};

/**
 * @brief reloadModule() was called for an unregistered module name.
 * @code BASE_RELOAD_NOT_REGISTERED::code == 1204 @endcode
 */
struct BASE_RELOAD_NOT_REGISTERED {
    static constexpr int code = 1204;

    static constexpr std::string_view description() noexcept {
        return "reload attempted on a module that is not registered";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Call HotReloadManager::registerModule() for this module before "
               "invoking reloadModule(). Check for typos in the module name "
               "and verify the registration lifecycle in the startup code.";
    }

    /**
     * @param module_name  Module name that was not found.
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& module_name) {
        return "[BASE_RELOAD_NOT_REGISTERED:" + std::to_string(code) + "] "
               "reloadModule() called for unregistered module '"
               + module_name + "'";
    }
};

// =============================================================================
// BASE_DEPENDENCY_xxx  (1250–1299)  — dependency resolution failures
// =============================================================================

/**
 * @brief Two modules declare conflicting version requirements for a shared dep.
 * @code BASE_DEPENDENCY_CONFLICT::code == 1250 @endcode
 */
struct BASE_DEPENDENCY_CONFLICT {
    static constexpr int code = 1250;

    static constexpr std::string_view description() noexcept {
        return "conflicting version requirements for a shared dependency";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Resolve the version conflict by aligning the modules' "
               "dependency declarations to a compatible version range, or by "
               "upgrading the shared dependency to a version that satisfies "
               "all constraints. Use the dependency graph export for analysis.";
    }

    /**
     * @param dep_name   Name of the shared dependency.
     * @param mod_a      First requiring module and its constraint.
     * @param mod_b      Second requiring module and its constraint.
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& dep_name,
                              const std::string& mod_a,
                              const std::string& mod_b) {
        return "[BASE_DEPENDENCY_CONFLICT:" + std::to_string(code) + "] "
               "dependency '" + dep_name + "' version conflict between '"
               + mod_a + "' and '" + mod_b + "'";
    }
};

/**
 * @brief A cyclic dependency was detected among registered modules.
 * @code BASE_DEPENDENCY_CYCLE::code == 1251 @endcode
 */
struct BASE_DEPENDENCY_CYCLE {
    static constexpr int code = 1251;

    static constexpr std::string_view description() noexcept {
        return "cyclic dependency detected in module graph";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Use the DOT/JSON/ASCII export from PluginDependencyGraph to "
               "visualize the cycle and identify the modules involved. Break "
               "the cycle by introducing an interface module or re-designing "
               "the dependency direction.";
    }

    /**
     * @param cycle_str  Human-readable cycle description, e.g. "A→B→C→A".
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& cycle_str) {
        return "[BASE_DEPENDENCY_CYCLE:" + std::to_string(code) + "] "
               "dependency cycle detected: " + cycle_str;
    }
};

/**
 * @brief A required module dependency is absent from the registry.
 * @code BASE_DEPENDENCY_MISSING_REQUIRED::code == 1252 @endcode
 */
struct BASE_DEPENDENCY_MISSING_REQUIRED {
    static constexpr int code = 1252;

    static constexpr std::string_view description() noexcept {
        return "required module dependency not registered";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Register and load the missing dependency module before loading "
               "the module that requires it. Check the module manifest and "
               "deployment order in the startup configuration.";
    }

    /**
     * @param module_name  Module that has the unsatisfied dependency.
     * @param dep_name     Name of the missing required dependency.
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& module_name,
                              const std::string& dep_name) {
        return "[BASE_DEPENDENCY_MISSING_REQUIRED:" + std::to_string(code) + "] "
               "module '" + module_name + "' requires missing dependency '"
               + dep_name + "'";
    }
};

/**
 * @brief A registered dependency's version does not satisfy declared constraints.
 * @code BASE_DEPENDENCY_VERSION_RANGE_MISMATCH::code == 1253 @endcode
 */
struct BASE_DEPENDENCY_VERSION_RANGE_MISMATCH {
    static constexpr int code = 1253;

    static constexpr std::string_view description() noexcept {
        return "dependency version does not satisfy declared version constraints";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Deploy a version of the dependency that falls within the "
               "declared version range, or relax the requiring module's version "
               "constraint if the installed version is known-compatible.";
    }

    /**
     * @param module_name  Requiring module.
     * @param dep_name     Dependency module name.
     * @param have         Registered version.
     * @param need_min     Required minimum version.
     * @param need_max     Required maximum version (empty = unconstrained).
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& module_name,
                              const std::string& dep_name,
                              const std::string& have,
                              const std::string& need_min,
                              const std::string& need_max) {
        return "[BASE_DEPENDENCY_VERSION_RANGE_MISMATCH:" + std::to_string(code) + "] "
               "module '" + module_name + "' dep '" + dep_name
               + "': have=" + have
               + " need=[" + need_min + "," + need_max + "]";
    }
};

// =============================================================================
// BASE_REGISTRY_xxx  (1300–1349)  — remote registry client failures
// =============================================================================

/**
 * @brief Network error while contacting the remote registry.
 * @code BASE_REGISTRY_NETWORK_ERROR::code == 1300 @endcode
 */
struct BASE_REGISTRY_NETWORK_ERROR {
    static constexpr int code = 1300;

    static constexpr std::string_view description() noexcept {
        return "network error while contacting the remote plugin registry";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Verify network connectivity to the registry URL. Check firewall "
               "rules, DNS resolution, and TLS certificate validity. Inspect "
               "the curl error detail for connection-refused, timeout, or SSL "
               "handshake errors and address the underlying network issue.";
    }

    /**
     * @param registry_url  URL of the registry.
     * @param http_status   HTTP status code (0 if no response received).
     * @param curl_error    cURL error string or OS-level error description.
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& registry_url,
                              int http_status,
                              const std::string& curl_error) {
        return "[BASE_REGISTRY_NETWORK_ERROR:" + std::to_string(code) + "] "
               "registry '" + registry_url + "' HTTP "
               + std::to_string(http_status) + ": " + curl_error;
    }
};

/**
 * @brief Authentication to the remote registry was rejected (401/403).
 * @code BASE_REGISTRY_AUTH_FAILURE::code == 1301 @endcode
 */
struct BASE_REGISTRY_AUTH_FAILURE {
    static constexpr int code = 1301;

    static constexpr std::string_view description() noexcept {
        return "authentication to the remote plugin registry was rejected";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Verify that the registry credentials (token/API key) are "
               "correct and have not expired. Check that the credential has "
               "the required scopes for listing and downloading plugins. "
               "Rotate the credential if it may have been compromised.";
    }

    /**
     * @param registry_url  URL of the registry.
     * @param http_status   HTTP status code (typically 401 or 403).
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& registry_url,
                              int http_status) {
        return "[BASE_REGISTRY_AUTH_FAILURE:" + std::to_string(code) + "] "
               "registry '" + registry_url + "' auth rejected (HTTP "
               + std::to_string(http_status) + ")";
    }
};

/**
 * @brief Downloaded plugin binary SHA-256 does not match the registry manifest.
 * @code BASE_REGISTRY_CHECKSUM_MISMATCH::code == 1302 @endcode
 */
struct BASE_REGISTRY_CHECKSUM_MISMATCH {
    static constexpr int code = 1302;

    static constexpr std::string_view description() noexcept {
        return "downloaded plugin checksum does not match registry manifest";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Do not load the downloaded artifact — it may be corrupt or "
               "tampered. Re-trigger the download. If the mismatch persists, "
               "contact the registry administrator to verify the manifest "
               "integrity.";
    }

    /**
     * @param plugin_name     Plugin name.
     * @param expected_sha256 Expected SHA-256 from the registry manifest.
     * @param actual_sha256   Computed SHA-256 of the downloaded file.
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& plugin_name,
                              const std::string& expected_sha256,
                              const std::string& actual_sha256) {
        return "[BASE_REGISTRY_CHECKSUM_MISMATCH:" + std::to_string(code) + "] "
               "plugin '" + plugin_name + "' checksum mismatch: "
               "expected=" + expected_sha256
               + " actual=" + actual_sha256;
    }
};

/**
 * @brief Plugin binary download failed (I/O or disk error after HTTP success).
 * @code BASE_REGISTRY_DOWNLOAD_FAILED::code == 1303 @endcode
 */
struct BASE_REGISTRY_DOWNLOAD_FAILED {
    static constexpr int code = 1303;

    static constexpr std::string_view description() noexcept {
        return "plugin binary download failed after HTTP response";
    }

    /// @brief Actionable operator remediation hint.
    static constexpr std::string_view remediationHint() noexcept {
        return "Verify that the destination directory exists and has sufficient "
               "disk space with write permissions. Check the I/O error detail "
               "in the diagnostic message. Re-trigger the download after "
               "resolving the file-system issue.";
    }

    /**
     * @param plugin_name   Plugin name.
     * @param download_url  Full download URL.
     * @param reason        I/O or file-system error description.
     * @brief TBD: Describe format.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    static std::string format(const std::string& plugin_name,
                              const std::string& download_url,
                              const std::string& reason) {
        return "[BASE_REGISTRY_DOWNLOAD_FAILED:" + std::to_string(code) + "] "
               "plugin '" + plugin_name + "' download from '"
               + download_url + "' failed: " + reason;
    }
};

// =============================================================================
// Runtime code → description resolver
// =============================================================================

/**
 * @brief Resolve a numeric error code to its taxonomy description at runtime.
 *
 * Checks every known code in the taxonomy and returns its description string.
 * Returns @c "unknown error code" for values outside all known ranges.
 *
 * @param error_code  Integer error code (e.g. @c BASE_LOADER_ABI_MISMATCH::code).
 * @return Human-readable description string view or @c "unknown error code".
 */
inline std::string_view resolveDescription(int error_code) noexcept {
    switch (error_code) {
        /**
         * @brief Loader
         * @return Return value.
         */
        case BASE_LOADER_PATH_NOT_FOUND::code:         return BASE_LOADER_PATH_NOT_FOUND::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_LOADER_SIGNATURE_REJECTED::code:     return BASE_LOADER_SIGNATURE_REJECTED::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_LOADER_ABI_MISMATCH::code:           return BASE_LOADER_ABI_MISMATCH::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_LOADER_LOAD_FAILED::code:            return BASE_LOADER_LOAD_FAILED::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_LOADER_INIT_FAILED::code:            return BASE_LOADER_INIT_FAILED::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_LOADER_HEALTH_CHECK_FAILED::code:    return BASE_LOADER_HEALTH_CHECK_FAILED::description();
        /**
         * @brief Sandbox
         * @return Return value.
         */
        case BASE_SANDBOX_LAUNCH_FAILED::code:         return BASE_SANDBOX_LAUNCH_FAILED::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_SANDBOX_RESOURCE_LIMIT::code:        return BASE_SANDBOX_RESOURCE_LIMIT::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_SANDBOX_TIMEOUT::code:               return BASE_SANDBOX_TIMEOUT::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_SANDBOX_DEGRADED::code:              return BASE_SANDBOX_DEGRADED::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_SANDBOX_INACTIVE_STATS::code:        return BASE_SANDBOX_INACTIVE_STATS::description();
        /**
         * @brief Reload
         * @return Return value.
         */
        case BASE_RELOAD_NO_BACKUP::code:              return BASE_RELOAD_NO_BACKUP::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_RELOAD_ROLLBACK_FAILED::code:        return BASE_RELOAD_ROLLBACK_FAILED::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_RELOAD_CANDIDATE_LOAD_FAILED::code:  return BASE_RELOAD_CANDIDATE_LOAD_FAILED::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_RELOAD_STATE_RESTORE_FAILED::code:   return BASE_RELOAD_STATE_RESTORE_FAILED::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_RELOAD_NOT_REGISTERED::code:         return BASE_RELOAD_NOT_REGISTERED::description();
        /**
         * @brief Dependency
         * @return Return value.
         */
        case BASE_DEPENDENCY_CONFLICT::code:           return BASE_DEPENDENCY_CONFLICT::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_DEPENDENCY_CYCLE::code:              return BASE_DEPENDENCY_CYCLE::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_DEPENDENCY_MISSING_REQUIRED::code:   return BASE_DEPENDENCY_MISSING_REQUIRED::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_DEPENDENCY_VERSION_RANGE_MISMATCH::code: return BASE_DEPENDENCY_VERSION_RANGE_MISMATCH::description();
        /**
         * @brief Registry
         * @return Return value.
         */
        case BASE_REGISTRY_NETWORK_ERROR::code:        return BASE_REGISTRY_NETWORK_ERROR::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_REGISTRY_AUTH_FAILURE::code:         return BASE_REGISTRY_AUTH_FAILURE::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_REGISTRY_CHECKSUM_MISMATCH::code:    return BASE_REGISTRY_CHECKSUM_MISMATCH::description();
        /**
         * @brief TBD: Describe description.
         * @return Return value.
         */
        case BASE_REGISTRY_DOWNLOAD_FAILED::code:      return BASE_REGISTRY_DOWNLOAD_FAILED::description();
        default:                                       return "unknown error code";
    }
}

/**
 * @brief Resolve a numeric error code to its operator remediation hint.
 *
 * Returns a short, actionable string for each known taxonomy code.
 * Returns @c "no remediation hint available" for unknown codes.
 *
 * @param error_code  Integer error code (e.g. @c BASE_LOADER_ABI_MISMATCH::code).
 * @return Actionable hint string view.
 */
inline std::string_view resolveRemediationHint(int error_code) noexcept {
    switch (error_code) {
        /**
         * @brief Loader
         * @return Return value.
         */
        case BASE_LOADER_PATH_NOT_FOUND::code:         return BASE_LOADER_PATH_NOT_FOUND::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_LOADER_SIGNATURE_REJECTED::code:     return BASE_LOADER_SIGNATURE_REJECTED::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_LOADER_ABI_MISMATCH::code:           return BASE_LOADER_ABI_MISMATCH::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_LOADER_LOAD_FAILED::code:            return BASE_LOADER_LOAD_FAILED::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_LOADER_INIT_FAILED::code:            return BASE_LOADER_INIT_FAILED::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_LOADER_HEALTH_CHECK_FAILED::code:    return BASE_LOADER_HEALTH_CHECK_FAILED::remediationHint();
        /**
         * @brief Sandbox
         * @return Return value.
         */
        case BASE_SANDBOX_LAUNCH_FAILED::code:         return BASE_SANDBOX_LAUNCH_FAILED::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_SANDBOX_RESOURCE_LIMIT::code:        return BASE_SANDBOX_RESOURCE_LIMIT::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_SANDBOX_TIMEOUT::code:               return BASE_SANDBOX_TIMEOUT::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_SANDBOX_DEGRADED::code:              return BASE_SANDBOX_DEGRADED::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_SANDBOX_INACTIVE_STATS::code:        return BASE_SANDBOX_INACTIVE_STATS::remediationHint();
        /**
         * @brief Reload
         * @return Return value.
         */
        case BASE_RELOAD_NO_BACKUP::code:              return BASE_RELOAD_NO_BACKUP::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_RELOAD_ROLLBACK_FAILED::code:        return BASE_RELOAD_ROLLBACK_FAILED::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_RELOAD_CANDIDATE_LOAD_FAILED::code:  return BASE_RELOAD_CANDIDATE_LOAD_FAILED::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_RELOAD_STATE_RESTORE_FAILED::code:   return BASE_RELOAD_STATE_RESTORE_FAILED::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_RELOAD_NOT_REGISTERED::code:         return BASE_RELOAD_NOT_REGISTERED::remediationHint();
        /**
         * @brief Dependency
         * @return Return value.
         */
        case BASE_DEPENDENCY_CONFLICT::code:           return BASE_DEPENDENCY_CONFLICT::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_DEPENDENCY_CYCLE::code:              return BASE_DEPENDENCY_CYCLE::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_DEPENDENCY_MISSING_REQUIRED::code:   return BASE_DEPENDENCY_MISSING_REQUIRED::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_DEPENDENCY_VERSION_RANGE_MISMATCH::code: return BASE_DEPENDENCY_VERSION_RANGE_MISMATCH::remediationHint();
        /**
         * @brief Registry
         * @return Return value.
         */
        case BASE_REGISTRY_NETWORK_ERROR::code:        return BASE_REGISTRY_NETWORK_ERROR::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_REGISTRY_AUTH_FAILURE::code:         return BASE_REGISTRY_AUTH_FAILURE::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_REGISTRY_CHECKSUM_MISMATCH::code:    return BASE_REGISTRY_CHECKSUM_MISMATCH::remediationHint();
        /**
         * @brief TBD: Describe remediationHint.
         * @return Return value.
         */
        case BASE_REGISTRY_DOWNLOAD_FAILED::code:      return BASE_REGISTRY_DOWNLOAD_FAILED::remediationHint();
        default:                                       return "no remediation hint available";
    }
}

/**
 * @brief Return true when @p code falls within any known taxonomy range.
 * @param error_code  Integer error code to test.
 */
inline constexpr bool isKnownCode(int error_code) noexcept {
    return (error_code >= 1100 && error_code <= 1149)  // loader
        || (error_code >= 1150 && error_code <= 1199)  // sandbox
        || (error_code >= 1200 && error_code <= 1249)  // reload
        || (error_code >= 1250 && error_code <= 1299)  // dependency
        || (error_code >= 1300 && error_code <= 1349); // registry
}

} // namespace BaseErrorTaxonomy
} // namespace modules
} // namespace themis
