/**
 * @file module_sandbox.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=11, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Module Sandbox & ABI Checker – Implementation
//
// AbiChecker: deep ABI compatibility validation for hot-reload
// ModuleSandbox: resource-limited OS jail for plugin code

#include "themis/base/module_sandbox.h"

#include <cctype>
#include <cerrno>
#include <cstring>
#include <sstream>

#include "themis/base/module_loader.h"
#include "themis/base/wasm_plugin_sandbox.h"
#include "themis/base/wasm_runtime_injector.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <fstream>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#if __has_include(<sys/prctl.h>)
#include <sys/prctl.h>
#define THEMIS_HAVE_PRCTL 1
#endif
#endif

#include <spdlog/spdlog.h>

namespace themis {
namespace modules {

namespace {

// =============================================================================
// WasmRuntimeAdapter – bridges IWasmRuntime (injector API) to WasmRuntime
//                      (WasmPluginSandbox API)
//
// WasmRuntimeInjector::create() returns IWasmRuntime, while
// WasmPluginSandbox::setRuntime() takes a WasmRuntime.  This adapter owns
// the IWasmRuntime and implements the WasmRuntime contract.
// =============================================================================

class WasmRuntimeAdapter final : public WasmRuntime {
  public:
    explicit WasmRuntimeAdapter(std::unique_ptr<IWasmRuntime> impl) : impl_(std::move(impl)) {}

    bool instantiate(const std::vector<uint8_t> &wasm_bytes, const std::vector<WasmHostFunction> &host_fns,
                     uint8_t * /*linear_memory*/, size_t memory_size) override {
        // IWasmRuntime::instantiate() takes a memory limit in bytes and manages
        // its own linear memory allocation.
        return impl_->instantiate(wasm_bytes, host_fns, memory_size);
    }

    bool call(cons[[maybe_unused]] t st[[maybe_unused]] d::string &[[maybe_unused]] export_name, cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<uint8_[[maybe_unused]] t> &arg[[maybe_unused]] s, st[[maybe_unused]] d::vecto[[maybe_unused]] r<uint8_[[maybe_unused]] t> &ou[[maybe_unused]] t) override {
        return impl_->call(export_name, args, out);
    }

    void destroy() override {
        impl_.reset();
    }

    std::string engineName() const override {
        return impl_ ? impl_->name() : std::string{};
    }

  private:
    std::unique_ptr<IWasmRuntime> impl_;
};

} // anonymous namespace

// =============================================================================
// Linux cgroup v2 helpers
// =============================================================================

#if defined(__linux__)
namespace {

/// Returns true when the cgroup v2 unified hierarchy is mounted at
/// /sys/fs/cgroup AND the process has write permission to create sub-cgroups
/// under /sys/fs/cgroup/themis/.
static bool isCgroupV2Available() {
    // cgroup v2 exposes a "cgroup.controllers" file at the root of the
    // unified hierarchy.  Its absence means the kernel uses only cgroup v1.
    struct stat st{};
    if (::stat("/sys/fs/cgroup/cgroup.controllers", &st) != 0) {
        return false;
    }
    // A writable root means we can create /sys/fs/cgroup/themis/<id>/.
    return ::access("/sys/fs/cgroup", W_OK) == 0;
}

/// Replace characters that are invalid inside a cgroup directory name
/// (anything that is not alphanumeric, '_', or '-') with '_'.
static std::string sanitizeCgroupName(const std::string &name) {
    std::string out = {};
    out.reserve(name.size());
    for (unsigned char c : name) {
        out += (std::isalnum(c) || c == '_' || c == '-') ? static_cast<char>(c) : '_';
    }
    return out.empty() ? "sandbox" : out;
}

} // anonymous namespace
#endif

// =============================================================================
// AbiChecker
// =============================================================================

AbiChecker::AbiChecker()  = default;
AbiChecker::~AbiChecker() = default;

void AbiChecker::addRequiredSymbol(const std::string &sym) {
    required_symbols_.push_back(sym);
}

void AbiChecker::addDeprecatedSymbol(const std::string &sym) {
    deprecated_symbols_.push_back(sym);
}

void AbiChecker::useDefaultLists() {
    required_symbols_ = {
        "themis_module_init",       "themis_module_shutdown",   "themis_module_version",
        "themis_api_version_major", "themis_api_version_minor",
    };
    // No deprecated symbols in v1.x
}

