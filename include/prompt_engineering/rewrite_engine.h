/**
 * @file rewrite_engine.h
 * @brief RewriteEngine interface contract for prompt normalization and policy enforcement (Phase 1 design).
 * @version 1.0.0
 * @note Maturity: 🟡 DESIGN/CONTRACT
 * @note Status: Phase 1 frozen interface (Q3 2026)
 *
 * RewriteEngine is a deterministic rule-based transformation system for normalizing and
 * enforcing policy on prompt content. It operates in ordered phases:
 *
 * Phase 1: Input Normalization  (lexical rewriting, whitespace, encoding)
 * Phase 2: Policy Enforcement   (blocked patterns, safety rules, allow-list checks)
 * Phase 3: NL→AQL Preprocessing (semantic normalization, intent markers, schema hints)
 * Phase 4: Post-Generation      (structured output canonicalization, agent/tool normalization)
 *
 * This header defines the core interfaces. Implementation is Phase 2 work (Q4 2026).
 *
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>
#include "prompt_engineering_errors.h"

namespace themis {
namespace prompt_engineering {

/**
 * @enum RewritePhase
 * @brief Ordered execution phases for rewrite rules.
 *
 * Phases must execute in this order. No out-of-order or phase-skipping execution is allowed.
 */
enum class RewritePhase : uint8_t {
    PHASE_1_INPUT_NORMALIZATION = 1,    ///< Lexical/encoding normalization (YAML-configurable rules)
    PHASE_2_POLICY_ENFORCEMENT = 2,     ///< Safety and policy enforcement (compiled C++ rules)
    PHASE_3_NL_AQL_PREPROCESSING = 3,   ///< NL→AQL semantic preprocessing
    PHASE_4_POST_GENERATION = 4,        ///< Post-generation output canonicalization
};

/**
 * @enum RewriteRuleType
 * @brief Classification of rewrite rule safety and mutability.
 */
enum class RewriteRuleType : uint8_t {
    LEXICAL = 0,       ///< Safe lexical rule (regex/dictionary, no semantic knowledge required)
    SEMANTIC = 1,      ///< Semantic rule (requires NL understanding, compiled C++ only)
    POLICY_TERMINAL = 2, ///< Terminal rule: matched content is blocked and cannot be transformed
    POLICY_ALLOW_LIST = 3, ///< Allow-list rule: only matched patterns are permitted
};

/**
 * @struct RewriteTrace
 * @brief Audit trail of rewrite rule application for debugging and compliance.
 */
struct RewriteTrace {
    std::string rule_id;                ///< Identifier of the rule that matched
    RewritePhase phase{RewritePhase::PHASE_1_INPUT_NORMALIZATION}; ///< Phase in which rule was applied
    uint32_t match_count{0};            ///< Number of matches found by this rule
    uint64_t text_offset{0};            ///< Byte offset where first match occurred
    std::string matched_text;           ///< Original matched text (up to 1024 bytes)
    std::string replacement_text;       ///< Replacement text (up to 1024 bytes)
    uint64_t rule_latency_micros{0};    ///< Latency of rule execution in microseconds
    bool transformation_applied{false}; ///< Whether transformation was actually applied
    std::string safety_notes;           ///< Any safety or audit notes from rule execution
};

/**
 * @struct RewriteResult
 * @brief Structured result of a rewrite operation.
 */
struct RewriteResult {
    bool success = 0;                           ///< Whether rewrite completed without error
    PromptEngineeringErrorCode error_code; ///< Error code if success == false
    std::string error_message;              ///< Detailed error message if applicable
    std::string transformed_text;           ///< Output text after all rewrites (empty if error)
    std::vector<RewriteTrace> traces;      ///< Audit trail of rules that matched
    uint32_t rules_matched;                 ///< Number of unique rules that matched
    uint32_t total_transformations;         ///< Total number of text transformations applied
    uint64_t total_latency_micros;         ///< Total execution latency in microseconds
    bool was_blocked;                       ///< True if terminal rule blocked output
};

