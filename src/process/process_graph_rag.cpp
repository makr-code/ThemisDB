/**
 * @file process_graph_rag.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=35, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB - Process Modeling Module
 *
 * File:    process_graph_rag.cpp
 * Module:  src/process/
 * Purpose: Graph-RAG engine for German administrative proceedings.
 *          Bridges the process execution graph with the KnowledgeGraphRetriever
 *          to produce LLM-ready retrieval contexts.
 */

#include "process/process_graph_rag.h"
#include <stdexcept>
#include "process/process_common.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <numeric>
#include <queue>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace themis {
namespace process {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Anonymous namespace helpers
// ─────────────────────────────────────────────────────────────────────────────
namespace {

/// Cosine similarity between two float vectors; returns 0.0 when either is
/// empty or the norms are zero.
float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.empty() || b.empty() || a.size() != b.size()) {
      return 0.f;
    }
    float dot = 0.f, na = 0.f, nb = 0.f;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        na  += a[i] * a[i];
        nb  += b[i] * b[i];
    }
    if (na == 0.f || nb == 0.f) {
      return 0.f;
    }
    return dot / (std::sqrt(na) * std::sqrt(nb));
}

/// Jaccard similarity between two sets of strings.
float jaccardSimilarity(const std::set<std::string>& a,
                        const std::set<std::string>& b) {
    if (a.empty() && b.empty()) {
      return 1.f;
    }
    size_t intersection = 0;
    for (const auto& s : a) {
        if (b.count(s)) {
          ++intersection;
        }
    }
    const size_t union_size = static_cast<int>(a.size()) + static_cast<int>(b.size()) - intersection;
    if (union_size == 0) {
      return 0.f;
    }
    return static_cast<float>(intersection) / static_cast<float>(union_size);
}

/// Convert a ProcessInstance::State enum value to a string.
std::string instanceStateStr(ProcessInstance::State s) {
    switch (s) {
        case ProcessInstance::State::CREATED:    return "CREATED";
        case ProcessInstance::State::RUNNING:    return "RUNNING";
        case ProcessInstance::State::SUSPENDED:  return "SUSPENDED";
        case ProcessInstance::State::COMPLETED:  return "COMPLETED";
        case ProcessInstance::State::TERMINATED: return "TERMINATED";
        case ProcessInstance::State::FAILED:     return "FAILED";
    }
    return "UNKNOWN";
}

/// Check whether @p needle appears (case-insensitive) in @p haystack.
bool containsInsensitive(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) {
      return true;
    }
    auto it = std::search(haystack.begin(), haystack.end(),
                          needle.begin(), needle.end(),
                          [](char a, char b) {
                              return std::tolower(static_cast<unsigned char>(a)) ==
                                     std::tolower(static_cast<unsigned char>(b));
                          });
    return it != haystack.end();
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

ProcessGraphRag::ProcessGraphRag(RocksDBWrapper&       db,
                                 ProcessGraphManager&  engine,
                                 ProcessModelManager&  models,
                                 ProcessLinker&        linker)
    : db_(db), engine_(engine), models_(models), linker_(linker) {}

// ─────────────────────────────────────────────────────────────────────────────
// buildKnowledgeGraph
// ─────────────────────────────────────────────────────────────────────────────

ProcessGraphRag::ProcessKnowledgeGraph
ProcessGraphRag::buildKnowledgeGraph(std::string_view model_id) const {
    ProcessKnowledgeGraph kg;

    auto opt = models_.load(std::string(model_id));
    if (!opt.has_value()) {
        SPDLOG_WARN("[process_graph_rag] buildKnowledgeGraph: model '{}' not found — "
                    "verify model ID exists and has been imported", model_id);
        return kg;
    }

    const ProcessModelRecord& rec = *opt;
    const json& norm = rec.normalized;

    // ── Convert process nodes to KGNode objects ──────────────────────────
    if (norm.contains("nodes") && norm["nodes"].is_array()) {
        for (const auto& n : norm["nodes"]) {
            rag::kg::KGNode kgn;
            kgn.id             = n.value("id", "");
            kgn.canonical_name = n.value("name", kgn.id);
            kgn.type           = rag::kg::EntityType::CONCEPT;

            // Aliases: English name and subtype as extra surface forms
            if (n.contains("name_en") && n["name_en"].is_string()) {
                kgn.aliases.push_back(n["name_en"].get<std::string>());
            }
            if (n.contains("subtype") && n["subtype"].is_string()) {
                kgn.aliases.push_back(n["subtype"].get<std::string>());
            }

            // Properties
            kgn.properties["description"] = n.value("description", "");
            kgn.properties["node_type"]   = n.value("node_type", "");
            kgn.properties["model_id"]    = std::string(model_id);

            if (!kgn.id.empty()) {
                kg.nodes.push_back(std::move(kgn));
            }
        }
    }

    // ── Convert process edges to KGEdge objects ──────────────────────────
    if (norm.contains("edges") && norm["edges"].is_array()) {
        for (const auto& e : norm["edges"]) {
            rag::kg::KGEdge kge;
            kge.from_id  = e.value("from", "");
            kge.to_id    = e.value("to", "");
            kge.weight   = e.value("weight", 1.0);

            // Map edge type to RelationType
            std::string etype = e.value("edge_type", "SEQUENCE_FLOW");
            if (etype == "CONDITIONAL_FLOW" || etype == "MESSAGE_FLOW") {
                kge.relation = rag::kg::RelationType::CAUSES;
            } else if (etype == "ASSOCIATION" || etype == "DATA_ASSOCIATION") {
                kge.relation = rag::kg::RelationType::RELATED_TO;
            } else {
                kge.relation = rag::kg::RelationType::CAUSES; // SEQUENCE_FLOW → causes
            }

            if (!kge.from_id.empty() && !kge.to_id.empty()) {
                kg.edges.push_back(std::move(kge));
            }
        }
    }

    SPDLOG_INFO("[process_graph_rag] built KG for model '{}': {} nodes, {} edges",
                model_id, kg.nodes.size(), kg.edges.size());
    return kg;
}

// ─────────────────────────────────────────────────────────────────────────────
// buildInstanceKnowledgeGraph
// ─────────────────────────────────────────────────────────────────────────────

