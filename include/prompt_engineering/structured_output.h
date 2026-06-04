/**
 * @file structured_output.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>

namespace themis {
namespace prompt_engineering {

// ── Constraint types ──────────────────────────────────────────────────────────

/**
 * @brief JSON-schema constraint: required-field list + strict-mode flag.
 *
 * The enforcer parses `schema_json` for a top-level `"required"` array and
 * verifies that all listed keys appear in the output object.  In
 * `strict_mode`, keys present in the output but **not** listed under
 * `"properties"` are rejected.
 */
struct JsonSchemaConstraint {
    std::string schema_json;  ///< Inline JSON schema (subset: required/properties).
    bool strict_mode = true;  ///< Reject unknown top-level keys if true.
    int  max_retries = 3;     ///< Maximum repair+validate attempts.
};

/**
 * @brief Regex grammar constraint applied to the (optionally stripped) output.
 */
struct RegexGrammarConstraint {
    std::string pattern;       ///< ECMAScript-compatible regex pattern.
    bool full_match  = true;   ///< true → std::regex_match; false → std::regex_search.
    int  max_tokens  = 512;    ///< Soft cap: warn if output exceeds this token estimate.
};

enum class OutputConstraintType {
    JSON_SCHEMA, ///< Enforce JSON validity + schema.
    REGEX,       ///< Enforce regex match.
    NONE,        ///< No constraint; always passes.
};

// ── Configuration ─────────────────────────────────────────────────────────────

/**
 * @brief Configuration for a single structured-output enforcement pass.
 */
struct StructuredOutputConfig {
    OutputConstraintType   type           = OutputConstraintType::NONE;
    JsonSchemaConstraint   json_schema;
    RegexGrammarConstraint regex_grammar;
    bool repair_json    = true;  ///< Apply JSON repair pipeline before validation.
    bool strip_markdown = true;  ///< Strip Markdown fences before any validation.
};

// ── Result ────────────────────────────────────────────────────────────────────

/**
 * @brief Result of a structured-output enforcement pass.
 */
struct StructuredOutputResult {
    std::string              raw_output;         ///< Original, unmodified output.
    std::string              validated_output;   ///< Repaired / processed output.
    bool                     is_valid       = false;
    std::vector<std::string> validation_errors;  ///< Non-empty when !is_valid.
    int                      attempts_used  = 1;
    double                   total_latency_ms = 0.0;
};

// ── Abstract interface ────────────────────────────────────────────────────────

/**
 * @brief Abstract interface for structured LLM output enforcement.
 */
class IStructuredOutputEnforcer {
public:
    virtual ~IStructuredOutputEnforcer() = default;

    /**
     * @brief Enforce the constraint on @p raw_output.
     *
     * Applies repair (if configured) and validates against the declared
     * constraint type.  Retries up to `JsonSchemaConstraint::max_retries`
     * times for JSON_SCHEMA mode.
     */
    virtual StructuredOutputResult enforce(
        const std::string&           raw_output,
        const StructuredOutputConfig& config) = 0;

    /**
     * @brief Validate @p output against @p config; populate @p errors.
     *
     * @return `true` iff validation passes (errors is empty).
     */
    virtual bool validate(const std::string&           output,
                          const StructuredOutputConfig& config,
                          std::vector<std::string>&     errors) = 0;
};

// ── Concrete implementation ───────────────────────────────────────────────────

/**
 * @brief Production implementation of `IStructuredOutputEnforcer`.
 *
 * All parsing is done with the C++ standard library only (no external JSON
 * library dependency); the JSON validator handles single-level objects and
 * arrays sufficient for the prompt-engineering pipeline.
 *
 * Thread safety: stateless — all methods are `const`-compatible; the concrete
 * class holds no mutable state and is safe to share across threads.
 */
class StructuredOutputEnforcer final : public IStructuredOutputEnforcer {
public:
    StructuredOutputResult enforce(
        const std::string&           raw_output,
        const StructuredOutputConfig& config) override;

    bool validate(const std::string&           output,
                  const StructuredOutputConfig& config,
                  std::vector<std::string>&     errors) override;

private:
    // ── Repair helpers ───────────────────────────────────────────────────────

    /// Strip ``` ```json … ``` ``` fences from @p text.
    static std::string stripMarkdownFences(const std::string& text);

    /// Remove trailing commas before `}` or `]`.
    static std::string removeTrailingCommas(const std::string& text);

    /// Strip `// …` line comments.
    static std::string stripLineComments(const std::string& text);

    /// Run the full repair pipeline in sequence.
    static std::string repairJson(const std::string& text);

    // ── JSON validation helpers ──────────────────────────────────────────────

    /// Check that @p text is structurally valid JSON (balanced braces/brackets,
    /// basic string quoting).  Populates @p errors on failure.
    static bool checkJsonStructure(const std::string&       text,
                                   std::vector<std::string>& errors);

    /// Extract the string values of a JSON array at top-level key @p key.
    /// Returns empty vector if the key is absent.
    static std::vector<std::string> extractStringArray(
        const std::string& json, const std::string& key);

    /// Extract the property names from a `"properties": { … }` block.
    static std::vector<std::string> extractPropertyNames(const std::string& schema);

    /// Extract all top-level key names from a flat JSON object string.
    static std::vector<std::string> extractTopLevelKeys(const std::string& json);

    /// Validate JSON output against the schema constraint.
    static bool validateJsonSchema(const std::string&         output,
                                   const JsonSchemaConstraint& schema,
                                   std::vector<std::string>&   errors);

    // ── Regex validation helper ──────────────────────────────────────────────

    /// Validate output against the regex grammar constraint.
    static bool validateRegex(const std::string&              output,
                               const RegexGrammarConstraint&  grammar,
                               std::vector<std::string>&       errors);
};

} // namespace prompt_engineering
} // namespace themis
