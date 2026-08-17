/**
 * @file delegate_evaluator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace themis::document {
class IRoundTripEditor;
}

namespace themis::rag::delegate_eval {

// ─────────────────────────────────────────────────────────────────────────────
// Domain types
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Supported document domains for round-trip evaluation.
 *
 * Add new domains by extending this enum and providing a corresponding
 * evaluator class registered in `DelegateEvaluatorFactory::createForDomain()`.
 */
enum class DomainType {
    JSON_DOCUMENT, ///< JSON object — field-level overlap scoring
    AQL_QUERY,     ///< AQL query string — token-level Jaccard scoring
    PLAIN_TEXT,    ///< Arbitrary UTF-8 text — character-level edit distance
    MARKDOWN,      ///< Markdown document — treated as plain text for scoring
    XML_PROCESS,   ///< XML/ARIS process model — element-count + attribute overlap
};

// ─────────────────────────────────────────────────────────────────────────────
// ReconstructionScore
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Scalar reconstruction fidelity in `[0.0, 1.0]`.
 *
 * 1.0 — perfectly reconstructed (original == recovered).
 * 0.0 — complete loss (nothing in common).
 */
using ReconstructionScore = double;

// ─────────────────────────────────────────────────────────────────────────────
// IDomainEvaluator
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Domain-specific reconstruction-score evaluator interface.
 *
 * Implementations compute how much of @p original survives in @p recovered
 * after one or more forward/backward agentic edits.  All methods are
 * logically `const`; state must not be mutated.
 *
 * ### Error / edge-case contracts
 *   - Both @p original and @p recovered empty → return 0.0 (no content to preserve).
 *   - @p original empty, @p recovered non-empty → return 0.0.
 *   - @p recovered empty, @p original non-empty → return 0.0.
 *   - Score always in `[0.0, 1.0]` — implementations must clamp.
 */
class IDomainEvaluator {
public:
    virtual ~IDomainEvaluator() = default;

    /**
     * @brief Compute the reconstruction score for a single round-trip result.
     *
     * @param original  The seed document before any agentic edits.
     * @param recovered The document after one or more forward/backward edits.
     * @return          RS in `[0.0, 1.0]`.
     */
    virtual ReconstructionScore evaluate(const std::string& original,
                                         const std::string& recovered) const = 0;

    /**
     * @brief Domain identifier for logging and diagnostics.
     */
    virtual DomainType domain() const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// RoundTripEditPair
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A paired forward + backward edit instruction for one round trip.
 *
 * The pair captures the two natural-language (or prompt-style) instructions
 * passed to the `EditFn` lambda:
 *   1. `forward_instruction` — transforms the document toward a target state.
 *   2. `backward_instruction` — attempts to undo the forward transformation.
 *
 * The `seed_document` field is informational metadata that identifies which
 * seed was used to derive this pair; it is **not** used by `RoundTripSimulator`
 * directly (the simulator operates on its own current-document state).
 */
struct RoundTripEditPair {
    std::string forward_instruction;   ///< Instruction for the forward edit
    std::string backward_instruction;  ///< Instruction for the backward edit
    std::string seed_document;         ///< Informational: origin seed identifier
    DomainType  domain_type;           ///< Target domain for score computation
};

// ─────────────────────────────────────────────────────────────────────────────
// EditFn
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Caller-supplied editing function.
 *
 * The function receives:
 *   - @p current_doc — the document in its current state.
 *   - @p instruction — the edit instruction (forward or backward).
 *
 * It must return the edited document as a string.  Throwing an exception is
 * treated as a failed edit: `RoundTripSimulator` records RS = 0.0 for that
 * iteration and terminates the relay with `StopReason::EDIT_FAILED`.
 *
 * **Thread safety:** the lambda is invoked sequentially inside `run()`; no
 * concurrent calls to the same lambda occur.
 */
using EditFn = std::function<std::string(const std::string& current_doc,
                                          const std::string& instruction)>;

// ─────────────────────────────────────────────────────────────────────────────
// StopReason
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Reason the `RoundTripSimulator` relay terminated.
 */
enum class StopReason {
    COMPLETED_NORMALLY, ///< All requested round trips executed successfully
    EDIT_FAILED,        ///< `EditFn` threw an exception during a forward or backward edit
    BUDGET_EXCEEDED,    ///< `DelegateEvaluatorConfig::max_interactions` hit (future use)
};

// ─────────────────────────────────────────────────────────────────────────────
// DelegateEvaluatorConfig
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Configuration for `RoundTripSimulator`.
 */
struct DelegateEvaluatorConfig {
    /// Number of complete forward→backward round trips to execute (default: 10).
    size_t num_round_trips = 10;

