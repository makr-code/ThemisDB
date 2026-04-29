#include "plugins/ethics_ai/ethics_selection_router.h"
#include "utils/logger.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <mutex>
#include <numeric>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#ifdef HAVE_YAML_CPP
#include <yaml-cpp/yaml.h>
#endif

namespace themis {
namespace plugins {
namespace ethics {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Case-fold and tokenise text into lower-cased words (≥ 3 chars).
std::vector<std::string> tokenise(const std::string& text) {
    std::vector<std::string> tokens;
    std::string cur;
    for (unsigned char ch : text) {
        if (std::isalnum(ch) || ch > 127) { // keep umlauts
            cur += static_cast<char>(std::tolower(ch));
        } else if (!cur.empty()) {
            if (cur.size() >= 3) tokens.push_back(cur);
            cur.clear();
        }
    }
    if (cur.size() >= 3) tokens.push_back(cur);
    return tokens;
}

/// Build token frequency map.
std::unordered_map<std::string, double> termFreq(
    const std::vector<std::string>& tokens)
{
    std::unordered_map<std::string, double> freq;
    for (const auto& t : tokens) freq[t] += 1.0;
    // Normalise
    if (!tokens.empty()) {
        const double n = static_cast<double>(tokens.size());
        for (auto& kv : freq) kv.second /= n;
    }
    return freq;
}

/// Cosine-like term-overlap similarity between two token frequency maps.
double termOverlapSimilarity(
    const std::unordered_map<std::string, double>& a,
    const std::unordered_map<std::string, double>& b)
{
    if (a.empty() || b.empty()) return 0.0;
    double dot = 0.0;
    double norm_a = 0.0, norm_b = 0.0;
    for (const auto& [t, fa] : a) {
        norm_a += fa * fa;
        auto it = b.find(t);
        if (it != b.end()) dot += fa * it->second;
    }
    for (const auto& [t, fb] : b) norm_b += fb * fb;
    const double denom = std::sqrt(norm_a) * std::sqrt(norm_b);
    if (denom < 1e-12) return 0.0;
    return std::min(1.0, dot / denom);
}

/// Sigmoid mapping raw score to [0, 1].
double sigmoid(double x) { return 1.0 / (1.0 + std::exp(-x)); }

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Pimpl struct
// ─────────────────────────────────────────────────────────────────────────────

struct EthicsSelectionRouter::Impl {
    IEthicsProfileRegistry* registry;
    RouterConfig            config;

    // Taxonomy: class_name → [school_id, ...]
    std::unordered_map<std::string, std::vector<std::string>> taxonomy_map;
    // Domain → [class_name, ...]
    std::unordered_map<std::string, std::vector<std::string>> domain_class_map;
    // always_include_when flags
    bool always_compliance_on_regulatory{true};
    bool always_regulatory_auth_on_public{false};
    bool always_academic_on_academic{false};

    // Precedent store: dilemma_type → {school_id → cumulative DC score, count}
    struct PrecedentEntry { double dc_sum{0.0}; size_t count{0}; };
    mutable std::mutex precedent_mutex;
    std::unordered_map<std::string,
        std::unordered_map<std::string, PrecedentEntry>> precedent_store;

