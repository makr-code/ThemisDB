/**
 * @file knowledge_graph_reasoner.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief A (subject, predicate, object) triple — the atomic unit of knowledge.
 *
 * Both base facts and derived facts are represented as triples.
 * Variables in rule conditions start with '?' (e.g. "?A", "?B").
 */
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

/**
 * @brief A Horn-clause rule for forward-chaining inference.
 *
 * @par Syntax example
 * @code{.yaml}
 * id: transitive_reports_to
 * conditions:
 *   - [?A, reports_to, ?B]
 *   - [?B, reports_to, ?C]
 * conclusions:
 *   - [?A, indirectly_reports_to, ?C]
 * @endcode
 *
 * Variables start with '?'. Literals are plain strings.  The reasoner finds all
 * variable bindings that satisfy every condition simultaneously, then emits the
 * conclusions for each binding set.
 */
struct Rule {
    /// Unique identifier — must be non-empty; duplicates are silently overwritten.
    std::string id;
    /// Antecedents: triple patterns (may contain variables).
    std::vector<Triple> conditions;
    /// Consequents: triple patterns (same variables as conditions).
    std::vector<Triple> conclusions;
    /// Optional: name of a LoRA adapter that scores the derived edge.
    std::string lora_adapter;
    /// Minimum LoRA score required to emit the conclusion (0 ≤ score ≤ 1).
    double min_lora_score = 0.0;
};

/**
 * @brief A single derived edge in an inference chain, with full provenance.
 */
struct InferenceEdge {
    /// The derived (ground) triple.
    Triple fact;
    /// ID of the rule that derived this edge.
    std::string rule_id;
    /// The ground-fact premises used to satisfy the rule's conditions.
    std::vector<Triple> premises;
    /// Soft-plausibility score from a LoRA adapter; negative = not scored.
    double lora_score = -1.0;
};

/**
 * @brief Result of `KnowledgeGraphReasoner::infer()`.
 *
 * An ordered list of derived edges reachable from `subject_id` up to the
 * requested hop depth.  The list is in derivation order (breadth-first).
 */
struct InferenceChain {
    /// The starting subject for which inference was requested.
    std::string subject_id;
    /// All inferred edges, in BFS derivation order.
    std::vector<InferenceEdge> edges;

    /// Convenience: true when at least one edge was derived.
    [[nodiscard]] bool empty() const noexcept { return edges.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return edges.size(); }
};

/**
 * @brief Event from the Change-Data-Capture (CDC) pipeline.
 *
 * Delivered to `KnowledgeGraphReasoner::onCDCEvent()` to trigger incremental
 * forward-chaining without a full re-evaluation.
 */
struct CDCEvent {
    enum class Op : uint8_t {
        INSERT,  ///< A new edge was added.
        REMOVE   ///< An existing edge was removed.
    };
    Op op = Op::INSERT;
    /// The inserted or deleted edge as a ground triple.
    Triple edge;
};

// ─────────────────────────────────────────────────────────────────────────────
// InferenceStore
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief In-memory store for derived triples with TTL-based eviction.
 *
 * @par Design
 * - Bounded by `kMaxTriples`.  When full, the oldest entry (FIFO insertion
 *   order) is evicted before inserting a new one.
 * - Every entry carries an expiry timestamp; `evictExpired()` prunes stale
 *   entries lazily (called before each `store()` invocation).
 * - Thread-safe: all public methods are protected by `std::shared_mutex`.
 */
class InferenceStore {
public:
    /// Maximum number of derived triples stored simultaneously.
    static constexpr std::size_t kMaxTriples = 1'000'000;
    /// Default time-to-live for derived triples.
    static constexpr std::chrono::seconds kDefaultTTL{3600};

    InferenceStore() = default;

    // Non-copyable.
    InferenceStore(const InferenceStore&) = delete;
    InferenceStore& operator=(const InferenceStore&) = delete;

    /**
     * @brief Store a derived triple.
     *
     * If the key already exists the entry is refreshed. If the store is full,
     * the oldest entry is evicted first. `evictExpired()` is called before
     * each insertion.
     */
    void store(Triple fact, std::string rule_id, std::vector<Triple> premises,
               std::chrono::seconds ttl = kDefaultTTL);

