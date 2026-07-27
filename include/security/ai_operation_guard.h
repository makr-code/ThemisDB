/**
 * @file ai_operation_guard.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: ai_operation_guard.h | Version: 1.0.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// AI Safety Layer — Schichten 1 & 2: Destructive Operation Guard (DOG) + HILG
//
// AiOperationGuard classifies every AI-initiated MCP tool call by its
// destructive potential BEFORE the operation is executed.  It is the
// primary gate that the McpServer consults for any mutating tool.
//
// Full documentation:
//   docs/de/security/ai_safety/AI_SAFETY_OPERATION_GUARD.md
// Roadmap:
//   src/security/ROADMAP.md § Phase 2 (ASL-4)

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// ---------------------------------------------------------------------------
// Platform-portable snapshot directory default
// ---------------------------------------------------------------------------

/**
 * @brief Returns the platform-portable default directory for AI pre-operation
 *        snapshots.
 *
 * - Windows: `%PROGRAMDATA%\themis\ai-snapshots` (via `PROGRAMDATA` env var,
 *            falls back to `C:\ProgramData\themis\ai-snapshots`)
 * - Other:   `/var/themis/ai-snapshots`
 *
 * @return Absolute path string suitable for use as `Config::snapshot_dir`.
 */
inline std::string themisDefaultSnapshotDir() {
#ifdef _WIN32
    if (const char* pd = std::getenv("PROGRAMDATA"); pd && *pd) {
        return std::string(pd) + "\\themis\\ai-snapshots";
    }
    return "C:\\ProgramData\\themis\\ai-snapshots";
#else
    return "/var/themis/ai-snapshots";
#endif
}

namespace themis {
namespace security {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// OperationClass
// ---------------------------------------------------------------------------

/**
 * @brief Destruktionspotenzial einer KI-initiierten Datenbankoperation.
 *
 * Values increase with risk so that comparisons like
 * `op_class >= OperationClass::DESTRUCTIVE` are valid.
 */
enum class OperationClass : uint8_t {
    READ_ONLY   = 0,  ///< Safe read — execute immediately
    WRITE_SAFE  = 1,  ///< Single-record write — execute immediately
    DESTRUCTIVE = 2,  ///< Record deletion — requires approval
    CRITICAL    = 3,  ///< Full-scope delete / DDL — approval + prod role
};

/// Human-readable name for an OperationClass value.
const char* operationClassName(OperationClass c) noexcept;

// ---------------------------------------------------------------------------
// OperationPreview
// ---------------------------------------------------------------------------

/**
 * @brief Human-readable preview of a classified operation.
 *
 * Returned inside `GuardDecision` and serialised in the MCP
 * `"requires_approval"` response so that the operator can make an
 * informed decision.
 */
struct OperationPreview {
    std::string tool_name;          ///< MCP tool name (e.g. "delete_entity")
    std::string description;        ///< Human-readable summary
    json        args;               ///< Sanitised args (no credentials)
    std::string aql_query;          ///< Only set when tool_name == "query"
    std::string target_collection;  ///< Affected collection name
    uint64_t    estimated_affected; ///< Estimated affected records (0 = unknown)
};

// ---------------------------------------------------------------------------
// GuardDecision
// ---------------------------------------------------------------------------

/**
 * @brief Return value of `AiOperationGuard::evaluate()`.
 *
 * When `requires_approval` is true, the McpServer stores this decision in
 * the `pending_approvals_` map (HILG) and returns a
 * `"requires_approval"` response to the AI agent.
 *
 * When `block_reason` is non-empty, the operation is hard-blocked (no
 * approval possible) and `requires_approval` is false.
 */
struct GuardDecision {
    OperationClass   op_class;
    OperationPreview preview;
    bool             requires_approval;  ///< = op_class >= approval_threshold
    std::string      operation_id;       ///< UUID for HILG tracking
    std::string      block_reason;       ///< Non-empty → hard-block; no approval
};

// ---------------------------------------------------------------------------
// AiOperationGuard
// ---------------------------------------------------------------------------

/**
 * @brief Destructive Operation Guard (DOG) — AI Safety Layer, Schichten 1 & 2.
 *
 * Classifies each AI-initiated MCP tool call by its destructive potential
 * **before** execution and optionally enforces hard blocks for CRITICAL
 * operations in production environments.
 *
 * ### Classification algorithm
 * 1. Tool-name-based classification for entity/index tools.
 * 2. AQL content-based classification for the "query" tool.
 * 3. Environment Guard: blocks CRITICAL ops in production unless the caller
 *    holds the `critical_ops_role`.
 *
 * ### Thread safety
 * `AiOperationGuard` is stateless after construction; safe for concurrent use.
 */
class AiOperationGuard {
public:
    // ──────────────────────────────────────────────────────────────────────
    // Configuration
    // ──────────────────────────────────────────────────────────────────────

