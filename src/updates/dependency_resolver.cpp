/**
 * @file dependency_resolver.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Dependency resolution engine for the Updates module.
//
// Algorithm overview:
//   Phase 1 – BFS over the registered dependency graph starting from the
//             main package at the requested version.  Discovers all packages
//             that need to be installed or updated and records their target
//             versions.
//
//   Phase 2 – Build a directed acyclic graph (DAG) over those packages using
//             their registered inter-dependencies.  Edge A → B means "update A
//             before B" (A is a dependency of B).
//
//   Phase 3 – Kahn's algorithm (deterministic BFS topological sort) over the
//             DAG.  Nodes with equal in-degree are processed in alphabetical
//             order for reproducible output.
//
//   Phase 4 – Cycle detection: nodes that still have in-degree > 0 after
//             exhausting the queue form a cyclic subgraph.
//
// Version constraint format: comma-separated tokens, each of the form
//   <op><version>, where <op> is one of >=, <=, >, <, ==, !=.
// All tokens are ANDed; an empty constraint is always satisfied.

#include "updates/dependency_resolver.h"

#include "utils/string_utils.h"
#include <algorithm>
#include <cstdio>
#include <map>
#include <queue>
#include <sstream>
#include <tuple>
#include <unordered_set>

namespace themis {
namespace updates {

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

/// Parse "major.minor.patch" into a comparable tuple.
/// Non-parseable or empty strings yield (0, 0, 0).
static std::tuple<int, int, int> parseVersion(const std::string& v) {
    int major = 0, minor = 0, patch = 0;
    if (!v.empty()) {
        (void)std::sscanf(v.c_str(), "%d.%d.%d", &major, &minor, &patch);
    }
    return {major, minor, patch};
}

/// Trim leading and trailing whitespace.
// Using themis::utils::trim() from string_utils.h (Phase 1 consolidation)

/// Split @p s on delimiter @p ch and trim each part.
static std::vector<std::string> splitOn(const std::string& s, char ch) {
    std::vector<std::string> parts;
    // Pre-allocate to reduce reallocations (Error Code: 7455)
    parts.reserve(std::count(s.begin(), s.end(), ch) + 1);
    std::string cur;
    for (char c : s) {
        if (c == ch) {
            parts.push_back(themis::utils::trim(cur));
            cur.clear();
        } else {
            cur += c;
        }
    }
    parts.push_back(themis::utils::trim(cur));
    return parts;
}

enum class ConstraintOp { GTE, GT, LTE, LT, EQ, NEQ };

struct ConstraintPart {
    ConstraintOp op;
    std::string  version; // e.g. "1.4.0"
};

/// Parse a single constraint token (e.g. ">=1.4.0") into a ConstraintPart.
/// Returns false when the token is malformed or does not start with a
/// recognised operator.
static bool parseConstraintToken(const std::string& token, ConstraintPart& out) {
    if (token.size() < 2) return false;
    size_t pos = 0;
    if (token[0] == '>' && token[1] == '=') {
        out.op = ConstraintOp::GTE; pos = 2;
    } else if (token[0] == '<' && token[1] == '=') {
        out.op = ConstraintOp::LTE; pos = 2;
    } else if (token[0] == '=' && token[1] == '=') {
        out.op = ConstraintOp::EQ;  pos = 2;
    } else if (token[0] == '!' && token[1] == '=') {
        out.op = ConstraintOp::NEQ; pos = 2;
    } else if (token[0] == '>') {
        out.op = ConstraintOp::GT;  pos = 1;
    } else if (token[0] == '<') {
        out.op = ConstraintOp::LT;  pos = 1;
    } else {
        return false;
    }
    out.version = themis::utils::trim(token.substr(pos));
    return !out.version.empty();
}

/// Evaluate whether @p ver satisfies a single ConstraintPart.
static bool evalConstraint(const std::tuple<int, int, int>& ver,
                            const ConstraintPart& c) {
    const auto cv = parseVersion(c.version);
    switch (c.op) {
        case ConstraintOp::GTE: return ver >= cv;
        case ConstraintOp::GT:  return ver >  cv;
        case ConstraintOp::LTE: return ver <= cv;
        case ConstraintOp::LT:  return ver <  cv;
        case ConstraintOp::EQ:  return ver == cv;
        case ConstraintOp::NEQ: return ver != cv;
    }
    return false; // unreachable
}

} // anonymous namespace

// ============================================================================
// DependencyResolver – static helpers
// ============================================================================

/*static*/ bool DependencyResolver::satisfiesConstraint(
    const std::string& version,
    const std::string& constraint)
{
    if (constraint.empty()) {
        return true; // no constraint → always satisfied
    }

    const auto ver    = parseVersion(version);
    const auto tokens = splitOn(constraint, ',');

    for (const auto& token : tokens) {
        if (token.empty()) continue;
        ConstraintPart part;
        if (!parseConstraintToken(token, part)) {
            // Malformed token – fail closed
            return false;
        }
        if (!evalConstraint(ver, part)) {
            return false;
        }
    }
    return true;
}

