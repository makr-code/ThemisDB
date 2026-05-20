// THEMIS_GAP_STATS: gaps=2 unimpl=1 stub=0 mock=0 sim=0 todo=0 debt=0 scanned=2026-05-18
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            knowledge_graph_retriever.cpp                      ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:50:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     567                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file knowledge_graph_retriever.cpp
 * @brief Implementation of knowledge graph-augmented retrieval with entity linking.
 */

#include "rag/knowledge_graph_retriever.h"
#include "utils/logger.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <mutex>
#include <queue>
#include <sstream>
#include <unordered_map>

namespace themis::rag::kg {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Case-fold (ASCII) and collapse runs of whitespace to a single space.
std::string normaliseText(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    bool last_was_space = true;
    for (unsigned char ch : s) {
        if (std::isspace(ch)) {
            if (!last_was_space) {
                out += ' ';
                last_was_space = true;
            }
        } else {
            out += static_cast<char>(std::tolower(ch));
            last_was_space = false;
        }
    }
    // Trim trailing space
    if (!out.empty() && out.back() == ' ') {
        out.pop_back();
    }
    return out;
}

/// Compute normalised string similarity: exact = 1.0; prefix scaled by
/// length ratio; otherwise 0.0.  Both inputs should already be normalised.
double stringSimilarity(const std::string& a, const std::string& b) {
    if (a.empty() && b.empty()) return 1.0;
    if (a.empty() || b.empty()) return 0.0;
    if (a == b) return 1.0;

    // Prefix match: shorter is prefix of longer → score = |shorter|/|longer|
    const std::string& shorter = a.size() <= b.size() ? a : b;
    const std::string& longer  = a.size() <= b.size() ? b : a;
    if (longer.substr(0, shorter.size()) == shorter) {
        return static_cast<double>(shorter.size()) /
               static_cast<double>(longer.size());
    }
    return 0.0;
}

/// Normalised Jaccard similarity between two sets of strings.
double jaccardSets(const std::unordered_set<std::string>& A,
                   const std::unordered_set<std::string>& B) {
    if (A.empty() && B.empty()) return 1.0;
    if (A.empty() || B.empty()) return 0.0;

    size_t intersection = 0;
    for (const auto& elem : A) {
        if (B.count(elem)) ++intersection;
    }
    const size_t unionSize = A.size() + B.size() - intersection;
    return unionSize == 0 ? 0.0
                          : static_cast<double>(intersection) /
                                static_cast<double>(unionSize);
}

} // anonymous namespace

// =============================================================================
// KnowledgeGraph::Impl
// =============================================================================

struct KnowledgeGraph::Impl {
    mutable std::mutex                          mtx;
    std::unordered_map<std::string, KGNode>     nodes;  // id → node
    // Adjacency list: from_id → list of edges
    std::unordered_map<std::string, std::vector<KGEdge>> adj;
    size_t edge_count = 0;
};

// =============================================================================
// KnowledgeGraph
// =============================================================================

KnowledgeGraph::KnowledgeGraph()  : impl_(std::make_unique<Impl>()) {}
KnowledgeGraph::~KnowledgeGraph() = default;
KnowledgeGraph::KnowledgeGraph(KnowledgeGraph&&) = default;
KnowledgeGraph& KnowledgeGraph::operator=(KnowledgeGraph&&) = default;

void KnowledgeGraph::addNode(KGNode node) {
    std::lock_guard<std::mutex> lk(impl_->mtx);
    THEMIS_DEBUG("KnowledgeGraph::addNode id='{}' name='{}'",
                 node.id, node.canonical_name);
    impl_->nodes[node.id] = std::move(node);
}

bool KnowledgeGraph::removeNode(const std::string& node_id) {
    std::lock_guard<std::mutex> lk(impl_->mtx);
    if (!impl_->nodes.count(node_id)) return false;

    impl_->nodes.erase(node_id);

    // Decrement edge_count for all outgoing edges from this node before erasing
    auto adj_it = impl_->adj.find(node_id);
    if (adj_it != impl_->adj.end()) {
        impl_->edge_count -= adj_it->second.size();
        impl_->adj.erase(adj_it);
    }

    // Remove edges that point to this node
    for (auto& [src, edges] : impl_->adj) {
        const size_t before = edges.size();
        edges.erase(std::remove_if(edges.begin(), edges.end(),
                        [&](const KGEdge& e) { return e.to_id == node_id; }),
                    edges.end());
        impl_->edge_count -= (before - edges.size());
    }
    return true;
}

