/**
 * @file ai_operation_guard.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// AI Safety Layer — Schichten 1 & 2: Destructive Operation Guard (DOG)
// Full documentation:
//   docs/de/security/ai_safety/AI_SAFETY_OPERATION_GUARD.md
// Roadmap:
//   src/security/ROADMAP.md § Phase 2 (ASL-4)

#include "security/ai_operation_guard.h"
#include "utils/uuid.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <ctime>
#include <fmt/format.h>
#include <iomanip>
#include <sstream>

namespace themis::security {

// ---------------------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------------------

const char* operationClassName(OperationClass c) noexcept {
    switch (c) {
        case OperationClass::READ_ONLY:   return "READ_ONLY";
        case OperationClass::WRITE_SAFE:  return "WRITE_SAFE";
        case OperationClass::DESTRUCTIVE: return "DESTRUCTIVE";
        case OperationClass::CRITICAL:    return "CRITICAL";
        default:                          return "UNKNOWN";
    }
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/// Worst-case estimated affected rows for a CRITICAL (full-scope) AQL operation.
/// Used in operation previews when no query plan is available.
constexpr uint64_t k_critical_op_max_affected = 9'999'999;

/// Case-insensitive uppercase conversion.
std::string toUpper(const std::string& s) {
    std::string out = {};
    out.reserve(s.size());
    for (unsigned char c : s) {
        out.push_back(static_cast<char>(std::toupper(c)));
    }
    return out;
}

/// System collections that must never be touched by AI agents.
constexpr std::array<std::string_view, 8> k_system_collections = {
    "_system", "_graphs", "_analyzers", "_jobs",
    "_users", "_queues", "_wal", "_snapshots"
};

bool isSystemCollection(const std::string& col) {
    const std::string lower = [&] {
        std::string s = col;
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) {
                           return static_cast<char>(std::tolower(c));
                       });
        return s;
    }();
    return std::any_of(k_system_collections.begin(), k_system_collections.end(),
                       [&]([[maybe_unused]] std::string_view sc) { return lower == sc; });
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

AiOperationGuard::AiOperationGuard(Config cfg)
    : config_(std::move(cfg)) {}

// ---------------------------------------------------------------------------
// evaluate()
// ---------------------------------------------------------------------------

GuardDecision AiOperationGuard::evaluate(
    const std::string& tool_name,
    const json&        args,
    const std::string& ai_session_id,
    const std::string& caller_role
) const {
    (void)ai_session_id;

    // If the guard is disabled, pass everything through as READ_ONLY
    // (i.e., no blocking, no approval).
    if (!config_.enabled) {
        OperationPreview preview;
        preview.tool_name   = tool_name;
        preview.description = fmt::format("Guard disabled — tool '{}' passes through", tool_name);
        preview.args        = args;
        return GuardDecision{
            OperationClass::READ_ONLY,
            std::move(preview),
            /*requires_approval=*/false,
            /*operation_id=*/"",
            /*block_reason=*/""
        };
    }

    // --- Step 1: Classify by tool name (and AQL content for "query") -------
    OperationClass op_class = OperationClass::READ_ONLY;
    if (tool_name == "query") {
        const std::string aql = args.value("query", "");
        op_class = classifyAql(aql);
    } else {
        op_class = classifyByTool(tool_name);
    }

    // --- Step 2: Determine target collection --------------------------------
    const std::string collection = extractCollection(tool_name, args);

    // Escalate to CRITICAL for system collections
    if (!collection.empty() && isSystemCollection(collection)) {
        op_class = OperationClass::CRITICAL;
    }

    // --- Step 3: Build preview ----------------------------------------------
    OperationPreview preview = buildPreview(tool_name, args, op_class);

    // --- Step 4: Environment Guard ------------------------------------------
    const std::string block_reason =
        checkEnvironmentBlock(op_class, collection, caller_role);
    if (!block_reason.empty()) {
        // Hard-block: no approval possible
        return GuardDecision{
            op_class,
            std::move(preview),
            /*requires_approval=*/false,
            /*operation_id=*/"",
            block_reason
        };
    }

    // --- Step 5: Check approval threshold -----------------------------------
    const bool needs_approval = (op_class >= config_.approval_threshold);
    std::string operation_id = {};
    if (needs_approval) {
        // Generate a unique, time-ordered operation ID with "op-" prefix.
        operation_id = "op-" + utils::generate_uuid_v7();
    }

    return GuardDecision{
        op_class,
        std::move(preview),
        needs_approval,
        std::move(operation_id),
        /*block_reason=*/""
    };
}

// ---------------------------------------------------------------------------
// buildRequiresApprovalResponse()
// ---------------------------------------------------------------------------