ProcessGraphRag::ProcessKnowledgeGraph
ProcessGraphRag::buildInstanceKnowledgeGraph(std::string_view        instance_id,
                                              const ProcessRagConfig& config) const {
    auto [status, inst] = engine_.getProcessInstance(instance_id);
    if (!status.ok) {
        SPDLOG_WARN("[process_graph_rag] buildInstanceKG: instance '{}' not found", instance_id);
        
        // Create diagnostic incident for retrieval failure
        DiagnosticContext ctx_diag;
        ctx_diag.recordResourceMetric("instance_id_length", instance_id.length());
        ctx_diag.setRemediationSuggestion(
            "Process instance '" + std::string(instance_id) + "' not found in storage. "
            "Verify instance ID exists and has been committed."
        );
        auto incident = ProcessDiagnostics::createRetrievalIncident(
            ProcError::kRetrievalFailed,
            instance_id,
            "Process instance not found: " + std::string(instance_id)
        );
        
        return {};
    }

    // Start from the model graph
    ProcessKnowledgeGraph kg = buildKnowledgeGraph(inst.process_definition_id);

    // ── Add instance-state node ──────────────────────────────────────────
    {
        rag::kg::KGNode inst_node;
        inst_node.id             = "inst:" + std::string(instance_id);
        inst_node.canonical_name = inst.name;
        inst_node.type           = rag::kg::EntityType::EVENT;
        inst_node.properties["state"]      = instanceStateStr(inst.state);
        inst_node.properties["model_id"]   = inst.process_definition_id;
        inst_node.properties["instance_id"]= std::string(instance_id);
        kg.nodes.push_back(std::move(inst_node));
    }

    // ── Add active token nodes and edges to their current process nodes ──
    for (const auto& token : inst.tokens) {
        rag::kg::KGNode tok_node;
        tok_node.id             = "tok:" + token.token_id;
        tok_node.canonical_name = "Token @ " + token.current_node;
        tok_node.type           = rag::kg::EntityType::EVENT;
        tok_node.properties["current_node"] = token.current_node;
        tok_node.properties["instance_id"]  = std::string(instance_id);
        kg.nodes.push_back(std::move(tok_node));

        rag::kg::KGEdge inst_edge;
        inst_edge.from_id  = "inst:" + std::string(instance_id);
        inst_edge.to_id    = "tok:" + token.token_id;
        inst_edge.relation = rag::kg::RelationType::HAS_PART;
        inst_edge.weight   = 1.0;
        kg.edges.push_back(std::move(inst_edge));

        rag::kg::KGEdge tok_edge;
        tok_edge.from_id  = "tok:" + token.token_id;
        tok_edge.to_id    = token.current_node;
        tok_edge.relation = rag::kg::RelationType::MENTIONS;
        tok_edge.weight   = 1.0;
        kg.edges.push_back(std::move(tok_edge));
    }

    // ── Add attachment nodes ─────────────────────────────────────────────
    if (config.include_attachments) {
        auto attachments = linker_.getAttachments(instance_id);
        for (const auto& att : attachments) {
            rag::kg::KGNode att_node;
            att_node.id             = "att:" + att.object_id;
            att_node.canonical_name = att.object_id;
            att_node.type           = rag::kg::EntityType::PRODUCT;
            att_node.properties["collection"] = att.object_collection;
            att_node.properties["link_type"]  = std::string(toString(att.link_type));
            if (att.metadata.contains("doc_type") && att.metadata["doc_type"].is_string()) {
                att_node.properties["doc_type"] = att.metadata["doc_type"].get<std::string>();
            }
            kg.nodes.push_back(std::move(att_node));

            rag::kg::KGEdge att_edge;
            att_edge.from_id  = "inst:" + std::string(instance_id);
            att_edge.to_id    = "att:" + att.object_id;
            att_edge.relation = rag::kg::RelationType::HAS_PART;
            att_edge.weight   = 0.8;
            kg.edges.push_back(std::move(att_edge));
        }
    }

    return kg;
}

// ─────────────────────────────────────────────────────────────────────────────
// extractSubgraph
// ─────────────────────────────────────────────────────────────────────────────

json ProcessGraphRag::extractSubgraph(std::string_view                model_id,
                                       const std::vector<std::string>& seed_node_ids,
                                       int                             max_depth) const {
    auto opt = models_.load(std::string(model_id));
    if (!opt.has_value()) {
        return {{"nodes", json::array()}, {"edges", json::array()}};
    }

    const json& norm = (*opt).normalized;
    if (!norm.contains("nodes") || !norm.contains("edges")) {
        return {{"nodes", json::array()}, {"edges", json::array()}};
    }

    // Build adjacency map: node_id → list of (neighbor_id, edge_json)
    // Use std::map for deterministic iteration order
    std::map<std::string, std::vector<std::pair<std::string, json>>> adj;
    for (const auto& e : norm["edges"]) {
        std::string from = e.value("from", "");
        std::string to   = e.value("to", "");
        if (!from.empty() && !to.empty()) {
            adj[from].push_back({to, e});
            adj[to].push_back({from, e}); // undirected BFS for context
        }
    }

    // BFS from seed nodes
    std::set<std::string> visited_nodes;
    std::set<std::string> visited_edges;
    std::queue<std::pair<std::string, int>> bfs_queue;

    for (const auto& seed : seed_node_ids) {
        bfs_queue.push({seed, 0});
        visited_nodes.insert(seed);
    }

    while (!bfs_queue.empty()) {
        auto [node_id, depth] = bfs_queue.front();
        bfs_queue.pop();
        if (depth >= max_depth) {
          continue;
        }
        for (const auto& [neighbor, edge_doc] : adj[node_id]) {
            std::string eid = edge_doc.value("edge_id", node_id + "->" + neighbor);
            if (!visited_nodes.count(neighbor)) {
                visited_nodes.insert(neighbor);
                bfs_queue.push({neighbor, depth + 1});
            }
            visited_edges.insert(eid);
        }
    }

    // Collect matching nodes and edges from the normalized graph
    json result_nodes = json::array();
    json result_edges = json::array();

    for (const auto& n : norm["nodes"]) {
        if (visited_nodes.count(n.value("id", ""))) {
            result_nodes.push_back(n);
        }
    }
    for (const auto& e : norm["edges"]) {
        std::string from = e.value("from", "");
        std::string to   = e.value("to", "");
        if (visited_nodes.count(from) && visited_nodes.count(to)) {
            result_edges.push_back(e);
        }
    }

    return {{"nodes", result_nodes}, {"edges", result_edges}};
}

