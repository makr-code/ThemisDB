/**
 * @file ai_operation_guard.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
 * @details Calls: std::getenv(), std::string().
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

enum class OperationClass : uint8_t {
    READ_ONLY   = 0,  ///< Safe read — execute immediately
    WRITE_SAFE  = 1,  ///< Single-record write — execute immediately
    DESTRUCTIVE = 2,  ///< Record deletion — requires approval
    CRITICAL    = 3,  ///< Full-scope delete / DDL — approval + prod role
};

/**
 * @brief Operation Class Name.
 * @param[in] c Input parameter.
 * @return Pointer to the result.
 * @note Exception safety: noexcept.
 */
const char* operationClassName(OperationClass c) noexcept;

// ---------------------------------------------------------------------------
// OperationPreview
// ---------------------------------------------------------------------------

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

class AiOperationGuard {
public:
    // ──────────────────────────────────────────────────────────────────────
    // Configuration
    // ──────────────────────────────────────────────────────────────────────

    struct Config {
        bool enabled = true;

        OperationClass approval_threshold = OperationClass::DESTRUCTIVE;

        int approval_timeout_s = 60;

        bool auto_snapshot = true;

        std::string snapshot_dir;

        bool dry_run_preview = true;

        // Environment Guard
        std::string              environment = "development";
        bool                     block_critical_in_prod = true;
        std::vector<std::string> denied_collections;   ///< Always hard-block
        std::vector<std::string> allowed_collections;  ///< Empty = all allowed
        std::string              critical_ops_role = "AI_DESTRUCTIVE_PRODUCTION_OPS";

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

    [[nodiscard]] GuardDecision evaluate(
        const std::string& tool_name,
        const json&        args,
        const std::string& ai_session_id,
        const std::string& caller_role = ""
    ) const;

    [[nodiscard]] json buildRequiresApprovalResponse(
        const GuardDecision& decision,
        std::chrono::system_clock::time_point now =
            std::chrono::system_clock::now()
    ) const;

    [[nodiscard]] json buildBlockedResponse(const GuardDecision& decision) const;

    // ──────────────────────────────────────────────────────────────────────
    // Accessors
    // ──────────────────────────────────────────────────────────────────────

    const Config& config() const noexcept { return config_; }

private:
    Config config_;

    // ── Classification helpers ─────────────────────────────────────────────

    [[nodiscard]] OperationClass classifyByTool(const std::string& tool_name) const noexcept;

    [[nodiscard]] OperationClass classifyAql(const std::string& aql_query) const noexcept;

    [[nodiscard]] OperationPreview buildPreview(
        const std::string& tool_name,
        const json&        args,
        OperationClass     op_class
    ) const;

    [[nodiscard]] std::string checkEnvironmentBlock(
        OperationClass     op_class,
        const std::string& target_collection,
        const std::string& caller_role
    ) const;

    [[nodiscard]] bool isCollectionDenied(const std::string& collection) const noexcept;

    /**
     * @brief Extract Collection.
     * @param[in] tool_name Name of the tool.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    static std::string extractCollection(
        const std::string& tool_name,
        const json&        args
    );

    /**
     * @brief To Iso8601.
     * @param[in] tp Input parameter.
     * @return Return value.
     */
    static std::string toIso8601(std::chrono::system_clock::time_point tp);
};

} // namespace security
} // namespace themis