json AiOperationGuard::buildRequiresApprovalResponse(
    const GuardDecision&                  decision,
    std::chrono::system_clock::time_point now
) const {
    const auto expires_tp =
        now + std::chrono::seconds(config_.approval_timeout_s);

    // data_loss_risk heuristic
    const char* data_loss = "NONE";
    if (decision.op_class == OperationClass::DESTRUCTIVE) {
        data_loss = "LOW";
    } else if (decision.op_class == OperationClass::CRITICAL) {
        data_loss = "HIGH";
    }

    return {
        {"status",          "requires_approval"},
        {"operation_id",    decision.operation_id},
        {"classification",  operationClassName(decision.op_class)},
        {"tool",            decision.preview.tool_name},
        {"preview", {
            {"description",        decision.preview.description},
            {"target_collection",  decision.preview.target_collection},
            {"estimated_affected", decision.preview.estimated_affected},
            {"args",               decision.preview.args},
            {"aql_query",          decision.preview.aql_query}
        }},
        {"impact_estimate", {
            {"data_loss_risk",  data_loss},
            {"reversible",      true},
            {"auto_snapshot",   config_.auto_snapshot},
            {"snapshot_dir",    config_.snapshot_dir}
        }},
        {"expires_at",  toIso8601(expires_tp)},
        {"approve_url",
         fmt::format("/v1/ai/approve/{}", decision.operation_id)}
    };
}

// ---------------------------------------------------------------------------
// buildBlockedResponse()
// ---------------------------------------------------------------------------

json AiOperationGuard::buildBlockedResponse(const GuardDecision& decision) const {
    return {
        {"status",         "blocked"},
        {"reason",         decision.block_reason},
        {"classification", operationClassName(decision.op_class)},
        {"operation",      decision.preview.tool_name},
        {"environment",    config_.environment},
        {"contact",        "dba-team@example.com"}
    };
}

// ---------------------------------------------------------------------------
// classifyByTool()
// ---------------------------------------------------------------------------

OperationClass AiOperationGuard::classifyByTool(
    const std::string& tool_name
) const noexcept {
    if (tool_name == "get_entity"   ||
        tool_name == "get_schema"   ||
        tool_name == "get_stats"    ||
        tool_name == "list_indexes" ||
        tool_name == "get_error_info" ||
        tool_name == "search_errors") {
        return OperationClass::READ_ONLY;
    }
    if (tool_name == "put_entity"   ||
        tool_name == "create_index") {
        return OperationClass::WRITE_SAFE;
    }
    if (tool_name == "delete_entity" ||
        tool_name == "drop_index") {
        return OperationClass::DESTRUCTIVE;
    }
    // Unknown tool: treat as WRITE_SAFE (conservative but not blocking)
    return OperationClass::WRITE_SAFE;
}

// ---------------------------------------------------------------------------
// classifyAql()
// ---------------------------------------------------------------------------

OperationClass AiOperationGuard::classifyAql(
    const std::string& aql_query
) const noexcept {
    if (aql_query.empty()) {
        return OperationClass::READ_ONLY;
    }

    const std::string uq = toUpper(aql_query);

    // --- CRITICAL patterns -------------------------------------------------
    // Full-collection delete: FOR x IN col REMOVE x IN col (no FILTER)
    {
        const std::size_t forPos    = uq.find("FOR ");
        const std::size_t removePos = uq.find(" REMOVE ");
        if (forPos != std::string::npos && removePos != std::string::npos &&
            forPos < removePos) {
            const std::size_t filterPos = uq.find("FILTER ");
            const bool hasFilter = (filterPos != std::string::npos &&
                                    filterPos > forPos &&
                                    filterPos < removePos);
            if (!hasFilter) {
                return OperationClass::CRITICAL;
            }
            // Filtered REMOVE → still DESTRUCTIVE (not CRITICAL)
            return OperationClass::DESTRUCTIVE;
        }
    }
    if (uq.find("DROP COLLECTION") != std::string::npos) {
        return OperationClass::CRITICAL;
    }
    if (uq.find("TRUNCATE ") != std::string::npos) {
        return OperationClass::CRITICAL;
    }

    // --- DESTRUCTIVE patterns ----------------------------------------------
    if (uq.find("REMOVE ") != std::string::npos) {
        return OperationClass::DESTRUCTIVE;
    }
    if (uq.find("DROP ") != std::string::npos) {
        return OperationClass::DESTRUCTIVE;
    }

    // --- WRITE_SAFE patterns -----------------------------------------------
    if (uq.find("INSERT ")  != std::string::npos ||
        uq.find("UPDATE ")  != std::string::npos ||
        uq.find("REPLACE ") != std::string::npos ||
        uq.find("UPSERT ")  != std::string::npos) {
        return OperationClass::WRITE_SAFE;
    }

    return OperationClass::READ_ONLY;
}

// ---------------------------------------------------------------------------
// buildPreview()
// ---------------------------------------------------------------------------