// ─────────────────────────────────────────────────────────────────────────────
// computePpr
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::pair<std::string, float>> ProcessGraphRag::computePpr(
    const json&                    normalized_graph,
    const std::vector<std::string>& seed_node_ids,
    const PprConfig&               cfg) const
{
    if (seed_node_ids.empty() ||
        !normalized_graph.contains("nodes") ||
        !normalized_graph.contains("edges")) {
        return {};
    }

    // Build ordered node index
    std::vector<std::string> node_ids = {};

    for (const auto& n : normalized_graph["nodes"]) {
        std::string nid = n.value("id", "");
        if (!nid.empty()) {
          node_ids.push_back(nid);
        }
    }
    const int N = static_cast<int>(node_ids.size());
    if (N == 0) return {};

    std::unordered_map<std::string, int> node_index;
    node_index.reserve(N);
    for (int i = 0; i < N; ++i) {
      node_index[node_ids[i]] = i;
    }

    // Build column-stochastic transition matrix stored as sparse out-degree lists
    // transition[from] = [(to, weight)]  (uniform weight over out-edges)
    std::vector<std::vector<int>> out_neighbors(N);
    for (const auto& e : normalized_graph["edges"]) {
        std::string from = e.value("from", "");
        std::string to   = e.value("to", "");
        // Extract indices immediately to avoid iterator invalidation
        // Use direct value extraction to prevent holding stale iterators
        auto fi = node_index.find(from);
        if (fi == node_index.end()) {
          continue;
        }
        int from_idx = fi->second;  // Extract value immediately
         
        auto ti = node_index.find(to);
        if (ti == node_index.end()) {
          continue;
        }
        int to_idx = ti->second;    // Extract value immediately
         
        out_neighbors[from_idx].push_back(to_idx);
        // For undirected context propagation also add reverse edge
        out_neighbors[to_idx].push_back(from_idx);
    }

    // Build personalisation vector: uniform over seeds
    std::vector<float> personal(N, 0.f);
    for (const auto& seed : seed_node_ids) {
        auto it = node_index.find(seed);
        if (it != node_index.end()) {
            int seed_idx = it->second;
            personal[seed_idx] += 1.f;
        }
    }
    float psum = 0.f;
    for (float v : personal) {
      psum += v;
    }
    if (psum > 0.f) {
        for (float& v : personal) {
          v /= psum;
        }
    } else {
        // Fallback: uniform
        const float uni = 1.f / static_cast<float>(N);
        for (float& v : personal) {
          v = uni;
        }
    }

    // Power iteration: r = α * A^T * r + (1-α) * p
    std::vector<float> r(personal);   // initialise to personalisation vector
    std::vector<float> r_new(N, 0.f);

    for (int iter = 0; iter < cfg.max_iterations; ++iter) {
        // A^T * r:  for each destination j, sum contributions from sources i
        std::fill(r_new.begin(), r_new.end(), 0.f);
        for (int i = 0; i < N; ++i) {
            if (out_neighbors[i].empty()) {
                // Dangling node: redistribute uniformly
                const float contrib = r[i] / static_cast<float>(N);
                for (int j = 0; j < N; ++j) {
                  r_new[j] += contrib;
                }
            } else {
                const float contrib = r[i] / static_cast<float>(out_neighbors[i].size());
                for (int j : out_neighbors[i]) {
                  r_new[j] += contrib;
                }
            }
        }

        // r_new = α * A^T*r + (1-α) * personal
        float l1_diff = 0.f;
        for (int j = 0; j < N; ++j) {
            r_new[j] = cfg.damping * r_new[j] + (1.f - cfg.damping) * personal[j];
            l1_diff += std::abs(r_new[j] - r[j]);
        }
        r = r_new;
        if (l1_diff < cfg.convergence_epsilon) {
          break;
        }
    }

    // Collect top-k by PPR score
    std::vector<std::pair<std::string, float>> scored;
    scored.reserve(N);
    for (int i = 0; i < N; ++i) {
        scored.emplace_back(node_ids[i], r[i]);
    }
    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    const int k = std::min(cfg.top_k_nodes, N);
    scored.resize(k);
    return scored;
}

// ─────────────────────────────────────────────────────────────────────────────
// scoreNodeRelevance_
// ─────────────────────────────────────────────────────────────────────────────

float ProcessGraphRag::scoreNodeRelevance_(
    const json&                    node_doc,
    std::string_view               query,
    const std::vector<std::string>& active_nodes) const
{
    float score = 0.f;
    const std::string qstr(query);

    // Boost for active nodes
    std::string nid = node_doc.value("id", "");
    for (const auto& an : active_nodes) {
        if (an == nid) { score += 0.5f; break; }
    }

    // Keyword overlap with name and description
    std::string name = node_doc.value("name", "");
    std::string desc = node_doc.value("description", "");
    if (containsInsensitive(name, qstr)) {
      score += 0.3f;
    }
    if (containsInsensitive(desc, qstr)) {
      score += 0.2f;
    }

    return std::min(score, 1.f);
}

// ─────────────────────────────────────────────────────────────────────────────
// retrieve
// ─────────────────────────────────────────────────────────────────────────────