/*static*/ std::string DependencyResolver::minVersionFromConstraint(
    const std::string& constraint)
{
    if (constraint.empty()) return "";

    const auto tokens = splitOn(constraint, ',');
    for (const auto& token : tokens) {
        if (token.empty()) continue;
        ConstraintPart part;
        if (!parseConstraintToken(token, part)) continue;

        if (part.op == ConstraintOp::GTE || part.op == ConstraintOp::EQ) {
            return part.version;
        }
        if (part.op == ConstraintOp::GT) {
            // Bump patch by one to satisfy strict >
            auto [maj, min, pat] = parseVersion(part.version);
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%d.%d.%d", maj, min, pat + 1);
            return std::string(buf);
        }
    }
    return ""; // only upper-bound or != constraints → no lower bound
}

// ============================================================================
// DependencyResolver – registration
// ============================================================================

void DependencyResolver::addDependency(const std::string& version, Dependency dep) {
    deps_[""][version].push_back(std::move(dep));
}

void DependencyResolver::addPackageDependency(const std::string& package,
                                               const std::string& version,
                                               Dependency dep)
{
    deps_[package][version].push_back(std::move(dep));
}

// ============================================================================
// DependencyResolver::resolve
// ============================================================================

ResolutionResult DependencyResolver::resolve(
    const std::string& version,
    const std::unordered_map<std::string, std::string>& current_versions) const
{
    ResolutionResult result;

    // ── Phase 1: BFS to collect packages that need updating ───────────────
    //
    // node_target[pkg] = the version we intend to update/install pkg to.
    // The main package ("") is the BFS root; it never appears in node_target.
    std::unordered_map<std::string, std::string> node_target;

    // BFS state
    std::queue<std::pair<std::string, std::string>> bfs_queue; // (pkg, ver)
    std::unordered_set<std::string> visited;                   // "pkg@ver" keys

    auto enqueue = [&](const std::string& pkg, const std::string& ver) {
        const std::string key = pkg + "@" + ver;
        if (visited.insert(key).second) {
            bfs_queue.push({pkg, ver});
        }
    };

    enqueue("", version);

    while (!bfs_queue.empty()) {
        auto [pkg, ver] = bfs_queue.front();
        bfs_queue.pop();

        auto it_pkg = deps_.find(pkg);
        if (it_pkg == deps_.end()) continue;

        auto it_ver = it_pkg->second.find(ver);
        if (it_ver == it_pkg->second.end()) continue;

        for (const auto& dep : it_ver->second) {
            // ── Conflict detection ────────────────────────────────────────
            for (const auto& conflict_pkg : dep.conflicts) {
                // Conflict if the conflicting package is already installed or
                // already scheduled for update.
                if (current_versions.count(conflict_pkg) ||
                    node_target.count(conflict_pkg)) {
                    result.conflicts.push_back({
                        dep.package,
                        conflict_pkg,
                        dep.package + " conflicts with " + conflict_pkg
                    });
                }
            }

            // ── Determine whether this dep needs to be updated ────────────
            auto cur_it = current_versions.find(dep.package);
            const std::string cur_ver =
                (cur_it != current_versions.end()) ? cur_it->second : "";

            bool needs_update = false;
            std::string target_ver;

            if (cur_ver.empty()) {
                // Package not installed → backfill (required deps only)
                if (!dep.optional) {
                    result.backfilled.push_back(dep.package);
                    needs_update = true;
                    target_ver = minVersionFromConstraint(dep.version_constraint);
                    // When the constraint has no lower bound (e.g. "<=2.0.0", "!=1.0.0")
                    // or is empty, we cannot derive an install target automatically.
                    // For an empty constraint: to_version="" signals "any version" and
                    // the caller is responsible for choosing a concrete version.
                    // For a non-empty upper-bound-only constraint: fail immediately with
                    // an actionable error rather than storing an invalid version string.
                    if (target_ver.empty() && !dep.version_constraint.empty()) {
                        std::ostringstream oss;
                        oss << "Cannot auto-resolve target version for " << dep.package
                            << ": constraint \"" << dep.version_constraint
                            << "\" has no lower bound.  Specify an explicit >= or == clause.";
                        result.error_message = oss.str();
                        result.success = false;
                        return result;
                    }
                }
                // Optional absent deps are silently skipped.
            } else if (!dep.version_constraint.empty() &&
                       !satisfiesConstraint(cur_ver, dep.version_constraint)) {
                // Installed but version does not satisfy constraint → update
                needs_update = true;
                target_ver = minVersionFromConstraint(dep.version_constraint);
                // Same guard: upper-bound-only or exclusion-only constraints cannot
                // be auto-resolved to a valid install target.
                if (target_ver.empty()) {
                    std::ostringstream oss;
                    oss << "Cannot auto-resolve target version for " << dep.package
                        << ": constraint \"" << dep.version_constraint
                        << "\" has no lower bound.  Installed version is \"" << cur_ver
                        << "\".  Specify an explicit >= or == clause.";
                    result.error_message = oss.str();
                    result.success = false;
                    return result;
                }
            }

            if (needs_update && !node_target.count(dep.package)) {
                node_target[dep.package] = target_ver;
            }

            // Continue BFS using the post-update version (or current if no
            // update is needed) to discover transitive dependencies.
            const std::string next_ver =
                needs_update ? target_ver : cur_ver;
            if (!next_ver.empty()) {
                enqueue(dep.package, next_ver);
            }
        }
    }

    // ── Fail immediately on conflicts ─────────────────────────────────────
    if (!result.conflicts.empty()) {
        std::ostringstream oss;
        oss << "Dependency conflicts detected:";
        for (const auto& c : result.conflicts) {
            oss << " [" << c.package1 << " conflicts with " << c.package2 << "]";
        }
        result.error_message = oss.str();
        result.success = false;
        return result;
    }

    if (node_target.empty()) {
        // Nothing to update – all constraints already satisfied.
        result.success = true;
        return result;
    }

    // Deduplicate backfilled list.
    {
        auto& bf = result.backfilled;
        std::sort(bf.begin(), bf.end());
        bf.erase(std::unique(bf.begin(), bf.end()), bf.end());
    }

    // ── Phase 2: Build DAG for topological sort ───────────────────────────
    //
    // Edge convention: "dep.package → pkg" means dep.package must be updated
    // before pkg (i.e. dep.package is a predecessor of pkg in the sort).
    //
    // We only add edges for packages that are both in node_target.

    std::map<std::string, int> in_degree;
    std::map<std::string, std::vector<std::string>> successors; // dep → dependents

    for (const auto& kv : node_target) {
        in_degree[kv.first] = 0;
    }

    // Pre-reserve space in successors vectors to avoid reallocations (Error Code: 7457)
    size_t avg_deps_per_pkg = std::max(size_t(1), node_target.size() / 4);
    
    for (const auto& kv : node_target) {
        const std::string& pkg      = kv.first;
        const std::string& tgt_ver  = kv.second;

        auto it_pkg = deps_.find(pkg);
        if (it_pkg == deps_.end()) continue;

        auto it_ver = it_pkg->second.find(tgt_ver);
        if (it_ver == it_pkg->second.end()) continue;

        // Track edges already added for this pkg to guard against duplicate deps.
        std::unordered_set<std::string> added_edges;
        added_edges.reserve(it_ver->second.size());

        for (const auto& dep : it_ver->second) {
            // Only create an ordering edge when dep.package is also being updated.
            if (!in_degree.count(dep.package)) continue;

            // Deduplicate: only add each (dep.package → pkg) edge once.
            if (!added_edges.insert(dep.package).second) continue;

            // Pre-allocate in successors vector if not yet sized (Error Code: 7458)
            auto& succ_vec = successors[dep.package];
            if (succ_vec.empty()) {
                succ_vec.reserve(avg_deps_per_pkg);
            }
            succ_vec.push_back(pkg);
            in_degree[pkg]++;
        }
    }

    // ── Phase 3: Kahn's topological sort (deterministic) ─────────────────
    // Use std::set for ready queue to maintain deterministic sorted order
    // and enable efficient insertion/deletion (Error Code: 7456-7459)

    std::set<std::string> ready_set;
    for (const auto& kv : in_degree) {
        if (kv.second == 0) {
            ready_set.insert(kv.first);
        }
    }

    std::vector<std::string> sorted_pkgs;
    sorted_pkgs.reserve(node_target.size());  // Pre-allocate for efficiency
    
    while (!ready_set.empty()) {
        // Pop from front (alphabetically first) for determinism
        const std::string cur = *ready_set.begin();
        ready_set.erase(ready_set.begin());
        sorted_pkgs.push_back(cur);

        auto succ_it = successors.find(cur);
        if (succ_it != successors.end()) {
            for (const auto& succ : succ_it->second) {
                if (--in_degree[succ] == 0) {
                    ready_set.insert(succ);
                }
            }
        }
    }

    // ── Phase 4: Cycle detection ──────────────────────────────────────────

    if (sorted_pkgs.size() != node_target.size()) {
        std::vector<std::string> cycle_nodes;
       cycle_nodes.reserve(in_degree.size());  // Pre-allocate (Error Code: 7459)
       for (const auto& kv : in_degree) {
           if (kv.second > 0) cycle_nodes.push_back(kv.first);
       }
       std::sort(cycle_nodes.begin(), cycle_nodes.end());

       std::ostringstream oss;
       oss << "Circular dependency detected among packages:";
       for (const auto& cn : cycle_nodes) {
           oss << " " << cn;
       }
       result.error_message = oss.str();
       result.success = false;
       return result;
    }

    // ── Build UpdateStep list ─────────────────────────────────────────────

    for (const auto& pkg : sorted_pkgs) {
        UpdateStep step;
        step.package = pkg;
        const auto it = current_versions.find(pkg);
        step.from_version = (it != current_versions.end()) ? it->second : "";
        step.to_version   = node_target.at(pkg);
        result.steps.push_back(std::move(step));
    }

    result.success = true;
    return result;
}