    /// Returns true when the exact triple is present and not yet expired.
    [[nodiscard]] bool contains(const Triple& t) const;

    /// Retrieve the `InferenceEdge` for a specific triple; nullopt if absent/expired.
    [[nodiscard]] std::optional<InferenceEdge> get(const Triple& t) const;

    /**
     * @brief All derived edges whose `fact.subject == subject`.
     *
     * Expired entries are skipped but not removed (call `evictExpired()` to
     * purge them explicitly).
     */
    [[nodiscard]] std::vector<InferenceEdge> getDerived(std::string_view subject) const;

    /// Remove all expired entries.  O(n) in the number of stored triples.
    void evictExpired();

    /// Current number of stored (possibly expired) triples.
    [[nodiscard]] std::size_t size() const;

    /// Remove all entries.
    void clear();

private:
    struct Entry {
        InferenceEdge edge;
        std::chrono::steady_clock::time_point expires_at;
    };

    /// Build canonical map key: "subject\0predicate\0object".
    static std::string makeKey(const Triple& t);

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, Entry> entries_;
    std::list<std::string> insertion_order_; ///< front = oldest
};

// ─────────────────────────────────────────────────────────────────────────────
// KnowledgeGraphReasoner
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Horn-clause forward-chaining reasoner for knowledge graphs.
 *
 * @par Overview
 * Users register `Rule` objects (Horn clauses) and base-level `Triple` facts.
 * `infer(subjectId, depth)` runs BFS forward-chaining from all base facts up to
 * `depth` hops, collecting every new triple that can be derived by matching rule
 * conditions against the growing working set.
 *
 * `explain(fact)` reconstructs the proof chain that produced a specific derived
 * triple by walking the `InferenceStore` provenance links.
 *
 * `onCDCEvent(event)` performs incremental forward-chaining when a single new
 * edge arrives from the CDC pipeline, without re-evaluating the whole graph.
 *
 * @par Thread-safety
 * `infer()` and `explain()` are read-only and hold a shared lock on `facts_`
 * and `rules_`.  `addRule()`, `addFact()`, and `onCDCEvent()` take an exclusive
 * lock, so concurrent reads are safe during reasoning.
 *
 * @par LoRA integration
 * `applyLoRAScore()` is guarded by `THEMIS_ENABLE_LLM`.  When the flag is not
 * set the method is still declared but is a no-op, so callers never need `#ifdef`.
 *
 * @par Memory
 * Derived triples live in `InferenceStore` (max `InferenceStore::kMaxTriples`).
 * Base facts and rules are held in `std::vector`s.
 *
 * @par Example
 * @code{.cpp}
 * KnowledgeGraphReasoner kgr;
 *
 * // Add Horn clause: reports_to is transitive
 * kgr.addRule({ "transitive_reports_to",
 *               {{"?A","reports_to","?B"}, {"?B","reports_to","?C"}},
 *               {{"?A","indirectly_reports_to","?C"}} });
 *
 * // Add base facts
 * kgr.addFact({"alice", "reports_to", "bob"});
 * kgr.addFact({"bob",   "reports_to", "carol"});
 *
 * auto chain = kgr.infer("alice", 2);
 * // chain.edges contains {"alice","indirectly_reports_to","carol"}
 *
 * auto proof = kgr.explain({"alice","indirectly_reports_to","carol"});
 * // proof->premises == [ {"alice","reports_to","bob"},
 * //                      {"bob","reports_to","carol"} ]
 * @endcode
 */
class KnowledgeGraphReasoner {
public:
    /// Default maximum inference hops per `infer()` call.
    static constexpr int kDefaultMaxHops = 5;
    /// Hard upper cap (even if the caller requests more).
    inline static constexpr int kHardMaxHops = 20;

    // ── Life-cycle ──────────────────────────────────────────────────────────

    explicit KnowledgeGraphReasoner(int max_inference_hops = kDefaultMaxHops);
    ~KnowledgeGraphReasoner() noexcept = default;

