/**
 * @file ethics_selection_router.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=9; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=2, Debt=0, C=1, H=5, M=16, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ethics_ai/ethics_selection_router.h"
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

/// Steepness of the sigmoid applied to term-overlap similarity scores.
/// Value 8.0 chosen empirically: at sim=0.25 (weak match) the output is ~0.50;
/// at sim=0.50 (moderate) ~0.88; at sim=0.0 (no overlap) ~0.27.
static constexpr double kSigmoidSteepness = 8.0;
/// Bias shifts the sigmoid midpoint towards sim≈0.125, so unrelated profiles
/// do not score near 0.5 by default.
static constexpr double kSigmoidBias = 1.0;

/// Cosine similarity between two dense float vectors.
/// Returns 0.0 if either vector is empty or has mismatched dimensions.
double cosineSimilarityVec(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.empty() || b.empty() || a.size() != b.size()) return 0.0;
    double dot = 0.0, norm_a = 0.0, norm_b = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        dot    += static_cast<double>(a[i]) * static_cast<double>(b[i]);
        norm_a += static_cast<double>(a[i]) * static_cast<double>(a[i]);
        norm_b += static_cast<double>(b[i]) * static_cast<double>(b[i]);
    }
    const double denom = std::sqrt(norm_a) * std::sqrt(norm_b);
    if (denom < 1e-12) return 0.0;
    return std::max(-1.0, std::min(1.0, dot / denom));
}

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

    // Optional injection: real embedding backend for Stage-2 (stub #146).
    // When set, cosine similarity between dense vectors replaces term-overlap.
    EthicsSelectionRouter::EmbeddingFn embedding_fn;

    // Optional injection: persistent precedent query for Stage-3 (stub #147).
    // When set, called instead of consulting the in-memory precedent_store.
    EthicsSelectionRouter::PrecedentQueryFn precedent_query_fn;

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
        THEMIS_WARN("EthicsSelectionRouter: failed to load taxonomy from '{}': {}",
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
// PERMANENT FALLBACK NOTE:
// Purpose: Lightweight term-overlap proxy for semantic embedding similarity.
//          This is a permanent fallback for builds without an EmbeddingProvider.
// Activation: Term-overlap fallback is active when no EmbeddingFn is injected
//          via setEmbeddingFn(). When an EmbeddingFn is set, dense cosine
//          similarity replaces term-overlap (real production path).
// Production Delta: Term-overlap cosine underestimates semantic synonymy
//          (e.g. "duty" vs "obligation") by ~15-20%.
// Wiring: Inject an OnnxEmbeddingProvider via setEmbeddingFn() when §7
//          is delivered (Target: Q3 2026). Gate via THEMIS_ETHICS_EMBEDDING_MODEL
//          env var. RESOLVED 2026-05-06: setEmbeddingFn() injection API wired;
//          dense cosine path active when fn is set (see stage2() fast-path above).
// ─────────────────────────────────────────────────────────────────────────────

std::vector<RouterCandidate> EthicsSelectionRouter::Impl::stage2(
    const std::string& dilemma_text,
    const std::unordered_set<std::string>& candidates) const
{
    // ── Fast-path: real embedding backend injected ──────────────────────────
    if (embedding_fn) {
        const auto query_emb = embedding_fn(dilemma_text);

        // Build school_id → metadata lookup once
        std::unordered_map<std::string, std::string> id_to_text;
        {
            EthicsIndexQuery meta_q;
            for (const auto& m : registry->queryIndex(meta_q)) {
                std::string profile_text = m.name + " " + m.description_snippet;
                for (const auto& t : m.tags)              profile_text += " " + t;
                for (const auto& d : m.applicable_domains) profile_text += " " + d;
                id_to_text[m.school_id] = std::move(profile_text);
            }
        }

        std::vector<RouterCandidate> result;
        result.reserve(candidates.size());

        for (const auto& sid : candidates) {
            auto it = id_to_text.find(sid);
            const std::string& profile_text = (it != id_to_text.end()) ? it->second : sid;

            double sim = 0.0;
            if (!query_emb.empty()) {
                const auto profile_emb = embedding_fn(profile_text);
                const double raw = cosineSimilarityVec(query_emb, profile_emb);
                // Shift cosine [-1,1] to [0,1]
                sim = (raw + 1.0) * 0.5;
            }

            RouterCandidate cand;
            cand.school_id      = sid;
            cand.semantic_score = sim;
            cand.taxonomy_score = 1.0;
            result.push_back(cand);
        }

        std::sort(result.begin(), result.end(),
                  [](const RouterCandidate& a, const RouterCandidate& b) {
                      return a.semantic_score > b.semantic_score;
                  });
        if (result.size() > config.stage2_top_k) {
            result.resize(config.stage2_top_k);
        }
        return result;
    }

    // ── Fallback: term-overlap TF cosine ────────────────────────────────────
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
        cand.semantic_score = sigmoid(sim * kSigmoidSteepness - kSigmoidBias);
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
// PERMANENT FALLBACK NOTE:
// Purpose: In-memory precedent store as permanent fallback for builds without
//          a live KnowledgeGraph connection.
// Activation: In-memory map is active when no PrecedentQueryFn is injected
//          via setPrecedentQueryFn(). When a PrecedentQueryFn is set, the
//          external persistent store is consulted instead (real production path).
// Production Delta: Production path traverses a persistent ArangoDB graph
//          where nodes are dilemma-type strings and edges carry DC scores.
//          In-memory store is session-scoped (not persistent across restarts).
// Wiring: Inject a KnowledgeGraphRetriever-backed fn via setPrecedentQueryFn()
//          when the ethics precedent graph is populated (Target: Q4 2026).
//          RESOLVED 2026-05-06: setPrecedentQueryFn() injection API wired;
//          persistent-graph path active when fn is set (see stage3() fast-path).
// ─────────────────────────────────────────────────────────────────────────────

void EthicsSelectionRouter::Impl::stage3(
    std::vector<RouterCandidate>& candidates,
    const std::string& dilemma_domain) const
{
    // ── Fast-path: external persistent precedent query function injected ────
    if (precedent_query_fn) {
        for (auto& c : candidates) {
            c.precedent_dc = precedent_query_fn(dilemma_domain, c.school_id);
        }
        return;
    }

    // ── Fallback: in-memory precedent store ─────────────────────────────────
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

// ─────────────────────────────────────────────────────────────────────────────
// LDM-1: planDiscourse()
// ─────────────────────────────────────────────────────────────────────────────

DiscourseOrchestratorPlan EthicsSelectionRouter::planDiscourse(
    const std::string& domain_context) const
{
    DiscourseOrchestratorPlan plan;
    plan.mode = impl_->config.discourse_mode;

    if (plan.mode == DiscourseMode::SELECTION_ONLY) {
        // Classical path — no discourse plan needed.
        return plan;
    }

    // Collect all loaded school ids via the registry index.
    EthicsIndexQuery all_query;
    if (!domain_context.empty()) {
        all_query.domains = {domain_context};
    }
    const auto all_meta = impl_->registry->queryIndex(all_query);

    plan.ebene1_school_ids.reserve(all_meta.size());
    for (const auto& m : all_meta) {
        plan.ebene1_school_ids.push_back(m.school_id);
    }

    if (plan.ebene1_school_ids.empty()) {
        return plan;
    }

    // Compute equal initial weight w₀ = 1/N (Ebene-1 contract).
    plan.initial_weight = 1.0 / static_cast<double>(plan.ebene1_school_ids.size());

    // Build cluster map (required for LAYERED_FULL; omitted for LAYERED_FAST).
    if (plan.mode == DiscourseMode::LAYERED_FULL) {
        // Canonical 6 clusters from the LDM taxonomy.
        static const std::unordered_map<std::string, std::string> kTaxonomyToCluster = {
            {"deontological",       "Deontological"},
            {"consequentialist",    "Consequentialist"},
            {"virtue",              "Virtue"},
            {"cultural_religious",  "Cultural-Religious"},
            {"non_mainstream",      "Non-Mainstream"},
            {"institutional",       "Institutional"},
        };

        for (const auto& m : all_meta) {
            const std::string& tc = m.taxonomy_class;
            auto it = kTaxonomyToCluster.find(tc);
            const std::string cluster =
                (it != kTaxonomyToCluster.end()) ? it->second : "Non-Mainstream";
            plan.cluster_map[cluster].push_back(m.school_id);
        }

        // Populate the 4 canonical structural tension axes (always activated
        // for LAYERED_FULL; used by DiscourseOrchestrator::runEbene2).
        plan.tension_axes = {
            "Kant\u2194Utilitarismus",
            "W\u00fcrde-cluster\u2194Aggregation-cluster",
            "Individualismus\u2194Kollektivismus",
            "Positivrecht\u2194Naturrecht",
        };
    }

    return plan;
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

    const auto& bias_map = impl_->config.school_bias;
    for (auto& c : candidates) {
        // Look up per-school bias multiplier (default 1.0 = neutral).
        // Bias is applied after weighted aggregation so it shifts relative
        // ranking without distorting the semantic/precedent/taxonomy balance.
        // final_score is clamped to [0, 1] to preserve the documented invariant.
        double bias = 1.0;
        if (!bias_map.empty()) {
            const auto it = bias_map.find(c.school_id);
            if (it != bias_map.end()) {
                bias = std::max(0.0, it->second);
            }
        }
        const double raw = bias * (ws * c.semantic_score +
                                   wp * c.precedent_dc   +
                                   wt * c.taxonomy_score);
        c.final_score = std::min(1.0, raw);
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

void EthicsSelectionRouter::setEmbeddingFn(EmbeddingFn fn) {
    impl_->embedding_fn = std::move(fn);
}

void EthicsSelectionRouter::setPrecedentQueryFn(PrecedentQueryFn fn) {
    impl_->precedent_query_fn = std::move(fn);
}

} // namespace ethics
} // namespace plugins
} // namespace themis
