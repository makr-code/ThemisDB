/**
 * @file module_dependency_resolver.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=11, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Module dependency resolver implementation
// Provides topological-sort-based load-order management for ThemisDB modules.
//
// Algorithm: Kahn's algorithm (BFS-based topological sort).
//   - Required dependencies create ordering edges; missing required deps are errors.
//   - Optional dependencies create ordering edges only when both modules are registered.
//   - Circular dependencies are detected and reported.
//   - Version constraints (minVersion/maxVersion) are validated against the registered
//     version of each dependency; violations are reported via versionMismatches.

#include "themis/base/module_loader.h"
#include <algorithm>
#include <cstdio>
#include <queue>
#include <sstream>
#include <tuple>

namespace themis {
namespace modules {

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

/// Parse a "major.minor.patch" string into a comparable tuple.
/// Non-parseable strings and empty strings both yield (0, 0, 0).
/// Components that cannot be parsed (e.g. "1.2.x" → patch fails) retain
/// their zero-initialised value, giving a conservative lower bound.
std::tuple<int, int, int> parseVersion(const std::string& v) {
    int major = 0, minor = 0, patch = 0;
    if (!v.empty()) {
        // Intentionally ignore return value: variables are pre-initialised to 0,
        // so partial parses (e.g. "1.2.x") degrade gracefully to (1, 2, 0).
        (void)std::sscanf(v.c_str(), "%d.%d.%d", &major, &minor, &patch);
    }
    return {major, minor, patch};
}

} // anonymous namespace

// ============================================================================
// ModuleDependencyResolver::isVersionCompatible
// ============================================================================

/*static*/ bool ModuleDependencyResolver::isVersionCompatible(
    const std::string& version,
    const std::string& minVersion,
    const std::string& maxVersion)
{
    // An unversioned module satisfies only unconstrained dependencies.
    if (version.empty()) {
        return minVersion.empty() && maxVersion.empty();
    }

    const auto ver = parseVersion(version);

    if (!minVersion.empty() && ver < parseVersion(minVersion)) {
        return false;
    }
    if (!maxVersion.empty() && ver > parseVersion(maxVersion)) {
        return false;
    }
    return true;
}

// ============================================================================
// ModuleDependencyResolver public interface
// ============================================================================

void ModuleDependencyResolver::registerModule(
    const std::string& name,
    const std::string& version,
    const std::vector<ModuleDependency>& deps)
{
    modules_[name] = {version, deps};
}

void ModuleDependencyResolver::registerModule(
    const std::string& name,
    const std::vector<ModuleDependency>& deps)
{
    modules_[name] = {"", deps};
}

void ModuleDependencyResolver::clear()
{
    modules_.clear();
}

std::vector<ModuleDependencyResolver::RegisteredModuleInfo>
ModuleDependencyResolver::getRegisteredModules() const
{
    std::vector<RegisteredModuleInfo> result = {};

    result.reserve(modules_.size());
    for (const auto& kv : modules_) {
        RegisteredModuleInfo info;
        info.name    = kv.first;
        info.version = kv.second.version;
        info.deps    = kv.second.deps;
        result.push_back(std::move(info));
    }
    // Already sorted because modules_ is a std::map (sorted by key).
    return result;
}

DependencyResolutionResult ModuleDependencyResolver::resolve() const
{
    std::vector<std::string> all = {};

    all.reserve(modules_.size());
    for (const auto& kv : modules_) {
        all.push_back(kv.first);
    }
    return topologicalSort(all);
}

DependencyResolutionResult ModuleDependencyResolver::resolveFor(
    const std::vector<std::string>& moduleNames) const
{
    // Build the closure: moduleNames + all registered transitive dependencies.
    std::map<std::string, bool> visited;
    std::queue<std::string> toVisit;
    DependencyResolutionResult precheck;

    for (const auto& n : moduleNames) {
        if (!modules_.count(n)) {
            precheck.missingRequired.push_back(n);
            continue;
        }

        if (visited.emplace(n, true).second) {
            toVisit.push(n);
        }
    }

    if (!precheck.missingRequired.empty()) {
        std::sort(precheck.missingRequired.begin(), precheck.missingRequired.end());
        precheck.missingRequired.erase(
            std::unique(precheck.missingRequired.begin(), precheck.missingRequired.end()),
            precheck.missingRequired.end());

        std::ostringstream oss = {};
        oss << "Missing required dependencies:";
        for (const auto& m : precheck.missingRequired) {
            oss << ' ' << m;
        }
        precheck.errorMessage = oss.str();
        precheck.success = false;
        return precheck;
    }

    while (!toVisit.empty()) {
        auto cur = toVisit.front();
        toVisit.pop();

        auto it = modules_.find(cur);
        if (it == modules_.end()) {
            continue;
        }
        for (const auto& dep : it->second.deps) {
            if (!modules_.count(dep.name)) {
                continue;
            }

            if (visited.emplace(dep.name, true).second) {
                toVisit.push(dep.name);
            }
        }
    }

    std::vector<std::string> closure = {};

    closure.reserve(visited.size());
    for (const auto& kv : visited) {
        closure.push_back(kv.first);
    }

    return topologicalSort(closure);
}

// ============================================================================
// Kahn's algorithm — deterministic topological sort
// ============================================================================