/*static*/ void *AbiChecker::resolveSymbol(void *handle, const std::string &name) noexcept {
#ifdef _WIN32
    return reinterpret_cast<void *>(GetProcAddress(static_cast<HMODULE>(handle), name.c_str()));
#else
    return dlsym(handle, name.c_str());
#endif
}

AbiCheckResult AbiChecker::checkVersions(const ModuleMetadata &meta, uint32_t host_major, uint32_t host_minor) const {
    AbiCheckResult result;
    result.compatible = true;

    if (meta.themisMajor != host_major) {
        result.compatible = false;
        result.issues.push_back("Major version mismatch: module=" + std::to_string(meta.themisMajor)
                                + " host=" + std::to_string(host_major));
    }

    if (meta.themisMinor > host_minor) {
        result.compatible = false;
        result.issues.push_back("Module minor version too new: module=" + std::to_string(meta.themisMinor)
                                + " host=" + std::to_string(host_minor));
    }

    if (meta.version.empty()) {
        result.issues.push_back("Module does not export version string (warning)");
        // Treat as non-fatal: allow it but note the issue
    }

    result.summary = result.compatible ? "Version compatibility OK (" + meta.version + ")" : "Version incompatible";
    return result;
}

AbiCheckResult AbiChecker::checkRequiredSymbols(void *handle) const {
    AbiCheckResult result;
    result.compatible = true;

    for (const auto &sym : required_symbols_) {
        if (!resolveSymbol(handle, sym)) {
            result.compatible = false;
            result.issues.push_back("Required symbol missing: " + sym);
        }
    }

    result.summary = result.compatible ? "All required symbols present"
                                       : "Missing required symbols (" + std::to_string(result.issues.size()) + ")";
    return result;
}

AbiCheckResult AbiChecker::checkDeprecatedSymbols(void *handle) const {
    AbiCheckResult result;
    result.compatible = true; // Deprecated symbols are warnings, not failures

    for (const auto &sym : deprecated_symbols_) {
        if (resolveSymbol(handle, sym)) {
            result.issues.push_back("Deprecated symbol still present: " + sym
                                    + " (rebuild module against current headers)");
        }
    }

    result.summary = result.issues.empty()
                         ? "No deprecated symbols"
                         : "Deprecated symbol(s) detected (" + std::to_string(result.issues.size()) + ")";
    return result;
}

AbiCheckResult AbiChecker::check(void *module_handle, const ModuleMetadata &module_meta, uint32_t host_major,
                                 uint32_t host_minor) const {
    AbiCheckResult combined;
    combined.compatible = true;

    auto versions = checkVersions(module_meta, host_major, host_minor);
    if (!versions.compatible) {
        combined.compatible = false;
        for (auto &i : versions.issues) {
            combined.issues.push_back(i);
        }
    }

    if (module_handle) {
        auto symbols = checkRequiredSymbols(module_handle);
        if (!symbols.compatible) {
            combined.compatible = false;
            for (auto &i : symbols.issues) {
                combined.issues.push_back(i);
            }
        }

        auto deprecated = checkDeprecatedSymbols(module_handle);
        // Deprecated symbols are warnings only
        for (auto &i : deprecated.issues) {
            combined.issues.push_back("[WARN] " + i);
        }
    }

    std::ostringstream oss = {};
    oss << (combined.compatible ? "ABI OK" : "ABI INCOMPATIBLE");
    oss << " – " << combined.issues.size() << " issue(s)";
    combined.summary = oss.str();

    return combined;
}

// =============================================================================
// ModuleSandbox – Platform handle
// =============================================================================

struct ModuleSandbox::PlatformHandle {
#ifdef _WIN32
    HANDLE job_object = nullptr;
#else
    // cgroup path for this sandbox instance (Linux v2 unified hierarchy)
    std::string cgroup_path;
    // PID this sandbox applies to
    pid_t target_pid = 0;
    // rlimits saved for restoration
    struct rlimit saved_mem_limit{};
    bool mem_limit_applied = false;
    struct rlimit saved_cpu_limit{};
    bool cpu_limit_applied = false;
    // true when setupCgroupV2() succeeded
    bool cgroup_v2_active = false;
#endif
};

// =============================================================================
// ModuleSandbox
// =============================================================================

ModuleSandbox::ModuleSandbox(const Config &config) : config_(config), platform_(std::make_unique<PlatformHandle>()) {}

ModuleSandbox::~ModuleSandbox() {
    if (active_) {
        shutdown();
    }
}

