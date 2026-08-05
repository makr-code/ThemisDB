/**
 * @file distributed_flame_graph.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: distributed_flame_graph.h | Version: 0.0.15 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 217
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #5056 docs(observability): update... (2026-05-13) | #3320 feat(observability): distri... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "observability/continuous_profiler.h"

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace observability {

using json = nlohmann::json;

/**
 * @brief Profile snapshot contributed by a single cluster node.
 *
 * The @c snapshot field holds the profiling data in pprof folded-stacks
 * text format, identical to the format produced by @c ContinuousProfiler.
 */
struct NodeProfile {
    /// Unique identifier for the node (e.g. "node-1", "10.0.0.5:9100").
    std::string node_id;
    /// Optional human-readable hostname for display purposes.
    std::string host;
    /// Monotonic version for last-write-wins conflict resolution per node.
    uint64_t version{0};
    /// The profile snapshot contributed by this node.
    ProfileSnapshot snapshot;
};

/**
 * @brief A flame graph merged from one or more @c NodeProfile objects.
 *
 * The merged stacks are stored in the pprof folded-stacks text format
 * (one stack per line, space-separated frames, followed by an aggregate
 * sample count).  Sample counts are summed across all contributing nodes.
 *
 * Use @c toFoldedText() to obtain the merged profile as a string that
 * can be consumed directly by:
 *   - `flamegraph.pl` (Brendan Gregg's FlameGraph toolkit)
 *   - `go tool pprof`
 *   - async-profiler's collapsed output pipeline
 */
struct MergedFlameGraph {
    /// Merged stacks: folded-stack-string → aggregate sample count.
    std::map<std::string, uint64_t> stacks;
    /// Node IDs whose profiles contributed to this merge.
    std::vector<std::string> node_ids;
    /// Node versions captured in the merged output for deterministic provenance.
    std::map<std::string, uint64_t> node_versions;
    /// Timestamp at which the merge was performed.
    std::chrono::system_clock::time_point generated_at;

    /**
     * @brief Return the merged profile as pprof folded-stacks text.
     *
     * Each line has the form:
     * ```
     * frame1;frame2;frame3 <count>
     * ```
     * Lines are sorted by stack string for deterministic output.
     */
    std::string toFoldedText() const;

    /** @brief Serialize to JSON (metadata + folded text payload). */
    json toJSON() const;
};

/**
 * @brief Configuration for @c DistributedFlameGraph.
 */
struct DistributedFlameGraphConfig {
    /// Maximum number of distinct node IDs accepted before older entries
    /// are overwritten (last-write-wins per node_id).
    size_t max_nodes = 128;

    /**
     * @brief When true, sample counts from each node are normalised to a
     * unit weight of 1.0 before summing.  This gives equal visual weight
     * to every node regardless of its absolute sample volume, which is
     * useful when nodes have different sampling rates or uptime.
     */
    bool normalize_per_node = false;

    /// Maximum number of merged hotspots to expose in diff() output.
    size_t max_diff_hotspots = 20;
};

/**
 * @brief Aggregates CPU-profile snapshots from multiple cluster nodes and
 *        produces a unified distributed flame graph.
 *
 * ### Overview
 * Each ThemisDB node collects CPU profiling data via @c ContinuousProfiler.
 * @c DistributedFlameGraph collects those per-node snapshots, merges the
 * pprof folded-stacks data by summing (or normalising) sample counts for
 * each unique stack, and exposes the result as a @c MergedFlameGraph.
 *
 * ### Folded-stacks merging
 * Given two nodes with profiles:
 * ```
 * # node-1
 * main;query_exec;scan 40
 * main;compaction 10
 *
 * # node-2
 * main;query_exec;scan 30
 * main;flush 20
 * ```
 * The merged graph is:
 * ```
 * main;compaction 10
 * main;flush 20
 * main;query_exec;scan 70
 * ```
 *
 * ### Thread safety
 * All public methods are thread-safe.
 *
 * ### Usage
 * ```cpp
 * DistributedFlameGraph dfg;
 *
 * // Each node submits its local profile snapshot
 * dfg.addNodeProfile({"node-1", "10.0.0.1", profiler1.snapshot()});
 * dfg.addNodeProfile({"node-2", "10.0.0.2", profiler2.snapshot()});
 *
 * auto merged = dfg.merge();
 * merged.toFoldedText();   // pipe to flamegraph.pl
 * merged.toJSON();         // send to API consumer
 * ```
 */
class DistributedFlameGraph {
public:
    explicit DistributedFlameGraph(
        const DistributedFlameGraphConfig& config = DistributedFlameGraphConfig{});
    ~DistributedFlameGraph();

    // Non-copyable
    DistributedFlameGraph(const DistributedFlameGraph&) = delete;
    DistributedFlameGraph& operator=(const DistributedFlameGraph&) = delete;

    /**
     * @brief Add or replace the profile for @p profile.node_id.
     *
     * If a profile with the same @c node_id already exists it is
     * overwritten (last-write-wins).  Only @c ProfileType::CPU snapshots
     * contribute sample data; other types are accepted but ignored during
     * @c merge().
     */
    void addNodeProfile(const NodeProfile& profile);

    /**
     * @brief Remove all stored node profiles.
     */
    void clearProfiles();

    /**
     * @brief Merge all stored node profiles into a single flame graph.
     *
     * @return A @c MergedFlameGraph whose @c stacks map contains the
     *         aggregate (or normalised) sample counts across all nodes.
     *         Returns an empty graph if no profiles have been added.
     */
    MergedFlameGraph merge() const;

    /**
     * @brief Merge only the profiles for the specified @p node_ids.
     *
     * Node IDs not present in the store are silently ignored.
     *
     * @param node_ids  Subset of node IDs to include.
     * @return Merged flame graph for the selected nodes.
     */
    MergedFlameGraph mergeFiltered(const std::vector<std::string>& node_ids) const;

    /**
     * @brief Compute a differential between two merged flame graphs.
     *
     * Reuses @c ContinuousProfiler semantics: new/removed/changed hotspots
     * are identified by comparing the normalised share of each stack between
     * @p baseline and @p current.
     *
     * @param baseline  Earlier flame graph (reference).
     * @param current   Later flame graph (subject).
     * @return A @c ProfileDiff describing regressions and hotspot changes.
     */
    ProfileDiff diff(const MergedFlameGraph& baseline,
                     const MergedFlameGraph& current) const;

    /**
     * @brief Return the list of node IDs for which profiles are stored.
     */
    std::vector<std::string> getNodeIds() const;

    /**
     * @brief Return the number of stored node profiles.
     */
    size_t nodeCount() const;

    /**
     * @brief Retrieve the active configuration.
     */
    DistributedFlameGraphConfig getConfig() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace observability
} // namespace themis