    /// RS below this threshold counts as a "catastrophic" corruption event
    /// (default: 0.80, matching DELEGATE-52 paper criterion).
    double catastrophic_threshold = 0.80;

    /// RS at or above this value is considered a "success" (default: 0.98).
    double success_threshold = 0.98;
};

// ─────────────────────────────────────────────────────────────────────────────
// ReconstructionScoreAtK
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief RS\@k aggregation over all interactions in a relay.
 *
 * Mirrors the DELEGATE-52 evaluation protocol where RS is sampled at
 * interaction indices k = 1, 5, 10, 20 (two-sided: forward = odd,
 * backward = even in the raw interaction sequence).
 *
 * In ThemisDB's model each element of `rs_per_interaction` stores the RS
 * against the original seed *after* that numbered interaction (1-based).
 * So `rs_at(k)` simply retrieves element [k-1] when `k ≤ size`.
 */
struct ReconstructionScoreAtK {
    /// RS sampled after each interaction (1-based; index i → interaction i+1).
    std::vector<ReconstructionScore> rs_per_interaction;

    /**
     * @brief RS after interaction @p k (1-based).
     *
     * @param k  Interaction index starting at 1.
     * @return   RS at that position, or 1.0 if k == 0 (no edit), or
     *           the last available score if k exceeds the recorded history.
     * @note     Return value must not be discarded; the function silently
     *           clamps out-of-range k to the last available index.
     * @note     No exception is thrown for out-of-range k.
     */
    [[nodiscard]] ReconstructionScore rs_at(size_t k) const noexcept {
        if (k == 0) return 1.0;
        if (rs_per_interaction.empty()) return 1.0;
        const size_t idx = std::min(k, rs_per_interaction.size()) - 1;
        return rs_per_interaction[idx];
    }