bool ModuleSandbox::launch(const std::string &module_name) {
    module_name_ = module_name;
    last_error_.clear();
    launch_warnings_.clear();

    bool ok = true;

    ok &= applyMemoryLimit();
    ok &= applyCpuLimit();

    if (config_.allow_network == false) {
        applyNetworkIsolation(); // Best-effort; failures are warnings
    }

    applyFilesystemRestrictions(); // Best-effort
    applySyscallFilter();          // Best-effort

    if (!ok) {
        return false;
    }

    // ── WASM isolation (v1.8.0) ──────────────────────────────────────────
    if (config_.enable_wasm_isolation) {
        if (!WasmRuntimeInjector::available()) {
            launch_warnings_.push_back("WASM isolation requested but no WasmRuntime backend is registered; "
                                       "falling back to OS-only sandbox. Register a backend via "
                                       "WasmRuntimeInjector::registerRuntime() before calling launch().");
        } else {
            auto rt = WasmRuntimeInjector::create(config_.wasm_runtime_name);
            if (!rt) {
                launch_warnings_.push_back("WASM isolation: WasmRuntimeInjector::create() returned nullptr; "
                                           "OS-only sandbox active.");
            } else {
                WasmPluginSandbox::Config wasm_cfg;
                wasm_cfg.linear_memory_pages        = config_.wasm_linear_memory_pages;
                wasm_cfg.max_memory_mb              = config_.max_memory_mb;
                wasm_cfg.max_cpu_time_seconds       = config_.max_cpu_time_seconds;
                wasm_cfg.allow_unregistered_imports = config_.wasm_allow_unregistered_imports;

                wasm_sandbox_ = std::make_unique<WasmPluginSandbox>(wasm_cfg);
                // Wrap the IWasmRuntime (injector API) to WasmRuntime (sandbox API)
                wasm_sandbox_->setRuntime(std::make_unique<WasmRuntimeAdapter>(std::move(rt)));
                wasm_isolation_active_ = true;
            }
        }
    }

    active_ = true;
    return true;
}

void ModuleSandbox::shutdown() {
    if (!active_) {
        return;
    }

#ifdef _WIN32
    if (platform_->job_object) {
        CloseHandle(platform_->job_object);
        platform_->job_object = nullptr;
    }
#else
    // Remove rlimit overrides by restoring saved limits
    if (platform_->mem_limit_applied) {
        setrlimit(RLIMIT_AS, &platform_->saved_mem_limit);
        platform_->mem_limit_applied = false;
    }
    if (platform_->cpu_limit_applied) {
        setrlimit(RLIMIT_CPU, &platform_->saved_cpu_limit);
        platform_->cpu_limit_applied = false;
    }
    // Remove the cgroup v2 hierarchy created during launch.
    teardownCgroupV2();
#endif

    // Release WASM sandbox (v1.8.0)
    if (wasm_isolation_active_) {
        wasm_sandbox_.reset();
        wasm_isolation_active_ = false;
    }

    active_ = false;
}