const KGNode* KnowledgeGraph::findNode(const std::string& node_id) const {
    std::lock_guard<std::mutex> lk(impl_->mtx);
    auto it = impl_->nodes.find(node_id);
    return it != impl_->nodes.end() ? &it->second : nullptr;
}

const KGNode* KnowledgeGraph::findNodeByName(const std::string& text) const {
    std::lock_guard<std::mutex> lk(impl_->mtx);
    const std::string norm = normaliseText(text);

    const KGNode* best     = nullptr;
    double        best_sim = 0.0;

    for (const auto& [id, node] : impl_->nodes) {
        // Check canonical name
        double sim = stringSimilarity(norm, normaliseText(node.canonical_name));
        if (sim > best_sim) { best_sim = sim; best = &node; }

        // Check aliases
        for (const auto& alias : node.aliases) {
            sim = stringSimilarity(norm, normaliseText(alias));
            if (sim > best_sim) { best_sim = sim; best = &node; }
        }
    }

    // Only return if similarity exceeds a minimal threshold (0.5)
    return (best_sim >= 0.5) ? best : nullptr;
}

size_t KnowledgeGraph::nodeCount() const {
    std::lock_guard<std::mutex> lk(impl_->mtx);
    return impl_->nodes.size();
}

void KnowledgeGraph::addEdge(KGEdge edge) {
    std::lock_guard<std::mutex> lk(impl_->mtx);
    THEMIS_DEBUG("KnowledgeGraph::addEdge '{}' → '{}'",
                 edge.from_id, edge.to_id);
    impl_->adj[edge.from_id].push_back(std::move(edge));
    ++impl_->edge_count;
}

bool KnowledgeGraph::removeEdge(const std::string& from_id,
                                 const std::string& to_id,
                                 RelationType       relation) {
    std::lock_guard<std::mutex> lk(impl_->mtx);
    auto it = impl_->adj.find(from_id);
    if (it == impl_->adj.end()) return false;

    auto& edges = it->second;
    const size_t before = edges.size();
    edges.erase(std::remove_if(edges.begin(), edges.end(),
                    [&](const KGEdge& e) {
                        return e.to_id == to_id && e.relation == relation;
                    }),
                edges.end());
    const size_t removed = before - edges.size();
    impl_->edge_count -= removed;
    return removed > 0;
}

size_t KnowledgeGraph::edgeCount() const {
    std::lock_guard<std::mutex> lk(impl_->mtx);
    return impl_->edge_count;
}

std::unordered_set<std::string> KnowledgeGraph::neighbours(
    const std::string& start_id,
    size_t             max_depth,
    double             min_edge_weight,
    size_t             max_nodes) const
{
    std::lock_guard<std::mutex> lk(impl_->mtx);

    std::unordered_set<std::string> visited;
    if (!impl_->nodes.count(start_id)) return visited;

    // GAP-010: BFS is capped at max_nodes to prevent unbounded traversal
    // (DoS) on densely-connected graphs.  The caller can raise the cap
    // explicitly for trusted internal paths that genuinely need wider
    // traversal, but the safe default is 4096 nodes.
    // BFS queue: (node_id, depth)
    std::queue<std::pair<std::string, size_t>> q;
    q.push({start_id, 0});
    visited.insert(start_id);

    while (!q.empty()) {
        // GAP-010: Check node count before dequeuing.
        // visited always contains at least start_id (inserted before the loop),
        // so visited.size() >= 1 and the subtraction is safe from underflow.
        if (max_nodes > 0 && visited.size() - 1u >= max_nodes) {
            spdlog::warn("KnowledgeGraph::neighbours: BFS node cap ({}) reached "
                         "from '{}'; truncating traversal", max_nodes, start_id);
            break;
        }

        auto [cur_id, depth] = q.front();
        q.pop();

        if (depth >= max_depth) continue;

        auto adj_it = impl_->adj.find(cur_id);
        if (adj_it == impl_->adj.end()) continue;

        for (const auto& edge : adj_it->second) {
            if (max_nodes > 0 && visited.size() - 1u >= max_nodes) break;
            if (edge.weight < min_edge_weight) continue;
            if (visited.count(edge.to_id))     continue;
            if (!impl_->nodes.count(edge.to_id)) continue;

            visited.insert(edge.to_id);
            q.push({edge.to_id, depth + 1});
        }
    }

    visited.erase(start_id);   // exclude the start node itself
    return visited;
}

std::vector<KGEdge> KnowledgeGraph::outEdges(const std::string& node_id) const {
    std::lock_guard<std::mutex> lk(impl_->mtx);
    auto it = impl_->adj.find(node_id);
    if (it == impl_->adj.end()) return {};
    return it->second;
}