OperationPreview AiOperationGuard::buildPreview(
    const std::string& tool_name,
    const json&        args,
    OperationClass     op_class
) const {
    OperationPreview p;
    p.tool_name   = tool_name;
    p.args        = args;
    p.estimated_affected = 0;

    const std::string collection = extractCollection(tool_name, args);
    p.target_collection = collection;

    if (tool_name == "query") {
        p.aql_query   = args.value("query", "");
        const char* riskLabel = operationClassName(op_class);
        p.description = fmt::format(
            "AQL-Query klassifiziert als {} — Ziel-Collection: '{}'",
            riskLabel, collection.empty() ? "(unbekannt)" : collection);
        if (op_class == OperationClass::CRITICAL) {
            p.estimated_affected = k_critical_op_max_affected;
        } else if (op_class == OperationClass::DESTRUCTIVE) {
            p.estimated_affected = 1;
        }
    } else if (tool_name == "delete_entity") {
        const std::string key = args.value("key", "");
        p.description = fmt::format(
            "Löscht Datensatz mit Key '{}' aus Collection '{}'",
            key, collection.empty() ? "(unbekannt)" : collection);
        p.estimated_affected = 1;
    } else if (tool_name == "drop_index") {
        const std::string table  = args.value("table", "");
        const std::string column = args.value("column", "");
        p.description = fmt::format(
            "Löscht Index auf '{}'.'{}'", table, column);
        p.estimated_affected = 0;
    } else {
        p.description = fmt::format(
            "Tool '{}' mit OperationClass {}", tool_name,
            operationClassName(op_class));
    }

    return p;
}

// ---------------------------------------------------------------------------
// checkEnvironmentBlock()
// ---------------------------------------------------------------------------

std::string AiOperationGuard::checkEnvironmentBlock(
    OperationClass     op_class,
    const std::string& target_collection,
    const std::string& caller_role
) const {
    // Check denied_collections
    if (!target_collection.empty() && isCollectionDenied(target_collection)) {
        return fmt::format(
            "Collection '{}' is on the denied list and cannot be modified by AI agents.",
            target_collection);
    }

    // CRITICAL in production: hard-block unless caller has the required role
    if (op_class == OperationClass::CRITICAL &&
        config_.block_critical_in_prod &&
        config_.environment == "production" &&
        caller_role != config_.critical_ops_role) {
        return fmt::format(
            "CRITICAL operations in production environment require role '{}'. "
            "Current role: '{}'.",
            config_.critical_ops_role,
            caller_role.empty() ? "(none)" : caller_role);
    }

    return {};
}

// ---------------------------------------------------------------------------
// isCollectionDenied()
// ---------------------------------------------------------------------------

bool AiOperationGuard::isCollectionDenied(
    const std::string& collection
) const noexcept {
    for (const auto& denied : config_.denied_collections) {
        if (denied == collection) { return true; }
    }
    if (!config_.allowed_collections.empty()) {
        const bool found =
            std::find(config_.allowed_collections.begin(),
                      config_.allowed_collections.end(),
                      collection) != config_.allowed_collections.end();
        if (!found) { return true; }
    }
    return false;
}

// ---------------------------------------------------------------------------
// extractCollection()
// ---------------------------------------------------------------------------

// static
std::string AiOperationGuard::extractCollection(
    const std::string& tool_name,
    const json&        args
) {
    if (tool_name == "delete_entity" || tool_name == "get_entity" ||
        tool_name == "put_entity") {
        const std::string key = args.value("key", "");
        // Key may be in the form "collection:doc_id"
        const auto sep = key.find(':');
        if (sep != std::string::npos) {
            return key.substr(0, sep);
        }
        // Fallback: explicit "collection" field (e.g. {"collection":"users","key":"id"})
        return args.value("collection", "");
    }
    if (tool_name == "create_index" || tool_name == "drop_index" ||
        tool_name == "list_indexes") {
        return args.value("table", "");
    }
    if (tool_name == "query") {
        // Best-effort: parse "IN <collection>" from upper-case AQL
        const std::string uq = [&] {
            std::string s = args.value("query", "");
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c) { return std::toupper(c); });
            return s;
        }();
        const auto pos = uq.find(" IN ");
        if (pos != std::string::npos) {
            // Skip the " IN " and take the next word
            std::size_t start = pos + 4;
            while (start < uq.size() && uq[start] == ' ') { ++start; }
            std::size_t end = start;
            while (end < uq.size() && (std::isalnum(static_cast<unsigned char>(uq[end])) || uq[end] == '_')) {
                ++end;
            }
            if (end > start) {
                // Return lowercase version
                std::string col = args.value("query", "").substr(start, end - start);
                std::transform(col.begin(), col.end(), col.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                return col;
            }
        }
        return {};
    }
    return {};
}

// ---------------------------------------------------------------------------
// toIso8601()
// ---------------------------------------------------------------------------

// static
std::string AiOperationGuard::toIso8601(
    std::chrono::system_clock::time_point tp
) {
    const std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_utc{};
#if defined(_WIN32)
    gmtime_s(&tm_utc, &t);
#else
    gmtime_r(&t, &tm_utc);
#endif
    std::ostringstream oss = {};
    oss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

} // namespace themis::security