bool ModuleSandbox::applyMemoryLimit() {
    if (config_.max_memory_mb == 0) {
        return true;
    }

#ifdef _WIN32
    // Create a new Job Object for this sandbox
    if (!platform_->job_object) {
        platform_->job_object = CreateJobObjectA(nullptr, nullptr);
        if (!platform_->job_object) {
            last_error_ = "CreateJobObject failed: " + std::to_string(GetLastError());
            return false;
        }
        // Assign current process to the job
        AssignProcessToJobObject(platform_->job_object, GetCurrentProcess());
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ji{};
    ji.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_PROCESS_MEMORY;
    ji.ProcessMemoryLimit               = config_.max_memory_mb * 1024 * 1024;
    if (!SetInformationJobObject(platform_->job_object, JobObjectExtendedLimitInformation, &ji, sizeof(ji))) {
        last_error_ = "SetInformationJobObject (memory) failed: " + std::to_string(GetLastError());
        return false;
    }
    return true;

#elif defined(__linux__)
    // Attempt cgroup v2 enforcement first (memory.max) by setting up a
    // shared cgroup v2 sandbox. setupCgroupV2() initializes cgroup v2
    // state; applyCpuLimit() is responsible for writing cpu.max when
    // cgroup_v2_active is true.
    if (!platform_->cgroup_v2_active) {
        if (isCgroupV2Available()) {
            if (!setupCgroupV2()) {
                spdlog::warn("ModuleSandbox({}): cgroup v2 setup failed – "
                             "falling back to RLIMIT_AS for memory enforcement",
                             module_name_);
            }
        } else {
            spdlog::warn("ModuleSandbox({}): cgroup v2 unavailable "
                         "(no write access to /sys/fs/cgroup) – "
                         "using RLIMIT_AS as memory-limit fallback",
                         module_name_);
        }
    }

    if (!platform_->cgroup_v2_active) {
        // Fall back to RLIMIT_AS (virtual address space).
        // GAP-FIX unchecked_result: check getrlimit return value and warn on
        // failure so that a failed save does not silently corrupt the saved limit.
        if (getrlimit(RLIMIT_AS, &platform_->saved_mem_limit) != 0) {
            launch_warnings_.push_back("getrlimit(RLIMIT_AS) failed – saved memory limit may be invalid");
        }

        struct rlimit new_limit{};
        new_limit.rlim_cur = static_cast<rlim_t>(config_.max_memory_mb) * 1024 * 1024;
        new_limit.rlim_max = new_limit.rlim_cur;

        if (setrlimit(RLIMIT_AS, &new_limit) == 0) {
            platform_->mem_limit_applied = true;
        } else {
            launch_warnings_.push_back("RLIMIT_AS not supported on this kernel – memory limit not enforced");
        }
    }
    return true;

#else
    launch_warnings_.push_back("Memory limit: platform not supported");
    return true;
#endif
}

bool ModuleSandbox::applyCpuLimit() {
    if (config_.max_cpu_percent == 0 && config_.max_cpu_time_seconds == 0) {
        return true;
    }

#ifdef _WIN32
    if (!platform_->job_object) {
        // Try to create job object if not already done
        platform_->job_object = CreateJobObjectA(nullptr, nullptr);
        if (platform_->job_object) {
            AssignProcessToJobObject(platform_->job_object, GetCurrentProcess());
        }
    }
    if (platform_->job_object) {
        JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cr{};
        cr.ControlFlags = JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP;
        // CpuRate is in units of 1/100 of a percent
        cr.CpuRate = static_cast<DWORD>(config_.max_cpu_percent) * 100;
        SetInformationJobObject(platform_->job_object, JobObjectCpuRateControlInformation, &cr, sizeof(cr));
    }
    return true;

#elif defined(__linux__)
    if (platform_->cgroup_v2_active && config_.max_cpu_percent > 0) {
        // cgroup v2 cpu.max: "<quota_us> <period_us>"
        // Use a 100 ms period; quota = percent * period / 100.
        const int period_us = 100000; // 100 ms
        const int quota_us  = config_.max_cpu_percent * period_us / 100;
        std::ofstream cpu_max(platform_->cgroup_path + "/cpu.max");
        if (!cpu_max) {
            spdlog::warn("ModuleSandbox({}): failed to write cpu.max – "
                         "CPU rate limit not enforced via cgroup v2",
                         module_name_);
        } else {
            cpu_max << quota_us << " " << period_us << "\n";
        }
    } else if (config_.max_cpu_percent > 0 && !platform_->cgroup_v2_active) {
        spdlog::warn("ModuleSandbox({}): cgroup v2 unavailable – "
                     "CPU rate ({}%) cannot be enforced; RLIMIT_CPU used as fallback",
                     module_name_, config_.max_cpu_percent);
    }

    // RLIMIT_CPU caps total CPU-seconds regardless of cgroup v2 availability.
    if (config_.max_cpu_time_seconds > 0) {
        // GAP-FIX unchecked_result: check getrlimit return value and warn on
        // failure so that a failed save does not silently corrupt the saved limit.
        if (getrlimit(RLIMIT_CPU, &platform_->saved_cpu_limit) != 0) {
            launch_warnings_.push_back("getrlimit(RLIMIT_CPU) failed – saved CPU limit may be invalid");
        }

        struct rlimit new_limit{};
        new_limit.rlim_cur = static_cast<rlim_t>(config_.max_cpu_time_seconds);
        new_limit.rlim_max = new_limit.rlim_cur;

        if (setrlimit(RLIMIT_CPU, &new_limit) == 0) {
            platform_->cpu_limit_applied = true;
        } else {
            launch_warnings_.push_back("RLIMIT_CPU not applied on this kernel – CPU time limit not enforced");
        }
    }
    return true;

#else
    launch_warnings_.push_back("CPU limit: platform not supported");
    return true;
#endif
}

// =============================================================================
// ModuleSandbox – cgroup v2 setup / teardown (Linux only)
// =============================================================================

#if defined(__linux__)

bool ModuleSandbox::setupCgroupV2() {
    // Derive a unique, filesystem-safe cgroup directory name from the module
    // name and the current PID.  Using the PID prevents collisions when the
    // same module name is launched concurrently by different processes.
    const std::string safe_name = sanitizeCgroupName(module_name_) + "_" + std::to_string(::getpid());
    const std::string base_dir  = "/sys/fs/cgroup/themis";
    const std::string cg_path   = base_dir + "/" + safe_name;

    // Ensure the "themis" parent directory exists.
    if (::mkdir(base_dir.c_str(), 0755) != 0 && errno != EEXIST) {
        spdlog::warn("ModuleSandbox({}): mkdir({}) failed: {} – "
                     "cgroup v2 unavailable",
                     module_name_, base_dir, ::strerror(errno));
        return false;
    }

    // Enable the memory and cpu controllers in the parent's subtree_control
    // so that child cgroups can use memory.max and cpu.max.  These writes are
    // best-effort: if the controller is already enabled the write is a no-op;
    // if the open/write fails we warn and continue, letting the later
    // memory.max / cpu.max writes surface any resulting errors.
    {
        std::ofstream subtree(base_dir + "/cgroup.subtree_control");
        if (!subtree) {
            spdlog::warn("ModuleSandbox({}): cannot open cgroup.subtree_control "
                         "in {} – memory/CPU controllers may not be available",
                         module_name_, base_dir);
            // Non-fatal: proceed and let the memory.max/cpu.max writes surface the real error.
        } else {
            subtree << "+memory +cpu\n";
            if (!subtree) {
                spdlog::warn("ModuleSandbox({}): write to cgroup.subtree_control "
                             "failed – memory/CPU controllers may not be available",
                             module_name_);
            }
        }
    }

    // Create the sandbox-specific sub-cgroup.
    if (::mkdir(cg_path.c_str(), 0755) != 0) {
        spdlog::warn("ModuleSandbox({}): mkdir({}) failed: {} – "
                     "cgroup v2 unavailable",
                     module_name_, cg_path, ::strerror(errno));
        return false;
    }

    platform_->cgroup_path = cg_path;

    // ── memory.max ──────────────────────────────────────────────────────
    if (config_.max_memory_mb > 0) {
        std::ofstream mem_max(cg_path + "/memory.max");
        if (!mem_max) {
            spdlog::warn("ModuleSandbox({}): cannot write memory.max – "
                         "falling back to RLIMIT_AS",
                         module_name_);
            ::rmdir(cg_path.c_str());
            platform_->cgroup_path.clear();
            return false;
        }
        mem_max << (static_cast<uint64_t>(config_.max_memory_mb) * 1024ULL * 1024ULL) << "\n";
        if (!mem_max) {
            spdlog::warn("ModuleSandbox({}): write to memory.max failed – "
                         "falling back to RLIMIT_AS",
                         module_name_);
            ::rmdir(cg_path.c_str());
            platform_->cgroup_path.clear();
            return false;
        }
    }

    // ── Enroll the current process in the new cgroup ─────────────────────
    {
        std::ofstream procs(cg_path + "/cgroup.procs");
        if (!procs) {
            spdlog::warn("ModuleSandbox({}): cannot write cgroup.procs – "
                         "falling back to RLIMIT_AS",
                         module_name_);
            ::rmdir(cg_path.c_str());
            platform_->cgroup_path.clear();
            return false;
        }
        procs << ::getpid() << "\n";
        if (!procs) {
            spdlog::warn("ModuleSandbox({}): write to cgroup.procs failed – "
                         "falling back to RLIMIT_AS",
                         module_name_);
            ::rmdir(cg_path.c_str());
            platform_->cgroup_path.clear();
            return false;
        }
    }

    platform_->cgroup_v2_active = true;
    // GAP-FIX sensitive_data_logging: cg_path encodes module name + PID;
    // keep this at debug level (not warn/info) to limit exposure in prod logs.
    spdlog::debug("ModuleSandbox({}): cgroup v2 sandbox active", module_name_);
    return true;
}

void ModuleSandbox::teardownCgroupV2() {
    if (!platform_->cgroup_v2_active || platform_->cgroup_path.empty())
        return;

    bool migrated = false;

    // Move the current process back to the cgroup v2 root so that the
    // sandbox sub-directory becomes empty and can be removed.
    {
        std::ofstream root_procs("/sys/fs/cgroup/cgroup.procs");
        if (!root_procs) {
            spdlog::warn("ModuleSandbox({}): failed to open cgroup v2 root procs file "
                         "for teardown; cgroup '{}' may still contain tasks",
                         module_name_, platform_->cgroup_path);
        } else {
            root_procs << ::getpid() << "\n";
            if (!root_procs) {
                spdlog::warn("ModuleSandbox({}): failed to write PID to cgroup v2 root "
                             "procs file during teardown; cgroup '{}' may still "
                             "contain tasks",
                             module_name_, platform_->cgroup_path);
            } else {
                migrated = true;
            }
        }
    }

    // rmdir(2) succeeds only when the cgroup has no tasks and no children.
    bool removed = false;
    if (::rmdir(platform_->cgroup_path.c_str()) != 0) {
        spdlog::warn("ModuleSandbox({}): rmdir({}) failed: {}", module_name_, platform_->cgroup_path,
                     ::strerror(errno));
    } else {
        removed = true;
    }

    // Only clear state if we successfully migrated tasks out and removed
    // the cgroup directory; otherwise, keep the path for diagnostics and
    // potential later cleanup attempts.
    if (migrated && removed) {
        platform_->cgroup_path.clear();
        platform_->cgroup_v2_active = false;
    } else {
        spdlog::warn("ModuleSandbox({}): cgroup v2 teardown incomplete; keeping cgroup path '{}' "
                     "for potential later cleanup",
                     module_name_, platform_->cgroup_path);
    }
}

#endif // __linux__

bool ModuleSandbox::applyNetworkIsolation() {
#if defined(__linux__)
    // Network namespace creation requires CAP_SYS_ADMIN.
    // We record a warning if we lack privileges; the module will still run
    // but without network isolation.
    launch_warnings_.push_back("Network isolation requires CAP_SYS_ADMIN (network namespace) – skipped");
#else
    launch_warnings_.push_back("Network isolation not supported on this platform");
#endif
    return true; // Non-fatal
}

bool ModuleSandbox::applyFilesystemRestrictions() {
    if (config_.fs_access == FilesystemAccess::FULL) {
        return true;
    }

#if defined(__linux__)
    // Full chroot / bind-mount sandboxing requires CAP_SYS_CHROOT.
    if (config_.fs_access == FilesystemAccess::NONE || config_.fs_access == FilesystemAccess::READ_ONLY) {
        launch_warnings_.push_back("Filesystem restrictions (chroot/bind-mount) require CAP_SYS_CHROOT – skipped");
    }
#else
    launch_warnings_.push_back("Filesystem restrictions not supported on this platform");
#endif
    return true;
}

bool ModuleSandbox::applySyscallFilter() {
#if defined(__linux__) && defined(THEMIS_HAVE_PRCTL)
    // seccomp-bpf requires CAP_SYS_ADMIN or PR_SET_SECCOMP privilege.
    // We note the attempt in warnings and proceed.
    launch_warnings_.push_back("Syscall filter (seccomp-bpf) requires additional privileges – skipped. "
                               "Deploy with systemd's SystemCallFilter= for production syscall filtering.");
#else
    if (!config_.allowed_syscalls.empty()) {
        launch_warnings_.push_back("Syscall filtering not supported on this platform");
    }
#endif
    return true;
}

SandboxStats ModuleSandbox::stats() const {
    SandboxStats s;

#if defined(__linux__)
    // Read peak memory from /proc/self/status
    std::ifstream status("/proc/self/status");
    std::string line = {};
    while (std::getline(status, line)) {
        if (line.rfind("VmPeak:", 0) == 0) {
            uint64_t kb = 0;
            std::istringstream(line.substr(7)) >> kb;
            s.peak_memory_bytes = kb * 1024;
            break;
        }
    }

#elif defined(_WIN32)
    if (platform_->job_object) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION ji{};
        if (QueryInformationJobObject(platform_->job_object, JobObjectExtendedLimitInformation, &ji, sizeof(ji),
                                      nullptr)) {
            s.peak_memory_bytes = ji.PeakJobMemoryUsed;
        }
    }
#endif

    s.killed = false;
    return s;
}

// =============================================================================
// ModuleSandbox – WASM isolation accessors (v1.8.0)
// =============================================================================

bool ModuleSandbox::isWasmIsolationActive() const noexcept {
    return wasm_isolation_active_;
}

WasmPluginSandbox *ModuleSandbox::wasmSandbox() noexcept {
    return wasm_sandbox_.get();
}

const WasmPluginSandbox *ModuleSandbox::wasmSandbox() const noexcept {
    return wasm_sandbox_.get();
}

} // namespace modules
} // namespace themis
