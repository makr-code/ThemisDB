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
 *  - @c code            — @c constexpr int unique error code.
 *  - @c description()   — @c static std::string_view one-line description.
 *  - @c format(...)     — @c static std::string diagnostic with context args.
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

    /**
     * @brief Build a formatted diagnostic message.
     * @param module_name  Logical module name.
     * @param path         Filesystem path that was not found.
     * @return Formatted diagnostic string.
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

    /**
     * @brief Build a formatted diagnostic message.
     * @param module_name  Logical module name.
     * @param reason       Human-readable rejection reason.
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

    /**
     * @brief Build a formatted diagnostic message.
     * @param module_name   Logical module name.
     * @param module_abi    ABI version string reported by the module.
     * @param host_abi      ABI version string required by the host.
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

    /**
     * @param module_name  Logical module name.
     * @param path         Path attempted.
     * @param os_error     dlerror() / GetLastError() message.
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

    /**
     * @param module_name   Logical module name.
     * @param init_symbol   Name of the init symbol that was called.
     * @param return_code   Return value from the init function.
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

    /**
     * @param module_name   Logical module name.
     * @param check_name    Name of the health check that failed.
     * @param detail        Diagnostic detail from the health check.
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

    /**
     * @param module_name  Name of the module being sandboxed.
     * @param last_error   ModuleSandbox::lastError() string.
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

    /**
     * @param module_name   Module name.
     * @param resource      "memory" or "cpu".
     * @param limit_value   Configured limit (e.g. "256 MB").
     * @param measured      Observed value.
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

    /**
     * @param module_name     Module name.
     * @param timeout_seconds Configured timeout value.
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

    /**
     * @param module_name   Module name.
     * @param warnings      Comma-separated list of launchWarnings().
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

    /**
     * @param module_name  Module name.
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

    /**
     * @param module_name  Module name.
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

    /**
     * @param module_name   Module name.
     * @param backup_path   Path to the backup binary.
     * @param reason        Failure reason.
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

    /**
     * @param module_name  Module name.
     * @param new_path     Path to the candidate binary.
     * @param reason       Load failure reason.
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

    /**
     * @param module_name  Module name.
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

    /**
     * @param module_name  Module name that was not found.
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

    /**
     * @param dep_name   Name of the shared dependency.
     * @param mod_a      First requiring module and its constraint.
     * @param mod_b      Second requiring module and its constraint.
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

    /**
     * @param cycle_str  Human-readable cycle description, e.g. "A→B→C→A".
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

    /**
     * @param module_name  Module that has the unsatisfied dependency.
     * @param dep_name     Name of the missing required dependency.
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

    /**
     * @param module_name  Requiring module.
     * @param dep_name     Dependency module name.
     * @param have         Registered version.
     * @param need_min     Required minimum version.
     * @param need_max     Required maximum version (empty = unconstrained).
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

    /**
     * @param registry_url  URL of the registry.
     * @param http_status   HTTP status code (0 if no response received).
     * @param curl_error    cURL error string or OS-level error description.
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

    /**
     * @param registry_url  URL of the registry.
     * @param http_status   HTTP status code (typically 401 or 403).
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

    /**
     * @param plugin_name     Plugin name.
     * @param expected_sha256 Expected SHA-256 from the registry manifest.
     * @param actual_sha256   Computed SHA-256 of the downloaded file.
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

    /**
     * @param plugin_name   Plugin name.
     * @param download_url  Full download URL.
     * @param reason        I/O or file-system error description.
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
        // Loader
        case BASE_LOADER_PATH_NOT_FOUND::code:         return BASE_LOADER_PATH_NOT_FOUND::description();
        case BASE_LOADER_SIGNATURE_REJECTED::code:     return BASE_LOADER_SIGNATURE_REJECTED::description();
        case BASE_LOADER_ABI_MISMATCH::code:           return BASE_LOADER_ABI_MISMATCH::description();
        case BASE_LOADER_LOAD_FAILED::code:            return BASE_LOADER_LOAD_FAILED::description();
        case BASE_LOADER_INIT_FAILED::code:            return BASE_LOADER_INIT_FAILED::description();
        case BASE_LOADER_HEALTH_CHECK_FAILED::code:    return BASE_LOADER_HEALTH_CHECK_FAILED::description();
        // Sandbox
        case BASE_SANDBOX_LAUNCH_FAILED::code:         return BASE_SANDBOX_LAUNCH_FAILED::description();
        case BASE_SANDBOX_RESOURCE_LIMIT::code:        return BASE_SANDBOX_RESOURCE_LIMIT::description();
        case BASE_SANDBOX_TIMEOUT::code:               return BASE_SANDBOX_TIMEOUT::description();
        case BASE_SANDBOX_DEGRADED::code:              return BASE_SANDBOX_DEGRADED::description();
        case BASE_SANDBOX_INACTIVE_STATS::code:        return BASE_SANDBOX_INACTIVE_STATS::description();
        // Reload
        case BASE_RELOAD_NO_BACKUP::code:              return BASE_RELOAD_NO_BACKUP::description();
        case BASE_RELOAD_ROLLBACK_FAILED::code:        return BASE_RELOAD_ROLLBACK_FAILED::description();
        case BASE_RELOAD_CANDIDATE_LOAD_FAILED::code:  return BASE_RELOAD_CANDIDATE_LOAD_FAILED::description();
        case BASE_RELOAD_STATE_RESTORE_FAILED::code:   return BASE_RELOAD_STATE_RESTORE_FAILED::description();
        case BASE_RELOAD_NOT_REGISTERED::code:         return BASE_RELOAD_NOT_REGISTERED::description();
        // Dependency
        case BASE_DEPENDENCY_CONFLICT::code:           return BASE_DEPENDENCY_CONFLICT::description();
        case BASE_DEPENDENCY_CYCLE::code:              return BASE_DEPENDENCY_CYCLE::description();
        case BASE_DEPENDENCY_MISSING_REQUIRED::code:   return BASE_DEPENDENCY_MISSING_REQUIRED::description();
        case BASE_DEPENDENCY_VERSION_RANGE_MISMATCH::code: return BASE_DEPENDENCY_VERSION_RANGE_MISMATCH::description();
        // Registry
        case BASE_REGISTRY_NETWORK_ERROR::code:        return BASE_REGISTRY_NETWORK_ERROR::description();
        case BASE_REGISTRY_AUTH_FAILURE::code:         return BASE_REGISTRY_AUTH_FAILURE::description();
        case BASE_REGISTRY_CHECKSUM_MISMATCH::code:    return BASE_REGISTRY_CHECKSUM_MISMATCH::description();
        case BASE_REGISTRY_DOWNLOAD_FAILED::code:      return BASE_REGISTRY_DOWNLOAD_FAILED::description();
        default:                                       return "unknown error code";
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