ProcessRagContext ProcessGraphRag::retrieve(std::string_view        instance_id,
                                             std::string_view        query,
                                             const ProcessRagConfig& config) const {
    ProcessRagContext ctx;
    ctx.instance_id = std::string(instance_id);
    ctx.query       = std::string(query);

    // 1. Load process instance
    auto [status, inst] = engine_.getProcessInstance(instance_id);
    if (!status.ok) {
        SPDLOG_WARN("[process_graph_rag] retrieve: instance '{}' not found", instance_id);
        
        // Create diagnostic incident for retrieval failure
        DiagnosticContext ctx_diag;
        ctx_diag.recordResourceMetric("instance_id_length", instance_id.size());
        ctx_diag.recordResourceMetric("query_length", query.length());
        ctx_diag.setRemediationSuggestion(
            "Requested process instance '" + std::string(instance_id) + "' not found in storage. "
            "Verify instance ID exists and has been committed to the database."
        );
        auto incident = ProcessDiagnostics::createRetrievalIncident(
            ProcError::kRetrievalFailed,
            instance_id,
            "Process instance not found: " + std::string(instance_id)
        );
        
        ctx.llm_prompt = "(Instanz nicht gefunden)";
        return ctx;
    }

    ctx.process_definition_id = inst.process_definition_id;
    ctx.process_name          = inst.name;
    ctx.current_state         = instanceStateStr(inst.state);

    // Active nodes from tokens
    for (const auto& tok : inst.tokens) {
        ctx.active_nodes.push_back(tok.current_node);
        if (config.include_history) {
            for (const auto& vn : tok.visited_nodes) {
                ctx.visited_nodes.push_back(vn);
            }
        }
    }

    // Deduplicate visited nodes
    std::sort(ctx.visited_nodes.begin(), ctx.visited_nodes.end());
    ctx.visited_nodes.erase(std::unique(ctx.visited_nodes.begin(), ctx.visited_nodes.end()),
                            ctx.visited_nodes.end());

    // 2. Load compliance tags from model
    if (config.include_compliance) {
        auto model_opt = models_.load(inst.process_definition_id);
        if (model_opt.has_value()) {
            ctx.compliance_tags = model_opt->compliance_tags;
        }
    }

    // 3. Extract subgraph around active nodes (BFS or PPR)
    if (config.use_ppr) {
        // PPR path: load the normalised graph and run Personalized PageRank
        auto model_opt = models_.load(inst.process_definition_id);
        if (model_opt.has_value() && model_opt->normalized.contains("nodes")) {
            PprConfig ppr_cfg;
            ppr_cfg.top_k_nodes = config.max_subgraph_depth * 10; // heuristic top-k
            auto ranked = computePpr(model_opt->normalized, ctx.active_nodes, ppr_cfg);

            // Build subgraph from top-k nodes
            std::set<std::string> top_set = {};

            for (const auto& [nid, score] : ranked) {
                top_set.insert(nid);
                ctx.node_scores[nid] = score;
            }

            json ppr_nodes = json::array();
            json ppr_edges = json::array();
            for (const auto& n : model_opt->normalized["nodes"]) {
                if (top_set.count(n.value("id", ""))) {
                  ppr_nodes.push_back(n);
                }
            }
            for (const auto& e : model_opt->normalized["edges"]) {
                if (top_set.count(e.value("from", "")) &&
                    top_set.count(e.value("to", ""))) {
                    ppr_edges.push_back(e);
                }
            }
            ctx.subgraph = {{"nodes", ppr_nodes}, {"edges", ppr_edges}};
        } else {
            ctx.subgraph = extractSubgraph(inst.process_definition_id,
                                           ctx.active_nodes,
                                           config.max_subgraph_depth);
        }
    } else {
        ctx.subgraph = extractSubgraph(inst.process_definition_id,
                                       ctx.active_nodes,
                                       config.max_subgraph_depth);
    }

    // Compute per-node relevance scores (only for BFS path; PPR scores already set)
    if (ctx.subgraph.contains("nodes") && ctx.subgraph["nodes"].is_array()) {
        for (const auto& n : ctx.subgraph["nodes"]) {
            std::string nid = n.value("id", "");
            if (!nid.empty() && ctx.node_scores.find(nid) == ctx.node_scores.end()) {
                ctx.node_scores[nid] = scoreNodeRelevance_(n, query, ctx.active_nodes);
            }
        }
    }
    if (!ctx.node_scores.empty()) {
        float total = 0.f;
        for (const auto& [nid, s] : ctx.node_scores) {
          total += s;
        }
        ctx.overall_relevance = total / static_cast<float>(ctx.node_scores.size());
    }

    // 4. Collect attachments
    if (config.include_attachments) {
        auto atts = linker_.getAttachments(instance_id);
        for (const auto& att : atts) {
            ctx.attachments.push_back(att.toDocument());
        }
    }

    // 5. Check missing documents for active nodes
    if (config.include_missing_docs) {
        for (const auto& node_id : ctx.active_nodes) {
            auto missing = linker_.getMissingDocuments(
                instance_id, node_id, inst.process_definition_id);
            for (auto& m : missing) {
                ctx.missing_documents.push_back(std::move(m));
            }
        }
        // Deduplicate
        std::sort(ctx.missing_documents.begin(), ctx.missing_documents.end());
        ctx.missing_documents.erase(
            std::unique(ctx.missing_documents.begin(), ctx.missing_documents.end()),
            ctx.missing_documents.end());
    }

    // 6. Find similar historical cases
    auto similar = findSimilarCases(instance_id, config.max_similar_cases,
                                    config.similarity_threshold);
    for (const auto& sc : similar) {
        json sc_doc;
        sc_doc["instance_id"]           = sc.instance_id;
        sc_doc["process_definition_id"] = sc.process_definition_id;
        sc_doc["name"]                  = sc.name;
        sc_doc["similarity"]            = sc.similarity;
        sc_doc["outcome"]               = sc.outcome;
        sc_doc["key_variables"]         = sc.key_variables;
        ctx.similar_cases.push_back(std::move(sc_doc));
    }

    // 7. Assemble LLM prompt
    ctx.llm_prompt = assemblePrompt_(ctx, config);

    return ctx;
}

// ─────────────────────────────────────────────────────────────────────────────
// retrieveForNode
// ─────────────────────────────────────────────────────────────────────────────

ProcessRagContext ProcessGraphRag::retrieveForNode(std::string_view        instance_id,
                                                    std::string_view        node_id,
                                                    std::string_view        query,
                                                    const ProcessRagConfig& config) const {
    // Build the base context, then narrow it to the specific node
    ProcessRagContext ctx = retrieve(instance_id, query, config);

    // Replace active_nodes with just the requested node
    ctx.active_nodes = {std::string(node_id)};

    // Re-extract subgraph around the specific node
    ctx.subgraph = extractSubgraph(ctx.process_definition_id,
                                   ctx.active_nodes,
                                   config.max_subgraph_depth);

    // Node attachments only
    if (config.include_attachments) {
        ctx.attachments.clear();
        auto node_atts = linker_.getNodeAttachments(instance_id, node_id);
        for (const auto& att : node_atts) {
            ctx.attachments.push_back(att.toDocument());
        }
    }

    // Missing documents for the specific node only
    if (config.include_missing_docs) {
        ctx.missing_documents = linker_.getMissingDocuments(
            instance_id, node_id, ctx.process_definition_id);
    }

    // Re-assemble prompt
    ctx.llm_prompt = assemblePrompt_(ctx, config);
    return ctx;
}

// ─────────────────────────────────────────────────────────────────────────────
// summarizeVerwaltungsvorgang
// ─────────────────────────────────────────────────────────────────────────────

