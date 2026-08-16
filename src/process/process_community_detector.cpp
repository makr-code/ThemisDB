/**
 * @file process_community_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB - Process Modeling Module
 *
 * File:    process_community_detector.cpp
 * Module:  src/process/
 * Purpose: Leiden-style greedy modularity-based community detection
 *          over process model graphs (P4 – GraphRAG, Edge 2024).
 */

#include "process/process_community_detector.h"
#include "process/process_model_manager.h"
#include "utils/logger.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace themis {
namespace process {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

struct Graph {
    std::vector<std::string> node_ids;
    std::map<std::string, int> node_index;  // node_id → index (deterministic order)
    std::vector<std::unordered_map<int, float>> adj;  // adjacency list (weighted, O(1) lookup)
    std::vector<float> degree;              // weighted degree per node
    float total_weight{0.f};                // 2m = sum of all edge weights
};

Graph buildGraph(const json& normalized) {
    Graph g;
    if (!normalized.contains("nodes") || !normalized.contains("edges")) {
        return g;
    }

    for (const auto& n : normalized["nodes"]) {
        const std::string id = n.value("id", "");
        if (id.empty()) continue;
        if (g.node_index.find(id) == g.node_index.end()) {
            g.node_index[id] = static_cast<int>(g.node_ids.size());
            g.node_ids.push_back(id);
        }
    }

    const int n = static_cast<int>(g.node_ids.size());
    g.adj.resize(n);
    g.degree.assign(n, 0.f);

    for (const auto& e : normalized["edges"]) {
        const std::string from = e.value("from", e.value("source", ""));
        const std::string to   = e.value("to",   e.value("target", ""));
        if (from.empty() || to.empty()) continue;

        auto fi = g.node_index.find(from);
        auto ti = g.node_index.find(to);
        if (fi == g.node_index.end() || ti == g.node_index.end()) continue;

        const int u = fi->second;
        const int v = ti->second;
        constexpr float w = 1.f;

        g.adj[u][v] += w;
        g.adj[v][u] += w;  // treat as undirected for modularity
        g.degree[u] += w;
        g.degree[v] += w;
        g.total_weight += w;
    }

    return g;
}

/// Modularity gain when moving node u into community C.
/// ΔQ = [Σ(in) / m − (Σ(tot) + k_u)^2 / (2m)^2]
///     − [Σ(in) / m − Σ(tot)^2 / (2m)^2 − k_u^2 / (2m)^2]
/// Simplified: ΔQ = (k_u_in / m) − (Σ(tot) * k_u / (2m)^2) * resolution
float modularityGain(
    int u,
    const std::set<int>& community_nodes,
    const Graph& g,
    float resolution)
{
    const float two_m = g.total_weight * 2.f;
    if (two_m <= 0.f) return 0.f;

    float k_u_in = 0.f;
    float sigma_tot = 0.f;
    for (int v : community_nodes) {
        auto it = g.adj[u].find(v);
        if (it != g.adj[u].end()) k_u_in += it->second;
        sigma_tot += g.degree[v];
    }

    return (k_u_in / g.total_weight) - (resolution * sigma_tot * g.degree[u] / (two_m * two_m / 2.f));
}

/// Sum of edge weights from node u into the given community.
float communityAttachment(
    int u,
    const std::set<int>& community_nodes,
    const Graph& g)
{
    float weight = 0.f;
    for (int v : community_nodes) {
        auto it = g.adj[u].find(v);
        if (it != g.adj[u].end()) {
            weight += it->second;
        }
    }
    return weight;
}

/// Run one phase of Louvain: iterate nodes and move each to the neighbouring
/// community with the best modularity gain. Returns true if any node moved.
bool louvainPhase(
    std::vector<int>& assignment,         // node → community label
    const Graph& g,
    float resolution)
{
    const int n = static_cast<int>(g.node_ids.size());
    bool improved = false;

    for (int u = 0; u < n; ++u) {
        const int current_comm = assignment[u];

        // Collect all neighbouring communities
        std::map<int, std::set<int>> comm_nodes;
        for (int i = 0; i < n; ++i) {
            comm_nodes[assignment[i]].insert(i);
        }

        float best_gain = 0.f;
        int best_comm = current_comm;
        const float current_attachment = communityAttachment(u, comm_nodes[current_comm], g);

        // Evaluate each neighbouring community
        std::set<int> visited_comms;
        for (const auto& [v, _] : g.adj[u]) {
            const int nc = assignment[v];
            if (!visited_comms.insert(nc).second) continue;
            if (nc == current_comm) continue;

            const float gain = modularityGain(u, comm_nodes[nc], g, resolution);
            const float target_attachment = communityAttachment(u, comm_nodes[nc], g);
            // Prevent bridge-driven over-merges: only move if the node is more
            // strongly attached to the target community than to its current one.
            if (gain > best_gain && target_attachment > current_attachment) {
                best_gain = gain;
                best_comm = nc;
            }
        }

        if (best_comm != current_comm) {
            assignment[u] = best_comm;
            improved = true;
        }
    }
    return improved;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// ProcessCommunityDetector
// ─────────────────────────────────────────────────────────────────────────────

ProcessCommunityDetector::ProcessCommunityDetector(RocksDBWrapper& db)
    : db_(db)
{}

std::vector<ProcessCommunity> ProcessCommunityDetector::detect(
    std::string_view model_id,
    float resolution) const
{
    ProcessModelManager mgr(db_);
    auto opt = mgr.load(model_id);
    if (!opt.has_value()) {
        THEMIS_WARN("ProcessCommunityDetector::detect: model '{}' not found", model_id);
        return {};
    }

    const auto& normalized = opt->normalized;
    if (!normalized.contains("nodes") || normalized["nodes"].empty()) {
        return {};
    }

    Graph g = buildGraph(normalized);
    const int n = static_cast<int>(g.node_ids.size());
    if (n == 0) return {};

    // Build a lookup: node_id → node name/description for report generation
    std::unordered_map<std::string, std::string> node_names;
    node_names.reserve(normalized["nodes"].size());
    for (const auto& node_json : normalized["nodes"]) {
        const std::string id = node_json.value("id", "");
        const std::string nm = node_json.value("name",
                                node_json.value("label", id));
        if (!id.empty()) node_names[id] = nm;
    }

    // Initialise: each node in its own community (community label = node index)
    std::vector<int> assignment(n);
    std::iota(assignment.begin(), assignment.end(), 0);

    // Phase 1: iterate until no improvement
    constexpr int kMaxIterations = 100;
    for (int iter = 0; iter < kMaxIterations; ++iter) {
        if (!louvainPhase(assignment, g, resolution)) break;
    }

    // Phase 2: build super-graph and repeat (one level of coarsening)
    if (n > 1) {
        // Map old community labels to [0..num_comms)
        std::map<int, int> label_remap;
        int comm_count = 0;
        for (int label : assignment) {
            if (label_remap.find(label) == label_remap.end()) {
                label_remap[label] = comm_count++;
            }
        }
        // Remap
        for (auto& a : assignment) a = label_remap[a];

        // Build super-graph only when there is enough structure to benefit
        // from a second-level optimization. For exactly two communities this
        // phase can over-merge into one community on small sparse bridge
        // graphs (e.g. two cliques connected by one edge).
        if (comm_count > 2) {
            Graph sg;
            sg.node_ids.resize(comm_count);
            for (int i = 0; i < comm_count; ++i) sg.node_ids[i] = std::to_string(i);
            sg.adj.resize(comm_count);
            sg.degree.assign(comm_count, 0.f);
            for (int u = 0; u < n; ++u) {
                for (const auto& [v, w] : g.adj[u]) {
                    const int cu = assignment[u];
                    const int cv = assignment[v];
                    if (cu == cv) continue;
                    sg.adj[cu][cv] += w;
                    sg.degree[cu] += w;
                    sg.total_weight += w;
                }
            }
            if (sg.total_weight > 0.f) {
                std::vector<int> sg_assign(comm_count);
                std::iota(sg_assign.begin(), sg_assign.end(), 0);
                for (int iter = 0; iter < kMaxIterations; ++iter) {
                    if (!louvainPhase(sg_assign, sg, resolution)) break;
                }
                // Propagate super-graph assignment back
                for (auto& a : assignment) a = sg_assign[a];
            }
        }
    }

    // Collect communities
    std::map<int, std::vector<int>> comm_map;
    for (int u = 0; u < n; ++u) {
        comm_map[assignment[u]].push_back(u);
    }

    // Compute global modularity Q for score computation
    const float two_m = g.total_weight * 2.f;

    std::vector<ProcessCommunity> communities;
    communities.reserve(comm_map.size());

    int idx = 0;
    for (const auto& [label, members] : comm_map) {
        ProcessCommunity pc;
        pc.community_id = "community_" + std::to_string(idx++);

        pc.node_ids.reserve(members.size());
        for (int u : members) pc.node_ids.push_back(g.node_ids[u]);

        // Compute local modularity contribution
        // OPTIMIZATION: Convert members to set for O(log n) lookup instead of O(n²) iterations
        float sum_in = 0.f;
        float sum_tot = 0.f;
        std::set<int> member_set(members.begin(), members.end());
        for (int u : members) {
            sum_tot += g.degree[u];
            // Iterate only through actual edges from u, checking if v is in community
            for (const auto& [v, weight] : g.adj[u]) {
                if (member_set.count(v)) {
                    sum_in += weight;
                }
            }
        }
        if (two_m > 0.f) {
            pc.modularity_score = (sum_in / g.total_weight) -
                resolution * (sum_tot / two_m) * (sum_tot / two_m);
        }

        // Label: first 3 node names joined with "; "
        std::ostringstream label_ss;
        const int label_count = std::min(static_cast<int>(pc.node_ids.size()), 3);
        for (int i = 0; i < label_count; ++i) {
            if (i > 0) label_ss << "; ";
            auto nit = node_names.find(pc.node_ids[i]);
            label_ss << (nit != node_names.end() ? nit->second : pc.node_ids[i]);
        }
        pc.label = label_ss.str();

        // Report via generateReport (stub)
        pc.report = generateReport(pc, model_id, "", "de");

        communities.push_back(std::move(pc));
    }

    // Sort by size descending
    std::sort(communities.begin(), communities.end(),
              [](const ProcessCommunity& a, const ProcessCommunity& b) {
                  return a.node_ids.size() > b.node_ids.size();
              });

    THEMIS_INFO("ProcessCommunityDetector: detected {} communities for model '{}'",
                communities.size(), model_id);
    return communities;
}

std::string ProcessCommunityDetector::generateReport(
    const ProcessCommunity& community,
    [[maybe_unused]] std::string_view model_id,
    [[maybe_unused]] std::string_view llm_endpoint,
    std::string_view language) const
{
    // Structured community report built from available metadata (node IDs, label,
    // modularity score).  Produces a richer summary than the former stub which
    // only listed the first three nodes.  The real LLM-backed path remains
    // planned for Q4 2026 (STUB_INVENTORY #238); once integrated, `llm_endpoint`
    // will be used to obtain an abstractive description via HTTP POST.

    if (community.node_ids.empty()) return {};

    const bool german = (language == "de");
    std::ostringstream oss;

    const int n = static_cast<int>(community.node_ids.size());

    if (german) {
        oss << "Gemeinschaft '" << community.community_id << "': "
            << n << (n == 1 ? " Knoten" : " Knoten");
    } else {
        oss << "Community '" << community.community_id << "': "
            << n << (n == 1 ? " node" : " nodes");
    }

    // Label (first 3 node names joined by "; ", already computed in detect())
    if (!community.label.empty()) {
        oss << " [" << community.label << "]";
    }

    // Modularity contribution (use ostringstream for safe float formatting)
    {
        std::ostringstream mod_oss;
        mod_oss << std::fixed;
        mod_oss.precision(4);
        mod_oss << community.modularity_score;
        if (german) {
            oss << "; Modularität=" << mod_oss.str();
        } else {
            oss << "; modularity=" << mod_oss.str();
        }
    }

    // Node list (up to 10 nodes, then ellipsis)
    constexpr int kMaxNodes = 10;
    const int show = std::min(n, kMaxNodes);
    if (german) {
        oss << "; Knoten:";
    } else {
        oss << "; nodes:";
    }
    for (int i = 0; i < show; ++i) {
        oss << (i == 0 ? " " : ", ") << community.node_ids[i];
    }
    if (n > kMaxNodes) {
        if (german) {
            oss << " ... (" << (n - kMaxNodes) << " weitere)";
        } else {
            oss << " ... (" << (n - kMaxNodes) << " more)";
        }
    }

    return oss.str();
}

bool ProcessCommunityDetector::persistCommunities(
    std::string_view model_id,
    const std::vector<ProcessCommunity>& communities)
{
    for (const auto& c : communities) {
        const std::string key = std::string("proc:community:") +
                                std::string(model_id) + ":" + c.community_id;
        json doc;
        doc["community_id"]     = c.community_id;
        doc["node_ids"]         = c.node_ids;
        doc["label"]            = c.label;
        doc["report"]           = c.report;
        doc["modularity_score"] = c.modularity_score;

        if (!db_.put(key, doc.dump())) {
            THEMIS_ERROR("ProcessCommunityDetector: failed to persist '{}'", key);
            return false;
        }
    }
    THEMIS_INFO("ProcessCommunityDetector: persisted {} communities for model '{}'",
                communities.size(), model_id);
    return true;
}

std::vector<ProcessCommunity> ProcessCommunityDetector::loadCommunities(
    std::string_view model_id) const
{
    const std::string prefix = std::string("proc:community:") + std::string(model_id) + ":";
    std::vector<ProcessCommunity> result;

    db_.scanPrefix(prefix, [&](std::string_view /*key*/, std::string_view value) -> bool {
        try {
            const auto doc = json::parse(value);
            ProcessCommunity c;
            c.community_id     = doc.value("community_id", "");
            c.node_ids         = doc.value("node_ids", std::vector<std::string>{});
            c.label            = doc.value("label", "");
            c.report           = doc.value("report", "");
            c.modularity_score = doc.value("modularity_score", 0.f);
            result.push_back(std::move(c));
        } catch (const std::exception& ex) {
            THEMIS_WARN("ProcessCommunityDetector::loadCommunities: parse error: {}", ex.what());
        }
        return true;  // continue scanning
    });

    return result;
}

} // namespace process
} // namespace themis