    void loadTaxonomy(const std::string& yaml_path);
    std::unordered_set<std::string> stage1(
        const std::string& domain,
        const std::vector<std::string>& tags,
        bool regulatory_context) const;
    std::vector<RouterCandidate> stage2(
        const std::string& dilemma_text,
        const std::unordered_set<std::string>& candidates) const;
    void stage3(
        std::vector<RouterCandidate>& candidates,
        const std::string& dilemma_domain) const;
};

// ─────────────────────────────────────────────────────────────────────────────
// Taxonomy loading
// ─────────────────────────────────────────────────────────────────────────────

void EthicsSelectionRouter::Impl::loadTaxonomy(const std::string& yaml_path)
{
    if (yaml_path.empty()) return;

#ifdef HAVE_YAML_CPP
    try {
        YAML::Node root = YAML::LoadFile(yaml_path);
        const auto& tax = root["taxonomy"];
        if (tax && tax.IsMap()) {
            for (const auto& kv : tax) {
                const std::string cls = kv.first.as<std::string>("");
                if (kv.second.IsSequence()) {
                    for (const auto& item : kv.second) {
                        if (item.IsScalar())
                            taxonomy_map[cls].push_back(item.as<std::string>());
                    }
                }
            }
        }
        const auto& dcm = root["domain_class_mapping"];
        if (dcm && dcm.IsMap()) {
            for (const auto& kv : dcm) {
                const std::string dom = kv.first.as<std::string>("");
                if (kv.second.IsSequence()) {
                    for (const auto& item : kv.second) {
                        if (item.IsScalar())
                            domain_class_map[dom].push_back(item.as<std::string>());
                    }
                }
            }
        }
    } catch (const std::exception& ex) {
        LOG_WARN("EthicsSelectionRouter: failed to load taxonomy from '{}': {}",
                 yaml_path, ex.what());
    }
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// Stage 1 — Tag / taxonomy filter
// ─────────────────────────────────────────────────────────────────────────────

std::unordered_set<std::string> EthicsSelectionRouter::Impl::stage1(
    const std::string& domain,
    const std::vector<std::string>& tags,
    bool regulatory_context) const
{
    std::unordered_set<std::string> candidates;

    auto addClassSchools = [&](const std::string& cls) {
        auto it = taxonomy_map.find(cls);
        if (it != taxonomy_map.end()) {
            for (const auto& sid : it->second) candidates.insert(sid);
        }
    };

    // --- 1a) Domain → taxonomy class mapping ---------------------------------
    if (!domain.empty()) {
        auto it = domain_class_map.find(domain);
        if (it != domain_class_map.end()) {
            for (const auto& cls : it->second) addClassSchools(cls);
        }
    }

    // --- 1b) Tags: if a tag exactly matches a school_id or taxonomy class ----
    for (const auto& tag : tags) {
        // Direct school_id match
        if (registry->hasProfile(tag)) candidates.insert(tag);
        // Taxonomy class match
        addClassSchools(tag);
    }

    // --- 1c) Always-include rules --------------------------------------------
    if (regulatory_context) addClassSchools("compliance");

    // --- 1d) If nothing matched, return all known profiles -------------------
    if (candidates.empty()) {
        EthicsIndexQuery q;
        for (const auto& meta : registry->queryIndex(q)) {
            candidates.insert(meta.school_id);
        }
    }

    // Remove school_ids not actually registered
    std::unordered_set<std::string> valid;
    for (const auto& sid : candidates) {
        if (registry->hasProfile(sid)) valid.insert(sid);
    }
    return valid;
}

// ─────────────────────────────────────────────────────────────────────────────
// Stage 2 — Semantic term-overlap filter
//
// STUB/SIMULATION NOTE:
// Purpose: Lightweight term-overlap proxy for semantic embedding similarity.
//          Used until IEmbeddingProvider + ArgumentStore::searchSimilarArguments()
//          are fully implemented (FUTURE_ENHANCEMENTS §7).
// Activation: Always active; replaces real embedding model.
// Production Delta: Real production path uses all-mpnet-base-v2 (768-dim)
//          via ONNX Runtime. Term-overlap cosine underestimates semantic
//          synonymy (e.g. "duty" vs "obligation") by ~15-20%.
// Removal Plan: Replace with OnnxEmbeddingProvider when §7 is delivered
//          (Target: Q3 2026). Gate via THEMIS_ETHICS_EMBEDDING_MODEL env var.
// ─────────────────────────────────────────────────────────────────────────────

std::vector<RouterCandidate> EthicsSelectionRouter::Impl::stage2(
    const std::string& dilemma_text,
    const std::unordered_set<std::string>& candidates) const
{
    const auto dilemma_tokens = tokenise(dilemma_text);
    const auto dilemma_freq   = termFreq(dilemma_tokens);

    // Build school_id → metadata lookup once (O(n) instead of O(n²))
    std::unordered_map<std::string, std::string> id_to_text;
    {
        EthicsIndexQuery meta_q; // no filters — fetch all
        for (const auto& m : registry->queryIndex(meta_q)) {
            std::string profile_text = m.name + " " + m.description_snippet;
            for (const auto& t : m.tags)             profile_text += " " + t;
            for (const auto& d : m.applicable_domains) profile_text += " " + d;
            id_to_text[m.school_id] = std::move(profile_text);
        }
    }

    std::vector<RouterCandidate> result;
    result.reserve(candidates.size());

    for (const auto& sid : candidates) {
        // Use metadata text if available; fall back to school_id itself
        auto it = id_to_text.find(sid);
        const std::string& profile_text = (it != id_to_text.end()) ? it->second : sid;

        const auto profile_tokens = tokenise(profile_text);
        const auto profile_freq   = termFreq(profile_tokens);
        const double sim = termOverlapSimilarity(dilemma_freq, profile_freq);

        RouterCandidate cand;
        cand.school_id      = sid;
        cand.semantic_score = sigmoid(sim * 8.0 - 1.0); // map to [0,1]
        cand.taxonomy_score = 1.0;                       // set later by caller
        result.push_back(cand);
    }

    // Sort descending by semantic score, keep top stage2_top_k
    std::sort(result.begin(), result.end(),
              [](const RouterCandidate& a, const RouterCandidate& b) {
                  return a.semantic_score > b.semantic_score;
              });
    if (result.size() > config.stage2_top_k) {
        result.resize(config.stage2_top_k);
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Stage 3 — Precedent lookup
//
// STUB/SIMULATION NOTE:
// Purpose: In-memory precedent store as proxy for full KnowledgeGraph
//          integration with `_themis_ethics_precedents` graph collection.
// Activation: Always active; KG paths guarded by THEMIS_ETHICS_KG_PRECEDENTS.
// Production Delta: Production path traverses a persistent ArangoDB graph
//          where nodes are dilemma-type strings and edges carry DC scores.
//          In-memory store is session-scoped (not persistent across restarts).
// Removal Plan: Replace in-memory map with KnowledgeGraphRetriever.retrieve()
//          call when the ethics precedent graph is populated (Target: Q4 2026).
// ─────────────────────────────────────────────────────────────────────────────

void EthicsSelectionRouter::Impl::stage3(
    std::vector<RouterCandidate>& candidates,
    const std::string& dilemma_domain) const
{
    std::lock_guard<std::mutex> lock(precedent_mutex);
    auto it = precedent_store.find(dilemma_domain);
    if (it == precedent_store.end()) {
        // No precedents yet: all candidates get equal neutral precedent score
        for (auto& c : candidates) c.precedent_dc = 0.5;
        return;
    }

    const auto& school_map = it->second;
    for (auto& c : candidates) {
        auto jt = school_map.find(c.school_id);
        if (jt != school_map.end() && jt->second.count > 0) {
            c.precedent_dc = jt->second.dc_sum / static_cast<double>(jt->second.count);
        } else {
            c.precedent_dc = 0.5; // neutral for unseen schools
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// EthicsSelectionRouter public API
// ─────────────────────────────────────────────────────────────────────────────

EthicsSelectionRouter::EthicsSelectionRouter(IEthicsProfileRegistry* registry,
                                              const RouterConfig& cfg)
    : impl_(std::make_unique<Impl>())
{
    impl_->registry = registry;
    impl_->config   = cfg;

    // Normalise weights
    const double total = cfg.weight_semantic + cfg.weight_precedent + cfg.weight_taxonomy;
    if (total > 1e-9) {
        impl_->config.weight_semantic  = cfg.weight_semantic  / total;
        impl_->config.weight_precedent = cfg.weight_precedent / total;
        impl_->config.weight_taxonomy  = cfg.weight_taxonomy  / total;
    }

    impl_->loadTaxonomy(cfg.taxonomy_yaml_path);
}

EthicsSelectionRouter::~EthicsSelectionRouter() = default;

const RouterConfig& EthicsSelectionRouter::config() const {
    return impl_->config;
}

RouterResult EthicsSelectionRouter::route(
    const std::string& dilemma_text,
    const std::string& dilemma_domain,
    const std::vector<std::string>& dilemma_tags,
    bool regulatory_context) const
{
    RouterResult result;

    // ── Stage 1 ─────────────────────────────────────────────────────────────
    auto stage1_set = impl_->stage1(dilemma_domain, dilemma_tags, regulatory_context);
    result.stage1_count = stage1_set.size();

    // ── Stage 2 ─────────────────────────────────────────────────────────────
    auto candidates = impl_->stage2(dilemma_text, stage1_set);
    result.stage2_count = candidates.size();

    // ── Stage 3 ─────────────────────────────────────────────────────────────
    impl_->stage3(candidates, dilemma_domain);
    result.stage3_count = candidates.size();

    // ── Aggregation ──────────────────────────────────────────────────────────
    const double ws = impl_->config.weight_semantic;
    const double wp = impl_->config.weight_precedent;
    const double wt = impl_->config.weight_taxonomy;

    for (auto& c : candidates) {
        c.final_score = ws * c.semantic_score +
                        wp * c.precedent_dc   +
                        wt * c.taxonomy_score;
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const RouterCandidate& a, const RouterCandidate& b) {
                  return a.final_score > b.final_score;
              });

    const size_t n = std::min(candidates.size(), impl_->config.top_n);
    result.selected.assign(candidates.begin(), candidates.begin() + static_cast<ptrdiff_t>(n));

    return result;
}

void EthicsSelectionRouter::recordDecisionOutcome(
    const std::string& dilemma_type,
    const std::string& school_id,
    double dc_score)
{
    std::lock_guard<std::mutex> lock(impl_->precedent_mutex);
    auto& entry = impl_->precedent_store[dilemma_type][school_id];
    entry.dc_sum += std::max(0.0, std::min(1.0, dc_score));
    entry.count++;
}

} // namespace ethics
} // namespace plugins
} // namespace themis
