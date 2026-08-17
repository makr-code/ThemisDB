/**
 * @file preflight_health_check.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace updates {

// ---------------------------------------------------------------------------
// HealthCheckResult – result of one individual check
// ---------------------------------------------------------------------------

/**
 * @brief Result of a single pre-flight health check.
 */
struct HealthCheckResult {
    /// Human-readable name of the check (e.g. "disk_space").
    std::string check_name;

    /// true when the check passed and the update may proceed.
    bool passed = false;

    /// Short human-readable description of the check outcome.
    std::string message;

    /// Non-empty when @c passed == false; contains a diagnostic detail.
    std::string error;
};

// ---------------------------------------------------------------------------
// PreflightCheckResult – aggregate result of all checks
// ---------------------------------------------------------------------------

/**
 * @brief Aggregate result returned by PreflightHealthChecker::runAll().
 */
struct PreflightCheckResult {
    /// true only when every individual check passed.
    bool all_passed = false;

    /// Per-check results in the order they were executed.
    std::vector<HealthCheckResult> results;

    /// Non-empty when @c all_passed == false; contains the first error summary.
    std::string error_summary;

    /// Wall-clock time taken to execute all checks.
    std::chrono::milliseconds duration{0};
};

// ---------------------------------------------------------------------------
// IHealthCheck – abstract interface for a single check
// ---------------------------------------------------------------------------

/**
 * @brief Abstract interface for a single pre-flight health check.
 *
 * Implement this to add custom checks to PreflightHealthChecker.
 */
class IHealthCheck {
public:
    virtual ~IHealthCheck() = default;

    /**
     * @brief Human-readable name used in HealthCheckResult::check_name.
     * @return Stable name string (e.g. "disk_space").
     */
    [[nodiscard]] virtual std::string name() const = 0;

    /**
     * @brief Execute the check.
     * @return HealthCheckResult with @c passed set accordingly.
     */
    [[nodiscard]] virtual HealthCheckResult run() = 0;
};

// ---------------------------------------------------------------------------
// DiskSpaceChecker
// ---------------------------------------------------------------------------

/**
 * @brief Verifies that the filesystem hosting @p path has at least
 *        @c required_bytes bytes of free space.
 *
 * Per module FUTURE_ENHANCEMENTS.md the check must confirm ≥ 2× the bundle
 * size of free space before starting a download to prevent mid-install
 * exhaustion.  The caller should therefore pass @c 2 * bundle_size as
 * @p required_bytes.
 *
 * The @p space_provider callback is injected so that unit tests can exercise
 * both the pass and fail paths without touching the real filesystem.
 * The default provider uses @c std::filesystem::space().
 */
class DiskSpaceChecker : public IHealthCheck {
public:
    /**
     * @brief Callable that returns the number of free bytes available at
     *        @p path, or 0 on error.
     */
    using SpaceProvider = std::function<uint64_t(const std::string& path)>;

    /**
     * @brief Construct a disk-space checker.
     *
     * @param path           Filesystem path to check (usually the download or
     *                       install directory).
     * @param required_bytes Minimum free bytes required to proceed.
     * @param provider       Optional custom space provider (defaults to
     *                       std::filesystem::space).
     */
    explicit DiskSpaceChecker(
        std::string            path,
        uint64_t               required_bytes,
        SpaceProvider          provider = nullptr
    );

    std::string       name() const override;
    HealthCheckResult run()  override;

private:
    std::string   path_;
    uint64_t      required_bytes_;
    SpaceProvider provider_;
};

// ---------------------------------------------------------------------------
// MemoryHeadroomChecker
// ---------------------------------------------------------------------------

/**
 * @brief Verifies that the system has at least @c required_bytes of free RAM.
 *
 * The @p memory_provider callback is injected so tests can simulate low-memory
 * situations without requiring special system configuration.
 * The default provider reads @c /proc/meminfo on Linux and uses
 * @c GlobalMemoryStatusEx on Windows.
 */
class MemoryHeadroomChecker : public IHealthCheck {
public:
    /**
     * @brief Callable that returns available (free + reclaimable) RAM in bytes,
     *        or 0 on error.
     */
    using MemoryProvider = std::function<uint64_t()>;