    KnowledgeGraphReasoner(const KnowledgeGraphReasoner&) = delete;
    KnowledgeGraphReasoner& operator=(const KnowledgeGraphReasoner&) = delete;

    KnowledgeGraphReasoner(KnowledgeGraphReasoner&&) noexcept = default;
    KnowledgeGraphReasoner& operator=(KnowledgeGraphReasoner&&) noexcept = default;

    // ── Rule management ─────────────────────────────────────────────────────

    /**
     * @brief Add (or overwrite) a Horn-clause rule.
     *
     * Existing rules with the same `id` are replaced.
     * Rules with empty `id`, empty `conditions`, or empty `conclusions` are
     * rejected and the method returns `false`.
     *
     * @return true on success, false if the rule is malformed.
     */
    [[nodiscard]] bool addRule(Rule rule);

    /// Number of registered rules.
    [[nodiscard]] std::size_t ruleCount() const;

    /// Remove all rules.
    void clearRules();

    // ── Fact management ─────────────────────────────────────────────────────

    /**
     * @brief Add a ground-level base fact.
     *
     * Non-ground triples (containing '?' variables) are rejected.
     * Duplicate facts are silently ignored.
     */
    void addFact(Triple fact);

    /// Number of registered base facts.
    [[nodiscard]] std::size_t factCount() const;

    /// Remove all base facts (inference store is NOT cleared).
    void clearFacts();

    // ── Core reasoning ──────────────────────────────────────────────────────

    /**
     * @brief Run forward-chaining from all base facts and return an
     *        `InferenceChain` for @p subjectId.
     *
     * Applies all rules to the working set of known facts (base + previously
     * derived) up to @p depth iterations.  A depth of -1 uses `max_hops_`.
     * The result contains only edges whose `fact.subject == subjectId`.
     *
     * The `InferenceStore` is populated as a side-effect so that `explain()`
     * can reconstruct proof traces without re-running inference.
     *
     * @param subjectId  Starting entity (filter for the returned chain).
     * @param depth      Maximum hop count; clamped to `[1, kHardMaxHops]`.
     */
    [[nodiscard]] InferenceChain infer(std::string_view subjectId, int depth = -1) const;

    /**
     * @brief Reconstruct the proof trace for a derived triple.
     *
     * Walks the `InferenceStore` provenance links to produce an ordered list
     * of `InferenceEdge`s from the base facts up to the requested triple.
     *
     * Returns `nullopt` when @p fact is unknown (never derived or already
     * evicted from the `InferenceStore`).
     *
     * @par Complexity
     * O(premises) per level; typically ≤ 10 ms for depth ≤ kHardMaxHops.
     */
    [[nodiscard]] std::optional<InferenceEdge> explain(const Triple& fact) const;

    // ── Incremental CDC ─────────────────────────────────────────────────────

    /**
     * @brief Incremental forward-chaining triggered by a single CDC event.
     *
     * For `CDCEvent::Op::INSERT`: adds the edge to base facts and runs one
     * pass of all rules that involve the new triple, deriving any newly
     * entailed conclusions without a full re-evaluation.
     *
    * For `CDCEvent::Op::REMOVE`: removes the edge from base facts and clears
     * any derived triples whose premise set included the deleted edge.
     *
     * @note This method acquires an exclusive lock and may block concurrent
     *       `infer()` readers briefly.
     */
    void onCDCEvent(const CDCEvent& event);

    // ── LoRA scoring ────────────────────────────────────────────────────────

    /**
     * @brief Attach LoRA soft-plausibility scores to all edges in @p chain.
     *
     * When a `LoraScoreFn` has been injected via `setLoraScoreFn()`, that
     * function is called for each edge and its return value is stored in
     * `InferenceEdge::lora_score`.  Edges below the rule's `min_lora_score`
     * threshold are then filtered out.
     *
     * When no scorer callback is injected and `setMultiLoRAManager()` has been
     * configured (`THEMIS_ENABLE_LLM`), manager-backed adapter metadata is used
     * for soft plausibility scoring.
     *
     * Without an injected function the method falls back to a deterministic
     * heuristic: `score = 1 / (1 + premises.size())`.
     *
     * @param chain       The inference chain to annotate (modified in-place).
     * @param adapter_id  Identifier of the LoRA adapter to query.
     */
    void applyLoRAScore(InferenceChain& chain, std::string_view adapter_id) const;