    struct Config {
        /// Enable/disable the guard globally.  When false, evaluate() always
        /// returns READ_ONLY without any blocking.
        bool enabled = true;

        /// Minimum OperationClass that requires human approval.
        OperationClass approval_threshold = OperationClass::DESTRUCTIVE;

        /// Seconds until a pending approval entry expires.
        int approval_timeout_s = 60;

        /// Whether to add `"auto_snapshot": true` to the approval response.
        bool auto_snapshot = true;

        /// Directory for pre-operation snapshots (informational in response).
        std::string snapshot_dir;

        /// When true, return a dry-run preview in the approval response.
        /// Loaded from the agentic mode's safety.dry_run_preview (ASL-7).
        bool dry_run_preview = true;

        // Environment Guard
        std::string              environment = "development";
        bool                     block_critical_in_prod = true;
        std::vector<std::string> denied_collections;   ///< Always hard-block
        std::vector<std::string> allowed_collections;  ///< Empty = all allowed
        /// Role required for CRITICAL ops in production.
        std::string              critical_ops_role = "AI_DESTRUCTIVE_PRODUCTION_OPS";

        /// Default constructor initializes snapshot_dir with platform-portable path.
        Config() : snapshot_dir(themisDefaultSnapshotDir()) {}
    };

    // ──────────────────────────────────────────────────────────────────────
    // Construction
    // ──────────────────────────────────────────────────────────────────────

    explicit AiOperationGuard(Config cfg = {});
    ~AiOperationGuard()                              = default;
    AiOperationGuard(const AiOperationGuard&)            = delete;
    AiOperationGuard& operator=(const AiOperationGuard&) = delete;

    // ──────────────────────────────────────────────────────────────────────
    // Core API
    // ──────────────────────────────────────────────────────────────────────

    /**
     * @brief Classify an AI-initiated tool call and produce a GuardDecision.
     *
     * @param tool_name      MCP tool name (e.g. "query", "delete_entity").
     * @param args           Tool arguments JSON.
     * @param ai_session_id  Session identifier (for audit).
     * @param caller_role    Caller's role string (empty = no special role).
     * @return A `GuardDecision` with classification and optional operation_id.
     */
    [[nodiscard]] GuardDecision evaluate(
        const std::string& tool_name,
        const json&        args,
        const std::string& ai_session_id,
        const std::string& caller_role = ""
    ) const;

    /**
     * @brief Build the MCP JSON response for a `requires_approval` decision.
     *
     * @param decision  A GuardDecision with `requires_approval == true`.
     * @param now       Timestamp for `expires_at` calculation (injectable for tests).
     * @return JSON object following the MCP requires_approval format.
     */
    [[nodiscard]] json buildRequiresApprovalResponse(
        const GuardDecision& decision,
        std::chrono::system_clock::time_point now =
            std::chrono::system_clock::now()
    ) const;

    /**
     * @brief Build the MCP JSON response for a hard-blocked operation.
     */
    [[nodiscard]] json buildBlockedResponse(const GuardDecision& decision) const;

    // ──────────────────────────────────────────────────────────────────────
    // Accessors
    // ──────────────────────────────────────────────────────────────────────

    const Config& config() const noexcept { return config_; }

private:
    Config config_;

    // ── Classification helpers ─────────────────────────────────────────────

    /// Classify based on tool name (entity / index tools).
    [[nodiscard]] OperationClass classifyByTool(const std::string& tool_name) const noexcept;

    /// Classify an AQL query string (used when tool_name == "query").
    [[nodiscard]] OperationClass classifyAql(const std::string& aql_query) const noexcept;

    /// Build a sanitised OperationPreview from tool call inputs.
    [[nodiscard]] OperationPreview buildPreview(
        const std::string& tool_name,
        const json&        args,
        OperationClass     op_class
    ) const;

    /// Check Environment Guard: returns non-empty reason if hard-block applies.
    [[nodiscard]] std::string checkEnvironmentBlock(
        OperationClass     op_class,
        const std::string& target_collection,
        const std::string& caller_role
    ) const;

    /// Return true if @p collection is in denied_collections or not in
    /// allowed_collections (when the list is non-empty).
    [[nodiscard]] bool isCollectionDenied(const std::string& collection) const noexcept;

    /// Extract target collection name from args (best-effort).
    static std::string extractCollection(
        const std::string& tool_name,
        const json&        args
    );

    /// Convert a system_clock time_point to ISO 8601 string.
    static std::string toIso8601(std::chrono::system_clock::time_point tp);
};

} // namespace security
} // namespace themis