// =============================================================================
// EntityLinker helpers (static)
// =============================================================================

std::string EntityLinker::normalise(const std::string& s) {
    return normaliseText(s);
}

double EntityLinker::similarity(const std::string& a, const std::string& b) {
    return stringSimilarity(a, b);
}

// =============================================================================
// EntityLinker
// =============================================================================

EntityLinker::EntityLinker(const KnowledgeGraph&    graph,
                           const EntityLinkerConfig& config)
    : graph_(graph), config_(config) {}

std::vector<Entity> EntityLinker::extract(const std::string& text) const {
    std::vector<Entity> entities;

    // Strategy 1: capitalised multi-word and single-word spans.
    // Scan character by character collecting runs of capitalised-start tokens.
    std::string span_buf;
    size_t      span_start = std::string::npos;
    size_t      span_end   = 0;

    auto flushSpan = [&]() {
        if (span_buf.empty()) return;
        const std::string trimmed = [&]() {
            size_t s = span_buf.find_first_not_of(' ');
            size_t e = span_buf.find_last_not_of(' ');
            return (s == std::string::npos) ? std::string{} : span_buf.substr(s, e - s + 1);
        }();
        if (trimmed.size() >= config_.min_entity_length) {
            // Count words
            size_t words = 0;
            bool in_word = false;
            for (char c : trimmed) { if (c == ' ') { in_word = false; } else if (!in_word) { ++words; in_word = true; } }
            if (words > 1 || config_.extract_single_word_entities) {
                if (entities.size() < config_.max_entities_per_text) {
                    entities.push_back({trimmed, EntityType::OTHER, 0.7,
                                        span_start, span_end});
                }
            }
        }
        span_buf.clear();
        span_start = std::string::npos;
    };

    // Tokenise by whitespace and punctuation, track capitalised tokens.
    std::istringstream ss(text);
    std::string word;
    size_t pos = 0;
    while (ss >> word) {
        // Find position in text (approximate)
        pos = text.find(word, pos);
        const size_t word_end = (pos == std::string::npos) ? 0 : pos + word.size();

        // Strip leading/trailing punctuation for check
        std::string clean;
        for (unsigned char ch : word) {
            if (std::isalnum(ch) || ch == '-' || ch == '\'') {
                clean += static_cast<char>(ch);
            }
        }

        const bool is_cap = !clean.empty() && std::isupper(static_cast<unsigned char>(clean[0]));

        if (is_cap && clean.size() >= config_.min_entity_length) {
            if (span_start == std::string::npos) span_start = pos;
            if (!span_buf.empty()) span_buf += ' ';
            span_buf += clean;
            span_end = word_end;
        } else {
            flushSpan();
        }

        if (pos != std::string::npos) pos = word_end;
    }
    flushSpan();

    return entities;
}

std::vector<EntityLinkingMatch> EntityLinker::link(const std::string& text) const {
    std::vector<Entity> candidates = extract(text);
    std::vector<EntityLinkingMatch> matches;
    matches.reserve(candidates.size());

    for (auto& entity : candidates) {
        const std::string norm_text = normalise(entity.text);
        const KGNode* best_node     = nullptr;
        double         best_sim     = 0.0;

        // Search through all nodes for the best match
        // We use findNodeByName which already holds the lock internally.
        const KGNode* candidate_node = graph_.findNodeByName(entity.text);
        if (candidate_node) {
            best_sim  = stringSimilarity(norm_text,
                            normalise(candidate_node->canonical_name));
            // Also check aliases
            for (const auto& alias : candidate_node->aliases) {
                const double s = stringSimilarity(norm_text, normalise(alias));
                if (s > best_sim) best_sim = s;
            }
            best_node = candidate_node;
        }

        EntityLinkingMatch match;
        match.entity        = entity;
        match.linking_score = best_sim;
        match.is_linked     = (best_node != nullptr) &&
                              (best_sim >= config_.min_linking_score);
        match.node_id       = match.is_linked ? best_node->id : "";
        matches.push_back(std::move(match));
    }

    return matches;
}

// =============================================================================
// KnowledgeGraphRetriever::Impl
// =============================================================================

struct KnowledgeGraphRetriever::Impl {
    const KnowledgeGraph*                 graph;
    KGRetrieverConfig                     config;
    graph::KnowledgeGraphReasoner*        reasoner = nullptr;  ///< optional; not owned
};

// =============================================================================
// KnowledgeGraphRetriever
// =============================================================================