DependencyResolutionResult ModuleDependencyResolver::topologicalSort(
    const std::vector<std::string>& nodes) const
{
    DependencyResolutionResult result;

    // Build in-degree map and reverse-adjacency (dep → dependents).
    // Only edges within the supplied node set are considered.
    std::map<std::string, int> inDegree;
    std::map<std::string, std::vector<std::string>> dependents;

    for (const auto& n : nodes) {
        inDegree[n] = 0;
    }

    for (const auto& n : nodes) {
        auto it = modules_.find(n);
        if (it == modules_.end()) {
            continue;
        }
        for (const auto& dep : it->second.deps) {
            const bool inNodeSet = inDegree.count(dep.name) > 0;

            if (dep.required) {
                if (!modules_.count(dep.name)) {
                    // Required dependency not registered at all.
                    result.missingRequired.push_back(dep.name);
                    continue;
                }
                if (!inNodeSet) {
                    // Dep is registered but outside the requested closure —
                    // treat as missing for this resolution pass.
                    result.missingRequired.push_back(dep.name);
                    continue;
                }

                // Validate version constraints against the registered version.
                if (!dep.minVersion.empty() || !dep.maxVersion.empty()) {
                    const std::string& depVersion = modules_.at(dep.name).version;
                    if (!isVersionCompatible(depVersion, dep.minVersion, dep.maxVersion)) {
                        std::ostringstream oss = {};
                        oss << n << " requires " << dep.name
                            << " version";
                        if (!dep.minVersion.empty()) {
                            oss << " >=" << dep.minVersion;
                        }
                        if (!dep.maxVersion.empty()) {
                            oss << " <=" << dep.maxVersion;
                        }
                        oss << " but got \"" << depVersion << "\"";
                        result.versionMismatches.push_back(oss.str());
                    }
                }

                // Add ordering edge: dep must come before n.
                dependents[dep.name].push_back(n);
                inDegree[n]++;
            } else {
                // Optional dependency: create edge only when dep is in the set.
                if (inNodeSet) {
                    // Version constraints on optional deps are enforced the same
                    // way as required deps — a violation fails resolution.
                    if ((!dep.minVersion.empty() || !dep.maxVersion.empty()) &&
                        modules_.count(dep.name))
                    {
                        const std::string& depVersion = modules_.at(dep.name).version;
                        if (!isVersionCompatible(depVersion, dep.minVersion, dep.maxVersion)) {
                            std::ostringstream oss = {};
                            oss << n << " optionally requires " << dep.name
                                << " version";
                            if (!dep.minVersion.empty()) {
                                oss << " >=" << dep.minVersion;
                            }
                            if (!dep.maxVersion.empty()) {
                                oss << " <=" << dep.maxVersion;
                            }
                            oss << " but got \"" << depVersion << "\"";
                            result.versionMismatches.push_back(oss.str());
                        }
                    }
                    dependents[dep.name].push_back(n);
                    inDegree[n]++;
                }
                // If optional dep is absent, silently ignore it.
            }
        }
    }

    // Deduplicate missing required deps.
    {
        auto& mr = result.missingRequired;
        std::sort(mr.begin(), mr.end());
        mr.erase(std::unique(mr.begin(), mr.end()), mr.end());
    }

    if (!result.missingRequired.empty()) {
        std::ostringstream oss = {};
        oss << "Missing required dependencies:";
        for (const auto& m : result.missingRequired) {
            oss << " " << m;
        }
        result.errorMessage = oss.str();
        result.success = false;
        return result;
    }

    if (!result.versionMismatches.empty()) {
        std::ostringstream oss = {};
        oss << "Version constraint violations:";
        for (const auto& vm : result.versionMismatches) {
            oss << " [" << vm << "]";
        }
        result.errorMessage = oss.str();
        result.success = false;
        return result;
    }

    // Kahn's algorithm: start with all nodes that have no unresolved deps.
    // Use a sorted vector to guarantee deterministic (alphabetical) output.
    std::vector<std::string> ready = {};

    for (const auto& kv : inDegree) {
        if (kv.second == 0) {
            ready.push_back(kv.first);
        }
    }
    std::sort(ready.begin(), ready.end());

    while (!ready.empty()) {
        const std::string cur = ready.front();
        ready.erase(ready.begin());
        result.loadOrder.push_back(cur);

        for (const auto& dependent : dependents[cur]) {
            if (--inDegree[dependent] == 0) {
                // Insert in sorted order to maintain determinism.
                auto pos = std::lower_bound(ready.begin(), ready.end(), dependent);
                ready.insert(pos, dependent);
            }
        }
    }

    // If not all nodes were processed, at least one cycle exists.
    if (static_cast<int>(result.loadOrder.size()) != nodes.size()) {
        std::vector<std::string> cycleNodes = {};

        for (const auto& kv : inDegree) {
            if (kv.second > 0) {
                cycleNodes.push_back(kv.first);
            }
        }
        std::sort(cycleNodes.begin(), cycleNodes.end());
        result.cycles.push_back(cycleNodes);

        std::ostringstream oss = {};
        oss << "Circular dependency detected among modules:";
        for (const auto& cn : cycleNodes) {
            oss << " " << cn;
        }
        result.errorMessage = oss.str();
        result.success = false;
        return result;
    }

    result.success = true;
    return result;
}

} // namespace modules
} // namespace themis