    // ── LoRA score injection ─────────────────────────────────────────────────

    /// Signature: (adapter_id, edge) → plausibility score in [0.0, 1.0].
    using LoraScoreFn = std::function<double(std::string_view adapter_id,
                                             const InferenceEdge& edge)>;

    /// Inject a real LoRA scoring backend for `applyLoRAScore()`.
    /// Passing a null function resets to the built-in heuristic fallback.
    void setLoraScoreFn(LoraScoreFn fn);

#if defined(THEMIS_ENABLE_LLM)
    /// Inject optional MultiLoRAManager integration used by applyLoRAScore().
    void setMultiLoRAManager(std::shared_ptr<llm::MultiLoRAManager> manager);
#endif

    // ── Introspection ───────────────────────────────────────────────────────

    /// Read-only access to the inference store (derived triples).
    [[nodiscard]] const InferenceStore& inferenceStore() const noexcept {
        return inference_store_;
    }

    /// Set the default maximum hop count (clamped to [1, kHardMaxHops]).
    void setMaxHops(int hops) noexcept;

    /// Current default maximum hop count.
    [[nodiscard]] int maxHops() const noexcept { return max_hops_; }

    /// Total derived triples accumulated in the inference store.
    [[nodiscard]] std::size_t derivedTripleCount() const {
        return inference_store_.size();
    }

private:
    // ── Variable binding ────────────────────────────────────────────────────

    /// Variable-binding map: "?Var" → concrete string value.
    using Bindings = std::unordered_map<std::string, std::string>;

    /**
     * @brief Try to unify @p pattern with @p fact, extending @p bindings.
     *
     * Returns false if a variable is already bound to a different value.
     */
    [[nodiscard]] static bool unify(const Triple& pattern,
                                    const Triple& fact,
                                    Bindings& bindings);

    /// Apply @p bindings to @p pattern, producing a ground triple.
    [[nodiscard]] static Triple ground(const Triple& pattern,
                                       const Bindings& bindings);

    /**
     * @brief Recursively enumerate all binding sets that satisfy every
     *        condition in @p conditions[cond_idx..] given known @p facts.
     */
    static void matchConditions(const std::vector<Triple>& conditions,
                                std::size_t cond_idx,
                                const std::vector<Triple>& facts,
                                Bindings bindings,
                                std::vector<Bindings>& out);

    // ── Forward chaining internals ──────────────────────────────────────────

    /**
     * @brief Run one complete forward-chaining pass over @p working_set.
     *
     * @param working_set  All known facts (base + derived so far). Modified
     *                     in-place as new facts are derived.
     * @param derived_out  All newly derived `InferenceEdge`s are appended here.
     * @param max_depth    Iteration limit.
     */
    void forwardChain(std::vector<Triple>& working_set,
                      std::vector<InferenceEdge>& derived_out,
                      int max_depth) const;

    // ── Triple helpers ──────────────────────────────────────────────────────

    static std::string tripleKey(const Triple& t);

    // ── State ────────────────────────────────────────────────────────────────

    int max_hops_;

    mutable std::shared_mutex rules_mutex_;
    std::vector<Rule> rules_;

    mutable std::shared_mutex facts_mutex_;
    std::vector<Triple> base_facts_;

    mutable InferenceStore inference_store_;

    /// Injected LoRA scoring backend; null ⇒ heuristic fallback.
    mutable LoraScoreFn lora_score_fn_;

#if defined(THEMIS_ENABLE_LLM)
    /// Optional LoRA manager bridge used for adapter-aware plausibility scoring.
    std::shared_ptr<llm::MultiLoRAManager> lora_manager_;
#endif
};

} // namespace graph
} // namespace themis