KnowledgeGraphRetriever::KnowledgeGraphRetriever(const KnowledgeGraph&    graph,
                                                 const KGRetrieverConfig& config)
    : impl_(std::make_unique<Impl>())
{
    impl_->graph  = &graph;
    impl_->config = config;
    THEMIS_DEBUG("KnowledgeGraphRetriever created: depth={}, kg_weight={:.2f}",
                 config.max_traversal_depth, config.kg_score_weight);
}

KnowledgeGraphRetriever::~KnowledgeGraphRetriever() = default;

const KGRetrieverConfig& KnowledgeGraphRetriever::getConfig() const {
    return impl_->config;
}

void KnowledgeGraphRetriever::setConfig(const KGRetrieverConfig& config) {
    impl_->config = config;
}

void KnowledgeGraphRetriever::setReasoner(graph::KnowledgeGraphReasoner* reasoner) {
    impl_->reasoner = reasoner;
}

KGRetrievalResult KnowledgeGraphRetriever::retrieve(
    const std::string&                           query,
    const std::vector<judge::RetrievedDocument>& candidates) const
{
    const auto t_start = std::chrono::steady_clock::now();

    THEMIS_INFO("KnowledgeGraphRetriever::retrieve query='{}', candidates={}",
                query, candidates.size());

    KGRetrievalResult result;
    const KGRetrieverConfig& cfg = impl_->config;

    // ── Step 1: Extract and link entities from the query ─────────────────────
    EntityLinker linker(*impl_->graph, cfg.linker_config);
    result.query_entity_links = linker.link(query);

    size_t linked_count = 0;
    for (const auto& m : result.query_entity_links) {
        if (m.is_linked) ++linked_count;
    }
    result.entity_linking_coverage =
        result.query_entity_links.empty()
            ? 0.0
            : static_cast<double>(linked_count) /
                  static_cast<double>(result.query_entity_links.size());

    THEMIS_DEBUG("Query entity linking: {}/{} linked (coverage={:.2f})",
                 linked_count, result.query_entity_links.size(),
                 result.entity_linking_coverage);

    // ── Step 2: BFS traversal from query-linked nodes ─────────────────────────
    std::unordered_set<std::string> query_neighbourhood;
    size_t nodes_visited = 0;

    for (const auto& match : result.query_entity_links) {
        if (!match.is_linked) continue;
        if (nodes_visited >= cfg.max_nodes_visited) break;

        auto nbrs = impl_->graph->neighbours(match.node_id,
                                             cfg.max_traversal_depth,
                                             cfg.min_edge_weight);
        query_neighbourhood.insert(match.node_id);
        for (auto& nb : nbrs) {
            query_neighbourhood.insert(nb);
            if (++nodes_visited >= cfg.max_nodes_visited) break;
        }
    }
    result.visited_nodes = query_neighbourhood;

    THEMIS_DEBUG("KG traversal: {} nodes in query neighbourhood", query_neighbourhood.size());

    // ── Step 2b: KnowledgeGraphReasoner multi-hop inference ─────────────────
    // Run forward-chaining inference for each linked query entity when a
    // reasoner has been attached and max_inference_hops > 0.
    if (impl_->reasoner && cfg.max_inference_hops > 0) {
        const auto r_start = std::chrono::steady_clock::now();

        for (const auto& match : result.query_entity_links) {
            if (!match.is_linked) continue;

            // Guard against reasoning timeout using wall-clock budget.
            const auto now = std::chrono::steady_clock::now();
            const double elapsed_so_far =
                std::chrono::duration<double, std::milli>(now - r_start).count();
            if (elapsed_so_far > cfg.reasoning_timeout_ms) {
                THEMIS_WARN("KnowledgeGraphRetriever: reasoning timeout ({:.1f}ms); "
                            "falling back to direct KG query", elapsed_so_far);
                break;
            }

            auto chain = impl_->reasoner->infer(match.node_id, cfg.max_inference_hops);
            if (!chain.empty()) {
                result.has_reasoning = true;
                result.inference_chains.push_back(chain);

                // Expand query neighbourhood with nodes reachable via inference.
                for (const auto& edge : chain.edges) {
                    query_neighbourhood.insert(edge.fact.subject);
                    query_neighbourhood.insert(edge.fact.object);
                }
            }
        }

        result.reasoning_elapsed_ms =
            std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - r_start).count();

        THEMIS_DEBUG("KGR reasoning: {} chains, {:.2f}ms",
                     result.inference_chains.size(),
                     result.reasoning_elapsed_ms);
    }

    // ── Step 3: Score each candidate document ────────────────────────────────
    result.documents.reserve(candidates.size());

    for (const auto& doc : candidates) {
        KGAugmentedDocument aug_doc;
        aug_doc.document    = doc;
        aug_doc.kg_boost    = 0.0;

        // Link entities in this document
        aug_doc.entity_links = linker.link(doc.content);

        // Build set of node IDs from this document's linked entities
        std::unordered_set<std::string> doc_node_ids;
        for (const auto& match : aug_doc.entity_links) {
            if (!match.is_linked) continue;
            doc_node_ids.insert(match.node_id);

            // Also add 1-hop neighbours of document entities
            auto nbrs = impl_->graph->neighbours(match.node_id, 1,
                                                 cfg.min_edge_weight);
            for (auto& nb : nbrs) {
                doc_node_ids.insert(nb);
            }
        }

        // KG boost = Jaccard overlap between query neighbourhood and
        //            document entity neighbourhood
        if (!query_neighbourhood.empty() || !doc_node_ids.empty()) {
            aug_doc.kg_boost = jaccardSets(query_neighbourhood, doc_node_ids);
        }

        // Fuse scores: orig * (1 - w) + kg_boost * w
        const double w = cfg.kg_score_weight;
        aug_doc.final_score = doc.similarity_score * (1.0 - w) +
                              aug_doc.kg_boost * w;

        // Attach reasoning chain to document metadata when requested.
        if (cfg.attach_reasoning_chain_to_metadata && result.has_reasoning) {
            std::string chain_text;
            for (const auto& chain : result.inference_chains) {
                for (const auto& edge : chain.edges) {
                    // Only include edges relevant to this document.
                    bool relevant = false;
                    for (const auto& dm : aug_doc.entity_links) {
                        if (dm.is_linked &&
                            (edge.fact.subject == dm.node_id ||
                             edge.fact.object  == dm.node_id)) {
                            relevant = true;
                            break;
                        }
                    }
                    if (!relevant) continue;
                    if (!chain_text.empty()) chain_text += "; ";
                    chain_text += edge.fact.subject + " -[" +
                                  edge.fact.predicate + "]-> " +
                                  edge.fact.object +
                                  " (rule=" + edge.rule_id + ")";
                }
            }
            if (!chain_text.empty()) {
                aug_doc.document.metadata["reasoning_chain"] = chain_text;
            }
        }

        THEMIS_DEBUG("Doc '{}': orig={:.3f}, kg_boost={:.3f}, final={:.3f}",
                     doc.id, doc.similarity_score, aug_doc.kg_boost,
                     aug_doc.final_score);

        result.documents.push_back(std::move(aug_doc));
    }

    // ── Step 4: Sort by final score descending ────────────────────────────────
    std::stable_sort(result.documents.begin(), result.documents.end(),
        [](const KGAugmentedDocument& a, const KGAugmentedDocument& b) {
            return a.final_score > b.final_score;
        });

    // ── Finalise ──────────────────────────────────────────────────────────────
    const auto t_end = std::chrono::steady_clock::now();
    result.elapsed_ms =
        std::chrono::duration<double, std::milli>(t_end - t_start).count();

    THEMIS_INFO("KnowledgeGraphRetriever::retrieve complete: {} docs, "
                "coverage={:.2f}, elapsed={:.2f}ms",
                result.documents.size(),
                result.entity_linking_coverage,
                result.elapsed_ms);

    return result;
}

// =============================================================================
// KnowledgeGraphRetrieverFactory
// =============================================================================

std::unique_ptr<KnowledgeGraphRetriever>
KnowledgeGraphRetrieverFactory::createShallow(const KnowledgeGraph& graph) {
    KGRetrieverConfig cfg;
    cfg.max_traversal_depth = 1;
    cfg.kg_score_weight     = 0.2;
    return std::make_unique<KnowledgeGraphRetriever>(graph, cfg);
}

std::unique_ptr<KnowledgeGraphRetriever>
KnowledgeGraphRetrieverFactory::createBalanced(const KnowledgeGraph& graph) {
    KGRetrieverConfig cfg;
    cfg.max_traversal_depth = 2;
    cfg.kg_score_weight     = 0.3;
    return std::make_unique<KnowledgeGraphRetriever>(graph, cfg);
}

std::unique_ptr<KnowledgeGraphRetriever>
KnowledgeGraphRetrieverFactory::createDeep(const KnowledgeGraph& graph) {
    KGRetrieverConfig cfg;
    cfg.max_traversal_depth = 3;
    cfg.kg_score_weight     = 0.45;
    return std::make_unique<KnowledgeGraphRetriever>(graph, cfg);
}

} // namespace themis::rag::kg
