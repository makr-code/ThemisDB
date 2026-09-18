/**
 * @file knowledge_graph_reasoner.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>
#include <vector>
#include "themis/export.h"
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <shared_mutex>
#include <optional>
#include <chrono>
#include <functional>
#include <atomic>

namespace themis {

#if defined(THEMIS_ENABLE_LLM)
namespace llm {
class MultiLoRAManager;
} // namespace llm
#endif

namespace graph {

// ─────────────────────────────────────────────────────────────────────────────
// Core data structures
// ─────────────────────────────────────────────────────────────────────────────

struct Triple {
    std::string subject = {};
    std::string predicate = {};
    std::string object = {};

    [[nodiscard]] bool operator==(const Triple& o) const noexcept {
        return subject == o.subject && predicate == o.predicate && object == o.object;
    }
    [[nodiscard]] bool operator!=(const Triple& o) const noexcept { return !(*this == o); }
    [[nodiscard]] bool isGround() const noexcept {
        return ((subject.empty()   || subject[0]   != '?') &&
                (predicate.empty() || predicate[0] != '?') &&
                (object.empty()    || object[0]    != '?'));
    }
};

struct Rule {
    std::string id;
    std::vector<Triple> conditions;
    std::vector<Triple> conclusions;
    std::string lora_adapter;
    double min_lora_score = 0.0;
};

struct InferenceEdge {
    Triple fact;
    std::string rule_id;
    std::vector<Triple> premises;
    double lora_score = -1.0;
};

struct InferenceChain {
    std::string subject_id;
    std::vector<InferenceEdge> edges;

    [[nodiscard]] bool empty() const noexcept { return edges.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return edges.size(); }
};

struct CDCEvent {
    enum class Op : uint8_t {
        INSERT,  ///< A new edge was added.
        REMOVE   ///< An existing edge was removed.
    };
    Op op = Op::INSERT;
    Triple edge;
};

// ─────────────────────────────────────────────────────────────────────────────
// InferenceStore
// ─────────────────────────────────────────────────────────────────────────────

class InferenceStore {
public:
    static constexpr std::size_t kMaxTriples = 1'000'000;
    static constexpr std::chrono::seconds kDefaultTTL{3600};

    InferenceStore() = default;

    // Non-copyable.
    InferenceStore(const InferenceStore&) = delete;
    InferenceStore& operator=(const InferenceStore&) = delete;

    void store(Triple fact, std::string rule_id, std::vector<Triple> premises,
               std::chrono::seconds ttl = kDefaultTTL);

    [[nodiscard]] bool contains(const Triple& t) const;

    [[nodiscard]] std::optional<InferenceEdge> get(const Triple& t) const;

    [[nodiscard]] std::vector<InferenceEdge> getDerived(std::string_view subject) const;

    /**
     * @brief Evict Expired.
     */
    void evictExpired();

    [[nodiscard]] std::size_t size() const;

    /**
     * @brief Clear.
     */
    void clear();

private:
    struct Entry {
        InferenceEdge edge;
        std::chrono::steady_clock::time_point expires_at;
    };

    /**
     * @brief Make Key.
     * @param[in] t Input parameter.
     * @return Return value.
     */
    static std::string makeKey(const Triple& t);

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, Entry> entries_;
    std::list<std::string> insertion_order_; ///< front = oldest
};

// ─────────────────────────────────────────────────────────────────────────────
// KnowledgeGraphReasoner
// ─────────────────────────────────────────────────────────────────────────────

class KnowledgeGraphReasoner {
public:
    static constexpr int kDefaultMaxHops = 5;
    inline static constexpr int kHardMaxHops = 20;

    // ── Life-cycle ──────────────────────────────────────────────────────────

    explicit KnowledgeGraphReasoner(int max_inference_hops = kDefaultMaxHops);
    ~KnowledgeGraphReasoner() noexcept = default;

    KnowledgeGraphReasoner(const KnowledgeGraphReasoner&) = delete;
    KnowledgeGraphReasoner& operator=(const KnowledgeGraphReasoner&) = delete;

    KnowledgeGraphReasoner(KnowledgeGraphReasoner&&) noexcept = default;
    KnowledgeGraphReasoner& operator=(KnowledgeGraphReasoner&&) noexcept = default;

    // ── Rule management ─────────────────────────────────────────────────────