    /**
     * @brief True when any RS in the history is below the catastrophic threshold.
     *
     * @param threshold  Catastrophic threshold (default matches paper: 0.80).
     */
    bool hasCatastrophicEvent(double threshold = 0.80) const noexcept {
        return std::any_of(rs_per_interaction.begin(), rs_per_interaction.end(),
                           [threshold](double s) { return s < threshold; });
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// RelayResult
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Result of a complete `RoundTripSimulator::run()` relay.
 */
struct RelayResult {
    /// RS history per interaction (see `ReconstructionScoreAtK`).
    ReconstructionScoreAtK scores;

    /// Number of interactions whose RS fell below `catastrophic_threshold`.
    size_t catastrophic_corruption_count = 0;

    /// True when **every** interaction was catastrophic (total content loss).
    bool fully_catastrophic = false;

    /// The document in its final state after all edits.
    std::string final_doc;

    /// Reason the relay terminated.
    StopReason stop_reason = StopReason::COMPLETED_NORMALLY;

    /// Total number of individual forward/backward edit calls executed.
    size_t total_interactions = 0;

    /// Number of snapshot persistence writes that failed in best-effort mode.
    size_t persistence_write_failures = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// RoundTripSimulator
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Orchestrates forward→backward round-trip edits and measures RS\@k.
 *
 * For each round trip, the simulator:
 *   1. Calls `edit_fn(current_doc, pair.forward_instruction)` → `forward_doc`.
 *   2. Calls `edit_fn(forward_doc,  pair.backward_instruction)` → `backward_doc`.
 *   3. Evaluates `evaluator.evaluate(seed_doc, backward_doc)` → RS.
 *   4. Appends RS to `RelayResult::scores.rs_per_interaction`.
 *
 * An exception from `edit_fn` records RS = 0.0 and sets
 * `StopReason::EDIT_FAILED`.
 *
 * **Thread safety:** not shared; create one instance per concurrent request.
 *
 * @see AgenticRAG for the analogous iterative-retrieval design.
 */
class RoundTripSimulator {
public:
    /**
     * @brief Construct with default configuration.
     */
    RoundTripSimulator();

    /**
     * @brief Construct with custom configuration.
     * @param config  Simulator configuration.
     */
    explicit RoundTripSimulator(const DelegateEvaluatorConfig& config);

    ~RoundTripSimulator();

    // Non-copyable, movable
    RoundTripSimulator(const RoundTripSimulator&)            = delete;
    RoundTripSimulator& operator=(const RoundTripSimulator&) = delete;
    RoundTripSimulator(RoundTripSimulator&&)                 noexcept = default;
    RoundTripSimulator& operator=(RoundTripSimulator&&)      noexcept = default;

    // ─────────────────────────────────────────────────────────────────────────
    // Primary entry point
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Run the round-trip relay.
     *
     * Executes `config.num_round_trips` complete forward→backward cycles using
     * the provided `edit_pairs` (cycling if there are fewer pairs than rounds).
     * When `num_round_trips == 0` the function returns immediately with
     * RS\@0 = 1.0 and `StopReason::COMPLETED_NORMALLY`.
     *
     * @param seed_doc    Original document before any edits.
     * @param edit_pairs  Ordered list of edit pair instructions.  Must not be
     *                    empty unless `num_round_trips == 0`.
     * @param evaluator   Domain-specific scorer used after each backward edit.
     * @param edit_fn     Callable that applies a single edit instruction to a
     *                    document.  Must be non-null.
     * @return            `RelayResult` containing full RS history and metadata.
     *
     * @throws std::invalid_argument  If @p edit_pairs is empty and
     *                                `num_round_trips > 0`, or if @p edit_fn
     *                                is null.
     */
    RelayResult run(const std::string&           seed_doc,
                    const std::vector<RoundTripEditPair>& edit_pairs,
                    const IDomainEvaluator&      evaluator,
                    EditFn                       edit_fn);

    /**
     * @brief Return the current configuration.
     */
    DelegateEvaluatorConfig getConfig() const;

    /**
     * @brief Replace the current configuration.
     * @param config  New configuration.
     */
    void setConfig(const DelegateEvaluatorConfig& config);

    /**
     * @brief Attach an optional round-trip persistence backend.
     *
     * When set, `run()` stores the seed snapshot and every interaction snapshot
     * through this editor. Ownership remains with the caller.
     *
     * @param editor Non-owning pointer (may be nullptr to disable persistence).
     */
    void setRoundTripEditor(themis::document::IRoundTripEditor* editor) noexcept;

    /**
     * @brief Return the currently attached persistence backend.
     */
    [[nodiscard]] themis::document::IRoundTripEditor* getRoundTripEditor() const noexcept;

    /**
     * @brief Last relay id generated by `run()`.
     *
     * @return Empty string before the first run.
     */
    [[nodiscard]] const std::string& getLastRelayId() const noexcept;

private:
    DelegateEvaluatorConfig config_;
    themis::document::IRoundTripEditor* round_trip_editor_{nullptr};
    std::string last_relay_id_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Concrete evaluators
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief JSON document evaluator using field-level overlap.
 *
 * RS = `(total_fields − changed_fields) / total_fields`.
 *
 * "Changed" includes added, removed, and value-modified top-level keys.
 * If the input is not valid JSON, it falls back to `PlainTextEvaluator`.
 *
 * @note Only top-level keys are compared; nested object diffing is left to
 *       future versions.
 */
class JsonDocumentEvaluator final : public IDomainEvaluator {
public:
    JsonDocumentEvaluator() = default;

    /**
     * @brief Evaluate JSON field-level fidelity.
     * @param original  The original JSON string.
     * @param recovered The recovered JSON string.
     * @return RS in `[0.0, 1.0]`.
     */
    ReconstructionScore evaluate(const std::string& original,
                                  const std::string& recovered) const override;

    DomainType domain() const override { return DomainType::JSON_DOCUMENT; }
};

/**
 * @brief AQL query evaluator using token-level Jaccard similarity.
 *
 * RS = |tokens(original) ∩ tokens(recovered)| / |tokens(original) ∪ tokens(recovered)|
 *
 * Tokenisation splits on whitespace and punctuation, preserving AQL keywords,
 * identifiers, and string literals as individual tokens.
 */
class AqlQueryEvaluator final : public IDomainEvaluator {
public:
    AqlQueryEvaluator() = default;

    /**
     * @brief Evaluate AQL query fidelity via Jaccard similarity.
     * @param original  The original AQL string.
     * @param recovered The recovered AQL string.
     * @return RS in `[0.0, 1.0]`.
     * @note  **Empty-input convention:** when both @p original and @p recovered
     *        tokenise to the empty set (e.g. whitespace/punctuation-only input),
     *        this function returns `0.0`.  This intentionally deviates from the
     *        strict set-theory definition where Jaccard(∅, ∅) = 1.0: an empty
     *        token set carries no lexical content, so treating it as "perfect
     *        reconstruction" would be semantically misleading for the RS metric.
     */
    ReconstructionScore evaluate(const std::string& original,
                                  const std::string& recovered) const override;

    DomainType domain() const override { return DomainType::AQL_QUERY; }
};

/**
 * @brief Plain-text evaluator using normalised character-level edit distance.
 *
 * RS = 1 − edit_distance(original, recovered) / max(len(original), len(recovered))
 *
 * Uses the standard Levenshtein distance.  Efficient for documents up to
 * ~100 KB; larger inputs should use the XML or JSON domain evaluators.
 */
class PlainTextEvaluator final : public IDomainEvaluator {
public:
    PlainTextEvaluator() = default;

    /**
     * @brief Evaluate plain-text fidelity via normalised edit distance.
     * @param original  The original text.
     * @param recovered The recovered text.
     * @return RS in `[0.0, 1.0]`.
     * @note  **Large-string approximation:** for inputs exceeding 10 000
     *        characters the underlying `editDistance()` switches from the
     *        exact O(n×m) Levenshtein DP to an O(n) Hamming-style approximation
     *        (aligned-prefix character differences + length delta).  This keeps
     *        RS computation under ~5 ms for 100 KB inputs at the cost of
     *        reduced accuracy when the edit distance is large relative to the
     *        string length.  For document payloads beyond this size, consider
     *        using `XmlProcessEvaluator` or `JsonDocumentEvaluator` instead.
     */
    ReconstructionScore evaluate(const std::string& original,
                                  const std::string& recovered) const override;

    DomainType domain() const override { return DomainType::PLAIN_TEXT; }
};

/**
 * @brief XML process-model evaluator using element-count and attribute overlap.
 *
 * RS = 0.6 × (element_overlap) + 0.4 × (attribute_overlap)
 *
 * Element overlap: fraction of element names from @p original present in
 * @p recovered.  Attribute overlap: fraction of `key="value"` pairs preserved.
 * When @p original has no attributes, the attribute component scores 1.0
 * (no attributes to lose, so they do not penalise the overall RS).
 *
 * Designed for ARIS/BPMN process XML as used in `src/process/`.  Non-XML
 * input falls back to `PlainTextEvaluator`.
 */
class XmlProcessEvaluator final : public IDomainEvaluator {
public:
    XmlProcessEvaluator() = default;

    /**
     * @brief Evaluate XML process-model fidelity.
     * @param original  The original XML string.
     * @param recovered The recovered XML string.
     * @return RS in `[0.0, 1.0]`.
     */
    ReconstructionScore evaluate(const std::string& original,
                                  const std::string& recovered) const override;

    DomainType domain() const override { return DomainType::XML_PROCESS; }
};

/**
 * @brief Markdown evaluator (delegates to `PlainTextEvaluator`).
 *
 * Markdown structure is preserved through plain-text edit-distance; syntax
 * markers (headings, bullets, code fences) contribute to the character pool.
 */
class MarkdownEvaluator final : public IDomainEvaluator {
public:
    MarkdownEvaluator() = default;

    /**
     * @brief Evaluate Markdown fidelity via plain-text edit distance.
     * @param original  The original Markdown string.
     * @param recovered The recovered Markdown string.
     * @return RS in `[0.0, 1.0]`.
     */
    ReconstructionScore evaluate(const std::string& original,
                                  const std::string& recovered) const override;

    DomainType domain() const override { return DomainType::MARKDOWN; }

private:
    PlainTextEvaluator text_eval_;
};

// ─────────────────────────────────────────────────────────────────────────────
// DelegateEvaluatorFactory
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Factory for `IDomainEvaluator` instances keyed by `DomainType`.
 *
 * All returned objects are heap-allocated via `std::unique_ptr` with no
 * external library dependencies.
 */
class DelegateEvaluatorFactory {
public:
    /**
     * @brief Create the appropriate evaluator for the given domain.
     *
     * @param domain  Target document domain.
     * @return        A non-null `unique_ptr` to the matching evaluator.
     *                The return value must not be discarded.
     *
     * @throws std::invalid_argument  If an unknown `DomainType` value is passed.
     */
    [[nodiscard]] static std::unique_ptr<IDomainEvaluator> createForDomain(DomainType domain);

    /**
     * @brief Create a `RoundTripSimulator` with the given configuration.
     *
     * @param config  Simulator configuration (default: `DelegateEvaluatorConfig{}`).
     */
    static std::unique_ptr<RoundTripSimulator> createSimulator(
        const DelegateEvaluatorConfig& config = {});
};

} // namespace themis::rag::delegate_eval