/**
 * @struct RewriteContext
 * @brief Execution context for rewrite operations.
 *
 * Carries configuration, phase information, and operational state during rewrite execution.
 */
struct RewriteContext {
    RewritePhase current_phase;             ///< Current execution phase
    uint32_t max_steps;                     ///< Maximum transformation steps (prevents infinite loops)
    uint32_t current_step;                  ///< Current step count
    uint64_t deadline_micros;               ///< Deadline (microseconds since epoch) for execution
    std::string environment_tag;            ///< Environment tag (prod/staging/test) for conditional rules
    bool allow_unsafe_transformations;     ///< Whether to apply higher-risk rules (policy-controlled)
    std::string user_id;                    ///< User ID for audit/permission checks (if applicable)
    bool trace_enabled;                     ///< Whether to collect detailed trace information
    uint32_t max_trace_entries;             ///< Maximum number of trace entries to retain

    RewriteContext()
        : current_phase(RewritePhase::PHASE_1_INPUT_NORMALIZATION),
          max_steps(1000),
          current_step(0),
          deadline_micros(0),
          allow_unsafe_transformations(false),
          trace_enabled(true),
          max_trace_entries(512)
    {
    }
};

/**
 * @struct RewriteDocument
 * @brief Input/output container for rewrite operations.
 *
 * Represents the document being rewritten and tracks metadata.
 */
struct RewriteDocument {
    std::string content;                    ///< Document content (input/output)
    std::string document_id;                ///< Unique document identifier
    std::string document_type;              ///< Type hint (prompt/template/query/etc)
    std::string source_language;            ///< Source language (en/de/etc or "auto")
    uint64_t size_bytes;                    ///< Document size in bytes
    bool is_multi_modal;                    ///< Whether document contains multi-modal content
    uint32_t language_code;                 ///< BCP 47 language code for content

    RewriteDocument() : size_bytes(0), is_multi_modal(false), language_code(0) {}
};

/**
 * @class IRewriteRule
 * @brief Base interface for rewrite rules.
 *
 * All rewrite rules inherit from this interface and implement deterministic matching and
 * transformation logic. Rules are registered globally and executed in priority order.
 *
 * THREAD-SAFE: All implementations must be thread-safe.
 * DETERMINISTIC: Given the same input, must always produce the same output.
 * BOUNDED: Must not introduce unbounded loops, exponential state, or pathological execution.
 */
class IRewriteRule {
public:
    virtual ~IRewriteRule() = default;

    /**
     * @brief Unique identifier for this rule.
     * @return Rule ID (must be stable across reloads)
     */
    virtual std::string rule_id() const = 0;

    /**
     * @brief Rule type (lexical/semantic/terminal/allow-list).
     * @return The rule type
     */
    virtual RewriteRuleType rule_type() const = 0;

    /**
     * @brief Execution phase for this rule.
     * @return The phase this rule executes in
     */
    virtual RewritePhase execution_phase() const = 0;

    /**
     * @brief Priority within phase (lower numbers execute first).
     * @return Priority value (0-255)
     *
     * If two rules have the same priority, they execute in registration order.
     */
    virtual uint8_t priority() const = 0;

    /**
     * @brief Check if this rule matches the given document.
     *
     * @param doc The document to check
     * @param ctx Execution context
     * @return True if rule matches, false otherwise
     *
     * This should be a fast, side-effect-free check.
     */
    virtual bool matches(const RewriteDocument& doc, const RewriteContext& ctx) const = 0;

    /**
     * @brief Apply rewrite transformation to the document.
     *
     * @param doc Input document (content may be modified)
     * @param ctx Execution context
     * @param[out] trace Audit trace of transformation
     * @return Result with transformed content or error
     *
     * If terminal rule returns true from matches(), apply() MUST populate
     * result.was_blocked = true and set appropriate error code.
     */
    virtual RewriteResult apply(
        RewriteDocument& doc,
        const RewriteContext& ctx,
        RewriteTrace& trace
    ) = 0;

