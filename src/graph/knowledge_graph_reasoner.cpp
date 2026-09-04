/**
 * @file knowledge_graph_reasoner.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "graph/knowledge_graph_reasoner.h"

#if defined(THEMIS_ENABLE_LLM)
#include "llm/multi_lora_manager.h"
#endif

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace themis {
namespace graph {

namespace {

/// Build canonical triple key (shared between InferenceStore and KnowledgeGraphReasoner).
static std::string makeTripleKey(const themis::graph::Triple &t) {
    std::string k = {};
    k.reserve(t.subject.size() + t.predicate.size() + t.object.size() + 2);
    k += t.subject;
    k += '\0';
    k += t.predicate;
    k += '\0';
    k += t.object;
    return k;
}

} // namespace (helpers)

// ─────────────────────────────────────────────────────────────────────────────
// InferenceStore — helpers
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ std::string InferenceStore::makeKey(const Triple &t) {
    return makeTripleKey(t);
}

// ─────────────────────────────────────────────────────────────────────────────
// InferenceStore — public methods
// ─────────────────────────────────────────────────────────────────────────────

void InferenceStore::store(Triple fact, std::string rule_id, std::vector<Triple> premises, std::chrono::seconds ttl) {
    const auto key        = makeKey(fact);
    const auto expires_at = std::chrono::steady_clock::now() + ttl;

    std::unique_lock lock(mutex_);

    // Evict expired entries lazily (without lock upgrade: we hold unique already).
    {
        auto it = insertion_order_.begin();
        while (it != insertion_order_.end()) {
            auto eit = entries_.find(*it);
            if (eit != entries_.end() && eit->second.expires_at < std::chrono::steady_clock::now()) {
                entries_.erase(eit);
                it = insertion_order_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // If already present — refresh.
    auto existing = entries_.find(key);
    if (existing != entries_.end()) {
        existing->second.expires_at    = expires_at;
        existing->second.edge.rule_id  = std::move(rule_id);
        existing->second.edge.premises = std::move(premises);
        return;
    }

    // Evict oldest entry when full.
    if (entries_.size() >= kMaxTriples && !insertion_order_.empty()) {
        entries_.erase(insertion_order_.front());
        insertion_order_.pop_front();
    }

    InferenceEdge edge;
    edge.fact     = std::move(fact);
    edge.rule_id  = std::move(rule_id);
    edge.premises = std::move(premises);

    entries_[key] = Entry{std::move(edge), expires_at};
    insertion_order_.push_back(key);
}

bool InferenceStore::contains(const Triple &t) const {
    const auto key = makeKey(t);
    std::shared_lock lock(mutex_);
    auto it = entries_.find(key);
    if (it == entries_.end()) {
        return false;
    }
    return it->second.expires_at >= std::chrono::steady_clock::now();
}

std::optional<InferenceEdge> InferenceStore::get(const Triple &t) const {
    const auto key = makeKey(t);
    std::shared_lock lock(mutex_);
    auto it = entries_.find(key);
    if (it == entries_.end()) {
        return std::nullopt;
    }
    if (it->second.expires_at < std::chrono::steady_clock::now()) {
        return std::nullopt;
    }
    return it->second.edge;
}

std::vector<InferenceEdge> InferenceStore::getDerived(std::string_view subject) const {
    std::vector<InferenceEdge> result;
    const auto now = std::chrono::steady_clock::now();
    std::shared_lock lock(mutex_);
    result.reserve(std::min(entries_.size(), std::size_t{64}));
    for (const auto &[key, entry] : entries_) {
        if (entry.expires_at < now) {
            continue;
        }
        if (entry.edge.fact.subject == subject) {
            result.push_back(entry.edge);
        }
    }
    return result;
}

void InferenceStore::evictExpired() {
    const auto now = std::chrono::steady_clock::now();
    std::unique_lock lock(mutex_);
    auto it = insertion_order_.begin();
    while (it != insertion_order_.end()) {
        auto eit = entries_.find(*it);
        if (eit != entries_.end() && eit->second.expires_at < now) {
            entries_.erase(eit);
            it = insertion_order_.erase(it);
        } else {
            ++it;
        }
    }
}

std::size_t InferenceStore::size() const {
    std::shared_lock lock(mutex_);
    return entries_.size();
}

void InferenceStore::clear() {
    std::unique_lock lock(mutex_);
    entries_.clear();
    insertion_order_.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// KnowledgeGraphReasoner — construction
// ─────────────────────────────────────────────────────────────────────────────

KnowledgeGraphReasoner::KnowledgeGraphReasoner([[maybe_unused]] int max_inference_hops)
    : max_hops_(std::max(1, std::min(max_inference_hops, kHardMaxHops))) {}

// ─────────────────────────────────────────────────────────────────────────────
// Rule management
// ─────────────────────────────────────────────────────────────────────────────

bool KnowledgeGraphReasoner::addRule(Rule rule) {
    if (rule.id.empty() || rule.conditions.empty() || rule.conclusions.empty()) {
        return false;
    }
    std::unique_lock lock(rules_mutex_);
    // Remove any existing rule with the same id.
    auto it = std::find_if(rules_.begin(), rules_.end(), [&]([[maybe_unused]] const Rule &r) { return r.id == rule.id; });
    if (it != rules_.end()) {
        *it = std::move(rule);
    } else {
        rules_.push_back(std::move(rule));
    }
    return true;
}

std::size_t KnowledgeGraphReasoner::ruleCount() const {
    std::shared_lock lock(rules_mutex_);
    return rules_.size();
}

void KnowledgeGraphReasoner::clearRules() {
    std::unique_lock lock(rules_mutex_);
    rules_.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// Fact management
// ─────────────────────────────────────────────────────────────────────────────

void KnowledgeGraphReasoner::addFact(Triple fact) {
    if (!fact.isGround()) {
        return; // reject non-ground triples
    }
    std::unique_lock lock(facts_mutex_);
    // Deduplicate.
    for (const auto &f : base_facts_) {
        if (f == fact) {
            return;
        }
    }
    base_facts_.push_back(std::move(fact));
}

std::size_t KnowledgeGraphReasoner::factCount() const {
    std::shared_lock lock(facts_mutex_);
    return base_facts_.size();
}

void KnowledgeGraphReasoner::clearFacts() {
    std::unique_lock lock(facts_mutex_);
    base_facts_.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// Variable binding helpers (static)
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ bool KnowledgeGraphReasoner::unify(const Triple &pattern, const Triple &fact, Bindings &bindings) {
    // Helper: try to unify one field
    auto unifyField = [&](const std::string &pat, const std::string &val) -> bool {
        if (!pat.empty() && pat[0] == '?') {
            auto it = bindings.find(pat);
            if (it == bindings.end()) {
                bindings[pat] = val;
                return true;
            }
            return it->second == val;
        }
        return pat == val;
    };

    return unifyField(pattern.subject, fact.subject) && unifyField(pattern.predicate, fact.predicate)
           && unifyField(pattern.object, fact.object);
}

/*static*/ Triple KnowledgeGraphReasoner::ground(const Triple &pattern, const Bindings &bindings) {
    auto resolve = [&]([[maybe_unused]] const std::string &s) -> std::string {
        if (!s.empty() && s[0] == '?') {
            auto it = bindings.find(s);
            if (it != bindings.end()) {
                return it->second;
            }
        }
        return s;
    };
    return {resolve(pattern.subject), resolve(pattern.predicate), resolve(pattern.object)};
}