// ============================================================================
// DependencyResolver::detectConflicts
// ============================================================================

std::vector<DependencyConflict> DependencyResolver::detectConflicts(
    const std::vector<std::pair<std::string, std::string>>& installed) const
{
    std::vector<DependencyConflict> conflicts;
    conflicts.reserve(installed.size());  // Pre-allocate for efficiency (Error Code: 7460)

    // Build a fast lookup of installed packages.
    std::unordered_map<std::string, std::string> installed_map;
    installed_map.reserve(installed.size());
    for (const auto& p : installed) {
        installed_map[p.first] = p.second;
    }

    // Lambda to add a conflict if not already present (avoid duplicates).
    auto addConflict = [&](const std::string& p1, const std::string& p2,
                            const std::string& reason) {
        for (const auto& c : conflicts) {
            if ((c.package1 == p1 && c.package2 == p2) ||
                (c.package1 == p2 && c.package2 == p1)) {
                return; // already recorded
            }
        }
        conflicts.push_back({p1, p2, reason});
    };

    // Check every installed package against its registered dependency declarations.
    for (const auto& inst : installed) {
        const std::string& pkg = inst.first;
        const std::string& ver = inst.second;

        auto it_pkg = deps_.find(pkg);
        if (it_pkg == deps_.end()) continue;

        auto it_ver = it_pkg->second.find(ver);
        if (it_ver == it_pkg->second.end()) continue;

        for (const auto& dep : it_ver->second) {
            // 1. Explicit conflicts list.
            for (const auto& conflict_pkg : dep.conflicts) {
                if (installed_map.count(conflict_pkg)) {
                    addConflict(dep.package, conflict_pkg,
                                dep.package + " conflicts with " + conflict_pkg);
                }
            }

            // 2. Version-constraint violations against other installed packages.
            if (!dep.optional && !dep.version_constraint.empty()) {
                const auto dep_it = installed_map.find(dep.package);
                if (dep_it != installed_map.end()) {
                    if (!satisfiesConstraint(dep_it->second,
                                             dep.version_constraint)) {
                        addConflict(pkg, dep.package,
                                    pkg + "@" + ver + " requires " + dep.package +
                                    " " + dep.version_constraint +
                                    " but got " + dep_it->second);
                    }
                }
            }
        }
    }

    // Also check the main package's ("") registered dependencies for explicit
    // conflicts between installed packages.
    const auto it_main = deps_.find("");
    if (it_main != deps_.end()) {
        for (const auto& ver_kv : it_main->second) {
            for (const auto& dep : ver_kv.second) {
                for (const auto& conflict_pkg : dep.conflicts) {
                    if (installed_map.count(dep.package) &&
                        installed_map.count(conflict_pkg)) {
                        addConflict(dep.package, conflict_pkg,
                                    dep.package + " conflicts with " + conflict_pkg);
                    }
                }
            }
        }
    }

    return conflicts;
}

} // namespace updates
} // namespace themis