json ProcessGraphRag::summarizeVerwaltungsvorgang(std::string_view instance_id) const {
    auto [status, inst] = engine_.getProcessInstance(instance_id);
    if (!status.ok) {
        return {{"error", "instance not found"}, {"instance_id", std::string(instance_id)}};
    }

    json summary;
    summary["instance_id"]   = std::string(instance_id);
    summary["process_name"]  = inst.name;
    summary["state"]         = instanceStateStr(inst.state);
    summary["variables"]     = inst.variables;

    // Current tasks
    json current_tasks = json::array();
    for (const auto& tok : inst.tokens) {
        current_tasks.push_back(tok.current_node);
    }
    summary["current_tasks"] = current_tasks;

    // Progress: count of unique visited nodes relative to model total
    std::set<std::string> all_visited = {};

    for (const auto& tok : inst.tokens) {
        for (const auto& vn : tok.visited_nodes) {
          all_visited.insert(vn);
        }
    }

    auto model_opt = models_.load(inst.process_definition_id);
    size_t total_nodes = 1;
    std::vector<std::string> compliance_tags = {};

    if (model_opt.has_value()) {
        const json& norm = model_opt->normalized;
        if (norm.contains("nodes") && norm["nodes"].is_array()) {
            total_nodes = norm["nodes"].size();
        }
        compliance_tags = model_opt->compliance_tags;
    }
    summary["progress_pct"] = total_nodes > 0
        ? std::round(static_cast<float>(all_visited.size()) /
                     static_cast<float>(total_nodes) * 1000.f) / 10.f
        : 0.0;

    // Missing documents for all active nodes
    json missing = json::array();
    for (const auto& tok : inst.tokens) {
        auto ms = linker_.getMissingDocuments(
            instance_id, tok.current_node, inst.process_definition_id);
        for (auto& m : ms) {
          missing.push_back(m);
        }
    }
    summary["missing_documents"] = missing;

    // Compliance
    auto comp = checkCompliance(instance_id);
    summary["compliance_status"] = comp.is_compliant ? "ok" : "violation";
    summary["compliance_score"]  = comp.compliance_score;
    if (!comp.violations.empty()) {
        json viol = json::array();
        for (const auto& v : comp.violations) {
          viol.push_back(v);
        }
        summary["violations"] = viol;
    }

    // SLA status: compare started_at vs model timeout (if any)
    summary["sla_status"] = "on_time"; // default
    if (model_opt.has_value()) {
        const json& norm = model_opt->normalized;
        if (norm.contains("metadata") && norm["metadata"].contains("sla_ms")) {
            int64_t sla_ms = norm["metadata"]["sla_ms"].get<int64_t>();
            int64_t elapsed = nowMs() - inst.started_at_ms;
            if (elapsed > sla_ms) {
                summary["sla_status"] = "overdue";
            } else if (elapsed > static_cast<int64_t>(sla_ms * 0.8)) {
                summary["sla_status"] = "at_risk";
            }
        }
    }

    // Attachment count
    auto atts = linker_.getAttachments(instance_id);
    summary["attachments_count"] = static_cast<int>(atts.size());

    // Compliance tags
    json ctags = json::array();
    for (const auto& t : compliance_tags) {
      ctags.push_back(t);
    }
    summary["compliance_tags"] = ctags;

    return summary;
}

// ─────────────────────────────────────────────────────────────────────────────
// checkCompliance
// ─────────────────────────────────────────────────────────────────────────────