/*static*/ void KnowledgeGraphReasoner::matchConditions(const std::vector<Triple> &conditions, std::size_t cond_idx,
                                                        const std::vector<Triple> &facts, Bindings bindings,
                                                        std::vector<Bindings> &out) {
    if (cond_idx >= conditions.size()) {
        out.push_back(std::move(bindings));
        return;
    }

    const Triple &pattern = conditions[cond_idx];
    for (const auto &fact : facts) {
        Bindings candidate = bindings;
        if (unify(pattern, fact, candidate)) {
            matchConditions(conditions, cond_idx + 1, facts, std::move(candidate), out);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Triple key helper
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ std::string KnowledgeGraphReasoner::tripleKey(const Triple &t) {
    return makeTripleKey(t);
}

// ─────────────────────────────────────────────────────────────────────────────
// Forward-chaining core
// ─────────────────────────────────────────────────────────────────────────────

void KnowledgeGraphReasoner::forwardChain(std::vector<Triple> &working_set, std::vector<InferenceEdge> &derived_out,
                                          int max_depth) const {
    // Track which triples are already known to avoid cycles.
    std::unordered_set<std::string> known = {};

    known.reserve(working_set.size() * 2);
    for (const auto &t : working_set) {
        known.insert(tripleKey(t));
    }

    std::shared_lock rules_lock(rules_mutex_);

    for (int hop = 0; hop < max_depth; ++hop) {
        std::vector<Triple> new_this_hop;

        for (const auto &rule : rules_) {
            // Find all binding sets satisfying all conditions.
            std::vector<Bindings> all_bindings;
            matchConditions(rule.conditions, 0, working_set, {}, all_bindings);

            for (const auto &binds : all_bindings) {
                // Derive all conclusions.
                std::vector<Triple> conclusions;
                bool all_ground = true;
                for (const auto &conclusion_pat : rule.conclusions) {
                    Triple derived_triple = ground(conclusion_pat, binds);
                    if (!derived_triple.isGround()) {
                        all_ground = false;
                        break;
                    }
                    conclusions.push_back(std::move(derived_triple));
                }
                if (!all_ground) {
                    continue;
                }

                // Build premise list from bound conditions.
                std::vector<Triple> premises = {};

                for (const auto &cond : rule.conditions) {
                    premises.push_back(ground(cond, binds));
                }

                for (auto &conclusion : conclusions) {
                    const auto key = tripleKey(conclusion);
                    if (known.count(key)) {
                        continue; // already in working set
                    }

                    known.insert(key);
                    new_this_hop.push_back(conclusion);

                    InferenceEdge edge;
                    edge.fact     = conclusion;
                    edge.rule_id  = rule.id;
                    edge.premises = premises;

                    // Persist in store for explain().
                    inference_store_.store(conclusion, rule.id, premises);

                    derived_out.push_back(std::move(edge));
                }
            }
        }

        if (new_this_hop.empty()) {
            break; // fixpoint reached
        }

        for (auto &t : new_this_hop) {
            working_set.push_back(std::move(t));
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// infer()
// ─────────────────────────────────────────────────────────────────────────────

InferenceChain KnowledgeGraphReasoner::infer(std::string_view subjectId, int depth) const {
    const int hops = (depth <= 0) ? max_hops_ : std::max(1, std::min(depth, kHardMaxHops));

    InferenceChain chain;
    chain.subject_id = std::string(subjectId);

    // Snapshot base facts.
    std::vector<Triple> working_set;
    {
        std::shared_lock lock(facts_mutex_);
        working_set = base_facts_;
    }

    if (working_set.empty()) {
        return chain;
    }

    std::vector<InferenceEdge> all_derived;
    forwardChain(working_set, all_derived, hops);

    // Filter to those involving the requested subject.
    for (auto &edge : all_derived) {
        if (edge.fact.subject == subjectId) {
            chain.edges.push_back(std::move(edge));
        }
    }

    return chain;
}

// ─────────────────────────────────────────────────────────────────────────────
// explain()
// ─────────────────────────────────────────────────────────────────────────────

std::optional<InferenceEdge> KnowledgeGraphReasoner::explain(const Triple &fact) const {
    return inference_store_.get(fact);
}

// ─────────────────────────────────────────────────────────────────────────────
// onCDCEvent()
// ─────────────────────────────────────────────────────────────────────────────

void KnowledgeGraphReasoner::onCDCEvent([[maybe_unused]] const CDCEvent &event) {
    if ([[maybe_unused]] event.op == CDCEvent::Op::INSERT) {
        // Add to base facts (deduplication inside addFact).
        if ([[maybe_unused]] !event.edge.isGround()) {
            return;
        }

        // Add the new fact.
        addFact([[maybe_unused]] event.edge);

        // Run one incremental forward-chaining pass with just the new fact plus
        // existing base facts — much cheaper than a full re-evaluation.
        std::vector<Triple> working_set;
        {
            std::shared_lock lock(facts_mutex_);
            working_set = base_facts_;
        }

        std::vector<InferenceEdge> derived;
        forwardChain(working_set, derived, 1); // single incremental hop

    } else { // DELETE
        // Remove from base facts.
        {
            std::unique_lock lock(facts_mutex_);
            auto it = std::remove_if(base_facts_.begin(), base_facts_.end(),
                                     [&]([[maybe_unused]] const Triple &t) { return t == event.edge; });
            base_facts_.erase(it, base_facts_.end());
        }

        // Conservatively clear derived triples whose premises included this edge.
        // We rebuild on the next infer() call.
        std::vector<InferenceEdge> all_derived = inference_store_.getDerived([[maybe_unused]] event.edge.subject);
        for (auto &edge : all_derived) {
            for (const auto &premise : edge.premises) {
                if ([[maybe_unused]] premise == event.edge) {
                    // Evict this derived triple.
                    inference_store_.store(edge.fact, "__deleted__", {}, std::chrono::seconds{0});
                    break;
                }
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// setLoraScoreFn()
// ─────────────────────────────────────────────────────────────────────────────

void KnowledgeGraphReasoner::setLoraScoreFn(LoraScoreFn fn) {
    lora_score_fn_ = std::move(fn);
}

#if defined(THEMIS_ENABLE_LLM)
void KnowledgeGraphReasoner::setMultiLoRAManager(std::shared_ptr<llm::MultiLoRAManager> manager) {
    lora_manager_ = std::move(manager);
}
#endif

// ─────────────────────────────────────────────────────────────────────────────
// applyLoRAScore()
// ─────────────────────────────────────────────────────────────────────────────

void KnowledgeGraphReasoner::applyLoRAScore(InferenceChain &chain, std::string_view adapter_id) const {
    if (chain.edges.empty()) {
        return;
    }

    struct RuleLoRAConfig {
        double min_lora_score = 0.0;
        std::string adapter_id = {};
    };

    std::unordered_map<std::string, RuleLoRAConfig> rule_cfg_by_id;
    {
        std::shared_lock lock(rules_mutex_);
        rule_cfg_by_id.reserve(rules_.size());
        for (const auto &rule : rules_) {
            rule_cfg_by_id.emplace(rule.id, RuleLoRAConfig{rule.min_lora_score, rule.lora_adapter});
        }
    }

    const auto lookupRuleConfig = [&]([[maybe_unused]] const InferenceEdge &edge) -> const RuleLoRAConfig * {
        if (const auto it = rule_cfg_by_id.find(edge.rule_id); it != rule_cfg_by_id.end()) {
            return &it->second;
        }
        if (edge.rule_id.empty()) {
            const auto stored = inference_store_.get(edge.fact);
            if (stored.has_value() && !stored->rule_id.empty()) {
                if (const auto it = rule_cfg_by_id.find(stored->rule_id); it != rule_cfg_by_id.end()) {
                    return &it->second;
                }
            }
        }
        return nullptr;
    };

    const auto fallbackScore = [](const InferenceEdge &edge) {
        const std::size_t n = edge.premises.size();
        return 1.0 / static_cast<double>(1 + n);
    };
    const auto clampScore = []([[maybe_unused]] double score) {
        // Fail-closed hardening: malformed scorer outputs (NaN / +/-Inf) should
        // never bypass min_lora_score filters, therefore they normalize to 0.0.
        if (!std::isfinite(score)) {
            return 0.0;
        }
        return std::clamp(score, 0.0, 1.0);
    };

#if defined(THEMIS_ENABLE_LLM)
    // Avoid direct symbol coupling to optional MultiLoRAManager internals
    // across module boundaries. If no scorer callback is injected, we fall
    // back to config/fallback scoring below.
    const auto managerScore = [&](std::string_view adapter, const InferenceEdge &edge) -> std::optional<double> {
        (void)adapter;
        (void)edge;
        return std::nullopt;
    };
#endif

    for (auto &edge : chain.edges) {
        const RuleLoRAConfig *edge_cfg     = lookupRuleConfig(edge);
        std::string_view effective_adapter = adapter_id;
        if (effective_adapter.empty() && edge_cfg != nullptr && !edge_cfg->adapter_id.empty()) {
            effective_adapter = edge_cfg->adapter_id;
        }

        double score = fallbackScore(edge);
#if defined(THEMIS_ENABLE_LLM)
        if (lora_score_fn_ && !effective_adapter.empty()) {
            // Use injected backend (real LoRA/MultiLoRAManager bridge).
            score = lora_score_fn_(effective_adapter, edge);
        } else if (auto manager_score_result = managerScore(effective_adapter, edge);
                   manager_score_result.has_value()) {
            score = *manager_score_result;
        }
#endif
        edge.lora_score = clampScore(score);
    }

    // Filter out edges whose score falls below the rule's minimum threshold.
    chain.edges.erase(std::remove_if(chain.edges.begin(), chain.edges.end(),
                                     [&]([[maybe_unused]] const InferenceEdge &e) {
                                         const RuleLoRAConfig *cfg = lookupRuleConfig(e);
                                         if (cfg == nullptr) {
                                             return false;
                                         }
                                         if (e.lora_score < 0.0) {
                                             return false; // not scored
                                         }
                                         return e.lora_score < cfg->min_lora_score;
                                     }),
                      chain.edges.end());
}

// ─────────────────────────────────────────────────────────────────────────────
// setMaxHops()
// ─────────────────────────────────────────────────────────────────────────────

void KnowledgeGraphReasoner::setMaxHops([[maybe_unused]] int hops) noexcept {
    max_hops_ = std::max(1, std::min(hops, kHardMaxHops));
}

} // namespace graph
} // namespace themis
