/**
 * @file aql_safety_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// AI Safety Layer — Schicht 3: AQL Read-Only Enforcer
//
// AqlSafetyValidator scans a raw AQL query string for mutation operations
// BEFORE the query is handed to the query engine. It is a fast, deterministic,
// token-based filter with no external dependencies and p99 < 0.1 ms for
// queries up to 64 KB.
//
// Activated when a tool spec carries `enforce_read_only: true` (e.g. the
// `aql_execute` MCP tool when called from the `agentic` LLM mode).
//
// Full documentation: docs/de/security/ai_safety/AI_SAFETY_AQL_VALIDATOR.md
// Roadmap:            src/security/ROADMAP.md § Phase 5 (ASL-3)

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace themis {
namespace query {

/**
 * @brief AQL mutation detector — AI Safety Layer, Schicht 3.
 *
 * Scans a raw AQL query string for write/mutating keywords before execution.
 * Designed to be called from McpServer::toolQuery() when the tool is flagged
 * as `enforce_read_only: true` (e.g., the `aql_execute` tool in agentic mode).
 *
 * ### Detection strategy
 * Keyword-based scan on an uppercase copy of the query.  The validator
 * deliberately errs on the side of false positives (safe rejection) over
 * false negatives (missed mutations).
 *
 * ### Detected patterns
 * | Category | Keywords / Patterns |
 * |---|---|
 * | DML mutations | `INSERT `, `UPDATE `, `REPLACE `, `UPSERT `, `REMOVE ` |
 * | AQL FOR-REMOVE | `FOR ` + ` REMOVE ` anywhere in the same query |
 * | DDL | `DROP `, `TRUNCATE `, `CREATE COLLECTION` |
 *
 * ### Thread safety
 * `AqlSafetyValidator` is stateless and safe for concurrent use.
 *
 * ### Mutations-allowed mode (EPIC-004)
 * Construct with `ValidationMode::AllowMutations` to disable keyword
 * blocking while retaining injection-pattern checks.  This is intended for
 * contexts where DML is explicitly permitted (e.g. the `aql_mutate` MCP
 * tool).  The default `ReadOnly` mode preserves backward-compatible behaviour.
 */
class AqlSafetyValidator {
public:
    /// @brief Validation policy controlling whether DML keywords are blocked.
    enum class ValidationMode {
        ReadOnly,        ///< Default — block all mutation keywords (existing behaviour)
        AllowMutations,  ///< Permit DML (INSERT/UPDATE/REMOVE/REPLACE/UPSERT/DELETE)
    };

    /// Violation details returned when a mutation keyword is found.
    struct Violation {
        /// The mutation keyword that triggered the rejection (e.g. "REMOVE").
        std::string keyword;
        /// Zero-based byte offset of the first occurrence in the original query.
        std::size_t position;
        /// Human-readable error message suitable for returning to the MCP client.
        std::string message;
    };

    /**
     * @brief Construct with an explicit validation mode.
     *
     * @param mode  `ReadOnly` (default) blocks all mutation keywords.
     *              `AllowMutations` permits DML and only checks for injection.
     */
    explicit AqlSafetyValidator(ValidationMode mode = ValidationMode::ReadOnly)
        : mode_(mode) {}

    ~AqlSafetyValidator()                                    = default;
    AqlSafetyValidator(const AqlSafetyValidator&)            = default;
    AqlSafetyValidator& operator=(const AqlSafetyValidator&) = default;

    /**
     * @brief Validate that @p aql_query contains no mutation operations.
     *
     * @param aql_query  Raw AQL query string.
     * @return `std::nullopt` when the query is read-only (safe to execute).
     *         A `Violation` describing the first detected mutation otherwise.
     */
    [[nodiscard]] std::optional<Violation> validate(std::string_view aql_query) const;

    /**
     * @brief Validate mutation safety even when AllowMutations mode is active.
     *
     * Checks injection patterns and unsafe unbounded-update/delete patterns
     * that are dangerous regardless of whether mutations are permitted:
     * - Embedded NUL characters (`\\0`) — classic injection vector
     * - Multi-statement injection patterns (`;  DROP `, `; DELETE `, `; UPDATE `)
     * - UPDATE or REMOVE without any FILTER/WHERE — could affect entire collection
     * - Suspiciously large LIMIT values > 100000 that could indicate bulk-delete attacks
     *
     * This is called from validate() when mode is AllowMutations so that injection
     * protection is never fully disabled.
     *
     * @param aql_query  Raw AQL query string to inspect.
     * @return A @c Violation describing the first concern found, or
     *         @c std::nullopt if no safety issue is detected.
     */
    [[nodiscard]] std::optional<Violation> validateMutationSafety(
        std::string_view aql_query) const;

    /**
     * @brief Convenience wrapper: returns true when @p aql_query is safe
     *        (contains no mutation keywords).
     */
    [[nodiscard]] bool isSafe(std::string_view aql_query) const {
        return !validate(aql_query).has_value();
    }

private:
    ValidationMode mode_ = ValidationMode::ReadOnly;

    /// Translate @p s to uppercase in-place (ASCII only, no locale).
    static std::string toUpper(const std::string& s);

    /// Find @p needle in @p haystack, return npos if not found.
    static std::size_t findKeyword(const std::string& haystack,
                                   std::string_view   needle);
};

} // namespace query
} // namespace themis