ProcessGraphRag::ComplianceCheckResult
ProcessGraphRag::checkCompliance(std::string_view instance_id) const {
    ComplianceCheckResult result;
    result.is_compliant   = true;
    result.compliance_score = 1.0f;

    auto [status, inst] = engine_.getProcessInstance(instance_id);
    if (!status.ok) {
        result.is_compliant   = false;
        result.compliance_score = 0.f;
        result.violations.push_back("Instance not found: " + std::string(instance_id));
        return result;
    }

    int checks = 0, passed = 0;

    // Check 1: Required documents present for all active nodes
    for (const auto& tok : inst.tokens) {
        ++checks;
        auto missing = linker_.getMissingDocuments(
            instance_id, tok.current_node, inst.process_definition_id);
        if (missing.empty()) {
            ++passed;
        } else {
            result.is_compliant = false;
            for (const auto& m : missing) {
                result.violations.push_back(
                    "Fehlende Pflichtunterlage an Knoten '" + tok.current_node +
                    "': " + m);
            }
        }
    }

    // Check 2: SLA not exceeded
    ++checks;
    auto model_opt = models_.load(inst.process_definition_id);
    bool sla_ok = true;
    if (model_opt.has_value()) {
        const json& norm = model_opt->normalized;
        if (norm.contains("metadata") && norm["metadata"].contains("sla_ms")) {
            int64_t sla_ms = norm["metadata"]["sla_ms"].get<int64_t>();
            int64_t elapsed = nowMs() - inst.started_at_ms;
            if (elapsed > sla_ms) {
                sla_ok = false;
                result.is_compliant = false;
                result.violations.push_back(
                    "SLA überschritten: Vorgang läuft seit " +
                    std::to_string(elapsed / 1000) + "s, SLA: " +
                    std::to_string(sla_ms / 1000) + "s");
            } else if (elapsed > static_cast<int64_t>(sla_ms * 0.8)) {
                result.warnings.push_back("SLA-Warnung: 80% der SLA-Frist verbraucht");
            }
        }
    }
    if (sla_ok) {
      ++passed;
    }

    // Check 3: Process instance not stuck in FAILED state
    ++checks;
    if (inst.state == ProcessInstance::State::FAILED) {
        result.is_compliant = false;
        result.violations.push_back("Prozessinstanz befindet sich im FAILED-Zustand");
    } else {
        ++passed;
    }

    // Check 4: BPMN-S DSGVO annotation validation
    // Reads dsgvo_annotation persisted in node metadata during BPMN-S import.
    if (model_opt.has_value()) {
        const json& norm = model_opt->normalized;
        if (norm.contains("nodes") && norm["nodes"].is_array()) {
            for (const auto& jn : norm["nodes"]) {
                if (!jn.contains("metadata")) {
                  continue;
                }
                const auto& meta = jn["metadata"];
                if (!meta.contains("dsgvo_annotation")) {
                  continue;
                }
                const auto& ann = meta["dsgvo_annotation"];
                std::string node_name = jn.value("name", jn.value("id", "unknown"));
                std::string cat       = ann.value("data_category", "");
                std::string basis     = ann.value("legal_basis", "");
                bool        consent   = ann.value("requires_consent", false);

                ++checks;
                bool node_ok = true;
                if ((cat == "personal" || cat == "sensitive") && basis.empty()) {
                    node_ok = false;
                    result.is_compliant = false;
                    result.violations.push_back(
                        "Node '" + node_name +
                        "' handles personal data but has no DSGVO legal basis");
                }
                if (consent && basis.find("Art. 6(1)(a)") == std::string::npos) {
                    node_ok = false;
                    result.is_compliant = false;
                    result.violations.push_back(
                        "Node '" + node_name +
                        "' requires consent but legal_basis does not cite Art. 6(1)(a)");
                }
                if (node_ok) {
                  ++passed;
                }
            }
        }
    }

    // Compute score
    result.compliance_score = checks > 0
        ? static_cast<float>(passed) / static_cast<float>(checks)
        : 1.f;

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// findSimilarCases
// ─────────────────────────────────────────────────────────────────────────────

std::vector<ProcessGraphRag::SimilarCase>
ProcessGraphRag::findSimilarCases(std::string_view instance_id,
                                   int              k,
                                   float            min_similarity) const {
    // Load reference instance
    auto [status, ref_inst] = engine_.getProcessInstance(instance_id);
    if (!status.ok) {
        SPDLOG_WARN("[process_graph_rag] findSimilarCases: reference instance '{}' not found",
                    instance_id);
        
        // Create diagnostic incident for retrieval failure
        DiagnosticContext ctx_diag;
        ctx_diag.recordResourceMetric("instance_id_length", instance_id.length());
        ctx_diag.recordResourceMetric("k", k);
        ctx_diag.setRemediationSuggestion(
            "Reference process instance '" + std::string(instance_id) + "' not found in storage. "
            "Cannot find similar cases without a valid reference instance."
        );
        auto incident = ProcessDiagnostics::createRetrievalIncident(
            ProcError::kRetrievalFailed,
            instance_id,
            "Reference instance not found for similarity search: " + std::string(instance_id)
        );
        
        return {};
    }

    // Try to load reference embedding
    std::string ref_emb_key = "proc:inst_emb:" + std::string(instance_id);
    std::string ref_emb_val = {};
    std::vector<float> ref_embedding = {};

    if (db_.get(ref_emb_key, ref_emb_val)) {
        try {
            auto emb_json = json::parse(ref_emb_val);
            if (emb_json.is_array()) {
                ref_embedding.reserve(emb_json.size());
                for (const auto& v : emb_json) {
                    ref_embedding.push_back(v.get<float>());
                }
            }
        } catch (...) {
            ref_embedding.clear();
        }
    }

    // Build reference variable key set for Jaccard fallback
    std::set<std::string> ref_var_keys = {};

    if (ref_inst.variables.is_object()) {
        for (auto& [var_key, v] : ref_inst.variables.items()) {
            ref_var_keys.insert(var_key);
        }
    }

    // Scan all stored instance embeddings
    std::vector<SimilarCase> candidates;
    const std::string emb_prefix = "proc:inst_emb:";

    db_.scanPrefix(emb_prefix, [&](std::string_view key, std::string_view value) -> bool {
        // Extract instance id from key: "proc:inst_emb:<id>"
        std::string scan_iid = std::string(key).substr(emb_prefix.size());
        if (scan_iid == std::string(instance_id)) return true; // skip self

        float sim = 0.f;
        if (!ref_embedding.empty()) {
            try {
                auto emb_json = json::parse(value);
                if (emb_json.is_array()) {
                    std::vector<float> other_emb = {};

                    other_emb.reserve(emb_json.size());
                    for (const auto& v : emb_json) {
                      other_emb.push_back(v.get<float>());
                    }
                    sim = cosineSimilarity(ref_embedding, other_emb);
                }
            } catch (...) {}
        }

        if (sim >= min_similarity) {
            SimilarCase sc;
            sc.instance_id = scan_iid;
            sc.similarity  = sim;
            // Load instance metadata
            auto [s2, other_inst] = engine_.getProcessInstance(scan_iid);
            if (s2.ok) {
                sc.process_definition_id = other_inst.process_definition_id;
                sc.name    = other_inst.name;
                sc.outcome = instanceStateStr(other_inst.state);
                // Store top-5 variables as key context
                int cnt = 0;
                for (auto& [vk, vv] : other_inst.variables.items()) {
                    if (cnt++ >= 5) {
                      break;
                    }
                    sc.key_variables[vk] = vv;
                }
            }
            candidates.push_back(std::move(sc));
        }
        return true;
    });

    // Fallback: if no embedding-based candidates, use variable Jaccard
    if (candidates.empty()) {
        const std::string inst_prefix = "proc:inst:";
        db_.scanPrefix(inst_prefix, [&](std::string_view key, std::string_view value) -> bool {
            std::string scan_iid = std::string(key).substr(inst_prefix.size());
            if (scan_iid == std::string(instance_id)) {
              return true;
            }

            try {
                auto doc = json::parse(value);
                if (doc.value("deleted", false)) {
                  return true;
                }

                // Only compare instances from the same process definition
                if (doc.value("process_definition_id", "") != ref_inst.process_definition_id) {
                    return true;
                }

                std::set<std::string> other_var_keys = {};

                if (doc.contains("variables") && doc["variables"].is_object()) {
                    for (auto& [vk, vv] : doc["variables"].items()) {
                        other_var_keys.insert(vk);
                    }
                }

                float sim = jaccardSimilarity(ref_var_keys, other_var_keys);
                if (sim >= min_similarity) {
                    SimilarCase sc;
                    sc.instance_id           = scan_iid;
                    sc.process_definition_id = doc.value("process_definition_id", "");
                    sc.name                  = doc.value("name", scan_iid);
                    sc.outcome               = doc.value("state", "UNKNOWN");
                    sc.similarity            = sim;
                    int cnt = 0;
                    if (doc.contains("variables") && doc["variables"].is_object()) {
                        for (auto& [vk, vv] : doc["variables"].items()) {
                            if (cnt++ >= 5) {
                              break;
                            }
                            sc.key_variables[vk] = vv;
                        }
                    }
                    candidates.push_back(std::move(sc));
                }
            } catch (...) {}
            return true;
        });
    }

    // Sort by similarity descending and return top k
    std::sort(candidates.begin(), candidates.end(),
              [](const SimilarCase& a, const SimilarCase& b) {
                  return a.similarity > b.similarity;
              });
    if (static_cast<int>(candidates.size()) > k) {
        candidates.resize(static_cast<size_t>(k));
    }

    return candidates;
}

// ─────────────────────────────────────────────────────────────────────────────
// assemblePrompt_
// ─────────────────────────────────────────────────────────────────────────────

std::string ProcessGraphRag::assemblePrompt_(const ProcessRagContext& ctx,
                                              const ProcessRagConfig&  config) const {
    std::ostringstream ss = {};
    const bool de = (config.language != "en");

    if (de) {
        ss << "=== Verwaltungsvorgang: " << ctx.process_name
           << " (ID: " << ctx.instance_id << ") ===\n";
        ss << "Status: " << ctx.current_state << "\n";
        ss << "Prozessmodell: " << ctx.process_definition_id << "\n";

        ss << "Aktuelle Aufgaben: ";
        if (ctx.active_nodes.empty()) {
            ss << "(keine)";
        } else {
            for (size_t i = 0; i < ctx.active_nodes.size(); ++i) {
                if (i > 0) {
                  ss << ", ";
                }
                ss << ctx.active_nodes[i];
            }
        }
        ss << "\n";

        if (!ctx.visited_nodes.empty()) {
            ss << "Verlauf: ";
            for (size_t i = 0; i < ctx.visited_nodes.size(); ++i) {
                if (i > 0) {
                  ss << " → ";
                }
                ss << ctx.visited_nodes[i];
            }
            ss << "\n";
        }

        ss << "\nAngehängte Dokumente: ";
        if (ctx.attachments.empty()) {
            ss << "(keine)";
        } else {
            for (const auto& att : ctx.attachments) {
                ss << "\n  - " << att.value("object_id", "?")
                   << " [" << att.value("link_type", "?") << "]";
                if (att.contains("metadata") &&
                    att["metadata"].contains("doc_type")) {
                    ss << " (" << att["metadata"]["doc_type"].get<std::string>() << ")";
                }
            }
        }
        ss << "\n";

        ss << "\nFehlende Unterlagen: ";
        if (ctx.missing_documents.empty()) {
            ss << "(keine)";
        } else {
            for (const auto& m : ctx.missing_documents) {
                ss << "\n  - " << m;
            }
        }
        ss << "\n";

        if (!ctx.compliance_tags.empty()) {
            ss << "\nCompliance: ";
            for (size_t i = 0; i < ctx.compliance_tags.size(); ++i) {
                if (i > 0) {
                  ss << ", ";
                }
                ss << ctx.compliance_tags[i];
            }
            ss << "\n";
        }

        ss << "\nRelevante Prozessschritte (Teilgraph):\n";
        if (ctx.subgraph.contains("nodes") && !ctx.subgraph["nodes"].empty()) {
            for (const auto& n : ctx.subgraph["nodes"]) {
                std::string nid = n.value("id", "?");
                std::string nm  = n.value("name", nid);
                float score = 0.f;
                auto it = ctx.node_scores.find(nid);
                if (it != ctx.node_scores.end()) {
                  score = it->second;
                }
                ss << "  [" << nm << "] (" << nid << ")";
                if (score > 0.f) {
                  ss << " relevanz=" << static_cast<int>(score * 100) << "%";
                }
                std::string desc = n.value("description", "");
                if (!desc.empty()) {
                  ss << " – " << desc;
                }
                ss << "\n";
            }
        } else {
            ss << "  (kein Teilgraph verfügbar)\n";
        }

        if (!ctx.similar_cases.empty()) {
            ss << "\nÄhnliche abgeschlossene Vorgänge:\n";
            for (const auto& sc : ctx.similar_cases) {
                ss << "  - " << sc.value("name", "?")
                   << " [" << sc.value("outcome", "?") << "]"
                   << " (Ähnlichkeit: "
                   << static_cast<int>(sc.value("similarity", 0.f) * 100) << "%)\n";
            }
        }

        if (!ctx.query.empty()) {
            ss << "\nAnfrage: " << ctx.query << "\n";
        }
    } else {
        // English prompt
        ss << "=== Administrative Case: " << ctx.process_name
           << " (ID: " << ctx.instance_id << ") ===\n";
        ss << "Status: " << ctx.current_state << "\n";
        ss << "Process model: " << ctx.process_definition_id << "\n";

        ss << "Current tasks: ";
        if (ctx.active_nodes.empty()) {
            ss << "(none)";
        } else {
            for (size_t i = 0; i < ctx.active_nodes.size(); ++i) {
                if (i > 0) {
                  ss << ", ";
                }
                ss << ctx.active_nodes[i];
            }
        }
        ss << "\n";

        ss << "\nAttached documents: ";
        if (ctx.attachments.empty()) {
            ss << "(none)";
        } else {
            for (const auto& att : ctx.attachments) {
                ss << "\n  - " << att.value("object_id", "?")
                   << " [" << att.value("link_type", "?") << "]";
            }
        }
        ss << "\n";

        ss << "\nMissing documents: ";
        if (ctx.missing_documents.empty()) {
            ss << "(none)";
        } else {
            for (const auto& m : ctx.missing_documents) {
              ss << "\n  - " << m;
            }
        }
        ss << "\n";

        if (!ctx.compliance_tags.empty()) {
            ss << "\nCompliance: ";
            for (size_t i = 0; i < ctx.compliance_tags.size(); ++i) {
                if (i > 0) {
                  ss << ", ";
                }
                ss << ctx.compliance_tags[i];
            }
            ss << "\n";
        }

        ss << "\nRelevant process steps (subgraph):\n";
        if (ctx.subgraph.contains("nodes") && !ctx.subgraph["nodes"].empty()) {
            for (const auto& n : ctx.subgraph["nodes"]) {
                ss << "  [" << n.value("name", n.value("id", "?")) << "]\n";
            }
        }

        if (!ctx.similar_cases.empty()) {
            ss << "\nSimilar past cases:\n";
            for (const auto& sc : ctx.similar_cases) {
                ss << "  - " << sc.value("name", "?")
                   << " [" << sc.value("outcome", "?") << "]"
                   << " (similarity: "
                   << static_cast<int>(sc.value("similarity", 0.f) * 100) << "%)\n";
            }
        }

        if (!ctx.query.empty()) {
            ss << "\nQuery: " << ctx.query << "\n";
        }
    }

    return ss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// buildAdminProcessingPrompt / buildQueryPrompt
// ─────────────────────────────────────────────────────────────────────────────

std::string ProcessGraphRag::buildAdminProcessingPrompt(const ProcessRagContext& ctx) const {
    ProcessRagConfig cfg;
    cfg.language = "de";
    return assemblePrompt_(ctx, cfg);
}

std::string ProcessGraphRag::buildQueryPrompt(const ProcessRagContext& ctx) const {
    std::ostringstream ss = {};
    ss << ctx.llm_prompt;
    if (!ctx.query.empty()) {
        ss << "\n---\nFrage: " << ctx.query << "\n";
    }
    return ss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// SLA Monitoring (Q4 2026)
// ─────────────────────────────────────────────────────────────────────────────

void ProcessGraphRag::registerSlaRule(
    std::string_view                  instance_id,
    int64_t                           sla_ms,
    std::string_view                  process_name,
    themisdb::analytics::CEPEngine&   cep,
    SlaAlertCallback                  on_alert)
{
    if (!cep.isInitialized() || sla_ms <= 0) {
      return;
    }

    const std::string inst_id{instance_id};
    const std::string proc_name{process_name};
    const int64_t at_risk_ms = static_cast<int64_t>(static_cast<double>(sla_ms) * 0.8);

    const std::string at_risk_id = "sla_at_risk_" + inst_id;
    const std::string overdue_id = "sla_overdue_" + inst_id;
    const std::string stream_id  = "_sla_" + inst_id;

    // Create the instance-scoped stream (CEPEngine ignores duplicate calls)
    themisdb::analytics::StreamConfig sc;
    sc.stream_id   = stream_id;
    sc.buffer_size = 64;
    sc.partitions  = 1;
    cep.createStream(sc);

    // At-risk rule: window expires after 80% of sla_ms
    {
        themisdb::analytics::RuleConfig rc;
        rc.rule_id   = at_risk_id;
        rc.rule_name = "SLA at-risk: " + inst_id;
        rc.streams   = {stream_id};
        rc.tags["instance_id"]  = inst_id;
        rc.tags["process_name"] = proc_name;
        rc.tags["sla_ms"]       = std::to_string(sla_ms);
        rc.tags["alert_type"]   = "at_risk";

        themisdb::analytics::ActionConfig ac;
        ac.type         = themisdb::analytics::ActionType::ALERT;
        ac.retry_count  = 3;
        ac.retry_delay  = std::chrono::milliseconds{1000};
        ac.template_str = "SLA at-risk for instance " + inst_id;
        rc.actions.push_back(std::move(ac));

        themisdb::analytics::WindowConfig wc;
        wc.type = themisdb::analytics::WindowType::SLIDING;
        wc.size = std::chrono::milliseconds{at_risk_ms};
        rc.window = wc;

        cep.addRule(rc);
    }

    // Overdue rule: window expires after sla_ms
    {
        themisdb::analytics::RuleConfig rc;
        rc.rule_id   = overdue_id;
        rc.rule_name = "SLA overdue: " + inst_id;
        rc.streams   = {stream_id};
        rc.tags["instance_id"]  = inst_id;
        rc.tags["process_name"] = proc_name;
        rc.tags["sla_ms"]       = std::to_string(sla_ms);
        rc.tags["alert_type"]   = "overdue";

        themisdb::analytics::ActionConfig ac;
        ac.type         = themisdb::analytics::ActionType::ALERT;
        ac.retry_count  = 3;
        ac.retry_delay  = std::chrono::milliseconds{1000};
        ac.template_str = "SLA OVERDUE for instance " + inst_id;
        rc.actions.push_back(std::move(ac));

        themisdb::analytics::WindowConfig wc;
        wc.type = themisdb::analytics::WindowType::SLIDING;
        wc.size = std::chrono::milliseconds{sla_ms};
        rc.window = wc;

        cep.addRule(rc);
    }

    {
        std::lock_guard<std::mutex> lock(sla_rules_mutex_);
        sla_rules_[inst_id] = SlaRuleEntry{at_risk_id, overdue_id, std::move(on_alert)};
    }

    SPDLOG_DEBUG("ProcessGraphRag: registered SLA rules for instance '{}' sla={}ms",
                 inst_id, sla_ms);
}

void ProcessGraphRag::deregisterSlaRule(
    std::string_view                instance_id,
    themisdb::analytics::CEPEngine& cep)
{
    const std::string inst_id{instance_id};
    SlaRuleEntry entry;
    {
        std::lock_guard<std::mutex> lock(sla_rules_mutex_);
        auto it = sla_rules_.find(inst_id);
        if (it == sla_rules_.end()) {
          return;
        }
        entry = std::move(it->second);
        sla_rules_.erase(it);
    }
    if (cep.isInitialized()) {
        cep.removeRule(entry.at_risk_rule_id);
        cep.removeRule(entry.overdue_rule_id);
        cep.removeStream("_sla_" + inst_id);
    }
    SPDLOG_DEBUG("ProcessGraphRag: deregistered SLA rules for instance '{}'", inst_id);
}

void ProcessGraphRag::fireSlaAlert_(
    const std::string& instance_id,
    const std::string& process_name,
    int64_t            sla_ms,
    int64_t            elapsed_ms,
    const std::string& status)
{
    SlaAlert alert{instance_id, process_name, sla_ms, elapsed_ms, status};
    std::lock_guard<std::mutex> lock(sla_rules_mutex_);
    auto it = sla_rules_.find(instance_id);
    if ([[maybe_unused]] it != sla_rules_.end() && it->second.callback) {
        try {
            it->second.callback([[maybe_unused]] alert);
        } catch (const std::exception& ex) {
            SPDLOG_WARN("ProcessGraphRag: SLA alert callback threw: {}", ex.what());
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Cross-Case Bottleneck Analytics (Q4 2026)
// ─────────────────────────────────────────────────────────────────────────────

void ProcessGraphRag::recordNodeCompletion(
    std::string_view model_id,
    std::string_view node_id,
    std::string_view node_name,
    int64_t          dwell_ms)
{
    const std::string key = "proc:dwell:" + std::string(model_id)
                          + ":" + std::string(node_id);

    // Load existing aggregate
    json agg = json::object();
    {
        std::string val = {};
        if (db_.get(key, val) && !val.empty()) {
            try { agg = json::parse(val); }
            catch (...) { agg = json::object(); }
        }
    }

    int64_t count   = agg.value("count", int64_t{0});
    double  sum_ms  = agg.value("sum_ms", 0.0);
    double  sum_sq  = agg.value("sum_sq_ms", 0.0);
    auto    samples = agg.value("samples", std::vector<double>{});

    count++;
    sum_ms += static_cast<double>(dwell_ms);
    sum_sq += static_cast<double>(dwell_ms) * static_cast<double>(dwell_ms);
    samples.push_back(static_cast<double>(dwell_ms));

    constexpr size_t kMaxSamples = 200;
    if (static_cast<int>(samples.size()) > kMaxSamples)
        samples.erase(samples.begin());

    agg["node_name"]  = std::string(node_name);
    agg["count"]      = count;
    agg["sum_ms"]     = sum_ms;
    agg["sum_sq_ms"]  = sum_sq;
    agg["samples"]    = samples;

    db_.put(key, agg.dump());
}

std::vector<ProcessGraphRag::NodeDwellStats> ProcessGraphRag::analyzeBottlenecks(
    std::string_view model_id,
    int              top_n) const
{
    const std::string prefix = "proc:dwell:" + std::string(model_id) + ":";
    std::vector<NodeDwellStats> result;

    db_.scanPrefix(prefix,
        [&](std::string_view key, std::string_view value) -> bool {
            // Extract node_id from key suffix
            std::string node_id = std::string(key).substr(prefix.size());

            json agg;
            try { agg = json::parse(value); }
            catch (...) { return true; }

            int64_t count  = agg.value("count", int64_t{0});
            if (count <= 0) {
              return true;
            }

            double sum_ms = agg.value("sum_ms", 0.0);
            auto   samples = agg.value("samples", std::vector<double>{});

            double avg = sum_ms / static_cast<double>(count);
            double p95 = avg;
            if (!samples.empty()) {
                std::vector<double> sorted_samples = samples;
                std::sort(sorted_samples.begin(), sorted_samples.end());
                size_t idx = static_cast<size_t>(
                    static_cast<double>(static_cast<int>(sorted_samples.size()) - 1) * 0.95);
                p95 = sorted_samples[idx];
            }

            NodeDwellStats stats;
            stats.node_id      = std::move(node_id);
            stats.node_name    = agg.value("node_name", stats.node_id);
            stats.avg_dwell_ms = avg;
            stats.p95_dwell_ms = p95;
            stats.sample_count = static_cast<size_t>(count);
            result.push_back(std::move(stats));
            return true;
        });

    std::sort(result.begin(), result.end(),
              [](const NodeDwellStats& a, const NodeDwellStats& b) {
                  return a.avg_dwell_ms > b.avg_dwell_ms;
              });

    if (static_cast<int>(result.size()) > top_n)
        result.resize(static_cast<size_t>(top_n));

    return result;
}

} // namespace process
} // namespace themis


