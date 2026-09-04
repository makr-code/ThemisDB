/**
 * @file dependency_resolver.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace themis {
namespace updates {

// ---------------------------------------------------------------------------
// Dependency – declaration of a single package dependency
// ---------------------------------------------------------------------------

/**
 * @brief Describes a dependency on an external package.
 */
struct Dependency {
    /// Package name (e.g. "themis-storage")
    std::string package;

    /**
     * @brief Version constraint string.
     *
     * Supported operators: >=, <=, >, <, ==, !=
     * Multiple constraints joined by comma are ANDed, e.g. ">=1.4.0,<2.0.0".
     * Empty string means "any version".
     */
    std::string version_constraint;

    /// When true, the dependency is optional; resolution does not fail if the
    /// package is absent, and it is not added to the backfilled list.
    bool optional = false;

    /// Package names that conflict with this dependency.
    /// If any package in this list is currently installed (or is being added to
    /// the update plan), a DependencyConflict is reported and resolution fails.
    std::vector<std::string> conflicts;
};

// ---------------------------------------------------------------------------
// UpdateStep – a single package update in the ordered plan
// ---------------------------------------------------------------------------

/**
 * @brief A single package update that must be applied as part of an update plan.
 */
struct UpdateStep {
    /// Package to update (e.g. "themis-storage")
    std::string package = {};

    /// Currently installed version, or "" if the package was not installed
    /// (i.e. it was backfilled).
    std::string from_version;

    /// Target version to install / update to (derived from the version constraint)
    std::string to_version;
};

// ---------------------------------------------------------------------------
// DependencyConflict – a detected conflict between two packages
// ---------------------------------------------------------------------------

/**
 * @brief Describes a conflict between two packages.
 */
struct DependencyConflict {
    std::string package1; ///< First package involved in the conflict
    std::string package2; ///< Second package involved in the conflict
    std::string reason;   ///< Human-readable explanation
};

// ---------------------------------------------------------------------------
// ResolutionResult – outcome of a resolve() call
// ---------------------------------------------------------------------------

/**
 * @brief Result of a dependency resolution.
 */
struct ResolutionResult {
    /// true when resolution succeeded (no cycles, no missing required deps,
    /// no conflicts).
    bool success = false;

    /// Topologically sorted list of package update steps.
    /// Dependencies always appear before the packages that depend on them.
    std::vector<UpdateStep> steps;

    /// Detected conflicts (non-empty implies success == false).
    std::vector<DependencyConflict> conflicts;

    /// Packages that were not present in current_versions and were automatically
    /// added ("backfilled") to the update plan.
    std::vector<std::string> backfilled;

    /// Human-readable failure description (empty on success).
    std::string error_message;
};

// ---------------------------------------------------------------------------
// DependencyResolver
// ---------------------------------------------------------------------------

/**
 * @brief Dependency resolver for the updates module.
 *
 * Thread-safety: the resolver is NOT thread-safe.  External synchronisation
 * must be provided if addDependency / addPackageDependency are called
 * concurrently with resolve / detectConflicts.
 */
class DependencyResolver {
public:
    DependencyResolver() = default;

    // ── Registration ──────────────────────────────────────────────────────

    /**
     * @brief Register a dependency for the given version of the main package.
     *
     * The "main package" is the ThemisDB instance being updated.  Call this
     * for each dependency of the target release version.
     *
     * @param version  Target release version (e.g. "1.5.0")
     * @param dep      Dependency descriptor
     */
    void addDependency(const std::string& version, Dependency dep);

    /**
     * @brief Register a dependency for a specific named package at the given
     *        version.
     *
     * Use this to model cascading (transitive) dependencies between external
     * packages.  For example, if themis-query 1.4.5 depends on themis-storage,
     * call addPackageDependency("themis-query", "1.4.5", storageDepDescriptor).
     *
     * @param package  Name of the external package (e.g. "themis-query")
     * @param version  Version of that package (e.g. "1.4.5")
     * @param dep      Dependency descriptor
     */
    void addPackageDependency(const std::string& package,
                              const std::string& version,
                              Dependency dep);

    // ── Resolution ────────────────────────────────────────────────────────

    /**
     * @brief Resolve the update plan for the given target version.
     *
     * Performs a BFS over the registered dependency graph starting from the
     * main package at @p version, then topologically sorts the packages that
     * need to be updated or installed.
     *
     * @param version          Target version of the main package (e.g. "1.5.0")
     * @param current_versions Map of package name → currently installed version
     * @return ResolutionResult with topologically sorted UpdateStep list
     */
    ResolutionResult resolve(
        const std::string& version,
        const std::unordered_map<std::string, std::string>& current_versions) const;

    /**
     * @brief Detect conflicts in a set of installed packages.
     *
     * For each installed package that has registered dependency declarations,
     * checks whether:
     *   - Any explicit conflicts[] package is also installed.
     *   - Any required dependency's installed version violates the constraint.
     *
     * @param installed  List of (package, version) pairs
     * @return List of detected conflicts (may be empty)
     */
    std::vector<DependencyConflict> detectConflicts(
        const std::vector<std::pair<std::string, std::string>>& installed) const;

    // ── Static helpers ────────────────────────────────────────────────────

    /**
     * @brief Check whether @p version satisfies @p constraint.
     *
     * @param version    A "major.minor.patch" version string
     * @param constraint A constraint such as ">=1.4.0,<2.0.0"
     * @return true if the version satisfies every clause in the constraint
     */
    static bool satisfiesConstraint(const std::string& version,
                                    const std::string& constraint);

    /**
     * @brief Extract the minimum version implied by @p constraint.
     *
     * Returns the version string from the first >= or == clause, or from the
     * first > clause with the patch component incremented by one.
     * Returns "" when no lower bound is present.
     *
     * @param constraint  A constraint string (e.g. ">=1.4.0,<2.0.0")
     * @return Minimum satisfying version string, or "" if unconstrained below
     */
    static std::string minVersionFromConstraint(const std::string& constraint);

private:
    // deps_[package][version] → list of Dependency
    // The "main package" (ThemisDB itself) is stored under the empty string key.
    std::unordered_map<std::string,
        std::unordered_map<std::string, std::vector<Dependency>>> deps_;
};

} // namespace updates
} // namespace themis