    [[nodiscard]] bool addRule(Rule rule);

    [[nodiscard]] std::size_t ruleCount() const;

    /**
     * @brief Clear Rules.
     */
    void clearRules();

    /**
     * @brief ── Fact management ─────────────────────────────────────────────────────
     * @param[in] fact Input parameter.
     */

    void addFact(Triple fact);

    [[nodiscard]] std::size_t factCount() const;

    /**
     * @brief Clear Facts.
     */
    void clearFacts();

    // ── Core reasoning ──────────────────────────────────────────────────────

    [[nodiscard]] InferenceChain infer(std::string_view subjectId, int depth = -1) const;

    [[nodiscard]] std::optional<InferenceEdge> explain(const Triple& fact) const;

    /**
     * @brief ── Incremental CDC ─────────────────────────────────────────────────────
     * @param[in] event Input parameter.
     */

    void onCDCEvent(const CDCEvent& event);

    /**
     * @brief ── LoRA scoring ────────────────────────────────────────────────────────
     * @param[in,out] chain Input/output parameter.
     * @param[in] adapter_id Identifier of the adapter.
     */

    void applyLoRAScore(InferenceChain& chain, std::string_view adapter_id) const;

    // ── LoRA score injection ─────────────────────────────────────────────────

    using LoraScoreFn = std::function<double(std::string_view adapter_id,
                                             const InferenceEdge& edge)>;

    /**
     * @brief Set Lora Score Fn.
     * @param[in] fn Input parameter.
     */
    void setLoraScoreFn(LoraScoreFn fn);

#if defined(THEMIS_ENABLE_LLM)
    /**
     * @brief Set Multi Lo RAManager.
     * @param[in] manager Input parameter.
     */
    void setMultiLoRAManager(std::shared_ptr<llm::MultiLoRAManager> manager);
#endif

    // ── Introspection ───────────────────────────────────────────────────────

    [[nodiscard]] const InferenceStore& inferenceStore() const noexcept {
        return inference_store_;
    }

    /**
     * @brief Set Max Hops.
     * @param[in] hops Input parameter.
     * @note Exception safety: noexcept.
     */
    void setMaxHops(int hops) noexcept;

    [[nodiscard]] int maxHops() const noexcept { return max_hops_; }

    [[nodiscard]] std::size_t derivedTripleCount() const {
        return inference_store_.size();
    }

private:
    // ── Variable binding ────────────────────────────────────────────────────

    using Bindings = std::unordered_map<std::string, std::string>;

    [[nodiscard]] static bool unify(const Triple& pattern,
                                    const Triple& fact,
                                    Bindings& bindings);

    [[nodiscard]] static Triple ground(const Triple& pattern,
                                       const Bindings& bindings);

    /**
     * @brief Match Conditions.
     * @param[in] conditions Input parameter.
     * @param[in] cond_idx Input parameter.
     * @param[in] facts Input parameter.
     * @param[in] bindings Input parameter.
     * @param[in,out] out Input/output parameter.
     */
    static void matchConditions(const std::vector<Triple>& conditions,
                                std::size_t cond_idx,
                                const std::vector<Triple>& facts,
                                Bindings bindings,
                                std::vector<Bindings>& out);

    /**
     * @brief ── Forward chaining internals ──────────────────────────────────────────
     * @param[in,out] working_set Input/output parameter.
     * @param[in,out] derived_out Input/output parameter.
     * @param[in] max_depth Input parameter.
     */

    void forwardChain(std::vector<Triple>& working_set,
                      std::vector<InferenceEdge>& derived_out,
                      int max_depth) const;

    /**
     * @brief ── Triple helpers ──────────────────────────────────────────────────────
     * @param[in] t Input parameter.
     * @return Return value.
     */

    static std::string tripleKey(const Triple& t);

    // ── State ────────────────────────────────────────────────────────────────

    int max_hops_;

    mutable std::shared_mutex rules_mutex_;
    std::vector<Rule> rules_;

    mutable std::shared_mutex facts_mutex_;
    std::vector<Triple> base_facts_;

    mutable InferenceStore inference_store_;

    mutable LoraScoreFn lora_score_fn_;

#if defined(THEMIS_ENABLE_LLM)
    std::shared_ptr<llm::MultiLoRAManager> lora_manager_;
#endif
};

} // namespace graph
} // namespace themis