    /**
     * @brief Check if transformation is idempotent (applying twice = applying once).
     * @return True if idempotent, false otherwise
     *
     * Non-idempotent rules are allowed but may be re-executed in edge cases.
     */
    virtual bool is_idempotent() const = 0;

    /**
     * @brief Get human-readable description of this rule.
     * @return Description for logging/debugging
     */
    virtual std::string description() const = 0;
};

/**
 * @class IRewriteEngine
 * @brief Main orchestration interface for rewrite operations.
 *
 * THREAD-SAFE: Rewrite execution is thread-safe.
 * All phase transitions are deterministic and reproducible.
 */
class IRewriteEngine {
public:
    virtual ~IRewriteEngine() = default;

    /**
     * @brief Register a rewrite rule for execution.
     *
     * @param rule Ownership transferred to engine
     * @return True if registration succeeded, false if rule_id already registered
     *
     * Rules cannot be re-registered; must unregister then re-register.
     * Rules are locked once execution begins.
     */
    virtual bool register_rule(std::shared_ptr<IRewriteRule> rule) = 0;

    /**
     * @brief Unregister a previously registered rule.
     *
     * @param rule_id ID of rule to remove
     * @return True if rule existed and was removed
     *
     * Cannot unregister while rewrite is in progress.
     */
    virtual bool unregister_rule(const std::string& rule_id) = 0;

    /**
     * @brief Get registered rule by ID.
     *
     * @param rule_id ID of rule to retrieve
     * @return Shared pointer to rule, or nullptr if not found
     */
    virtual std::shared_ptr<const IRewriteRule> get_rule(const std::string& rule_id) const = 0;

    /**
     * @brief List all registered rules.
     *
     * @return Vector of all registered rule IDs
     */
    virtual std::vector<std::string> list_rules() const = 0;

    /**
     * @brief Load rewrite rules from YAML configuration file.
     *
     * Only low-risk lexical rules (regex, dictionary) are loadable from YAML.
     * Semantic and policy rules must be registered programmatically in C++.
     *
     * @param yaml_path Path to YAML rule file
     * @return True if load succeeded
     *
     * If load fails, no rules are modified (all-or-nothing semantics).
     */
    virtual bool load_rules_from_yaml(const std::string& yaml_path) = 0;

    /**
     * @brief Execute all phases of rewrite on a document.
     *
     * Phases execute in strict order: 1 → 2 → 3 → 4.
     * Within each phase, rules execute by priority, then registration order.
     *
     * @param doc Input document (content will be modified in-place on success)
     * @param ctx Execution context
     * @return Result with transformed content or error
     *
     * If any phase returns an error or blocks the document, execution stops immediately.
     */
    virtual RewriteResult rewrite(RewriteDocument& doc, const RewriteContext& ctx) = 0;

    /**
     * @brief Execute a specific phase only.
     *
     * @param phase Which phase to execute
     * @param doc Input document (content will be modified in-place on success)
     * @param ctx Execution context
     * @return Result with transformed content or error
     *
     * For testing and diagnostic purposes only. Normal operation uses full rewrite().
     */
    virtual RewriteResult rewrite_phase(
        RewritePhase phase,
        RewriteDocument& doc,
        const RewriteContext& ctx
    ) = 0;

    /**
     * @brief Get execution statistics.
     *
     * @return JSON object with rule counts, total executions, latency stats
     */
    virtual std::string get_stats_json() const = 0;

    /**
     * @brief Reset all execution statistics.
     */
    virtual void reset_stats() = 0;
};

/**
 * @brief Factory function to create a RewriteEngine instance.
 *
 * @return Newly allocated engine (caller takes ownership)
 *
 * Allocator/deallocator must be same DLL/compilation unit for safety.
 */
std::unique_ptr<IRewriteEngine> create_rewrite_engine();

} // namespace prompt_engineering
} // namespace themis
