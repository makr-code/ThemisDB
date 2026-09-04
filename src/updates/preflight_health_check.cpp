/**
 * @file preflight_health_check.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "updates/preflight_health_check.h"
#include "utils/logger.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)  SPDLOG_WARN(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <stdexcept>

#if defined(_WIN32)
#  include <windows.h>
#elif defined(__linux__)
#  include <sys/sysinfo.h>
// macOS: sysinfo not available; defaultMemoryProvider returns 0 (check skipped)
#endif

namespace themis {
namespace updates {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/// Default disk-space provider using std::filesystem::space().
uint64_t defaultSpaceProvider(const std::string& path) {
    std::error_code ec = {};
    auto info = std::filesystem::space(path, ec);
    if (ec) {
        LOG_WARN("DiskSpaceChecker: std::filesystem::space('{}') failed: {}",
                 path, ec.message());
        return 0;
    }
    return static_cast<uint64_t>(info.available);
}

/// Default memory provider: reads available RAM in bytes.
uint64_t defaultMemoryProvider() {
#if defined(_WIN32)
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    if (!GlobalMemoryStatusEx(&status)) {
        return 0;
    }
    return static_cast<uint64_t>(status.ullAvailPhys);
#elif defined(__linux__)
    struct sysinfo si{};
    if (sysinfo(&si) != 0) {
        return 0;
    }
    // freeram is in units of mem_unit bytes.
    return static_cast<uint64_t>(si.freeram) *
           static_cast<uint64_t>(si.mem_unit);
#else
    // Unsupported platform (macOS, etc.): return 0 so the check is skipped
    // with a warning.  Callers should inject a real provider when needed.
    return 0;
#endif
}

/// Parse a dot-separated version string into a vector of integers.
std::vector<int> parseVersion(const std::string& v) {
    std::vector<int> parts;
    std::istringstream ss(v);
    std::string token = {};
    while (std::getline(ss, token, '.')) {
        try {
            parts.push_back(std::stoi(token));
        } catch (...) {
            parts.push_back(0);
        }
    }
    return parts;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// DiskSpaceChecker
// ---------------------------------------------------------------------------

DiskSpaceChecker::DiskSpaceChecker(
    std::string   path,
    uint64_t      required_bytes,
    SpaceProvider provider)
    : path_(std::move(path))
    , required_bytes_(required_bytes)
    , provider_(provider ? std::move(provider) : defaultSpaceProvider)
{}

std::string DiskSpaceChecker::name() const {
    return "disk_space";
}

HealthCheckResult DiskSpaceChecker::run() {
    HealthCheckResult result;
    result.check_name = name();

    const uint64_t available = provider_(path_);

    if (available >= required_bytes_) {
        result.passed  = true;
        result.message = "Disk space OK: " +
                         std::to_string(available / (1024 * 1024)) + " MiB available, " +
                         std::to_string(required_bytes_ / (1024 * 1024)) + " MiB required";
    } else {
        result.passed = false;
        result.error  = "Insufficient disk space on '" + path_ + "': " +
                        std::to_string(available / (1024 * 1024)) + " MiB available, " +
                        std::to_string(required_bytes_ / (1024 * 1024)) + " MiB required";
        result.message = result.error;
        LOG_WARN("PreflightHealthCheck: {}", result.error);
    }
    return result;
}

// ---------------------------------------------------------------------------
// MemoryHeadroomChecker
// ---------------------------------------------------------------------------

MemoryHeadroomChecker::MemoryHeadroomChecker(
    uint64_t       required_bytes,
    MemoryProvider provider)
    : required_bytes_(required_bytes)
    , provider_(provider ? std::move(provider) : defaultMemoryProvider)
{}

std::string MemoryHeadroomChecker::name() const {
    return "memory_headroom";
}

HealthCheckResult MemoryHeadroomChecker::run() {
    HealthCheckResult result;
    result.check_name = name();

    const uint64_t available = provider_();

    // If the provider returns 0 on an unsupported platform we skip the check
    // with a warning rather than blocking the update.
    if (available == 0) {
        result.passed  = true;
        result.message = "Memory headroom check skipped (provider unavailable on this platform)";
        LOG_WARN("PreflightHealthCheck: memory headroom provider returned 0; check skipped");
        return result;
    }

    if (available >= required_bytes_) {
        result.passed  = true;
        result.message = "Memory headroom OK: " +
                         std::to_string(available / (1024 * 1024)) + " MiB available, " +
                         std::to_string(required_bytes_ / (1024 * 1024)) + " MiB required";
    } else {
        result.passed = false;
        result.error  = "Insufficient memory headroom: " +
                        std::to_string(available / (1024 * 1024)) + " MiB available, " +
                        std::to_string(required_bytes_ / (1024 * 1024)) + " MiB required";
        result.message = result.error;
        LOG_WARN("PreflightHealthCheck: {}", result.error);
    }
    return result;
}

// ---------------------------------------------------------------------------
// DependencyVersionChecker
// ---------------------------------------------------------------------------

DependencyVersionChecker::DependencyVersionChecker(
    std::string     dep_name,
    std::string     min_version,
    VersionProvider version_provider)
    : dep_name_(std::move(dep_name))
    , min_version_(std::move(min_version))
    , version_provider_(std::move(version_provider))
{
    if (!version_provider_) {
        throw std::invalid_argument(
            "DependencyVersionChecker: version_provider must not be null");
    }
}

std::string DependencyVersionChecker::name() const {
    return "dependency_version[" + dep_name_ + "]";
}

HealthCheckResult DependencyVersionChecker::run() {
    HealthCheckResult result;
    result.check_name = name();

    const std::string installed = version_provider_(dep_name_);
    if (installed.empty()) {
        result.passed = false;
        result.error  = "Dependency '" + dep_name_ + "' not found";
        result.message = result.error;
        LOG_WARN("PreflightHealthCheck: {}", result.error);
        return result;
    }

    if (compareVersions(installed, min_version_) >= 0) {
        result.passed  = true;
        result.message = "Dependency '" + dep_name_ + "' version OK: " +
                         installed + " >= " + min_version_;
    } else {
        result.passed = false;
        result.error  = "Dependency '" + dep_name_ + "' version " +
                        installed + " is below minimum " + min_version_;
        result.message = result.error;
        LOG_WARN("PreflightHealthCheck: {}", result.error);
    }
    return result;
}

/*static*/
int DependencyVersionChecker::compareVersions(
    const std::string& a, const std::string& b)
{
    const auto pa = parseVersion(a);
    const auto pb = parseVersion(b);

    const size_t len = std::max(pa.size(), pb.size());
    for (size_t i = 0; i < len; ++i) {
        const int va = (i < pa.size()) ? pa[i] : 0;
        const int vb = (i < pb.size()) ? pb[i] : 0;
        if (va != vb) {
            return va - vb;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// PreflightHealthChecker
// ---------------------------------------------------------------------------

void PreflightHealthChecker::addCheck(std::unique_ptr<IHealthCheck> check) {
    if (!check) {
        throw std::invalid_argument("PreflightHealthChecker::addCheck: check must not be null");
    }
    checks_.push_back(std::move(check));
}

PreflightCheckResult PreflightHealthChecker::runAll() {
    PreflightCheckResult result;
    const auto start = std::chrono::steady_clock::now();

    result.all_passed = true;
    for (auto& check : checks_) {
        auto cr = check->run();
        if (!cr.passed) {
            if (result.error_summary.empty()) {
                result.error_summary = cr.error;
            }
            result.all_passed = false;
        }
        result.results.push_back(std::move(cr));
    }

    const auto end = std::chrono::steady_clock::now();
    result.duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    if (result.all_passed) {
        LOG_INFO("PreflightHealthCheck: all {} check(s) passed in {} ms",
                 result.results.size(), result.duration.count());
    } else {
        LOG_WARN("PreflightHealthCheck: check(s) failed in {} ms: {}",
                 result.duration.count(), result.error_summary);
    }

    return result;
}

size_t PreflightHealthChecker::checkCount() const {
    return checks_.size();
}

} // namespace updates
} // namespace themis