    /**
     * @brief Construct a memory-headroom checker.
     *
     * @param required_bytes Minimum free RAM bytes required.
     * @param provider       Optional custom memory provider (defaults to
     *                       platform-specific implementation).
     */
    explicit MemoryHeadroomChecker(
        uint64_t       required_bytes,
        MemoryProvider provider = nullptr
    );

    std::string       name() const override;
    HealthCheckResult run()  override;

private:
    uint64_t       required_bytes_;
    MemoryProvider provider_;
};

// ---------------------------------------------------------------------------
// DependencyVersionChecker
// ---------------------------------------------------------------------------

/**
 * @brief Verifies that a named dependency meets a minimum version requirement.
 *
 * Versions are compared lexicographically as dot-separated integers
 * (e.g. "1.4.0" < "1.10.0").
 *
 * The @p version_provider callback returns the currently installed version
 * string for @c dep_name.  An empty string means the dependency was not found
 * and the check fails.
 */
class DependencyVersionChecker : public IHealthCheck {
public:
    /**
     * @brief Callable that returns the currently installed version of
     *        @p dep_name, or an empty string if not found.
     */
    using VersionProvider = std::function<std::string(const std::string& dep_name)>;

    /**
     * @brief Construct a dependency-version checker.
     *
     * @param dep_name        Dependency name (used in error messages).
     * @param min_version     Minimum acceptable version string (e.g. "1.4.0").
     * @param version_provider Callable that returns the installed version.
     */
    explicit DependencyVersionChecker(
        std::string     dep_name,
        std::string     min_version,
        VersionProvider version_provider
    );

    std::string       name() const override;
    HealthCheckResult run()  override;

    /**
     * @brief Compare two dot-separated version strings.
     * @return negative if @p a < @p b, 0 if equal, positive if @p a > @p b.
     */
    static int compareVersions(const std::string& a, const std::string& b);

private:
    std::string     dep_name_;
    std::string     min_version_;
    VersionProvider version_provider_;
};

// ---------------------------------------------------------------------------
// PreflightHealthChecker – orchestrator
// ---------------------------------------------------------------------------

/**
 * @brief Runs a battery of IHealthCheck instances before an update begins.
 *
 * All registered checks are executed sequentially.  The overall result
 * reports whether every check passed and includes the total wall-clock
 * duration so callers can verify the ≤ 2 s budget.
 *
 * Usage:
 * @code
 * PreflightHealthChecker checker;
 * checker.addCheck(std::make_unique<DiskSpaceChecker>(download_dir, 2 * bundle_size));
 * checker.addCheck(std::make_unique<MemoryHeadroomChecker>(256 * 1024 * 1024));
 *
 * auto result = checker.runAll();
 * if (!result.all_passed) {
 *     LOG_ERROR("Pre-flight check failed: {}", result.error_summary);
 *     return;
 * }
 * engine.applyHotReload(version);
 * @endcode
 */
class PreflightHealthChecker {
public:
    PreflightHealthChecker() = default;

    /// Non-copyable (owns unique_ptr checks).
    PreflightHealthChecker(const PreflightHealthChecker&)            = delete;
    PreflightHealthChecker& operator=(const PreflightHealthChecker&) = delete;

    /// Movable.
    PreflightHealthChecker(PreflightHealthChecker&&)            noexcept = default;
    PreflightHealthChecker& operator=(PreflightHealthChecker&&) noexcept = default;

    /**
     * @brief Register a health check.  Checks are run in registration order.
     * @param check Non-null unique_ptr to an IHealthCheck implementation.
     */
    void addCheck(std::unique_ptr<IHealthCheck> check);

    /**
     * @brief Execute all registered checks and return the aggregate result.
     *
     * Checks are run sequentially.  All checks are always executed (no early
     * abort) so the caller receives the full diagnostic picture.
     *
     * @return PreflightCheckResult with per-check details and total duration.
     */
    PreflightCheckResult runAll();

    /**
     * @brief Number of registered checks.
     */
    size_t checkCount() const;

private:
    std::vector<std::unique_ptr<IHealthCheck>> checks_;
};

} // namespace updates
} // namespace themis
